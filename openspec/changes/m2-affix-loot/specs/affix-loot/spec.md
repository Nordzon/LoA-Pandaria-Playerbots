# affix-loot — delta

## ADDED Requirements

### Requirement: Affixes roll on item creation
An item of quality green, blue or purple created for a real player SHALL receive affixes when it is created, with the count
set by its quality (green 1–2, blue 2–3, purple 3–5), at most 3 prefixes and 3 suffixes, and no affix twice.

#### Scenario: Blue drop
- **WHEN** a level-60 player loots a blue item
- **THEN** it has 2 or 3 affixes, each from a tier unlocked at level 60 (T3), and they are still there after relog

#### Scenario: Bot items stay vanilla
- **WHEN** `Arpg.Loot.AffixBotItems` = 0 and a bot loots a green item
- **THEN** the item has no affixes

### Requirement: Values follow the player's level
Flat stat affix values SHALL be a share of B (the epic stat budget for the slot at the player's level) within the rolled tier's range.

#### Scenario: Same item, different levels
- **WHEN** the same green item is created for a level-20 and a level-80 player with "of the Wind" forced
- **THEN** the level-80 copy's Agility is larger and both lie inside their tier ranges

### Requirement: Boss Unique pity
Each boss SHALL drop its Unique with chance `Arpg.Loot.UniquePityBase` + `Arpg.Loot.UniquePityStep` × misses (percent), per character,
resetting the counter when it drops, so it is guaranteed by the 5th kill with the defaults.

#### Scenario: Forced by pity
- **WHEN** a character's pity counter for a boss is 4 and the boss is killed
- **THEN** the Unique drops and the counter reads 0

### Requirement: God-roll pity
Every `Arpg.Loot.GodRollPityEvery`-th boss kill without a 5-affix item SHALL drop one.

#### Scenario: Third kill
- **WHEN** a character has killed two bosses since the last 5-affix item
- **THEN** the third boss kill drops an item with 5 affixes

### Requirement: Affix sell value
Selling an item with affixes to a vendor SHALL give its normal price plus
affixCount × player level × `Arpg.Loot.SellCopperPerAffixLevel` copper.

#### Scenario: Selling a 3-affix item at 90
- **WHEN** a level-90 player sells a 3-affix item whose normal price is 10 g
- **THEN** they receive 10 g + 3 × 90 × 200 copper = 15 g 40 s
