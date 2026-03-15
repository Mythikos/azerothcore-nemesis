-- =============================================================================
-- Nemesis Elite Tables
-- Core persistence schema for nemesis elite entities.
-- Safe to drop and re-run from scratch (pre-production).
--
-- Teardown:
--   DELETE FROM creature_template WHERE entry >= 9000000;
--   DELETE FROM creature_template_model WHERE CreatureID >= 9000000;
--   DROP TABLE IF EXISTS nemesis_elite_areas;
--   DROP TABLE IF EXISTS nemesis_elite_kill_log;
--   DROP TABLE IF EXISTS nemesis_elite;
-- =============================================================================

-- -----------------------------------------------------------------------------
-- nemesis_elite
-- One row per promoted nemesis entity. Persists until killed by players.
-- Origin traits are frozen at promotion; earned traits accumulate over lifetime.
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `nemesis_elite` (
	`elite_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
	`creature_guid` BIGINT UNSIGNED NOT NULL COMMENT 'Spawn GUID from the creature table',
	`creature_entry` MEDIUMINT UNSIGNED NOT NULL COMMENT 'Original creature template entry',
	`custom_entry` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Cloned creature_template entry (ELITE_CUSTOM_ENTRY_BASE + elite_id), 0 if not yet created',
	`name` VARCHAR(100) NOT NULL COMMENT 'Procedurally generated nemesis name' COLLATE 'utf8mb4_0900_ai_ci',
	`effective_level` TINYINT UNSIGNED NOT NULL DEFAULT '1',
	`threat_score` INT UNSIGNED NOT NULL DEFAULT '0',
	`kill_count` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Total number of players killed (counts repeated kills of the same player)',
	`unique_kill_count` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Number of distinct players killed (each player counted once regardless of how many times they were killed)',
	`survival_attempts` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Number of player combat encounters survived (evade/reset while alive)',
	`origin_traits` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Bitmask of origin traits, frozen at promotion',
	`earned_traits` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Bitmask of earned traits, accumulated over lifetime',
	`origin_player_guid` BIGINT UNSIGNED NOT NULL COMMENT 'Player whose death triggered promotion',
	`origin_stack_count` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Victim nemesis stack count at time of death (Empowered trait source)',
	`umbral_moon_origin` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT '1 = promoted during an Umbral Moon',
	`last_map_id` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
	`last_zone_id` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
	`last_area_id` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
	`last_pos_x` FLOAT NOT NULL DEFAULT '0',
	`last_pos_y` FLOAT NOT NULL DEFAULT '0',
	`last_pos_z` FLOAT NOT NULL DEFAULT '0',
	`last_seen_at` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix timestamp of last observed in-world position update',
	`promoted_at` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix timestamp of promotion',
	`last_age_level_at` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix timestamp of last age-based level gain; defaults to promoted_at on first use',
	`killed_faction_mask` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Bitmask: bit 0 = Alliance kill, bit 1 = Horde kill; used for Blight trait detection',
	`is_alive` TINYINT UNSIGNED NOT NULL DEFAULT '1',
	`killed_at` INT UNSIGNED NULL DEFAULT NULL COMMENT 'Unix timestamp of death',
	`killed_by_player_guid` BIGINT UNSIGNED NULL DEFAULT NULL COMMENT 'Player who landed the killing blow',
	PRIMARY KEY (`elite_id`) USING BTREE,
	INDEX `idx_creature_guid` (`creature_guid`) USING BTREE,
	INDEX `idx_origin_player` (`origin_player_guid`) USING BTREE,
	INDEX `idx_alive_threat` (`is_alive`, `threat_score`) USING BTREE
)
COMMENT='Persistent nemesis elite entities'
COLLATE='utf8mb4_0900_ai_ci'
ENGINE=InnoDB
AUTO_INCREMENT=69;

-- -----------------------------------------------------------------------------
-- nemesis_elite_kill_log
-- One row per player killed by a nemesis.
-- Used for: unique kill tallying, Notorious/Survivor earned trait evaluation,
-- revenge system tracking (which player's death spawned which nemesis).
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `nemesis_elite_areas` (
	`elite_id` INT UNSIGNED NOT NULL,
	`area_id` SMALLINT UNSIGNED NOT NULL,
	PRIMARY KEY (`elite_id`, `area_id`) USING BTREE
)
COMMENT='Area visit log per nemesis elite for Nomad trait tracking'
COLLATE='utf8mb4_0900_ai_ci'
ENGINE=InnoDB;

-- -----------------------------------------------------------------------------
-- nemesis_elite_areas
-- Tracks every distinct area a nemesis elite has been observed in.
-- Used for the Nomad earned trait.
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `nemesis_elite_kill_log` (
	`id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
	`elite_id` BIGINT UNSIGNED NOT NULL,
	`player_guid` BIGINT UNSIGNED NOT NULL,
	`killed_at` INT UNSIGNED NOT NULL,
	PRIMARY KEY (`id`) USING BTREE,
	INDEX `idx_elite_id` (`elite_id`) USING BTREE,
	INDEX `idx_player_guid` (`player_guid`) USING BTREE
)
COMMENT='Log of all player kills made by each nemesis elite'
COLLATE='utf8mb4_0900_ai_ci'
ENGINE=InnoDB
AUTO_INCREMENT=88;

-- -----------------------------------------------------------------------------
-- nemesis_item_affixes
-- Tracks affixed items generated from nemesis kills. One row per cloned item.
-- Persisted indefinitely.
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `nemesis_item_affixes` (
	`item_entry` INT UNSIGNED NOT NULL,
	`elite_id` BIGINT UNSIGNED NOT NULL,
	`prefix_trait` INT UNSIGNED NOT NULL DEFAULT '0',
	`suffix_trait` INT UNSIGNED NOT NULL DEFAULT '0',
	`quality_tier` TINYINT UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (`item_entry`) USING BTREE,
	INDEX `idx_elite_id` (`elite_id`) USING BTREE
)
COMMENT='Tracks affixed items generated from nemesis kills. One row per cloned item. Persisted indefinitely.'
COLLATE='utf8mb4_0900_ai_ci'
ENGINE=InnoDB;