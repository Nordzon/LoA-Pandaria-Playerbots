-- mod_arpg m4: Paragon progress per character (SPEC 4.5, change m4-paragon). Idempotent.
CREATE TABLE IF NOT EXISTS `character_paragon` (
  `guid`  INT UNSIGNED NOT NULL COMMENT 'characters.guid',
  `level` INT UNSIGNED NOT NULL DEFAULT 0,
  `xp`    BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'XP into the current Paragon level',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg Paragon level and XP';

-- Points spent: tab 0 Core, 1 Offense, 2 Defense, 3 Utility; stat = the chosen bonus inside the tab (Paragon.h).
CREATE TABLE IF NOT EXISTS `character_paragon_point` (
  `guid`   INT UNSIGNED NOT NULL,
  `tab`    TINYINT UNSIGNED NOT NULL,
  `stat`   TINYINT UNSIGNED NOT NULL,
  `points` SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`, `tab`, `stat`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg Paragon points spent';
