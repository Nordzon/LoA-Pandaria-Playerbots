-- mod_arpg m9-charms: the Charm Pouch, charms and Unique charms (ids 720000+; no client patch: item_template + hotfix_data,
-- like m6-custom-items). Lines: arpg_charm_affix (random) and arpg_charm_unique (fixed). Idempotent.
--   720000 Charm Pouch    soul-bag-type container (subclass 1, BagFamily 4): only charms fit; copy of Test Soul Pouch (58810)
--   720001 Small Charm    1 line, green      icon of Good Luck Charm (12723)
--   720002 Large Charm    2 lines, blue      icon of Feather Charm (6753)
--   720003 Grand Charm    3 lines, purple    icon of Thorim's Charm of Earth (41505)
--   720010 Heart of the Pandaren   Unique (orange), one per character     icon of Feathered Charm (34026)
--   720011 Goblin's Lucky Coin     Unique                                 icon of Anduin Wrynn's Gold Coin (43641)
--   720012 Riftwalker's Sigil      Unique                                 icon of Troll Charm (16602)

DROP TEMPORARY TABLE IF EXISTS `arpg_tmp_item`;
CREATE TEMPORARY TABLE `arpg_tmp_item` LIKE `item_template`;

INSERT INTO `arpg_tmp_item` SELECT * FROM `item_template` WHERE `entry` = 58810;
UPDATE `arpg_tmp_item` SET `entry` = 720000, `name` = 'Charm Pouch', `Quality` = 3, `ContainerSlots` = 8, `BagFamily` = 4,
  `maxcount` = 1, `bonding` = 1, `RequiredLevel` = 0, `ItemLevel` = 1, `AllowableClass` = -1, `AllowableRace` = -1, `BuyPrice` = 0, `SellPrice` = 0,
  `description` = 'Charms inside this pouch grant their power. Charms anywhere else do nothing.' WHERE `entry` = 58810;

INSERT INTO `arpg_tmp_item` SELECT * FROM `item_template` WHERE `entry` = 12723;
UPDATE `arpg_tmp_item` SET `entry` = 720001, `name` = 'Small Charm', `Quality` = 2,
  `description` = 'One random power. Works only inside your Charm Pouch.' WHERE `entry` = 12723;
INSERT INTO `arpg_tmp_item` SELECT * FROM `item_template` WHERE `entry` = 6753;
UPDATE `arpg_tmp_item` SET `entry` = 720002, `name` = 'Large Charm', `Quality` = 3,
  `description` = 'Two random powers. Works only inside your Charm Pouch.' WHERE `entry` = 6753;
INSERT INTO `arpg_tmp_item` SELECT * FROM `item_template` WHERE `entry` = 41505;
UPDATE `arpg_tmp_item` SET `entry` = 720003, `name` = 'Grand Charm', `Quality` = 4,
  `description` = 'Three random powers. Works only inside your Charm Pouch.' WHERE `entry` = 41505;
INSERT INTO `arpg_tmp_item` SELECT * FROM `item_template` WHERE `entry` = 34026;
UPDATE `arpg_tmp_item` SET `entry` = 720010, `name` = 'Heart of the Pandaren', `Quality` = 5, `maxcount` = 1,
  `description` = 'Strength, agility and intellect, and faster learning. Rare Superunique drop. Charm Pouch only.' WHERE `entry` = 34026;
INSERT INTO `arpg_tmp_item` SELECT * FROM `item_template` WHERE `entry` = 43641;
UPDATE `arpg_tmp_item` SET `entry` = 720011, `name` = 'Goblin''s Lucky Coin', `Quality` = 5, `maxcount` = 1,
  `description` = 'Much more gold and better loot. Carried by Treasure Goblins. Charm Pouch only.' WHERE `entry` = 43641;
INSERT INTO `arpg_tmp_item` SELECT * FROM `item_template` WHERE `entry` = 16602;
UPDATE `arpg_tmp_item` SET `entry` = 720012, `name` = 'Riftwalker''s Sigil', `Quality` = 5, `maxcount` = 1,
  `description` = 'Stamina, critical strike, haste and speed. Reward of rift level 10 and deeper. Charm Pouch only.' WHERE `entry` = 16602;

-- Charms: misc items (class 15 / 4) of the soul-shard bag family so they fit the pouch; one per stack, tradeable.
UPDATE `arpg_tmp_item` SET `class` = 15, `subclass` = 4, `BagFamily` = 4, `stackable` = 1, `bonding` = 0, `RequiredLevel` = 0,
  `ItemLevel` = 1, `startquest` = 0, `Flags` = 0, `InventoryType` = 0, `BuyPrice` = 0, `SellPrice` = 2500,
  `spellid_1` = 0, `spellid_2` = 0, `spellid_3` = 0, `spellid_4` = 0, `spellid_5` = 0
  WHERE `entry` IN (720001, 720002, 720003, 720010, 720011, 720012);
UPDATE `arpg_tmp_item` SET `maxcount` = 0 WHERE `entry` IN (720001, 720002, 720003);

DELETE FROM `item_template` WHERE `entry` IN (720000, 720001, 720002, 720003, 720010, 720011, 720012);
INSERT INTO `item_template` SELECT * FROM `arpg_tmp_item`;
DROP TEMPORARY TABLE `arpg_tmp_item`;

DELETE FROM `hotfix_data` WHERE `entry` IN (720000, 720001, 720002, 720003, 720010, 720011, 720012) AND `type` IN (1344507586, 2442913102);
INSERT INTO `hotfix_data` (`entry`, `type`, `hotfixDate`) VALUES
  (720000, 1344507586, '2026-11-28 03:00:00'), (720000, 2442913102, '2026-11-28 03:00:00'),
  (720001, 1344507586, '2026-11-28 03:00:00'), (720001, 2442913102, '2026-11-28 03:00:00'),
  (720002, 1344507586, '2026-11-28 03:00:00'), (720002, 2442913102, '2026-11-28 03:00:00'),
  (720003, 1344507586, '2026-11-28 03:00:00'), (720003, 2442913102, '2026-11-28 03:00:00'),
  (720010, 1344507586, '2026-11-28 03:00:00'), (720010, 2442913102, '2026-11-28 03:00:00'),
  (720011, 1344507586, '2026-11-28 03:00:00'), (720011, 2442913102, '2026-11-28 03:00:00'),
  (720012, 1344507586, '2026-11-28 03:00:00'), (720012, 2442913102, '2026-11-28 03:00:00');

-- Random charm lines. kind 0 = flat stat (lo/hi = permille of a ring's stat budget B; gear affix lines are 15-50% of B at the loot level),
-- kind 1 = effect (lo/hi in hundredths of a percent, as gear affixes). min_level: first character level it can roll at.
CREATE TABLE IF NOT EXISTS `arpg_charm_affix` (
  `id` INT UNSIGNED NOT NULL,
  `name` VARCHAR(40) NOT NULL,
  `kind` TINYINT UNSIGNED NOT NULL,
  `what` VARCHAR(32) NOT NULL,
  `weight` INT UNSIGNED NOT NULL,
  `min_level` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `lo` INT NOT NULL,
  `hi` INT NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
DELETE FROM `arpg_charm_affix`;
INSERT INTO `arpg_charm_affix` (`id`, `name`, `kind`, `what`, `weight`, `min_level`, `lo`, `hi`) VALUES
  (1,  'Strength',            0, 'strength',          10, 1,  120, 250),
  (2,  'Agility',             0, 'agility',           10, 1,  120, 250),
  (3,  'Intellect',           0, 'intellect',         10, 1,  120, 250),
  (4,  'Stamina',             0, 'stamina',           14, 1,  150, 300),
  (5,  'Critical Strike',     0, 'crit_rating',        8, 10, 120, 250),
  (6,  'Haste',               0, 'haste_rating',       8, 10, 120, 250),
  (7,  'Mastery',             0, 'mastery_rating',     8, 10, 120, 250),
  (20, 'Magic Find',          1, 'magic_find_pct',     8, 1,  200, 600),
  (21, 'Gold Find',           1, 'gold_find_pct',      8, 1,  300, 900),
  (22, 'Experience',          1, 'xp_pct',             6, 1,  100, 300),
  (23, 'Swiftness',           1, 'move_speed_pct',     5, 10, 100, 200),
  (24, 'Toughness',           1, 'armor_pct',          6, 1,  100, 300),
  (25, 'Elite Slayer',        1, 'dmg_vs_elite_pct',   5, 20, 100, 300),
  (26, 'Elite Warding',       1, 'dr_vs_elite_pct',    5, 20, 100, 250);

-- Unique charms: fixed lines (same kinds and units).
CREATE TABLE IF NOT EXISTS `arpg_charm_unique` (
  `item` INT UNSIGNED NOT NULL,
  `position` TINYINT UNSIGNED NOT NULL,
  `kind` TINYINT UNSIGNED NOT NULL,
  `what` VARCHAR(32) NOT NULL,
  `lo` INT NOT NULL,
  `hi` INT NOT NULL,
  PRIMARY KEY (`item`, `position`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
DELETE FROM `arpg_charm_unique`;
INSERT INTO `arpg_charm_unique` (`item`, `position`, `kind`, `what`, `lo`, `hi`) VALUES
  (720010, 0, 0, 'strength',        350, 450),
  (720010, 1, 0, 'agility',         350, 450),
  (720010, 2, 0, 'intellect',       350, 450),
  (720010, 3, 1, 'xp_pct',          500, 1000),
  (720011, 0, 1, 'gold_find_pct',   2000, 3000),
  (720011, 1, 1, 'magic_find_pct',  1000, 2000),
  (720012, 0, 0, 'stamina',         400, 500),
  (720012, 1, 0, 'crit_rating',     300, 400),
  (720012, 2, 0, 'haste_rating',    300, 400),
  (720012, 3, 1, 'move_speed_pct',  300, 500);
