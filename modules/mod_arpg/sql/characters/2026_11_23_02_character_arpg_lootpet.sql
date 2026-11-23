-- mod_arpg m2b: loot companion on/off per character (SPEC 4.7). Idempotent.
CREATE TABLE IF NOT EXISTS `character_arpg_lootpet` (
  `guid`    INT UNSIGNED NOT NULL,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg loot companion setting';
