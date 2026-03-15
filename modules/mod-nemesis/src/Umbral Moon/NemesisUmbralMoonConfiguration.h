#ifndef MOD_NEMESIS_UMBRAL_MOON_CONFIGURATION_H
#define MOD_NEMESIS_UMBRAL_MOON_CONFIGURATION_H

#include "Common.h"
#include <string>

// ============================================================================
// Configuration structs for the Nemesis Umbral Moon system.
// All tunable values are loaded from the `nemesis_configuration` table at
// startup. Default member initializers match the SQL defaults so the system
// has sane values even if the DB rows are missing.
// ============================================================================

// ---------------------------------------------------------------------------
// Schedule timing — days, times, and duration of umbral moon windows.
// ---------------------------------------------------------------------------
struct UmbralMoonScheduleConfig
{
	std::string scheduleDays = "Wed,Sat";
	std::string scheduleTimes = "08:00,20:00";
	uint32 durationMinutes = 120;
};

// ---------------------------------------------------------------------------
// Gameplay effects — creature stat boosts, XP multiplier, and sky transition.
// ---------------------------------------------------------------------------
struct UmbralMoonEffectConfig
{
	float mobHPBoostPercent = 100.0f;
	float mobDamageBoostPercent = 100.0f;
	float bonusXPMultiplier = 1.5f;
	uint8 transitionToTime = 3;
	float transitionSpeed = 60.0f;
};

#endif // MOD_NEMESIS_UMBRAL_MOON_CONFIGURATION_H
