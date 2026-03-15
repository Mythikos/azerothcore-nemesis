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
-- Bounty Board Configuration Entries
-- =============================================================================

INSERT INTO `nemesis_configuration` (`config_key`, `config_value`, `description`)
VALUES ('bounty_board_top_count', '10', 'Number of top nemeses displayed on the bounty board.')
ON DUPLICATE KEY UPDATE `config_value` = VALUES(`config_value`);

-- =============================================================================
-- Nemesis Bounty Board GameObject Template
-- Entry 200100 — GAMEOBJECT_TYPE_GOOBER (type 10), interactable bulletin board.
-- =============================================================================

DELETE FROM `gameobject_template` WHERE `entry` = 200100;
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `size`, `data0`, `data1`, `ScriptName`)
VALUES (200100, 10, 3053, 'Nemesis Bounty Board', '', '', 1.5, 0, 0, 'NemesisBountyBoardScript');

-- =============================================================================
-- Nemesis Bounty Board Spawns — Major Cities
-- GUID range: 500100+
-- spawntimesecs = 0 means always present (permanent fixture).
-- state = 1 = ready/usable.
-- rotation2 = SIN(orientation/2), rotation3 = COS(orientation/2).
-- Coordinates are approximate — use `.gobject move` to fine-tune in-game.
-- =============================================================================

-- Stormwind (Trade District)
DELETE FROM `gameobject` WHERE `guid` = 500100;
INSERT IGNORE INTO `gameobject` (`guid`, `id`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `state`)
VALUES (500100, 200100, 0, -8845.064, 626.9414, 94.53795, 0.45665103, 0, 0, SIN(0.45665103/2), COS(0.45665103/2), 0, 1);

-- Ironforge (Great Forge)
DELETE FROM `gameobject` WHERE `guid` = 500101;
INSERT IGNORE INTO `gameobject` (`guid`, `id`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `state`)
VALUES (500101, 200100, 0, -4917.532, -984.5799, 501.44952, 2.262241, 0, 0, SIN(2.262241/2), COS(2.262241/2), 0, 1);

-- Darnassus (Temple Gardens)
DELETE FROM `gameobject` WHERE `guid` = 500102;
INSERT IGNORE INTO `gameobject` (`guid`, `id`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `state`)
VALUES (500102, 200100, 1, 9910.296, 2515.1523, 1316.5914, 0.10595107, 0, 0, SIN(0.10595107/2), COS(0.10595107/2), 0, 1);

-- Orgrimmar (Valley of Strength)
DELETE FROM `gameobject` WHERE `guid` = 500103;
INSERT IGNORE INTO `gameobject` (`guid`, `id`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `state`)
VALUES (500103, 200100, 1, 1580.7947, -4418.0806, 8.125035, 4.2630987, 0, 0, SIN(4.2630987/2), COS(4.2630987/2), 0, 1);

-- Thunder Bluff
DELETE FROM `gameobject` WHERE `guid` = 500104;
INSERT IGNORE INTO `gameobject` (`guid`, `id`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `state`)
VALUES (500104, 200100, 1, -1263.055, 82.60291, 128.14952, 4.2920423, 0, 0, SIN(4.2920423/2), COS(4.2920423/2), 0, 1);

-- Undercity (Trade Quarter)
DELETE FROM `gameobject` WHERE `guid` = 500105;
INSERT IGNORE INTO `gameobject` (`guid`, `id`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `state`)
VALUES (500105, 200100, 0, 1555.5929, 246.99307, -43.102516, 6.120772, 0, 0, SIN(6.120772/2), COS(6.120772/2), 0, 1);

-- Shattrath (Terrace of Light)
DELETE FROM `gameobject` WHERE `guid` = 500106;
INSERT IGNORE INTO `gameobject` (`guid`, `id`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `state`)
VALUES (500106, 200100, 530, -1828.6752, 5464.7188, -12.427975, 3.9277346, 0, 0, SIN(3.9277346/2), COS(3.9277346/2), 0, 1);

-- Dalaran (Runeweaver Square)
DELETE FROM `gameobject` WHERE `guid` = 500107;
INSERT IGNORE INTO `gameobject` (`guid`, `id`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `state`)
VALUES (500107, 200100, 571, 5787.88, 636.5031, 647.86163, 0.06763, 0, 0, SIN(0.06763/2), COS(0.06763/2), 0, 1);

-- Exodar (Seat of the Naaru)
DELETE FROM `gameobject` WHERE `guid` = 500108;
INSERT IGNORE INTO `gameobject` (`guid`, `id`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `state`)
VALUES (500108, 200100, 530, -3935.1648, -11605.079, -138.58574, 4.354251, 0, 0, SIN(4.354251/2), COS(4.354251/2), 0, 1);

-- Silvermoon (Court of the Sun)
DELETE FROM `gameobject` WHERE `guid` = 500109;
INSERT IGNORE INTO `gameobject` (`guid`, `id`, `map`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `state`)
VALUES (500109, 200100, 530, 9904.136, -7180.0957, 31.006014, 4.5047717, 0, 0, SIN(4.5047717/2), COS(4.5047717/2), 0, 1);