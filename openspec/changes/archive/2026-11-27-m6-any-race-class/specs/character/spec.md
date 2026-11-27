## ADDED Requirements

### Requirement: Any race with any class
Character creation SHALL allow all 165 race and class pairs; a new pair SHALL start at its race's start (Death Knights at
the shared Death Knight start) with its class's spells, action bar, start items and level stats, and the client patch
SHALL list every pair in `CharBaseInfo`.

#### Scenario: Headless Undead Monk
- WHEN `test-char-create.ps1` creates an Undead Monk
- THEN the character exists with the right race, class, level and start map, and is erased again

#### Scenario: Unusual pairs in the real client
- WHEN the player opens character creation in `Wow-64-all.exe`
- THEN every class can be picked for every race (owner check 2026-11-27: Goblin Monk made and played)
