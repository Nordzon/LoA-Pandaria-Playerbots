-- mod_arpg: the companion's mount shop (owner 2026-11-26: "rep grind mounts available for gold, start ~5k"). Idempotent.
-- One row per mount spell that the game sells for reputation (mount items with a required faction); buying teaches the spell
-- (no reputation check). price_gold 0 = Arpg.MountShop.Price. Faction-only mounts are open to everyone (single player).
CREATE TABLE IF NOT EXISTS `arpg_mount_shop` (
  `spell_id`   INT UNSIGNED NOT NULL COMMENT 'mount spell taught',
  `item_id`    INT UNSIGNED NOT NULL COMMENT 'the reputation item it comes from (for its name/icon)',
  `name`       VARCHAR(80)  NOT NULL,
  `price_gold` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = Arpg.MountShop.Price',
  `sort`       INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'list order (faction id x 1000 + item order)',
  PRIMARY KEY (`spell_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg companion mount shop';

-- Rebuild the rows from the item data, keeping any price someone set by hand.
DROP TEMPORARY TABLE IF EXISTS `arpg_tmp_price`;
CREATE TEMPORARY TABLE `arpg_tmp_price` AS SELECT `spell_id`, `price_gold` FROM `arpg_mount_shop` WHERE `price_gold` <> 0;
DELETE FROM `arpg_mount_shop`;
INSERT INTO `arpg_mount_shop` (`spell_id`, `item_id`, `name`, `price_gold`, `sort`)
SELECT i.`spellid_2`, MIN(i.`entry`),
       TRIM(REPLACE(REPLACE(MIN(i.`name`), 'Reins of the ', ''), 'Reins of ', '')),
       0, MIN(i.`RequiredReputationFaction`) * 1000 + MIN(i.`entry`) % 1000
FROM `item_template` i
WHERE i.`class` = 15 AND i.`subclass` = 5 AND i.`RequiredReputationFaction` > 0 AND i.`spellid_1` = 55884 AND i.`spellid_2` > 0
GROUP BY i.`spellid_2`;
UPDATE `arpg_mount_shop` m JOIN `arpg_tmp_price` p ON p.`spell_id` = m.`spell_id` SET m.`price_gold` = p.`price_gold`;
DROP TEMPORARY TABLE `arpg_tmp_price`;
