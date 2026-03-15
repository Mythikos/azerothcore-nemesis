#ifndef NEMESIS_ELITE_HELPERS_H
#define NEMESIS_ELITE_HELPERS_H

#include "NemesisEliteConstants.h"
#include "NemesisHelpers.h"
#include <vector>
#include <algorithm>

// ---------------------------------------------------------------------------
// Trait utility functions — mapping trait enum values to display names,
// randomized prefix/suffix strings, and reverse name lookups.
// Lives in the NemesisEliteConstants namespace so existing callers
// continue to work unchanged.
// ---------------------------------------------------------------------------
namespace NemesisEliteConstants
{
	// Get a random prefix for an elite based on one of its origin traits.
	inline const char* GetOriginTraitPrefix(uint32 trait)
	{
		std::vector<const char*> prefixOptions;

		switch (trait)
		{
			case ORIGIN_TRAIT_AMBUSHER:
				prefixOptions.push_back("Lurking");
				prefixOptions.push_back("Stalking");
				prefixOptions.push_back("Conniving");
				prefixOptions.push_back("Opportune");
				break;
			case ORIGIN_TRAIT_EXECUTIONER:
				prefixOptions.push_back("Executioner");
				prefixOptions.push_back("Finishing");
				prefixOptions.push_back("Merciless");
				break;
			case ORIGIN_TRAIT_MAGE_BANE:
				prefixOptions.push_back("Spell-Breaking");
				prefixOptions.push_back("Mana-Starved");
				prefixOptions.push_back("Arcane-Bane");
				prefixOptions.push_back("Hex-Scarred");
				break;
			case ORIGIN_TRAIT_HEALER_BANE:
				prefixOptions.push_back("Bane-Touched");
				prefixOptions.push_back("Faith-Crushing");
				break;
			case ORIGIN_TRAIT_GIANT_SLAYER:
				prefixOptions.push_back("Giant-Slaying");
				prefixOptions.push_back("Oathbreaker");
				prefixOptions.push_back("Apex");
				break;
			case ORIGIN_TRAIT_OPPORTUNIST:
				prefixOptions.push_back("Skulking");
				prefixOptions.push_back("Vulturous");
				prefixOptions.push_back("Prowling");
				prefixOptions.push_back("Craven");
				break;
			case ORIGIN_TRAIT_UMBRALFORGED:
				prefixOptions.push_back("Umbra-Forged");
				prefixOptions.push_back("Moon-Cursed");
				prefixOptions.push_back("Shadow-Wrought");
				break;
			case ORIGIN_TRAIT_DEATHBLOW:
				prefixOptions.push_back("Devastating");
				prefixOptions.push_back("Ruinous");
				prefixOptions.push_back("Annihilating");
				break;
			case ORIGIN_TRAIT_UNDERDOG:
				prefixOptions.push_back("Defiant");
				prefixOptions.push_back("Unbroken");
				break;
			case ORIGIN_TRAIT_SCAVENGER:
				prefixOptions.push_back("Corpse-Fed");
				prefixOptions.push_back("Carrion");
				break;
			case ORIGIN_TRAIT_DUELIST:
				prefixOptions.push_back("Dueling");
				prefixOptions.push_back("Challenger");
				prefixOptions.push_back("Honorbound");
				break;
			case ORIGIN_TRAIT_PLUNDERER:
				prefixOptions.push_back("Plundering");
				prefixOptions.push_back("Pillaging");
				prefixOptions.push_back("Greedy");
				break;
			case ORIGIN_TRAIT_IRONBREAKER:
				prefixOptions.push_back("Iron-Breaking");
				prefixOptions.push_back("Sunder");
				break;
			case ORIGIN_TRAIT_SKINNER:
				prefixOptions.push_back("Pelt-Draped");
				prefixOptions.push_back("Flayed");
				prefixOptions.push_back("Hide-Hungry");
				break;
			case ORIGIN_TRAIT_ORE_GORGED:
				prefixOptions.push_back("Ore-Gorged");
				prefixOptions.push_back("Vein-Cracker");
				prefixOptions.push_back("Stone-Bellied");
				break;
			case ORIGIN_TRAIT_ROOT_RIPPER:
				prefixOptions.push_back("Root-Ripping");
				prefixOptions.push_back("Bloom-Devouring");
				prefixOptions.push_back("Thorn-Crowned");
				break;
			case ORIGIN_TRAIT_FORGE_BREAKER:
				prefixOptions.push_back("Forge-Breaking");
				prefixOptions.push_back("Anvil-Born");
				prefixOptions.push_back("Hammer-Scarred");
				break;
			case ORIGIN_TRAIT_HIDE_MANGLER:
				prefixOptions.push_back("Hide-Mangling");
				prefixOptions.push_back("Leather-Gnawing");
				prefixOptions.push_back("Stitchripper");
				break;
			case ORIGIN_TRAIT_THREAD_RIPPER:
				prefixOptions.push_back("Thread-Ripping");
				prefixOptions.push_back("Loom-Wrecking");
				prefixOptions.push_back("Cloth-Shredding");
				break;
			case ORIGIN_TRAIT_GEAR_GRINDER:
				prefixOptions.push_back("Gear-Grinding");
				prefixOptions.push_back("Spring-Jawed");
				prefixOptions.push_back("Cog-Crunching");
				break;
			case ORIGIN_TRAIT_VIAL_SHATTER:
				prefixOptions.push_back("Vial-Shattering");
				prefixOptions.push_back("Brew-Spilling");
				prefixOptions.push_back("Fume-Drunk");
				break;
			case ORIGIN_TRAIT_RUNE_EATER:
				prefixOptions.push_back("Rune-Eating");
				prefixOptions.push_back("Glyph-Scarred");
				prefixOptions.push_back("Spell-Leeching");
				break;
			case ORIGIN_TRAIT_GEM_CRUSHER:
				prefixOptions.push_back("Gem-Crushing");
				prefixOptions.push_back("Facet-Cracking");
				prefixOptions.push_back("Shard-Toothed");
				break;
			case ORIGIN_TRAIT_INK_DRINKER:
				prefixOptions.push_back("Ink-Drinking");
				prefixOptions.push_back("Scroll-Chewing");
				prefixOptions.push_back("Parchment-Stained");
				break;
			default:
				prefixOptions.push_back("Risen");
				prefixOptions.push_back("Forsaken");
				prefixOptions.push_back("Awakened");
				prefixOptions.push_back("Ascended");
				prefixOptions.push_back("Bloodied");
				break;
		}

		uint32 prefixIndex = urand(0, static_cast<uint32>(prefixOptions.size() - 1));
		return prefixOptions[prefixIndex];
	}

	// Get a random suffix for an elite based on one of its earned traits.
	inline const char* GetEarnedTraitSuffix(uint32 trait)
	{
		std::vector<const char*> suffixOptions;

		switch (trait)
		{
			case EARNED_TRAIT_COWARD:
				suffixOptions.push_back("of Cowardice");
				suffixOptions.push_back("of the Craven");
				suffixOptions.push_back("of Fleeting Shadow");
				break;
			case EARNED_TRAIT_NOTORIOUS:
				suffixOptions.push_back("of Notoriety");
				suffixOptions.push_back("of Infamy");
				suffixOptions.push_back("of the Manhunter");
				break;
			case EARNED_TRAIT_SURVIVOR:
				suffixOptions.push_back("of Endurance");
				suffixOptions.push_back("of the Unkillable");
				suffixOptions.push_back("of Persistence");
				break;
			case EARNED_TRAIT_TERRITORIAL:
				suffixOptions.push_back("of Dominion");
				suffixOptions.push_back("of the Warden");
				suffixOptions.push_back("of Claimed Ground");
				break;
			case EARNED_TRAIT_BLIGHT:
				suffixOptions.push_back("of Blight");
				suffixOptions.push_back("of Pestilence");
				suffixOptions.push_back("of the Plague");
				break;
			case EARNED_TRAIT_SPELLPROOF:
				suffixOptions.push_back("of Spell-Warding");
				suffixOptions.push_back("of the Unyielding Mind");
				suffixOptions.push_back("of Nullification");
				break;
			case EARNED_TRAIT_SCARRED:
				suffixOptions.push_back("of Scarring");
				suffixOptions.push_back("of Wounding");
				suffixOptions.push_back("of the Open Wound");
				break;
			case EARNED_TRAIT_ENRAGED:
				suffixOptions.push_back("of Rage");
				suffixOptions.push_back("of the Frenzy");
				suffixOptions.push_back("of Wrath");
				break;
			case EARNED_TRAIT_DAYBORN:
				suffixOptions.push_back("of the Sunborn");
				suffixOptions.push_back("of High Noon");
				suffixOptions.push_back("of the Blazing Hour");
				break;
			case EARNED_TRAIT_NIGHTBORN:
				suffixOptions.push_back("of the Nightborn");
				suffixOptions.push_back("of Dusk");
				suffixOptions.push_back("of the Witching Hour");
				break;
			case EARNED_TRAIT_NOMAD:
				suffixOptions.push_back("of Wandering");
				suffixOptions.push_back("of the Drifter");
				suffixOptions.push_back("of Far Roads");
				break;
			case EARNED_TRAIT_SAGE:
				suffixOptions.push_back("of the Sage");
				suffixOptions.push_back("of Wisdom");
				suffixOptions.push_back("of the Learned");
				break;
			case EARNED_TRAIT_STUDIOUS:
				suffixOptions.push_back("of Craft");
				suffixOptions.push_back("of the Artisan");
				suffixOptions.push_back("of Apprenticeship");
				break;
			default:
				suffixOptions.push_back("of the Fallen");
				suffixOptions.push_back("of Shadows");
				suffixOptions.push_back("of the Damned");
				break;
		}

		uint32 suffixIndex = urand(0, static_cast<uint32>(suffixOptions.size() - 1));
		return suffixOptions[suffixIndex];
	}

	// Get a random prefix for an affixed item name based on the origin trait.
	// Uses the same word pool as GetOriginTraitPrefix so creature and item names share vocabulary.
	inline const char* GetAffixPrefixName(uint32 originTrait) { return GetOriginTraitPrefix(originTrait); }

	// Get a random suffix for an affixed item name based on an earned trait.
	// Uses the same word pool as GetEarnedTraitSuffix so creature and item names share vocabulary.
	inline const char* GetAffixSuffixName(uint32 earnedTrait) { return GetEarnedTraitSuffix(earnedTrait); }

	// Get the display name of a single origin trait bit.
	inline const char* GetOriginTraitName(uint32 trait)
	{
		switch (trait)
		{
			case ORIGIN_TRAIT_AMBUSHER:
				return "Ambusher";
			case ORIGIN_TRAIT_EXECUTIONER:
				return "Executioner";
			case ORIGIN_TRAIT_MAGE_BANE:
				return "Mage Bane";
			case ORIGIN_TRAIT_HEALER_BANE:
				return "Healer Bane";
			case ORIGIN_TRAIT_GIANT_SLAYER:
				return "Giant Slayer";
			case ORIGIN_TRAIT_OPPORTUNIST:
				return "Opportunist";
			case ORIGIN_TRAIT_UMBRALFORGED:
				return "Umbralforged";
			case ORIGIN_TRAIT_DEATHBLOW:
				return "Deathblow";
			case ORIGIN_TRAIT_UNDERDOG:
				return "Underdog";
			case ORIGIN_TRAIT_SCAVENGER:
				return "Scavenger";
			case ORIGIN_TRAIT_DUELIST:
				return "Duelist";
			case ORIGIN_TRAIT_PLUNDERER:
				return "Plunderer";
			case ORIGIN_TRAIT_IRONBREAKER:
				return "Ironbreaker";
			case ORIGIN_TRAIT_SKINNER:
				return "Skinner";
			case ORIGIN_TRAIT_ORE_GORGED:
				return "Ore-Gorged";
			case ORIGIN_TRAIT_ROOT_RIPPER:
				return "Root-Ripper";
			case ORIGIN_TRAIT_FORGE_BREAKER:
				return "Forge-Breaker";
			case ORIGIN_TRAIT_HIDE_MANGLER:
				return "Hide-Mangler";
			case ORIGIN_TRAIT_THREAD_RIPPER:
				return "Thread-Ripper";
			case ORIGIN_TRAIT_GEAR_GRINDER:
				return "Gear-Grinder";
			case ORIGIN_TRAIT_VIAL_SHATTER:
				return "Vial-Shatter";
			case ORIGIN_TRAIT_RUNE_EATER:
				return "Rune-Eater";
			case ORIGIN_TRAIT_GEM_CRUSHER:
				return "Gem-Crusher";
			case ORIGIN_TRAIT_INK_DRINKER:
				return "Ink-Drinker";
			default:
				return "None";
		}
	}

	// Get the display name of a single earned trait bit.
	inline const char* GetEarnedTraitName(uint32 trait)
	{
		switch (trait)
		{
			case EARNED_TRAIT_COWARD:
				return "Coward";
			case EARNED_TRAIT_NOTORIOUS:
				return "Notorious";
			case EARNED_TRAIT_SURVIVOR:
				return "Survivor";
			case EARNED_TRAIT_TERRITORIAL:
				return "Territorial";
			case EARNED_TRAIT_BLIGHT:
				return "Blight";
			case EARNED_TRAIT_SPELLPROOF:
				return "Spellproof";
			case EARNED_TRAIT_SCARRED:
				return "Scarred";
			case EARNED_TRAIT_ENRAGED:
				return "Enraged";
			case EARNED_TRAIT_DAYBORN:
				return "Dayborn";
			case EARNED_TRAIT_NIGHTBORN:
				return "Nightborn";
			case EARNED_TRAIT_NOMAD:
				return "Nomad";
			case EARNED_TRAIT_SAGE:
				return "Sage";
			case EARNED_TRAIT_STUDIOUS:
				return "Studious";
			default:
				return "None";
		}
	}

	// Extract all origin traits from a combined bitmask as name strings.
	inline std::vector<const char*> GetAllOriginTraits(uint32 originTraits)
	{
		std::vector<const char*> traits;

		if (originTraits & ORIGIN_TRAIT_AMBUSHER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_AMBUSHER));
		if (originTraits & ORIGIN_TRAIT_EXECUTIONER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_EXECUTIONER));
		if (originTraits & ORIGIN_TRAIT_MAGE_BANE)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_MAGE_BANE));
		if (originTraits & ORIGIN_TRAIT_HEALER_BANE)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_HEALER_BANE));
		if (originTraits & ORIGIN_TRAIT_GIANT_SLAYER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_GIANT_SLAYER));
		if (originTraits & ORIGIN_TRAIT_OPPORTUNIST)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_OPPORTUNIST));
		if (originTraits & ORIGIN_TRAIT_UMBRALFORGED)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_UMBRALFORGED));
		if (originTraits & ORIGIN_TRAIT_DEATHBLOW)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_DEATHBLOW));
		if (originTraits & ORIGIN_TRAIT_UNDERDOG)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_UNDERDOG));
		if (originTraits & ORIGIN_TRAIT_SCAVENGER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_SCAVENGER));
		if (originTraits & ORIGIN_TRAIT_DUELIST)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_DUELIST));
		if (originTraits & ORIGIN_TRAIT_PLUNDERER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_PLUNDERER));
		if (originTraits & ORIGIN_TRAIT_IRONBREAKER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_IRONBREAKER));
		if (originTraits & ORIGIN_TRAIT_SKINNER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_SKINNER));
		if (originTraits & ORIGIN_TRAIT_ORE_GORGED)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_ORE_GORGED));
		if (originTraits & ORIGIN_TRAIT_ROOT_RIPPER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_ROOT_RIPPER));
		if (originTraits & ORIGIN_TRAIT_FORGE_BREAKER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_FORGE_BREAKER));
		if (originTraits & ORIGIN_TRAIT_HIDE_MANGLER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_HIDE_MANGLER));
		if (originTraits & ORIGIN_TRAIT_THREAD_RIPPER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_THREAD_RIPPER));
		if (originTraits & ORIGIN_TRAIT_GEAR_GRINDER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_GEAR_GRINDER));
		if (originTraits & ORIGIN_TRAIT_VIAL_SHATTER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_VIAL_SHATTER));
		if (originTraits & ORIGIN_TRAIT_RUNE_EATER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_RUNE_EATER));
		if (originTraits & ORIGIN_TRAIT_GEM_CRUSHER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_GEM_CRUSHER));
		if (originTraits & ORIGIN_TRAIT_INK_DRINKER)
			traits.push_back(GetOriginTraitName(ORIGIN_TRAIT_INK_DRINKER));

		return traits;
	}

	// Extract all earned traits from a combined bitmask as name strings.
	inline std::vector<const char*> GetAllEarnedTraits(uint32 earnedTraits)
	{
		std::vector<const char*> traits;

		if (earnedTraits & EARNED_TRAIT_COWARD)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_COWARD));
		if (earnedTraits & EARNED_TRAIT_NOTORIOUS)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_NOTORIOUS));
		if (earnedTraits & EARNED_TRAIT_SURVIVOR)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_SURVIVOR));
		if (earnedTraits & EARNED_TRAIT_TERRITORIAL)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_TERRITORIAL));
		if (earnedTraits & EARNED_TRAIT_BLIGHT)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_BLIGHT));
		if (earnedTraits & EARNED_TRAIT_SPELLPROOF)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_SPELLPROOF));
		if (earnedTraits & EARNED_TRAIT_SCARRED)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_SCARRED));
		if (earnedTraits & EARNED_TRAIT_ENRAGED)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_ENRAGED));
		if (earnedTraits & EARNED_TRAIT_DAYBORN)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_DAYBORN));
		if (earnedTraits & EARNED_TRAIT_NIGHTBORN)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_NIGHTBORN));
		if (earnedTraits & EARNED_TRAIT_NOMAD)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_NOMAD));
		if (earnedTraits & EARNED_TRAIT_SAGE)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_SAGE));
		if (earnedTraits & EARNED_TRAIT_STUDIOUS)
			traits.push_back(GetEarnedTraitName(EARNED_TRAIT_STUDIOUS));

		return traits;
	}

	// Resolve an earned trait enum value from a case-insensitive name string.
	// Returns EARNED_TRAIT_NONE if the name is unrecognized.
	inline NemesisEliteEarnedTrait GetEarnedTraitByName(const std::string& name)
	{
		std::string lower = name;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

		if (lower == "coward")
			return EARNED_TRAIT_COWARD;
		if (lower == "notorious")
			return EARNED_TRAIT_NOTORIOUS;
		if (lower == "survivor")
			return EARNED_TRAIT_SURVIVOR;
		if (lower == "territorial")
			return EARNED_TRAIT_TERRITORIAL;
		if (lower == "blight")
			return EARNED_TRAIT_BLIGHT;
		if (lower == "spellproof")
			return EARNED_TRAIT_SPELLPROOF;
		if (lower == "scarred")
			return EARNED_TRAIT_SCARRED;
		if (lower == "enraged")
			return EARNED_TRAIT_ENRAGED;
		if (lower == "dayborn")
			return EARNED_TRAIT_DAYBORN;
		if (lower == "nightborn")
			return EARNED_TRAIT_NIGHTBORN;
		if (lower == "nomad")
			return EARNED_TRAIT_NOMAD;
		if (lower == "sage")
			return EARNED_TRAIT_SAGE;
		if (lower == "studious")
			return EARNED_TRAIT_STUDIOUS;

		return EARNED_TRAIT_NONE;
	}

	// Resolve an origin trait enum value from a case-insensitive name string.
	// Returns ORIGIN_TRAIT_NONE if the name is unrecognized.
	inline NemesisEliteOriginTrait GetOriginTraitByName(const std::string& name)
	{
		std::string lower = name;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

		if (lower == "ambusher")
			return ORIGIN_TRAIT_AMBUSHER;
		if (lower == "executioner")
			return ORIGIN_TRAIT_EXECUTIONER;
		if (lower == "magebane")
			return ORIGIN_TRAIT_MAGE_BANE;
		if (lower == "healerbane")
			return ORIGIN_TRAIT_HEALER_BANE;
		if (lower == "giantslayer")
			return ORIGIN_TRAIT_GIANT_SLAYER;
		if (lower == "opportunist")
			return ORIGIN_TRAIT_OPPORTUNIST;
		if (lower == "umbralforged")
			return ORIGIN_TRAIT_UMBRALFORGED;
		if (lower == "deathblow")
			return ORIGIN_TRAIT_DEATHBLOW;
		if (lower == "underdog")
			return ORIGIN_TRAIT_UNDERDOG;
		if (lower == "scavenger")
			return ORIGIN_TRAIT_SCAVENGER;
		if (lower == "duelist")
			return ORIGIN_TRAIT_DUELIST;
		if (lower == "plunderer")
			return ORIGIN_TRAIT_PLUNDERER;
		if (lower == "ironbreaker")
			return ORIGIN_TRAIT_IRONBREAKER;
		if (lower == "skinner")
			return ORIGIN_TRAIT_SKINNER;
		if (lower == "oregorged")
			return ORIGIN_TRAIT_ORE_GORGED;
		if (lower == "rootripper")
			return ORIGIN_TRAIT_ROOT_RIPPER;
		if (lower == "forgebreaker")
			return ORIGIN_TRAIT_FORGE_BREAKER;
		if (lower == "hidemangler")
			return ORIGIN_TRAIT_HIDE_MANGLER;
		if (lower == "threadripper")
			return ORIGIN_TRAIT_THREAD_RIPPER;
		if (lower == "geargrinder")
			return ORIGIN_TRAIT_GEAR_GRINDER;
		if (lower == "vialshatter")
			return ORIGIN_TRAIT_VIAL_SHATTER;
		if (lower == "runeeater")
			return ORIGIN_TRAIT_RUNE_EATER;
		if (lower == "gemcrusher")
			return ORIGIN_TRAIT_GEM_CRUSHER;
		if (lower == "inkdrinker")
			return ORIGIN_TRAIT_INK_DRINKER;

		return ORIGIN_TRAIT_NONE;
	}
}

#endif // NEMESIS_ELITE_HELPERS_H
