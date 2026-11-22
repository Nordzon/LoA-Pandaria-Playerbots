-- mod_arpg m2 (design.md): the affixes an item instance rolled. Source of truth for every affix (native ones are also
-- in the item's random-property enchant slots). Orphans are deleted at startup. Idempotent.
CREATE TABLE IF NOT EXISTS `item_instance_arpg_affix` (
  `item_guid` INT UNSIGNED NOT NULL COMMENT 'item_instance.guid',
  `position` TINYINT UNSIGNED NOT NULL COMMENT '0-4 (prop enchant slot order)',
  `affix_id` INT UNSIGNED NOT NULL COMMENT 'arpg_affix.id',
  `tier` TINYINT UNSIGNED NOT NULL,
  `value` INT NOT NULL COMMENT 'B-based flat value (snapped to the native enchant), or hundredths of a percent',
  `enchant_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'native SpellItemEnchantment used, 0 = non-native',
  PRIMARY KEY (`item_guid`, `position`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg item affixes';
