#include "NemesisEliteManager.h"
#include "NemesisEliteHelpers.h"
#include "NemesisEliteTraitBehavior.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "LootMgr.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "SharedDefines.h"
#include "Unit.h"
#include "NemesisRevengeManager.h"
#include "NemesisUmbralMoonManager.h"
#include "CreatureAI.h"
#include "ObjectAccessor.h"
#include "SpellAuraEffects.h"
#include <ctime>
#include <sstream>

NemesisEliteManager::NemesisEliteManager()
{
    _traitBehavior = std::make_unique<NemesisEliteTraitBehavior>(*this);
}

NemesisEliteManager::~NemesisEliteManager() = default;

NemesisEliteManager* NemesisEliteManager::Instance()
{
    static NemesisEliteManager instance;
    return &instance;
}

void NemesisEliteManager::ReloadConfig()
{
    LOG_INFO("server.loading", "{} Loading configuration from nemesis_configuration...", NemesisConstants::LOG_PREFIX);

    _scalingConfig.promotionChanceNormal = std::max(0.0f, std::min(100.0f, std::stof(NemesisLoadConfigValue("elite_promotion_chance_normal", "7"))));
    _scalingConfig.promotionChanceUmbralMoon = std::max(0.0f, std::min(100.0f, std::stof(NemesisLoadConfigValue("elite_promotion_chance_umbral_moon", "25"))));
    _scalingConfig.hpBoostPercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_stat_hp_boost_percent", "25")));
    _scalingConfig.damageBoostPercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_stat_damage_boost_percent", "25")));
    _scalingConfig.levelGainOnPromote = static_cast<uint8>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_level_gain_on_promote", "3"))));
    _scalingConfig.levelGainPerKill = static_cast<uint8>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_level_gain_per_kill", "1"))));
    _scalingConfig.levelCap = static_cast<uint8>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_level_cap", "83"))));
    _scalingConfig.ageLevelIntervalHours = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_age_level_interval_hours", "24"))));
    _threatConfig.weightLevel = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_threat_weight_level", "5"))));
    _threatConfig.weightKill = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_threat_weight_kill", "1"))));
    _threatConfig.weightUniqueKill = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_threat_weight_unique_kill", "12"))));
    _threatConfig.weightTrait = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_threat_weight_trait", "15"))));
    _threatConfig.ageBonusCap = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_threat_age_bonus_cap", "100"))));
    _threatConfig.ageBonusPerDay = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_threat_age_bonus_per_day", "4"))));
    _scalingConfig.growLevelsPerStack = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_visual_grow_levels_per_stack", "10"))));

    _traitBehavior->LoadConfig();
    sAffixMgr->LoadConfig();

    LOG_INFO("server.loading", "{} Config loaded. Normal: {:.0f}%, UmbralMoon: {:.0f}%, HP boost: +{:.0f}%, Damage boost: +{:.0f}%, LevelPerKill: {}, LevelCap: {}, AgeLevelIntervalHours: {}", NemesisConstants::LOG_PREFIX, _scalingConfig.promotionChanceNormal, _scalingConfig.promotionChanceUmbralMoon, _scalingConfig.hpBoostPercent, _scalingConfig.damageBoostPercent, static_cast<uint32>(_scalingConfig.levelGainPerKill), static_cast<uint32>(_scalingConfig.levelCap), _scalingConfig.ageLevelIntervalHours);
}

void NemesisEliteManager::LoadConfig()
{
    ReloadConfig();
    LoadElitesFromDB();
}

void NemesisEliteManager::LoadElitesFromDB()
{
    _loadedElites.clear();
    _buffedElites.clear();

    auto elites = NemesisEliteCreature::LoadAllAlive();
    for (auto& elite : elites)
        _loadedElites[elite->GetSpawnId()] = elite;

    LOG_INFO("server.loading", "{} Loaded {} living elite(s) from DB.", NemesisConstants::LOG_PREFIX, _loadedElites.size());
}

float NemesisEliteManager::GetCurrentPromotionChance()
{
    return sUmbralMoonMgr->IsUmbralMoonActive() ? _scalingConfig.promotionChanceUmbralMoon : _scalingConfig.promotionChanceNormal;
}

bool NemesisEliteManager::IsEligibleCreature(Creature* creature)
{
    if (!creature || !creature->IsAlive())
        return false;

    if (creature->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE))
        return false;

    if (creature->IsCritter())
        return false;

    if (creature->IsVehicle() || creature->IsTaxi() || creature->IsGuard() || creature->IsVendor() || creature->IsTrainer() || creature->IsQuestGiver() || creature->IsBanker() || creature->IsInnkeeper() || creature->IsAuctioner())
        return false;

    if (creature->IsCreatedByPlayer())
        return false;

    // Dynamic spawns (no persistent entry in the creature table) can't be meaningfully persisted.
    if (creature->GetSpawnId() == 0)
        return false;

    if (!creature->GetMap()->IsWorldMap())
        return false;

    return true;
}

uint32 NemesisEliteManager::DetermineOriginTrait(Creature* creature, Player* playerKilled)
{
    std::vector<NemesisEliteOriginTrait> candidates;

    uint32 creatureLowGuid = creature->GetGUID().GetCounter();
    uint32 playerLowGuid = playerKilled->GetGUID().GetCounter();

    // Ambusher: this creature entered combat while the player was already being attacked
    // by at least one other unit. Checked at combat-entry time because getAttackers() is
    // cleared before OnPlayerKilledByCreature fires.
    if (IsAmbusherCandidate(creatureLowGuid, playerLowGuid))
        candidates.push_back(ORIGIN_TRAIT_AMBUSHER);
    ClearAmbusherCandidate(creatureLowGuid, playerLowGuid);

    // Executioner: this creature entered combat while the player was already at or below 20% HP.
    if (IsExecutionerCandidate(creatureLowGuid, playerLowGuid))
        candidates.push_back(ORIGIN_TRAIT_EXECUTIONER);
    ClearExecutionerCandidate(creatureLowGuid, playerLowGuid);

    uint8 playerClass = playerKilled->getClass();

    // Mage-Bane: killed a pure caster class.
    if (playerClass == CLASS_MAGE || playerClass == CLASS_WARLOCK)
        candidates.push_back(ORIGIN_TRAIT_MAGE_BANE);

    // Healer-Bane: killed a class with a healing specialization.
    if (playerClass == CLASS_PRIEST || playerClass == CLASS_PALADIN || playerClass == CLASS_DRUID || playerClass == CLASS_SHAMAN)
        candidates.push_back(ORIGIN_TRAIT_HEALER_BANE);

    auto const& traitConfig = _traitBehavior->GetTraitConfig();

    // Giant-Slayer / Underdog: mutually exclusive level-gap checks. Giant-Slayer takes priority.
    if (playerKilled->GetLevel() >= creature->GetLevel() + traitConfig.giantSlayerLevelGap)
        candidates.push_back(ORIGIN_TRAIT_GIANT_SLAYER);
    else if (playerKilled->GetLevel() >= creature->GetLevel() + traitConfig.underdogLevelGap)
        candidates.push_back(ORIGIN_TRAIT_UNDERDOG);

    // Opportunist: player was AFK when this creature first engaged them.
    if (IsOpportunistCandidate(creatureLowGuid, playerLowGuid))
        candidates.push_back(ORIGIN_TRAIT_OPPORTUNIST);
    ClearOpportunistCandidate(creatureLowGuid, playerLowGuid);

    // Umbralforged: promotion is happening during an Umbral Moon.
    if (sUmbralMoonMgr->IsUmbralMoonActive())
        candidates.push_back(ORIGIN_TRAIT_UMBRALFORGED);

    // Deathblow: this creature landed a single hit on a full-HP player that killed them.
    if (IsDeathblowCandidate(creatureLowGuid, playerLowGuid))
        candidates.push_back(ORIGIN_TRAIT_DEATHBLOW);
    ClearDeathblowCandidate(creatureLowGuid, playerLowGuid);

    // Scavenger: player had resurrection sickness when this creature engaged them.
    if (IsScavengerCandidate(creatureLowGuid, playerLowGuid))
        candidates.push_back(ORIGIN_TRAIT_SCAVENGER);
    ClearScavengerCandidate(creatureLowGuid, playerLowGuid);

    // Duelist: this creature engaged the player 1-on-1 as a humanoid.
    if (IsDuelistCandidate(creatureLowGuid, playerLowGuid))
        candidates.push_back(ORIGIN_TRAIT_DUELIST);
    ClearDuelistCandidate(creatureLowGuid, playerLowGuid);

    // Plunderer: killed a player carrying at least plundererGoldPerLevel copper per character level.
    if (playerKilled->GetMoney() >= static_cast<uint32>(playerKilled->GetLevel()) * traitConfig.plundererGoldPerLevel)
        candidates.push_back(ORIGIN_TRAIT_PLUNDERER);

    // Ironbreaker: killed a plate-wearing class (Warrior, Paladin, Death Knight).
    if (playerClass == CLASS_WARRIOR || playerClass == CLASS_PALADIN || playerClass == CLASS_DEATH_KNIGHT)
        candidates.push_back(ORIGIN_TRAIT_IRONBREAKER);

    // Profession traits: mutually exclusive with each other; one is chosen at random if multiple qualify.
    std::vector<NemesisEliteOriginTrait> professionCandidates;
    if (playerKilled->HasSkill(SKILL_SKINNING))
        professionCandidates.push_back(ORIGIN_TRAIT_SKINNER);

    if (playerKilled->HasSkill(SKILL_MINING))
        professionCandidates.push_back(ORIGIN_TRAIT_ORE_GORGED);

    if (playerKilled->HasSkill(SKILL_HERBALISM))
        professionCandidates.push_back(ORIGIN_TRAIT_ROOT_RIPPER);

    if (playerKilled->HasSkill(SKILL_BLACKSMITHING))
        professionCandidates.push_back(ORIGIN_TRAIT_FORGE_BREAKER);

    if (playerKilled->HasSkill(SKILL_LEATHERWORKING))
        professionCandidates.push_back(ORIGIN_TRAIT_HIDE_MANGLER);

    if (playerKilled->HasSkill(SKILL_TAILORING))
        professionCandidates.push_back(ORIGIN_TRAIT_THREAD_RIPPER);

    if (playerKilled->HasSkill(SKILL_ENGINEERING))
        professionCandidates.push_back(ORIGIN_TRAIT_GEAR_GRINDER);

    if (playerKilled->HasSkill(SKILL_ALCHEMY))
        professionCandidates.push_back(ORIGIN_TRAIT_VIAL_SHATTER);

    if (playerKilled->HasSkill(SKILL_ENCHANTING))
        professionCandidates.push_back(ORIGIN_TRAIT_RUNE_EATER);

    if (playerKilled->HasSkill(SKILL_JEWELCRAFTING))
        professionCandidates.push_back(ORIGIN_TRAIT_GEM_CRUSHER);

    if (playerKilled->HasSkill(SKILL_INSCRIPTION))
        professionCandidates.push_back(ORIGIN_TRAIT_INK_DRINKER);

    if (!professionCandidates.empty())
        candidates.push_back(professionCandidates[urand(0, static_cast<uint32>(professionCandidates.size() - 1))]);

    if (candidates.empty())
        return ORIGIN_TRAIT_NONE;

    return candidates[urand(0, static_cast<uint32>(candidates.size() - 1))];
}

std::string NemesisEliteManager::GenerateName(uint32 originTraits, const std::string& originalCreatureName)
{
    return std::string(NemesisEliteConstants::GetOriginTraitPrefix(originTraits)) + " " + originalCreatureName;
}

uint32 NemesisEliteManager::CalculateThreatScore(uint8 effectiveLevel, uint32 killCount, uint32 uniqueKillCount, uint32 originTraits, uint32 earnedTraits, uint32 promotedAt)
{
    // Formula: (level * weight_level) + (kills * weight_kill) + (uniqueKills * weight_unique_kill) + (traits * weight_trait) + ageBonus
    // ageBonus = min(elite_threat_age_bonus_cap, daysSincePromotion * elite_threat_age_bonus_per_day)
    // Unique kills are weighted far higher than raw kills to discourage farming (dying repeatedly to the same nemesis).
    uint32 score = static_cast<uint32>(effectiveLevel) * _threatConfig.weightLevel;
    score += killCount * _threatConfig.weightKill;
    score += uniqueKillCount * _threatConfig.weightUniqueKill;

    uint32 traitCount = 0;
    for (uint32 mask = originTraits; mask; mask >>= 1)
        traitCount += (mask & 1);
    for (uint32 mask = earnedTraits; mask; mask >>= 1)
        traitCount += (mask & 1);
    score += traitCount * _threatConfig.weightTrait;

    uint32 now = static_cast<uint32>(time(nullptr));
    uint32 daysSince = (now > promotedAt) ? (now - promotedAt) / 86400u : 0u;
    score += std::min(_threatConfig.ageBonusCap, daysSince * _threatConfig.ageBonusPerDay);

    return score;
}

void NemesisEliteManager::OnCreatureUpdate(Creature* creature)
{
    if (!creature)
        return;

    uint32 spawnId = creature->GetSpawnId();
    if (spawnId == 0)
        return;

    auto eliteIterator = _loadedElites.find(spawnId);
    if (eliteIterator == _loadedElites.end())
        return;

    // If the creature is dead but still tracked, it was killed by a non-player source
    // (faction fight, guards, environmental, etc.). Clean up combat state but keep the
    // elite in _loadedElites so it re-applies on respawn. Only player kills permanently
    // end a nemesis (handled in OnEliteKilledByPlayer).
    if (!creature->IsAlive())
    {
        _traitBehavior->ClearCombatState(spawnId, creature->GetMap());
        _buffedElites.erase(creature->GetGUID());
        return;
    }

    // Persist last known position (throttled to once per minute).
    eliteIterator->second->UpdateLastSeen(creature);

    // Stat boosts are applied only once per session (multiplying repeatedly would stack them).
    if (!_buffedElites.count(creature->GetGUID()))
    {
        // ApplyStats calls UpdateEntry which strips all auras, so RefreshAuras must run
        // after — not before — to avoid the grow aura being immediately removed.
        eliteIterator->second->ApplyStats(creature);
        _buffedElites.insert(creature->GetGUID());
        eliteIterator->second->RefreshAuras(creature);
        _traitBehavior->ApplyTraitBehaviors(creature, eliteIterator->second);
        return;
    }

    _traitBehavior->ProcessTraitTick(creature, eliteIterator->second, spawnId);
}

void NemesisEliteManager::OnEliteKilledByPlayer(Creature* creature, Player* killer)
{
    uint32 spawnId = creature->GetSpawnId();

    auto eliteIterator = _loadedElites.find(spawnId);
    if (eliteIterator == _loadedElites.end())
        return;

    // Clear combat state
    _traitBehavior->ClearCombatState(spawnId, creature->GetMap());

    // Generate affixed loot BEFORE marking dead (MarkDead deletes the creature_template clone)
    if (killer)
        sAffixMgr->GenerateAffixedLoot(creature, killer, eliteIterator->second);

    // Revenge system — check if the killer (or group) includes the origin player.
    // Must run before MarkDead/erase so the elite data is still available.
    if (killer)
        sRevengeMgr->OnNemesisKilledByPlayer(eliteIterator->second->GetEliteId(), eliteIterator->second->GetOriginPlayer(), killer);

    // Log event
    uint32 killerGuid = killer ? killer->GetGUID().GetCounter() : 0;
    LOG_INFO("elite", "{} '{}' (elite_id {}, spawnId {}) slain by {}.", NemesisConstants::LOG_PREFIX, eliteIterator->second->GetName(), eliteIterator->second->GetEliteId(), spawnId, killer ? killer->GetName() : "unknown");

    eliteIterator->second->MarkDead(killerGuid);
    _buffedElites.erase(creature->GetGUID());
    _loadedElites.erase(eliteIterator);
}

void NemesisEliteManager::OnNemesisKilledPlayer(Creature* creature, Player* victim)
{
    uint32 spawnId = creature->GetSpawnId();

    auto eliteIterator = _loadedElites.find(spawnId);
    if (eliteIterator == _loadedElites.end())
        return;

    auto& elite = eliteIterator->second;
    uint32 now = static_cast<uint32>(time(nullptr));
    uint32 playerGuid = victim->GetGUID().GetCounter();

    // Record the kill in the log table. DirectExecute so the uniqueness check below sees the new row.
    WorldDatabase.DirectExecute("INSERT INTO `nemesis_elite_kill_log` (`elite_id`, `player_guid`, `killed_at`) VALUES ({}, {}, {})", elite->GetEliteId(), playerGuid, now);

    // Unique kill check: if this is the first time this player has been killed by this nemesis, increment unique kill count.
    QueryResult uniqueCheckResult = WorldDatabase.Query("SELECT COUNT(*) FROM nemesis_elite_kill_log WHERE elite_id = {} AND player_guid = {}", elite->GetEliteId(), playerGuid);
    if (uniqueCheckResult && uniqueCheckResult->Fetch()[0].Get<uint32>() == 1)
        elite->IncrementUniqueKillCount();

    auto const& traitConfig = _traitBehavior->GetTraitConfig();

    // Blight: set the faction bit for the victim's team, grant trait when both factions have been killed.
    if (!(elite->GetEarnedTraits() & EARNED_TRAIT_BLIGHT))
    {
        uint8 factionBit = (victim->GetTeamId() == TEAM_HORDE) ? 2u : 1u;
        elite->SetKilledFactionMask(elite->GetKilledFactionMask() | factionBit);

        if ((elite->GetKilledFactionMask() & 3u) == 3u)
        {
            elite->AddEarnedTrait(EARNED_TRAIT_BLIGHT);

            uint32 blightThreat = CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
            elite->SetThreatScore(blightThreat);

            LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait BLIGHT | threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), blightThreat);
        }
    }

    // Increment kill count.
    elite->IncrementKillCount();

    // Check for Notorious trait: notoriousKillThreshold+ unique players killed.
    if (!(elite->GetEarnedTraits() & EARNED_TRAIT_NOTORIOUS))
    {
        uint32 uniqueKills = elite->GetUniqueKillCount();
        if (uniqueKills >= traitConfig.notoriousKillThreshold)
        {
            elite->AddEarnedTrait(EARNED_TRAIT_NOTORIOUS);

            uint32 notoriousThreat = CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
            elite->SetThreatScore(notoriousThreat);

            LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait NOTORIOUS | uniqueKills={} threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), uniqueKills, notoriousThreat);
        }
    }

    // Gain effective levels from the kill, capped at the configured ceiling.
    uint8 currentLevel = elite->GetEffectiveLevel();
    if (currentLevel < _scalingConfig.levelCap)
    {
        uint8 newLevel = static_cast<uint8>(std::min(static_cast<uint32>(_scalingConfig.levelCap), static_cast<uint32>(currentLevel) + static_cast<uint32>(_scalingConfig.levelGainPerKill)));
        elite->SetEffectiveLevel(newLevel);

        // Keep the cloned template in sync so the next UpdateEntry reads the right level.
        if (elite->GetCustomEntry() != 0)
            SyncCreatureTemplateLevel(elite->GetCustomEntry(), newLevel);
        // Erase from _buffedElites so the next OnCreatureUpdate tick fires ApplyStats,
        // which calls UpdateEntry (re-reads minlevel/maxlevel, runs SelectLevel, rebuilds
        // base HP/damage) then re-applies the percentage multipliers fresh.
        _buffedElites.erase(creature->GetGUID());
    }

    // Recalculate threat score with updated kill count and level.
    uint32 newThreat = CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
    elite->SetThreatScore(newThreat);

    LOG_INFO("elite", "{} '{}' (elite_id {}) killed player {} | kills={} level={} threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), victim->GetName(), elite->GetKillCount(), static_cast<uint32>(elite->GetEffectiveLevel()), newThreat);
}

bool NemesisEliteManager::PromoteToElite(Creature* creature, Player* playerKilled)
{
    if (!creature || !playerKilled)
        return false;

    if (!IsEligibleCreature(creature))
        return false;

    // Don't promote a creature that is already a nemesis.
    if (IsNemesisElite(creature->GetSpawnId()))
        return false;

    auto const& traitConfig = _traitBehavior->GetTraitConfig();

    std::string originalName = creature->GetName();
    uint32 originTraits = DetermineOriginTrait(creature, playerKilled);
    std::string name = GenerateName(originTraits, originalName);
    uint8 effectiveLevel = static_cast<uint8>(std::min(static_cast<uint32>(_scalingConfig.levelCap), static_cast<uint32>(creature->GetLevel()) + static_cast<uint32>(_scalingConfig.levelGainOnPromote)));
    uint32 now = static_cast<uint32>(time(nullptr));
    uint32 threatScore = CalculateThreatScore(effectiveLevel, 0, 0, originTraits, EARNED_TRAIT_NONE, now);
    bool umbralOrigin = (originTraits & ORIGIN_TRAIT_UMBRALFORGED) != 0;

    uint16 zoneId = static_cast<uint16>(creature->GetZoneId());
    uint16 areaId = static_cast<uint16>(creature->GetAreaId());
    auto elite = NemesisEliteCreature::Insert(creature->GetSpawnId(), creature->GetEntry(), name, effectiveLevel, threatScore, originTraits, playerKilled->GetGUID().GetCounter(), umbralOrigin, creature->GetMapId(), creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), zoneId, areaId);

    if (!elite)
        return false;

    _loadedElites[creature->GetSpawnId()] = elite;

    // Dayborn / Nightborn: assigned at promotion based on the server-local hour.
    // Mutually exclusive — whichever matches the time of birth sticks permanently.
    {
        time_t promoteTime = static_cast<time_t>(GameTime::GetGameTime().count());
        struct tm localTm;
        localtime_r(&promoteTime, &localTm);
        uint32 hour = static_cast<uint32>(localTm.tm_hour);
        if (hour >= traitConfig.daybornHourStart && hour <= traitConfig.daybornHourEnd)
        {
            elite->AddEarnedTrait(EARNED_TRAIT_DAYBORN);
            LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait DAYBORN at promotion | hour={}", NemesisConstants::LOG_PREFIX, name, elite->GetEliteId(), hour);
        }
        else
        {
            elite->AddEarnedTrait(EARNED_TRAIT_NIGHTBORN);
            LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait NIGHTBORN at promotion | hour={}", NemesisConstants::LOG_PREFIX, name, elite->GetEliteId(), hour);
        }
        threatScore = CalculateThreatScore(effectiveLevel, 0, 0, originTraits, elite->GetEarnedTraits(), now);
        elite->SetThreatScore(threatScore);
    }

    // NOTE: creature->SetName() is server-side only; client nameplates need a separate push.
    elite->ApplyStats(creature);
    _buffedElites.insert(creature->GetGUID());
    elite->RefreshAuras(creature);

    LOG_INFO("elite", "{} '{}' (entry {}) promoted in map {} after killing {} | effectiveLevel={} traits=0x{:04X} threat={}", NemesisConstants::LOG_PREFIX, name, creature->GetEntry(), creature->GetMapId(), playerKilled->GetName(), effectiveLevel, originTraits, threatScore);

    return true;
}

bool NemesisEliteManager::DemoteFromElite(Creature* creature)
{
    uint32 spawnId = creature->GetSpawnId();
    auto eliteIterator = _loadedElites.find(spawnId);
    if (eliteIterator == _loadedElites.end())
        return false;

    auto& elite = eliteIterator->second;

    if (creature->HasAura(NemesisEliteConstants::ELITE_GROW_SPELL))
        creature->RemoveAura(NemesisEliteConstants::ELITE_GROW_SPELL);

    // Strip all trait auras — both permanent passives and conditional combat buffs.
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_UMBRAL_BURST))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_UMBRAL_BURST);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_UNDERDOG))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_UNDERDOG);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_IRONBREAKER))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_IRONBREAKER);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_EXECUTIONER))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_EXECUTIONER);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_GIANT_SLAYER))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_GIANT_SLAYER);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_SCAVENGER))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_SCAVENGER);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_DUELIST))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_DUELIST);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_NOTORIOUS))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_NOTORIOUS);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_SURVIVOR))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_SURVIVOR);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_BLIGHT))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_BLIGHT);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_SPELLPROOF))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_SPELLPROOF);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_ENRAGED))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_ENRAGED);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_DAYBORN))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_DAYBORN);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_NIGHTBORN))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_NIGHTBORN);
    if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_NOMAD))
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_NOMAD);

    // Reverse the base elite stat boosts applied in ApplyStats.
    creature->ApplyStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, InversePctModifier(_scalingConfig.hpBoostPercent));
    creature->ApplyStatPctModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, InversePctModifier(_scalingConfig.damageBoostPercent));
    creature->ApplyStatPctModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, InversePctModifier(_scalingConfig.damageBoostPercent));
    creature->ApplyStatPctModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_PCT, InversePctModifier(_scalingConfig.damageBoostPercent));

    creature->UpdateEntry(elite->GetCreatureEntry());

    creature->UpdateAllStats();
    creature->UpdateMaxHealth();
    creature->SetHealth(creature->GetMaxHealth());
    creature->UpdateDamagePhysical(BASE_ATTACK);
    creature->UpdateDamagePhysical(OFF_ATTACK);
    creature->UpdateDamagePhysical(RANGED_ATTACK);

    LOG_INFO("elite", "{} '{}' (elite_id {}, spawnId {}) demoted by GM.", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), spawnId);

    // Mark dead to prevent respawn
    elite->MarkDead(0);

    // Clear combat state
    _traitBehavior->ClearCombatState(spawnId, creature->GetMap());

    // Erase from tracking containers
    _buffedElites.erase(creature->GetGUID());
    _loadedElites.erase(eliteIterator);

    return true;
}

bool NemesisEliteManager::SetEliteLevel(Creature* creature, uint8 level)
{
    uint32 spawnId = creature->GetSpawnId();
    auto eliteIterator = _loadedElites.find(spawnId);
    if (eliteIterator == _loadedElites.end())
        return false;

    auto& elite = eliteIterator->second;
    uint8 newLevel = level == 0 ? 1 : std::min(level, _scalingConfig.levelCap);
    elite->SetEffectiveLevel(newLevel);

    if (elite->GetCustomEntry() != 0)
        SyncCreatureTemplateLevel(elite->GetCustomEntry(), newLevel);

    _buffedElites.erase(creature->GetGUID());

    uint32 newThreat = CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
    elite->SetThreatScore(newThreat);

    LOG_INFO("elite", "{} '{}' (elite_id {}) level set to {} by GM | threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), static_cast<uint32>(newLevel), newThreat);

    return true;
}

bool NemesisEliteManager::AddEliteEarnedTrait(Creature* creature, uint32 trait)
{
    uint32 spawnId = creature->GetSpawnId();
    auto eliteIterator = _loadedElites.find(spawnId);
    if (eliteIterator == _loadedElites.end())
        return false;

    auto& elite = eliteIterator->second;
    if (elite->GetEarnedTraits() & trait)
        return false;

    elite->AddEarnedTrait(trait);
    _traitBehavior->ApplyTraitBehaviors(creature, elite);

    uint32 newThreat = CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
    elite->SetThreatScore(newThreat);

    LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait 0x{:04X} added by GM | threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), static_cast<uint32>(trait), newThreat);

    return true;
}

bool NemesisEliteManager::RemoveEliteEarnedTrait(Creature* creature, uint32 trait)
{
    uint32 spawnId = creature->GetSpawnId();
    auto eliteIterator = _loadedElites.find(spawnId);
    if (eliteIterator == _loadedElites.end())
        return false;

    auto& elite = eliteIterator->second;
    if (!(elite->GetEarnedTraits() & trait))
        return false;

    elite->RemoveEarnedTrait(trait);

    // Strip persistent auras associated with the removed trait. The _buffedElites
    // erasure below triggers a re-buff cycle that would also clean these up, but
    // explicit removal is belt-and-suspenders — same pattern as RemoveEliteOriginTrait.
    if (trait & EARNED_TRAIT_NOTORIOUS)
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_NOTORIOUS);
    if (trait & EARNED_TRAIT_SURVIVOR)
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_SURVIVOR);
    if (trait & EARNED_TRAIT_BLIGHT)
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_BLIGHT);
    if (trait & EARNED_TRAIT_SPELLPROOF)
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_SPELLPROOF);
    if (trait & EARNED_TRAIT_ENRAGED)
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_ENRAGED);
    if (trait & EARNED_TRAIT_DAYBORN)
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_DAYBORN);
    if (trait & EARNED_TRAIT_NIGHTBORN)
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_NIGHTBORN);
    if (trait & EARNED_TRAIT_NOMAD)
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_NOMAD);

    _buffedElites.erase(creature->GetGUID());

    uint32 newThreat = CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
    elite->SetThreatScore(newThreat);

    LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait 0x{:04X} removed by GM | threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), static_cast<uint32>(trait), newThreat);

    return true;
}

bool NemesisEliteManager::AddEliteOriginTrait(Creature* creature, uint32 trait)
{
    uint32 spawnId = creature->GetSpawnId();
    auto eliteIterator = _loadedElites.find(spawnId);
    if (eliteIterator == _loadedElites.end())
        return false;

    auto& elite = eliteIterator->second;
    if (elite->GetOriginTrait() & trait)
        return false;

    elite->SetOriginTrait(elite->GetOriginTrait() | trait);

    auto const& traitConfig = _traitBehavior->GetTraitConfig();

    // Apply Plunderer gold to the cloned template immediately so it takes effect
    // without waiting for a re-buff cycle.
    if ((trait & ORIGIN_TRAIT_PLUNDERER) && elite->GetCustomEntry() != 0)
    {
        CreatureTemplate const* baseTemplate = sObjectMgr->GetCreatureTemplate(elite->GetCreatureEntry());
        if (baseTemplate)
        {
            uint32 bonusCopper = traitConfig.plundererBonusGoldBase + (static_cast<uint32>(elite->GetEffectiveLevel()) * traitConfig.plundererBonusGoldPerLevel);
            uint32 newMinGold = baseTemplate->mingold + bonusCopper;
            uint32 newMaxGold = baseTemplate->maxgold + bonusCopper;

            WorldDatabase.Execute("UPDATE `creature_template` SET `mingold` = {}, `maxgold` = {} WHERE `entry` = {}", newMinGold, newMaxGold, elite->GetCustomEntry());

            if (CreatureTemplate const* clonedTemplate = sObjectMgr->GetCreatureTemplate(elite->GetCustomEntry()))
            {
                const_cast<CreatureTemplate*>(clonedTemplate)->mingold = newMinGold;
                const_cast<CreatureTemplate*>(clonedTemplate)->maxgold = newMaxGold;
            }
        }
    }

    uint32 newThreat = CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
    elite->SetThreatScore(newThreat);

    LOG_INFO("elite", "{} '{}' (elite_id {}) origin trait 0x{:04X} added by GM | threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), static_cast<uint32>(trait), newThreat);

    return true;
}

bool NemesisEliteManager::RemoveEliteOriginTrait(Creature* creature, uint32 trait)
{
    uint32 spawnId = creature->GetSpawnId();
    auto eliteIterator = _loadedElites.find(spawnId);
    if (eliteIterator == _loadedElites.end())
        return false;

    auto& elite = eliteIterator->second;
    if (!(elite->GetOriginTrait() & trait))
        return false;

    elite->SetOriginTrait(elite->GetOriginTrait() & ~trait);

    // Strip persistent auras associated with the removed trait. RefreshAuras
    // has !HasAura guards so it won't re-add, but it also never removes.
    if (trait & ORIGIN_TRAIT_UNDERDOG)
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_UNDERDOG);
    if (trait & ORIGIN_TRAIT_IRONBREAKER)
        creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_IRONBREAKER);

    auto const& traitConfig = _traitBehavior->GetTraitConfig();

    // Revert Plunderer gold on the cloned template back to the base creature's values.
    if ((trait & ORIGIN_TRAIT_PLUNDERER) && elite->GetCustomEntry() != 0)
    {
        CreatureTemplate const* baseTemplate = sObjectMgr->GetCreatureTemplate(elite->GetCreatureEntry());
        if (baseTemplate)
        {
            WorldDatabase.Execute("UPDATE `creature_template` SET `mingold` = {}, `maxgold` = {} WHERE `entry` = {}", baseTemplate->mingold, baseTemplate->maxgold, elite->GetCustomEntry());

            if (CreatureTemplate const* clonedTemplate = sObjectMgr->GetCreatureTemplate(elite->GetCustomEntry()))
            {
                const_cast<CreatureTemplate*>(clonedTemplate)->mingold = baseTemplate->mingold;
                const_cast<CreatureTemplate*>(clonedTemplate)->maxgold = baseTemplate->maxgold;
            }
        }
    }

    uint32 newThreat = CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
    elite->SetThreatScore(newThreat);

    LOG_INFO("elite", "{} '{}' (elite_id {}) origin trait 0x{:04X} removed by GM | threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), static_cast<uint32>(trait), newThreat);

    return true;
}

void NemesisEliteManager::SyncCreatureTemplateLevel(uint32 customEntry, uint8 level)
{
    // Update the DB row so the change survives restarts.
    WorldDatabase.Execute("UPDATE `creature_template` SET `minlevel` = {}, `maxlevel` = {} WHERE `entry` = {}", static_cast<uint32>(level), static_cast<uint32>(level), customEntry);

    // Sync the in-memory template cache so UpdateEntry/SelectLevel uses the new
    // level when computing base HP/damage without requiring a server restart.
    if (CreatureTemplate const* tmpl = sObjectMgr->GetCreatureTemplate(customEntry))
    {
        const_cast<CreatureTemplate*>(tmpl)->minlevel = level;
        const_cast<CreatureTemplate*>(tmpl)->maxlevel = level;
    }
}

// ============================================================================
// Forwarding methods — delegate to NemesisEliteTraitBehavior
// ============================================================================

std::unordered_set<uint32> const& NemesisEliteManager::GetInCombatEliteSpawnIds() const
{
    return _traitBehavior->GetInCombatEliteSpawnIds();
}

void NemesisEliteManager::MarkHealedAgainstElite(uint32 spawnId)
{
    _traitBehavior->MarkHealedAgainstElite(spawnId);
}

void NemesisEliteManager::TrackCombatPlayerHit(uint32 spawnId, uint32 playerGuid)
{
    _traitBehavior->TrackCombatPlayerHit(spawnId, playerGuid);
}

void NemesisEliteManager::GrantNomadTrait(uint32 spawnId)
{
    _traitBehavior->GrantNomadTrait(spawnId);
}

void NemesisEliteManager::GrantStudiousTrait(uint32 spawnId)
{
    _traitBehavior->GrantStudiousTrait(spawnId);
}

uint32 NemesisEliteManager::GetDeathblowStrikeDamage(uint32 spawnId) const
{
    return _traitBehavior->GetDeathblowStrikeDamage(spawnId);
}

void NemesisEliteManager::MarkDeathblowSpent(uint32 spawnId)
{
    _traitBehavior->MarkDeathblowSpent(spawnId);
}

uint32 NemesisEliteManager::GetCowardHealAmount(uint32 spawnId) const
{
    return _traitBehavior->GetCowardHealAmount(spawnId);
}

uint32 NemesisEliteManager::GetUmbralBurstDamage(uint32 spawnId) const
{
    return _traitBehavior->GetUmbralBurstDamage(spawnId);
}

EliteTraitConfig const& NemesisEliteManager::GetTraitConfig() const
{
    return _traitBehavior->GetTraitConfig();
}

EliteAffixConfig const& NemesisEliteManager::GetAffixConfig() const
{
    return sAffixMgr->GetAffixConfig();
}
