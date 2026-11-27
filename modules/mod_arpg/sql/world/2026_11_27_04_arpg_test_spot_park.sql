-- m8-friends: where the second test character (Arpgtesttwo) waits between tests - Stormwind (game_tele "Stormwind"): no test
-- runs there and friendly city NPCs are never scaled, so it can't change what Arpgtest's tests see. Idempotent.
REPLACE INTO `arpg_test_spot` (`name`, `map`, `x`, `y`, `z`, `o`, `comment`)
VALUES ('park-stormwind', 0, -8833.07, 622.778, 93.9317, 0.6771, 'm8: parking spot for Arpgtesttwo between tests');
