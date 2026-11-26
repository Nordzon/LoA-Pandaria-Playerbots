-- mod_arpg m2b-mount-shop: the mount shop as a vendor window (owner 2026-11-27: "make the UI a vendor one"). Idempotent.
-- The reputation mount items need their faction standing (the vendor window checks it), so each gets a copy at
-- 800000 + its id: no reputation, no faction/race limit, buy price = arpg_mount_shop.price_gold (0 = 5000 g). The copies
-- teach the same mount spell. They are sold by creature 1000002 "Mount Merchant" (never spawned): the companion's
-- "Mounts for gold" opens that vendor list (WorldSession::SendListInventory with a vendor entry).

-- Vendor entry (a copy of the companion's template, only its vendor list is used).
DELETE FROM `creature_template_model` WHERE `CreatureID` = 1000002;
DELETE FROM `creature_template` WHERE `entry` = 1000002;
DROP TEMPORARY TABLE IF EXISTS `arpg_tmp_ct`;
CREATE TEMPORARY TABLE `arpg_tmp_ct` LIKE `creature_template`;
INSERT INTO `arpg_tmp_ct` SELECT * FROM `creature_template` WHERE `entry` = 1000001;
UPDATE `arpg_tmp_ct` SET `entry` = 1000002, `name` = 'Mount Merchant', `subname` = 'ARPG', `npcflag` = 128, `ScriptName` = '';
INSERT INTO `creature_template` SELECT * FROM `arpg_tmp_ct`;
DROP TEMPORARY TABLE `arpg_tmp_ct`;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
SELECT 1000002, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild` FROM `creature_template_model` WHERE `CreatureID` = 1000001;

-- Mount item copies.
DELETE FROM `item_template` WHERE `entry` BETWEEN 800000 AND 899999;
DROP TEMPORARY TABLE IF EXISTS `arpg_tmp_it`;
CREATE TEMPORARY TABLE `arpg_tmp_it` LIKE `item_template`;
INSERT INTO `arpg_tmp_it` SELECT i.* FROM `item_template` i JOIN `arpg_mount_shop` m ON m.`item_id` = i.`entry`;
UPDATE `arpg_tmp_it` t JOIN `arpg_mount_shop` m ON m.`item_id` = t.`entry`
SET t.`BuyPrice` = IF(m.`price_gold` = 0, 5000, m.`price_gold`) * 10000,
    t.`SellPrice` = 0,
    t.`RequiredReputationFaction` = 0, t.`RequiredReputationRank` = 0,
    t.`AllowableRace` = -1, t.`AllowableClass` = -1,
    t.`FlagsExtra` = t.`FlagsExtra` & ~3,          -- no Horde-only / Alliance-only
    t.`entry` = 800000 + t.`entry`;
INSERT INTO `item_template` SELECT * FROM `arpg_tmp_it`;
DROP TEMPORARY TABLE `arpg_tmp_it`;

-- The vendor list, in shop order.
DELETE FROM `npc_vendor` WHERE `entry` = 1000002;
SET @slot := 0;
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `type`)
SELECT 1000002, (@slot := @slot + 1), 800000 + m.`item_id`, 0, 0, 0, 1
FROM `arpg_mount_shop` m ORDER BY m.`sort`, m.`spell_id`;

-- The client reads the copies from the server (Item + Item-sparse hotfixes).
DELETE FROM `hotfix_data` WHERE `entry` BETWEEN 800000 AND 899999 AND `type` IN (1344507586, 2442913102);
INSERT INTO `hotfix_data` (`entry`, `type`, `hotfixDate`)
SELECT 800000 + `item_id`, 1344507586, '2026-11-27 01:00:00' FROM `arpg_mount_shop`
UNION ALL
SELECT 800000 + `item_id`, 2442913102, '2026-11-27 01:00:00' FROM `arpg_mount_shop`;
