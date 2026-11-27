# world-scaling — delta

## ADDED Requirements

### Requirement: Upward-only scaling near the player
An open-world creature within `Arpg.Scale.Radius` of the real player SHALL take level
max(its normal level, player level + `Arpg.Scale.LevelOffset`), with stats recalculated for that level
and multiplied by `Arpg.Scale.HealthMod` and `Arpg.Scale.DamageMod`.

#### Scenario: Low-level mob raised to the player
- **WHEN** a level-85 player stands in Northshire with offset 0
- **THEN** a nearby level-5 wolf reports level 85

#### Scenario: Higher-level zone stays higher
- **WHEN** a level-80 player enters the Jade Forest
- **THEN** Jade Forest creatures keep their normal levels

### Requirement: Bots are not disturbed
A creature already in combat with a bot SHALL keep its current level until it resets or respawns, and creatures
outside `Arpg.Scale.Radius` of the real player SHALL keep their normal level.

#### Scenario: Bot's fight keeps its level
- **WHEN** a level-22 bot is fighting a level-22 mob and a level-85 player arrives
- **THEN** that mob stays level 22 until it resets or respawns

### Requirement: Revert on reset
A scaled creature SHALL return to its normal level when it resets or respawns while no real player is within range.

#### Scenario: Player leaves
- **WHEN** the player leaves and a scaled mob respawns
- **THEN** it has its normal level

### Requirement: Kill switch
With `Arpg.Scale.Enabled = 0`, no creature SHALL be scaled, and the change SHALL apply after `.arpg reload` without a restart.

#### Scenario: Disabled
- **WHEN** scaling is disabled and reloaded
- **THEN** `.arpg scale probe` reports `scaled=none` for every creature
