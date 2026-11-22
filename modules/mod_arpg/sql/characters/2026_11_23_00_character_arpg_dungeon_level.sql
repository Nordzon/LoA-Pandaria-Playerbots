-- mod_arpg m5: dungeon level selector - the level a character wants a dungeon to run at (0 / no row = own level). Idempotent.
CREATE TABLE IF NOT EXISTS `character_arpg_dungeon_level` (
  `guid`  INT UNSIGNED NOT NULL,
  `map`   INT UNSIGNED NOT NULL,
  `level` TINYINT UNSIGNED NOT NULL,
  PRIMARY KEY (`guid`, `map`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg chosen dungeon levels';
