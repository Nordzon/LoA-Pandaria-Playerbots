-- mod_arpg m2 task 4.7: display row of each affix (our client patch SpellItemEnchantment id in a display slot 9-12;
-- 0 = not shown natively). Stats of these affixes are applied by Loot/AffixStats (enchant_id stays 0). Idempotent.
SET @col := (SELECT COUNT(*) FROM information_schema.COLUMNS
             WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'item_instance_arpg_affix' AND COLUMN_NAME = 'display_id');
SET @sql := IF(@col = 0,
    'ALTER TABLE `item_instance_arpg_affix` ADD COLUMN `display_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT ''client patch display row (Arpg.Display.*), 0 = none'' AFTER `enchant_id`',
    'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
