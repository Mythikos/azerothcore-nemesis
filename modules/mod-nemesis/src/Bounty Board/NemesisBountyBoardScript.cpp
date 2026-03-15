#include "NemesisBountyBoardConstants.h"
#include "NemesisBountyBoardManager.h"
#include "NemesisConstants.h"
#include "NemesisRevengeConstants.h"
#include "NemesisRevengeManager.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "ScriptedGossip.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "StringFormat.h"
#include <ctime>

// Shared send function — used by both the gossip handler and the chat command.
// Sends the full @@NBB@@ protocol: header, leaderboard entries, personal section, footer.
void SendBountyBoardData(Player* player)
{
	if (!player)
		return;

	ChatHandler handler(player->GetSession());
	uint32_t serverTime = static_cast<uint32_t>(time(nullptr));
	uint32_t playerGuid = player->GetGUID().GetCounter();

	// ── Leaderboard query ───────────────────────────────────────────
	QueryResult eliteResult = WorldDatabase.Query("SELECT elite_id, name, effective_level, threat_score, kill_count, origin_traits, earned_traits, last_zone_id, last_map_id, last_pos_x, last_pos_y, last_pos_z, last_seen_at, promoted_at, umbral_moon_origin, origin_player_guid FROM nemesis_elite WHERE is_alive = 1 ORDER BY threat_score DESC LIMIT {}", sBountyBoardMgr->GetConfig().topEliteCount);

	QueryResult countResult = WorldDatabase.Query("SELECT COUNT(*) FROM nemesis_elite WHERE is_alive = 1");

	uint32_t livingTotal = 0;
	if (countResult)
		livingTotal = countResult->Fetch()[0].Get<uint32_t>();

	uint32_t eliteCount = 0;
	if (eliteResult)
		eliteCount = static_cast<uint32_t>(eliteResult->GetRowCount());

	// ── Umbral Moon section ─────────────────────────────────────────
	// UH: version~serverTime~scheduleDays~scheduleTimes~durationMinutes
	handler.SendSysMessage(Acore::StringFormat("{}UH~{}~{}~{}~{}~{}", NemesisBountyBoardConstants::MSG_PREFIX, NemesisBountyBoardConstants::PROTOCOL_VERSION, serverTime, sBountyBoardMgr->GetConfig().scheduleDays, sBountyBoardMgr->GetConfig().scheduleTimes, sBountyBoardMgr->GetConfig().durationMinutes));

	// ── Leaderboard header ──────────────────────────────────────────
	// LH: eliteCount~livingTotal
	handler.SendSysMessage(Acore::StringFormat("{}LH~{}~{}", NemesisBountyBoardConstants::MSG_PREFIX, eliteCount, livingTotal));

	// ── Leaderboard entries ─────────────────────────────────────────
	if (eliteResult)
	{
		uint32_t rank = 0;
		do
		{
			++rank;
			Field* fields = eliteResult->Fetch();

			std::string name         = fields[1].Get<std::string>();
			uint32_t level           = fields[2].Get<uint32_t>();
			uint32_t threatScore     = fields[3].Get<uint32_t>();
			uint32_t killCount       = fields[4].Get<uint32_t>();
			uint32_t originTraits    = fields[5].Get<uint32_t>();
			uint32_t earnedTraits    = fields[6].Get<uint32_t>();
			uint32_t zoneId          = fields[7].Get<uint32_t>();
			uint32_t lastMapId       = fields[8].Get<uint32_t>();
			float lastPosX           = fields[9].Get<float>();
			float lastPosY           = fields[10].Get<float>();
			float lastPosZ           = fields[11].Get<float>();
			uint32_t lastSeenAt      = fields[12].Get<uint32_t>();
			uint32_t promotedAt      = fields[13].Get<uint32_t>();
			uint32_t umbralMoonOrigin = fields[14].Get<uint32_t>();
			uint64_t originPlayerGuid = fields[15].Get<uint64_t>();

			std::string zoneName = "Unknown";
			if (zoneId != 0)
				if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(zoneId))
					zoneName = area->area_name[0];

			std::string originPlayerName = "Unknown";
			if (originPlayerGuid != 0)
			{
				QueryResult playerNameResult = CharacterDatabase.Query("SELECT name FROM characters WHERE guid = {}", originPlayerGuid);
				if (playerNameResult)
					originPlayerName = playerNameResult->Fetch()[0].Get<std::string>();
			}

			uint32_t ageSeconds = (serverTime > promotedAt) ? (serverTime - promotedAt) : 0;

			handler.SendSysMessage(Acore::StringFormat("{}LE~{}~{}~{}~{}~{}~{}~{}~{}~{}~{}~{}~{}~{:.1f}~{:.1f}~{:.1f}~{}", NemesisBountyBoardConstants::MSG_PREFIX, rank, name, level, threatScore, killCount, originTraits, earnedTraits, zoneName, ageSeconds, umbralMoonOrigin, originPlayerName, lastMapId, lastPosX, lastPosY, lastPosZ, lastSeenAt));

		} while (eliteResult->NextRow());
	}

	// ── Personal grudge section (v2+) ───────────────────────────────
	uint32 vengeanceCount = sRevengeMgr->GetVengeanceCount(playerGuid);

	QueryResult grudgeResult = WorldDatabase.Query(
		"SELECT e.elite_id, e.name, e.effective_level, e.threat_score, e.kill_count, "
		"e.origin_traits, e.earned_traits, e.last_zone_id, e.last_map_id, "
		"e.last_pos_x, e.last_pos_y, e.last_pos_z, e.promoted_at, "
		"(SELECT COUNT(*) FROM nemesis_elite_kill_log k WHERE k.elite_id = e.elite_id AND k.player_guid = {}) AS deaths_to_nemesis "
		"FROM nemesis_elite e WHERE e.origin_player_guid = {} AND e.is_alive = 1 "
		"ORDER BY e.threat_score DESC",
		playerGuid, playerGuid);

	uint32 grudgeCount = grudgeResult ? static_cast<uint32>(grudgeResult->GetRowCount()) : 0;

	// Leaderboard footer
	handler.SendSysMessage(Acore::StringFormat("{}LF", NemesisBountyBoardConstants::MSG_PREFIX));

	// Revenge header: @@NBB@@RH~vengeanceCount~grudgeCount
	handler.SendSysMessage(Acore::StringFormat("{}RH~{}~{}", NemesisBountyBoardConstants::MSG_PREFIX, vengeanceCount, grudgeCount));

	if (grudgeResult)
	{
		do
		{
			Field* fields = grudgeResult->Fetch();
			uint32 eliteId       = fields[0].Get<uint32>();
			std::string name     = fields[1].Get<std::string>();
			uint8 level          = fields[2].Get<uint8>();
			uint32 threatScore   = fields[3].Get<uint32>();
			uint32 killCount     = fields[4].Get<uint32>();
			uint32 originTraits  = fields[5].Get<uint32>();
			uint32 earnedTraits  = fields[6].Get<uint32>();
			uint16 zoneId        = fields[7].Get<uint16>();
			uint16 mapId         = fields[8].Get<uint16>();
			float posX           = fields[9].Get<float>();
			float posY           = fields[10].Get<float>();
			float posZ           = fields[11].Get<float>();
			uint32 promotedAt    = fields[12].Get<uint32>();
			uint32 deathCount    = fields[13].Get<uint32>();

			std::string zoneName = "Unknown";
			if (zoneId != 0)
				if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(zoneId))
					zoneName = area->area_name[0];

			uint32 ageSeconds = (serverTime > promotedAt) ? (serverTime - promotedAt) : 0;

			// Revenge entry: @@NBB@@RE~eliteId~name~level~threat~kills~originTraits~earnedTraits~zoneName~mapId~posX~posY~posZ~age~deathsToYou
			handler.SendSysMessage(Acore::StringFormat("{}RE~{}~{}~{}~{}~{}~{}~{}~{}~{}~{:.1f}~{:.1f}~{:.1f}~{}~{}", NemesisBountyBoardConstants::MSG_PREFIX, eliteId, name, level, threatScore, killCount, originTraits, earnedTraits, zoneName, mapId, posX, posY, posZ, ageSeconds, deathCount));

		} while (grudgeResult->NextRow());
	}

	// Revenge footer
	handler.SendSysMessage(Acore::StringFormat("{}RF", NemesisBountyBoardConstants::MSG_PREFIX));
}

class NemesisBountyBoardScript : public GameObjectScript
{
public:
	NemesisBountyBoardScript() : GameObjectScript("NemesisBountyBoardScript") {}

	bool OnGossipHello(Player* player, GameObject* /*gameObject*/) override
	{
		SendBountyBoardData(player);
		CloseGossipMenuFor(player);
		return true;
	}
};

void AddNemesisBountyBoardScript()
{
	new NemesisBountyBoardScript();
}
