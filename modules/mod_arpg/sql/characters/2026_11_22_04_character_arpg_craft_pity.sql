-- Crafted-Unique pity per character and profession (SPEC 4.8, m1b task 2b.2): crafts of orange/yellow recipes without a Unique.
CREATE TABLE IF NOT EXISTS `character_arpg_craft_pity` (
  `guid`       INT UNSIGNED      NOT NULL COMMENT 'characters.guid',
  `skill_line` SMALLINT UNSIGNED NOT NULL COMMENT 'SkillLine.dbc id of the profession',
  `misses`     SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`, `skill_line`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg crafted Unique pity';
