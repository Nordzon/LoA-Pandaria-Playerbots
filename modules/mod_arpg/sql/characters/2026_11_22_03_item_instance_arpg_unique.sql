-- mod_arpg m3: an item instance that is a Unique (BALANCE §8b): which Unique, power tier, and the two rolled numbers.
-- Orphans are removed at startup with the affix rows. Idempotent.
CREATE TABLE IF NOT EXISTS `item_instance_arpg_unique` (
  `item_guid` INT UNSIGNED NOT NULL COMMENT 'item_instance.guid',
  `unique_id` INT UNSIGNED NOT NULL COMMENT 'arpg_unique.id',
  `tier` TINYINT UNSIGNED NOT NULL,
  `trigger_value` SMALLINT NOT NULL COMMENT 'chance % or N (every Nth hit)',
  `magnitude` SMALLINT NOT NULL COMMENT '% of Power or % of max health',
  `perfect` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`item_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg Unique item instances';
