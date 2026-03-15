#include "NemesisRevengeManager.h"
#include "NemesisRevengeConstants.h"
#include "NemesisConstants.h"
#include "NemesisHelpers.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Group.h"
#include "Log.h"
#include "Player.h"
#include "StringFormat.h"

NemesisRevengeManager* NemesisRevengeManager::Instance()
{
	static NemesisRevengeManager instance;
	return &instance;
}

void NemesisRevengeManager::LoadConfig()
{
	_config.groupVengeanceEnabled = NemesisLoadConfigInt("revenge_group_vengeance_enabled", 1) != 0;
	_config.groupVengeanceMaxDistance = NemesisLoadConfigFloat("revenge_group_vengeance_max_distance", 100.0f);

	LOG_INFO("nemesis_revenge", "{} Revenge config loaded. GroupVengeance: {}, MaxDistance: {:.1f}",
		NemesisConstants::LOG_PREFIX,
		_config.groupVengeanceEnabled,
		_config.groupVengeanceMaxDistance);
}

void NemesisRevengeManager::LoadFromDB()
{
	_vengeanceCounts.clear();

	QueryResult result = WorldDatabase.Query("SELECT player_guid, vengeance_count FROM nemesis_player_vengeance");
	if (!result)
	{
		LOG_INFO("nemesis_revenge", "{} No vengeance records found.", NemesisConstants::LOG_PREFIX);
		return;
	}

	uint32 count = 0;
	do
	{
		Field* fields = result->Fetch();
		uint32 playerGuid = fields[0].Get<uint32>();
		uint32 vengeanceCount = fields[1].Get<uint32>();
		_vengeanceCounts[playerGuid] = vengeanceCount;
		++count;
	} while (result->NextRow());

	LOG_INFO("nemesis_revenge", "{} Loaded {} vengeance records.", NemesisConstants::LOG_PREFIX, count);
}

void NemesisRevengeManager::OnNemesisKilledByPlayer(uint32 eliteId, uint32 originPlayerGuid, Player* killer)
{
	if (!killer || originPlayerGuid == 0)
		return;

	auto checkVengeance = [&](Player* player)
	{
		if (!player)
			return;

		if (player->GetGUID().GetCounter() != originPlayerGuid)
			return;

		IncrementVengeance(player);

		uint32 newCount = GetVengeanceCount(player->GetGUID().GetCounter());
		ChatHandler(player->GetSession()).PSendSysMessage("{} Vengeance! You have slain a nemesis born from your death. ({} total)", NemesisConstants::CHAT_PREFIX, newCount);

		LOG_INFO("nemesis_revenge", "{} Player '{}' (guid {}) earned vengeance kill #{} against elite_id {}.",
			NemesisConstants::LOG_PREFIX, player->GetName(), player->GetGUID().GetCounter(), newCount, eliteId);
	};

	// Check the killer themselves
	checkVengeance(killer);

	// If group vengeance is enabled, check all group members in range
	if (_config.groupVengeanceEnabled)
	{
		Group* group = killer->GetGroup();
		if (group)
		{
			for (GroupReference* reference = group->GetFirstMember(); reference; reference = reference->next())
			{
				Player* member = reference->GetSource();
				if (!member || member == killer)
					continue;

				if (member->GetDistance(killer) > _config.groupVengeanceMaxDistance)
					continue;

				checkVengeance(member);
			}
		}
	}
}

uint32 NemesisRevengeManager::GetVengeanceCount(uint32 playerGuid) const
{
	auto iterator = _vengeanceCounts.find(playerGuid);
	if (iterator != _vengeanceCounts.end())
		return iterator->second;
	return 0;
}

void NemesisRevengeManager::IncrementVengeance(Player* player)
{
	uint32 playerGuid = player->GetGUID().GetCounter();
	uint32 newCount = ++_vengeanceCounts[playerGuid];

	// Upsert the vengeance count in DB
	WorldDatabase.Execute(
		"INSERT INTO nemesis_player_vengeance (player_guid, vengeance_count) VALUES ({}, 1) "
		"ON DUPLICATE KEY UPDATE vengeance_count = vengeance_count + 1",
		playerGuid);

	CheckAndGrantTitles(player, newCount);
}

void NemesisRevengeManager::CheckAndGrantTitles(Player* player, uint32 vengeanceCount)
{
	for (auto const& threshold : NemesisRevengeConstants::DEFAULT_TITLE_THRESHOLDS)
	{
		if (vengeanceCount < threshold.vengeanceCount)
			break;

		CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(threshold.titleId);
		if (!titleEntry)
		{
			LOG_WARN("nemesis_revenge", "{} Title ID {} not found in CharTitles DBC — skipping grant for '{}'.",
				NemesisConstants::LOG_PREFIX, threshold.titleId, threshold.titleName);
			continue;
		}

		if (player->HasTitle(titleEntry))
			continue;

		player->SetTitle(titleEntry);
		ChatHandler(player->GetSession()).PSendSysMessage("{} You have earned the title: |cFFFFD700{}|r!", NemesisConstants::CHAT_PREFIX, threshold.titleName);

		LOG_INFO("nemesis_revenge", "{} Granted title '{}' (ID {}) to player '{}' (guid {}) at vengeance count {}.",
			NemesisConstants::LOG_PREFIX, threshold.titleName, threshold.titleId,
			player->GetName(), player->GetGUID().GetCounter(), vengeanceCount);
	}
}
