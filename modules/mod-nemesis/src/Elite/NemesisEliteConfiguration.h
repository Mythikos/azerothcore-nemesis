#ifndef MOD_NEMESIS_ELITE_CONFIGURATION_H
#define MOD_NEMESIS_ELITE_CONFIGURATION_H

#include "Common.h"
#include <array>
#include <string>

// ============================================================================
// Configuration structs for the Nemesis Elite system.
// All tunable values are loaded from the `nemesis_configuration` table at
// startup. Default member initializers match the SQL defaults so the system
// has sane values even if the DB rows are missing.
// Each std::array<T, 4> holds per-quality-tier values: { Green, Blue, Purple, Legendary }.
// ============================================================================

// ---------------------------------------------------------------------------
// Promotion, stat scaling, leveling, and visual growth.
// ---------------------------------------------------------------------------
struct EliteScalingConfig
{
	float promotionChanceNormal = 7.0f;
	float promotionChanceUmbralMoon = 25.0f;
	float hpBoostPercent = 25.0f;
	float damageBoostPercent = 25.0f;
	uint8 levelGainOnPromote = 3;
	uint8 levelGainPerKill = 1;
	uint8 levelCap = 83;
	uint32 ageLevelIntervalHours = 24;
	uint32 growLevelsPerStack = 10;
};

// ---------------------------------------------------------------------------
// Threat score calculation weights.
// ---------------------------------------------------------------------------
struct EliteThreatConfig
{
	uint32 weightLevel = 5;
	uint32 weightKill = 1;
	uint32 weightUniqueKill = 12;
	uint32 weightTrait = 15;
	uint32 ageBonusCap = 100;
	uint32 ageBonusPerDay = 4;
};

// ---------------------------------------------------------------------------
// Origin and earned trait thresholds, intervals, and scaled values.
// ---------------------------------------------------------------------------
struct EliteTraitConfig
{
	// Ambusher
	float ambusherSpawnRadius = 50.0f;
	uint32 ambusherPullIntervalMs = 8000;
	uint32 ambusherAddsPerPull = 2;
	uint32 ambusherMaxAddsPerEncounter = 8;

	// Notorious
	uint32 notoriousKillThreshold = 5;
	float notoriousDamageBasePercent = 5.0f;
	float notoriousDamagePerLevelPercent = 0.15f;
	float notoriousHealthBase = 50.0f;
	float notoriousHealthPerLevel = 10.0f;

	// Territorial
	uint32 territorialDaysThreshold = 3;
	float territorialWanderLevelMultiplier = 2.0f;
	float territorialDetectionMultiplier = 1.5f;
	float territorialWanderMinRadius = 20.0f;

	// Survivor
	uint32 survivorAttemptsThreshold = 3;
	float survivorArmorBase = 100.0f;
	float survivorArmorPerLevel = 5.0f;
	float survivorCritReductionBase = 5.0f;
	float survivorCritReductionPerLevel = 0.15f;

	// Coward
	float cowardHpThreshold = 15.0f;
	float cowardFleeChance = 30.0f;
	uint32 cowardFleeDurationMs = 5000;
	uint32 cowardHealBase = 100;
	uint32 cowardHealPerLevel = 25;

	// Enraged
	uint32 enragedPlayerThreshold = 5;
	float enragedDamageBasePercent = 3.0f;
	float enragedDamagePerLevelPercent = 0.10f;
	float enragedDamagePerPlayerPercent = 4.0f;

	// Sage
	uint32 sageDaysThreshold = 7;

	// Studious
	float studiousSearchRange = 30.0f;

	// Nomad
	uint32 nomadAreaThreshold = 3;
	float nomadSpeedPercent = 30.0f;

	// Executioner
	float executionerHpThreshold = 20.0f;
	float executionerAuraHpThreshold = 20.0f;

	// Mage Bane
	uint32 mageBaneSilenceIntervalMs = 10000;
	float mageBaneSilenceRange = 30.0f;

	// Healer Bane
	uint32 healerBaneReapplyIntervalMs = 12000;

	// Giant Slayer
	uint32 giantSlayerLevelGap = 10;
	uint32 giantSlayerAuraLevelGap = 5;

	// Opportunist
	uint32 opportunistCheapShotIntervalMs = 15000;

	// Umbral Burst
	uint32 umbralBurstRecastIntervalMs = 10000;
	uint32 umbralBurstBase = 50;
	uint32 umbralBurstPerLevel = 15;

	// Underdog
	uint32 underdogLevelGap = 3;

	// Scavenger
	float scavengerDamageBasePercent = 6.0f;
	float scavengerDamagePerLevelPercent = 0.15f;

	// Duelist
	float duelistDamageBasePercent = 8.0f;
	float duelistDamagePerLevelPercent = 0.25f;

	// Plunderer
	uint32 plundererGoldPerLevel = 10000;
	uint32 plundererBonusGoldBase = 5000;
	uint32 plundererBonusGoldPerLevel = 1000;

	// Dayborn / Nightborn
	uint32 daybornHourStart = 6;
	uint32 daybornHourEnd = 17;
	float daybornDamageBasePercent = 5.0f;
	float daybornDamagePerLevelPercent = 0.15f;
	float nightbornDamageBasePercent = 5.0f;
	float nightbornDamagePerLevelPercent = 0.15f;

	// Deathblow
	float deathblowHpThreshold = 25.0f;
	uint32 deathblowBaseDamage = 200;
	uint32 deathblowDamagePerLevel = 50;
	uint32 deathblowRearmCooldownMs = 10000;

	// Blight
	float blightDamageBase = 10.0f;
	float blightDamagePerLevel = 3.0f;

	// Spellproof
	float spellproofCcDurationReduction = 50.0f;

	// Scarred
	float scarredHealingReductionPercent = 30.0f;
	uint32 scarredReapplyIntervalMs = 12000;
};

// ---------------------------------------------------------------------------
// Affixed loot system: drop rules, tier thresholds, and all tier-scaled
// values for item stats, equipped auras, and runtime proc hooks.
// ---------------------------------------------------------------------------
struct EliteAffixConfig
{
	// Loot generation
	uint32 dropChance = 85;
	uint32 tierThresholdBlue = 300;
	uint32 tierThresholdPurple = 600;
	uint32 tierThresholdLegendary = 700;
	uint32 legendaryTopCount = 10;
	std::string flavorTemplates = "Taken from the corpse of {name}.;Looted from {name}.;Pried from the remains of {name}.;Stripped from {name} after its final defeat.;Once wielded by {name}.";

	// Prefix item stat injection
	std::array<int32, 4> ambusherCritRating = {{ 15, 26, 38, 52 }};
	std::array<int32, 4> healerBaneSpellPower = {{ 20, 35, 50, 70 }};
	std::array<int32, 4> giantSlayerAttackPower = {{ 20, 35, 50, 70 }};
	std::array<int32, 4> underdogExpertise = {{ 10, 18, 26, 36 }};
	std::array<int32, 4> ironbreakerArmorPen = {{ 15, 26, 38, 52 }};

	// Suffix item stat injection
	std::array<int32, 4> notoriousAllStats = {{ 4, 7, 10, 14 }};
	std::array<int32, 4> survivorArmor = {{ 80, 140, 200, 280 }};
	std::array<int32, 4> spellproofResilience = {{ 15, 26, 38, 52 }};
	std::array<int32, 4> scarredMp5 = {{ 6, 10, 15, 21 }};
	std::array<int32, 4> enragedHaste = {{ 15, 26, 38, 52 }};
	std::array<int32, 4> daybornAttackPower = {{ 15, 26, 38, 52 }};
	std::array<int32, 4> nightbornAgility = {{ 12, 21, 30, 42 }};

	// Equipped aura values
	std::array<float, 4> deathblowCritPercent = {{ 2.0f, 4.0f, 5.0f, 7.0f }};
	std::array<float, 4> wanderingSpeedPercent = {{ 3.0f, 5.0f, 7.0f, 8.0f }};
	std::array<float, 4> cowardiceThreatPercent = {{ 3.0f, 5.0f, 8.0f, 12.0f }};
	std::array<float, 4> dominionThreatPercent = {{ 3.0f, 5.0f, 8.0f, 12.0f }};
	std::array<int32, 4> professionSkillBonus = {{ 100, 100, 100, 100 }};

	// Proc values (runtime hooks)
	std::array<float, 4> executionerProcChance = {{ 3.0f, 5.0f, 7.0f, 10.0f }};
	std::array<int32, 4> executionerProcDamage = {{ 40, 70, 100, 140 }};
	float executionerHpThreshold = 20.0f;
	std::array<float, 4> mageBaneProcChance = {{ 2.0f, 3.0f, 4.0f, 6.0f }};
	std::array<int32, 4> mageBaneSlowPercent = {{ 15, 20, 25, 30 }};
	std::array<int32, 4> mageBaneDurationMs = {{ 4000, 5000, 6000, 6000 }};
	std::array<float, 4> opportunistProcChance = {{ 2.0f, 3.0f, 4.0f, 6.0f }};
	std::array<int32, 4> opportunistDurationMs = {{ 3000, 4000, 5000, 5000 }};
	std::array<float, 4> scavengerBonusDamagePercent = {{ 3.0f, 5.0f, 8.0f, 11.0f }};
	std::array<float, 4> duelistBonusDamagePercent = {{ 3.0f, 5.0f, 8.0f, 11.0f }};
	std::array<int32, 4> umbralforgedAoeDamage = {{ 8, 14, 20, 28 }};
	std::array<int32, 4> blightReflectDamage = {{ 10, 18, 26, 36 }};
	std::array<float, 4> plundererBonusGoldPercent = {{ 5.0f, 10.0f, 15.0f, 25.0f }};
	std::array<float, 4> sageBonusXpPercent = {{ 3.0f, 5.0f, 8.0f, 12.0f }};

	// Timing
	uint32 umbralEchoIntervalMs = 3000;
};

#endif // MOD_NEMESIS_ELITE_CONFIGURATION_H
