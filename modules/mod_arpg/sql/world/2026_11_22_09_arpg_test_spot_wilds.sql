-- mod_arpg: a test spot in Elwynn Forest away from town guards (m3 31-superuniques: Goldshire guards take a Superunique's
-- aggro, which is right in play but makes the ability check depend on them). Gruff Swiftbite's spawn point (creature guid
-- 3906, zone 12). Idempotent.
REPLACE INTO `arpg_test_spot` (`name`, `map`, `x`, `y`, `z`, `o`, `comment`)
SELECT 'elwynn-wilds', `map`, `position_x`, `position_y`, `position_z`, `orientation`, 'Gruff Swiftbite spawn, Elwynn west (no guards)'
FROM `creature` WHERE `guid` = 3906;
