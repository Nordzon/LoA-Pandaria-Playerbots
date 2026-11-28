## ADDED Requirements

### Requirement: Treasure Goblins appear and flee
With `Arpg.Goblin.Enabled` = 1, a real player's open-world kill SHALL summon a Treasure Goblin with `Arpg.Goblin.KillChance`
% (per-player cooldown `Arpg.Goblin.Cooldown`). The goblin SHALL never attack, SHALL flee once hit or approached, and SHALL
despawn without loot `Arpg.Goblin.EscapeSeconds` after the first hit.

#### Scenario: Goblin escapes
- WHEN `Arpg.Goblin.EscapeSeconds` is 3 and a test character hits a spawned goblin without killing it
- THEN after 5 s `.arpg goblin info` reports `alive=0 escaped=1`

### Requirement: A killed goblin showers loot
A killed Treasure Goblin SHALL drop gold (`Arpg.Goblin.GoldPerLevel` x level), `Arpg.Goblin.Items` gear items with affixes,
salvage essences and, by chance, a catalyst, a charm and the Unique charm Goblin's Lucky Coin.

#### Scenario: Goblin killed
- WHEN a test character kills a spawned goblin and loots it
- THEN the kill event shows `goblin killed gold=[1-9]` with `items>=4`, and the bags gain at least 4 affixed items
