#include "NemesisEliteCreature.h"
#include "NemesisEliteManager.h"
#include "CellImpl.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "GameObject.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "QueryResult.h"
#include "Log.h"
#include "LootMgr.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Unit.h"
#include <ctime>

// Column order used in every SELECT against nemesis_elite.
// 0=elite_id 1=creature_guid 2=creature_entry 3=name 4=effective_level 5=threat_score
// 6=kill_count 7=unique_kill_count 8=survival_attempts 9=origin_traits 10=earned_traits
// 11=origin_player_guid 12=umbral_moon_origin 13=last_map_id 14=last_zone_id
// 15=last_pos_x 16=last_pos_y 17=last_pos_z 18=promoted_at 19=last_seen_at
// 20=custom_entry 21=last_age_level_at 22=killed_faction_mask 23=last_area_id
#define NEMESIS_ELITE_SELECT_COLS \
    "elite_id, creature_guid, creature_entry, name, effective_level, threat_score, " \
    "kill_count, unique_kill_count, survival_attempts, origin_traits, earned_traits, origin_player_guid, " \
    "umbral_moon_origin, last_map_id, last_zone_id, last_pos_x, last_pos_y, last_pos_z, promoted_at, last_seen_at, " \
    "custom_entry, last_age_level_at, killed_faction_mask, last_area_id"

// SELECT that matches the exact field order expected by ObjectMgr::LoadCreatureTemplate().
// Used when registering a freshly-inserted custom entry into the in-memory template cache.
// On server restart the normal LoadCreatureTemplates() picks up all rows automatically.
#define NEMESIS_CT_LOAD_QUERY \
    "SELECT ct.entry, ct.difficulty_entry_1, ct.difficulty_entry_2, ct.difficulty_entry_3, " \
    "ct.KillCredit1, ct.KillCredit2, ct.name, ct.subname, ct.IconName, ct.gossip_menu_id, " \
    "ct.minlevel, ct.maxlevel, ct.exp, ct.faction, ct.npcflag, " \
    "ct.speed_walk, ct.speed_run, ct.speed_swim, ct.speed_flight, ct.detection_range, " \
    "ct.scale, ct.`rank`, ct.dmgschool, ct.DamageModifier, ct.BaseAttackTime, ct.RangeAttackTime, " \
    "ct.BaseVariance, ct.RangeVariance, ct.unit_class, ct.unit_flags, ct.unit_flags2, ct.dynamicflags, " \
    "ct.family, ct.type, ct.type_flags, ct.lootid, ct.pickpocketloot, ct.skinloot, " \
    "ct.PetSpellDataId, ct.VehicleId, ct.mingold, ct.maxgold, ct.AIName, ct.MovementType, " \
    "ctm.Ground, ctm.Swim, ctm.Flight, ctm.Rooted, ctm.Chase, ctm.Random, ctm.InteractionPauseTimer, " \
    "ct.HoverHeight, ct.HealthModifier, ct.ManaModifier, ct.ArmorModifier, ct.ExperienceModifier, " \
    "ct.RacialLeader, ct.movementId, ct.RegenHealth, ct.mechanic_immune_mask, ct.spell_school_immune_mask, " \
    "ct.flags_extra, ct.ScriptName " \
    "FROM creature_template ct " \
    "LEFT JOIN creature_template_movement ctm ON ct.entry = ctm.CreatureId " \
    "WHERE ct.entry = {}"

/*static*/ std::shared_ptr<NemesisEliteCreature> NemesisEliteCreature::FromRow(Field* fields)
{
    auto eliteCreature = std::shared_ptr<NemesisEliteCreature>(new NemesisEliteCreature());
    eliteCreature->_eliteId = fields[0].Get<uint32>();
    eliteCreature->_spawnId = fields[1].Get<uint32>();
    eliteCreature->_creatureEntry = fields[2].Get<uint32>();
    eliteCreature->_name = fields[3].Get<std::string>();
    eliteCreature->_effectiveLevel = fields[4].Get<uint8>();
    eliteCreature->_threatScore = fields[5].Get<uint32>();
    eliteCreature->_killCount = fields[6].Get<uint32>();
    eliteCreature->_uniqueKillCount = fields[7].Get<uint32>();
    eliteCreature->_survivalAttempts = fields[8].Get<uint32>();
    eliteCreature->_originTrait = fields[9].Get<uint32>();
    eliteCreature->_earnedTraits = fields[10].Get<uint32>();
    eliteCreature->_originPlayerGuid = fields[11].Get<uint32>();
    eliteCreature->_umbralMoonOrigin = fields[12].Get<uint8>() != 0;
    eliteCreature->_lastMapId = fields[13].Get<uint32>();
    eliteCreature->_lastZoneId = fields[14].Get<uint16>();
    eliteCreature->_lastPosX = fields[15].Get<float>();
    eliteCreature->_lastPosY = fields[16].Get<float>();
    eliteCreature->_lastPosZ = fields[17].Get<float>();
    eliteCreature->_promotedAt = fields[18].Get<uint32>();
    eliteCreature->_lastSeenAt = fields[19].Get<uint32>();
    eliteCreature->_customEntry = fields[20].Get<uint32>();
    eliteCreature->_lastAgeLevelAt = fields[21].Get<uint32>();
    // If the column was added after promotion, fall back to promoted_at so the
    // first age tick doesn't fire immediately on an old row.
    if (eliteCreature->_lastAgeLevelAt == 0)
        eliteCreature->_lastAgeLevelAt = eliteCreature->_promotedAt;
    eliteCreature->_killedFactionMask = fields[22].Get<uint8>();
    eliteCreature->_lastAreaId = fields[23].Get<uint16>();
    return eliteCreature;
}

/*static*/ std::vector<std::shared_ptr<NemesisEliteCreature>> NemesisEliteCreature::LoadAllAlive()
{
    std::vector<std::shared_ptr<NemesisEliteCreature>> elites;

    QueryResult result = WorldDatabase.Query("SELECT " NEMESIS_ELITE_SELECT_COLS " FROM nemesis_elite WHERE is_alive = 1");
    if (!result)
        return elites;

    do
        elites.push_back(FromRow(result->Fetch()));
    while (result->NextRow());

    return elites;
}

/*static*/ std::shared_ptr<NemesisEliteCreature> NemesisEliteCreature::LoadAliveBySpawnId(uint32 spawnId)
{
    QueryResult result = WorldDatabase.Query("SELECT " NEMESIS_ELITE_SELECT_COLS " FROM nemesis_elite WHERE creature_guid = {} AND is_alive = 1", spawnId);
    if (!result)
        return nullptr;

    return FromRow(result->Fetch());
}

/*static*/ std::shared_ptr<NemesisEliteCreature> NemesisEliteCreature::Insert(uint32 spawnId, uint32 creatureEntry, std::string name, uint8 effectiveLevel, uint32 threatScore, uint32 originTraits, uint32 originPlayerGuid, bool umbralMoonOrigin, uint32 mapId, float posX, float posY, float posZ, uint16 zoneId, uint16 areaId)
{
    uint32 now = static_cast<uint32>(time(nullptr));

    // Escape the name so apostrophes and other special characters don't break the SQL queries below.
    std::string escapedName = name;
    WorldDatabase.EscapeString(escapedName);

    WorldDatabase.DirectExecute(
        "INSERT INTO `nemesis_elite` "
        "(`creature_guid`, `creature_entry`, `name`, `effective_level`, `threat_score`, `kill_count`, `unique_kill_count`, `survival_attempts`, "
        "`origin_traits`, `earned_traits`, `origin_player_guid`, `umbral_moon_origin`, "
        "`last_map_id`, `last_zone_id`, `last_area_id`, `last_pos_x`, `last_pos_y`, `last_pos_z`, `promoted_at`, `last_age_level_at`, `is_alive`) "
        "VALUES ({}, {}, '{}', {}, {}, 0, 0, 0, {}, 0, {}, {}, {}, {}, {}, {:.4f}, {:.4f}, {:.4f}, {}, {}, 1)",
        spawnId, creatureEntry, escapedName, static_cast<uint32>(effectiveLevel), threatScore,
        static_cast<uint32>(originTraits), originPlayerGuid,
        static_cast<uint32>(umbralMoonOrigin ? 1 : 0), mapId, static_cast<uint32>(zoneId), static_cast<uint32>(areaId), posX, posY, posZ, now, now);

    QueryResult idResult = WorldDatabase.Query("SELECT elite_id FROM nemesis_elite WHERE creature_guid = {} AND is_alive = 1 ORDER BY elite_id DESC LIMIT 1", spawnId);

    if (!idResult)
    {
        LOG_ERROR("elite", "{} Failed to read back elite_id after insert for spawnId {}.", NemesisConstants::LOG_PREFIX, spawnId);
        return nullptr;
    }

    uint32 eliteId = idResult->Fetch()[0].Get<uint32>();

    // Seed the area tracking table with the promotion area so the first area is never lost.
    if (areaId != 0)
        WorldDatabase.DirectExecute("INSERT IGNORE INTO `nemesis_elite_areas` (`elite_id`, `area_id`) VALUES ({}, {})", eliteId, static_cast<uint32>(areaId));

    // Compute the custom creature_template entry that carries the nemesis name
    // and effective level to the client (CMSG_CREATURE_QUERY uses this entry).
    uint32 customEntry = NemesisEliteConstants::ELITE_CUSTOM_ENTRY_BASE + eliteId;

    // Clone the base creature_template row into a new row for the custom entry.
    // We override three things: the entry ID, the display name, and the level range.
    // INSERT IGNORE is idempotent — if a prior crash left the row behind we skip it.
    WorldDatabase.DirectExecute(
        "INSERT IGNORE INTO `creature_template` "
        "(`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, "
        "`KillCredit1`, `KillCredit2`, `name`, `subname`, `IconName`, `gossip_menu_id`, "
        "`minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, "
        "`speed_walk`, `speed_run`, `speed_swim`, `speed_flight`, `detection_range`, "
        "`scale`, `rank`, `dmgschool`, `DamageModifier`, `BaseAttackTime`, `RangeAttackTime`, "
        "`BaseVariance`, `RangeVariance`, `unit_class`, `unit_flags`, `unit_flags2`, `dynamicflags`, "
        "`family`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, "
        "`PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, "
        "`HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`, `ExperienceModifier`, "
        "`RacialLeader`, `movementId`, `RegenHealth`, `mechanic_immune_mask`, `spell_school_immune_mask`, "
        "`flags_extra`, `ScriptName`, `VerifiedBuild`) "
        "SELECT "
        "{}, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, "
        "`KillCredit1`, `KillCredit2`, "
        "'{}', "                                   // name override
        "`subname`, `IconName`, `gossip_menu_id`, "
        "{}, {}, "                                 // minlevel, maxlevel override
        "`exp`, `faction`, `npcflag`, "
        "`speed_walk`, `speed_run`, `speed_swim`, `speed_flight`, `detection_range`, "
        "`scale`, `rank`, `dmgschool`, `DamageModifier`, `BaseAttackTime`, `RangeAttackTime`, "
        "`BaseVariance`, `RangeVariance`, `unit_class`, `unit_flags`, `unit_flags2`, `dynamicflags`, "
        "`family`, `type`, `type_flags`, {}, `pickpocketloot`, `skinloot`, "
        "`PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, "
        "`HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`, `ExperienceModifier`, "
        "`RacialLeader`, `movementId`, `RegenHealth`, `mechanic_immune_mask`, `spell_school_immune_mask`, "
        "`flags_extra`, `ScriptName`, `VerifiedBuild` "
        "FROM `creature_template` WHERE `entry` = {}",
        customEntry, escapedName,
        static_cast<uint32>(effectiveLevel), static_cast<uint32>(effectiveLevel),
        customEntry,
        creatureEntry);

    // Clone all model rows so InitEntry() can find a valid display ID.
        WorldDatabase.DirectExecute("INSERT IGNORE INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`) SELECT {}, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability` FROM `creature_template_model` WHERE `CreatureID` = {}", customEntry, creatureEntry);

    // Copy the base creature's loot table into a private loot table keyed by customEntry.
    // This gives the nemesis its own lootid so affixed items can be injected per-nemesis
    // without polluting the shared loot table.
    // Look up the original creature's actual lootid (falls back to creatureEntry if lootid is 0).
    uint32 originalLootId = 0;
    {
        QueryResult lootIdResult = WorldDatabase.Query("SELECT `lootid` FROM `creature_template` WHERE `entry` = {}", creatureEntry);
        if (lootIdResult)
            originalLootId = lootIdResult->Fetch()[0].Get<uint32>();
        if (originalLootId == 0)
            originalLootId = creatureEntry;
    }
    WorldDatabase.DirectExecute("INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) SELECT {}, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment` FROM `creature_loot_template` WHERE `Entry` = {}", customEntry, originalLootId);

    // Register the private loot table in the in-memory LootStore so the loot system
    // can find it without a server restart. On restart, LoadAndCollectLootIds reads
    // the DB rows automatically.
    {
        LootTemplate* lootTemplate = new LootTemplate();
        QueryResult lootRows = WorldDatabase.Query(
            "SELECT `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount` "
            "FROM `creature_loot_template` WHERE `Entry` = {}", customEntry);
        if (lootRows)
        {
            do
            {
                Field* lootFields = lootRows->Fetch();
                uint32 itemId = lootFields[0].Get<uint32>();
                int32 reference = lootFields[1].Get<int32>();
                float chance = lootFields[2].Get<float>();
                bool needsQuest = lootFields[3].Get<bool>();
                uint16 lootMode = lootFields[4].Get<uint16>();
                uint8 groupId = lootFields[5].Get<uint8>();
                int32 minCount = lootFields[6].Get<int32>();
                uint8 maxCount = lootFields[7].Get<uint8>();
                lootTemplate->AddEntry(new LootStoreItem(itemId, reference, chance, needsQuest, lootMode, groupId, minCount, maxCount));
            } while (lootRows->NextRow());
        }
        LootTemplates_Creature.AddLootTemplate(customEntry, lootTemplate);
    }

        // Persist the custom entry reference so it survives restarts.
        WorldDatabase.DirectExecute("UPDATE `nemesis_elite` SET `custom_entry` = {} WHERE `elite_id` = {}", customEntry, eliteId);
    // Register the new template in the ObjectMgr in-memory cache for the current
    // session. On a clean restart, LoadCreatureTemplates() picks it up automatically.
    QueryResult ctResult = WorldDatabase.Query(NEMESIS_CT_LOAD_QUERY, customEntry);
    if (ctResult)
    {
        sObjectMgr->LoadCreatureTemplate(ctResult->Fetch());

        // LoadCreatureTemplate populates the template struct but leaves Models empty.
        // Populate model data separately, mirroring LoadCreatureTemplateModels().
        QueryResult modelResult = WorldDatabase.Query("SELECT `CreatureID`, `CreatureDisplayID`, `DisplayScale`, `Probability` FROM `creature_template_model` WHERE `CreatureID` = {} ORDER BY `Idx` ASC", customEntry);

        if (modelResult)
        {
            if (CreatureTemplate const* tmpl = sObjectMgr->GetCreatureTemplate(customEntry))
            {
                do
                {
                    Field* modelFields = modelResult->Fetch();
                    const_cast<CreatureTemplate*>(tmpl)->Models.emplace_back(modelFields[1].Get<uint32>(), modelFields[2].Get<float>(), modelFields[3].Get<float>());
                } while (modelResult->NextRow());

                sObjectMgr->CheckCreatureTemplate(tmpl);
                const_cast<CreatureTemplate*>(tmpl)->InitializeQueryData();
            }
        }
        else
        {
            LOG_WARN("elite", "{} No creature_template_model rows found for custom entry {} — client may see no model.", NemesisConstants::LOG_PREFIX, customEntry);
        }
    }
    else
    {
        LOG_ERROR("elite", "{} Failed to read back creature_template row for custom entry {} (elite_id {}). Client nameplate will show original mob name.", NemesisConstants::LOG_PREFIX, customEntry, eliteId);
        customEntry = 0;
    }

    auto eliteCreature = std::shared_ptr<NemesisEliteCreature>(new NemesisEliteCreature());
    eliteCreature->_eliteId = eliteId;
    eliteCreature->_spawnId = spawnId;
    eliteCreature->_creatureEntry = creatureEntry;
    eliteCreature->_name = std::move(name);
    eliteCreature->_effectiveLevel = effectiveLevel;
    eliteCreature->_threatScore = threatScore;
    eliteCreature->_killCount = 0;
    eliteCreature->_uniqueKillCount = 0;
    eliteCreature->_survivalAttempts = 0;
    eliteCreature->_originTrait = originTraits;
    eliteCreature->_earnedTraits = 0;
    eliteCreature->_originPlayerGuid = originPlayerGuid;
    eliteCreature->_umbralMoonOrigin = umbralMoonOrigin;
    eliteCreature->_lastMapId = mapId;
    eliteCreature->_lastPosX = posX;
    eliteCreature->_lastPosY = posY;
    eliteCreature->_lastPosZ = posZ;
    eliteCreature->_lastZoneId = zoneId;
    eliteCreature->_lastAreaId = areaId;
    eliteCreature->_promotedAt = now;
    eliteCreature->_lastAgeLevelAt = now;
    eliteCreature->_killedFactionMask = 0;
    eliteCreature->_customEntry = customEntry;
    return eliteCreature;
}

void NemesisEliteCreature::ApplyStats(Creature* creature) const
{
    float currentHpPercent = creature->GetHealthPct();

    // Switch the creature to the cloned template so the client sees the nemesis name
    // and correct effective level via CMSG_CREATURE_QUERY.
    // UpdateEntry internally calls SelectLevel (which reads minlevel/maxlevel from the
    // cloned template) and preserves absolute health; we restore HP% afterwards.
    //
    // Remove the grow aura BEFORE UpdateEntry. UpdateEntry resets ObjectScale via
    // InitEntry/SetObjectScale, which bypasses the aura system. If the grow aura
    // survives RemoveAllAuras (e.g. as a persistent/passive aura) its scale modifier
    // becomes desynced from the actual OBJECT_FIELD_SCALE_X value. RefreshAuras would
    // then see HasAura==true and return early, leaving the creature at original size.
    // Removing it here while the scale is still grow-modified ensures the unapply math
    // is correct, and RefreshAuras will always re-add it cleanly afterwards.
    if (creature->HasAura(NemesisEliteConstants::ELITE_GROW_SPELL))
        creature->RemoveAura(NemesisEliteConstants::ELITE_GROW_SPELL);

    if (_customEntry != 0)
    {
        creature->UpdateEntry(_customEntry);

        // SelectLevel inside UpdateEntry may have used a stale minlevel/maxlevel if
        // effective_level was changed in the DB since promotion. Override explicitly.
        creature->SetLevel(_effectiveLevel);

        // Keep the cloned template's level range in sync so future UpdateEntry calls
        // and server restarts produce the correct level without needing this override.
        WorldDatabase.Execute("UPDATE `creature_template` SET `minlevel` = {}, `maxlevel` = {} WHERE `entry` = {}", static_cast<uint32>(_effectiveLevel), static_cast<uint32>(_effectiveLevel), _customEntry);
    }

    creature->ApplyStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, sEliteMgr->GetScalingConfig().hpBoostPercent);
    creature->ApplyStatPctModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, sEliteMgr->GetScalingConfig().damageBoostPercent);
    creature->ApplyStatPctModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, sEliteMgr->GetScalingConfig().damageBoostPercent);
    creature->ApplyStatPctModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_PCT, sEliteMgr->GetScalingConfig().damageBoostPercent);

    creature->UpdateAllStats();
    creature->UpdateMaxHealth();
    creature->SetHealth(creature->CountPctFromMaxHealth(currentHpPercent));
    creature->UpdateDamagePhysical(BASE_ATTACK);
    creature->UpdateDamagePhysical(OFF_ATTACK);
    creature->UpdateDamagePhysical(RANGED_ATTACK);

    // If the custom entry is missing (template clone failed), fall back to server-side
    // name only. Client nameplates won't reflect it but internal log messages will.
    if (_customEntry == 0)
        creature->SetName(_name);

    LOG_DEBUG("elite", "{} Stats applied to '{}' (spawnId {}, effectiveLevel {}, customEntry {}).", NemesisConstants::LOG_PREFIX, _name, _spawnId, _effectiveLevel, _customEntry);
}

void NemesisEliteCreature::RefreshAuras(Creature* creature) const
{
    // --- GROW aura (visual size scaling based on effective level) ---
    uint8 stacks = GetGrowStacks();

    if (Aura* existingAura = creature->GetAura(NemesisEliteConstants::ELITE_GROW_SPELL))
    {
        // Aura is present — only act if the stack count is wrong.
        if (existingAura->GetStackAmount() != stacks)
        {
            // Stack count changed (e.g. level crossed a 10-level threshold).
            // Remove and re-add so the scale modifier is applied cleanly from the
            // current native scale rather than accumulated on top of a stale value.
            creature->RemoveAura(NemesisEliteConstants::ELITE_GROW_SPELL);

            if (stacks > 0)
            {
                if (Aura* growAura = creature->AddAura(NemesisEliteConstants::ELITE_GROW_SPELL, creature))
                    growAura->SetStackAmount(stacks);
            }
        }
    }
    else if (stacks > 0)
    {
        if (Aura* growAura = creature->AddAura(NemesisEliteConstants::ELITE_GROW_SPELL, creature))
            growAura->SetStackAmount(stacks);
    }

    // --- Underdog aura (permanent expertise buff) ---
    if ((_originTrait & ORIGIN_TRAIT_UNDERDOG) && !creature->HasAura(NemesisEliteConstants::ELITE_AURA_UNDERDOG))
        creature->AddAura(NemesisEliteConstants::ELITE_AURA_UNDERDOG, creature);

    // --- Ironbreaker aura (permanent armor penetration buff) ---
    if ((_originTrait & ORIGIN_TRAIT_IRONBREAKER) && !creature->HasAura(NemesisEliteConstants::ELITE_AURA_IRONBREAKER))
        creature->AddAura(NemesisEliteConstants::ELITE_AURA_IRONBREAKER, creature);

    // --- Notorious aura (permanent damage% + flat HP buff, level-scaled) ---
    if ((_earnedTraits & EARNED_TRAIT_NOTORIOUS) && !creature->HasAura(NemesisEliteConstants::ELITE_AURA_NOTORIOUS))
    {
        int32 damagePercent = static_cast<int32>(sEliteMgr->GetTraitConfig().notoriousDamageBasePercent + (static_cast<float>(_effectiveLevel) * sEliteMgr->GetTraitConfig().notoriousDamagePerLevelPercent));
        int32 healthBonus = static_cast<int32>(sEliteMgr->GetTraitConfig().notoriousHealthBase + (static_cast<float>(_effectiveLevel) * sEliteMgr->GetTraitConfig().notoriousHealthPerLevel));
        if (Aura* aura = creature->AddAura(NemesisEliteConstants::ELITE_AURA_NOTORIOUS, creature))
        {
            if (AuraEffect* damageEffect = aura->GetEffect(0))
            {
                damageEffect->ChangeAmount(damagePercent);
                LOG_INFO("elite", "{} RefreshAuras '{}' (spawnId {}) — NOTORIOUS aura E0 damage% ChangeAmount({}), auraType={}", NemesisConstants::LOG_PREFIX, _name, _spawnId, damagePercent, static_cast<uint32>(damageEffect->GetAuraType()));
            }
            if (AuraEffect* healthEffect = aura->GetEffect(1))
            {
                healthEffect->ChangeAmount(healthBonus);
                LOG_INFO("elite", "{} RefreshAuras '{}' (spawnId {}) — NOTORIOUS aura E1 health ChangeAmount({}), auraType={}", NemesisConstants::LOG_PREFIX, _name, _spawnId, healthBonus, static_cast<uint32>(healthEffect->GetAuraType()));
            }
            else
            {
                LOG_ERROR("elite", "{} RefreshAuras '{}' (spawnId {}) — NOTORIOUS aura GetEffect(1) returned NULL — DBC spell {} is missing Effect 2", NemesisConstants::LOG_PREFIX, _name, _spawnId, NemesisEliteConstants::ELITE_AURA_NOTORIOUS);
            }
        }
        else
        {
            LOG_ERROR("elite", "{} RefreshAuras '{}' (spawnId {}) — NOTORIOUS AddAura FAILED for spell {}", NemesisConstants::LOG_PREFIX, _name, _spawnId, NemesisEliteConstants::ELITE_AURA_NOTORIOUS);
        }
    }

    // --- Survivor aura (permanent armor + crit reduction buff, level-scaled) ---
    if ((_earnedTraits & EARNED_TRAIT_SURVIVOR) && !creature->HasAura(NemesisEliteConstants::ELITE_AURA_SURVIVOR))
    {
        int32 armorValue = static_cast<int32>(sEliteMgr->GetTraitConfig().survivorArmorBase + (static_cast<float>(_effectiveLevel) * sEliteMgr->GetTraitConfig().survivorArmorPerLevel));
        int32 critReduction = -static_cast<int32>(sEliteMgr->GetTraitConfig().survivorCritReductionBase + (static_cast<float>(_effectiveLevel) * sEliteMgr->GetTraitConfig().survivorCritReductionPerLevel));
        if (Aura* aura = creature->AddAura(NemesisEliteConstants::ELITE_AURA_SURVIVOR, creature))
        {
            if (AuraEffect* armorEffect = aura->GetEffect(0))
            {
                armorEffect->ChangeAmount(armorValue);
                LOG_INFO("elite", "{} RefreshAuras '{}' (spawnId {}) — SURVIVOR aura E0 armor ChangeAmount({}), auraType={}", NemesisConstants::LOG_PREFIX, _name, _spawnId, armorValue, static_cast<uint32>(armorEffect->GetAuraType()));
            }
            if (AuraEffect* critEffect = aura->GetEffect(1))
            {
                critEffect->ChangeAmount(critReduction);
                LOG_INFO("elite", "{} RefreshAuras '{}' (spawnId {}) — SURVIVOR aura E1 critReduction ChangeAmount({}), auraType={}", NemesisConstants::LOG_PREFIX, _name, _spawnId, critReduction, static_cast<uint32>(critEffect->GetAuraType()));
            }
            else
            {
                LOG_ERROR("elite", "{} RefreshAuras '{}' (spawnId {}) — SURVIVOR aura GetEffect(1) returned NULL — DBC spell {} is missing Effect 2", NemesisConstants::LOG_PREFIX, _name, _spawnId, NemesisEliteConstants::ELITE_AURA_SURVIVOR);
            }
        }
        else
        {
            LOG_ERROR("elite", "{} RefreshAuras '{}' (spawnId {}) — SURVIVOR AddAura FAILED for spell {}", NemesisConstants::LOG_PREFIX, _name, _spawnId, NemesisEliteConstants::ELITE_AURA_SURVIVOR);
        }
    }

    // --- Blight aura (permanent shadow damage shield, level-scaled) ---
    if ((_earnedTraits & EARNED_TRAIT_BLIGHT) && !creature->HasAura(NemesisEliteConstants::ELITE_AURA_BLIGHT))
    {
        int32 damageValue = static_cast<int32>(sEliteMgr->GetTraitConfig().blightDamageBase + (static_cast<float>(_effectiveLevel) * sEliteMgr->GetTraitConfig().blightDamagePerLevel));
        if (Aura* aura = creature->AddAura(NemesisEliteConstants::ELITE_AURA_BLIGHT, creature))
        {
            if (AuraEffect* effect = aura->GetEffect(0))
            {
                effect->ChangeAmount(damageValue);
                LOG_INFO("elite", "{} RefreshAuras '{}' (spawnId {}) — BLIGHT aura applied, ChangeAmount({}) on effect 0, auraType={}", NemesisConstants::LOG_PREFIX, _name, _spawnId, damageValue, static_cast<uint32>(effect->GetAuraType()));
            }
        }
        else
        {
            LOG_ERROR("elite", "{} RefreshAuras '{}' (spawnId {}) — BLIGHT AddAura FAILED for spell {}", NemesisConstants::LOG_PREFIX, _name, _spawnId, NemesisEliteConstants::ELITE_AURA_BLIGHT);
        }
    }

    // --- Spellproof aura (permanent CC duration reduction) ---
    if ((_earnedTraits & EARNED_TRAIT_SPELLPROOF) && !creature->HasAura(NemesisEliteConstants::ELITE_AURA_SPELLPROOF))
    {
        int32 reductionPercent = -static_cast<int32>(sEliteMgr->GetTraitConfig().spellproofCcDurationReduction);
        if (Aura* aura = creature->AddAura(NemesisEliteConstants::ELITE_AURA_SPELLPROOF, creature))
        {
            if (AuraEffect* e0 = aura->GetEffect(0))
                e0->ChangeAmount(reductionPercent);
            if (AuraEffect* e1 = aura->GetEffect(1))
                e1->ChangeAmount(reductionPercent);
            if (AuraEffect* e2 = aura->GetEffect(2))
                e2->ChangeAmount(reductionPercent);
            LOG_INFO("elite", "{} RefreshAuras '{}' (spawnId {}) — SPELLPROOF aura applied, ChangeAmount({}) on all 3 effects", NemesisConstants::LOG_PREFIX, _name, _spawnId, reductionPercent);
        }
        else
        {
            LOG_ERROR("elite", "{} RefreshAuras '{}' (spawnId {}) — SPELLPROOF AddAura FAILED for spell {}", NemesisConstants::LOG_PREFIX, _name, _spawnId, NemesisEliteConstants::ELITE_AURA_SPELLPROOF);
        }
    }

    // --- Nomad aura (permanent movement speed buff) ---
    if ((_earnedTraits & EARNED_TRAIT_NOMAD) && !creature->HasAura(NemesisEliteConstants::ELITE_AURA_NOMAD))
    {
        int32 speedPercent = static_cast<int32>(sEliteMgr->GetTraitConfig().nomadSpeedPercent);
        if (Aura* aura = creature->AddAura(NemesisEliteConstants::ELITE_AURA_NOMAD, creature))
        {
            if (AuraEffect* effect = aura->GetEffect(0))
            {
                effect->ChangeAmount(speedPercent);
                LOG_INFO("elite", "{} RefreshAuras '{}' (spawnId {}) — NOMAD aura applied, ChangeAmount({}) on effect 0", NemesisConstants::LOG_PREFIX, _name, _spawnId, speedPercent);
            }
        }
        else
        {
            LOG_ERROR("elite", "{} RefreshAuras '{}' (spawnId {}) — NOMAD AddAura FAILED for spell {}", NemesisConstants::LOG_PREFIX, _name, _spawnId, NemesisEliteConstants::ELITE_AURA_NOMAD);
        }
    }
}

uint8 NemesisEliteCreature::GetGrowStacks() const
{
    uint32 divisor = sEliteMgr->GetScalingConfig().growLevelsPerStack;
    return divisor > 0 ? static_cast<uint8>(_effectiveLevel / divisor) : 0;
}

void NemesisEliteCreature::IncrementKillCount()
{
    ++_killCount;
    WorldDatabase.Execute("UPDATE nemesis_elite SET kill_count = {} WHERE elite_id = {}", _killCount, _eliteId);
}

void NemesisEliteCreature::IncrementUniqueKillCount()
{
    ++_uniqueKillCount;
    WorldDatabase.Execute("UPDATE nemesis_elite SET unique_kill_count = {} WHERE elite_id = {}", _uniqueKillCount, _eliteId);
}

void NemesisEliteCreature::SetUniqueKillCount(uint32 count)
{
    _uniqueKillCount = count;
    WorldDatabase.Execute("UPDATE nemesis_elite SET unique_kill_count = {} WHERE elite_id = {}", _uniqueKillCount, _eliteId);
}

void NemesisEliteCreature::IncrementSurvivalAttempts()
{
    ++_survivalAttempts;
    WorldDatabase.Execute("UPDATE nemesis_elite SET survival_attempts = {} WHERE elite_id = {}", _survivalAttempts, _eliteId);
}

void NemesisEliteCreature::AddEarnedTrait(uint32 trait)
{
    _earnedTraits |= trait;
    WorldDatabase.Execute("UPDATE nemesis_elite SET earned_traits = {} WHERE elite_id = {}", static_cast<uint32>(_earnedTraits), _eliteId);
}

void NemesisEliteCreature::RemoveEarnedTrait(uint32 trait)
{
    _earnedTraits &= ~trait;
    WorldDatabase.Execute("UPDATE nemesis_elite SET earned_traits = {} WHERE elite_id = {}", static_cast<uint32>(_earnedTraits), _eliteId);
}

void NemesisEliteCreature::SetOriginTrait(uint32 trait)
{
    _originTrait = trait;
    WorldDatabase.Execute("UPDATE nemesis_elite SET origin_traits = {} WHERE elite_id = {}", static_cast<uint32>(_originTrait), _eliteId);
}

void NemesisEliteCreature::SetEffectiveLevel(uint8 level)
{
    _effectiveLevel = level;
    WorldDatabase.Execute("UPDATE nemesis_elite SET effective_level = {} WHERE elite_id = {}", static_cast<uint32>(level), _eliteId);
}

void NemesisEliteCreature::SetThreatScore(uint32 score)
{
    _threatScore = score;
    WorldDatabase.Execute("UPDATE nemesis_elite SET threat_score = {} WHERE elite_id = {}", score, _eliteId);
}

void NemesisEliteCreature::SetLastAgeLevelAt(uint32 timestamp)
{
    _lastAgeLevelAt = timestamp;
    WorldDatabase.Execute("UPDATE nemesis_elite SET last_age_level_at = {} WHERE elite_id = {}", timestamp, _eliteId);
}

void NemesisEliteCreature::SetKilledFactionMask(uint8 mask)
{
    _killedFactionMask = mask;
    WorldDatabase.Execute("UPDATE nemesis_elite SET killed_faction_mask = {} WHERE elite_id = {}", static_cast<uint32>(_killedFactionMask), _eliteId);
}

void NemesisEliteCreature::UpdateLastSeen(Creature* creature)
{
    uint32 now = static_cast<uint32>(time(nullptr));
    if (now - _lastSeenAt < 60)
        return;

    _lastSeenAt = now;
    _lastMapId = creature->GetMapId();
    _lastPosX = creature->GetPositionX();
    _lastPosY = creature->GetPositionY();
    _lastPosZ = creature->GetPositionZ();

    uint16 newZoneId = static_cast<uint16>(creature->GetZoneId());
    _lastZoneId = newZoneId;

    uint16 newAreaId = static_cast<uint16>(creature->GetAreaId());
    bool areaChanged = (newAreaId != _lastAreaId);
    _lastAreaId = newAreaId;

    WorldDatabase.Execute("UPDATE nemesis_elite SET last_map_id = {}, last_pos_x = {:.4f}, last_pos_y = {:.4f}, last_pos_z = {:.4f}, last_zone_id = {}, last_area_id = {}, last_seen_at = {} WHERE elite_id = {}", _lastMapId, _lastPosX, _lastPosY, _lastPosZ, static_cast<uint32>(_lastZoneId), static_cast<uint32>(_lastAreaId), _lastSeenAt, _eliteId);

    // Nomad: track every distinct area this elite enters.
    if (areaChanged && newAreaId != 0 && !(_earnedTraits & EARNED_TRAIT_NOMAD))
    {
        WorldDatabase.DirectExecute("INSERT IGNORE INTO `nemesis_elite_areas` (`elite_id`, `area_id`) VALUES ({}, {})", _eliteId, static_cast<uint32>(newAreaId));

        QueryResult areaCountResult = WorldDatabase.Query("SELECT COUNT(*) FROM `nemesis_elite_areas` WHERE `elite_id` = {}", _eliteId);
        if (areaCountResult)
        {
            uint32 areaCount = areaCountResult->Fetch()[0].Get<uint32>();
            if (areaCount >= sEliteMgr->GetTraitConfig().nomadAreaThreshold)
                sEliteMgr->GrantNomadTrait(_spawnId);
        }
    }

    // Studious: grant if a chest is found nearby (checked once per minute alongside the position update).
    if (!(_earnedTraits & EARNED_TRAIT_STUDIOUS))
    {
        struct ChestInRangeCheck
        {
            WorldObject const* _source;
            float _range;
            ChestInRangeCheck(WorldObject const* source, float range) : _source(source), _range(range) {}
            bool operator()(GameObject* go) const
            {
                return go->GetGoType() == GAMEOBJECT_TYPE_CHEST && _source->IsWithinDistInMap(go, _range);
            }
        };

        float searchRange = sEliteMgr->GetTraitConfig().studiousSearchRange;
        GameObject* nearestChest = nullptr;
        ChestInRangeCheck check(creature, searchRange);
        Acore::GameObjectSearcher<ChestInRangeCheck> searcher(creature, nearestChest, check);
        Cell::VisitObjects(creature, searcher, searchRange);

        if (nearestChest)
            sEliteMgr->GrantStudiousTrait(_spawnId);
    }
}

void NemesisEliteCreature::MarkDead(uint32 killerGuid) const
{
    uint32 now = static_cast<uint32>(time(nullptr));

    if (killerGuid != 0)
    {
        WorldDatabase.Execute("UPDATE nemesis_elite SET is_alive = 0, killed_at = {}, killed_by_player_guid = {} WHERE elite_id = {}", now, killerGuid, _eliteId);
    }
    else
    {
        WorldDatabase.Execute("UPDATE nemesis_elite SET is_alive = 0, killed_at = {} WHERE elite_id = {}", now, _eliteId);
    }

    // Remove the cloned creature_template rows so dead nemeses don't leave orphaned
    // entries in the world DB. The ObjectMgr in-memory cache retains them until the
    // next server restart, which is harmless since the creature won't respawn.
    if (_customEntry != 0)
    {
        // DO NOT remove the in-memory LootTemplate here. AzerothCore generates loot
        // lazily — FillLoot runs when the player opens the corpse, not when the creature
        // dies. If we delete the template now, the loot window will be empty because
        // FillLoot can't find the lootid. The in-memory template is cleaned up on server
        // shutdown by LootStore::Clear(). The DB rows are cleaned up by the DELETE below
        // (immediate) and by the startup cleanup in NemesisEliteWorldScript (safety net).
        //
        // LootTemplates_Creature.RemoveLootTemplate(_customEntry);

        WorldDatabase.Execute("DELETE FROM `creature_loot_template` WHERE `Entry` = {}", _customEntry);
        WorldDatabase.Execute("DELETE FROM `creature_template_model` WHERE `CreatureID` = {}", _customEntry);
        WorldDatabase.Execute("DELETE FROM `creature_template` WHERE `entry` = {}", _customEntry);
    }
}