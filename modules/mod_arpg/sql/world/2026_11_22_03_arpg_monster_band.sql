-- mod_arpg m3: Champion / Superunique level bands and monster affixes (SPEC 4.3, BALANCE §14). Idempotent.
-- A band applies from min_player_level up to the next row's min_player_level - 1. kind: 0 Champion, 1 Superunique.
-- affix_min/max = monster affixes (Champion) or abilities (Superunique). affix_damage_pct = % of the player's max health.
CREATE TABLE IF NOT EXISTS `arpg_monster_band` (
  `kind` TINYINT UNSIGNED NOT NULL, `min_player_level` TINYINT UNSIGNED NOT NULL,
  `level_offset` TINYINT NOT NULL, `health_mult` FLOAT NOT NULL, `damage_mult` FLOAT NOT NULL,
  `affix_min` TINYINT UNSIGNED NOT NULL, `affix_max` TINYINT UNSIGNED NOT NULL,
  `minions_min` TINYINT UNSIGNED NOT NULL, `minions_max` TINYINT UNSIGNED NOT NULL,
  `orb_pct` TINYINT UNSIGNED NOT NULL, `affix_damage_pct` TINYINT UNSIGNED NOT NULL,
  PRIMARY KEY (`kind`, `min_player_level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg Champion/Superunique power by player level band';

REPLACE INTO `arpg_monster_band` VALUES
(0,  1, 0, 2.0, 1.15, 1, 1, 1, 1, 20, 15),
(0, 15, 1, 2.5, 1.30, 1, 2, 2, 3, 20, 15),
(0, 35, 2, 3.0, 1.50, 1, 2, 2, 4, 20, 25),
(0, 60, 2, 3.0, 1.50, 2, 2, 2, 4, 20, 25),
(1,  1, 1, 3.0, 1.20, 2, 2, 0, 0, 40, 15),
(1, 15, 2, 4.0, 1.35, 3, 3, 0, 0, 40, 15),
(1, 35, 3, 5.0, 1.50, 3, 4, 0, 0, 40, 25),
(1, 60, 3, 6.0, 1.60, 4, 4, 0, 0, 40, 25);

CREATE TABLE IF NOT EXISTS `arpg_monster_affix` (
  `id` INT UNSIGNED NOT NULL, `name` VARCHAR(32) NOT NULL, `effect` VARCHAR(32) NOT NULL,
  `unlock_level` TINYINT UNSIGNED NOT NULL, `weight` INT UNSIGNED NOT NULL, `exclusive_group` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg Champion monster affixes';

REPLACE INTO `arpg_monster_affix` VALUES
(1, 'Extra Strong',        'damage_up',          1,  100, 0),
(2, 'Extra Fast',          'speed_up',           1,  100, 0),
(3, 'Stoneskin',           'armor_up',           1,  100, 0),
(4, 'Fire Enchanted',      'death_explosion',    15, 100, 1),
(5, 'Cold Enchanted',      'chill_on_hit',       15, 100, 1),
(6, 'Lightning Enchanted', 'sparks_on_hit',      15, 100, 1),
(7, 'Arcane Caster',       'interruptible_nuke', 25, 60,  0);
