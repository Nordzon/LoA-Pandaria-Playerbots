-- mod_arpg m3: Superuniques (SPEC 4.3, BALANCE §14, change m3 task 2.2). Idempotent.
-- A zone keeps Arpg.Superunique.PerZone of its pool alive; Arpg.Superunique.RespawnDelay after a kill a DIFFERENT one spawns
-- at one of its own spawn points. Power from arpg_monster_band kind 1 (by the player's level); abilities from its kit.
-- First 3 zones: Elwynn Forest (12), Westfall (40), Dun Morogh (1); the creatures are the zones' existing rares.
CREATE TABLE IF NOT EXISTS `arpg_superunique` (
  `id`             INT UNSIGNED NOT NULL,
  `zone_id`        INT UNSIGNED NOT NULL COMMENT 'AreaTable zone the pool belongs to',
  `creature_entry` INT UNSIGNED NOT NULL COMMENT 'creature_template.entry (model, name, loot)',
  `kit_id`         INT UNSIGNED NOT NULL COMMENT 'arpg_superunique_ability.kit_id',
  `weight`         INT UNSIGNED NOT NULL DEFAULT 100 COMMENT 'relative chance in the zone rotation; 0 = off',
  PRIMARY KEY (`id`), KEY `zone` (`zone_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg Superunique pools';

REPLACE INTO `arpg_superunique` VALUES
-- Elwynn Forest
(1, 12,  99, 2, 100),   -- Morgaine the Sly (caster)
(2, 12, 100, 1, 100),   -- Gruff Swiftbite (brute)
(3, 12, 471, 1, 100),   -- Mother Fang (brute)
(4, 12, 472, 2, 100),   -- Fedfennel (caster)
(5, 12,  79, 1, 100),   -- Narg the Taskmaster (brute)
-- Westfall
(11, 40, 506, 1, 100),  -- Sergeant Brashclaw
(12, 40, 519, 1, 100),  -- Slark
(13, 40, 520, 1, 100),  -- Brack
(14, 40, 572, 1, 100),  -- Leprithus
(15, 40, 599, 2, 100),  -- Marisa du'Paige (caster)
-- Dun Morogh
(21, 1, 1119, 1, 100),  -- Hammerspine
(22, 1, 1130, 1, 100),  -- Bjarn
(23, 1, 1137, 1, 100);  -- Edan the Howler

-- Spawn points: the creature's own spawn rows in that zone (point = creature.guid).
CREATE TABLE IF NOT EXISTS `arpg_superunique_spawn` (
  `superunique_id` INT UNSIGNED NOT NULL,
  `point`          INT UNSIGNED NOT NULL COMMENT 'creature.guid the position was copied from',
  `map`            SMALLINT UNSIGNED NOT NULL,
  `x` FLOAT NOT NULL, `y` FLOAT NOT NULL, `z` FLOAT NOT NULL, `o` FLOAT NOT NULL,
  PRIMARY KEY (`superunique_id`, `point`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg Superunique spawn points';

DELETE FROM `arpg_superunique_spawn` WHERE `superunique_id` < 100;
INSERT INTO `arpg_superunique_spawn` (`superunique_id`, `point`, `map`, `x`, `y`, `z`, `o`)
SELECT s.`id`, c.`guid`, c.`map`, c.`position_x`, c.`position_y`, c.`position_z`, c.`orientation`
FROM `arpg_superunique` s JOIN `creature` c ON c.`id` = s.`creature_entry` AND c.`zoneId` = s.`zone_id` AND c.`map` = 0
WHERE s.`id` < 100;

-- Ability kits: a Superunique uses the first N abilities its band allows (arpg_monster_band kind 1 affix_min..affix_max)
-- whose min_player_level <= the player's level. kind: 0 instant, 1 interruptible cast, 2 must-interrupt cast.
-- damage_pct = % of the band's affix damage (itself a % of the player's max health); spells only provide the visual/cast.
CREATE TABLE IF NOT EXISTS `arpg_superunique_ability` (
  `kit_id`           INT UNSIGNED NOT NULL,
  `slot`             TINYINT UNSIGNED NOT NULL,
  `spell_id`         INT UNSIGNED NOT NULL,
  `kind`             TINYINT UNSIGNED NOT NULL,
  `every_ms`         INT UNSIGNED NOT NULL,
  `damage_pct`       SMALLINT UNSIGNED NOT NULL,
  `min_player_level` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`kit_id`, `slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg Superunique ability kits';

REPLACE INTO `arpg_superunique_ability` VALUES
-- kit 1: brute (melee + stomp; a slow cast from 15, a must-interrupt from 35)
(1, 1,  845, 0,  6000,  50,  1),   -- Cleave
(1, 2, 6343, 0, 10000,  60,  1),   -- Thunder Clap
(1, 3,  116, 1, 12000,  80, 15),   -- Frostbolt (slow, interruptible)
(1, 4,  686, 2, 20000, 160, 35),   -- Shadow Bolt (must interrupt)
-- kit 2: caster (long telegraphed casts at low level; must-interrupt from 35)
(2, 1,  133, 1,  8000,  70,  1),   -- Fireball
(2, 2, 2136, 0,  9000,  50,  1),   -- Fire Blast
(2, 3,  120, 0, 12000,  50, 15),   -- Cone of Cold
(2, 4,  403, 2, 20000, 160, 35);   -- Lightning Bolt (must interrupt)
