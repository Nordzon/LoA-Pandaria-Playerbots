-- mod_arpg m2b 3b.3: the loot companion look each character picked (arpg_companion_model.id). Idempotent.
CREATE TABLE IF NOT EXISTS `character_arpg_companion` (
  `guid`     INT UNSIGNED NOT NULL,
  `model_id` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg companion appearance';
