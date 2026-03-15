-- =============================================================================
-- nemesis_configuration
-- Shared key-value config table used by all nemesis modules.
-- Created here if it doesn't exist; other modules can safely re-run this.
-- =============================================================================
CREATE TABLE IF NOT EXISTS `nemesis_configuration` (
    `config_key` VARCHAR(128) NOT NULL,
    `config_value` VARCHAR(4098) NOT NULL DEFAULT '',
    `description` VARCHAR(512) NOT NULL DEFAULT '',
    PRIMARY KEY (`config_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =============================================================================
-- Promotion & Scaling
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_promotion_chance_normal', '7.0', 'Chance for a creature to be promoted to elite.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_promotion_chance_umbral_moon', '25.0', 'Chance for a creature to be promoted to elite during an Umbral Moon.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_stat_hp_boost_percent', '25.0', 'Percent HP increase applied when a creature is promoted to elite.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_stat_damage_boost_percent', '25.0', 'Percent damage increase applied when a creature is promoted to elite.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_level_gain_on_promote', '3', 'How many effective levels a nemesis gains when they promote for the first time.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_level_gain_per_kill', '1', 'How many effective levels a nemesis gains each time it kills a player.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_level_cap', '83', 'Maximum effective level a nemesis can reach (83 = hard raid boss tier).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_age_level_interval_hours', '24', 'Hours a nemesis must survive before gaining one passive age-based effective level (one level per day default).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_visual_grow_levels_per_stack', '10', 'Number of effective levels per visual grow stack (e.g. 10 = one grow stack at level 10, two at 20, etc).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Threat Score
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_threat_weight_level', '5', 'Threat score points per effective level.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_threat_weight_kill', '1', 'Threat score points per total player kill (counts repeated kills of the same player). Weighted low to discourage farming.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_threat_weight_unique_kill', '12', 'Threat score points per unique player killed. Unique kills are weighted far higher than raw kill count to discourage farming (dying repeatedly to the same nemesis) and reward organic diverse-player kills.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_threat_weight_trait', '15', 'Threat score points per trait (origin + earned).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_threat_age_bonus_per_day', '4', 'Threat score points gained per day alive (capped by elite_threat_age_bonus_cap).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_threat_age_bonus_cap', '100', 'Maximum threat score bonus from age (days alive).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Origin Trait: Ambusher
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_ambusher_spawn_radius', '50', 'Range in yards that an Ambusher nemesis spawns adds.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_ambusher_pull_interval_ms', '5000', 'Milliseconds between each wave of adds pulled by an Ambusher nemesis during combat.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_ambusher_adds_per_pull', '2', 'Number of nearby creatures an Ambusher nemesis attempts to pull per wave.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_ambusher_max_adds_per_encounter', '8', 'Maximum total adds an Ambusher nemesis can pull during a single combat encounter.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Origin Trait: Deathblow
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_deathblow_hp_threshold', '25', 'HP percentage below which a Deathblow nemesis begins casting its heavy strike (0-100).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_deathblow_base_damage', '200', 'Base damage of the Deathblow Strike spell before level scaling is applied.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_deathblow_damage_per_level', '50', 'Bonus damage per effective level added to the Deathblow Strike base damage.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_deathblow_rearm_cooldown_ms', '10000', 'Milliseconds after being healed above the HP threshold before a Deathblow nemesis can cast again on the next threshold crossing.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Origin Trait: Giant Slayer
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_giant_slayer_level_gap', '10', 'Level gap (player level minus creature level) required for a creature to earn the Giant Slayer origin trait.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_giant_slayer_aura_level_gap', '5', 'Level gap (player level minus nemesis level) at which the Giant Slayer combat aura activates, granting bonus damage and HP against overleveled opponents.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Origin Trait: Underdog
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_underdog_level_gap', '3', 'Level gap (player level minus creature level) required for a creature to earn the Underdog origin trait (must be below Giant Slayer threshold).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Origin Trait: Scavenger
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_scavenger_damage_base_percent', '6', 'Base percent bonus damage a Scavenger nemesis deals while its current victim has at least one debuff.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_scavenger_damage_per_level_percent', '0.15', 'Additional percent bonus damage per effective level added to the Scavenger base. Total = base + (effective_level * per_level).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Origin Trait: Duelist
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_duelist_damage_base_percent', '8', 'Base percent bonus damage a Duelist nemesis deals while exactly one player is in combat with it.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_duelist_damage_per_level_percent', '0.25', 'Additional percent bonus damage per effective level added to the Duelist base. Total = base + (effective_level * per_level).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Origin Trait: Plunderer
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_plunderer_gold_per_level', '10000', 'Copper per player level the victim must be carrying for the Plunderer origin trait (10000 = 1 gold per level).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_plunderer_bonus_gold_base', '5000', 'Base bonus copper awarded to the killer when a Plunderer nemesis is slain (5000 = 50 silver).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_plunderer_bonus_gold_per_level', '1000', 'Bonus copper per effective level added to the Plunderer gold reward (1000 = 10 silver per level).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Origin Trait: Opportunist
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_opportunist_cheap_shot_interval_ms', '15000', 'Milliseconds between Opportunist cheap shot stuns during combat. Each cast stuns the current melee victim for the DBC spell duration.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Origin Trait: Mage Bane
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_mage_bane_silence_interval_ms', '10000', 'Milliseconds between Mage Bane silence casts during combat. Each cast targets a player caught mid-cast within range.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_mage_bane_silence_range', '30.0', 'Range in yards that a Mage Bane nemesis scans for casting players to silence.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Origin Trait: Healer Bane
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_healer_bane_reapply_interval_ms', '12000', 'Milliseconds between Healer Bane debuff applications during combat. Gives players a window to dispel and benefit from full healing before the next application.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Notorious
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_notorious_kill_threshold', '5', 'Number of unique players a nemesis must kill to earn the Notorious earned trait.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Territorial
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_territorial_days_threshold', '3', 'Number of days a nemesis must remain alive to earn the Territorial earned trait.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_territorial_wander_level_multiplier', '2.0', 'Multiplier applied to effective level to compute Territorial wander radius in yards (e.g. level 40 * 2.0 = 80 yard radius).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_territorial_wander_min_radius', '20.0', 'Minimum wander radius in yards for Territorial nemeses, regardless of effective level.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_territorial_detection_multiplier', '1.5', 'Multiplier applied to a Territorial nemesis detection range (e.g. 1.5 = 50% larger aggro radius).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Survivor
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_survivor_attempts_threshold', '3', 'Number of player combat encounters a nemesis must survive (evade/reset while alive) to earn the Survivor earned trait.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Coward
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_coward_hp_threshold', '15.0', 'HP percent below which a nemesis may flee and self-heal.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_coward_flee_chance', '30.0', 'Percent chance that a nemesis below the Coward HP threshold will flee.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_coward_flee_duration_ms', '5000', 'Duration in milliseconds that a nemesis flees when the Coward flee triggers.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_coward_heal_base', '100', 'Base heal amount of the Coward Heal spell before level scaling is applied.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_coward_heal_per_level', '25', 'Bonus heal per effective level added to the Coward Heal base amount.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Enraged
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_enraged_player_threshold', '5', 'Number of players required to be attacking the nemesis before it gains the Enraged trait.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Executioner
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_executioner_hp_threshold', '20.0', 'HP percent at or below which the player must be when a creature enters combat for Executioner trait eligibility.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_executioner_aura_hp_threshold', '20.0', 'HP percent at or below which the Executioner damage bonus aura activates against the current victim during combat.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Dayborn / Nightborn
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_dayborn_hour_start', '6', 'Server-local hour (0-23) at which daytime begins for Dayborn/Nightborn trait evaluation.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_dayborn_hour_end', '17', 'Server-local hour (0-23) at which daytime ends (inclusive) for Dayborn/Nightborn trait evaluation.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Sage
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_sage_days_threshold', '7', 'Number of days alive required for a nemesis to earn the Sage trait, which grants increased effective level growth over time.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Studious
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_studious_search_range', '30', 'Range in yards that a nemesis will detect chest type objects for the Studious trait.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Umbral Burst (Umbral Moon combat aura)
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_umbral_burst_recast_interval_ms', '10000', 'Milliseconds before the Umbral Burst aura can be re-applied after being interrupted or removed during combat.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_umbral_burst_base', '50', 'Base shadow damage per pulse of the Umbral Burst before level scaling is applied.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_umbral_burst_per_level', '15', 'Bonus shadow damage per effective level added to each Umbral Burst pulse.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Notorious
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_notorious_damage_base_percent', '5', 'Base damage percent bonus granted by the Notorious trait (Dread Renown aura) before level scaling.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_notorious_damage_per_level_percent', '0.15', 'Bonus damage percent per effective level added to the Notorious trait (Dread Renown aura).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_notorious_health_base', '50', 'Base flat max HP bonus granted by the Notorious trait (Dread Renown aura) before level scaling.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_notorious_health_per_level', '10', 'Bonus flat max HP per effective level added to the Notorious trait (Dread Renown aura).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Survivor
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_survivor_armor_base', '100', 'Base bonus armor granted by the Survivor trait (Battle-Hardened aura) before level scaling.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_survivor_armor_per_level', '5', 'Bonus armor per effective level added to the Survivor trait (Battle-Hardened aura).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_survivor_crit_reduction_base', '5', 'Base crit damage reduction percent granted by the Survivor trait (Battle-Hardened aura) before level scaling.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_survivor_crit_reduction_per_level', '0.15', 'Crit damage reduction percent per effective level added to the Survivor trait (Battle-Hardened aura).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Blight
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_blight_damage_base', '10', 'Base shadow damage dealt to melee attackers by the Blight trait (Blighted Aura damage shield) before level scaling.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_blight_damage_per_level', '3', 'Bonus shadow damage per effective level added to the Blight trait (Blighted Aura damage shield).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Spellproof
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_spellproof_cc_duration_reduction', '50', 'Percent reduction in Stun, Polymorph, and Fear duration on a Spellproof nemesis (0-100). Applied as a negative MECHANIC_DURATION_MOD.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Scarred
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_scarred_healing_reduction_percent', '30', 'Percent healing reduction applied to the current melee victim of a Scarred nemesis (0-100).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_scarred_reapply_interval_ms', '12000', 'Milliseconds between Scarred healing reduction debuff applications during combat. Same pattern as Healer-Bane.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Enraged
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_enraged_damage_base_percent', '3', 'Base percent bonus damage an Enraged nemesis deals while 2+ players are in combat.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_enraged_damage_per_level_percent', '0.10', 'Additional percent bonus damage per effective level for the Enraged trait.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_enraged_damage_per_player_percent', '4', 'Additional percent bonus damage per player beyond the first on the Enraged nemesis threat list.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Dayborn
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_dayborn_damage_base_percent', '5', 'Base percent bonus damage a Dayborn nemesis deals during daytime hours (within dayborn_hour_start to dayborn_hour_end).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_dayborn_damage_per_level_percent', '0.15', 'Additional percent bonus damage per effective level for the Dayborn trait during daytime.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Nightborn
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_nightborn_damage_base_percent', '5', 'Base percent bonus damage a Nightborn nemesis deals during nighttime hours (outside dayborn_hour_start to dayborn_hour_end).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_nightborn_damage_per_level_percent', '0.15', 'Additional percent bonus damage per effective level for the Nightborn trait during nighttime.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Earned Trait: Nomad
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_nomad_speed_percent', '100', 'Percent movement speed increase granted to a Nomad nemesis (e.g. 30 = 30% faster).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_trait_nomad_area_threshold', '3', 'Number of distinct areas a nemesis must visit to earn the Nomad trait.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Affixed Loot System
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_drop_chance', '85', 'Percent chance (0-100) that a nemesis drops an affixed item on death. Each nemesis is a one-time permanent kill.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_tier_threshold_blue', '300', 'Minimum threat score for Blue (Rare) quality affixed drops. Below this = Green.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_tier_threshold_purple', '600', 'Minimum threat score for Purple (Epic) quality affixed drops.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_tier_threshold_legendary', '700', 'Minimum threat score floor for Legendary quality affixed drops. Also requires Bounty Board top-N placement.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_legendary_top_count', '10', 'Number of top nemeses by threat score that qualify for Legendary tier (must ALSO meet the score floor).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_item_flavor_templates', 'Taken from the corpse of {name}.;Looted from {name}.;Pried from the remains of {name}.;Stripped from {name} after its final defeat.;Once wielded by {name}.', 'Semicolon-delimited flavor text templates for affixed items. {name} is replaced with the nemesis name.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Affix Tier-Scaled Values (comma-delimited: green,blue,purple,legendary)
-- =============================================================================

-- Prefix item stat injection
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_ambusher_crit_rating', '15,26,38,52', 'Crit Rating bonus per quality tier (green,blue,purple,legendary) for Ambusher prefix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_healer_bane_spell_power', '20,35,50,70', 'Spell Power bonus per quality tier for Healer-Bane prefix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_giant_slayer_attack_power', '20,35,50,70', 'Attack Power bonus per quality tier for Giant Slayer prefix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_underdog_expertise', '10,18,26,36', 'Expertise Rating bonus per quality tier for Underdog prefix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_ironbreaker_armor_pen', '15,26,38,52', 'Armor Penetration Rating bonus per quality tier for Ironbreaker prefix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- Suffix item stat injection
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_notorious_all_stats', '4,7,10,14', 'All-stats (STR/AGI/STA/INT/SPI) bonus per quality tier for Notorious suffix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_survivor_armor', '80,140,200,280', 'Bonus armor per quality tier for Survivor suffix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_spellproof_resilience', '15,26,38,52', 'Resilience Rating bonus per quality tier for Spellproof suffix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_scarred_mp5', '6,10,15,21', 'MP5 bonus per quality tier for Scarred suffix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_enraged_haste', '15,26,38,52', 'Haste Rating bonus per quality tier for Enraged suffix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_dayborn_attack_power', '15,26,38,52', 'Attack Power bonus per quality tier for Dayborn suffix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_nightborn_agility', '12,21,30,42', 'Agility bonus per quality tier for Nightborn suffix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- Equipped aura values
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_deathblow_crit_percent', '2,4,5,7', 'Crit damage bonus percent per quality tier for equipped Deathblow prefix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_wandering_speed_percent', '3,5,7,8', 'Movement speed bonus percent per quality tier for equipped Wandering suffix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_cowardice_threat_percent', '3,5,8,12', 'Threat reduction percent per quality tier for equipped Cowardice suffix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_dominion_threat_percent', '3,5,8,12', 'Threat increase percent per quality tier for equipped Dominion suffix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_profession_skill_bonus', '100,100,100,100', 'Profession skill bonus per quality tier for equipped profession prefix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- Proc values (runtime hooks)
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_executioner_proc_chance', '3,5,7,10', 'Proc chance percent per quality tier for Executioner prefix on-hit effect.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_executioner_proc_damage', '40,70,100,140', 'Flat damage per quality tier for Executioner prefix proc.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_executioner_hp_threshold', '20', 'Target HP percent below which Executioner prefix proc activates.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_mage_bane_proc_chance', '2,3,4,6', 'Proc chance percent per quality tier for Mage-Bane prefix on-hit cast slow.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_mage_bane_slow_percent', '15,20,25,30', 'Cast speed slow percent per quality tier for Mage-Bane prefix proc (applied as negative).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_mage_bane_duration_ms', '4000,5000,6000,6000', 'Duration in milliseconds per quality tier for Mage-Bane prefix slow debuff.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_opportunist_proc_chance', '2,3,4,6', 'Proc chance percent per quality tier for Opportunist prefix on-hit daze.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_opportunist_duration_ms', '3000,4000,5000,5000', 'Duration in milliseconds per quality tier for Opportunist prefix daze debuff.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_scavenger_bonus_damage_percent', '3,5,8,11', 'Bonus damage percent per quality tier for Scavenger prefix vs debuffed targets.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_duelist_bonus_damage_percent', '3,5,8,11', 'Bonus damage percent per quality tier for Duelist prefix when solo-attacking.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_umbralforged_aoe_damage', '8,14,20,28', 'Shadow AoE damage per pulse per quality tier for equipped Umbralforged prefix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_blight_reflect_damage', '10,18,26,36', 'Flat shadow damage reflected per melee hit per quality tier for Blight suffix items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_plunderer_bonus_gold_percent', '5,10,15,25', 'Bonus gold percent per quality tier for equipped Plunderer prefix items on creature kill.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_sage_bonus_xp_percent', '3,5,8,12', 'Bonus XP percent per quality tier for equipped Sage suffix items on creature kill.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('elite_affix_umbral_echo_interval_ms', '3000', 'Interval in milliseconds between Umbral Echo AoE pulses for equipped Umbralforged items.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);
