-- mod_arpg m3: Uniques and their signature powers (BALANCE §8b, SPEC 4.4). Idempotent.
--
-- arpg_unique_power       one row per power: trigger (0 chance on hit, 1 chance on heal, 2 when struck, 3 every Nth hit,
--                         4 passive), kind (0 damage = % of Power, 1 heal/absorb = % of max health, 2 flat), school mask,
--                         radius / targets (area and chain powers), spell for the combat log, text for the tooltip row.
-- arpg_unique_power_tier  per power and tier (1-8): trigger range (chance % or N), magnitude range (% of Power / max health),
--                         internal cooldown. Rows here = the BALANCE §8b template; a Unique may shift its own rows.
-- arpg_unique             a Unique = name + power + where it can appear: slot (InventoryType), armour subclass (0 = any),
--                         sources (bit 1 Superunique, 2 dungeon boss, 4 raid boss, 8 crafting), weight.
CREATE TABLE IF NOT EXISTS `arpg_unique_power` (
  `id` INT UNSIGNED NOT NULL, `name` VARCHAR(48) NOT NULL, `trigger` TINYINT UNSIGNED NOT NULL, `kind` TINYINT UNSIGNED NOT NULL,
  `school_mask` INT UNSIGNED NOT NULL DEFAULT 1, `radius` FLOAT NOT NULL DEFAULT 0, `max_targets` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `spell_id` INT UNSIGNED NOT NULL DEFAULT 0, `text` VARCHAR(160) NOT NULL COMMENT '{c}=chance/N, {m}=magnitude, {t}=tier (roman)',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg Unique powers';

CREATE TABLE IF NOT EXISTS `arpg_unique_power_tier` (
  `power_id` INT UNSIGNED NOT NULL, `tier` TINYINT UNSIGNED NOT NULL,
  `trigger_min` SMALLINT NOT NULL, `trigger_max` SMALLINT NOT NULL, `magnitude_min` SMALLINT NOT NULL, `magnitude_max` SMALLINT NOT NULL,
  `cooldown_ms` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`power_id`, `tier`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg Unique power tier ranges';

CREATE TABLE IF NOT EXISTS `arpg_unique` (
  `id` INT UNSIGNED NOT NULL, `name` VARCHAR(64) NOT NULL, `power_id` INT UNSIGNED NOT NULL,
  `inventory_type` TINYINT UNSIGNED NOT NULL COMMENT 'core InventoryType (robes count as chest)',
  `armor_subclass` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 any, 1 cloth, 2 leather, 3 mail, 4 plate',
  `sources` TINYINT UNSIGNED NOT NULL DEFAULT 15, `weight` INT UNSIGNED NOT NULL DEFAULT 100, `comment` VARCHAR(128) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg Uniques';

DELETE FROM `arpg_unique_power`;
INSERT INTO `arpg_unique_power` VALUES
(1, 'Keg Spill',          0, 0, 4,  8, 10, 1535, '{c}% chance on hit to unleash Keg Spill, dealing {m}% of your Power as Fire damage to enemies within 8 yards. (Tier {t})'),
(2, 'Ox Guard',           2, 1, 1,  8, 10, 0,    '{c}% chance when struck to gain Ox Guard, absorbing {m}% of your maximum health; when it breaks it hits nearby enemies for the absorbed amount. (Tier {t})'),
(3, 'Venom Burst',        3, 0, 8,  0,  1, 2818, 'Every {c}th hit unleashes Venom Burst: {m}% of your Power as Nature damage, plus poison. (Tier {t})'),
(4, 'Radiant Overflow',   1, 0, 2,  0,  1, 0,    '{c}% chance when you heal to strike your target with a Holy bolt for {m}% of your Power. (Tier {t})'),
(5, 'Static Field',       0, 0, 8, 10,  3, 421,  '{c}% chance on hit to call Static Field: Chain Lightning on 3 targets for {m}% of your Power each. (Tier {t})'),
(6, 'Long Road',          4, 2, 1,  0,  1, 0,    '+10% movement speed, above the movement speed cap.');

-- BALANCE §8b template, T1-T8 (damage: % of Power; heal/absorb: % of max health; every-Nth: N).
DELETE FROM `arpg_unique_power_tier`;
INSERT INTO `arpg_unique_power_tier` VALUES
(1,1,15,20, 60, 90,3000),(1,2,18,23, 90,130,3000),(1,3,20,25,130,170,2500),(1,4,22,28,170,210,2000),(1,5,25,30,210,260,2000),(1,6,25,30,260,300,1500),(1,7,28,32,300,340,1500),(1,8,30,35,340,400,1000),
(2,1,15,20,  3,  5,3000),(2,2,18,23,  5,  7,3000),(2,3,20,25,  7,  9,2500),(2,4,22,28,  9, 11,2000),(2,5,25,30, 11, 13,2000),(2,6,25,30, 13, 15,1500),(2,7,28,32, 15, 17,1500),(2,8,30,35, 17, 20,1000),
(3,1, 7, 8, 60, 90,   0),(3,2, 6, 7, 90,130,   0),(3,3, 6, 7,130,170,   0),(3,4, 5, 6,170,210,   0),(3,5, 5, 5,210,260,   0),(3,6, 4, 5,260,300,   0),(3,7, 4, 4,300,340,   0),(3,8, 3, 3,340,400,   0),
(4,1,15,20, 60, 90,3000),(4,2,18,23, 90,130,3000),(4,3,20,25,130,170,2500),(4,4,22,28,170,210,2000),(4,5,25,30,210,260,2000),(4,6,25,30,260,300,1500),(4,7,28,32,300,340,1500),(4,8,30,35,340,400,1000),
(5,1,15,20, 40, 65,3000),(5,2,18,23, 65, 90,3000),(5,3,20,25, 90,120,2500),(5,4,22,28,120,145,2000),(5,5,25,30,145,180,2000),(5,6,25,30,180,210,1500),(5,7,28,32,210,240,1500),(5,8,30,35,240,280,1000),
(6,1, 0, 0, 10, 10,   0),(6,2, 0, 0, 10, 10,   0),(6,3, 0, 0, 10, 10,   0),(6,4, 0, 0, 10, 10,   0),(6,5, 0, 0, 10, 10,   0),(6,6, 0, 0, 10, 10,   0),(6,7, 0, 0, 10, 10,   0),(6,8, 0, 0, 10, 10,   0);

-- First six Uniques (BALANCE §8b examples). inventory_type: 17 two-hand, 3 shoulders, 13/21 one-hand, 23 held off-hand, 11 ring, 8 feet.
DELETE FROM `arpg_unique`;
INSERT INTO `arpg_unique` VALUES
(1, 'Keg-Breaker''s Burden',        1, 17, 0, 15, 100, 'Brewmaster staff (any two-hander)'),
(2, 'Ox-Soul Mantle',               2,  3, 0, 15, 100, 'tank shoulders'),
(3, 'Twin Fangs of Venomspite',     3, 13, 0, 15, 100, 'rogue one-hander'),
(4, 'Censer of the Unseen Light',   4, 23, 0, 15, 100, 'healer held off-hand'),
(5, 'Stormcaller''s Band',          5, 11, 0, 15, 100, 'ring, anyone'),
(6, 'Boots of the Long Road',       6,  8, 0, 15, 100, 'boots, anyone');
