# m1-world-scaling

## Why
Old zones become pointless once you outlevel them. Raising nearby creatures to your level keeps every zone worth playing, without disturbing the bots questing in the same zone.

## What changes
- New module `modules/mod_arpg` with config loading, `.arpg reload`, `.arpg version`, `.arpg config set`, `.arpg test setup`.
- Per-creature, upward-only level scaling for open-world creatures near the real player.
- `.arpg scale probe` debug command; unit tests for the scaling formula; `10-world-scaling.gmtest`.
- Quests set to scale with the player (quest level -1).

## Config / SQL added
`Arpg.Scale.Enabled` (1), `Arpg.Scale.LevelOffset` (0), `Arpg.Scale.Radius` (visibility range),
`Arpg.Scale.HealthMod` (1.0), `Arpg.Scale.DamageMod` (1.0); table `arpg_test_spot`.

## Out of scope
Dungeon scaling (m5). Quest reward items.

## Milestone
1
