#include "NemesisBountyBoardManager.h"
#include "NemesisConstants.h"
#include "NemesisHelpers.h"
#include "Log.h"

NemesisBountyBoardManager* NemesisBountyBoardManager::Instance()
{
	static NemesisBountyBoardManager instance;
	return &instance;
}

void NemesisBountyBoardManager::LoadConfig()
{
	LOG_INFO("nemesis_bounty_board", "{} Loading configuration...", NemesisConstants::LOG_PREFIX);

	_config.topEliteCount = static_cast<uint32>(std::max(1, NemesisLoadConfigInt("bounty_board_top_count", 10)));

	// Umbral Moon keys belong to the umbral moon subsystem. Leave empty if not present.
	_config.scheduleDays = NemesisLoadConfigValue("umbral_moon_schedule_days", "");
	_config.scheduleTimes = NemesisLoadConfigValue("umbral_moon_schedule_times", "");
	_config.durationMinutes = NemesisLoadConfigValue("umbral_moon_duration_minutes", "");

	LOG_INFO("nemesis_bounty_board", "{} Config loaded. TopEliteCount: {}, ScheduleDays: '{}', ScheduleTimes: '{}', DurationMinutes: '{}'",
		NemesisConstants::LOG_PREFIX,
		_config.topEliteCount,
		_config.scheduleDays,
		_config.scheduleTimes,
		_config.durationMinutes);
}
