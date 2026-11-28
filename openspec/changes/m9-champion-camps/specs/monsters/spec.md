## ADDED Requirements
### Requirement: Champion camps
Every open-world square of `Arpg.Champion.CampSize` yards holding at least `Arpg.Champion.CampMin` eligible monsters SHALL
hold one Champion; when it is gone the square SHALL get a new one after `Arpg.Champion.CampRespawn` seconds.

#### Scenario: A camp gets its Champion
- WHEN a real player comes near a square with enough eligible monsters and no Champion
- THEN one of them becomes a Champion, and asking again makes no second one

#### Scenario: Respawn delay
- WHEN the square's Champion is killed and CampRespawn is 600
- THEN the square has no Champion and waits about 600 seconds

### Requirement: Never Champions
NPCs with a menu or quest flag, scripted story NPCs, monsters friendly to the player, critters, rares, bosses and entries in
`arpg_champion_exclude` SHALL never become Champions.

#### Scenario: Quest giver
- WHEN a quest giver is near a camp
- THEN it is not eligible

### Requirement: Superuniques in every zone
Every open-world zone with hostile monsters SHALL have a Superunique pool of at least 5 (its rares first, then its toughest
ordinary hostile monsters), with up to `Arpg.Superunique.PerZone` (3) alive.

#### Scenario: Wandering Isle
- WHEN a player is on the Wandering Isle
- THEN "Unique" monsters from the island's own monsters spawn there (manual check)
