## ADDED Requirements

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
