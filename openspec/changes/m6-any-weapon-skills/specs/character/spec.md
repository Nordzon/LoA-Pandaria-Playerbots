## ADDED Requirements

### Requirement: Class abilities work with any melee weapon
With `Arpg.AnyWeapon.Skills` = 1, every class spell whose weapon requirement includes a melee weapon SHALL accept any melee
weapon type in any hand (no dagger, one-hand, two-hand, main-hand or off-hand limit). Spells listed in
`Arpg.AnyWeapon.MainHandSpells` SHALL be usable without an off-hand weapon and hit with the main-hand weapon. Requirements for
a shield, for armour, or for a ranged weapon only SHALL stay unchanged. The client patch SHALL carry the same requirements.

#### Scenario: Backstab with a two-handed sword
- WHEN a character with a two-handed sword in the main hand is checked against Backstab (53)
- THEN `.arpg test spellreq <char> 53 <2H sword>` reports `fits=1`

#### Scenario: Lava Lash with a staff
- WHEN Lava Lash (60103) is checked
- THEN it reports `offhand=0` and `fits=1` for a staff

#### Scenario: Shield and ranged requirements stay
- WHEN Shield Slam (23922) is checked against a sword, and a Hunter shot against a sword
- THEN both report `fits=0`
