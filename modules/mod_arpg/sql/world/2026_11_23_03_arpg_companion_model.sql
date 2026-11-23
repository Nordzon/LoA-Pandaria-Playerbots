-- mod_arpg m2b 3b.3: loot companion looks and its vendor list (SPEC 4.7). Idempotent.
CREATE TABLE IF NOT EXISTS `arpg_companion_model` (
  `id`         TINYINT UNSIGNED NOT NULL,
  `display_id` INT UNSIGNED NOT NULL,
  `name`       VARCHAR(48) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg companion appearances';

REPLACE INTO `arpg_companion_model` VALUES
(1, 7937, 'Mechanical Squirrel'), (2, 5448, 'Black Tabby Cat'), (3, 9990, 'White Kitten'), (4, 6192, 'Hyacinth Macaw'),
(5, 6288, 'Dark Whelpling'), (6, 6299, 'Hawk Owl'), (7, 1072, 'Brown Prairie Dog'), (8, 15436, 'Disgusting Oozeling'),
(9, 15992, 'Peddlefeet');

-- The companion's shop: water, bread, flour, spices, parchment and vellum (selling and repair work without a list too).
DELETE FROM `npc_vendor` WHERE `entry` = 7000001;
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `type`) VALUES
(7000001, 1, 159, 0, 0, 0, 1), (7000001, 2, 1179, 0, 0, 0, 1), (7000001, 3, 8766, 0, 0, 0, 1), (7000001, 4, 58274, 0, 0, 0, 1),
(7000001, 5, 4540, 0, 0, 0, 1), (7000001, 6, 4541, 0, 0, 0, 1), (7000001, 7, 30817, 0, 0, 0, 1), (7000001, 8, 2678, 0, 0, 0, 1),
(7000001, 9, 39354, 0, 0, 0, 1), (7000001, 10, 38682, 0, 0, 0, 1);
