#include "NemesisEliteAffixManager.h"
#include "NemesisEliteHelpers.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "LootMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "SpellAuraEffects.h"
#include <sstream>

const std::vector<EquippedAffixEntry> NemesisEliteAffixManager::_emptyAffixEntries = {};

NemesisEliteAffixManager::NemesisEliteAffixManager()
{
}

NemesisEliteAffixManager* NemesisEliteAffixManager::Instance()
{
	static NemesisEliteAffixManager instance;
	return &instance;
}

void NemesisEliteAffixManager::LoadConfig()
{
	LOG_INFO("server.loading", "{} Loading affix configuration from nemesis_configuration...", NemesisConstants::LOG_PREFIX);

	// Affix loot config
	_affixConfig.dropChance = static_cast<uint32>(std::max(0, std::min(100, std::stoi(NemesisLoadConfigValue("elite_affix_drop_chance", "85")))));
	_affixConfig.tierThresholdBlue = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_affix_tier_threshold_blue", "300"))));
	_affixConfig.tierThresholdPurple = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_affix_tier_threshold_purple", "600"))));
	_affixConfig.tierThresholdLegendary = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_affix_tier_threshold_legendary", "700"))));
	_affixConfig.legendaryTopCount = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_affix_legendary_top_count", "10"))));
	_affixConfig.flavorTemplates = NemesisLoadConfigValue("elite_affix_item_flavor_templates", "Taken from the corpse of {name}.;Looted from {name}.;Pried from the remains of {name}.;Stripped from {name} after its final defeat.;Once wielded by {name}.");

	// Affix tier-scaled values (green,blue,purple,legendary)
	// Prefix item stats
	_affixConfig.ambusherCritRating = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_ambusher_crit_rating", "15,26,38,52"), {{ 15, 26, 38, 52 }});
	_affixConfig.healerBaneSpellPower = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_healer_bane_spell_power", "20,35,50,70"), {{ 20, 35, 50, 70 }});
	_affixConfig.giantSlayerAttackPower = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_giant_slayer_attack_power", "20,35,50,70"), {{ 20, 35, 50, 70 }});
	_affixConfig.underdogExpertise = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_underdog_expertise", "10,18,26,36"), {{ 10, 18, 26, 36 }});
	_affixConfig.ironbreakerArmorPen = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_ironbreaker_armor_pen", "15,26,38,52"), {{ 15, 26, 38, 52 }});

	// Suffix item stats
	_affixConfig.notoriousAllStats = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_notorious_all_stats", "4,7,10,14"), {{ 4, 7, 10, 14 }});
	_affixConfig.survivorArmor = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_survivor_armor", "80,140,200,280"), {{ 80, 140, 200, 280 }});
	_affixConfig.spellproofResilience = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_spellproof_resilience", "15,26,38,52"), {{ 15, 26, 38, 52 }});
	_affixConfig.scarredMp5 = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_scarred_mp5", "6,10,15,21"), {{ 6, 10, 15, 21 }});
	_affixConfig.enragedHaste = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_enraged_haste", "15,26,38,52"), {{ 15, 26, 38, 52 }});
	_affixConfig.daybornAttackPower = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_dayborn_attack_power", "15,26,38,52"), {{ 15, 26, 38, 52 }});
	_affixConfig.nightbornAgility = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_nightborn_agility", "12,21,30,42"), {{ 12, 21, 30, 42 }});

	// Equipped aura values
	_affixConfig.deathblowCritPercent = NemesisParseTierFloat(NemesisLoadConfigValue("elite_affix_deathblow_crit_percent", "2,4,5,7"), {{ 2.0f, 4.0f, 5.0f, 7.0f }});
	_affixConfig.wanderingSpeedPercent = NemesisParseTierFloat(NemesisLoadConfigValue("elite_affix_wandering_speed_percent", "3,5,7,8"), {{ 3.0f, 5.0f, 7.0f, 8.0f }});
	_affixConfig.cowardiceThreatPercent = NemesisParseTierFloat(NemesisLoadConfigValue("elite_affix_cowardice_threat_percent", "3,5,8,12"), {{ 3.0f, 5.0f, 8.0f, 12.0f }});
	_affixConfig.dominionThreatPercent = NemesisParseTierFloat(NemesisLoadConfigValue("elite_affix_dominion_threat_percent", "3,5,8,12"), {{ 3.0f, 5.0f, 8.0f, 12.0f }});
	_affixConfig.professionSkillBonus = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_profession_skill_bonus", "100,100,100,100"), {{ 100, 100, 100, 100 }});

	// Proc values
	_affixConfig.executionerProcChance = NemesisParseTierFloat(NemesisLoadConfigValue("elite_affix_executioner_proc_chance", "3,5,7,10"), {{ 3.0f, 5.0f, 7.0f, 10.0f }});
	_affixConfig.executionerProcDamage = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_executioner_proc_damage", "40,70,100,140"), {{ 40, 70, 100, 140 }});
	_affixConfig.executionerHpThreshold = std::max(0.0f, std::min(100.0f, std::stof(NemesisLoadConfigValue("elite_affix_executioner_hp_threshold", "20"))));
	_affixConfig.mageBaneProcChance = NemesisParseTierFloat(NemesisLoadConfigValue("elite_affix_mage_bane_proc_chance", "2,3,4,6"), {{ 2.0f, 3.0f, 4.0f, 6.0f }});
	_affixConfig.mageBaneSlowPercent = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_mage_bane_slow_percent", "15,20,25,30"), {{ 15, 20, 25, 30 }});
	_affixConfig.mageBaneDurationMs = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_mage_bane_duration_ms", "4000,5000,6000,6000"), {{ 4000, 5000, 6000, 6000 }});
	_affixConfig.opportunistProcChance = NemesisParseTierFloat(NemesisLoadConfigValue("elite_affix_opportunist_proc_chance", "2,3,4,6"), {{ 2.0f, 3.0f, 4.0f, 6.0f }});
	_affixConfig.opportunistDurationMs = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_opportunist_duration_ms", "3000,4000,5000,5000"), {{ 3000, 4000, 5000, 5000 }});
	_affixConfig.scavengerBonusDamagePercent = NemesisParseTierFloat(NemesisLoadConfigValue("elite_affix_scavenger_bonus_damage_percent", "3,5,8,11"), {{ 3.0f, 5.0f, 8.0f, 11.0f }});
	_affixConfig.duelistBonusDamagePercent = NemesisParseTierFloat(NemesisLoadConfigValue("elite_affix_duelist_bonus_damage_percent", "3,5,8,11"), {{ 3.0f, 5.0f, 8.0f, 11.0f }});
	_affixConfig.umbralforgedAoeDamage = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_umbralforged_aoe_damage", "8,14,20,28"), {{ 8, 14, 20, 28 }});
	_affixConfig.blightReflectDamage = NemesisParseTierInt(NemesisLoadConfigValue("elite_affix_blight_reflect_damage", "10,18,26,36"), {{ 10, 18, 26, 36 }});
	_affixConfig.plundererBonusGoldPercent = NemesisParseTierFloat(NemesisLoadConfigValue("elite_affix_plunderer_bonus_gold_percent", "5,10,15,25"), {{ 5.0f, 10.0f, 15.0f, 25.0f }});
	_affixConfig.sageBonusXpPercent = NemesisParseTierFloat(NemesisLoadConfigValue("elite_affix_sage_bonus_xp_percent", "3,5,8,12"), {{ 3.0f, 5.0f, 8.0f, 12.0f }});
	_affixConfig.umbralEchoIntervalMs = static_cast<uint32>(std::max(100, std::stoi(NemesisLoadConfigValue("elite_affix_umbral_echo_interval_ms", "3000"))));

	LOG_INFO("server.loading", "{} Affix config loaded. Drop chance: {}%, Blue threshold: {}, Purple threshold: {}, Legendary threshold: {}", NemesisConstants::LOG_PREFIX, _affixConfig.dropChance, _affixConfig.tierThresholdBlue, _affixConfig.tierThresholdPurple, _affixConfig.tierThresholdLegendary);
}

void NemesisEliteAffixManager::PopulatePlayerAffixCache(Player* player)
{
	if (!player)
		return;

	ObjectGuid playerGuid = player->GetGUID();
	_playerAffixCache.erase(playerGuid);

	std::vector<EquippedAffixEntry> entries;

	// Scan all equipment slots (0-18 in 3.3.5).
	for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
	{
		Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
		if (!item)
			continue;

		uint32 itemEntry = item->GetEntry();
		if (itemEntry < NemesisEliteConstants::AFFIX_ITEM_ENTRY_BASE)
			continue;

		// This is a nemesis affix item — look up its affix data.
		QueryResult result = WorldDatabase.Query("SELECT `prefix_trait`, `suffix_trait`, `quality_tier` FROM `nemesis_item_affixes` WHERE `item_entry` = {}", itemEntry);
		if (result)
		{
			EquippedAffixEntry entry;
			entry.itemEntry = itemEntry;
			entry.prefixTrait = result->Fetch()[0].Get<uint32>();
			entry.suffixTrait = result->Fetch()[1].Get<uint32>();
			entry.qualityTier = result->Fetch()[2].Get<uint8>();
			entries.push_back(entry);
		}
	}

	if (!entries.empty())
	{
		_playerAffixCache[playerGuid] = std::move(entries);
		LOG_DEBUG("elite", "{} Populated affix cache for {} — {} equipped affix item(s).", NemesisConstants::LOG_PREFIX, player->GetName(), _playerAffixCache[playerGuid].size());
	}

	RefreshAffixAuras(player);
}

void NemesisEliteAffixManager::ClearPlayerAffixCache(ObjectGuid playerGuid)
{
	_playerAffixCache.erase(playerGuid);
	LOG_DEBUG("elite", "{} Cleared affix cache for player GUID {}.", NemesisConstants::LOG_PREFIX, playerGuid.ToString());
}

void NemesisEliteAffixManager::OnPlayerEquipAffixItem(Player* player, uint32 itemEntry)
{
	if (!player || itemEntry < NemesisEliteConstants::AFFIX_ITEM_ENTRY_BASE)
		return;

	auto& entries = _playerAffixCache[player->GetGUID()];
	for (auto const& existing : entries)
	{
		if (existing.itemEntry == itemEntry)
			return;
	}

	QueryResult result = WorldDatabase.Query("SELECT `prefix_trait`, `suffix_trait`, `quality_tier` FROM `nemesis_item_affixes` WHERE `item_entry` = {}", itemEntry);
	if (!result)
		return;

	EquippedAffixEntry entry;
	entry.itemEntry = itemEntry;
	entry.prefixTrait = result->Fetch()[0].Get<uint32>();
	entry.suffixTrait = result->Fetch()[1].Get<uint32>();
	entry.qualityTier = result->Fetch()[2].Get<uint8>();

	entries.push_back(entry);
	LOG_DEBUG("elite", "{} {} equipped affix item {} (prefix 0x{:X}, suffix 0x{:X}, tier {}).", NemesisConstants::LOG_PREFIX, player->GetName(), itemEntry, entry.prefixTrait, entry.suffixTrait, static_cast<uint32>(entry.qualityTier));

	RefreshAffixAuras(player);
}

void NemesisEliteAffixManager::OnPlayerUnequipAffixItem(Player* player, uint32 itemEntry)
{
	if (!player)
		return;

	ObjectGuid playerGuid = player->GetGUID();
	auto cacheIterator = _playerAffixCache.find(playerGuid);
	if (cacheIterator == _playerAffixCache.end())
		return;

	auto& entries = cacheIterator->second;
	std::size_t sizeBefore = entries.size();
	entries.erase(std::remove_if(entries.begin(), entries.end(), [itemEntry](EquippedAffixEntry const& entry) { return entry.itemEntry == itemEntry; }), entries.end());

	if (entries.size() < sizeBefore)
		LOG_DEBUG("elite", "{} {} unequipped affix item {} — {} affix item(s) remaining.", NemesisConstants::LOG_PREFIX, player->GetName(), itemEntry, entries.size());

	if (entries.empty())
	{
		_playerAffixCache.erase(cacheIterator);
		LOG_DEBUG("elite", "{} {} has no more equipped affix items; cleared affix cache.", NemesisConstants::LOG_PREFIX, playerGuid.ToString());
	}

	RefreshAffixAuras(player);
}

std::vector<EquippedAffixEntry> const& NemesisEliteAffixManager::GetPlayerAffixEntries(ObjectGuid playerGuid) const
{
	auto cacheIterator = _playerAffixCache.find(playerGuid);
	if (cacheIterator != _playerAffixCache.end())
		return cacheIterator->second;
	return _emptyAffixEntries;
}

void NemesisEliteAffixManager::RefreshAffixAuras(Player* player)
{
	if (!player)
		return;

	// All affix aura spell IDs that this method manages.
	static constexpr uint32 AFFIX_AURA_SPELLS[] = {
		NemesisEliteConstants::ELITE_AFFIX_AURA_DEATHBLOW,
		NemesisEliteConstants::ELITE_AFFIX_AURA_WANDERING,
		NemesisEliteConstants::ELITE_AFFIX_AURA_COWARDICE,
		NemesisEliteConstants::ELITE_AFFIX_AURA_DOMINION,
		NemesisEliteConstants::ELITE_AFFIX_AURA_SKINNING,
		NemesisEliteConstants::ELITE_AFFIX_AURA_MINING,
		NemesisEliteConstants::ELITE_AFFIX_AURA_HERBALISM,
		NemesisEliteConstants::ELITE_AFFIX_AURA_BLACKSMITHING,
		NemesisEliteConstants::ELITE_AFFIX_AURA_LEATHERWORKING,
		NemesisEliteConstants::ELITE_AFFIX_AURA_TAILORING,
		NemesisEliteConstants::ELITE_AFFIX_AURA_ENGINEERING,
		NemesisEliteConstants::ELITE_AFFIX_AURA_ALCHEMY,
		NemesisEliteConstants::ELITE_AFFIX_AURA_ENCHANTING,
		NemesisEliteConstants::ELITE_AFFIX_AURA_JEWELCRAFTING,
		NemesisEliteConstants::ELITE_AFFIX_AURA_INSCRIPTION,
	};

	// Strip all existing affix auras before recalculating.
	for (uint32 spellId : AFFIX_AURA_SPELLS)
		player->RemoveAura(spellId);

	auto const& entries = GetPlayerAffixEntries(player->GetGUID());
	if (entries.empty())
		return;

	// Aggregate values per affix type across all equipped items.
	float deathblowBonus = 0.0f;
	float wanderingSpeed = 0.0f;
	float cowardiceReduction = 0.0f;
	float dominionIncrease = 0.0f;

	// Profession skill mapping: origin trait → affix aura spell ID.
	struct ProfessionMapping { uint32 originTrait; uint32 spellId; };
	static constexpr ProfessionMapping PROFESSION_SPELLS[] = {
		{ ORIGIN_TRAIT_SKINNER, NemesisEliteConstants::ELITE_AFFIX_AURA_SKINNING },
		{ ORIGIN_TRAIT_ORE_GORGED, NemesisEliteConstants::ELITE_AFFIX_AURA_MINING },
		{ ORIGIN_TRAIT_ROOT_RIPPER, NemesisEliteConstants::ELITE_AFFIX_AURA_HERBALISM },
		{ ORIGIN_TRAIT_FORGE_BREAKER, NemesisEliteConstants::ELITE_AFFIX_AURA_BLACKSMITHING },
		{ ORIGIN_TRAIT_HIDE_MANGLER, NemesisEliteConstants::ELITE_AFFIX_AURA_LEATHERWORKING },
		{ ORIGIN_TRAIT_THREAD_RIPPER, NemesisEliteConstants::ELITE_AFFIX_AURA_TAILORING },
		{ ORIGIN_TRAIT_GEAR_GRINDER, NemesisEliteConstants::ELITE_AFFIX_AURA_ENGINEERING },
		{ ORIGIN_TRAIT_VIAL_SHATTER, NemesisEliteConstants::ELITE_AFFIX_AURA_ALCHEMY },
		{ ORIGIN_TRAIT_RUNE_EATER, NemesisEliteConstants::ELITE_AFFIX_AURA_ENCHANTING },
		{ ORIGIN_TRAIT_GEM_CRUSHER, NemesisEliteConstants::ELITE_AFFIX_AURA_JEWELCRAFTING },
		{ ORIGIN_TRAIT_INK_DRINKER, NemesisEliteConstants::ELITE_AFFIX_AURA_INSCRIPTION },
	};

	// Per-profession aggregated skill bonus. Indexed by position in PROFESSION_SPELLS.
	int32 professionBonuses[11] = {};

	auto const& cfg = _affixConfig;

	for (auto const& entry : entries)
	{
		// --- Prefix: Deathblow (crit damage bonus) ---
		if (entry.prefixTrait == ORIGIN_TRAIT_DEATHBLOW)
			deathblowBonus += cfg.deathblowCritPercent[entry.qualityTier];

		// --- Prefix: Profession skills ---
		for (uint32 i = 0; i < 11; ++i)
		{
			if (entry.prefixTrait == PROFESSION_SPELLS[i].originTrait)
				professionBonuses[i] += cfg.professionSkillBonus[entry.qualityTier];
		}

		// --- Suffix: of Wandering (movement speed) ---
		if (entry.suffixTrait == EARNED_TRAIT_NOMAD)
			wanderingSpeed += cfg.wanderingSpeedPercent[entry.qualityTier];

		// --- Suffix: of Cowardice (threat reduction) ---
		if (entry.suffixTrait == EARNED_TRAIT_COWARD)
			cowardiceReduction += cfg.cowardiceThreatPercent[entry.qualityTier];

		// --- Suffix: of Dominion (threat increase) ---
		if (entry.suffixTrait == EARNED_TRAIT_TERRITORIAL)
			dominionIncrease += cfg.dominionThreatPercent[entry.qualityTier];
	}

	// Apply aggregated auras.
	if (deathblowBonus > 0.0f)
	{
		int32 bp = static_cast<int32>(deathblowBonus);
		player->CastCustomSpell(player, NemesisEliteConstants::ELITE_AFFIX_AURA_DEATHBLOW, &bp, nullptr, nullptr, true);
	}
	if (wanderingSpeed > 0.0f)
	{
		int32 bp = static_cast<int32>(wanderingSpeed);
		player->CastCustomSpell(player, NemesisEliteConstants::ELITE_AFFIX_AURA_WANDERING, &bp, nullptr, nullptr, true);
	}
	if (cowardiceReduction > 0.0f)
	{
		int32 bp = -static_cast<int32>(cowardiceReduction);
		player->CastCustomSpell(player, NemesisEliteConstants::ELITE_AFFIX_AURA_COWARDICE, &bp, nullptr, nullptr, true);
	}
	if (dominionIncrease > 0.0f)
	{
		int32 bp = static_cast<int32>(dominionIncrease);
		player->CastCustomSpell(player, NemesisEliteConstants::ELITE_AFFIX_AURA_DOMINION, &bp, nullptr, nullptr, true);
	}
	for (uint32 i = 0; i < 11; ++i)
	{
		if (professionBonuses[i] > 0)
			player->CastCustomSpell(player, PROFESSION_SPELLS[i].spellId, &professionBonuses[i], nullptr, nullptr, true);
	}
}

void NemesisEliteAffixManager::ActivateUmbralforgedAffix(Player* player)
{
	if (!player)
		return;

	auto const& entries = GetPlayerAffixEntries(player->GetGUID());
	if (entries.empty())
		return;

	int32 totalDamage = 0;
	for (auto const& entry : entries)
	{
		if (entry.prefixTrait == ORIGIN_TRAIT_UMBRALFORGED)
			totalDamage += _affixConfig.umbralforgedAoeDamage[entry.qualityTier];
	}

	if (totalDamage <= 0)
		return;

	UmbralforgedPlayerState state;
	state.nextPulseAt = getMSTime() + _affixConfig.umbralEchoIntervalMs;
	state.aggregatedDamage = totalDamage;
	_umbralforgedActivePlayers[player->GetGUID()] = state;
}

void NemesisEliteAffixManager::DeactivateUmbralforgedAffix(ObjectGuid playerGuid)
{
	_umbralforgedActivePlayers.erase(playerGuid);
}

void NemesisEliteAffixManager::UpdateUmbralEchoes()
{
	if (_umbralforgedActivePlayers.empty())
		return;

	uint32 now = getMSTime();

	for (auto iterator = _umbralforgedActivePlayers.begin(); iterator != _umbralforgedActivePlayers.end(); )
	{
		Player* player = ObjectAccessor::FindPlayer(iterator->first);
		if (!player || !player->IsAlive() || !player->IsInCombat())
		{
			iterator = _umbralforgedActivePlayers.erase(iterator);
			continue;
		}

		if (now >= iterator->second.nextPulseAt)
		{
			int32 damage = iterator->second.aggregatedDamage;
			player->CastCustomSpell(player, NemesisEliteConstants::ELITE_AFFIX_SPELL_UMBRAL_ECHO, &damage, nullptr, nullptr, true);
			iterator->second.nextPulseAt = now + _affixConfig.umbralEchoIntervalMs;
		}

		++iterator;
	}
}

uint8 NemesisEliteAffixManager::DetermineAffixTier(uint32 threatScore, uint32 eliteId)
{
	// Check Legendary: requires BOTH score floor AND bounty board top-N placement
	if (threatScore >= _affixConfig.tierThresholdLegendary)
	{
		QueryResult topResult = WorldDatabase.Query("SELECT `elite_id` FROM `nemesis_elite` WHERE `is_alive` = 1 ORDER BY `threat_score` DESC LIMIT {}", _affixConfig.legendaryTopCount);
		if (topResult)
		{
			do
			{
				if (static_cast<uint32>(topResult->Fetch()[0].Get<uint64>()) == eliteId)
					return NemesisEliteConstants::AFFIX_TIER_LEGENDARY;
			} while (topResult->NextRow());
		}
	}

	if (threatScore >= _affixConfig.tierThresholdPurple)
		return NemesisEliteConstants::AFFIX_TIER_PURPLE;
	if (threatScore >= _affixConfig.tierThresholdBlue)
		return NemesisEliteConstants::AFFIX_TIER_BLUE;

	return NemesisEliteConstants::AFFIX_TIER_GREEN;
}

// Returns the stat entries to inject for a prefix (origin trait) at the given quality tier.
// Server-side affixes return an empty vector — no stat injection, handled by runtime hooks.
std::vector<AffixStatEntry> NemesisEliteAffixManager::GetPrefixStatData(uint32 originTrait, uint8 qualityTier)
{
	// Tier indices: 0=Green, 1=Blue, 2=Purple, 3=Legendary
	auto const& cfg = _affixConfig;
	switch (originTrait)
	{
		case ORIGIN_TRAIT_AMBUSHER:
			return {{ 32, cfg.ambusherCritRating[qualityTier] }};
		case ORIGIN_TRAIT_HEALER_BANE:
			return {{ 45, cfg.healerBaneSpellPower[qualityTier] }};
		case ORIGIN_TRAIT_GIANT_SLAYER:
			return {{ 38, cfg.giantSlayerAttackPower[qualityTier] }};
		case ORIGIN_TRAIT_UNDERDOG:
			return {{ 37, cfg.underdogExpertise[qualityTier] }};
		case ORIGIN_TRAIT_IRONBREAKER:
			return {{ 44, cfg.ironbreakerArmorPen[qualityTier] }};
		// Server-side prefixes — no stat injection:
		// Executioner (proc), Mage-Bane (proc), Opportunist (proc), Umbralforged (periodic AoE),
		// Deathblow (crit damage), Scavenger (conditional), Duelist (conditional),
		// Plunderer (gold on kill), all profession traits (skill bonuses use spell effects)
		default:
			return {};
	}
}

// Returns the stat entries to inject for a suffix (earned trait) at the given quality tier.
// Server-side affixes return an empty vector. "of Notoriety" returns 5 entries (all primary stats).
// "of Persistence" returns one entry with the AFFIX_STAT_ARMOR sentinel.
std::vector<AffixStatEntry> NemesisEliteAffixManager::GetSuffixStatData(uint32 earnedTrait, uint8 qualityTier)
{
	auto const& cfg = _affixConfig;
	switch (earnedTrait)
	{
		case EARNED_TRAIT_NOTORIOUS:
		{
			int32 value = cfg.notoriousAllStats[qualityTier];
			return {{ 4, value }, { 3, value }, { 7, value }, { 5, value }, { 6, value }};
		}
		case EARNED_TRAIT_SURVIVOR:
			return {{ AFFIX_STAT_ARMOR, cfg.survivorArmor[qualityTier] }};
		case EARNED_TRAIT_SPELLPROOF:
			return {{ 35, cfg.spellproofResilience[qualityTier] }};
		case EARNED_TRAIT_SCARRED:
			return {{ 43, cfg.scarredMp5[qualityTier] }};
		case EARNED_TRAIT_ENRAGED:
			return {{ 36, cfg.enragedHaste[qualityTier] }};
		case EARNED_TRAIT_DAYBORN:
			return {{ 38, cfg.daybornAttackPower[qualityTier] }};
		case EARNED_TRAIT_NIGHTBORN:
			return {{ 3, cfg.nightbornAgility[qualityTier] }};
		// Server-side suffixes — no stat injection:
		// Coward (threat reduction), Territorial (threat generation), Blight (reflection),
		// Nomad (movement speed), Sage (XP), Studious (profession skillup)
		default:
			return {};
	}
}

void NemesisEliteAffixManager::GenerateAffixedLoot(Creature* creature, Player* /*killer*/, std::shared_ptr<NemesisEliteCreature> const& elite)
{
	// 1. Roll drop chance
	uint32 roll = urand(1, 100);
	if (roll > _affixConfig.dropChance)
	{
		LOG_INFO("elite", "{} No affix drop for '{}' (elite_id {}): rolled {} > {}%.", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), roll, _affixConfig.dropChance);
		return;
	}

	// 2. Determine quality tier
	uint8 qualityTier = DetermineAffixTier(elite->GetThreatScore(), elite->GetEliteId());

	// 3. Query equippable items from the ORIGINAL creature's loot table
	uint32 originalEntry = elite->GetCreatureEntry();
	QueryResult lootIdResult = WorldDatabase.Query("SELECT `lootid` FROM `creature_template` WHERE `entry` = {}", originalEntry);
	uint32 lootId = 0;
	if (lootIdResult)
		lootId = lootIdResult->Fetch()[0].Get<uint32>();
	if (lootId == 0)
		lootId = originalEntry;

	std::vector<uint32> candidates;

	// Pass 1: Direct equippable items (weapons and armor)
	QueryResult directResult = WorldDatabase.Query(
		"SELECT clt.`Item` FROM `creature_loot_template` clt "
		"JOIN `item_template` it ON clt.`Item` = it.`entry` "
		"WHERE clt.`Entry` = {} AND clt.`Reference` = 0 AND it.`class` IN (2, 4)", lootId);
	if (directResult)
	{
		do
		{
			candidates.push_back(directResult->Fetch()[0].Get<uint32>());
		} while (directResult->NextRow());
	}

	// Pass 2: One-level-deep reference entries (creature_loot_template → reference_loot_template → item)
	QueryResult ref1Result = WorldDatabase.Query(
		"SELECT rlt.`Item` FROM `creature_loot_template` clt "
		"JOIN `reference_loot_template` rlt ON clt.`Reference` = rlt.`Entry` "
		"JOIN `item_template` it ON rlt.`Item` = it.`entry` "
		"WHERE clt.`Entry` = {} AND clt.`Reference` != 0 AND rlt.`Reference` = 0 AND it.`class` IN (2, 4)", lootId);
	if (ref1Result)
	{
		do
		{
			candidates.push_back(ref1Result->Fetch()[0].Get<uint32>());
		} while (ref1Result->NextRow());
	}

	// Pass 3: Two-level-deep reference entries (creature_loot_template → ref1 → ref2 → item)
	QueryResult ref2Result = WorldDatabase.Query(
		"SELECT rlt2.`Item` FROM `creature_loot_template` clt "
		"JOIN `reference_loot_template` rlt1 ON clt.`Reference` = rlt1.`Entry` "
		"JOIN `reference_loot_template` rlt2 ON rlt1.`Reference` = rlt2.`Entry` "
		"JOIN `item_template` it ON rlt2.`Item` = it.`entry` "
		"WHERE clt.`Entry` = {} AND clt.`Reference` != 0 AND rlt1.`Reference` != 0 AND rlt2.`Reference` = 0 AND it.`class` IN (2, 4)", lootId);
	if (ref2Result)
	{
		do
		{
			candidates.push_back(ref2Result->Fetch()[0].Get<uint32>());
		} while (ref2Result->NextRow());
	}

	// Pass 4: Three-level-deep reference entries (creature_loot_template → ref1 → ref2 → ref3 → item)
	QueryResult ref3Result = WorldDatabase.Query(
		"SELECT rlt3.`Item` FROM `creature_loot_template` clt "
		"JOIN `reference_loot_template` rlt1 ON clt.`Reference` = rlt1.`Entry` "
		"JOIN `reference_loot_template` rlt2 ON rlt1.`Reference` = rlt2.`Entry` "
		"JOIN `reference_loot_template` rlt3 ON rlt2.`Reference` = rlt3.`Entry` "
		"JOIN `item_template` it ON rlt3.`Item` = it.`entry` "
		"WHERE clt.`Entry` = {} AND clt.`Reference` != 0 AND rlt1.`Reference` != 0 AND rlt2.`Reference` != 0 AND rlt3.`Reference` = 0 AND it.`class` IN (2, 4)", lootId);
	if (ref3Result)
	{
		do
		{
			candidates.push_back(ref3Result->Fetch()[0].Get<uint32>());
		} while (ref3Result->NextRow());
	}

	if (candidates.empty())
	{
		LOG_INFO("elite", "{} No equippable items in loot table for '{}' (elite_id {}, lootid {}).", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), lootId);
		return;
	}

	// 4. Pick one candidate at random
	uint32 selectedItemEntry = candidates[urand(0, static_cast<uint32>(candidates.size() - 1))];

	// 5. Select prefix and suffix traits
	uint32 prefixTrait = elite->GetOriginTrait();
	uint32 suffixTrait = 0;

	// Collect individual earned trait bits into a vector
	std::vector<uint32> earnedTraitBits;
	uint32 earnedTraits = elite->GetEarnedTraits();
	for (uint32 bit = 0; bit < 32; ++bit)
	{
		if (earnedTraits & (1u << bit))
			earnedTraitBits.push_back(1u << bit);
	}

	if (qualityTier == NemesisEliteConstants::AFFIX_TIER_GREEN)
	{
		if (!earnedTraitBits.empty())
		{
			if (urand(0, 1) == 0)
			{
				// Prefix only: origin trait prefix, no suffix
				suffixTrait = 0;
			}
			else
			{
				// Suffix only: no prefix, random earned trait suffix
				prefixTrait = 0;
				suffixTrait = earnedTraitBits[urand(0, static_cast<uint32>(earnedTraitBits.size() - 1))];
			}
		}
		// else: prefix = origin trait, suffix = 0 (no earned traits)
	}
	else
	{
		// BLUE/PURPLE/LEGENDARY: prefix = origin trait, suffix = random earned trait
		if (!earnedTraitBits.empty())
			suffixTrait = earnedTraitBits[urand(0, static_cast<uint32>(earnedTraitBits.size() - 1))];
	}

	// 6. Clone the selected item_template
	uint32 newEntry = NemesisEliteConstants::AFFIX_ITEM_ENTRY_BASE + static_cast<uint32>(elite->GetEliteId());

	// Get the base item template from the in-memory cache
	ItemTemplate const* baseTemplate = sObjectMgr->GetItemTemplate(selectedItemEntry);
	if (!baseTemplate)
	{
		LOG_ERROR("elite", "{} Base item template {} not found in ObjectMgr for affix generation.", NemesisConstants::LOG_PREFIX, selectedItemEntry);
		return;
	}

	// Build affixed name.
	// Strip any existing "of ..." suffix from the base item name to avoid
	// double suffixes like "Draconic Choker of Ferocity of the Sunborn".
	// Only strip when we're appending our own suffix.
	std::string baseName = baseTemplate->Name1;
	if (suffixTrait != 0)
	{
		size_t ofPosition = baseName.rfind(" of ");
		if (ofPosition != std::string::npos)
			baseName = baseName.substr(0, ofPosition);
	}

	std::string affixedName;
	if (prefixTrait && suffixTrait)
		affixedName = std::string(NemesisEliteConstants::GetAffixPrefixName(prefixTrait)) + " " + baseName + " " + NemesisEliteConstants::GetAffixSuffixName(suffixTrait);
	else if (prefixTrait)
		affixedName = std::string(NemesisEliteConstants::GetAffixPrefixName(prefixTrait)) + " " + baseName;
	else if (suffixTrait)
		affixedName = baseName + " " + NemesisEliteConstants::GetAffixSuffixName(suffixTrait);
	else
		affixedName = baseName;

	// Build flavor text from templates
	std::string flavorText;
	{
		std::vector<std::string> flavorTemplates;
		std::istringstream flavorStream(_affixConfig.flavorTemplates);
		std::string flavorToken;
		while (std::getline(flavorStream, flavorToken, ';'))
		{
			if (!flavorToken.empty())
				flavorTemplates.push_back(flavorToken);
		}
		if (!flavorTemplates.empty())
		{
			flavorText = flavorTemplates[urand(0, static_cast<uint32>(flavorTemplates.size() - 1))];
			size_t namePos = flavorText.find("{name}");
			while (namePos != std::string::npos)
			{
				flavorText.replace(namePos, 6, elite->GetName());
				namePos = flavorText.find("{name}", namePos + elite->GetName().length());
			}
		}
	}

	// Collect all affix stats to inject from prefix and suffix traits.
	std::vector<AffixStatEntry> affixStats;
	int32 bonusArmor = 0;
	if (prefixTrait != 0)
	{
		auto prefixStats = GetPrefixStatData(prefixTrait, qualityTier);
		for (auto const& entry : prefixStats)
		{
			if (entry.statType == AFFIX_STAT_ARMOR)
				bonusArmor += entry.statValue;
			else
				affixStats.push_back(entry);
		}
	}
	if (suffixTrait != 0)
	{
		auto suffixStats = GetSuffixStatData(suffixTrait, qualityTier);
		for (auto const& entry : suffixStats)
		{
			if (entry.statType == AFFIX_STAT_ARMOR)
				bonusArmor += entry.statValue;
			else
				affixStats.push_back(entry);
		}
	}

	// Find empty stat slots on the base item for injection.
	std::vector<uint32> emptyStatSlots;
	for (uint32 statIndex = 0; statIndex < MAX_ITEM_PROTO_STATS; ++statIndex)
	{
		if (baseTemplate->ItemStat[statIndex].ItemStatType == 0)
			emptyStatSlots.push_back(statIndex);
	}

	// Delete any stale row from a prior session before inserting the fresh clone.
	// INSERT IGNORE would silently skip the insert if a stale row exists, leaving
	// the DB out of sync with the in-memory cache.
	WorldDatabase.DirectExecute("DELETE FROM `item_template` WHERE `entry` = {}", newEntry);

	// Clone the item_template row into DB with overridden fields
	std::string escapedAffixedName = affixedName;
	WorldDatabase.EscapeString(escapedAffixedName);

	std::string escapedFlavorText = flavorText;
	WorldDatabase.EscapeString(escapedFlavorText);

	WorldDatabase.DirectExecute(
		"INSERT INTO `item_template` "
		"(`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`, `Quality`, "
		"`Flags`, `FlagsExtra`, `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, "
		"`AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `RequiredSkill`, "
		"`RequiredSkillRank`, `requiredspell`, `requiredhonorrank`, `RequiredCityRank`, "
		"`RequiredReputationFaction`, `RequiredReputationRank`, `maxcount`, `stackable`, "
		"`ContainerSlots`, "
		"`stat_type1`, `stat_value1`, `stat_type2`, `stat_value2`, `stat_type3`, `stat_value3`, "
		"`stat_type4`, `stat_value4`, `stat_type5`, `stat_value5`, `stat_type6`, `stat_value6`, "
		"`stat_type7`, `stat_value7`, `stat_type8`, `stat_value8`, `stat_type9`, `stat_value9`, "
		"`stat_type10`, `stat_value10`, "
		"`ScalingStatDistribution`, `ScalingStatValue`, "
		"`dmg_min1`, `dmg_max1`, `dmg_type1`, `dmg_min2`, `dmg_max2`, `dmg_type2`, "
		"`armor`, `holy_res`, `fire_res`, `nature_res`, `frost_res`, `shadow_res`, `arcane_res`, "
		"`delay`, `ammo_type`, `RangedModRange`, "
		"`spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellppmRate_1`, `spellcooldown_1`, `spellcategory_1`, `spellcategorycooldown_1`, "
		"`spellid_2`, `spelltrigger_2`, `spellcharges_2`, `spellppmRate_2`, `spellcooldown_2`, `spellcategory_2`, `spellcategorycooldown_2`, "
		"`spellid_3`, `spelltrigger_3`, `spellcharges_3`, `spellppmRate_3`, `spellcooldown_3`, `spellcategory_3`, `spellcategorycooldown_3`, "
		"`spellid_4`, `spelltrigger_4`, `spellcharges_4`, `spellppmRate_4`, `spellcooldown_4`, `spellcategory_4`, `spellcategorycooldown_4`, "
		"`spellid_5`, `spelltrigger_5`, `spellcharges_5`, `spellppmRate_5`, `spellcooldown_5`, `spellcategory_5`, `spellcategorycooldown_5`, "
		"`bonding`, `description`, `PageText`, `LanguageID`, `PageMaterial`, "
		"`startquest`, `lockid`, `Material`, `sheath`, `RandomProperty`, `RandomSuffix`, "
		"`block`, `itemset`, `MaxDurability`, `area`, `Map`, `BagFamily`, "
		"`TotemCategory`, `socketColor_1`, `socketContent_1`, `socketColor_2`, `socketContent_2`, "
		"`socketColor_3`, `socketContent_3`, `socketBonus`, "
		"`GemProperties`, `RequiredDisenchantSkill`, `ArmorDamageModifier`, `duration`, "
		"`ItemLimitCategory`, `HolidayId`, `ScriptName`, `DisenchantID`, "
		"`FoodType`, `minMoneyLoot`, `maxMoneyLoot`, `flagsCustom`) "
		"SELECT "
		"{}, `class`, `subclass`, `SoundOverrideSubclass`, '{}', `displayid`, `Quality`, "
		"`Flags`, `FlagsExtra`, `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, "
		"`AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `RequiredSkill`, "
		"`RequiredSkillRank`, `requiredspell`, `requiredhonorrank`, `RequiredCityRank`, "
		"`RequiredReputationFaction`, `RequiredReputationRank`, `maxcount`, `stackable`, "
		"`ContainerSlots`, "
		"`stat_type1`, `stat_value1`, `stat_type2`, `stat_value2`, `stat_type3`, `stat_value3`, "
		"`stat_type4`, `stat_value4`, `stat_type5`, `stat_value5`, `stat_type6`, `stat_value6`, "
		"`stat_type7`, `stat_value7`, `stat_type8`, `stat_value8`, `stat_type9`, `stat_value9`, "
		"`stat_type10`, `stat_value10`, "
		"`ScalingStatDistribution`, `ScalingStatValue`, "
		"`dmg_min1`, `dmg_max1`, `dmg_type1`, `dmg_min2`, `dmg_max2`, `dmg_type2`, "
		"`armor`, `holy_res`, `fire_res`, `nature_res`, `frost_res`, `shadow_res`, `arcane_res`, "
		"`delay`, `ammo_type`, `RangedModRange`, "
		"`spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellppmRate_1`, `spellcooldown_1`, `spellcategory_1`, `spellcategorycooldown_1`, "
		"`spellid_2`, `spelltrigger_2`, `spellcharges_2`, `spellppmRate_2`, `spellcooldown_2`, `spellcategory_2`, `spellcategorycooldown_2`, "
		"`spellid_3`, `spelltrigger_3`, `spellcharges_3`, `spellppmRate_3`, `spellcooldown_3`, `spellcategory_3`, `spellcategorycooldown_3`, "
		"`spellid_4`, `spelltrigger_4`, `spellcharges_4`, `spellppmRate_4`, `spellcooldown_4`, `spellcategory_4`, `spellcategorycooldown_4`, "
		"`spellid_5`, `spelltrigger_5`, `spellcharges_5`, `spellppmRate_5`, `spellcooldown_5`, `spellcategory_5`, `spellcategorycooldown_5`, "
		"2, '{}', `PageText`, `LanguageID`, `PageMaterial`, "
		"`startquest`, `lockid`, `Material`, `sheath`, 0, 0, "
		"`block`, `itemset`, `MaxDurability`, `area`, `Map`, `BagFamily`, "
		"`TotemCategory`, `socketColor_1`, `socketContent_1`, `socketColor_2`, `socketContent_2`, "
		"`socketColor_3`, `socketContent_3`, `socketBonus`, "
		"`GemProperties`, `RequiredDisenchantSkill`, `ArmorDamageModifier`, `duration`, "
		"`ItemLimitCategory`, `HolidayId`, `ScriptName`, `DisenchantID`, "
		"`FoodType`, `minMoneyLoot`, `maxMoneyLoot`, `flagsCustom` "
		"FROM `item_template` WHERE `entry` = {}",
		newEntry, escapedAffixedName, escapedFlavorText, selectedItemEntry);

	// Inject affix stats into empty stat slots on the cloned item.
	uint32 statsInjected = 0;
	for (uint32 affixIndex = 0; affixIndex < affixStats.size() && statsInjected < emptyStatSlots.size(); ++affixIndex)
	{
		uint32 slotIndex = emptyStatSlots[statsInjected];
		WorldDatabase.DirectExecute("UPDATE `item_template` SET `stat_type{}` = {}, `stat_value{}` = {} WHERE `entry` = {}",
			slotIndex + 1, affixStats[affixIndex].statType, slotIndex + 1, affixStats[affixIndex].statValue, newEntry);
		++statsInjected;
	}

	// Apply bonus armor if any affix grants it (e.g., of Persistence).
	if (bonusArmor > 0)
	{
		WorldDatabase.DirectExecute("UPDATE `item_template` SET `armor` = `armor` + {} WHERE `entry` = {}", bonusArmor, newEntry);
	}

	if (statsInjected < affixStats.size())
	{
		LOG_WARN("elite", "{} Not enough empty stat slots on base item {} to inject all affix stats ({}/{} injected) for '{}' (elite_id {}).",
			NemesisConstants::LOG_PREFIX, selectedItemEntry, statsInjected, affixStats.size(), elite->GetName(), elite->GetEliteId());
	}

	// Register the new item template in the ObjectMgr in-memory cache.
	// Copy the base template and modify the relevant fields.
	{
		auto* itemStore = const_cast<ItemTemplateContainer*>(sObjectMgr->GetItemTemplateStore());
		ItemTemplate& newTemplate = (*itemStore)[newEntry];
		newTemplate = *baseTemplate;
		newTemplate.ItemId = newEntry;
		newTemplate.Name1 = affixedName;
		newTemplate.Bonding = 2; // Bind on Equip
		newTemplate.Description = flavorText;
		newTemplate.RandomProperty = 0;
		newTemplate.RandomSuffix = 0;

		// Inject affix stats into the in-memory template.
		uint32 memStatsInjected = 0;
		for (uint32 affixIndex = 0; affixIndex < affixStats.size() && memStatsInjected < emptyStatSlots.size(); ++affixIndex)
		{
			uint32 slotIndex = emptyStatSlots[memStatsInjected];
			newTemplate.ItemStat[slotIndex].ItemStatType = affixStats[affixIndex].statType;
			newTemplate.ItemStat[slotIndex].ItemStatValue = affixStats[affixIndex].statValue;
			++memStatsInjected;
		}

		// Apply bonus armor to the in-memory template.
		if (bonusArmor > 0)
			newTemplate.Armor += bonusArmor;

		// Recount stats for the client tooltip.
		newTemplate.StatsCount = 0;
		for (uint32 statIndex = 0; statIndex < MAX_ITEM_PROTO_STATS; ++statIndex)
		{
			if (newTemplate.ItemStat[statIndex].ItemStatType != 0)
				newTemplate.StatsCount++;
		}

		// Register in the fast lookup vector so GetItemTemplate() can find it.
		// The fast store is the primary lookup path in AzerothCore — entries beyond
		// the vector's size return nullptr without falling back to the map store.
		auto* fastStore = const_cast<std::vector<ItemTemplate*>*>(sObjectMgr->GetItemTemplateStoreFast());
		if (fastStore->size() <= newEntry)
			fastStore->resize(static_cast<size_t>(newEntry) + 1, nullptr);
		(*fastStore)[newEntry] = &newTemplate;

		// DEBUG: verify the template is findable after cache insertion
		ItemTemplate const* verifyTemplate = sObjectMgr->GetItemTemplate(newEntry);
		LOG_INFO("elite", "{} Cache verify for item entry {}: {}", NemesisConstants::LOG_PREFIX, newEntry, verifyTemplate ? "FOUND" : "NOT FOUND");
	}

	// 7. Insert the affix tracking row
	// Delete any stale tracking row before inserting fresh data.
	WorldDatabase.DirectExecute("DELETE FROM `nemesis_item_affixes` WHERE `item_entry` = {}", newEntry);
	WorldDatabase.DirectExecute(
		"INSERT INTO `nemesis_item_affixes` (`item_entry`, `elite_id`, `prefix_trait`, `suffix_trait`, `quality_tier`) VALUES ({}, {}, {}, {}, {})",
		newEntry, elite->GetEliteId(), prefixTrait, suffixTrait, static_cast<uint32>(qualityTier));

	// 8. Inject the affixed item into the creature's loot.
	//    AzerothCore pre-generates the Loot object before OnPlayerCreatureKill fires,
	//    so adding to the LootTemplate is too late — the Loot is already populated.
	//    Instead, inject directly into creature->loot using AddItem, which appends
	//    to the already-generated loot. Group loot (Need/Greed/Pass) still works
	//    because the Loot object is shared across all group members.
	//
	//    The DB INSERT into creature_loot_template is kept so the affix item is
	//    discoverable on restart if the corpse somehow persists (edge case safety).
	{
		uint32 customLootEntry = elite->GetCustomEntry();
		if (customLootEntry != 0)
		{
			WorldDatabase.DirectExecute(
				"INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) "
				"VALUES ({}, {}, 0, 100, 0, 1, 0, 1, 1, 'Nemesis affix drop')", customLootEntry, newEntry);
		}

		// Inject directly into the creature's already-generated Loot object.
		LootStoreItem affixLootItem(newEntry, 0, 100.0f, false, 1, 0, 1, 1);
		creature->loot.AddItem(affixLootItem);
	}

	// 9. Log the generation
	LOG_INFO("elite", "{} Generated affixed loot for '{}' (elite_id {}): item entry {} (base: {}), tier {}, prefix 0x{:X}, suffix 0x{:X}",
		NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), newEntry, selectedItemEntry, static_cast<uint32>(qualityTier), prefixTrait, suffixTrait);
}

void NemesisEliteAffixManager::ValidatePlayerAffixCaches(uint32 diff)
{
	_affixCacheValidationTimer += diff;
	if (_affixCacheValidationTimer < 1000)
		return;
	_affixCacheValidationTimer = 0;

	for (auto& [playerGuid, entries] : _playerAffixCache)
	{
		Player* player = ObjectAccessor::FindPlayerByLowGUID(playerGuid.GetCounter());
		if (!player)
			continue;

		bool needsRefresh = false;
		for (auto const& entry : entries)
		{
			bool found = false;
			for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
			{
				if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
				{
					if (item->GetEntry() == entry.itemEntry)
					{
						found = true;
						break;
					}
				}
			}
			if (!found)
			{
				needsRefresh = true;
				break;
			}
		}

		if (needsRefresh)
		{
			LOG_INFO("elite", "{} Affix cache mismatch detected for {} — rebuilding.", NemesisConstants::LOG_PREFIX, player->GetName());
			PopulatePlayerAffixCache(player);
		}
	}
}
