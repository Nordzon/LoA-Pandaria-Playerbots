## ADDED Requirements

### Requirement: Typed blank bases
The server SHALL provide blank copies of droppable gear that keep only their armour type's main stat and Stamina, and a blank
that drops or is bought SHALL roll the most affixes its quality allows.

#### Scenario: A blank drops
- WHEN `Arpg.Blank.DropChance` is 100 and a Champion with a green and a blue floor item is killed
- THEN both items are blanks of their slot and quality, and each rolls its quality's maximum affix count (2 / 3)

#### Scenario: Blank mapping follows the loot level and spec
- WHEN a blank is chosen for a level-20 agility character and a leather chest of blue quality
- THEN it is the Agility variant of the 20 bracket's leather chest blank

### Requirement: Crafted gear comes out as a blank
With `Arpg.Blank.Crafted` on, a tradeskill recipe that creates gear SHALL create the blank of that item, rolled with the
crafting wheel's rules.

#### Scenario: Crafting a chest
- WHEN a tailor crafts a cloth chest
- THEN the created item is that chest's blank and rolls affixes as a craft

### Requirement: The companion sells blanks for essences
The companion dialogue SHALL offer blank bases by slot and quality for `Arpg.Blank.EssenceCost.*` essences.

#### Scenario: Buying a blue blank
- WHEN a character with 15 essences buys a blue chest blank
- THEN 15 essences are taken, the blank of the character's armour type and spec is added, and it has 3 affixes
