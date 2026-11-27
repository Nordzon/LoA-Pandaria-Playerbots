## ADDED Requirements

### Requirement: Potions heal a share of max health
An ARPG potion SHALL heal its tier's percentage of the drinker's max health (10, 12, 14, 16, 18, 20%), at least 1, replacing
the item's flat heal.

#### Scenario: Minor potion at level 90
- WHEN a character with 400,000 max health drinks a tier-1 potion
- THEN it is healed by 40,000

#### Scenario: Major potion at level 10
- WHEN a character with 1,000 max health drinks a tier-6 potion
- THEN it is healed by 200

### Requirement: Stronger monsters drop stronger potions
A kill SHALL drop at most one potion, with the tier and chance of the monster's kind: normal 1 (~5%), elite 2 (~15%),
Champion 3 (50%), Superunique 5 (100%), dungeon or world boss 6 (100%); chances in `arpg_potion`.

#### Scenario: Superunique always drops one
- WHEN a character kills a Superunique
- THEN its loot holds exactly one tier-5 potion

#### Scenario: Champion drop forced
- WHEN the Champion potion chance is set to 100% and a Champion is killed
- THEN its loot holds a tier-3 potion

### Requirement: Short shared cooldown
All ARPG potions SHALL share one cooldown of `Arpg.Potion.Cooldown` seconds, on the server and in the client (patched DBC),
in and out of combat.

#### Scenario: Second drink blocked
- WHEN a character drinks a potion and tries another 2 seconds later
- THEN the second is refused (cooldown) and health is unchanged

#### Scenario: Ready again
- WHEN the cooldown has passed
- THEN the next potion heals normally
