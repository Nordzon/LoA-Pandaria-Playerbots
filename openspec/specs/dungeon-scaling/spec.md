# dungeon-scaling Specification

## Purpose
TBD - created by archiving change m5-dungeon-scaling. Update Purpose after archive.

## Requirements

### Requirement: Party-size scaling in dungeons and raids
Instance creatures SHALL scale by the number of players in the instance, bots included, from the `arpg_party_scale`
table (5-man solo 30% health / 40% damage up to 100% at five; raids from 10% / 25% solo), times the raid's tier
multiplier `1 + 0.15 × tier` from `arpg_raid_tier`.

#### Scenario: Solo in a 5-man
- WHEN one player enters the Deadmines
- THEN its creatures have 30% health and 40% damage (`50-dungeons.gmtest`)

#### Scenario: Player and one bot
- WHEN a player and a bot of its group are in the Deadmines
- THEN its creatures have 45% health and 55% damage (`54-bots.gmtest`)

#### Scenario: Solo raid
- WHEN one player enters Molten Core (raid tier 1)
- THEN its creatures have 11.5% health and 28.75% damage

### Requirement: Solo roles
A lone player in a 5-man SHALL get the role factor of its spec: non-tank specs face creature damage ×
`Arpg.Dungeon.SoloNonTankDamage`; tanks deal × `SoloTankDamageDealt`; healers deal × `SoloHealerDamageDealt`.

#### Scenario: A damage spec soloes
- WHEN a non-tank character is alone in a 5-man
- THEN the creature damage carries the 0.75 factor

### Requirement: Dungeon level selector
A player SHALL be able to choose the level a dungeon runs at (the dungeon's minimum level up to its own level),
remembered per dungeon; creatures and loot SHALL follow the chosen level.

#### Scenario: Deadmines at level 20
- WHEN a level-90 character sets the Deadmines to level 20 and enters
- THEN the instance runs at level 20 (`instance level map=36 level=20`)

### Requirement: Ascended affix tiers need raids and Paragon
Affix tiers T6, T7 and T8 SHALL roll only on raid drops of raid tier 3, 5 and 7 when the character has at least
Paragon 25, 60 and 110 respectively.

#### Scenario: Paragon 59 in a tier-5 raid
- WHEN a Paragon-59 character gets a boss item in a tier-5 raid
- THEN its affixes roll at most T6
