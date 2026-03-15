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
-- Nemesis Revenge System — Configuration
-- =============================================================================
INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`) VALUES
	('revenge_group_vengeance_enabled', '1', 'Allow group members to earn vengeance if the origin player is in the group (0 = solo only, 1 = group)'),
	('revenge_group_vengeance_max_distance', '100', 'Maximum distance (yards) from the killer for group members to receive vengeance credit')
ON DUPLICATE KEY UPDATE `config_key` = `config_key`;
