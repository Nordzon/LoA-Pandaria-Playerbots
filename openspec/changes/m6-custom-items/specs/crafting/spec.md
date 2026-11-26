## ADDED Requirements

### Requirement: Catalysts strengthen the next craft
A character carrying a catalyst SHALL have one used up automatically when a craft rolls affixes; that craft SHALL roll one
affix tier higher and with twice the Unique chance.

#### Scenario: Craft with a catalyst in the bags
- WHEN a character with a Lesser Catalyst crafts an item that rolls affixes
- THEN the catalyst count drops by one and the roll reports catalyst=1

### Requirement: Salvage turns affixed items into essences
The companion's Salvage option SHALL destroy the listed affixed items (not equipped, not protected) after a confirmation and
SHALL give essences by affix count; essences SHALL buy catalysts in the same dialogue.

#### Scenario: Salvage two items
- WHEN a character salvages a 2-affix and a 3-affix item
- THEN both items are gone and 5 essences are added
