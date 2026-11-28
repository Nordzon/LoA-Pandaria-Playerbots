## ADDED Requirements

### Requirement: Shrines appear while clearing
With `Arpg.Shrine.Enabled` = 1, a kill by a real player SHALL raise a shrine at the corpse with `Arpg.Shrine.KillChance` %,
no more often than `Arpg.Shrine.Cooldown` seconds per player; the shrine SHALL despawn after `Arpg.Shrine.Lifetime` seconds.
The pick SHALL follow the `arpg_shrine` weights.

#### Scenario: Forced shrine
- WHEN `Arpg.Shrine.KillChance` is 100, the cooldown 0, and a test character kills a monster
- THEN the event log shows `shrine spawn id=` and a shrine object stands within 10 yards

### Requirement: Using a shrine gives a timed buff
Using a shrine SHALL give its buff (spell from `arpg_shrine`, amount and duration from the row) to the user and to group
members within 40 yards, and the shrine SHALL disappear. The Shrine of Fortune SHALL also raise magic find and gold find by
its values while the buff lasts.

#### Scenario: Shrine of Combat
- WHEN a test character uses a Shrine of Combat
- THEN `.arpg shrine info` shows its buff with `amount=15` and about 120 s left, and the shrine is gone

#### Scenario: Shrine of Fortune ends
- WHEN the Fortune buff runs out (or is removed)
- THEN its magic find and gold find lines are removed (`.arpg loot effects` back to the earlier totals)
