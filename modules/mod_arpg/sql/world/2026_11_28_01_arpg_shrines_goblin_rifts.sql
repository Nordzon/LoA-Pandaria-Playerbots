-- mod_arpg m9-shrines, m9-goblins, m9-rifts. Idempotent.

-- ---------------------------------------------------------------------------------------------------------------------------
-- Shrines: one game object per shrine (goober, the Pandaren "Shrine of the Seeker" look, display 11506), script arpg_shrine.
-- The buff is an existing client spell whose effect-0 amount (amount, 0 = keep) and duration (duration_s) we set, so its
-- icon and tooltip show the real numbers. effect1/2: our affix effects (Fortune: magic find, gold find) while it lasts.
CREATE TABLE IF NOT EXISTS `arpg_shrine` (
  `id` INT UNSIGNED NOT NULL,
  `name` VARCHAR(40) NOT NULL,
  `go_entry` INT UNSIGNED NOT NULL,
  `spell` INT UNSIGNED NOT NULL,
  `amount` INT NOT NULL DEFAULT 0,
  `effect1` VARCHAR(32) NOT NULL DEFAULT '',
  `value1` INT NOT NULL DEFAULT 0,
  `effect2` VARCHAR(32) NOT NULL DEFAULT '',
  `value2` INT NOT NULL DEFAULT 0,
  `duration_s` INT UNSIGNED NOT NULL DEFAULT 120,
  `weight` INT UNSIGNED NOT NULL DEFAULT 10,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
DELETE FROM `arpg_shrine`;
INSERT INTO `arpg_shrine` (`id`, `name`, `go_entry`, `spell`, `amount`, `effect1`, `value1`, `effect2`, `value2`, `duration_s`, `weight`) VALUES
  (1, 'Frenzied Shrine',      800201, 127466,  20, '', 0, '', 0, 120, 20),   -- Ancient Magic: haste
  (2, 'Shrine of Combat',     800202, 139368,  15, '', 0, '', 0, 120, 20),   -- Power Overwhelming: damage done
  (3, 'Shrine of Protection', 800203, 106213, -20, '', 0, '', 0, 120, 15),   -- Blood of Neltharion: damage taken
  (4, 'Fleeting Shrine',      800204,  57727,  30, '', 0, '', 0, 120, 15),   -- Fate Rune of Fleet Feet: run speed
  (5, 'Enlightened Shrine',   800205,  95988,  25, '', 0, '', 0, 120, 15),   -- Reverence for the Flame: kill XP
  (6, 'Shrine of Fortune',    800206, 118496,   0, 'magic_find_pct', 5000, 'gold_find_pct', 5000, 120, 15);  -- Made-a-Wish (icon)

DROP TEMPORARY TABLE IF EXISTS `arpg_tmp_go`;
CREATE TEMPORARY TABLE `arpg_tmp_go` LIKE `gameobject_template`;
INSERT INTO `arpg_tmp_go` SELECT * FROM `gameobject_template` WHERE `entry` = 211597;
UPDATE `arpg_tmp_go` SET `name` = 'Shrine', `IconName` = 'Interact', `castBarCaption` = '', `size` = 1.4,
  `questItem1` = 0, `questItem2` = 0, `questItem3` = 0, `questItem4` = 0, `questItem5` = 0, `questItem6` = 0,
  `data0` = 0, `data1` = 0, `data2` = 0, `data3` = 3000, `data4` = 0, `data5` = 0, `data6` = 0, `data7` = 0, `data8` = 0,
  `data9` = 0, `data10` = 0, `data11` = 0, `data12` = 0, `data13` = 0, `data14` = 0, `data15` = 0, `data16` = 0,
  `data17` = 0, `data18` = 0, `data19` = 0, `data20` = 0, `data21` = 0, `data22` = 0, `data23` = 0,
  `AIName` = '', `ScriptName` = 'arpg_shrine';
DELETE FROM `gameobject_template` WHERE `entry` BETWEEN 800201 AND 800206;
UPDATE `arpg_tmp_go` SET `entry` = 800201, `name` = 'Frenzied Shrine' WHERE `entry` = 211597;
INSERT INTO `gameobject_template` SELECT * FROM `arpg_tmp_go`;
UPDATE `arpg_tmp_go` SET `entry` = 800202, `name` = 'Shrine of Combat' WHERE `entry` = 800201;
INSERT INTO `gameobject_template` SELECT * FROM `arpg_tmp_go`;
UPDATE `arpg_tmp_go` SET `entry` = 800203, `name` = 'Shrine of Protection' WHERE `entry` = 800202;
INSERT INTO `gameobject_template` SELECT * FROM `arpg_tmp_go`;
UPDATE `arpg_tmp_go` SET `entry` = 800204, `name` = 'Fleeting Shrine' WHERE `entry` = 800203;
INSERT INTO `gameobject_template` SELECT * FROM `arpg_tmp_go`;
UPDATE `arpg_tmp_go` SET `entry` = 800205, `name` = 'Enlightened Shrine' WHERE `entry` = 800204;
INSERT INTO `gameobject_template` SELECT * FROM `arpg_tmp_go`;
UPDATE `arpg_tmp_go` SET `entry` = 800206, `name` = 'Shrine of Fortune' WHERE `entry` = 800205;
INSERT INTO `gameobject_template` SELECT * FROM `arpg_tmp_go`;
DROP TEMPORARY TABLE `arpg_tmp_go`;
DELETE FROM `gameobject_template_addon` WHERE `entry` BETWEEN 800201 AND 800206;

-- ---------------------------------------------------------------------------------------------------------------------------
-- Treasure Goblin: creature 1000010, a copy of Venture Co. Strip Miner (674) with the client's own Treasure Goblin model
-- (51994, from the pet 74413). Faction 7 (neutral, attackable, never aggressive); the AI (arpg_treasure_goblin) flees.
DROP TEMPORARY TABLE IF EXISTS `arpg_tmp_cre`;
CREATE TEMPORARY TABLE `arpg_tmp_cre` LIKE `creature_template`;
INSERT INTO `arpg_tmp_cre` SELECT * FROM `creature_template` WHERE `entry` = 674;
UPDATE `arpg_tmp_cre` SET `entry` = 1000010, `name` = 'Treasure Goblin', `subname` = '', `minlevel` = 1, `maxlevel` = 1,
  `faction` = 7, `npcflag` = 0, `npcflag2` = 0, `speed_walk` = 1.2, `speed_run` = 1.25, `scale` = 1.2, `rank` = 0,
  `lootid` = 0, `pickpocketloot` = 0, `skinloot` = 0, `mingold` = 0, `maxgold` = 0, `KillCredit1` = 0, `KillCredit2` = 0,
  `gossip_menu_id` = 0, `AIName` = '', `MovementType` = 0, `unit_flags` = 0, `flags_extra` = 0, `Health_mod` = 1,
  `spell1` = 0, `spell2` = 0, `spell3` = 0, `spell4` = 0, `ScriptName` = 'arpg_treasure_goblin';
DELETE FROM `creature_template_model` WHERE `CreatureID` = 1000010;
DELETE FROM `creature_template` WHERE `entry` = 1000010;
INSERT INTO `creature_template` SELECT * FROM `arpg_tmp_cre`;
DROP TEMPORARY TABLE `arpg_tmp_cre`;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
  VALUES (1000010, 0, 51994, 1, 1, 18414);
DELETE FROM `creature_equip_template` WHERE `CreatureID` = 1000010;

-- ---------------------------------------------------------------------------------------------------------------------------
-- Rifts: par time per dungeon (seconds); other 5-man dungeons use Arpg.Rift.DefaultParMinutes.
CREATE TABLE IF NOT EXISTS `arpg_rift_dungeon` (
  `map` INT UNSIGNED NOT NULL,
  `name` VARCHAR(60) NOT NULL,
  `par_s` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`map`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
DELETE FROM `arpg_rift_dungeon`;
INSERT INTO `arpg_rift_dungeon` (`map`, `name`, `par_s`) VALUES
  (36,   'The Deadmines',            1500),
  (959,  'Shado-Pan Monastery',      1500),
  (960,  'Temple of the Jade Serpent', 1500),
  (961,  'Stormstout Brewery',       1320),
  (962,  'Gate of the Setting Sun',  1320),
  (994,  'Mogu''shan Palace',        1200),
  (1001, 'Scarlet Halls',            1200),
  (1004, 'Scarlet Monastery',        1200),
  (1007, 'Scholomance',              1500),
  (1011, 'Siege of Niuzao Temple',   1500);
