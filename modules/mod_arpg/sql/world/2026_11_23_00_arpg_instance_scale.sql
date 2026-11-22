-- mod_arpg m5: dungeon / raid scaling (SPEC 4.6, BALANCE §5/§6, change m5-dungeon-scaling). Idempotent.
-- Creature health / damage as a share of the full-group values by party size (bots count); between rows: linear.
-- kind 0 = 5-player instances, 1 = raids (10-man normal = 100%).
CREATE TABLE IF NOT EXISTS `arpg_party_scale` (
  `kind`    TINYINT UNSIGNED NOT NULL,
  `players` TINYINT UNSIGNED NOT NULL,
  `health`  FLOAT NOT NULL,
  `damage`  FLOAT NOT NULL,
  PRIMARY KEY (`kind`, `players`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg creature health/damage by party size';

REPLACE INTO `arpg_party_scale` VALUES
(0, 1, 0.30, 0.40), (0, 2, 0.45, 0.55), (0, 3, 0.65, 0.70), (0, 4, 0.80, 0.85), (0, 5, 1.00, 1.00),
(1, 1, 0.10, 0.25), (1, 2, 0.16, 0.32), (1, 3, 0.22, 0.40), (1, 5, 0.35, 0.55), (1, 10, 1.00, 1.00);

-- Raid tier ladder (BALANCE §6): extra 1 + 0.15 x tier on health and damage; recommended Paragon.
CREATE TABLE IF NOT EXISTS `arpg_raid_tier` (
  `map`               INT UNSIGNED NOT NULL,
  `tier`              TINYINT UNSIGNED NOT NULL,
  `recommend_paragon` SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `comment`           VARCHAR(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`map`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg raid difficulty tiers';

REPLACE INTO `arpg_raid_tier` VALUES
(409, 1, 0, 'Molten Core'), (469, 1, 0, 'Blackwing Lair'), (509, 1, 0, 'Ruins of Ahn''Qiraj'), (531, 1, 0, 'Temple of Ahn''Qiraj'),
(532, 1, 0, 'Karazhan'), (565, 1, 0, 'Gruul''s Lair'), (544, 1, 0, 'Magtheridon''s Lair'),
(548, 2, 10, 'Serpentshrine Cavern'), (550, 2, 10, 'Tempest Keep'), (534, 2, 10, 'Hyjal Summit'), (564, 2, 10, 'Black Temple'),
(580, 2, 10, 'Sunwell Plateau'), (533, 2, 10, 'Naxxramas'), (616, 2, 10, 'Eye of Eternity'), (615, 2, 10, 'Obsidian Sanctum'),
(603, 3, 25, 'Ulduar'), (649, 3, 25, 'Trial of the Crusader'), (631, 3, 25, 'Icecrown Citadel'), (724, 3, 25, 'Ruby Sanctum'),
(669, 4, 40, 'Blackwing Descent'), (671, 4, 40, 'Bastion of Twilight'), (754, 4, 40, 'Throne of the Four Winds'),
(720, 4, 40, 'Firelands'), (967, 4, 40, 'Dragon Soul'),
(1008, 5, 60, 'Mogu''shan Vaults'), (1009, 5, 60, 'Heart of Fear'), (996, 5, 60, 'Terrace of Endless Spring'),
(1098, 6, 80, 'Throne of Thunder'), (1136, 7, 110, 'Siege of Orgrimmar');

-- Test spot inside the Deadmines (map 36, 5-player instance) for 50-dungeons.gmtest.
REPLACE INTO `arpg_test_spot` (`name`, `map`, `x`, `y`, `z`, `o`, `comment`)
SELECT 'deadmines-inside', map, position_x, position_y, position_z, orientation, 'game_tele Deadmines (instance map 36)' FROM game_tele WHERE name = 'Deadmines';

-- Test spot inside Molten Core (map 409, raid tier 1) for 50-dungeons.gmtest.
REPLACE INTO `arpg_test_spot` (`name`, `map`, `x`, `y`, `z`, `o`, `comment`)
SELECT 'molten-core-inside', map, position_x, position_y, position_z, orientation, 'game_tele MC (raid map 409, tier 1)' FROM game_tele WHERE name = 'MC';
