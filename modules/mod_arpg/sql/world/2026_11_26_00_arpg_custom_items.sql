-- mod_arpg m6-custom-items: our own items (ids 700000+), no client patch needed. Idempotent.
-- The server answers the client's item queries (Item + Item-sparse DB2 replies) from item_template, so a new id shows its
-- name, icon and tooltip; hotfix_data makes the client re-read them at login. Icons/models are copied from existing items.
--   700001 Lesser Catalyst   (tier band T1-T2, green)    icon of Lesser Magic Essence (10938)
--   700002 Greater Catalyst  (tier band T3-T4, blue)     icon of Greater Cosmic Essence (34055)
--   700003 Grand Catalyst    (tier band T5+,   purple)   icon of Ethereal Shard (74247)
--   700010 Salvaged Essence  (salvage currency, white)   icon of Mysterious Essence (74250)

DROP TEMPORARY TABLE IF EXISTS `arpg_tmp_item`;
CREATE TEMPORARY TABLE `arpg_tmp_item` LIKE `item_template`;

-- One copy per new item, from its icon source.
INSERT INTO `arpg_tmp_item` SELECT * FROM `item_template` WHERE `entry` = 10938;
UPDATE `arpg_tmp_item` SET `entry` = 700001, `name` = 'Lesser Catalyst', `Quality` = 2,
  `description` = 'Used up by your next craft that rolls affixes: one tier higher, twice the Unique chance. Tiers 1-2.'
  WHERE `entry` = 10938;
INSERT INTO `arpg_tmp_item` SELECT * FROM `item_template` WHERE `entry` = 34055;
UPDATE `arpg_tmp_item` SET `entry` = 700002, `name` = 'Greater Catalyst', `Quality` = 3,
  `description` = 'Used up by your next craft that rolls affixes: one tier higher, twice the Unique chance. Tiers 3-4.'
  WHERE `entry` = 34055;
INSERT INTO `arpg_tmp_item` SELECT * FROM `item_template` WHERE `entry` = 74247;
UPDATE `arpg_tmp_item` SET `entry` = 700003, `name` = 'Grand Catalyst', `Quality` = 4,
  `description` = 'Used up by your next craft that rolls affixes: one tier higher, twice the Unique chance. Tier 5 and up.'
  WHERE `entry` = 74247;
INSERT INTO `arpg_tmp_item` SELECT * FROM `item_template` WHERE `entry` = 74250;
UPDATE `arpg_tmp_item` SET `entry` = 700010, `name` = 'Salvaged Essence', `Quality` = 1,
  `description` = 'Left over from salvaged gear. Your companion trades essences for catalysts.'
  WHERE `entry` = 74250;

-- Shared: stack to 999, no level, no binding (tradeable), cheap to sell, buy only through the companion.
UPDATE `arpg_tmp_item` SET `stackable` = 999, `maxcount` = 0, `RequiredLevel` = 0, `bonding` = 0,
  `BuyPrice` = 0, `SellPrice` = 1;

DELETE FROM `item_template` WHERE `entry` IN (700001, 700002, 700003, 700010);
INSERT INTO `item_template` SELECT * FROM `arpg_tmp_item`;
DROP TEMPORARY TABLE `arpg_tmp_item`;

-- Make the client re-read them (Item.db2 = 1344507586, Item-sparse.db2 = 2442913102).
DELETE FROM `hotfix_data` WHERE `entry` IN (700001, 700002, 700003, 700010) AND `type` IN (1344507586, 2442913102);
INSERT INTO `hotfix_data` (`entry`, `type`, `hotfixDate`) VALUES
  (700001, 1344507586, '2026-11-26 16:00:00'), (700001, 2442913102, '2026-11-26 16:00:00'),
  (700002, 1344507586, '2026-11-26 16:00:00'), (700002, 2442913102, '2026-11-26 16:00:00'),
  (700003, 1344507586, '2026-11-26 16:00:00'), (700003, 2442913102, '2026-11-26 16:00:00'),
  (700010, 1344507586, '2026-11-26 16:00:00'), (700010, 2442913102, '2026-11-26 16:00:00');
