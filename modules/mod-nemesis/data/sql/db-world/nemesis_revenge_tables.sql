-- =============================================================================
-- Nemesis Revenge System — Tables
-- =============================================================================

-- -----------------------------------------------------------------------------
-- nemesis_player_vengeance
-- Tracks the total number of vengeance kills per player (kills of nemeses that
-- originated from that player's death). Drives title progression.
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `nemesis_player_vengeance` (
	`player_guid` INT UNSIGNED NOT NULL,
	`vengeance_count` INT UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (`player_guid`) USING BTREE
)
COMMENT='Per-player vengeance kill counter for revenge title progression'
COLLATE='utf8mb4_0900_ai_ci'
ENGINE=InnoDB;
