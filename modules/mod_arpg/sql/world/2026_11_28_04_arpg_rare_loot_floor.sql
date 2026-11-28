-- mod_arpg m9-rare-loot-floor (owner 2026-11-28: "move all of the unique-exclusive drop rates to at least 30%"):
-- the rares in Superunique pools (their "Unique <name>" copy uses the same loot table) drop their own exclusive loot
-- (mounts, toys, pets, special gear) at least @floor %. "Exclusive" = an item that at most 3 creatures drop directly;
-- quest items (negative chance), references and grouped rows are left alone.
-- The stock chance is kept in arpg_rare_loot_floor (first run only), so re-running is safe and it can be undone:
--   UPDATE creature_loot_template l JOIN arpg_rare_loot_floor f ON f.entry = l.entry AND f.item = l.item
--   SET l.ChanceOrQuestChance = f.stock_chance;
SET @floor := 30;

CREATE TABLE IF NOT EXISTS `arpg_rare_loot_floor` (
  `entry`        INT UNSIGNED NOT NULL COMMENT 'creature_loot_template.entry (the rare''s lootid)',
  `item`         INT UNSIGNED NOT NULL,
  `stock_chance` FLOAT NOT NULL COMMENT 'chance before the floor',
  PRIMARY KEY (`entry`, `item`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DROP TEMPORARY TABLE IF EXISTS `tmp_rare_lootids`;
CREATE TEMPORARY TABLE `tmp_rare_lootids` AS
  SELECT DISTINCT t.`lootid` AS `lootid` FROM `arpg_superunique` s JOIN `creature_template` t ON t.`entry` = s.`creature_entry`
  WHERE t.`rank` IN (2, 4) AND t.`lootid` <> 0;

DROP TEMPORARY TABLE IF EXISTS `tmp_item_sources`;
CREATE TEMPORARY TABLE `tmp_item_sources` AS
  SELECT `item`, COUNT(DISTINCT `entry`) AS `n` FROM `creature_loot_template` WHERE `mincountOrRef` > 0 GROUP BY `item`;

INSERT IGNORE INTO `arpg_rare_loot_floor` (`entry`, `item`, `stock_chance`)
SELECT l.`entry`, l.`item`, l.`ChanceOrQuestChance`
FROM `creature_loot_template` l
JOIN `tmp_rare_lootids` r ON r.`lootid` = l.`entry`
JOIN `tmp_item_sources` s ON s.`item` = l.`item`
WHERE s.`n` <= 3 AND l.`mincountOrRef` > 0 AND l.`groupid` = 0
  AND l.`ChanceOrQuestChance` > 0 AND l.`ChanceOrQuestChance` < @floor;

UPDATE `creature_loot_template` l
JOIN `arpg_rare_loot_floor` f ON f.`entry` = l.`entry` AND f.`item` = l.`item`
SET l.`ChanceOrQuestChance` = GREATEST(f.`stock_chance`, @floor);

DROP TEMPORARY TABLE IF EXISTS `tmp_rare_lootids`;
DROP TEMPORARY TABLE IF EXISTS `tmp_item_sources`;
