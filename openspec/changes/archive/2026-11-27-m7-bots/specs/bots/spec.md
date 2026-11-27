## ADDED Requirements

### Requirement: Random bots live alongside the player
The server SHALL run random bots (500 by default, `write-confs.ps1`), and ARPG features SHALL treat bots as non-real
players: bot fights don't change world scaling, and bots' own items get no affixes.

#### Scenario: A bot's fight keeps its level
- WHEN a bot fights a mob that was scaled to the player's level and the player's level then changes
- THEN the mob keeps its level until the fight ends, then scales again (`54-bots.gmtest`)

#### Scenario: A bot's own item
- WHEN a bot gets an item for itself
- THEN the item has no affixes

### Requirement: Dungeon Finder at any level
A real player SHALL be able to queue for any dungeon at any level, with bots matched to the player's level ±3; the dungeon
SHALL scale to the group (m5).

#### Scenario: Level-50 player queues the Deadmines
- WHEN a level-50 character queues for the Deadmines
- THEN bots of level 47-53 fill the group and a proposal follows (`51-dungeon-finder.gmtest`)

### Requirement: Party bots understand affixes
A bot in a real player's group SHALL score items with their affix stats (also planned affixes of loot on a corpse), and
its equipped greens to epics SHALL roll affixes once when it joins, `Arpg.Bots.PartyAffixTierOffset` tiers below the player.

#### Scenario: A party bot values an affixed blue
- WHEN a party bot scores an affixed blue
- THEN its score with affixes is not lower than the template-only score, and `Arpg.Bots.AffixScoring` 0 turns it off
  (`52-bot-affixes.gmtest`)
