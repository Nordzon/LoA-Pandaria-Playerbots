# character Specification

## Purpose
TBD - created by archiving change m6-any-race-class. Update Purpose after archive.

## Requirements

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

### Requirement: Any class can use any weapon or shield
With `Arpg.AnyWeapon.Enabled` = 1, every real player SHALL know every weapon proficiency and Shield (spell and skill), so
the server and the client let any class equip any weapon type and a shield. Bots SHALL get them only with
`Arpg.AnyWeapon.Bots` = 1; Dual Wield SHALL be granted only with `Arpg.AnyWeapon.DualWield` = 1.

#### Scenario: A warrior equips a wand
- WHEN the warrior test character logs in with the feature on
- THEN `.arpg test weapons <char> 5069` reports 15 proficiencies and the Fire Wand as equippable

#### Scenario: A caster equips a shield
- WHEN a mage logs in with the feature on
- THEN a shield and a two-handed axe in the bags are not shown red and can be equipped
