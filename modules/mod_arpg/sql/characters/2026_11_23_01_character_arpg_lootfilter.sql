-- mod_arpg m2b: loot filter rules per character, in order (SPEC 4.7, Core/LootFilter text form). Idempotent.
CREATE TABLE IF NOT EXISTS `character_arpg_lootfilter` (
  `guid`     INT UNSIGNED NOT NULL,
  `position` TINYINT UNSIGNED NOT NULL,
  `rule`     VARCHAR(255) NOT NULL COMMENT 'e.g. "quality<=0 -> sell"',
  PRIMARY KEY (`guid`, `position`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg loot filter rules';
