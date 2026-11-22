-- mod_arpg m3b: ARPG potions (owner 2026-11-22, change m3b-potions). Idempotent.
-- Existing potion items heal / restore a % of the drinker's max health / resource (Loot/Potions), drop by monster tier,
-- stack to 999, need level 1 and share a short cooldown. Item changes reach the client through Item-sparse hotfixes
-- (hotfix_data type 2442913102 = Item-sparse.db2: the client re-reads these items from the server at login).
CREATE TABLE IF NOT EXISTS `arpg_potion` (
  `item_id` INT UNSIGNED NOT NULL,
  `kind`    TINYINT UNSIGNED NOT NULL COMMENT '0 healing, 1 mana (mana %, energy/focus at half, rage/runic power %), 2 rejuvenation (both)',
  `tier`    TINYINT UNSIGNED NOT NULL COMMENT '1-6: drop tier (normal 1 .. boss 6); a drop uses the highest tier <= its own of that kind',
  `pct`     TINYINT UNSIGNED NOT NULL COMMENT '% of max health / resource',
  PRIMARY KEY (`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg potions';

REPLACE INTO `arpg_potion` VALUES
(118,   0, 1, 10), (858,   0, 2, 12), (929,   0, 3, 14), (1710,  0, 4, 16), (3928,  0, 5, 18), (13446, 0, 6, 20),  -- healing
(2455,  1, 1, 10), (3385,  1, 2, 12), (3827,  1, 3, 14), (6149,  1, 4, 16), (13443, 1, 5, 18), (13444, 1, 6, 20),  -- mana
(2456,  2, 1, 8),  (18253, 2, 4, 12), (76094, 2, 6, 15);                                                          -- rejuvenation

-- Drop chance and kind mix per monster kind. source: 0 normal, 1 elite (incl. rare elite), 2 Champion, 3 Superunique, 4 boss.
CREATE TABLE IF NOT EXISTS `arpg_potion_drop` (
  `source`     TINYINT UNSIGNED NOT NULL,
  `tier`       TINYINT UNSIGNED NOT NULL,
  `chance_pct` FLOAT NOT NULL,
  `w_heal`     SMALLINT UNSIGNED NOT NULL,
  `w_mana`     SMALLINT UNSIGNED NOT NULL,
  `w_rejuv`    SMALLINT UNSIGNED NOT NULL,
  PRIMARY KEY (`source`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg potion drops by monster kind';

REPLACE INTO `arpg_potion_drop` VALUES
(0, 1,   5, 60, 30, 10),
(1, 2,  15, 60, 30, 10),
(2, 3,  50, 55, 30, 15),
(3, 5, 100, 50, 30, 20),
(4, 6, 100, 50, 30, 20);

-- Big stacks (owner: 999), any level, one short shared cooldown (category 4 = potions, 10 s instead of 60 s).
UPDATE `item_template` SET `stackable` = 999, `RequiredLevel` = 1, `spellcooldown_1` = 0, `spellcategory_1` = 4,
       `spellcategorycooldown_1` = 10000
WHERE `entry` IN (SELECT `item_id` FROM `arpg_potion`);

-- Tell the client to re-read these items (Item-sparse) from the server.
DELETE FROM `hotfix_data` WHERE `type` = 2442913102 AND `entry` IN (SELECT `item_id` FROM `arpg_potion`);
INSERT INTO `hotfix_data` (`entry`, `type`, `hotfixDate`)
SELECT `item_id`, 2442913102, '2026-11-22 20:00:00' FROM `arpg_potion`;

-- The use is handled by mod_arpg's item script (heal/resource by %, any class, shared cooldown): Loot/Potions "arpg_potion".
DELETE FROM `item_script_names` WHERE `Id` IN (SELECT `item_id` FROM `arpg_potion`);
INSERT INTO `item_script_names` (`Id`, `ScriptName`) SELECT `item_id`, 'arpg_potion' FROM `arpg_potion`;
