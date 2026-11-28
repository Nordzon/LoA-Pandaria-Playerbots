-- mod_arpg m9-charms + m9-rifts: per-item charm lines, per-character rift key. Idempotent.
CREATE TABLE IF NOT EXISTS `item_instance_arpg_charm` (
  `item_guid` INT UNSIGNED NOT NULL,
  `position` TINYINT UNSIGNED NOT NULL,
  `kind` TINYINT UNSIGNED NOT NULL,          -- 0 flat stat, 1 effect
  `what` VARCHAR(32) NOT NULL,               -- stat or effect name (arpg_charm_affix.what)
  `value` INT NOT NULL,                      -- stat points / hundredths of a percent
  `display_id` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`item_guid`, `position`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_arpg_rift` (
  `guid` INT UNSIGNED NOT NULL,
  `key_level` SMALLINT UNSIGNED NOT NULL DEFAULT 1,
  `best_level` SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `runs` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
