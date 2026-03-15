-- =============================================================================
-- Nemesis Configuration Table
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
-- Umbral Moon Configuration Entries
-- =============================================================================

-- Schedule: Days of the week umbral moons occur (3-letter abbreviations, comma-separated)
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('umbral_moon_schedule_days', 'Wed,Sat', 'Days of the week when Umbral Moon events occur.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- Schedule: Times during those days (HH:MM 24h format, comma-separated)
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('umbral_moon_schedule_times', '08:00,20:00', 'Times of day (24h) when Umbral Moon events start.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- Duration in minutes
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('umbral_moon_duration_minutes', '120', 'Duration of each Umbral Moon event in minutes.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- Remove old multiplier-format keys if they exist from a prior schema
DELETE FROM `nemesis_configuration` WHERE `config_key` IN ('umbral_moon_mob_hp_multiplier', 'umbral_moon_mob_damage_multiplier');

-- Mob stat percent boosts during Umbral Moon
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('umbral_moon_mob_hp_boost_percent', '100', 'Percent HP increase applied to open world mobs during Umbral Moon (e.g. 100 = +100%, doubling HP).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('umbral_moon_mob_damage_boost_percent', '100', 'Percent damage increase applied to open world mobs during Umbral Moon (e.g. 100 = +100%, doubling damage).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('umbral_moon_bonus_xp_multiplier', '1.5', 'XP multiplier for open world kills during Umbral Moon.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- Transition Visuals
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('umbral_moon_transition_to_time', '0', 'Hour of day (0-23) that we transiiton to. Default 0 (12 AM).')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('umbral_moon_transition_speed', '60', 'Sky transition speed in game-minutes per real-second (e.g. 60 = 1 game-hour per second). Set to 0 for instant transitions.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);