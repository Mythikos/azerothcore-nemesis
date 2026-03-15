#ifndef NEMESIS_HELPERS_H
#define NEMESIS_HELPERS_H

#include "NemesisConstants.h"
#include "QueryResult.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include <array>
#include <sstream>

// ---------------------------------------------------------------------------
// Configuration loading helpers — used by all managers to read from
// the nemesis_configuration table and parse comma-delimited tier values.
// ---------------------------------------------------------------------------

inline std::string NemesisLoadConfigValue(const std::string& key, const std::string& defaultValue)
{
	QueryResult result = WorldDatabase.Query("SELECT config_value FROM nemesis_configuration WHERE config_key = '{}'", key);
	if (result)
		return result->Fetch()[0].Get<std::string>();

	LOG_WARN("server.loading", "{} Config key '{}' not found, using default: '{}'", NemesisConstants::LOG_PREFIX, key, defaultValue);
	return defaultValue;
}

inline int32 NemesisLoadConfigInt(const std::string& key, int32 defaultValue)
{
	return std::stoi(NemesisLoadConfigValue(key, std::to_string(defaultValue)));
}

inline float NemesisLoadConfigFloat(const std::string& key, float defaultValue)
{
	return std::stof(NemesisLoadConfigValue(key, std::to_string(defaultValue)));
}

inline std::array<int32, 4> NemesisParseTierInt(const std::string& csv, const std::array<int32, 4>& defaults)
{
	std::array<int32, 4> result = defaults;
	std::istringstream stream(csv);
	std::string token;
	for (uint8 i = 0; i < 4 && std::getline(stream, token, ','); ++i)
		result[i] = std::stoi(token);
	return result;
}

inline std::array<float, 4> NemesisParseTierFloat(const std::string& csv, const std::array<float, 4>& defaults)
{
	std::array<float, 4> result = defaults;
	std::istringstream stream(csv);
	std::string token;
	for (uint8 i = 0; i < 4 && std::getline(stream, token, ','); ++i)
		result[i] = std::stof(token);
	return result;
}

// ---------------------------------------------------------------------------
// Stat modifier math — compute the multiplicative inverse of a TOTAL_PCT
// modifier so it can be cleanly reverted.
//
// ApplyStatPctModifier(TOTAL_PCT) is multiplicative:
//     internal_modifier *= (100 + val) / 100
//
// Applying +val then -val does NOT cancel out. For example, +100% (2x) then
// -100% gives 2.0 * (0/100) = 0.0, not 1.0. To invert a prior +val, we need
// a reversal value whose multiplier is the reciprocal of the original:
//     (100 + reversal) / 100 = 100 / (100 + val)
//     reversal = (-100 * val) / (100 + val)
// ---------------------------------------------------------------------------

inline float InversePctModifier(float appliedPct)
{
	return (-100.0f * appliedPct) / (100.0f + appliedPct);
}

#endif // NEMESIS_HELPERS_H
