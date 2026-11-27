# professions — delta

## ADDED Requirements

### Requirement: Learn every primary profession
A character SHALL be able to learn all 11 primary professions (Alchemy, Blacksmithing, Enchanting, Engineering,
Herbalism, Inscription, Jewelcrafting, Leatherworking, Mining, Skinning, Tailoring) as well as all secondary ones.

#### Scenario: Third profession
- **WHEN** a character with Mining and Herbalism talks to an Alchemy trainer
- **THEN** they can learn Alchemy

#### Scenario: All eleven
- **WHEN** a character has learned all 11 primary professions
- **THEN** each one gains skill normally from crafting or gathering

### Requirement: Every profession reachable in the UI
The ArpgUI addon SHALL list every learned profession with its skill level, and clicking one SHALL open that profession's window.

#### Scenario: Beyond the spellbook slots
- **WHEN** a character has 5 primary professions
- **THEN** all 5 appear in the ArpgUI professions panel and each opens its window
