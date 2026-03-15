#ifndef NEMESIS_ELITE_CONSTANTS_H
#define NEMESIS_ELITE_CONSTANTS_H

#include "Random.h"

// Origin traits are determined at the moment of promotion and never change.
enum NemesisEliteOriginTrait : uint32
{
    ORIGIN_TRAIT_NONE = 0,
    ORIGIN_TRAIT_AMBUSHER = 1 << 0,     // Killed player already in combat with other mobs
    ORIGIN_TRAIT_EXECUTIONER = 1 << 1,  // Killed player below 20% HP
    ORIGIN_TRAIT_MAGE_BANE = 1 << 2,    // Killed a pure caster class (Mage, Warlock)
    ORIGIN_TRAIT_HEALER_BANE = 1 << 3,  // Killed a healer class (Priest, Paladin, Druid, Shaman)
    ORIGIN_TRAIT_GIANT_SLAYER = 1 << 4, // Killed a player 10+ levels above it
    ORIGIN_TRAIT_OPPORTUNIST = 1 << 5,  // Killed an AFK player
    ORIGIN_TRAIT_UMBRALFORGED = 1 << 6, // Promoted during an Umbral Moon
    // Bit 7 (1 << 7) intentionally left unused — formerly ORIGIN_TRAIT_EMPOWERED, removed from design.
    ORIGIN_TRAIT_DEATHBLOW = 1 << 8,    // Killed a player in a single hit
    ORIGIN_TRAIT_UNDERDOG = 1 << 9,     // Killed a player 3+ levels above it but not high enough to earn Giant Slayer
    ORIGIN_TRAIT_SCAVENGER = 1 << 10,   // Killed a player who has resurrection sickness debuff
    ORIGIN_TRAIT_DUELIST = 1 << 11,     // Killed a player solo as a humanoid creature
    ORIGIN_TRAIT_PLUNDERER = 1 << 12,   // Killed a player carrying significant gold
    ORIGIN_TRAIT_IRONBREAKER = 1 << 13, // Killed a plate-wearing class (Warrior, Paladin, Death Knight)
    ORIGIN_TRAIT_SKINNER = 1 << 14,     // Killed a player with the Skinning profession
    ORIGIN_TRAIT_ORE_GORGED = 1 << 15,  // Killed a player with the Mining profession
    ORIGIN_TRAIT_ROOT_RIPPER = 1 << 16, // Killed a player with the Herbalism profession
    ORIGIN_TRAIT_FORGE_BREAKER = 1 << 17, // Killed a player with the Blacksmithing profession
    ORIGIN_TRAIT_HIDE_MANGLER = 1 << 18, // Killed a player with the Leatherworking profession
    ORIGIN_TRAIT_THREAD_RIPPER = 1 << 19, // Killed a player with the Tailoring profession
    ORIGIN_TRAIT_GEAR_GRINDER = 1 << 20, // Killed a player with the Engineering profession
    ORIGIN_TRAIT_VIAL_SHATTER = 1 << 21, // Killed a player with the Alchemy profession
    ORIGIN_TRAIT_RUNE_EATER = 1 << 22,  // Killed a player with the Enchanting profession
    ORIGIN_TRAIT_GEM_CRUSHER = 1 << 23, // Killed a player with the Jewelcrafting profession
    ORIGIN_TRAIT_INK_DRINKER = 1 << 24, // Killed a player with the Inscription profession
};

// Earned traits accumulate over the nemesis's lifetime.
enum NemesisEliteEarnedTrait : uint32
{
    EARNED_TRAIT_NONE = 0,
    EARNED_TRAIT_COWARD = 1 << 0,      // Fled from a losing fight
    EARNED_TRAIT_NOTORIOUS = 1 << 1,   // Killed 5+ unique players
    EARNED_TRAIT_SURVIVOR = 1 << 2,    // Survived 3+ player attempts
    EARNED_TRAIT_TERRITORIAL = 1 << 3, // Alive 3+ days
    EARNED_TRAIT_BLIGHT      = 1 << 4, // Killed players of both factions
    EARNED_TRAIT_SPELLPROOF  = 1 << 5, // Survived combat while CC'd
    EARNED_TRAIT_SCARRED     = 1 << 6, // Survived combat while a player healed against it
    EARNED_TRAIT_ENRAGED     = 1 << 7, // Survived combat against multiple distinct players
    EARNED_TRAIT_DAYBORN     = 1 << 8, // Survived combat during daytime (hours 6-17)
    EARNED_TRAIT_NIGHTBORN   = 1 << 9, // Survived combat during nighttime (hours 18-5)
    EARNED_TRAIT_NOMAD       = 1 << 10, // Explored multiple distinct areas
    EARNED_TRAIT_SAGE        = 1 << 11, // Alive 7+ days
    EARNED_TRAIT_STUDIOUS    = 1 << 12, // Observed near a treasure chest
};

// Stat injection data for a single affix stat bonus.
struct AffixStatEntry
{
    uint32 statType;  // ITEM_MOD_* constant, or AFFIX_STAT_ARMOR sentinel
    int32 statValue;  // The value to inject into the stat slot
};

// Special sentinel: affix modifies the armor column instead of a stat slot.
constexpr uint32 AFFIX_STAT_ARMOR = 9999;

namespace NemesisEliteConstants
{
    // ----------------------------------------------------------------
    // Constant values
    // ----------------------------------------------------------------
    // Faction 16 (Monster) — hostile to all players (Alliance + Horde) but NOT to other creatures.
    // Applied to the cloned creature_template when a nemesis earns EARNED_TRAIT_TERRITORIAL.
    constexpr uint32 ELITE_HOSTILE_FACTION = 16u;
    
    // Custom creature_template entry range for nemesis elites.
    // The entry for a given elite is ELITE_CUSTOM_ENTRY_BASE + elite_id.
    // Cloned rows are inserted on promotion and deleted when the nemesis dies.
    constexpr uint32 ELITE_CUSTOM_ENTRY_BASE = 9000000u;
    
    // Custom item_template entry range for affixed loot items.
    // The entry for a given elite's drop is AFFIX_ITEM_ENTRY_BASE + elite_id.
    // item_template and creature_template are separate tables, so the same
    // numeric entry can exist in both without conflict.
    constexpr uint32 AFFIX_ITEM_ENTRY_BASE = 9000000u;
    
    // Quality tier enum for affixed loot items.
    // These map to the quality_tier column in nemesis_item_affixes.
    constexpr uint8 AFFIX_TIER_GREEN = 0;
    constexpr uint8 AFFIX_TIER_BLUE = 1;
    constexpr uint8 AFFIX_TIER_PURPLE = 2;
    constexpr uint8 AFFIX_TIER_LEGENDARY = 3;
    
    // WoW item quality values used in item_template.Quality
    constexpr uint8 ITEM_QUALITY_UNCOMMON = 2;  // Green
    constexpr uint8 ITEM_QUALITY_RARE = 3;      // Blue
    constexpr uint8 ITEM_QUALITY_EPIC = 4;      // Purple
    constexpr uint8 ITEM_QUALITY_LEGENDARY = 5; // Orange
    
    // Spell cast on a creature when we grow the elite based on effective level
    constexpr uint32 ELITE_GROW_SPELL = 74996u; // Grow rank 1

    // Spell cast by a Coward nemesis at the end of its flee to heal itself.
    constexpr uint32 ELITE_SPELL_COWARD_HEAL = 100001u;

    // Spell cast by a Coward nemesis when it flees to hide itself.
    constexpr uint32 ELITE_SPELL_COWARD_STEALTH = 100002u;

    // Aura applied to an Executioner nemesis while its current victim is at or below
    // the configured HP threshold. Carries SPELL_AURA_MOD_DAMAGE_PERCENT_DONE in the
    // DBC so the bonus shows in combat log and floating text. Conditionally managed
    // in OnCreatureUpdate; stripped on combat exit.
    constexpr uint32 ELITE_AURA_EXECUTIONER = 100003u;

    // Spell cast by a Mage Bane nemesis on a player caught casting within range.
    // DBC entry should carry SPELL_AURA_MOD_SILENCE with a short duration (e.g. 3 sec).
    // Fired periodically during combat at a configurable interval.
    constexpr uint32 ELITE_SPELL_MAGE_BANE_SILENCE = 100004u;

    // Debuff applied by a Healer Bane nemesis to its current melee victim.
    // DBC entry should carry SPELL_AURA_MOD_HEALING_PCT with a negative value
    // (e.g. -50%) and a moderate duration (e.g. 8 sec). Applied on a configurable
    // cooldown so players have a window to dispel and benefit from full healing
    // before the next application.
    constexpr uint32 ELITE_AURA_HEALER_BANE = 100005u;

    // Aura applied to a Giant Slayer nemesis while its current victim is a player
    // whose level exceeds the nemesis's level by a configurable gap (default 5).
    // DBC entry effects:
    //   Effect 1: SPELL_AURA_MOD_DAMAGE_PERCENT_DONE (+40%)
    //   Effect 2: SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN (-20%)
    //   Effect 3: SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN (-20%, ranged school mask)
    // Tooltip: "Emboldened by the memory of felling a giant. Attack damage
    // increased by $s1%. Damage taken is reduced by $s2% and range damage
    // taken by $s3%."
    constexpr uint32 ELITE_AURA_GIANT_SLAYER = 100006u;

    // Spell cast by an Opportunist nemesis on its current melee victim on a
    // configurable cooldown. DBC entry should carry SPELL_AURA_MOD_STUN with
    // a short duration (e.g. 2 sec) and MECHANIC_STUN so it can be broken by
    // trinkets and stun-removal abilities.
    constexpr uint32 ELITE_SPELL_OPPORTUNIST_CHEAP_SHOT = 100007u;

    // Aura applied to an Umbralforged nemesis when it enters combat during an
    // active Umbral Moon. DBC entry should carry two effects on the caster:
    //   Effect 1: SPELL_AURA_MOD_STAT (Stamina, MiscValue = 2)
    //   Effect 2: SPELL_AURA_PERIODIC_TRIGGER_SPELL → triggers spell 100009
    // Duration: infinite (server manages application/removal).
    // The triggered spell (100009, "Umbral Burst") should be a separate DBC
    // entry with:
    //   Effect 1: SPELL_EFFECT_SCHOOL_DAMAGE, shadow school mask (32),
    //             BasePoints = 1 (overridden at runtime via SpellScript)
    //             Target A = TARGET_UNIT_SRC_AREA_ENEMY with an appropriate radius
    //             (e.g. 8-10 yards). This pulses shadow damage to all nearby players
    //             at the amplitude interval defined on Effect 2 of spell 100008.
    // Must be applied via CastSpell (not AddAura) so the periodic trigger
    // timer is properly initialized through the full spell pipeline.
    constexpr uint32 ELITE_AURA_UMBRAL_BURST = 100008u;
    constexpr uint32 ELITE_SPELL_UMBRAL_BURST = 100009u;

    // Spell cast by a Deathblow nemesis when its HP drops below a configurable
    // threshold (default 25%). This is a desperate, telegraphed heavy strike
    // with a 3-second cast time that can be interrupted by players.
    //
    // DBC entry requirements:
    //   Cast time: 3000 ms (interruptible, use CastingTimeIndex for 3 sec)
    //   Effect 1: SPELL_EFFECT_SCHOOL_DAMAGE, physical school mask (1)
    //             BasePoints = 1 (overridden at runtime via SpellScript::SetHitDamage)
    //             Target A = TARGET_UNIT_TARGET_ENEMY
    //   InterruptFlags: standard (movement, pushback, interrupt)
    //
    // The actual damage is computed server-side as:
    //   base + (effectiveLevel × perLevel)
    // Both values are configurable. The DBC BasePoints value is irrelevant;
    // the SpellScript replaces it before the damage pipeline runs, so combat
    // log, floating numbers, absorbs, and resists all work naturally.
    //
    // State machine (per nemesis per threshold crossing):
    //   IDLE → HP drops below threshold → cast initiated → CASTING
    //   CASTING → cast completes or is interrupted → SPENT
    //   SPENT → HP healed above threshold → IDLE (with re-arm cooldown)
    // Only one cast attempt per threshold crossing. Interrupt = real counterplay.
    constexpr uint32 ELITE_SPELL_DEATHBLOW_STRIKE = 100010u;

    // Passive aura applied to an Underdog nemesis at all times (permanent self-buff).
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_EXPERTISE (240)
    //             BasePoints = 9 (+ 1 Die Side = 10 flat expertise)
    //             MiscValue A = 0 (no weapon-type restriction)
    //             Target A = TARGET_UNIT_CASTER
    //   Duration: ~25 days (effectively permanent; RefreshAuras re-applies if stripped)
    //   Flags: Passive
    //   School: Physical
    //
    // 10 expertise = 2.5% reduced dodge/parry against this nemesis. Reflects the
    // creature's experience fighting opponents above its weight class — it learned
    // to land cleaner strikes that slip past defenses.
    constexpr uint32 ELITE_AURA_UNDERDOG = 100011u;

    // Conditional aura applied to a Scavenger nemesis while its current victim
    // has at least one debuff. Toggled every tick in OnCreatureUpdate, stripped
    // on combat exit — same pattern as Executioner and Giant Slayer.
    //
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_DAMAGE_PERCENT_DONE (79)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount)
    //             MiscValue A = 127 (all school mask — physical + all magic schools)
    //             Target A = TARGET_UNIT_CASTER
    //   Duration: 0 (permanent while applied; server manages application/removal)
    //   School: Physical
    //
    // The actual percentage is computed server-side as:
    //   base + (effectiveLevel × perLevel)
    // Both values are configurable. The DBC BasePoints value is irrelevant;
    // ChangeAmount replaces it after AddAura so the aura carries the correct
    // level-scaled percentage.
    constexpr uint32 ELITE_AURA_SCAVENGER = 100012u;

    // Conditional aura applied to a Duelist nemesis while exactly one player is
    // in its attacker list. Toggled every tick in OnCreatureUpdate, stripped on
    // combat exit — same pattern as Executioner, Giant Slayer, and Scavenger.
    //
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_DAMAGE_PERCENT_DONE (79)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount)
    //             MiscValue A = 127 (all school mask — physical + all magic schools)
    //             Target A = TARGET_UNIT_CASTER
    //   Duration: ~25 days or equivalent long duration (server manages removal)
    //   School: Physical
    //
    // The actual percentage is computed server-side as:
    //   base + (effectiveLevel × perLevel)
    // Both values are configurable. ChangeAmount replaces the DBC BasePoints
    // after AddAura so the aura carries the correct level-scaled percentage.
    // The buff drops the instant a second player enters combat, making grouping
    // a natural counter.
    constexpr uint32 ELITE_AURA_DUELIST = 100013u;

    // Passive aura applied to an Ironbreaker nemesis at all times (permanent self-buff).
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_ARMOR_PENETRATION_PCT (346)
    //             BasePoints = 9 (+ 1 Die Side = 10% armor penetration)
    //             MiscValue A = 0
    //             Target A = TARGET_UNIT_CASTER
    //   Duration: ~25 days (effectively permanent; RefreshAuras re-applies if stripped)
    //   Flags: Passive
    //   School: Physical
    //
    // 10% armor penetration means this nemesis ignores 10% of its victim's armor.
    // Reflects the creature's learned ability to exploit gaps in plate armor —
    // it strikes where the joints are weakest.
    constexpr uint32 ELITE_AURA_IRONBREAKER = 100014u;

    // Permanent scaled passive aura applied to a Notorious nemesis (killed 5+
    // unique players). Boosts damage output and maximum health — the two stats
    // that actually affect creature combat in 3.3.5 (creatures do not use the
    // player attribute system, so SPELL_AURA_MOD_STAT has no effect).
    //
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_DAMAGE_PERCENT_DONE (79)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount)
    //             MiscValue A = 127 (all school mask — physical + all magic schools)
    //             Target A = TARGET_UNIT_CASTER
    //   Effect 2: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_INCREASE_HEALTH (176)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount)
    //             MiscValue A = 0
    //             Target A = TARGET_UNIT_CASTER
    //   Duration: ~25 days (permanent; RefreshAuras re-applies if stripped)
    //   Flags: Passive
    //   School: Physical
    //   Name: "Dread Renown"
    //   Tooltip: "This creature's infamy precedes it. Damage and vitality are bolstered."
    //
    // Both effect values are computed server-side as:
    //   base + (effectiveLevel × perLevel)
    // ChangeAmount replaces DBC BasePoints on each effect independently.
    // The "bonus to all stats" flavor is deferred to the Affixed Loot system
    // (System 3), where the Notorious suffix grants flat stat bonuses on gear.
    constexpr uint32 ELITE_AURA_NOTORIOUS = 100016u;

    // Permanent scaled passive aura applied to a Survivor nemesis (survived 3+
    // player combat encounters). Grants bonus armor and reduces incoming critical
    // strike damage.
    //
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_RESISTANCE (22)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount)
    //             MiscValue A = 1 (armor / physical school)
    //             Target A = TARGET_UNIT_CASTER
    //   Effect 2: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE (189)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount, negative)
    //             MiscValue A = 0
    //             Target A = TARGET_UNIT_CASTER
    //   Duration: ~25 days (permanent; RefreshAuras re-applies if stripped)
    //   Flags: Passive
    //   School: Physical
    //   Name: "Battle-Hardened"
    //   Tooltip: "Tempered by countless failed assaults. Armor is reinforced and critical strikes deal less damage."
    //
    // Both effect values are computed server-side as:
    //   base + (effectiveLevel × perLevel)
    // ChangeAmount replaces DBC BasePoints on each effect independently.
    // Effect 2 is set negative at runtime (e.g. -17) to reduce crit bonus damage.
    constexpr uint32 ELITE_AURA_SURVIVOR = 100015u;

    // Permanent scaled passive aura applied to a Blight nemesis (killed both
    // Horde and Alliance players). Deals shadow damage to any melee attacker
    // via a damage shield (thorns-style).
    //
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_DAMAGE_SHIELD (201)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount)
    //             MiscValue A = 0
    //             Target A = TARGET_UNIT_CASTER
    //   Duration: ~25 days (permanent; RefreshAuras re-applies if stripped)
    //   Flags: Passive
    //   School: Shadow (SchoolMask = 32)
    //   Name: "Blighted Aura"
    //   Tooltip: "Suffused with the death of both factions. Deals Shadow damage to any attacker."
    //
    // The damage value is computed server-side as:
    //   base + (effectiveLevel × perLevel)
    // ChangeAmount replaces the DBC BasePoints after AddAura.
    constexpr uint32 ELITE_AURA_BLIGHT = 100017u;

    // Permanent passive aura applied to a Spellproof nemesis (survived combat
    // while CC'd by a player). Reduces the duration of Stun, Polymorph, and Fear
    // mechanics.
    //
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MECHANIC_DURATION_MOD (147)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount, negative)
    //             MiscValue A = 12 (MECHANIC_STUN)
    //             Target A = TARGET_UNIT_CASTER
    //   Effect 2: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MECHANIC_DURATION_MOD (147)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount, negative)
    //             MiscValue A = 17 (MECHANIC_POLYMORPH)
    //             Target A = TARGET_UNIT_CASTER
    //   Effect 3: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MECHANIC_DURATION_MOD (147)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount, negative)
    //             MiscValue A = 5 (MECHANIC_FEAR)
    //             Target A = TARGET_UNIT_CASTER
    //   Duration: ~25 days (permanent; RefreshAuras re-applies if stripped)
    //   Flags: Passive
    //   School: Physical
    //   Name: "Spellwarded"
    //   Tooltip: "Hardened against magical manipulation. Crowd control effects wear off faster."
    //
    // The actual reduction percentage is computed server-side from config.
    // ChangeAmount replaces the DBC BasePoints on all three effects identically.
    constexpr uint32 ELITE_AURA_SPELLPROOF = 100018u;

    // Debuff applied by a Scarred nemesis to its current melee victim on a
    // configurable cooldown. Reduces healing received.
    //
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_HEALING_PCT (118)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount, negative)
    //             MiscValue A = 0
    //             Target A = TARGET_UNIT_TARGET_ENEMY
    //   Duration: 8 seconds
    //   School: Physical
    //   Name: "Scarred Presence"
    //   Tooltip: "This creature learned to punish those who rely on healing. Healing received is reduced."
    constexpr uint32 ELITE_AURA_SCARRED = 100019u;

    // Conditional aura applied to an Enraged nemesis while 2+ players are on its
    // threat list. Damage bonus scales with both effective level and the number
    // of players currently in combat. Toggled per-tick, stripped on combat exit.
    //
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_DAMAGE_PERCENT_DONE (79)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount)
    //             MiscValue A = 127 (all school mask)
    //             Target A = TARGET_UNIT_CASTER
    //   Duration: 0 (permanent while applied; server manages application/removal)
    //   School: Physical
    //   Name: "Enraged Fury"
    //   Tooltip: "Driven to frenzy by the swarm of attackers. Damage increases with each combatant."
    constexpr uint32 ELITE_AURA_ENRAGED = 100020u;

    // Conditional aura applied to a Dayborn nemesis while the current server-local
    // hour falls within the configured daytime window (default 6:00-17:59). Grants
    // a level-scaled damage bonus. Toggled per-tick during combat, stripped on exit.
    //
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_DAMAGE_PERCENT_DONE (79)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount)
    //             MiscValue A = 127 (all school mask)
    //             Target A = TARGET_UNIT_CASTER
    //   Duration: 0 (permanent while applied; server manages application/removal)
    //   School: Holy
    //   Name: "Sun-Tempered"
    //   Tooltip: "Born under the light. Empowered during daytime hours."
    constexpr uint32 ELITE_AURA_DAYBORN = 100021u;

    // Conditional aura applied to a Nightborn nemesis while the current server-local
    // hour falls within nighttime (outside the configured daytime window, default
    // 18:00-5:59). Mirror of Sun-Tempered. Toggled per-tick during combat.
    //
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_DAMAGE_PERCENT_DONE (79)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount)
    //             MiscValue A = 127 (all school mask)
    //             Target A = TARGET_UNIT_CASTER
    //   Duration: 0 (permanent while applied; server manages application/removal)
    //   School: Shadow
    //   Name: "Moon-Tempered"
    //   Tooltip: "Forged in darkness. Empowered during nighttime hours."
    constexpr uint32 ELITE_AURA_NIGHTBORN = 100022u;

    // Permanent passive aura applied to a Nomad nemesis (visited 3+ distinct areas).
    // Grants increased movement speed.
    //
    // DBC entry requirements:
    //   Effect 1: SPELL_EFFECT_APPLY_AURA
    //             Apply Aura Name: SPELL_AURA_MOD_INCREASE_SPEED (31)
    //             BasePoints = 0 (overridden at runtime via ChangeAmount)
    //             MiscValue A = 0
    //             Target A = TARGET_UNIT_CASTER
    //   Duration: ~25 days (permanent; RefreshAuras re-applies if stripped)
    //   Flags: Passive
    //   School: Physical
    //   Name: "Nomad's Stride"
    //   Tooltip: "This creature has wandered far. Movement speed increased."
    constexpr uint32 ELITE_AURA_NOMAD = 100023u;

    // ----------------------------------------------------------------
    // Affix runtime hook spell constants
    // ----------------------------------------------------------------

    // Debuff applied by the Mage-Bane's prefix affix when it procs on a player's melee/spell hit.
    // DBC entry: SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, duration 6s (shortened via SetDuration),
    // BasePoints overridden via CastCustomSpell value parameter.
    constexpr uint32 ELITE_AFFIX_SPELL_MAGE_BANE_SLOW = 100024u;

    // Debuff applied by the Opportunist's prefix affix when it procs on a player's hit.
    // DBC entry: SPELL_AURA_MOD_DECREASE_SPEED (-50%), MECHANIC_DAZE, duration 5s (shortened via SetDuration).
    constexpr uint32 ELITE_AFFIX_SPELL_OPPORTUNIST_DAZE = 100025u;

    // ----------------------------------------------------------------
    // Affix aura spell constants (applied on equip, removed on unequip)
    // ----------------------------------------------------------------

    // Deathblow prefix: increases critical strike bonus damage.
    // DBC: SPELL_AURA_MOD_CRIT_DAMAGE_BONUS (138), BasePoints overridden via CastCustomSpell.
    constexpr uint32 ELITE_AFFIX_AURA_DEATHBLOW = 100026u;

    // of Wandering suffix: increases movement speed.
    // DBC: SPELL_AURA_MOD_INCREASE_SPEED (31), BasePoints overridden via CastCustomSpell.
    constexpr uint32 ELITE_AFFIX_AURA_WANDERING = 100027u;

    // of Cowardice suffix: reduces threat generation for all spell schools.
    // DBC: SPELL_AURA_MOD_THREAT (24), MiscValue = 127, BasePoints overridden (negative).
    constexpr uint32 ELITE_AFFIX_AURA_COWARDICE = 100028u;

    // of Dominion suffix: increases threat generation for all spell schools.
    // DBC: SPELL_AURA_MOD_THREAT (24), MiscValue = 127, BasePoints overridden (positive).
    constexpr uint32 ELITE_AFFIX_AURA_DOMINION = 100029u;

    // Profession prefix auras: each grants bonus skill points via SPELL_AURA_MOD_SKILL (115).
    // MiscValue in the DBC is set to the profession's skill ID. BasePoints overridden.
    constexpr uint32 ELITE_AFFIX_AURA_SKINNING = 100030u;
    constexpr uint32 ELITE_AFFIX_AURA_MINING = 100031u;
    constexpr uint32 ELITE_AFFIX_AURA_HERBALISM = 100032u;
    constexpr uint32 ELITE_AFFIX_AURA_BLACKSMITHING = 100033u;
    constexpr uint32 ELITE_AFFIX_AURA_LEATHERWORKING = 100034u;
    constexpr uint32 ELITE_AFFIX_AURA_TAILORING = 100035u;
    constexpr uint32 ELITE_AFFIX_AURA_ENGINEERING = 100036u;
    constexpr uint32 ELITE_AFFIX_AURA_ALCHEMY = 100037u;
    constexpr uint32 ELITE_AFFIX_AURA_ENCHANTING = 100038u;
    constexpr uint32 ELITE_AFFIX_AURA_JEWELCRAFTING = 100039u;
    constexpr uint32 ELITE_AFFIX_AURA_INSCRIPTION = 100040u;

    // Umbralforged prefix affix: instant AoE shadow damage centered on the player.
    // Cast every 3 seconds while the player is in combat via the WorldScript update loop.
    // DBC: SPELL_EFFECT_SCHOOL_DAMAGE, Shadow, TARGET_SRC_CASTER + TARGET_UNIT_SRC_AREA_ENEMY, 10yd.
    // BasePoints overridden via CastCustomSpell.
    constexpr uint32 ELITE_AFFIX_SPELL_UMBRAL_ECHO = 100041u;
    
    // of Blight suffix: instant shadow damage reflected back to melee attackers.
    // DBC: SPELL_EFFECT_SCHOOL_DAMAGE, Shadow, TARGET_UNIT_TARGET_ENEMY, BasePoints overridden.
    constexpr uint32 ELITE_AFFIX_SPELL_BLIGHTED_REFLECTION = 100042u;

    // Executioner's prefix affix: instant physical damage proc against low-HP targets.
    // DBC: SPELL_EFFECT_SCHOOL_DAMAGE, Physical, DmgClass Melee (scales with AP).
    // BasePoints overridden via CastCustomSpell.
    constexpr uint32 ELITE_AFFIX_SPELL_EXECUTIONERS_STRIKE = 100043u;

    // Scavenger's prefix affix: instant physical damage proc against debuffed targets.
    // Fires as a separate combat log event when a player with Scavenger-affixed gear
    // hits a target that has at least one debuff. Damage is computed as a percentage
    // of the triggering hit and passed via CastCustomSpell BasePoints override.
    // DBC: SPELL_EFFECT_SCHOOL_DAMAGE, Physical, TARGET_UNIT_TARGET_ENEMY,
    //      BasePoints = 1 (overridden at runtime).
    constexpr uint32 ELITE_AFFIX_SPELL_SCAVENGERS_STRIKE = 100044u;

    // Duelist's prefix affix: instant physical damage proc when solo-attacking a target.
    // Fires as a separate combat log event when a player with Duelist-affixed gear
    // hits a creature while being the only player on its threat list. Damage is computed
    // as a percentage of the triggering hit and passed via CastCustomSpell BasePoints override.
    // DBC: SPELL_EFFECT_SCHOOL_DAMAGE, Physical, TARGET_UNIT_TARGET_ENEMY,
    //      BasePoints = 1 (overridden at runtime).
    constexpr uint32 ELITE_AFFIX_SPELL_DUELISTS_STRIKE = 100045u;

}

#endif // NEMESIS_ELITE_CONSTANTS_H
