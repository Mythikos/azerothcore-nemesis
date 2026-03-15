#ifndef MOD_NEMESIS_REVENGE_CONFIGURATION_H
#define MOD_NEMESIS_REVENGE_CONFIGURATION_H

#include "Common.h"

// ============================================================================
// Configuration struct for the Nemesis Revenge system.
// All tunable values are loaded from the `nemesis_configuration` table at
// startup. Default member initializers match the SQL defaults so the system
// has sane values even if the DB rows are missing.
// ============================================================================

// ---------------------------------------------------------------------------
// Group vengeance sharing and title progression.
// ---------------------------------------------------------------------------
struct RevengeConfig
{
	bool groupVengeanceEnabled = true;
	float groupVengeanceMaxDistance = 100.0f;
};

#endif // MOD_NEMESIS_REVENGE_CONFIGURATION_H
