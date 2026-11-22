-- mod_arpg m4: a test spot in the Vale of Eternal Blossoms (zone 5840, uncapped: world tier), 45 yards from a fixed probe (outside its aggro range: the scaling tick leaves monsters in combat alone)
-- (Krik'thik Warrior, creature guid 523035, level 90). Idempotent.
REPLACE INTO `arpg_test_spot` (`name`, `map`, `x`, `y`, `z`, `o`, `comment`)
SELECT 'vale-krikthik', `map`, `position_x` + 45, `position_y`, `position_z` + 1, `orientation`, 'Vale of Eternal Blossoms, near Krik''thik Warrior 523035'
FROM `creature` WHERE `guid` = 523035;
