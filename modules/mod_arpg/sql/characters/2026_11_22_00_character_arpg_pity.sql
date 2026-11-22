-- mod_arpg m2: per-character, per-boss pity counters (SPEC 4.4, BALANCE §13). Idempotent.
CREATE TABLE IF NOT EXISTS `character_arpg_pity` (
  `guid` INT UNSIGNED NOT NULL COMMENT 'characters.guid',
  `boss_entry` INT UNSIGNED NOT NULL COMMENT 'creature_template.entry (0 = global god-roll counter)',
  `unique_misses` SMALLINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'kills since this boss last dropped its Unique',
  `godroll_kills` SMALLINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'boss kills since the last 5-affix item',
  PRIMARY KEY (`guid`, `boss_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg loot pity';
