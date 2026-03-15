#pragma once

#include <cstdint>
#include <string>
#include <array>

namespace NemesisRevengeConstants
{
	// -----------------------------------------------------------------------
	// Title DBC IDs — must match entries added to CharTitles.dbc.
	// DBC index 178-183, Mask_ID (bit_index) 143-148.
	// -----------------------------------------------------------------------
	constexpr uint32_t TITLE_ID_THE_WRONGED     = 178;  // Mask_ID 143
	constexpr uint32_t TITLE_ID_THE_GRUDGEKEEPER = 179;  // Mask_ID 144
	constexpr uint32_t TITLE_ID_THE_RELENTLESS   = 180;  // Mask_ID 145
	constexpr uint32_t TITLE_ID_THE_VENGEFUL     = 181;  // Mask_ID 146
	constexpr uint32_t TITLE_ID_THE_UNFORGIVING  = 182;  // Mask_ID 147
	constexpr uint32_t TITLE_ID_THE_INEVITABLE   = 183;  // Mask_ID 148

	// -----------------------------------------------------------------------
	// Title thresholds — vengeance kill count required for each title.
	// Ordered from lowest to highest for iteration.
	// -----------------------------------------------------------------------
	struct TitleThreshold
	{
		uint32_t vengeanceCount;
		uint32_t titleId;
		const char* titleName;
	};

	constexpr uint32_t TITLE_TIER_COUNT = 6;

	// Thresholds loaded from config at startup. These are the defaults.
	// The actual runtime values live in the extern array below.
	constexpr std::array<TitleThreshold, TITLE_TIER_COUNT> DEFAULT_TITLE_THRESHOLDS = {{
		{   1, TITLE_ID_THE_WRONGED,     "the Wronged"     },
		{   5, TITLE_ID_THE_GRUDGEKEEPER, "the Grudgekeeper" },
		{  15, TITLE_ID_THE_RELENTLESS,   "the Relentless"   },
		{  25, TITLE_ID_THE_VENGEFUL,     "the Vengeful"     },
		{  50, TITLE_ID_THE_UNFORGIVING,  "the Unforgiving"  },
		{ 100, TITLE_ID_THE_INEVITABLE,   "the Inevitable"   },
	}};

}
