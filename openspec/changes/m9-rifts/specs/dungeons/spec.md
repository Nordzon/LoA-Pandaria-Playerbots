## ADDED Requirements

### Requirement: Opening a rift
A character SHALL be able to open a rift at any level from 1 to its key level (companion menu, `.rift open`,
`.arpg rift open`); the next 5-man dungeon it enters SHALL run as that rift, with the timer starting on entry.

#### Scenario: Level 3 rift in the Deadmines
- WHEN a test character with key 5 opens level 3 and enters the Deadmines
- THEN `.arpg rift info` shows `active=1 level=3 map=36` and the instance-scale event shows `rift=3`

#### Scenario: Level above the key
- WHEN a character with key 2 tries to open level 4
- THEN it is refused and nothing is pending

### Requirement: Rifts scale with their level
Inside a rift, monster health SHALL be multiplied by `Arpg.Rift.HealthGrowth`^level and damage by
`Arpg.Rift.DamageGrowth`^level, on top of party-size scaling; from `Arpg.Rift.AffixLevel` a run SHALL be Fortified or
Tyrannical.

#### Scenario: Level 3 multipliers
- WHEN a level 3 rift is active with growth 1.10 / 1.07
- THEN `.arpg rift info` shows `health=1.331 damage=1.225` (before Fortified/Tyrannical)

### Requirement: Finishing a rift moves the key
Killing the dungeon's final boss SHALL finish the rift: elapsed time plus `Arpg.Rift.DeathPenalty` per death against the
par time SHALL raise the key by 3 (<= 60% of par), 2 (<= 80%), 1 (<= 100%) or 0 (late); the best level SHALL be kept.

#### Scenario: Fast clear
- WHEN a level 3 rift with par 1500 s finishes after 800 s
- THEN the key becomes 6 and the reply shows `upgrade=3`

#### Scenario: Late clear
- WHEN it finishes after 1700 s
- THEN the key stays, `upgrade=0`

### Requirement: Rift rewards
A finished rift SHALL give `Arpg.Rift.RewardItems` + level/3 gear items (max 8; half when late) rolled as boss loot, gold,
Paragon XP at the max level, and by chance a charm (not when late) and at level 10+ the Unique charm Riftwalker's Sigil.

#### Scenario: Reward items
- WHEN a level 3 rift is finished in time
- THEN the character's bags gain 3 affixed items
