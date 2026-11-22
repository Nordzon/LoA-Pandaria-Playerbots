-- mod_arpg: named places for `.arpg test setup <char> <level> <spot>` (m1-world-scaling). Idempotent.
CREATE TABLE IF NOT EXISTS `arpg_test_spot` (
  `name` VARCHAR(48) NOT NULL,
  `map` INT UNSIGNED NOT NULL,
  `x` FLOAT NOT NULL,
  `y` FLOAT NOT NULL,
  `z` FLOAT NOT NULL,
  `o` FLOAT NOT NULL DEFAULT 0,
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg test spots';

-- Coordinates come from the core's own game_tele rows, so they are valid ground positions.
REPLACE INTO `arpg_test_spot` (`name`, `map`, `x`, `y`, `z`, `o`, `comment`)
SELECT 'elwynn-northshire', map, position_x, position_y, position_z, orientation, 'game_tele NorthshireValley (level 1-5)' FROM game_tele WHERE name = 'NorthshireValley'
UNION ALL
SELECT 'elwynn-goldshire', map, position_x, position_y, position_z, orientation, 'game_tele Goldshire (level 5-10)' FROM game_tele WHERE name = 'Goldshire'
UNION ALL
SELECT 'westfall-sentinelhill', map, position_x, position_y, position_z, orientation, 'game_tele SentinelHill (level 10-15)' FROM game_tele WHERE name = 'SentinelHill'
UNION ALL
SELECT 'jadeforest-dawnsblossom', map, position_x, position_y, position_z, orientation, 'game_tele TheJadeForest (level 85-86)' FROM game_tele WHERE name = 'TheJadeForest';
