# Nemesis — AzerothCore Server Design Document
## Overview

Nemesis is a custom AzerothCore server built around three interlocking systems: a **Shadow of Mordor-inspired Nemesis system** where mobs that kill players get promoted into persistent, evolving named elites; a **Umbral Moon event** that periodically increases difficulty and nemesis spawn rates; and an **affixed loot system** where nemesis kills drop procedurally generated gear with prefix/suffix combinations that tell the story of the nemesis that dropped them.

A fourth mechanic — **permanent stat stacking from nemesis kills** — ties the loop together by rewarding survival and making death meaningful at every stage of the game.

The server is otherwise largely blizzlike. The core WoW experience is intact. These systems layer on top of it to create emergent, player-driven content.

---

## Core Loop

```
Umbral Moon starts
    → Mobs are harder, rewards are better, nemesis spawn chance spikes
    → Players push content and die more
    → Nemeses flood the world
Umbral Moon ends
    → World is littered with promoted mobs of varying threat levels
    → Players hunt nemeses for affixed loot
    → High-threat nemeses above the power threshold also grant permanent stat stacks (diminishing returns)
    → Stacks make players stronger → they take on nastier nemeses
    → Dying erases ALL stacks and births an even more dangerous nemesis fueled by accumulated power (of course if the % chance is met)
    → Bounty board tracks the biggest threats server-wide
    → Cycle repeats
```

---

## System 1: Nemesis Promotion

### Trigger

When a mob kills a player, there is a configurable chance the mob is promoted into a **Nemesis**. The base chance defaults to 7% under normal conditions and 25% during Umbral Moon. Both values are configurable via `nemesis_configuration` keys `elite_promotion_chance_normal` and `elite_promotion_chance_umbral_moon`.

### Scope

Nemesis promotion **only occurs in open world content**. Player deaths inside instanced content (dungeons, raids, battlegrounds, arenas) do not trigger promotion rolls. Instanced content is already tuned with its own difficulty curves, and nemesis persistence inside instances creates unresolvable problems (instance resets, despawn behavior, zone ownership). The check is a simple filter on map type via `IsWorldMap()` — if the death occurs on a non-world map, the promotion hook exits early.

Additional eligibility filters exclude creatures that are non-attackable, not selectable, critters, vehicles, taxis, guards, vendors, trainers, quest givers, bankers, innkeepers, auctioneers, player-created, or dynamic spawns (spawnId == 0).

### What Happens on Promotion

The mob is converted into a persistent named elite. It receives:

- A trait-driven name combining an origin trait prefix with the creature's original name (e.g., "Umbra-Forged Mana Wyrm", "Executioner Kobold Vermin"). Each origin trait has multiple possible prefix variants chosen at random — for example, Ambusher can produce "Lurking", "Stalking", "Conniving", or "Opportune". The prefix tells the story of how the nemesis was born.
- A cloned `creature_template` entry in the 9,000,000+ range (`ELITE_CUSTOM_ENTRY_BASE + elite_id`), with `UpdateEntry` called to push the new name/level to the client nameplate.
- Bonus stats: configurable HP and damage percentage boosts (default +25% each) applied via `ApplyStatPctModifier(TOTAL_PCT)`.
- A visual GROW aura scaled to effective level (1 stack per 10 levels) using spell 74996.
- An effective level gain on promotion (default +3 levels, configurable).
- One origin **trait** derived from the circumstances of the kill (see Trait System below). If multiple origin traits qualify, one is selected at random.
- A **threat score** (see Threat Score below).
- A persistent entry in the `nemesis_elite` database table tracking its full history.

The nemesis remains in the world as a living entity. It does not despawn on normal timers. It patrols, it fights, it grows.

**Promotion is a one-time event.** A nemesis is never re-promoted. The initial promotion establishes its identity (name, origin traits). All subsequent growth happens through continuous scaling and earned trait accumulation — a nemesis that kills more players gets stronger and gains new earned traits, but it is not "promoted again." There is no second birth event. If the killing creature is already a nemesis, the kill is routed to `OnNemesisKilledPlayer` which logs the kill and applies scaling instead.

### Trait System

Traits are assigned based on combat log analysis of the killing blow and surrounding context.

**Origin Traits (assigned at promotion, frozen permanently):**

Origin trait is determined at the moment of promotion and **never changes**. They represent the circumstances of the nemesis's birth and cannot be gained later. A nemesis born from killing a warrior will never gain Mage-Bane from subsequently killing a mage. This keeps the nemesis's identity coherent and makes origin-based prefixes on affixed loot meaningful — the prefix tells the story of how the nemesis was created, not a summary of everything it ever did.

If multiple origin traits are eligible at the time of promotion, one is randomly selected and locked in.

| Trait | Condition | Detection | Effect on Nemesis |
|-------|-----------|-----------|-------------------|
| Ambusher | Killed a player already in combat with other mobs | Checks `getAttackers()` for non-self attackers | Calls adds when engaging players |
| Executioner | Killed a player below 20% HP | Pre-damage HP projection via `UnitScript::OnDamage`, cleared via `OnHeal` | Gains bonus damage against low-HP targets |
| Mage-Bane | Killed a caster class (Mage, Warlock) | Class check on victim | Gains interrupt / spell lock ability |
| Healer-Bane | Killed a healer class (Priest, Paladin, Druid, Shaman) | Class check on victim | Gains mortal strike / healing reduction |
| Giant-Slayer | Killed a player 5+ levels above it | Level comparison | Bonus HP and damage |
| Opportunist | Killed an AFK player | `isAFK()` check on victim | Periodically dazes the target |
| Umbralforged | Promoted during Umbral Moon | `IsUmbralMoonActive()` check | Bonus intellect, AoE burst damage during umbral moon (Umbral Burst (Spell 100008)) |
| Deathblow | Killed a player in a single hit | Detect `OnDamage()` to verify player's full hp was dealt in a single hit | Telegraphed heavy strike when HP drops below configurable threshold (default 25%) — 3-second interruptible cast, damage scales with effective level. One attempt per threshold crossing; interrupt prevents recast until healed above threshold and dropped again. |
| Underdog | Killed a player whow as 5+ levels higher than them | Compare level of player to level of creature at combat start | Bonus expertise |
| Scavenger | Killed a player that had resurrection sickness | Detect weak aura 15007 on combat start | Bonus damage against debuffed targets |
| Duelist | Killed a player solo as a humanoid creature | Check `player->getAttackers().size() == 1` and `creature->GetCreatureType() == CREATURE_TYPE_HUMANOID` | Single target bonus damage |
| Plunderer | Killed a player that has 10000 * their level in currency | Evaluated using `GetMoney()` at time of kill. | Bonus gold injected into `creature_template` `mingold`/`maxgold` on the cloned entry, scaled by effective level. Appears in loot window. |
| Ironbreaker | Killed a plate-wearing class such as warrior, paladin, or death knight | Simple class check similar to Mage-Bane / Healer-Bane | Permanent passive aura (spell 100014, "Sundered Armor") granting 10% armor penetration via `SPELL_AURA_MOD_ARMOR_PENETRATION_PCT`. Applied in `RefreshAuras`, same pattern as Underdog. |
| Skinner | Killed a player with the Skinning profession | Snapshot `HasSkill(SKILL_SKINNING)` at combat entry | Affixed loot prefix grants +5/+10/+15/+20 Skinning skill via aura (spell 100030). |
| Ore-Gorged | Killed a player with the Mining profession | Snapshot `HasSkill(SKILL_MINING)` at combat entry | Affixed loot prefix grants +5/+10/+15/+20 Mining skill via aura (spell 100031). |
| Root-Ripper | Killed a player with the Herbalism profession | Snapshot `HasSkill(SKILL_HERBALISM)` at combat entry | Affixed loot prefix grants +5/+10/+15/+20 Herbalism skill via aura (spell 100032). |
| Forge-Breaker | Killed a player with the Blacksmithing profession | Snapshot `HasSkill(SKILL_BLACKSMITHING)` at combat entry | Affixed loot prefix grants +5/+10/+15/+20 Blacksmithing skill via aura (spell 100033). |
| Hide-Mangler | Killed a player with the Leatherworking profession | Snapshot `HasSkill(SKILL_LEATHERWORKING)` at combat entry | Affixed loot prefix grants +5/+10/+15/+20 Leatherworking skill via aura (spell 100034). |
| Thread-Ripper | Killed a player with the Tailoring profession | Snapshot `HasSkill(SKILL_TAILORING)` at combat entry | Affixed loot prefix grants +5/+10/+15/+20 Tailoring skill via aura (spell 100035). |
| Gear-Grinder | Killed a player with the Engineering profession | Snapshot `HasSkill(SKILL_ENGINEERING)` at combat entry | Affixed loot prefix grants +5/+10/+15/+20 Engineering skill via aura (spell 100036). |
| Vial-Shatter | Killed a player with the Alchemy profession | Snapshot `HasSkill(SKILL_ALCHEMY)` at combat entry | Affixed loot prefix grants +5/+10/+15/+20 Alchemy skill via aura (spell 100037). |
| Rune-Eater | Killed a player with the Enchanting profession | Snapshot `HasSkill(SKILL_ENCHANTING)` at combat entry | Affixed loot prefix grants +5/+10/+15/+20 Enchanting skill via aura (spell 100038). |
| Gem-Crusher | Killed a player with the Jewelcrafting profession | Snapshot `HasSkill(SKILL_JEWELCRAFTING)` at combat entry | Affixed loot prefix grants +5/+10/+15/+20 Jewelcrafting skill via aura (spell 100039). |
| Ink-Drinker | Killed a player with the Inscription profession | Snapshot `HasSkill(SKILL_INSCRIPTION)` at combat entry | Affixed loot prefix grants +5/+10/+15/+20 Inscription skill via aura (spell 100040). |

**Earned Traits (accumulated over lifetime):**

Earned traits are gained as conditions are met over the nemesis's lifetime. Multiple earned traits can coexist — a long-lived nemesis that has killed many players could have Territorial, Notorious, and Survivor simultaneously. Each earned trait adds its combat effect to the nemesis, making multi-trait nemeses progressively more dangerous to fight.

| Trait | Condition | Detection | Effect on Nemesis |
|-------|-----------|-----------|-------------------|
| Coward | HP drops below configurable threshold (default 15%) during combat | Flee chance rolled once at combat entry (default 30%); fires once per encounter if candidate | Flees with stealth aura (spell 100002), heals after flee duration (spell 100001), re-engages. Already-Coward nemeses always flee. |
| Notorious | Killed 5+ unique players (configurable) | `COUNT(DISTINCT player_guid)` query against `nemesis_elite_kill_log` on each player kill | Bonus damage% and flat HP (Dread Renown aura); affixed loot grants bonus to all stats |
| Survivor | Survived 3+ player combat encounters (configurable) | Combat exit while alive tracked via `_inCombatElites` set; death path clears tracking first to avoid false counts | Bonus armor and reduced crit damage taken (Battle-Hardened aura, spell 100015). Both values scale with effective level. |
| Territorial | Alive 3+ days (configurable) | Age check on every `OnCreatureUpdate` tick, short-circuits once earned | Sets faction to Monster (hostile to all players), sets `REACT_AGGRESSIVE`, expands wander radius (effective_level × configurable multiplier, default 2.0), increases detection range (configurable multiplier, default 1.5×) |
| Blight | Killed both horde and alliance players | `OnNemesisKillPlayer` query other players to determine of both factions are in kill log + current. | Shadow damage shield — reflects shadow damage to melee attackers (Blighted Aura, spell 100017). Melee-only; `SPELL_AURA_DAMAGE_SHIELD` is a 3.3.5 engine limitation. Scales with effective level. |
| Spellproof | Surive a fight where a player cast a crowd control spell on it | Detect using `OnAuraApply` and if the nemesis has an aura with a CC mechanic flag (`MECHANIC_POLYMORPH`, `MECHANIC_FEAR`, `MECHANIC_STUN`, `MECHANIC_ROOT`, etc), grant trait on combat exit | Reduced CC duration |
| Scarred | Survived a fight where a player actively cast a healing spell while in combat with it | Track via `OnSpellCast` — if the caster is a Player in combat and any creature in the caster's threat list is a nemesis, mark that nemesis's spawnId in a `_healedAgainstElites` set. Grant on survived combat exit, clear on combat exit. | Applies healing reduction debuff |
| Enraged | Took damage from 5+ unique players in a single combat encounter and survived | Track `_combatPlayerHits` as `unordered_map`. Populate `OnDamage` when victim is nemesis. Grant on combat exit if threshold met. | Stacking damage buff the more players are in combat with it |
| Dayborn | Survived a combat encounter during daytime (6:00-17:59) | Check current server time on combat exit | Bonus damage during daytime hours |
| Nightborn | Survived a combat encounter during nighttime (18:00-5:59) | Check current server time on combat exit | Bonus damage during nighttime hours | 
| Nomad | Has been seen in 3+ zones | nemesis_elite_zones table (elite_id, zone_id, unique on the pair) with INSERT IGNORE on zone change, then SELECT COUNT(*) to check the threshold. The check should live in UpdateLastSeen since that already runs throttled and tracks zone. If the number of unique zones on UpdateLastSeen is 3 or more, grant trait | Increased movement speed |
| Sage | Alive for a configurable number of days (default 7) | Check age on combat tick, same pattern as Territorial. Configurable via `elite_trait_sage_days_threshold`. | Affixed loot suffix grants +3%/+5%/+8%/+12% bonus XP on kill via `OnPlayerGiveXP` hook. |
| Studious | Existed near a gathering node | Once per minute during `UpdateLastSeen`, scan for a `GAMEOBJECT_TYPE_CHEST` within configurable range (default 30 yards). If found, grant trait. | Affixed loot suffix grants +5%/+10%/+15%/+25% chance of a bonus skillup on successful profession skillup via `OnPlayerUpdateSkill` hook. |


### Threat Score

The threat score is a composite number representing how dangerous a nemesis currently is. It is used to determine loot quality tier, bounty board ranking, and whether a kill qualifies for power stacks.

**Experience and history are the primary drivers of threat score.** A high-level nemesis that just spawned is dangerous by nature but unproven. A mid-level nemesis that has killed a dozen unique players, earned seven traits, and survived for weeks is the real threat — its score reflects the story it has lived. Effective level gets a nemesis in the door; kills, traits, and age push it to the top.

**Unique kills are weighted far higher than raw kill count.** This discourages farming (a player dying repeatedly to the same nemesis barely moves the needle) and rewards organic diverse-player encounters. A nemesis that has killed 20 total players but only 3 unique players scores dramatically lower than one with 20 kills across 15 unique players.

**Implemented formula:** `(effectiveLevel × 5) + (killCount × 1) + (uniqueKillCount × 12) + (traitCount × 15) + min(100, daysSincePromotion × 4)`

Where `traitCount` is the total number of set bits across both origin traits and earned traits, and `uniqueKillCount` is the number of distinct player GUIDs in `nemesis_elite_kill_log` for this nemesis.

All formula weights and the age cap are configurable via `nemesis_configuration`:

| Config Key | Default | Description |
|---|---|---|
| `elite_threat_weight_level` | 5 | Threat score points per effective level |
| `elite_threat_weight_kill` | 1 | Threat score points per total player kill (raw count) |
| `elite_threat_weight_unique_kill` | 12 | Threat score points per unique player killed |
| `elite_threat_weight_trait` | 15 | Threat score points per trait (origin + earned combined) |
| `elite_threat_age_bonus_per_day` | 4 | Threat score points gained per day alive (capped) |
| `elite_threat_age_bonus_cap` | 100 | Maximum threat score bonus from age |

The threat score updates continuously as the nemesis scales, kills players, and earns traits. It is recalculated on every event that changes any component — kill, level gain, trait earned, age tick — and persisted to the database immediately.

### Nemesis Scaling

Nemeses gain effective level over time through kills and survival. A Kobold Vermin promoted in Elwynn can eventually become a serious threat far beyond its original zone level. Scaling factors:

- **On promotion:** +3 effective levels (configurable via `elite_level_gain_on_promote`).
- **Per player kill:** +1 effective level (configurable via `elite_level_gain_per_kill`). The cloned `creature_template` row is updated in sync so the next `UpdateEntry` call reads the correct level.
- **Passive age scaling:** +1 effective level per configurable interval (default 24 hours, via `elite_age_level_interval_hours`). Tracked via `last_age_level_at` timestamp to survive server restarts.
- **Hard cap:** 83 (configurable via `elite_level_cap`), matching the hardest raid bosses in the game.

When a nemesis gains a level, it is removed from the `_buffedElites` tracking set so the next `OnCreatureUpdate` tick fires `ApplyStats` fresh — this calls `UpdateEntry` (which re-reads minlevel/maxlevel, runs `SelectLevel`, rebuilds base HP/damage) and then re-applies the percentage multipliers cleanly.

### Nemesis Behavior

- **Territory:** Long-lived nemeses that earn the Territorial trait use random movement with a scaled wander radius proportional to their effective level.
- **Rivalries:** If two nemeses of conflicting factions are in proximity, they fight each other. Players can exploit this.
- **Umbralforged:** Nemeses with the Umbralforged origin trait receive bonus stats during future Umbral Moon events.

### Death and Persistence

Nemeses persist until killed by players. They do not despawn, reset, or decay. Death is detected via `OnPlayerCreatureKill` in `NemesisElitePlayerScript`, which provides a real killer reference. `MarkDead` records `killed_at` and `killed_by_player_guid`, and deletes the cloned `creature_template` and `creature_template_model` rows to prevent orphaned entries from accumulating. The ObjectMgr in-memory cache retains the template for the remainder of the session (harmless since the creature won't respawn). If the world gets oversaturated (see Purge Events), purge mechanics can thin the herd.

Position is persisted via `UpdateLastSeen`, throttled to one DB write per minute to avoid excessive load.

### GM Commands

All commands are under `.nemesis elite`:

| Command | Description |
|---------|-------------|
| `.nemesis elite promote` | Force-promote the targeted creature into a nemesis elite (bypasses chance roll, uses the GM as the "killed player" for origin trait evaluation). |
| `.nemesis elite demote` | Revert the targeted nemesis to its original creature template and mark it dead in the DB. |
| `.nemesis elite traits` | Display the targeted nemesis's full status: name, elite ID, spawn ID, effective level, threat score, kill count, survival attempts, all origin and earned traits. |
| `.nemesis elite setlevel <level>` | Set the targeted nemesis's effective level (capped at configured max). |
| `.nemesis elite addtrait earned <name>` | Add an earned trait by name (coward, notorious, survivor, territorial). |
| `.nemesis elite removetrait earned <name>` | Remove an earned trait by name. |
| `.nemesis elite addtrait origin <name>` | Add an origin trait by name (ambusher, executioner, magebane, healerbane, giantslayer, opportunist, umbralforged). |
| `.nemesis elite removetrait origin <name>` | Remove an origin trait by name. |
| `.nemesis elite debug` | Dump the targeted creature's stats (HP, armor, damage, attributes, resistances) and list all active nemesis auras with per-effect amount, base, and aura type. |

---

## System 2: Umbral Moon

### Schedule

Umbral Moon occurs on a fixed, predictable schedule. All schedule parameters are configurable via `nemesis_configuration`:

- **Days of week:** Wednesday and Saturday (default), comma-separated 3-letter abbreviations.
- **Times:** 8:00 AM and 8:00 PM (default), comma-separated HH:MM 24h format.
- **Duration:** 2 hours (default).

This results in 4 Umbral Moon events per week, 8 hours total. The schedule is inclusive of different timezones by having both morning and evening windows.

Umbral Moon active state is **derived from the schedule at query time** — the code checks whether the current server time falls within a scheduled window. There is no separate "is_active" flag to maintain or risk getting out of sync. A manual GM override layer allows starting/stopping an Umbral Moon independently of the schedule.

Umbral Moon schedules are discoverable via the physical bounty board in major cities, which displays the next scheduled Umbral Moon alongside the active nemesis leaderboard.

### Effects During Umbral Moon

- **Visual — Sky:** The day/night cycle is overridden via `SMSG_LOGIN_SETTIMESPEED` to force a target hour (default midnight). A smooth sky interpolation transitions the client through dusk/dawn at a configurable speed (default 60 game-minutes per real-second), then a "freeze" packet locks the sky in place. The freeze is maintained by periodic resync packets every 30 seconds (since the 3.3.5 client ignores the speed field). On Umbral Moon end, the sky smoothly transitions back to real server time. The ambient persists across map/zone transfers via `OnPlayerMapChanged`, which re-applies the override instantly.
- **Visual — Creatures:** All eligible open-world creatures receive visual aura 100000 (a custom `spell_dbc` entry) for the duration of the Umbral Moon. The aura is applied and removed per-creature via `AllCreatureScript::OnAllCreatureUpdate`, which naturally covers both existing and newly-spawned creatures without a separate spawn hook.
- **Mob Difficulty:** All eligible open-world mobs gain stat buffs — HP multiplier (default 2×) and damage multiplier (default 2×), applied via `ApplyStatPctModifier(TOTAL_PCT)`. Reverted at event end using the multiplicative inverse formula `(-100 × val) / (100 + val)`. Health percentage is preserved across buff/debuff transitions.
- **Rewards — XP:** Bonus XP multiplier (default 1.5×) applied via `OnPlayerGiveXP`, open-world only.
- **Rewards — Loot:** Enhanced loot entries are regenerated on every server startup into `creature_loot_template` using a dedicated LootMode bitmask (default 0x4) and GroupId (default 250). Items are selected from `item_template` using quality-weighted tiers (Poor/Common as fallback pool; Uncommon/Rare/Epic+ get escalating weighted chances). During Umbral Moon, the LootMode is toggled on creatures to enable these bonus drops.
- **Nemesis Spawn Rate:** Promotion chance increases to the configured Umbral Moon rate (default 25%).
- **Server-wide Announcement:** Messages are sent at event start and end via `SendServerMessage`.

### Scope

Umbral Moon effects **only apply to open world content**. Instanced content (dungeons, raids, battlegrounds, arenas) is unaffected — no stat buffs on instanced mobs, no bonus XP/loot, no increased nemesis spawn chance. Eligibility uses the same `IsEligibleCreature` check as the nemesis promotion system.

### GM Commands

| Command | Description |
|---------|-------------|
| `.umbralmoon start` | Start a manual Umbral Moon immediately for the configured duration. |
| `.umbralmoon stop` | Stop the current manual Umbral Moon override. |
| `.umbralmoon status` | Display current Umbral Moon state (active/inactive, manual override, next scheduled window). |

### Design Intent

Umbral Moon is the pressure valve that seeds the world with nemeses. It's the "planting time bombs" phase. The days between Umbral Moons are when those time bombs go off and players deal with the consequences.

---

## System 3: Affixed Loot

### Generation Philosophy

Loot dropped by nemeses is **contextually generated from the nemesis itself**. The nemesis IS the loot table. No reference is made to what the victim was wearing when they died — this is critical to prevent exploitation (dying in soulbound BIS gear to generate tradeable affixed versions).

Loot table shares in the loot from its derivative mob. The affixed item should simply be ADDED to the rolled loot from the original loot table.

### Drop Quantity

**At most one affixed item drops per nemesis kill, regardless of group size.** The drop chance is configurable (default 85%) — see Drop Chance below. When an affixed item does drop, this design has several consequences:

- Solo players get the drop to themselves — no competition, no rolling. But they must be strong enough to handle the nemesis alone.
- Groups make the kill safer but the single item is distributed via standard group loot rules (Need/Greed/Pass). No custom loot system needed.
- High-threat nemeses with desirable trait pools become contested content. Guilds organizing kills for top-10 bounty board targets still only get one drop, creating internal loot decisions and a reason for bounty hunting specialists to operate solo or in small trusted groups.
- Since each nemesis is a one-time kill (they die permanently), the specific combination of threat level, origin traits, earned trait suffix pool, and effective level will never exist again. Every drop has unique provenance, driving a healthy trade economy where no two items are truly identical.

### Item Generation

**Slot:** Determined by whichever item is cloned from the nemesis's original creature loot table. The system does not select a slot and then generate an item — it selects a real equippable item from the creature's loot, and the slot is whatever that item occupies. This keeps the drop thematic: a nemesis promoted from a sword-wielding humanoid drops an affixed sword, not a random pair of boots.

**Item Level / Quality Tier:** Driven by the nemesis's **threat score**. All thresholds are configurable via `nemesis_configuration`:

| Threat Score Range | Quality | Affix Slots | Config Key |
|--------------------|---------|-------------|------------|
| 0–299 | Green (Uncommon) | 1 (prefix OR suffix) | `affix_tier_threshold_green` (default 0) |
| 300–599 | Blue (Rare) | 2 (prefix AND suffix) | `elite_affix_tier_threshold_blue` (default 300) |
| 600+ | Purple (Epic) | 2 (from full affix pools) | `elite_affix_tier_threshold_purple` (default 600) |
| 700+ AND Bounty Board top N | Legendary | 2 (from full affix pools) | `elite_affix_tier_threshold_legendary` (default 700) |

Legendary tier requires **both** a minimum threat score floor (configurable, default 700) **and** placement in the Bounty Board top N (configurable via `bounty_board_top_count`, default 10). This prevents low-score nemeses on a fresh server from producing Legendary drops purely by position, while ensuring that even on a mature server, only the most visible and storied nemeses qualify. A nemesis at 800 threat that is ranked #15 on the bounty board drops Purple, not Legendary — the position gate creates social competition for top-N slots.

**Green Tier Affix Selection:** Green items receive exactly one affix. If the nemesis has at least one earned trait, a 50/50 coin flip determines whether the item gets a prefix (from the origin trait) or a suffix (from the earned trait pool). If the nemesis has no earned traits, the item always receives a prefix — there is no suffix pool to draw from. This means Green drops from freshly promoted nemeses always tell the origin story, while Green drops from older low-threat nemeses have a chance at a suffix.

**Base Stats:** The affixed item is cloned from the nemesis's **original creature's loot table** — not generated from a formula. On nemesis death, the system queries `creature_loot_template` (using the original `creature_entry`, not the cloned 9M+ entry) for equippable items (weapons and armor), resolving reference loot entries one level deep. One item is selected at random from the candidates. That `item_template` entry is cloned into a custom entry range, the affix stats are bolted on, the item is renamed with the prefix/suffix, and the quality color is upgraded to match the tier.

This means the base item is always thematic to the creature — a Scarlet Crusader nemesis drops affixed Scarlet gear, a Dragonkin drops dragon-themed weapons. The affix system is purely additive: it enhances what the creature naturally drops, it does not conjure gear out of thin air.

**No equippable fallback:** If the creature's loot table contains no equippable items (only junk, reagents, quest items, etc.), no affixed drop is generated. The nemesis drops its normal loot only. No generic fallback table is used.

**Quality floor:** None. If the loot table only contains grey/white items, the clone is a grey/white base item with affix stats added on top. The affix compensates for the low base quality. A level 5 Kobold Vermin nemesis drops an affixed Kobold mining pick — the affix makes it interesting, not the base item.

**Drop chance:** Affixed item drops are not guaranteed. The chance is configurable via `elite_affix_drop_chance` (default 85, representing 85%). Each nemesis is a one-time permanent kill, so the high default respects the player's effort while preserving RNG excitement. If the economy floods with affixed gear, dial it back; if it feels punishing, crank it to 100.

### Affix System

Each affixed item has at most **one prefix and one suffix**. The prefix is drawn from the nemesis's origin traits. The suffix is drawn from the nemesis's earned trait pool.

**Suffix Pool Selection:** When a nemesis has multiple earned traits, the suffix is selected from that pool — either randomly or weighted toward the rarest/hardest-to-earn trait. This means a nemesis with more earned traits has a larger pool of *possible* suffixes, making multi-trait nemeses more valuable to hunt — not because the gear has more affixes, but because the pool of potential suffixes is wider. Players hunting for a specific suffix seek out nemeses whose earned trait pools include what they want, but there's still a roll involved.

This creates several interesting dynamics:

- Two players who kill the same nemesis in a group could want different suffixes, but only one item drops — suffix negotiation becomes part of the loot decision.
- The bounty board becomes a tool for evaluating suffix odds — players can see the full trait list and judge whether the earned trait pool includes what they're after.
- No two items have the same provenance. Once a nemesis dies, its exact combination of origin traits, earned trait pool, and threat level is gone forever.

**Prefixes — "How It Was Born" (drawn from origin traits):**

Each prefix maps 1:1 to an origin trait. The prefix on an affixed item tells the story of how the nemesis that dropped it was born. Values scale by item quality tier (Green / Blue / Purple / Legendary). Proc-based affixes scale primarily on proc chance with secondary magnitude bumps. All values are configurable via `nemesis_configuration` and represent level-60-equivalent output — damage/heal values also scale with the item's level via `baseDamage + (itemLevel × perLevel)`.

| Prefix | Origin Trait | Effect | Green | Blue | Purple | Legendary |
|--------|-------------|--------|-------|------|--------|---------|
| Ambusher's | Ambusher | Bonus Crit Rating | +15 | +26 | +38 | +52 |
| Executioner's | Executioner | Execute-style proc (bonus damage vs targets below 20% HP) | 3% proc, 40 damage | 5% proc, 70 damage | 7% proc, 100 damage | 10% proc, 140 damage |
| Mage-Bane's | Mage-Bane | Chance on hit to increase target cast time | 2% proc, +15% cast time for 4s | 3% proc, +20% cast time for 5s | 4% proc, +25% cast time for 6s | 6% proc, +30% cast time for 6s |
| Healer-Bane's | Healer-Bane | Bonus Healing Done | +20 | +35 | +50 | +70 |
| Giant-Slayer's | Giant-Slayer | Bonus Attack / Spell Power | +20 | +35 | +50 | +70 |
| Opportunist's | Opportunist | Chance on hit to daze target | 2% proc, 3s daze | 3% proc, 4s daze | 4% proc, 5s daze | 6% proc, 5s daze |
| Umbralforged | Umbralforged | Periodic AoE shadow damage while in combat (server-side) | 8 damage / 3s, 10yd | 14 damage / 3s, 10yd | 20 damage / 3s, 10yd | 28 damage / 3s, 10yd |
| Deathblow | Deathblow | Bonus Critical Strike Damage | +2% | +4% | +5% | +7% |
| Underdog's | Underdog | Bonus Expertise Rating | +10 | +18 | +26 | +36 |
| Scavenger's | Scavenger | Bonus damage vs debuffed targets (server-side) | +3% | +5% | +8% | +11% |
| Duelist's | Duelist | Bonus damage when only one enemy is in combat with you (server-side) | +3% | +5% | +8% | +11% |
| Plunderer's | Plunderer | Bonus gold on creature kills (server-side, scales with item level) | +5% | +10% | +15% | +25% |
| Ironbreaker's | Ironbreaker | Bonus Armor Penetration Rating | +15 | +26 | +38 | +52 |
| Skinner's | Skinner | Bonus Skinning skill | +5 | +10 | +15 | +20 |
| Ore-Gorged | Ore-Gorged | Bonus Mining skill | +5 | +10 | +15 | +20 |
| Root-Ripper's | Root-Ripper | Bonus Herbalism skill | +5 | +10 | +15 | +20 |
| Forge-Breaker's | Forge-Breaker | Bonus Blacksmithing skill | +5 | +10 | +15 | +20 |
| Hide-Mangler's | Hide-Mangler | Bonus Leatherworking skill | +5 | +10 | +15 | +20 |
| Thread-Ripper's | Thread-Ripper | Bonus Tailoring skill | +5 | +10 | +15 | +20 |
| Gear-Grinder's | Gear-Grinder | Bonus Engineering skill | +5 | +10 | +15 | +20 |
| Vial-Shatter's | Vial-Shatter | Bonus Alchemy skill | +5 | +10 | +15 | +20 |
| Rune-Eater's | Rune-Eater | Bonus Enchanting skill | +5 | +10 | +15 | +20 |
| Gem-Crusher's | Gem-Crusher | Bonus Jewelcrafting skill | +5 | +10 | +15 | +20 |
| Ink-Drinker's | Ink-Drinker | Bonus Inscription skill | +5 | +10 | +15 | +20 |

**Suffixes — "What It Became" (one selected from earned trait pool):**

Each suffix maps 1:1 to an earned trait. When a nemesis has multiple earned traits, the suffix is selected from that pool. Values scale by item quality tier. Several suffixes require server-side runtime hooks on the player's equipped items rather than static `item_template` stats — these are tagged "(server-side)" below.

| Suffix | Earned Trait | Effect | Green | Blue | Purple | Legendary |
|--------|-------------|--------|-------|------|--------|---------|
| of Cowardice | Coward | Reduced threat generation (server-side) | -3% | -5% | -8% | -12% |
| of Notoriety | Notorious | Bonus to all stats (flat) | +4 all stats | +7 all stats | +10 all stats | +14 all stats |
| of Persistence | Survivor | Bonus Armor / Defense Rating | +80 Armor | +140 Armor | +200 Armor | +280 Armor |
| of Dominion | Territorial | Bonus threat generation (server-side) | +3% | +5% | +8% | +12% |
| of Blight | Blight | Shadow damage reflection while equipped (server-side) | 10 shadow per hit taken | 18 shadow per hit taken | 26 shadow per hit taken | 36 shadow per hit taken |
| of Spell-Warding | Spellproof | Bonus Resilience | +15 | +26 | +38 | +52 |
| of Wounding | Scarred | Bonus MP5 / Health Regeneration | +6 MP5 | +10 MP5 | +15 MP5 | +21 MP5 |
| of the Frenzy | Enraged | Bonus Haste Rating | +15 | +26 | +38 | +52 |
| of the Sunborn | Dayborn | Bonus Attack / Spell Power | +15 | +26 | +38 | +52 |
| of the Nightborn | Nightborn | Bonus Dodge / Agility | +12 Agility | +21 Agility | +30 Agility | +42 Agility |
| of Wandering | Nomad | Bonus Movement Speed (server-side) | +3% | +5% | +7% | +8% |
| of the Sage | Sage | Bonus XP on kill (server-side, scales with item level) | +3% | +5% | +8% | +12% |
| of Craft | Studious | Bonus skillup chance on successful profession skillup (server-side) | +5% | +10% | +15% | +25% |

**Affix combination count:** 24 prefixes × 13 suffixes = **312 possible prefix/suffix combinations** at the Blue+ tier (both slots filled), plus 24 + 13 = 37 single-affix variations at the Green tier. Each new trait added to the nemesis system should map to a new affix entry in the corresponding table.

**Server-side affixes:** 15 of the 37 total affixes require runtime hooks rather than static `item_template` stats. All 15 are fully implemented via the equipped affix cache (`nemesis_item_affixes` table + per-player in-memory cache). Hooks fire in `UnitScript::OnDamage` (proc-based prefixes, Blight reflection), `PlayerScript::OnPlayerCreatureKill` (Plunderer gold), `PlayerScript::OnPlayerGiveXP` (Sage XP), `PlayerScript::OnPlayerUpdateSkill` (Craft bonus skillup), `RefreshAffixAuras` on equip/unequip (Deathblow crit damage, Wandering speed, Cowardice/Dominion threat, 11 profession skills), and `WorldScript::OnUpdate` (Umbralforged periodic AoE via combat entry/exit tracking).

### Item Template Creation

Affixed items are created by cloning a real `item_template` row into a custom entry range, modifying it, and injecting it into the loot. This mirrors the `creature_template` cloning pattern used for nemesis nameplates.

**Entry range:** `9,000,000 + elite_id`, same offset as the creature template clone. Since `item_template` and `creature_template` are completely separate tables in AzerothCore, the same numeric entry can exist in both without conflict. Given an `elite_id`, both the creature clone and the item clone live at the same offset in their respective tables.

**Cloned fields modified:**

- **`entry`** — set to `9000000 + elite_id`
- **`name`** — formatted as `[Prefix] [Base Item Name] [Suffix]`, e.g., "Ambusher's Scarlet Boots of Dominion". Green items with only one affix omit the missing half: "Umbralforged Kobold Mining Shovel" (prefix only) or "Worn Leather Belt of the Frenzy" (suffix only).
- **`Quality`** — upgraded to match the quality tier (2=Uncommon/Green, 3=Rare/Blue, 4=Epic/Purple, 5=Legendary).
- **`bonding`** — set to `2` (Bind on Equip) regardless of the base item's original binding value.
- **`description`** — flavor text injected from a random template. The template pool is configurable via `elite_affix_item_flavor_templates`, a semicolon-delimited string of templates using `{name}` as the nemesis name placeholder. Default: `Taken from the corpse of {name}.;Looted from {name}.;Pried from the remains of {name}.;Stripped from {name} after its final defeat.;Once wielded by {name}.`
- **Stat slots** — `item_template` has `stat_type1` through `stat_type10` and `stat_value1` through `stat_value10`. The system finds empty stat slots on the cloned item and injects the affix stat values. For server-side affixes (procs, threat modification, damage reflection, etc.), no stat slots are consumed — those effects are handled by runtime hooks keyed off the `nemesis_item_affixes` table.

**`nemesis_item_affixes` table:**

Bridges generated items to the runtime hook system. One row per affixed item, persisted indefinitely (items live in player inventories, mail, and the auction house).

```
nemesis_item_affixes
  item_entry     INT UNSIGNED PK   -- cloned item_template entry (9M+ range)
  elite_id       BIGINT UNSIGNED    -- which nemesis dropped this item
  prefix_trait   INT UNSIGNED       -- origin trait bitmask value (0 if suffix-only Green)
  suffix_trait   INT UNSIGNED       -- earned trait bitmask value (0 if prefix-only Green)
  quality_tier   TINYINT UNSIGNED   -- 0=Green, 1=Blue, 2=Purple, 3=Legendary
```

Server-side affix hooks scan the player's equipped items, look up each entry in this table, and apply the appropriate effects. The `quality_tier` column determines which scaling row from the affix tables to use.

**Cleanup:** Item template clones are never deleted — unlike creature clones which are removed on nemesis death, item clones must persist as long as the item exists anywhere in the game (inventory, bank, mail, auction house). The table grows over time but each row is small. Future optimization could scan for orphaned entries whose items no longer exist in any character storage, but this is not a launch concern.

### Tradeability

Affixed gear is **Bind on Equip**. The cloned `item_template` row always sets `bonding = 2` regardless of the base item's original binding. This means affixed items can be freely traded, auctioned, and mailed until a player equips them — at which point they become soulbound. This enables:

- A bounty hunting "profession" where players hunt nemeses and sell the drops
- A player-driven economy around rare affix combinations
- Social value to the bounty board (checking for desirable affix sources)
- Future monetization option: a cash shop consumable that removes soulbound status from an equipped affixed item, making it tradeable again

### Loot Credit

Uses WoW's existing tagging system. If the nemesis's healthbar is gray (you haven't tagged it), you don't get loot. Group tagging works normally. No custom system needed — players already understand this intuitively.

This means a solo player tracking their personal nemesis gets guaranteed loot without worrying about snipe groups, and competitive convergence on bounty board targets creates natural race-to-tag moments.

---

## System 4: Nemesis Power Stacks

### Mechanic

When a player participates in killing a nemesis **above a minimum threat score threshold**, they receive a permanent stat buff (SP, AP, Armor, Expertise, Crit Rating, Hit Rating, etc). These buff stacks with each qualifying kill. When a nemesis is killed, each participating member should receive a prompt on screen of three stat buff options that they can choose from.

The stat offerings should have rarities through common, uncommon, rare, and epic. Common variants should give a lower amount of the stat, scaling up to epic which gives the most. Could follow something a scaling like 1x, 1.5x, 2.5x, 4x the base. Rarity on each of the three offerings is rolled independently, weighted by rarity (common most likely, epic least likely). Threat score does not influence rarity — the threat score threshold gates whether offerings are granted at all, but once qualified, all three options are pure random rolls. Three commons is possible. Three epics is possible. This preserves the roguelike feel where every qualifying kill is a genuine gamble.

Stat offerings should queue up like talent points, allowing the player to resolve them at will. When there are pending stat points, we should display a UI element that opens a UI for them to pick from (as well as options to hide that ui if they want to approach it again later).  

### Diminishing Returns

Each successive stack grants less benefit than the previous one. The first few kills provide substantial boosts; the difference between stack 50 and 60 is far less impactful than between 0 and 10.

> **TODO:** Define the exact diminishing returns curve. Logarithmic? Inverse? Flat reduction per tier? Need to playtest. Also define the minimum threat score threshold for qualifying kills.

### Death Penalty

**Dying wipes ALL accumulated stacks. No exceptions.**

This is the core tension of the entire server. The longer you survive and the more stacks you accumulate, the more you have to lose — and the nemesis born from your death will be all the more dangerous for it.

### High-Stack Player Deaths

When a player with a significant number of stacks dies, it should be a server event:

- Server-wide announcement: *"[Player] has fallen with [X] Nemesis stacks. A great darkness stirs in [Zone]."*
- This creates server-wide "oh shit" moments that drive community engagement

---

## System 5: Bounty Board

### Purpose

A public leaderboard of the top 10 most dangerous nemeses currently alive on the server. Serves both as an information tool and a social hub.

### Display Information

The bounty board sends structured data to the client via `SendSysMessage` using a custom protocol prefix (`@@NBB@@`) for addon consumption. Each nemesis entry includes:

- Nemesis name
- Current effective level
- Threat score
- Kill count (how many players it's taken down)
- Origin traits and earned traits (bitmasks — players use this to evaluate potential affix drops and suffix odds)
- Current zone (resolved from DBC `AreaTableEntry`) and last known map coordinates
- Age (computed from `promoted_at`)
- Umbral Moon origin flag
- Origin player name (resolved from `characters` table)
- Last seen timestamp

The header packet also includes the total living nemesis count and the Umbral Moon schedule (days, times, duration) so the client addon can display the next scheduled event.

### Access

Implemented as a physical `GAMEOBJECT_TYPE_GOOBER` (entry 200100, displayId 3053) spawned in all major cities: Stormwind, Ironforge, Darnassus, Orgrimmar, Thunder Bluff, Undercity, Shattrath, Dalaran, Exodar, and Silvermoon. Players interact with the board via the gossip system; the script queries `nemesis_elite` for the top N (configurable, default 10) living elites ordered by threat score, sends the data, then closes the gossip menu.

### Configuration

| config_key | config_value | Description |
|------------|-------------|-------------|
| bounty_board_top_count | 10 | Number of top nemeses displayed on the bounty board. |

---

## Purge Events

If the world becomes oversaturated with nemeses (threshold TBD), a server-wide purge event triggers:

- Purge existing nemeses, removing enough (lowest threat score first) to be the defined % of the threshold (for example, if the threshold is 100, and there are 200 nemesis in the server, and its configured at 50%, we should prune 150 of them to bring the number down to 50)
- Nemeses in the top 10 (which would be displayed on the bounty board) are NOT eligible for purging
- Purge can only occur once a week on Sundays at 12AM server time. This makes it such that players can't flood the server with nemesis just to get a purge to trigger.
- Could be tied to a narrative (e.g., "The Argent Crusade has culled the growing darkness in the realm")

---

## Revenge System

### Vengeance Tracking

The system tracks which player's death created each nemesis (stored as `origin_player_guid` in `nemesis_elite`). When that specific player participates in killing their nemesis, they earn a **vengeance point** — a persistent counter stored in the `nemesis_player_vengeance` table and cached in memory at startup for performance.

**Credit assignment:** The killer always receives vengeance credit if their GUID matches `origin_player_guid`. Group members within configurable range (default 100 yards) can also earn vengeance if group vengeance is enabled — but only if **their own** GUID matches the origin player, not simply for being in the group. This means group vengeance only applies when the origin player is in the group but didn't land the killing blow.

**On vengeance kill:** The player receives a chat notification (`"Vengeance! You have slain a nemesis born from your death."`) and the system checks whether any new title thresholds have been crossed.

### Title Progression

Vengeance points unlock permanent DBC titles at configurable thresholds. Titles are cumulative — crossing a higher threshold grants the new title without removing previous ones. All titles use custom CharTitles.dbc entries in the 178–183 ID range (Mask_ID / bit_index 143–148).

| Threshold | Title ID | Mask_ID | Title |
|-----------|----------|---------|-------|
| 1 | 178 | 143 | %s the Wronged |
| 5 | 179 | 144 | %s the Grudgekeeper |
| 15 | 180 | 145 | %s the Relentless |
| 25 | 181 | 146 | %s the Vengeful |
| 50 | 182 | 147 | %s the Unforgiving |
| 100 | 183 | 148 | %s the Inevitable |

Titles are restored silently on login from the cached vengeance count — no chat spam for titles already earned. New title grants produce a chat notification with the title name in gold.

### What Vengeance Does NOT Grant

- No bonus loot
- No exclusive affix tiers
- No mechanical advantage whatsoever

This is purely social prestige. It cannot be sold as a service because there's nothing transferable. The reward is reputation and bragging rights.

### Grudge List

The grudge list is sent as part of the `@@NBB@@` bounty board protocol (v2+). When a player opens the bounty board, in addition to the server-wide leaderboard, they receive a personal section showing all living nemeses that originated from their deaths:

```
@@NBB@@RH~{vengeanceCount}~{grudgeCount}
@@NBB@@RE~{eliteId}~{name}~{level}~{threatScore}~{kills}~{originTraits}~{earnedTraits}~{zoneName}~{mapId}~{posX}~{posY}~{posZ}~{ageSeconds}~{deathsToYou}
@@NBB@@RF
```

Message types: `RH` = Revenge Header, `RE` = Revenge Entry, `RF` = Revenge Footer. Entries are ordered by threat score descending. The `deathsToYou` field is computed from `nemesis_elite_kill_log` to show how many times that specific nemesis has killed the viewing player.

This enables the personal revenge fantasy — a player can see which nemeses were born from their deaths, where they are, and how dangerous they've become.

### Configuration

| Config Key | Default | Description |
|------------|---------|-------------|
| `revenge_enabled` | 1 | Enable/disable the entire revenge system |
| `revenge_group_vengeance_enabled` | 1 | Allow group members to earn vengeance credit |
| `revenge_group_vengeance_max_distance` | 100.0 | Max yards from killer for group vengeance credit |

### GM Commands

| Command | Description |
|---------|-------------|
| `.nemesis revenge grudge` | Send the full bounty board protocol (leaderboard + personal grudge list) to the GM. |
| `.nemesis revenge vengeance` | Display the target player's vengeance count, current title, and progress toward the next title. |

### Data Model

**`nemesis_player_vengeance`** — One row per player with vengeance history:
- `player_guid` (INT UNSIGNED, PK) — character GUID
- `vengeance_count` (INT UNSIGNED, default 0) — total vengeance kills

All vengeance counts are loaded into an in-memory cache at startup. Runtime increments update the cache and issue an async DB write (`INSERT ... ON DUPLICATE KEY UPDATE`).

---

## Technical Considerations

### Data Model

All custom tables use the **`nemesis_`** prefix for consistent identification.

**`nemesis_elite`** — One row per promoted nemesis entity. Tracks:
- `elite_id` (auto-increment PK), `creature_guid` (spawn GUID), `creature_entry` (original template), `custom_entry` (cloned template in 9,000,000+ range)
- `name`, `effective_level`, `threat_score`, `kill_count`, `survival_attempts`
- `origin_traits` (bitmask, frozen at promotion), `earned_traits` (bitmask, accumulated)
- `origin_player_guid`, `umbral_moon_origin`
- `last_map_id`, `last_zone_id`, `last_pos_x/y/z`, `last_seen_at`
- `promoted_at`, `last_age_level_at`
- `is_alive`, `killed_at`, `killed_by_player_guid`

**`nemesis_elite_kill_log`** — One row per player killed by a nemesis. Used for unique kill tallying (Notorious trait), revenge system tracking.
- `id` (auto-increment PK), `elite_id`, `player_guid`, `killed_at`

**`nemesis_player_vengeance`** — One row per player with revenge kill history. Loaded into memory at startup.
- `player_guid` (INT UNSIGNED, PK), `vengeance_count` (INT UNSIGNED, default 0)

**`nemesis_player_stacks`** — Player power stack persistence (System 4, not yet created):
- Player GUID, current stack count, stack value (accounting for diminishing returns)
- Per-stack detail: which stat was chosen, rarity rolled, value granted
- Vengeance count, nemesis kill history

**`nemesis_configuration`** — Shared key-value config table used by all nemesis modules:
- `config_key` (PK), `config_value`, `description`

### Module Architecture

Implemented as four AzerothCore modules with no compile-time dependencies between them. All inter-module communication flows through the shared `nemesis_configuration` table.

| Module | Scripts | Responsibility |
|--------|---------|---------------|
| **mod-nemesis-elite** | `WorldScript` (config load, DB init, Umbralforged affix pulse update), `PlayerScript` (promotion rolls, kill detection, affix cache management, equip/unequip aura refresh, combat enter/exit for Umbralforged, XP/gold/skillup affix hooks), `AllCreatureScript` (stat re-application on spawn/restart), `UnitScript` (Executioner HP tracking, Enraged tracking, affix proc hooks, Blight reflection), `CommandScript` (GM commands), `SpellScript` (Deathblow Strike, Coward Heal, Umbral Burst) | Promotion logic, trait assignment/detection, scaling, persistence, death handling, earned trait evaluation, Coward flee/heal behavior, affixed loot generation, runtime affix hooks, equipped affix cache |
| **mod-nemesis-umbral-moon** | `WorldScript` (config load, update loop), `PlayerScript` (login ambient, map transfer re-apply, bonus XP), `AllCreatureScript` (per-creature stat buff/debuff, aura application), `CommandScript` (GM commands) | Scheduling, sky override, creature stat scaling, loot mode toggling, bonus loot regeneration |
| **mod-nemesis-revenge** | `WorldScript` (config load, DB init), `PlayerScript` (login title restoration) | Vengeance tracking, title grants, grudge list data |
| **mod-nemesis-bounty-board** | `WorldScript` (config load), `GameObjectScript` (gossip interaction, data queries, grudge list protocol) | Leaderboard display, NPC spawns in major cities |

### Hook Usage

The following AzerothCore script hooks are in use:

| Hook | Script Class | Usage |
|------|-------------|-------|
| `PlayerScript::OnPlayerKilledByCreature` | `NemesisElitePlayerScript` | Promotion rolls and nemesis kill logging |
| `PlayerScript::OnPlayerCreatureKill` | `NemesisElitePlayerScript` | Nemesis death detection with real killer reference; Plunderer's affix gold bonus on all creature kills |
| `PlayerScript::OnPlayerLogin` | `NemesisElitePlayerScript` | Populate equipped affix cache and apply persistent affix auras |
| `PlayerScript::OnPlayerLogout` | `NemesisElitePlayerScript` | Clear equipped affix cache |
| `PlayerScript::OnPlayerEquip` | `NemesisElitePlayerScript` | Update affix cache and refresh persistent affix auras on item equip |
| `PlayerScript::OnPlayerUnequip` | `NemesisElitePlayerScript` | Update affix cache and refresh persistent affix auras on item unequip |
| `PlayerScript::OnPlayerGiveXP` | `NemesisElitePlayerScript` | Sage suffix XP bonus |
| `PlayerScript::OnPlayerEnterCombat` | `NemesisElitePlayerScript` | Activate Umbralforged affix AoE pulse timer |
| `PlayerScript::OnPlayerLeaveCombat` | `NemesisElitePlayerScript` | Deactivate Umbralforged affix AoE pulse timer |
| `PlayerScript::OnPlayerUpdateSkill` | `NemesisElitePlayerScript` | Craft suffix bonus skillup roll on successful profession skillup |
| `AllCreatureScript::OnAllCreatureUpdate` | `NemesisEliteAllCreatureScript` | Stat re-application, aura refresh, age scaling, earned trait evaluation, Coward heal timer |
| `AllCreatureScript::OnAllCreatureUpdate` | `NemesisUmbralMoonAllCreatureScript` | Per-creature Umbral Moon buff/debuff and visual aura |
| `UnitScript::OnDamage` | `NemesisEliteUnitScript` | Pre-damage HP projection for Executioner trait; Enraged tracking; Affix proc hooks (Executioner's, Mage-Bane's, Opportunist's, Scavenger's, Duelist's damage bonuses); Blight suffix reflection |
| `UnitScript::OnHeal` | `NemesisEliteUnitScript` | Clear Executioner flag when healed above 20%; Scarred trait tracking |
| `WorldScript::OnStartup` | `NemesisEliteWorldScript` | Config load |
| `WorldScript::OnUpdate` | `NemesisEliteWorldScript` | Umbralforged affix AoE pulse update loop |
| `PlayerScript::OnPlayerLogin` | `NemesisUmbralMoonPlayerScript` | Apply ambient on login, show schedule info |
| `PlayerScript::OnPlayerMapChanged` | `NemesisUmbralMoonPlayerScript` | Re-apply ambient after map transfer |
| `PlayerScript::OnPlayerGiveXP` | `NemesisUmbralMoonPlayerScript` | Bonus XP multiplier during Umbral Moon |
| `PlayerScript::OnPlayerLogin` | `NemesisRevengePlayerScript` | Restore earned revenge titles from cached vengeance count |
| `WorldScript::OnStartup` | `NemesisRevengeWorldScript` | Load revenge config, initialize vengeance cache from DB |
| `GameObjectScript::OnGossipHello` | `NemesisBountyBoardScript` | Bounty board data query and protocol send (includes grudge list) |

### Performance Concerns

- Nemeses are persistent — they don't despawn. Need to monitor creature count in populated zones
- Threat score updates continuously (on kill events and periodic age-based ticks) — lightweight since it's simple arithmetic on cached values
- Loot generation on kill (should be fast — single DB read for nemesis traits + deterministic item generation)
- Umbral Moon world buff application to all creatures — handled per-tick via `AllCreatureScript` with a tracking set to prevent double-application; stat removal uses multiplicative inverse
- Position persistence is throttled to one DB write per minute per nemesis

### Configuration

All tunable values across the Nemesis system are stored in a single **`nemesis_configuration`** key-value table. This includes Umbral Moon scheduling, nemesis thresholds, spawn chances, trait parameters, and any other system parameters.

Umbral Moon active state is **derived from the schedule at query time** — the code checks whether the current server time falls within a scheduled window. There is no separate "is_active" flag to maintain or risk getting out of sync.

Key-value format allows adding, renaming, and removing configuration during early development without schema migrations. All reads are infrequent (player death, event boundary checks) so string parsing overhead is negligible. Can migrate to typed columns later if needed.

No module has a compile-time dependency on any other module. All inter-module communication flows through this shared table.

### AzerothCore Core Modifications

The Nemesis system requires minimal modifications to AzerothCore's core source files. These changes must be re-applied after updating to a new AC version. Keep this list current — every core modification should be documented here.

| Id | File(s) | Change | Reason |
|----|---------|--------|--------|
| 1 | `src/server/game/Loot/LootMgr.h` | Add two methods to the `public` section of `class LootStore` (before `protected:`). See snippet below. | The affixed loot system needs to register and remove private per-nemesis loot tables in the in-memory `LootStore` at runtime. `m_LootTemplates` is `private` with no public insertion or removal methods. These two methods provide controlled access without exposing the map directly. `AddLootTemplate` takes ownership of a heap-allocated `LootTemplate*`. `RemoveLootTemplate` deletes the template and erases it from the map. `GetLootForConditionFill` (already public) is used to retrieve mutable templates for adding entries at runtime. |

**Core Change Id: 1 snippet** (`LootMgr.h`, `class LootStore`, before `protected:`):
```cpp
//////////////////////////////////////////////
// Start Nemesis Core Change Id: 1
//////////////////////////////////////////////
void AddLootTemplate(uint32 lootId, LootTemplate* lootTemplate) { m_LootTemplates[lootId] = lootTemplate; }

void RemoveLootTemplate(uint32 lootId)
{
    auto itr = m_LootTemplates.find(lootId);
    if (itr != m_LootTemplates.end())
    {
        
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdelete-incomplete"
        delete itr->second;
#pragma clang diagnostic pop
        m_LootTemplates.erase(itr);
    }
}
//////////////////////////////////////////////
```

---

## Key Design Decisions Log

| Decision | Rationale |
|----------|-----------|
| No re-promotion | Promotion is a one-time birth event. Subsequent growth is continuous scaling + earned traits. Avoids complexity and keeps nemesis identity coherent. |
| Origin trait frozen at birth | Preserves the nemesis's origin story. A nemesis born from killing a healer always tells that story. Prevents trait dilution from killing one of each class. |
| One random origin trait is selected and assigned at birth | If an elite has more than one eligible trait as its origin, we will randomly pick from the available traits and lock that one in. |
| Earned traits stack freely on nemesis | Multi-trait nemeses are more dangerous to fight and have a wider suffix pool on loot drops. Complexity lives on the nemesis, not on the gear. |
| One prefix + one suffix per item | Suffix is selected from the earned trait pool. More earned traits = larger pool of possible suffixes, not more affixes on the item. Keeps gear names readable and creates a gambling element on suffix rolls. |
| One affixed item per nemesis kill | Solo players get guaranteed loot. Groups use standard Need/Greed/Pass. Creates risk/reward tradeoff between solo safety risk vs. group loot competition. Each nemesis is a one-time kill so each drop has unique provenance. |
| Standard group loot rules | No need to reinvent the wheel. Players and groups decide how loot is divided using a system they already understand. |
| Experience and history are primary threat score components | A nemesis's threat score reflects what it has done, not just what level it started at. Unique kills are weighted far higher than raw kills to discourage farming. Traits, age, and kill diversity are the real drivers. Effective level gets a nemesis in the door but experience pushes it to the top. |
| Stat offerings queue like talent points | No pressure to pick mid-combat. Players resolve pending choices at their leisure. Creates a satisfying ritual between hunts. |
| Instanced content excluded | Umbral Moon buffs and nemesis promotion only apply in open world. Instanced content has its own tuning, and nemesis persistence inside instances creates unresolvable problems (resets, despawn, ownership). Simple map type check on both systems. |
| Unified configuration table | Single `nemesis_configuration` key-value table for all system settings. Umbral Moon state derived from schedule at query time rather than maintained as a flag. No inter-module compile-time dependencies — all communication flows through the shared table. Key-value format chosen for flexibility during early development. |
| Power stack rarity is pure random | Rarity on each offering is rolled independently weighted by rarity. Threat score gates qualification but does not influence rarity rolls. Preserves roguelike feel — sometimes you get three commons, sometimes three epics. |
| Purge targets lowest threat first | Clears noise/junk nemeses while preserving the interesting high-threat content players are engaged with. Top 10 bounty board nemeses remain explicitly protected. |
| Consistent `nemesis_` table prefix | All custom tables use `nemesis_` prefix (e.g., `nemesis_elite`, `nemesis_player_stacks`, `nemesis_configuration`). "Nemesis" is the project name in code-facing identifiers; "elite" distinguishes the promoted mob entity from the system. Players call them nemeses, the code calls them elites. |
| Affixed item cloned from creature loot table | The base item comes from the nemesis's original creature's loot table, not a formula or generic pool. This keeps drops thematic — a Scarlet Crusader nemesis drops affixed Scarlet gear. The affix system is purely additive. No equippable fallback: if the creature has no equippable loot, no affixed drop is generated. |
| Item template clone at 9M + elite_id | Same offset as creature template clones in a separate table. Given an elite_id, both the creature and item clones are at the same offset in their respective tables. Item clones are never deleted — they persist as long as the item exists in the game. |
| Affixed drop is configurable chance, not guaranteed | Default 85% drop chance. Each nemesis is a one-time permanent kill so the high default respects effort, but sub-100% preserves RNG excitement and prevents flooding the economy. Tunable via `elite_affix_drop_chance`. |
| Unique kills weighted over raw kills in threat score | Raw kill count contributes ×1, unique kill count contributes ×12. Dying repeatedly to the same nemesis barely moves its threat score. Organic diverse-player encounters are the real currency. Discourages deliberate farming without adding complex anti-exploit systems. |
| Physical bounty board over guard dialogue | Schedule information and leaderboard data are accessed via interactable bounty board objects in major cities rather than guard gossip scripts. Single point of interaction for all nemesis-related information. |
| `ApplyStatPctModifier` inverse for buff removal | Because `TOTAL_PCT` is multiplicative, removing a buff requires the inverse formula `(-100 × val) / (100 + val)`, not simply negating the original value. |
| Bonus gold via `creature_template` not `ModifyMoney` | Plunderer gold is injected into the cloned `creature_template`'s `mingold`/`maxgold` so it appears in the loot window. `ModifyMoney` bypasses the loot window entirely and the player has no visibility into why they received gold. |
| Trait removal must revert all side effects | `RefreshAuras` has `!HasAura` guards that prevent re-adding but never remove. `RemoveEliteOriginTrait` and `DemoteFromElite` must explicitly strip all persistent auras and revert template modifications. Every new permanent aura is a mandatory addition to both functions. |
| Profession traits share no combat aura | The 11 profession origin traits (Skinner through Ink-Drinker) have no runtime behavior on the **nemesis creature** in `mod-nemesis-elite`. The nemesis name prefix is the only player-facing indicator. Mechanical bonuses are delivered entirely through the Affixed Loot system (System 3) — equipped affix items with profession prefixes apply `SPELL_AURA_MOD_SKILL` auras (spells 100030–100040) via `RefreshAffixAuras` on equip/unequip. |
| Custom `creature_template` entries for nameplates | Each nemesis clones its original `creature_template` into entry 9,000,000 + elite_id. `UpdateEntry` pushes the new name and level to the client. |
| Script classes inline in `.cpp` files | AzerothCore's precompiled header system requires script classes to be defined inline in `.cpp` files; splitting across headers causes `override` failures. |
| No `CMakeLists.txt` | `CollectSourceFiles`/`AddModuleSourceFiles` patterns cause CMake errors in this AC version; modules omit CMakeLists entirely. |
| `SPELL_AURA_MOD_STAT` does not affect creatures | Creatures do not use the player attribute system (STR/AGI/STA/INT/SPI are always 0). Auras that modify player stats have no effect on creature combat. Use `SPELL_AURA_MOD_DAMAGE_PERCENT_DONE` (aura 79) for creature damage and `SPELL_AURA_MOD_INCREASE_HEALTH` (aura 34) for creature HP. `SPELL_AURA_MOD_RESISTANCE` (aura 22) works for armor. `SPELL_AURA_DAMAGE_SHIELD` (aura 15) works for melee-only damage reflection. |
| `SPELL_AURA_DAMAGE_SHIELD` is melee-only | The 3.3.5 engine only triggers damage shield reflection on melee hits, not spell damage. This is a hard engine limitation. For spell reflection, server-side handling via `OnDamage` would be required. |
| Affix proc hooks fire on all damage types | `UnitScript::OnDamage` fires on melee, spells, ranged, and DoT ticks. Proc-based affixes (Mage-Bane, Opportunist, Executioner, Scavenger, Duelist) roll independently on every damage event. This makes DoT-heavy classes particularly strong with proc affixes. Accepted as a tuning consideration for later — restricting to melee-only is possible via school/attack-type checks if needed. |
| Deathblow prefix uses persistent aura, not OnDamage | `OnDamage` does not expose whether the hit was a critical strike. The correct implementation is `SPELL_AURA_MOD_CRIT_DAMAGE_BONUS` (aura 138) applied on equip, removed on unequip, aggregated across items via `RefreshAffixAuras`. |
| RefreshAffixAuras strips-and-reapplies pattern | Same-caster same-spell auras don't stack in WoW — they refresh. Multiple equipped items with the same affix type must aggregate their values into a single aura application. `RefreshAffixAuras` removes all affix auras, iterates the cache, sums per-type, and reapplies each with the total value. Called on login, equip, and unequip. |
| Umbralforged affix uses combat enter/exit + WorldScript update loop | The player's AoE shadow pulse needs to fire every 3 seconds while in combat. `OnPlayerEnterCombat` registers the player in a map with aggregated damage and a timer. `WorldScript::OnUpdate` iterates the map and casts the spell when the timer elapses. `OnPlayerLeaveCombat` removes the player. Safety cleanup in the update loop handles disconnects and edge cases (player dead, no longer in combat). |
| Deathblow Blue tier rounded from 3.5% to 4% | `CastCustomSpell` takes `int32` BasePoints. 3.5 truncates to 3, losing half a percent vs design intent. Rounding up to 4 is closer to the design's intended power curve than rounding down to 3. |
| Blight reflection uses spell 26364 (Damage Shield) | Existing game spell used as a combat-log carrier for the thorns effect. `CastCustomSpell` passes the damage value. This makes the reflection appear naturally in the combat log without needing a custom DBC spell for a visual-only purpose. |
| Plunderer's affix gold uses ModifyMoney, not creature_template | Unlike the nemesis creature's Plunderer trait (which injects gold into `mingold`/`maxgold` so it appears in the loot window), the player's Plunderer prefix affix uses `ModifyMoney` directly. The bonus gold from equipped gear is a percentage of the killed creature's base gold, applied after the kill — it would be incorrect to modify every creature's loot table based on the player's gear. |
| Umbralforged affix radius unified to 10yd | Design doc originally specified 8yd for Green/Blue and 10yd for Purple/Legendary. Unified to 10yd for all tiers to avoid needing two DBC spells for a 2-yard difference. Single spell (100041, Umbral Echo) handles all tiers. |
| `OnPlayerEnterCombat` takes two parameters | AzerothCore's `PlayerScript::OnPlayerEnterCombat` signature includes `Unit* enemy` as a second parameter. The Umbralforged activation hook ignores the enemy parameter since it applies regardless of who the player is fighting. Same applies to `OnPlayerLeaveCombat`. |
| `OnDamage` damage modifications are invisible to combat log | `SendAttackStateUpdate` fires before `DealDamage` in the engine. Damage bonuses applied in `UnitScript::OnDamage` affect actual HP reduction but never appear in floating combat text or combat log. This affects Executioner's, Scavenger's, and Duelist's prefix affixes. DBC auras are the correct approach for visible damage modification, but the current implementation prioritizes simplicity. |

---

## Implementation Patterns

These patterns have been established through trial and error during development. They should be followed consistently for all new trait behaviors, auras, and combat mechanics.

### Aura Categories

There are three distinct categories of auras in the nemesis system, each with different lifecycle rules:

**Permanent passives** (Underdog, Ironbreaker): Applied in `RefreshAuras` with a `!HasAura` guard. Never removed during normal operation — only stripped on demotion (`DemoteFromElite`) or trait removal (`RemoveEliteOriginTrait`/`RemoveEliteEarnedTrait`). DBC values are baked in (no runtime `ChangeAmount`). These use the same simple `AddAura` one-liner pattern.

**Permanent scaled passives** (Scavenger, Duelist, Notorious, Survivor, Blight): Applied in `RefreshAuras` like permanent passives, but the DBC BasePoints are irrelevant — `ChangeAmount` overrides them at runtime with a level-scaled value computed from `base + (effectiveLevel × perLevel)`. These naturally refresh on level-up because the re-buff cycle (`_buffedElites` erasure → `ApplyStats` → `UpdateEntry` strips all auras → `RefreshAuras`) recomputes the amount. Multi-effect spells (Notorious has damage% + HP, Survivor has armor + crit reduction) call `ChangeAmount` on each effect index independently.

**Conditional combat auras** (Executioner, Giant Slayer, Scavenger, Duelist, Umbral Burst): Toggled every tick inside the `IsInCombat()` block of `OnCreatureUpdate`. Applied when a condition is met (victim HP below threshold, level gap, debuff present, etc.), removed when the condition is no longer met. Always stripped in the combat-exit cleanup block. Scavenger and Duelist appear in both categories — they are toggled per-tick during combat but use `ChangeAmount` for level scaling.

### Trait Removal Checklist

When removing a trait (via GM command or future systems), all side effects of that trait must be reverted. This is easy to miss because `RefreshAuras` has `!HasAura` guards that prevent re-adding but never actively remove. The following must be handled:

- **Persistent auras**: `RemoveEliteOriginTrait` must explicitly strip the trait's aura from the creature. Currently handles Underdog and Ironbreaker. `RemoveEliteEarnedTrait` handles Notorious, Survivor, and Blight.
- **Template modifications**: `RemoveEliteOriginTrait` must revert any changes to the cloned `creature_template`. Currently handles Plunderer's `mingold`/`maxgold` reversion.
- **Demotion**: `DemoteFromElite` must strip ALL trait auras (permanent and conditional) before `UpdateEntry`. Currently strips: Grow, Umbral Burst, Underdog, Ironbreaker, Executioner, Giant Slayer, Scavenger, Duelist, Notorious, Survivor, Blight.

**Every new permanent aura added to `RefreshAuras` must also be added to `DemoteFromElite` and the appropriate trait removal function (`RemoveEliteOriginTrait` or `RemoveEliteEarnedTrait`).** This is a mandatory checklist item.

### Active Spells vs. Passive Auras

If a trait's behavior involves a spell that is **actively cast** (has a cast time, triggers the spell pipeline, deals damage, heals, or has a periodic trigger), it requires a **SpellScript** in its own `.cpp` file. Examples: Deathblow Strike (100010), Coward Heal (100001), Umbral Burst (100009).

If a trait's behavior is a **passive stat modifier** (applied via `AddAura`, no cast, no periodic effect), it does NOT need a SpellScript. The DBC entry handles it. Examples: Underdog (100011), Ironbreaker (100014), Executioner (100003), Notorious (100016), Survivor (100015), Blight (100017).

### Loot and Gold via creature_template

Bonus gold rewards must be injected into the cloned `creature_template`'s `mingold`/`maxgold` fields so the gold appears in the creature's loot window. Do NOT use `ModifyMoney` to send gold directly to the player's inventory — this bypasses the loot window entirely and players won't know where the gold came from. The Plunderer trait is the reference implementation for this pattern:

1. Read the base creature's `mingold`/`maxgold` from `sObjectMgr->GetCreatureTemplate(creatureEntry)`.
2. Add the bonus amount to both values.
3. Write the new values to the cloned `creature_template` row in the DB.
4. Update the in-memory `CreatureTemplate` cache via `const_cast`.
5. On trait removal, revert to the base template's original values.

This is idempotent because it always computes from base + bonus, never stacking on top of a previously modified value.

### Custom Spell ID Allocation

All custom spells use IDs in the 100000+ range. Current allocation:

| ID | Name | Type | Trait/System |
|----|------|------|-------|
| 100000 | Umbral Moon Creature Visual | Visual aura | Umbral Moon system |
| 100001 | Coward's Heal | Active heal (SpellScript) | Coward |
| 100002 | Coward's Retreat | Stealth aura | Coward |
| 100003 | Executioner's Focus | Conditional damage buff | Executioner |
| 100004 | Mage-Bane Silence | Active silence | Mage-Bane |
| 100005 | Healer-Bane Wound | Healing reduction debuff | Healer-Bane |
| 100006 | Giant Slayer's Fury | Conditional damage/DR buff | Giant-Slayer |
| 100007 | Opportunist's Cheap Shot | Active stun | Opportunist |
| 100008 | Umbral Resonance | Periodic trigger aura | Umbralforged |
| 100009 | Umbral Burst | Triggered AoE damage (SpellScript) | Umbralforged |
| 100010 | Deathblow Strike | Active heavy strike (SpellScript) | Deathblow |
| 100011 | Underdog's Tenacity | Permanent expertise buff | Underdog |
| 100012 | Scavenger's Instinct | Conditional damage buff | Scavenger |
| 100013 | Duelist's Resolve | Conditional damage buff | Duelist |
| 100014 | Sundered Armor | Permanent armor pen buff | Ironbreaker |
| 100015 | Battle-Hardened | Permanent scaled armor + crit reduction buff | Survivor |
| 100016 | Dread Renown | Permanent scaled damage% + flat HP buff | Notorious |
| 100017 | Blighted Aura | Permanent scaled shadow damage shield | Blight |
| 100018 | Spellwarded | Permanent CC duration reduction | Spellproof |
| 100019 | Scarred Presence | Periodic healing reduction debuff | Scarred |
| 100020 | Enraged Fury | Conditional multi-attacker damage buff | Enraged |
| 100021 | Sun-Tempered | Conditional daytime damage buff | Dayborn |
| 100022 | Moon-Tempered | Conditional nighttime damage buff | Nightborn |
| 100023 | Nomad's Stride | Permanent movement speed buff | Nomad |
| 100024 | Mage-Bane's Curse | Affix: cast time slow debuff on enemy | Affix (Mage-Bane prefix) |
| 100025 | Nemesis Daze | Affix: daze/slow debuff on enemy | Affix (Opportunist prefix) |
| 100026 | Nemesis Precision | Affix: crit damage bonus aura on player | Affix (Deathblow prefix) |
| 100027 | Wanderer's Grace | Affix: movement speed aura on player | Affix (of Wandering suffix) |
| 100028 | Cowering Presence | Affix: threat reduction aura on player | Affix (of Cowardice suffix) |
| 100029 | Commanding Presence | Affix: threat increase aura on player | Affix (of Dominion suffix) |
| 100030 | Nemesis Skinning | Affix: profession skill aura on player | Affix (Skinner prefix) |
| 100031 | Nemesis Mining | Affix: profession skill aura on player | Affix (Ore-Gorged prefix) |
| 100032 | Nemesis Herbalism | Affix: profession skill aura on player | Affix (Root-Ripper prefix) |
| 100033 | Nemesis Blacksmithing | Affix: profession skill aura on player | Affix (Forge-Breaker prefix) |
| 100034 | Nemesis Leatherworking | Affix: profession skill aura on player | Affix (Hide-Mangler prefix) |
| 100035 | Nemesis Tailoring | Affix: profession skill aura on player | Affix (Thread-Ripper prefix) |
| 100036 | Nemesis Engineering | Affix: profession skill aura on player | Affix (Gear-Grinder prefix) |
| 100037 | Nemesis Alchemy | Affix: profession skill aura on player | Affix (Vial-Shatter prefix) |
| 100038 | Nemesis Enchanting | Affix: profession skill aura on player | Affix (Rune-Eater prefix) |
| 100039 | Nemesis Jewelcrafting | Affix: profession skill aura on player | Affix (Gem-Crusher prefix) |
| 100040 | Nemesis Inscription | Affix: profession skill aura on player | Affix (Ink-Drinker prefix) |
| 100041 | Umbral Echo | Affix: periodic AoE shadow damage from player (spell power scales) | Affix (Umbralforged prefix) |
| 100042 | Blighted Reflection | Affix: shadow damage reflected to melee attackers (spell power scales) | Affix (of Blight suffix) |

Next available ID: **100043**.

---

## Work Remaining

### System 4: Nemesis Power Stacks — Not Started

The entire power stacks system is unbuilt. This is the foundational blocker for several other features. Includes:

- **`nemesis_player_stacks` table:** Schema design for per-player stack persistence, per-stack stat/rarity detail.
- **Minimum threat score threshold:** Gate qualifying kills (config key `power_stack_minimum_threat_score`).
- **Stat buff offering generation:** Three random offerings per qualifying kill, each with independent rarity roll (common/uncommon/rare/epic, weighted). Stat pool: SP, AP, Armor, Expertise, Crit Rating, Hit Rating, etc.
- **Rarity scaling:** 1×, 1.5×, 2.5×, 4× base value for common through epic.
- **Queueing system:** Offerings queue like talent points; player resolves at will.
- **UI element:** Display pending offerings count, open selection UI, option to dismiss/defer.
- **Diminishing returns curve:** Each successive stack grants less benefit. Formula TBD — logarithmic, inverse, or flat tier reduction.
- **Death penalty:** Dying wipes ALL stacks. Hook into `OnPlayerKilledByCreature` to clear stacks on death.
- **High-stack player death announcements:** Server-wide message when a high-stack player dies.

### Purge Events — Not Started

The purge system is unbuilt:

- **Oversaturation threshold:** Config key for max living nemesis count.
- **Pruning query:** Delete lowest-threat-score nemeses down to configured percentage of threshold. Top-10 bounty board nemeses are immune.
- **Scheduling:** Purge can only fire once per week, Sundays at 12AM server time.
- **Narrative announcement:** Optional server-wide message (e.g., "The Argent Crusade has culled the growing darkness in the realm").
- **Implementation location:** This is a standalone system, not part of the bounty board module. Could be a `WorldScript` with a periodic check or a new module.

