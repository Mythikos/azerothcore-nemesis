#ifndef MOD_NEMESIS_BOUNTY_BOARD_CONFIGURATION_H
#define MOD_NEMESIS_BOUNTY_BOARD_CONFIGURATION_H

#include "Common.h"
#include <string>

// ============================================================================
// Configuration struct for the Nemesis Bounty Board system.
// All tunable values are loaded from the `nemesis_configuration` table at
// startup. Default member initializers match the SQL defaults so the system
// has sane values even if the DB rows are missing.
// ============================================================================

// ---------------------------------------------------------------------------
// Leaderboard display and schedule info forwarded to the client addon.
// ---------------------------------------------------------------------------
struct BountyBoardConfig
{
    uint32 topEliteCount = 10;
    std::string scheduleDays;
    std::string scheduleTimes;
    std::string durationMinutes;
};

#endif // MOD_NEMESIS_BOUNTY_BOARD_CONFIGURATION_H
