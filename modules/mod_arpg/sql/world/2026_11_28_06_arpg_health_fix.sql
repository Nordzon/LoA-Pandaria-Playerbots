-- mod_arpg: ordinary monsters whose template health multiplier is a data error (owner 2026-11-28: Darkened Horrors on the
-- Wandering Isle had 43k hp at level 9 / 83k at 16 - Health_mod 272 on a normal mob with 31 spawns). Each row sets the
-- multiplier for the creature and its Champion / Unique copies (entry + 400000 / + 700000). Add rows for others the owner
-- reports. The stock value is kept (first run), so re-running is safe and it can be undone:
--   UPDATE creature_template t JOIN arpg_health_fix f ON t.entry IN (f.entry, f.entry + 400000, f.entry + 700000)
--   SET t.Health_mod = f.stock_mod;
CREATE TABLE IF NOT EXISTS `arpg_health_fix` (
  `entry`      INT UNSIGNED NOT NULL,
  `health_mod` FLOAT NOT NULL COMMENT 'multiplier to use',
  `stock_mod`  FLOAT NULL COMMENT 'template value before the fix (filled on first run)',
  `comment`    VARCHAR(100) NOT NULL DEFAULT '',
  PRIMARY KEY (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT IGNORE INTO `arpg_health_fix` (`entry`, `health_mod`, `comment`) VALUES
  (56007, 1, 'Darkened Horror (Wandering Isle): Health_mod 272 on a normal mob');

UPDATE `arpg_health_fix` f JOIN `creature_template` t ON t.`entry` = f.`entry`
SET f.`stock_mod` = t.`Health_mod` WHERE f.`stock_mod` IS NULL;

UPDATE `creature_template` t JOIN `arpg_health_fix` f ON t.`entry` IN (f.`entry`, f.`entry` + 400000, f.`entry` + 700000)
SET t.`Health_mod` = f.`health_mod`;
