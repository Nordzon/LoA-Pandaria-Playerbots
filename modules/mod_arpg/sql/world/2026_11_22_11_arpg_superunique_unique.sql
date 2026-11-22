-- mod_arpg m3: each Superunique drops its own Unique (SPEC 4.3 "own Unique at 10%"). Until there are more Uniques, the 6 we
-- have are shared by theme (1 Keg-Breaker's Burden 2H, 2 Ox-Soul Mantle shoulders, 3 Twin Fangs of Venomspite 1H,
-- 4 Censer of the Unseen Light off-hand, 5 Stormcaller's Band ring, 6 Boots of the Long Road). Idempotent.
CREATE TABLE IF NOT EXISTS `arpg_superunique_unique` (
  `superunique_id` INT UNSIGNED NOT NULL,
  `unique_id`      INT UNSIGNED NOT NULL COMMENT 'arpg_unique.id',
  PRIMARY KEY (`superunique_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg the Unique each Superunique drops';

REPLACE INTO `arpg_superunique_unique` VALUES
(1, 3),   -- Morgaine the Sly: Twin Fangs of Venomspite
(2, 1),   -- Gruff Swiftbite: Keg-Breaker's Burden
(3, 3),   -- Mother Fang: Twin Fangs of Venomspite
(4, 4),   -- Fedfennel: Censer of the Unseen Light
(5, 2),   -- Narg the Taskmaster: Ox-Soul Mantle
(11, 1),  -- Sergeant Brashclaw: Keg-Breaker's Burden
(12, 5),  -- Slark: Stormcaller's Band
(13, 2),  -- Brack: Ox-Soul Mantle
(14, 4),  -- Leprithus: Censer of the Unseen Light
(15, 6),  -- Marisa du'Paige: Boots of the Long Road
(21, 1),  -- Hammerspine: Keg-Breaker's Burden
(22, 6),  -- Bjarn: Boots of the Long Road
(23, 5);  -- Edan the Howler: Stormcaller's Band
