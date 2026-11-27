# paragon Specification

## Purpose
TBD - created by archiving change m4-paragon. Update Purpose after archive.

## Requirements

### Requirement: Paragon levels after the level cap
At the maximum player level, experience from kills and quests SHALL go to Paragon experience (`character_paragon`); the
experience for the next Paragon level SHALL be `Arpg.Paragon.XpBase × Arpg.Paragon.XpGrowth^level`, and one gain MAY
give several levels.

#### Scenario: XP at 90 becomes Paragon levels
- WHEN a level-90 character gains experience
- THEN its Paragon experience rises and `.arpg paragon info <char>` shows the new Paragon level (`40-paragon.gmtest`)

### Requirement: Paragon points rotate through four tabs
Each Paragon level SHALL give one point to the next tab in the order Core, Offense, Defense, Utility; the player SHALL
choose the stat inside the tab, the bonus SHALL apply at once and at login, each stat SHALL respect its cap, and a respec
SHALL be free (`Arpg.Paragon.RespecGold`).

#### Scenario: Spending a point changes the stat exactly
- WHEN a character spends a Core point on strength
- THEN its strength rises by exactly the configured bonus per point, and a respec returns the point and removes the bonus

### Requirement: World tier in uncapped zones
In zones without a level cap (the Vale of Eternal Blossoms), creature health, damage and experience SHALL grow with the
character's Paragon level by `Arpg.Paragon.WorldTierPct` per level, up to `Arpg.Paragon.WorldTierCapPct`.

#### Scenario: Paragon raises the Vale's difficulty
- WHEN a Paragon character stands in the Vale of Eternal Blossoms
- THEN the scaled creatures there carry the world tier multiplier for its Paragon level (`40-paragon.gmtest`)
