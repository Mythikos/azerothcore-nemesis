#include "NemesisEliteTraitBehavior.h"
#include "NemesisEliteManager.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "NemesisUmbralMoonManager.h"
#include <ctime>

NemesisEliteTraitBehavior::NemesisEliteTraitBehavior(NemesisEliteManager& owner)
	: _owner(owner)
{
}

void NemesisEliteTraitBehavior::LoadConfig()
{
	_traitConfig.ambusherSpawnRadius = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_ambusher_spawn_radius", "50")));
	_traitConfig.ambusherPullIntervalMs = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_ambusher_pull_interval_ms", "8000"))));
	_traitConfig.ambusherAddsPerPull = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_ambusher_adds_per_pull", "2"))));
	_traitConfig.ambusherMaxAddsPerEncounter = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_ambusher_max_adds_per_encounter", "8"))));
	_traitConfig.notoriousKillThreshold = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_notorious_kill_threshold", "5"))));
	_traitConfig.territorialDaysThreshold = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_territorial_days_threshold", "3"))));
	_traitConfig.territorialWanderLevelMultiplier = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_territorial_wander_level_multiplier", "2.0")));
	_traitConfig.territorialDetectionMultiplier = std::max(1.0f, std::stof(NemesisLoadConfigValue("elite_trait_territorial_detection_multiplier", "1.5")));
	_traitConfig.territorialWanderMinRadius = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_territorial_wander_min_radius", "20")));
	_traitConfig.survivorAttemptsThreshold = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_survivor_attempts_threshold", "3"))));
	_traitConfig.cowardHpThreshold = std::max(0.0f, std::min(100.0f, std::stof(NemesisLoadConfigValue("elite_trait_coward_hp_threshold", "15"))));
	_traitConfig.cowardFleeChance = std::max(0.0f, std::min(100.0f, std::stof(NemesisLoadConfigValue("elite_trait_coward_flee_chance", "30"))));
	_traitConfig.cowardFleeDurationMs = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_coward_flee_duration_ms", "5000"))));
	_traitConfig.enragedPlayerThreshold = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_enraged_player_threshold", "5"))));
	_traitConfig.sageDaysThreshold = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_sage_days_threshold", "7"))));
	_traitConfig.studiousSearchRange = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_studious_search_range", "30")));
	_traitConfig.nomadAreaThreshold = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_nomad_area_threshold", "3"))));
	_traitConfig.executionerHpThreshold = std::max(0.0f, std::min(100.0f, std::stof(NemesisLoadConfigValue("elite_trait_executioner_hp_threshold", "20"))));
	_traitConfig.executionerAuraHpThreshold = std::max(0.0f, std::min(100.0f, std::stof(NemesisLoadConfigValue("elite_trait_executioner_aura_hp_threshold", "20"))));
	_traitConfig.mageBaneSilenceIntervalMs = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_mage_bane_silence_interval_ms", "10000"))));
	_traitConfig.mageBaneSilenceRange = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_mage_bane_silence_range", "30")));
	_traitConfig.healerBaneReapplyIntervalMs = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_healer_bane_reapply_interval_ms", "12000"))));
	_traitConfig.giantSlayerLevelGap = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_giant_slayer_level_gap", "10"))));
	_traitConfig.giantSlayerAuraLevelGap = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_giant_slayer_aura_level_gap", "5"))));
	_traitConfig.opportunistCheapShotIntervalMs = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_opportunist_cheap_shot_interval_ms", "15000"))));
	_traitConfig.umbralBurstRecastIntervalMs = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_umbral_burst_recast_interval_ms", "10000"))));
	_traitConfig.underdogLevelGap = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_underdog_level_gap", "3"))));
	_traitConfig.scavengerDamageBasePercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_scavenger_damage_base_percent", "6")));
	_traitConfig.scavengerDamagePerLevelPercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_scavenger_damage_per_level_percent", "0.15")));
	_traitConfig.duelistDamageBasePercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_duelist_damage_base_percent", "8")));
	_traitConfig.duelistDamagePerLevelPercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_duelist_damage_per_level_percent", "0.25")));
	_traitConfig.plundererGoldPerLevel = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_trait_plunderer_gold_per_level", "10000"))));
	_traitConfig.plundererBonusGoldBase = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_trait_plunderer_bonus_gold_base", "5000"))));
	_traitConfig.plundererBonusGoldPerLevel = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_trait_plunderer_bonus_gold_per_level", "1000"))));
	_traitConfig.daybornHourStart = static_cast<uint32>(std::max(0, std::min(23, std::stoi(NemesisLoadConfigValue("elite_trait_dayborn_hour_start", "6")))));
	_traitConfig.daybornHourEnd = static_cast<uint32>(std::max(0, std::min(23, std::stoi(NemesisLoadConfigValue("elite_trait_dayborn_hour_end", "17")))));
	_traitConfig.deathblowHpThreshold = std::max(0.0f, std::min(100.0f, std::stof(NemesisLoadConfigValue("elite_trait_deathblow_hp_threshold", "25"))));
	_traitConfig.deathblowBaseDamage = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_trait_deathblow_base_damage", "200"))));
	_traitConfig.deathblowDamagePerLevel = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_trait_deathblow_damage_per_level", "50"))));
	_traitConfig.deathblowRearmCooldownMs = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_trait_deathblow_rearm_cooldown_ms", "10000"))));
	_traitConfig.cowardHealBase = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_trait_coward_heal_base", "100"))));
	_traitConfig.cowardHealPerLevel = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_trait_coward_heal_per_level", "25"))));
	_traitConfig.umbralBurstBase = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_trait_umbral_burst_base", "50"))));
	_traitConfig.umbralBurstPerLevel = static_cast<uint32>(std::max(0, std::stoi(NemesisLoadConfigValue("elite_trait_umbral_burst_per_level", "15"))));
	_traitConfig.notoriousDamageBasePercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_notorious_damage_base_percent", "5")));
	_traitConfig.notoriousDamagePerLevelPercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_notorious_damage_per_level_percent", "0.15")));
	_traitConfig.notoriousHealthBase = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_notorious_health_base", "50")));
	_traitConfig.notoriousHealthPerLevel = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_notorious_health_per_level", "10")));
	_traitConfig.survivorArmorBase = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_survivor_armor_base", "100")));
	_traitConfig.survivorArmorPerLevel = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_survivor_armor_per_level", "5")));
	_traitConfig.survivorCritReductionBase = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_survivor_crit_reduction_base", "5")));
	_traitConfig.survivorCritReductionPerLevel = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_survivor_crit_reduction_per_level", "0.15")));
	_traitConfig.blightDamageBase = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_blight_damage_base", "10")));
	_traitConfig.blightDamagePerLevel = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_blight_damage_per_level", "3")));
	_traitConfig.spellproofCcDurationReduction = std::max(0.0f, std::min(100.0f, std::stof(NemesisLoadConfigValue("elite_trait_spellproof_cc_duration_reduction", "50"))));
	_traitConfig.scarredHealingReductionPercent = std::max(0.0f, std::min(100.0f, std::stof(NemesisLoadConfigValue("elite_trait_scarred_healing_reduction_percent", "30"))));
	_traitConfig.scarredReapplyIntervalMs = static_cast<uint32>(std::max(1, std::stoi(NemesisLoadConfigValue("elite_trait_scarred_reapply_interval_ms", "12000"))));
	_traitConfig.enragedDamageBasePercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_enraged_damage_base_percent", "3")));
	_traitConfig.enragedDamagePerLevelPercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_enraged_damage_per_level_percent", "0.10")));
	_traitConfig.enragedDamagePerPlayerPercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_enraged_damage_per_player_percent", "4")));
	_traitConfig.daybornDamageBasePercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_dayborn_damage_base_percent", "5")));
	_traitConfig.daybornDamagePerLevelPercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_dayborn_damage_per_level_percent", "0.15")));
	_traitConfig.nightbornDamageBasePercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_nightborn_damage_base_percent", "5")));
	_traitConfig.nightbornDamagePerLevelPercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_nightborn_damage_per_level_percent", "0.15")));
	_traitConfig.nomadSpeedPercent = std::max(0.0f, std::stof(NemesisLoadConfigValue("elite_trait_nomad_speed_percent", "30")));
}

void NemesisEliteTraitBehavior::ApplyTraitBehaviors(Creature* creature, std::shared_ptr<NemesisEliteCreature> const& elite)
{
	if (elite->GetEarnedTraits() & EARNED_TRAIT_TERRITORIAL)
	{
		creature->SetFaction(NemesisEliteConstants::ELITE_HOSTILE_FACTION);
		creature->SetReactState(REACT_AGGRESSIVE);

		float wanderRadius = std::max(_traitConfig.territorialWanderMinRadius, static_cast<float>(elite->GetEffectiveLevel()) * _traitConfig.territorialWanderLevelMultiplier);
		creature->GetMotionMaster()->MoveRandom(wanderRadius);

		creature->SetDetectionDistance(creature->GetDetectionRange() * _traitConfig.territorialDetectionMultiplier);
	}

	// Plunderer trait: inject bonus gold into the cloned creature_template so it
	// appears in the creature's loot window when killed. Always computed from the
	// base template's gold + level-scaled bonus so it's idempotent on re-buff.
	if ((elite->GetOriginTrait() & ORIGIN_TRAIT_PLUNDERER) && elite->GetCustomEntry() != 0)
	{
		CreatureTemplate const* baseTemplate = sObjectMgr->GetCreatureTemplate(elite->GetCreatureEntry());
		if (baseTemplate)
		{
			uint32 bonusCopper = _traitConfig.plundererBonusGoldBase + (static_cast<uint32>(elite->GetEffectiveLevel()) * _traitConfig.plundererBonusGoldPerLevel);
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
}

void NemesisEliteTraitBehavior::ProcessTraitTick(Creature* creature, std::shared_ptr<NemesisEliteCreature> const& elite, uint32 spawnId)
{
	// Re-apply GROW aura whenever it's missing (e.g. after a combat reset).
	elite->RefreshAuras(creature);

	// Dayborn trait: conditional damage buff during daytime hours.
	// Runs every tick regardless of combat state so players can see the aura
	// on the nameplate before engaging. Uses server-local time.
	if (elite->GetEarnedTraits() & EARNED_TRAIT_DAYBORN)
	{
		time_t nowTime = static_cast<time_t>(GameTime::GetGameTime().count());
		struct tm localTm;
		localtime_r(&nowTime, &localTm);
		uint32 hour = static_cast<uint32>(localTm.tm_hour);

		bool isDaytime = (hour >= _traitConfig.daybornHourStart && hour <= _traitConfig.daybornHourEnd);
		bool hasDaybornAura = creature->HasAura(NemesisEliteConstants::ELITE_AURA_DAYBORN);
		if (isDaytime && !hasDaybornAura)
		{
			int32 damagePercent = static_cast<int32>(_traitConfig.daybornDamageBasePercent + (static_cast<float>(elite->GetEffectiveLevel()) * _traitConfig.daybornDamagePerLevelPercent));
			if (Aura* aura = creature->AddAura(NemesisEliteConstants::ELITE_AURA_DAYBORN, creature))
			{
				if (AuraEffect* effect = aura->GetEffect(0))
					effect->ChangeAmount(damagePercent);
			}
		}
		else if (!isDaytime && hasDaybornAura)
		{
			creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_DAYBORN);
		}
	}

	// Nightborn trait: conditional damage buff during nighttime hours.
	// Mirror of Dayborn — always visible, not gated on combat.
	if (elite->GetEarnedTraits() & EARNED_TRAIT_NIGHTBORN)
	{
		time_t nowTime = static_cast<time_t>(GameTime::GetGameTime().count());
		struct tm localTm;
		localtime_r(&nowTime, &localTm);
		uint32 hour = static_cast<uint32>(localTm.tm_hour);

		bool isNighttime = (hour < _traitConfig.daybornHourStart || hour > _traitConfig.daybornHourEnd);
		bool hasNightbornAura = creature->HasAura(NemesisEliteConstants::ELITE_AURA_NIGHTBORN);
		if (isNighttime && !hasNightbornAura)
		{
			int32 damagePercent = static_cast<int32>(_traitConfig.nightbornDamageBasePercent + (static_cast<float>(elite->GetEffectiveLevel()) * _traitConfig.nightbornDamagePerLevelPercent));
			if (Aura* aura = creature->AddAura(NemesisEliteConstants::ELITE_AURA_NIGHTBORN, creature))
			{
				if (AuraEffect* effect = aura->GetEffect(0))
					effect->ChangeAmount(damagePercent);
			}
		}
		else if (!isNighttime && hasNightbornAura)
		{
			creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_NIGHTBORN);
		}
	}

	// Coward trait: apply the deferred post-flee heal once the flee duration has elapsed.
	{
		auto cowardHealIterator = _cowardHealAt.find(spawnId);
		if (cowardHealIterator != _cowardHealAt.end() && static_cast<uint32>(time(nullptr)) >= cowardHealIterator->second)
		{
			// Clear the fleeing flags from the creature
			creature->RemoveUnitFlag(UNIT_FLAG_FLEEING);
			creature->ClearUnitState(UNIT_STATE_FLEEING | UNIT_STATE_FLEEING_MOVE);

			// Remove the stealth aura if it's still present (it should be, but this is a safeguard in case of desync) before applying the heal so the creature becomes visible again.
			if (creature->HasAura(NemesisEliteConstants::ELITE_SPELL_COWARD_STEALTH))
				creature->RemoveAura(NemesisEliteConstants::ELITE_SPELL_COWARD_STEALTH);
			creature->CastSpell(creature, NemesisEliteConstants::ELITE_SPELL_COWARD_HEAL, false);

			// Remove from heal tracking
			_cowardHealAt.erase(cowardHealIterator);
		}
	}

	// Passive age-based level scaling — one level per ageLevelIntervalHours alive.
	auto const& scalingConfig = _owner.GetScalingConfig();
	if (elite->GetEffectiveLevel() < scalingConfig.levelCap)
	{
		uint32 now = static_cast<uint32>(time(nullptr));
		uint32 intervalSecs = scalingConfig.ageLevelIntervalHours * 3600u;
		if ((now - elite->GetLastAgeLevelAt()) >= intervalSecs)
		{
			uint8 newLevel = elite->GetEffectiveLevel() + 1;
			elite->SetEffectiveLevel(newLevel);
			elite->SetLastAgeLevelAt(now);

			if (elite->GetCustomEntry() != 0)
				_owner.SyncCreatureTemplateLevel(elite->GetCustomEntry(), newLevel);
			_owner.UnmarkBuffed(creature->GetGUID());

			uint32 newThreat = _owner.CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
			elite->SetThreatScore(newThreat);

			LOG_INFO("elite", "{} '{}' (elite_id {}) gained passive age level | level={} threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), static_cast<uint32>(newLevel), newThreat);
		}
	}

	// Territorial trait: grant once the nemesis has been alive long enough.
	// Runs every tick but short-circuits immediately once the trait is earned.
	if (!(elite->GetEarnedTraits() & EARNED_TRAIT_TERRITORIAL))
	{
		uint32 nowCheck = static_cast<uint32>(time(nullptr));
		uint32 aliveSeconds = (nowCheck > elite->GetPromotedAt()) ? (nowCheck - elite->GetPromotedAt()) : 0u;
		if (aliveSeconds >= _traitConfig.territorialDaysThreshold * 86400u)
		{
			elite->AddEarnedTrait(EARNED_TRAIT_TERRITORIAL);
			ApplyTraitBehaviors(creature, elite);

			uint32 newThreat = _owner.CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
			elite->SetThreatScore(newThreat);

			LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait TERRITORIAL | age={}d threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), aliveSeconds / 86400u, newThreat);
		}
	}

	// Sage trait: grant once the nemesis has been alive long enough (configurable threshold).
	if (!(elite->GetEarnedTraits() & EARNED_TRAIT_SAGE))
	{
		uint32 nowSage = static_cast<uint32>(time(nullptr));
		uint32 aliveSecondsSage = (nowSage > elite->GetPromotedAt()) ? (nowSage - elite->GetPromotedAt()) : 0u;
		if (aliveSecondsSage >= _traitConfig.sageDaysThreshold * 86400u)
		{
			elite->AddEarnedTrait(EARNED_TRAIT_SAGE);

			uint32 newThreat = _owner.CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
			elite->SetThreatScore(newThreat);

			LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait SAGE | age={}d threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), aliveSecondsSage / 86400u, newThreat);
		}
	}

	// Survivor trait: detect when a nemesis leaves combat alive.
	// Entry: creature enters combat -> track in _inCombatElites.
	// Exit:  creature leaves combat while alive -> count as a survived encounter.
	//        (Death path in OnEliteKilledByPlayer erases the spawnId first so a kill
	//        is never miscounted as a survived encounter.)
	if (creature->IsInCombat())
	{
		// Roll the Coward flee chance exactly once at combat entry.
		if (!_inCombatElites.count(spawnId))
		{
			_inCombatElites.insert(spawnId);

			// Creature trait: If the creature already has a coward trait from a prior encounter, it should always be a coward candidate.
			// Otherwise, roll for the coward trait on combat entry and add to candidates if it hits.
			if (elite->GetEarnedTraits() & EARNED_TRAIT_COWARD || frand(0.0f, 100.0f) < _traitConfig.cowardFleeChance)
				_cowardCandidates.insert(spawnId);
		}

		// Ambusher trait: periodically spawn adds of the same creature type to attack the nemesis's target.
		if (elite->GetOriginTrait() & ORIGIN_TRAIT_AMBUSHER)
		{
			uint32 now = getMSTime();

			// Initialize the pull timer immediately on first combat tick.
			if (!_ambusherNextPullAt.count(spawnId))
				_ambusherNextPullAt[spawnId] = now;

			uint32 addsPulled = _ambusherAddsPulled[spawnId];
			if (addsPulled < _traitConfig.ambusherMaxAddsPerEncounter && now >= _ambusherNextPullAt[spawnId])
			{
				Unit* victim = creature->GetVictim();
				if (victim)
				{
					uint32 spawnedThisWave = 0;

					for (uint32 i = 0; i < _traitConfig.ambusherAddsPerPull; ++i)
					{
						if (addsPulled + spawnedThisWave >= _traitConfig.ambusherMaxAddsPerEncounter)
							break;

						// Spawn the add at a random point near the nemesis within the configured radius.
						float angle = frand(0.0f, 2.0f * static_cast<float>(M_PI));
						float dist = frand(5.0f, std::min(15.0f, _traitConfig.ambusherSpawnRadius));
						float x = creature->GetPositionX() + dist * cosf(angle);
						float y = creature->GetPositionY() + dist * sinf(angle);
						float z = creature->GetPositionZ();
						creature->UpdateGroundPositionZ(x, y, z);

						Creature* add = creature->SummonCreature(elite->GetCreatureEntry(), x, y, z, creature->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);
						if (!add)
							continue;

						add->AI()->AttackStart(victim);
						_ambusherSpawnedAdds[spawnId].push_back(add->GetGUID());
						++spawnedThisWave;
					}

					_ambusherAddsPulled[spawnId] = addsPulled + spawnedThisWave;
					_ambusherNextPullAt[spawnId] = now + _traitConfig.ambusherPullIntervalMs;

					if (spawnedThisWave > 0)
						LOG_INFO("elite", "{} Ambusher '{}' (elite_id {}) spawned {} adds | total={}/{}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), spawnedThisWave, _ambusherAddsPulled[spawnId], _traitConfig.ambusherMaxAddsPerEncounter);
				}
			}

			uint32 homeUpdateNow = getMSTime();
			if (!_ambusherNextHomeUpdateAt.count(spawnId))
				_ambusherNextHomeUpdateAt[spawnId] = homeUpdateNow;

			if (homeUpdateNow >= _ambusherNextHomeUpdateAt[spawnId])
			{
				_ambusherNextHomeUpdateAt[spawnId] = homeUpdateNow + 1000;
				Unit* currentVictim = creature->GetVictim();
				if (currentVictim)
				{
					auto addsIterator = _ambusherSpawnedAdds.find(spawnId);
					if (addsIterator != _ambusherSpawnedAdds.end())
					{
						for (ObjectGuid const& addGuid : addsIterator->second)
						{
							if (Creature* add = creature->GetMap()->GetCreature(addGuid))
								add->SetHomePosition(currentVictim->GetPositionX(), currentVictim->GetPositionY(), currentVictim->GetPositionZ(), currentVictim->GetOrientation());
						}
					}
				}
			}
		}

		// Coward trait: flee and self-heal when health drops critically low.
		// Only candidates (rolled at combat entry) can trigger; only fires once per encounter.
		if (_cowardCandidates.count(spawnId) && !_fledThisCombat.count(spawnId) && creature->GetVictim() && creature->GetHealthPct() < _traitConfig.cowardHpThreshold)
		{
			_fledThisCombat.insert(spawnId);
			if (!creature->HasAura(NemesisEliteConstants::ELITE_SPELL_COWARD_STEALTH))
				creature->AddAura(NemesisEliteConstants::ELITE_SPELL_COWARD_STEALTH, creature);
			creature->GetMotionMaster()->MoveFleeing(creature->GetVictim(), _traitConfig.cowardFleeDurationMs);
			_cowardHealAt[spawnId] = static_cast<uint32>(time(nullptr)) + (_traitConfig.cowardFleeDurationMs / 1000u);

			if (!(elite->GetEarnedTraits() & EARNED_TRAIT_COWARD))
			{
				elite->AddEarnedTrait(EARNED_TRAIT_COWARD);

				uint32 newThreat = _owner.CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
				elite->SetThreatScore(newThreat);

				LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait COWARD | fled at {:.0f}% hp threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), creature->GetHealthPct(), newThreat);
			}
		}

		// Spellproof: mark this nemesis if it currently has a CC aura applied.
		if (!(elite->GetEarnedTraits() & EARNED_TRAIT_SPELLPROOF) && !_ccHitElites.count(spawnId))
		{
			constexpr uint32 ccMechanicMask = (1u << MECHANIC_POLYMORPH) | (1u << MECHANIC_FEAR)
				| (1u << MECHANIC_STUN) | (1u << MECHANIC_ROOT) | (1u << MECHANIC_FREEZE)
				| (1u << MECHANIC_BANISH) | (1u << MECHANIC_HORROR);
			if (creature->HasAuraWithMechanic(ccMechanicMask))
				_ccHitElites.insert(spawnId);
		}

		// Executioner trait: apply the bonus damage aura while the current victim is
		// a player at or below the configured HP threshold. Remove it otherwise so
		// the buff only activates when finishing a wounded target.
		if (elite->GetOriginTrait() & ORIGIN_TRAIT_EXECUTIONER)
		{
			bool shouldHaveAura = false;
			if (Unit* victim = creature->GetVictim())
			{
				if (Player* victimPlayer = victim->ToPlayer())
				{
					float victimHealthPercent = static_cast<float>(victimPlayer->GetHealth()) / static_cast<float>(victimPlayer->GetMaxHealth()) * 100.0f;
					shouldHaveAura = (victimHealthPercent <= _traitConfig.executionerAuraHpThreshold);
				}
			}

			bool hasAura = creature->HasAura(NemesisEliteConstants::ELITE_AURA_EXECUTIONER);
			if (shouldHaveAura && !hasAura)
				creature->AddAura(NemesisEliteConstants::ELITE_AURA_EXECUTIONER, creature);
			else if (!shouldHaveAura && hasAura)
				creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_EXECUTIONER);
		}

		// Mage Bane trait: periodically silence a player caught casting within range.
		if (elite->GetOriginTrait() & ORIGIN_TRAIT_MAGE_BANE)
		{
			uint32 now = getMSTime();

			if (!_mageBaneNextCastAt.count(spawnId))
				_mageBaneNextCastAt[spawnId] = now;

			if (now >= _mageBaneNextCastAt[spawnId])
			{
				Player* castingTarget = nullptr;
				for (auto const& ref : creature->GetThreatMgr().GetThreatList())
				{
					if (!ref->getTarget())
						continue;

					Player* threatPlayer = ref->getTarget()->ToPlayer();
					if (!threatPlayer || !threatPlayer->IsAlive())
						continue;

					if (!threatPlayer->IsNonMeleeSpellCast(false, true, true))
						continue;

					if (creature->GetDistance(threatPlayer) > _traitConfig.mageBaneSilenceRange)
						continue;

					castingTarget = threatPlayer;
					break;
				}

				if (castingTarget)
				{
					creature->AddAura(NemesisEliteConstants::ELITE_SPELL_MAGE_BANE_SILENCE, castingTarget);
					_mageBaneNextCastAt[spawnId] = now + _traitConfig.mageBaneSilenceIntervalMs;

					LOG_DEBUG("elite", "{} Mage Bane '{}' (elite_id {}) silenced {} mid-cast.", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), castingTarget->GetName());
				}
			}
		}

		// Healer Bane trait: apply a healing reduction debuff to the current melee
		// victim on a configurable cooldown.
		if (elite->GetOriginTrait() & ORIGIN_TRAIT_HEALER_BANE)
		{
			uint32 healerBaneNow = getMSTime();

			if (!_healerBaneNextCastAt.count(spawnId))
				_healerBaneNextCastAt[spawnId] = healerBaneNow;

			if (healerBaneNow >= _healerBaneNextCastAt[spawnId])
			{
				if (Unit* victim = creature->GetVictim())
				{
					if (Player* victimPlayer = victim->ToPlayer())
					{
						creature->AddAura(NemesisEliteConstants::ELITE_AURA_HEALER_BANE, victimPlayer);
						_healerBaneNextCastAt[spawnId] = healerBaneNow + _traitConfig.healerBaneReapplyIntervalMs;

						LOG_DEBUG("elite", "{} Healer Bane '{}' (elite_id {}) applied healing reduction to {}.", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), victimPlayer->GetName());
					}
				}
			}
		}

		// Giant Slayer trait: apply a damage and HP buff aura while the current
		// victim is a player whose level exceeds the nemesis by the configured gap.
		if (elite->GetOriginTrait() & ORIGIN_TRAIT_GIANT_SLAYER)
		{
			bool shouldHaveGiantSlayerAura = false;
			if (Unit* victim = creature->GetVictim())
			{
				if (Player* victimPlayer = victim->ToPlayer())
					shouldHaveGiantSlayerAura = (victimPlayer->GetLevel() >= creature->GetLevel() + _traitConfig.giantSlayerAuraLevelGap);
			}

			bool hasGiantSlayerAura = creature->HasAura(NemesisEliteConstants::ELITE_AURA_GIANT_SLAYER);
			if (shouldHaveGiantSlayerAura && !hasGiantSlayerAura)
				creature->AddAura(NemesisEliteConstants::ELITE_AURA_GIANT_SLAYER, creature);
			else if (!shouldHaveGiantSlayerAura && hasGiantSlayerAura)
				creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_GIANT_SLAYER);
		}

		// Opportunist trait: periodically stun the current melee victim with a
		// cheap shot.
		if (elite->GetOriginTrait() & ORIGIN_TRAIT_OPPORTUNIST)
		{
			uint32 cheapShotNow = getMSTime();

			if (!_opportunistNextCheapShotAt.count(spawnId))
				_opportunistNextCheapShotAt[spawnId] = cheapShotNow;

			if (cheapShotNow >= _opportunistNextCheapShotAt[spawnId])
			{
				if (Unit* victim = creature->GetVictim())
				{
					if (Player* victimPlayer = victim->ToPlayer())
					{
						creature->AddAura(NemesisEliteConstants::ELITE_SPELL_OPPORTUNIST_CHEAP_SHOT, victimPlayer);
						_opportunistNextCheapShotAt[spawnId] = cheapShotNow + _traitConfig.opportunistCheapShotIntervalMs;

						LOG_DEBUG("elite", "{} Opportunist '{}' (elite_id {}) cheap-shotted {}.", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), victimPlayer->GetName());
					}
				}
			}
		}

		// Umbralforged trait: apply the Umbral Burst aura while the nemesis
		// is in combat during an active Umbral Moon.
		if (elite->GetOriginTrait() & ORIGIN_TRAIT_UMBRALFORGED)
		{
			bool shouldHaveAura = sUmbralMoonMgr->IsUmbralMoonActive();
			bool hasAura = creature->HasAura(NemesisEliteConstants::ELITE_AURA_UMBRAL_BURST);

			if (shouldHaveAura && !hasAura)
			{
				uint32 umbralforgedNow = getMSTime();

				if (!_umbralBurstNextCastAt.count(spawnId))
					_umbralBurstNextCastAt[spawnId] = umbralforgedNow;

				if (umbralforgedNow >= _umbralBurstNextCastAt[spawnId])
				{
					creature->CastSpell(creature, NemesisEliteConstants::ELITE_AURA_UMBRAL_BURST, true);
					_umbralBurstNextCastAt[spawnId] = umbralforgedNow + _traitConfig.umbralBurstRecastIntervalMs;
				}
			}
			else if (!shouldHaveAura && hasAura)
			{
				creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_UMBRAL_BURST);
				_umbralBurstNextCastAt.erase(spawnId);
			}
		}

		// Deathblow trait: telegraphed heavy strike when HP drops below threshold.
		// State machine: IDLE -> CASTING -> SPENT -> (healed above threshold) -> IDLE.
		if (elite->GetOriginTrait() & ORIGIN_TRAIT_DEATHBLOW)
		{
			DeathblowState state = DEATHBLOW_IDLE;
			auto stateIterator = _deathblowState.find(spawnId);
			if (stateIterator != _deathblowState.end())
				state = stateIterator->second;

			float hpPercent = creature->GetHealthPct();

			if (state == DEATHBLOW_IDLE && hpPercent < _traitConfig.deathblowHpThreshold)
			{
				// Check re-arm cooldown to prevent rapid cycling from heal/damage oscillation.
				uint32 deathblowNow = getMSTime();
				auto rearmIterator = _deathblowRearmAt.find(spawnId);
				bool cooldownExpired = (rearmIterator == _deathblowRearmAt.end() || deathblowNow >= rearmIterator->second);

				if (cooldownExpired && creature->GetVictim())
				{
					creature->CastSpell(creature->GetVictim(), NemesisEliteConstants::ELITE_SPELL_DEATHBLOW_STRIKE, false);
					_deathblowState[spawnId] = DEATHBLOW_CASTING;

					LOG_INFO("elite", "{} Deathblow '{}' (elite_id {}) began casting Deathblow Strike at {:.0f}% HP.", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), hpPercent);
				}
			}
			else if (state == DEATHBLOW_CASTING)
			{
				// Cast resolved when the creature is no longer channeling/casting the spell.
				if (!creature->GetCurrentSpell(CURRENT_GENERIC_SPELL))
				{
					_deathblowState[spawnId] = DEATHBLOW_SPENT;

					LOG_INFO("elite", "{} Deathblow '{}' (elite_id {}) Deathblow Strike resolved (completed or interrupted).", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId());
				}
			}
			else if (state == DEATHBLOW_SPENT && hpPercent >= _traitConfig.deathblowHpThreshold)
			{
				// Healed back above threshold — reset to IDLE with a re-arm cooldown.
				_deathblowState[spawnId] = DEATHBLOW_IDLE;
				_deathblowRearmAt[spawnId] = getMSTime() + _traitConfig.deathblowRearmCooldownMs;

				LOG_INFO("elite", "{} Deathblow '{}' (elite_id {}) healed above {:.0f}% — re-armed.", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), _traitConfig.deathblowHpThreshold);
			}
		}

		// Scavenger trait: apply a level-scaled damage bonus aura while the current
		// victim has at least one debuff.
		if (elite->GetOriginTrait() & ORIGIN_TRAIT_SCAVENGER)
		{
			bool shouldHaveScavengerAura = false;
			if (Unit* victim = creature->GetVictim())
			{
				if (Player* victimPlayer = victim->ToPlayer())
				{
					for (auto const& itr : victimPlayer->GetAppliedAuras())
					{
						if (itr.second && !itr.second->IsPositive())
						{
							shouldHaveScavengerAura = true;
							break;
						}
					}
				}
			}

			bool hasScavengerAura = creature->HasAura(NemesisEliteConstants::ELITE_AURA_SCAVENGER);
			if (shouldHaveScavengerAura && !hasScavengerAura)
			{
				int32 damagePercent = static_cast<int32>(_traitConfig.scavengerDamageBasePercent + (static_cast<float>(elite->GetEffectiveLevel()) * _traitConfig.scavengerDamagePerLevelPercent));
				if (Aura* aura = creature->AddAura(NemesisEliteConstants::ELITE_AURA_SCAVENGER, creature))
				{
					if (AuraEffect* effect = aura->GetEffect(0))
						effect->ChangeAmount(damagePercent);
				}
			}
			else if (!shouldHaveScavengerAura && hasScavengerAura)
			{
				creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_SCAVENGER);
			}
		}

		// Duelist trait: apply a level-scaled damage bonus aura while exactly one
		// player is attacking this nemesis.
		if (elite->GetOriginTrait() & ORIGIN_TRAIT_DUELIST)
		{
			uint32 playerThreatCount = 0;
			for (auto const& ref : creature->GetThreatMgr().GetThreatList())
			{
				if (ref->getTarget() && ref->getTarget()->ToPlayer())
					++playerThreatCount;
			}

			bool shouldHaveDuelistAura = (playerThreatCount == 1);
			bool hasDuelistAura = creature->HasAura(NemesisEliteConstants::ELITE_AURA_DUELIST);
			if (shouldHaveDuelistAura && !hasDuelistAura)
			{
				int32 damagePercent = static_cast<int32>(_traitConfig.duelistDamageBasePercent + (static_cast<float>(elite->GetEffectiveLevel()) * _traitConfig.duelistDamagePerLevelPercent));
				if (Aura* aura = creature->AddAura(NemesisEliteConstants::ELITE_AURA_DUELIST, creature))
				{
					if (AuraEffect* effect = aura->GetEffect(0))
						effect->ChangeAmount(damagePercent);
				}
			}
			else if (!shouldHaveDuelistAura && hasDuelistAura)
			{
				creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_DUELIST);
			}
		}

		// Scarred trait: periodically apply a healing reduction debuff to the current
		// melee victim.
		if (elite->GetEarnedTraits() & EARNED_TRAIT_SCARRED)
		{
			uint32 scarredNow = getMSTime();

			if (!_scarredNextCastAt.count(spawnId))
				_scarredNextCastAt[spawnId] = scarredNow;

			if (scarredNow >= _scarredNextCastAt[spawnId])
			{
				if (Unit* victim = creature->GetVictim())
				{
					if (Player* victimPlayer = victim->ToPlayer())
					{
						int32 reductionPercent = -static_cast<int32>(_traitConfig.scarredHealingReductionPercent);
						if (Aura* aura = creature->AddAura(NemesisEliteConstants::ELITE_AURA_SCARRED, victimPlayer))
						{
							if (AuraEffect* effect = aura->GetEffect(0))
								effect->ChangeAmount(reductionPercent);
						}
						_scarredNextCastAt[spawnId] = scarredNow + _traitConfig.scarredReapplyIntervalMs;

						LOG_DEBUG("elite", "{} Scarred '{}' (elite_id {}) applied healing reduction to {}.", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), victimPlayer->GetName());
					}
				}
			}
		}

		// Enraged trait: stacking damage buff based on the number of players currently
		// on the threat list.
		if (elite->GetEarnedTraits() & EARNED_TRAIT_ENRAGED)
		{
			uint32 enragedPlayerCount = 0;
			for (auto const& ref : creature->GetThreatMgr().GetThreatList())
			{
				if (ref->getTarget() && ref->getTarget()->ToPlayer())
					++enragedPlayerCount;
			}

			bool shouldHaveEnragedAura = (enragedPlayerCount >= 2);
			bool hasEnragedAura = creature->HasAura(NemesisEliteConstants::ELITE_AURA_ENRAGED);
			if (shouldHaveEnragedAura)
			{
				int32 damagePercent = static_cast<int32>(_traitConfig.enragedDamageBasePercent + (static_cast<float>(elite->GetEffectiveLevel()) * _traitConfig.enragedDamagePerLevelPercent) + (static_cast<float>(enragedPlayerCount - 1) * _traitConfig.enragedDamagePerPlayerPercent));
				if (!hasEnragedAura)
				{
					if (Aura* aura = creature->AddAura(NemesisEliteConstants::ELITE_AURA_ENRAGED, creature))
					{
						if (AuraEffect* effect = aura->GetEffect(0))
							effect->ChangeAmount(damagePercent);
					}
				}
				else
				{
					// Aura exists but player count may have changed — update the amount.
					if (Aura* aura = creature->GetAura(NemesisEliteConstants::ELITE_AURA_ENRAGED))
					{
						if (AuraEffect* effect = aura->GetEffect(0))
							effect->ChangeAmount(damagePercent);
					}
				}
			}
			else if (!shouldHaveEnragedAura && hasEnragedAura)
			{
				creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_ENRAGED);
			}
		}
	}
	else if (_inCombatElites.count(spawnId))
	{
		// Strip the conditional executioner aura if it was active.
		if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_EXECUTIONER))
			creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_EXECUTIONER);

		// Strip the conditional giant slayer aura if it was active.
		if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_GIANT_SLAYER))
			creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_GIANT_SLAYER);

		// Strip the Umbral Burst aura if it was active.
		if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_UMBRAL_BURST))
			creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_UMBRAL_BURST);

		// Strip the Scavenger damage bonus aura if it was active.
		if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_SCAVENGER))
			creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_SCAVENGER);

		// Strip the Duelist damage bonus aura if it was active.
		if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_DUELIST))
			creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_DUELIST);

		// Strip the Enraged damage buff if it was active.
		if (creature->HasAura(NemesisEliteConstants::ELITE_AURA_ENRAGED))
			creature->RemoveAura(NemesisEliteConstants::ELITE_AURA_ENRAGED);

		// Snapshot encounter state before clearing tracking sets.
		bool wasCCd = _ccHitElites.count(spawnId) > 0;
		bool wasHealedAgainst = _healedAgainstElites.count(spawnId) > 0;
		uint32 uniqueAttackerCount = 0;
		{
			auto hitIt = _combatPlayerHits.find(spawnId);
			if (hitIt != _combatPlayerHits.end())
				uniqueAttackerCount = static_cast<uint32>(hitIt->second.size());
		}

		// Clear combat state
		ClearCombatState(spawnId, creature->GetMap());

		// Survivor
		elite->IncrementSurvivalAttempts();
		if (!(elite->GetEarnedTraits() & EARNED_TRAIT_SURVIVOR) && elite->GetSurvivalAttempts() >= _traitConfig.survivorAttemptsThreshold)
		{
			elite->AddEarnedTrait(EARNED_TRAIT_SURVIVOR);

			uint32 newThreat = _owner.CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
			elite->SetThreatScore(newThreat);

			LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait SURVIVOR | attempts={} threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), elite->GetSurvivalAttempts(), newThreat);
		}

		// Spellproof: survived combat while CC'd.
		if (wasCCd && !(elite->GetEarnedTraits() & EARNED_TRAIT_SPELLPROOF))
		{
			elite->AddEarnedTrait(EARNED_TRAIT_SPELLPROOF);

			uint32 newThreat = _owner.CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
			elite->SetThreatScore(newThreat);

			LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait SPELLPROOF | threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), newThreat);
		}

		// Scarred: survived while a player healed during the encounter.
		if (wasHealedAgainst && !(elite->GetEarnedTraits() & EARNED_TRAIT_SCARRED))
		{
			elite->AddEarnedTrait(EARNED_TRAIT_SCARRED);

			uint32 newThreat = _owner.CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
			elite->SetThreatScore(newThreat);

			LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait SCARRED | threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), newThreat);
		}

		// Enraged: survived combat against multiple distinct players.
		if (uniqueAttackerCount >= _traitConfig.enragedPlayerThreshold && !(elite->GetEarnedTraits() & EARNED_TRAIT_ENRAGED))
		{
			elite->AddEarnedTrait(EARNED_TRAIT_ENRAGED);

			uint32 newThreat = _owner.CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
			elite->SetThreatScore(newThreat);

			LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait ENRAGED | attackers={} threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), uniqueAttackerCount, newThreat);
		}

	}
}

void NemesisEliteTraitBehavior::ClearCombatState(uint32 spawnId, Map* map)
{
	// Remove any adds spawned by ambusher
	DespawnAmbusherAdds(spawnId, map);

	// Clear tracking in maps
	_ambusherNextPullAt.erase(spawnId);
	_ambusherAddsPulled.erase(spawnId);
	_ambusherNextHomeUpdateAt.erase(spawnId);
	_inCombatElites.erase(spawnId);
	_fledThisCombat.erase(spawnId);
	_cowardCandidates.erase(spawnId);
	_cowardHealAt.erase(spawnId);
	_ccHitElites.erase(spawnId);
	_healedAgainstElites.erase(spawnId);
	_combatPlayerHits.erase(spawnId);
	_mageBaneNextCastAt.erase(spawnId);
	_healerBaneNextCastAt.erase(spawnId);
	_opportunistNextCheapShotAt.erase(spawnId);
	_umbralBurstNextCastAt.erase(spawnId);
	_scarredNextCastAt.erase(spawnId);
	_deathblowState.erase(spawnId);
	_deathblowRearmAt.erase(spawnId);
}

void NemesisEliteTraitBehavior::DespawnAmbusherAdds(uint32 spawnId, Map* map)
{
	auto addIterator = _ambusherSpawnedAdds.find(spawnId);
	if (addIterator == _ambusherSpawnedAdds.end())
		return;

	for (ObjectGuid const& guid : addIterator->second)
	{
		if (Creature* add = map->GetCreature(guid))
		{
			add->DespawnOrUnsummon();
		}
	}
	_ambusherSpawnedAdds.erase(addIterator);
}

uint32 NemesisEliteTraitBehavior::GetDeathblowStrikeDamage(uint32 spawnId) const
{
	auto const& loadedElites = _owner.GetLoadedElites();
	auto eliteIterator = loadedElites.find(spawnId);
	if (eliteIterator == loadedElites.end())
		return _traitConfig.deathblowBaseDamage;

	return _traitConfig.deathblowBaseDamage + (static_cast<uint32>(eliteIterator->second->GetEffectiveLevel()) * _traitConfig.deathblowDamagePerLevel);
}

void NemesisEliteTraitBehavior::MarkDeathblowSpent(uint32 spawnId)
{
	_deathblowState[spawnId] = DEATHBLOW_SPENT;
}

uint32 NemesisEliteTraitBehavior::GetCowardHealAmount(uint32 spawnId) const
{
	auto const& loadedElites = _owner.GetLoadedElites();
	auto eliteIterator = loadedElites.find(spawnId);
	if (eliteIterator == loadedElites.end())
		return _traitConfig.cowardHealBase;

	return _traitConfig.cowardHealBase + (static_cast<uint32>(eliteIterator->second->GetEffectiveLevel()) * _traitConfig.cowardHealPerLevel);
}

uint32 NemesisEliteTraitBehavior::GetUmbralBurstDamage(uint32 spawnId) const
{
	auto const& loadedElites = _owner.GetLoadedElites();
	auto eliteIterator = loadedElites.find(spawnId);
	if (eliteIterator == loadedElites.end())
		return _traitConfig.umbralBurstBase;

	return _traitConfig.umbralBurstBase + (static_cast<uint32>(eliteIterator->second->GetEffectiveLevel()) * _traitConfig.umbralBurstPerLevel);
}

void NemesisEliteTraitBehavior::GrantNomadTrait(uint32 spawnId)
{
	auto const& loadedElites = _owner.GetLoadedElites();
	auto eliteIterator = loadedElites.find(spawnId);
	if (eliteIterator == loadedElites.end())
		return;

	auto& elite = eliteIterator->second;
	if (elite->GetEarnedTraits() & EARNED_TRAIT_NOMAD)
		return;

	elite->AddEarnedTrait(EARNED_TRAIT_NOMAD);

	uint32 newThreat = _owner.CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
	elite->SetThreatScore(newThreat);

	LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait NOMAD | areas reached threshold, threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), newThreat);
}

void NemesisEliteTraitBehavior::GrantStudiousTrait(uint32 spawnId)
{
	auto const& loadedElites = _owner.GetLoadedElites();
	auto eliteIterator = loadedElites.find(spawnId);
	if (eliteIterator == loadedElites.end())
		return;

	auto& elite = eliteIterator->second;
	if (elite->GetEarnedTraits() & EARNED_TRAIT_STUDIOUS)
		return;

	elite->AddEarnedTrait(EARNED_TRAIT_STUDIOUS);

	uint32 newThreat = _owner.CalculateThreatScore(elite->GetEffectiveLevel(), elite->GetKillCount(), elite->GetUniqueKillCount(), elite->GetOriginTrait(), elite->GetEarnedTraits(), elite->GetPromotedAt());
	elite->SetThreatScore(newThreat);

	LOG_INFO("elite", "{} '{}' (elite_id {}) earned trait STUDIOUS | threat={}", NemesisConstants::LOG_PREFIX, elite->GetName(), elite->GetEliteId(), newThreat);
}
