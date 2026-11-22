-- mod_arpg m5: Ascended affix tiers in raids (BALANCE §12, SPEC 4.6). A drop in a raid may roll above T5 when BOTH the
-- raid's tier (arpg_raid_tier) and the character's Paragon level are high enough; values = T5 range x value_pct. Idempotent.
CREATE TABLE IF NOT EXISTS `arpg_ascended_tier` (
  `tier`          TINYINT UNSIGNED NOT NULL,
  `min_raid_tier` TINYINT UNSIGNED NOT NULL,
  `min_paragon`   SMALLINT UNSIGNED NOT NULL,
  `value_pct`     SMALLINT UNSIGNED NOT NULL COMMENT '% of the T5 range',
  PRIMARY KEY (`tier`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg Ascended affix tiers T6-T8';

REPLACE INTO `arpg_ascended_tier` VALUES (6, 3, 25, 110), (7, 5, 60, 120), (8, 7, 110, 130);
