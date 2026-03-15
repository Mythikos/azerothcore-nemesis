# Nemesis

**Revenge fantasy meets roguelike meets WoW.**

Nemesis is a custom gameplay layer for World of Warcraft 3.3.5a built on [AzerothCore](https://www.azerothcore.org/). When a creature kills a player in the open world, it has a chance to be promoted into a persistent, named elite that grows stronger over time, develops unique combat behaviors, drops procedurally generated loot, and appears on a server-wide bounty board. Designed for up to 2,000 concurrent players.

> **This project is archived.** It is published as a portfolio piece and community reference. No issues, pull requests, or support will be provided.

---

## The Core Loop

```
Player dies in the open world
  → Creature rolls for nemesis promotion
  → Promoted into a persistent, named elite
  → Gains an origin trait based on how it killed the player
  → Levels up with each subsequent player kill
  → Ages passively, growing more dangerous over time
  → Earns new combat traits from its experiences
  → Appears on the Bounty Board in capital cities
  → When finally killed, drops procedurally affixed gear shaped by its life
  → Cycle repeats
```

A Kobold Vermin that catches a distracted mage can eventually become a level 83 "Executioner Kobold Vermin" with a body count, a dozen combat traits, and gear drops that tell the story of everything it survived.

---

## Systems

### Elite - The Core

When a creature kills a player in the open world, a configurable chance roll (7% normal, 25% during Umbral Moon) determines whether it becomes a Nemesis Elite - a persistent, evolving named mob that doesn't despawn, doesn't reset, and doesn't forget.

**On promotion, a nemesis receives:**
- A unique name derived from its origin trait (e.g., "Lurking Defias Pillager", "Umbra-Forged Mana Wyrm")
- A cloned `creature_template` entry for client nameplate display
- Bonus HP, damage, and an effective level boost
- An origin trait determined by the circumstances of the kill
- A threat score that tracks how dangerous it has become

**21 origin traits** detect context from the killing blow: Ambusher (player was already in combat), Executioner (player was below 20% HP), Deathblow (one-shot kill), Mage-Bane (killed a caster), Ironbreaker (killed plate armor), Umbralforged (promoted during an Umbral Moon event), and 11 profession-based traits that feed into the affix loot system, among others.

**13 earned traits** accumulate over a nemesis's lifetime as it survives encounters and kills more players: Coward (flees and heals when low), Notorious (killed 5+ unique players), Territorial (aged into permanent aggression), Blight (killed both factions), Enraged (survived being zerged), and more. A long-lived nemesis can stack many of these simultaneously, making it progressively harder to take down.

**Scaling:** Nemeses gain effective levels from kills (+1 per kill) and passive aging (+1 per configurable interval), up to a hard cap of 83. Their threat score - a composite of level, kill count, unique kills, trait count, and age - determines loot quality, bounty board ranking, and overall notoriety.

**Persistence:** Nemeses never despawn. Position is persisted on a throttled interval. Death is permanent - when killed, the cloned template is cleaned up and the nemesis is marked dead in the database.

---

### Affixed Loot

When a nemesis is killed, it has a chance (default 85%) to drop a single piece of procedurally generated gear. The item is cloned from the creature's original loot table, so a Scarlet Crusader nemesis drops affixed Scarlet gear, not random boots from nowhere.

**Every affix tells the nemesis's story.** The prefix comes from the origin trait (how the nemesis was born), and the suffix comes from the earned trait pool (what it became). An "Executioner's Scarlet Boots of Dominion" came from a nemesis born by finishing off a low-HP player (Executioner) that later became a territorial threat (Territorial → "of Dominion").

**24 prefixes × 13 suffixes = 312 unique affix combinations** at Blue quality and above, plus 37 single-affix variations at Green. Each affix scales across four quality tiers (Uncommon → Rare → Epic → Legendary), gated by threat score thresholds. Legendary quality additionally requires placement in the Bounty Board top 10.

**15 of the 37 affixes are server-side** - proc-based damage, threat modification, shadow reflection, bonus gold on kills, bonus XP, profession skill boosts, and movement speed. These are powered by an equipped-item affix cache that tracks which affixed items a player has equipped and applies the appropriate runtime hooks.

All affixed items are Bind on Equip, enabling a player-driven economy around rare combinations.

---

### Umbral Moon

A periodic server-wide event that spikes difficulty and seeds the world with nemeses. Runs on a fixed schedule (default: Wednesday and Saturday, morning and evening windows, 2 hours each).

During an Umbral Moon:
- The sky transitions to forced nighttime via `SMSG_LOGIN_SETTIMESPEED`, with smooth dusk/dawn interpolation
- All open-world creatures receive a visual aura and stat buffs (default 2× HP, 2× damage)
- Bonus XP multiplier applies to open-world kills
- Enhanced loot entries activate via a dedicated LootMode bitmask
- Nemesis promotion chance jumps to 25%

The event is the "planting time bombs" phase. The days between Umbral Moons are when those time bombs go off.

---

### Revenge

When a player kills a nemesis that was born from their own death, they earn a vengeance point. Vengeance is purely social prestige - no bonus loot, no mechanical advantage, just permanent titles:

| Vengeance Kills | Title |
|-----------------|-------|
| 1 | the Wronged |
| 5 | the Grudgekeeper |
| 15 | the Relentless |
| 25 | the Vengeful |
| 50 | the Unforgiving |
| 100 | the Inevitable |

The bounty board also includes a personal grudge list showing all living nemeses born from the viewing player's deaths - where they are, how strong they've become, and how many times they've killed you since.

---

### Bounty Board

A physical interactable object (the Garadar Bulletin Board model) spawned in all 10 major cities. Displays the top 10 most dangerous living nemeses by threat score, including their traits, location, age, kill count, and the name of the player whose death created them. Also shows the next scheduled Umbral Moon event.

Data is sent to a companion client addon (`Nemesis_BountyBoard`) via a structured protocol over `SendSysMessage` using the `@@NBB@@` prefix and `~` delimiter.

---

## Repository Structure

```
Nemesis WoW/
├── modules/                    AzerothCore C++ server modules
│   └── mod-nemesis/            Elite, Umbral Moon, Revenge, Bounty Board
├── addons/                     WoW client Lua addons
│   └── Nemesis_BountyBoard/    In-game bounty board for tracking top threats and grudges.
├── tools/                      DBC pipeline, deploy scripts, spell editor DB
├── launcher/                   Game launcher (C# / .NET MAUI)
└── Design Document.md          Full specification for all systems
```

The `server/` directory (AzerothCore itself) is not included - it has its own Git history. The module source in `modules/mod-nemesis/` is copied over to `server/modules/mod-nemesis/` at build time.

---

## Technical Highlights

A few things that might be interesting if you're building AzerothCore modules:

- **`creature_template` cloning** for dynamic nameplate display - each nemesis gets a unique entry in the 9,000,000+ range with `UpdateEntry` pushing name/level changes to the client.
- **`item_template` cloning** for procedural loot - same pattern, same ID range, different table. Affixed items persist indefinitely across inventories, mail, and the auction house. This would not have been possible without the [WoWPatcher335_r001](https://github.com/anzz1/WoWPatcher335). Without it, items that exceed a certain entry range become classless, lose their textures due to mapping issues with the cache, and stop functioning as intended. 
- **Custom DBC spell pipeline** - 42 custom spells authored in Stoneharry's Spell Editor, packaged via a PowerShell pipeline (`Deploy-SpellPatch.ps1`) that generates SQL via `node-dbc-reader`, builds client MPQ patches, and deploys to both Docker and the WoW client in one step.
- **`SMSG_LOGIN_SETTIMESPEED`** for client-side sky manipulation - `SMSG_OVERRIDE_LIGHT` is non-functional in 3.3.5; time-speed packets with periodic resync are the correct approach.
- **Multiplicative stat removal** - `ApplyStatPctModifier(TOTAL_PCT)` is multiplicative, not additive. Removal requires the inverse formula `(-100 × val) / (100 + val)`, not the negated original value.
- **`AddAura` vs `CastSpell`** - `AddAura` skips periodic timer initialization. Any spell using `SPELL_AURA_PERIODIC_TRIGGER_SPELL` must use `CastSpell` with the triggered flag.
- **Runtime loot injection** - AzerothCore loads `creature_loot_template` into memory before module scripts fire. Inserted rows are invisible until `LoadLootTemplates_Creature()` is called, or you inject directly into the creature's loot object.

---

## DBC / Client Patch

This project was developed against a clean 3.3.5a (12340) client sourced from [this gist](https://gist.github.com/devovh/803c1054e6b04ac2059a8884320c3dd2). It does **not** include the client files or the modified DBC files. These are derived from Blizzard's proprietary data and are not distributed here.

The `tools/` directory contains the deployment pipeline and documentation of all custom spell configurations. Anyone with access to [stoneharry's WoW Spell Editor](https://github.com/stoneharry/WoW-Spell-Editor) and the standard 3.3.5a DBC files can recreate the entries. The full spell allocation table is in the [Design Document](Design%20Document.md).

The other DBCs requiring a patch is the CharTitles.dbc for use with the revenge system. Details on the 6 titles and their configurations can be found in the [Design Document](Design%20Document.md).

A client addon (`Nemesis_BountyBoard`) is included and requires no DBC modifications - it communicates with the server module via chat message parsing.

---

## Configuration

All tunable values are stored in the `nemesis_configuration` database table in key/value pairs. Nothing gameplay-relevant is hardcoded. Key names follow `{system}_{parameter}` convention (e.g., `elite_promotion_chance_normal`, `umbral_moon_mob_hp_boost_percent`, `bounty_board_top_count`). The SQL files in the module source contain the full default configuration.

---

## What Was Left on the Table

Two systems from the original design were never implemented but are fully specced in the [Design Document](Design%20Document.md):

- **Power Stacks** - A permanent stat progression system where killing high-threat nemeses grants stacking stat buffs with roguelike-style rarity rolls (three random offerings per kill, common through epic). Dying wipes all stacks. This was the intended risk/reward endgame loop tying long-term player investment to the nemesis system.
- **Purge Events** - A weekly culling mechanic to prevent world oversaturation. Lowest-threat nemeses get pruned while Bounty Board top 10 entries stay immune, paired with a narrative server announcement.

Both are interesting problems if anyone wants to pick them up.

---

## License

[AGPL-3.0](LICENSE), consistent with AzerothCore's license.
