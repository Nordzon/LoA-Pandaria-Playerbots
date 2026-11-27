## ADDED Requirements

### Requirement: New race/class pairs can follow their starting zone's quests
With `Arpg.StartQuests.Enabled` = 1, a character of a race/class pair Blizzard never had SHALL be able to take the Warrior's
class quests of its race's starting zone (rows in `arpg_quest_proxy`), and those given or ended by a class trainer SHALL be
complete as soon as they are accepted. Characters of Blizzard's own pairs SHALL see no change.

#### Scenario: Night Elf Paladin after Demonic Thieves
- WHEN a Night Elf Paladin has handed in "Demonic Thieves"
- THEN Ilthalaine offers "Simple Sigil", and `.arpg test questproxy 4 2 3116` reports `allowed=1 autocomplete=1`

#### Scenario: Stock pairs unchanged
- WHEN a Human Warrior is checked against the Hunter's "Beating Them Back!" (28767)
- THEN `.arpg test cantake` reports `class_ok=0`
