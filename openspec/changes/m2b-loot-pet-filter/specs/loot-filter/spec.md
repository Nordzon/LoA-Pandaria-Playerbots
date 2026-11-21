# loot-filter — delta

## ADDED Requirements

### Requirement: Ordered rules per character
Each character SHALL have an ordered list of rules. A rule matches on any combination of: quality, "not usable
by my class", item class and subclass, affix count, best affix tier, item level compared to the equipped item
in that slot, and item ID lists. The first matching rule SHALL decide the action, and items that match no rule SHALL be picked up.

#### Scenario: Greys sold
- **WHEN** a rule says quality = grey → Auto-sell, and the pet loots a grey item
- **THEN** the item never enters the bags, and the player gains its vendor price in gold

#### Scenario: Unusable armour left
- **WHEN** a plate wearer has the rule "not usable by my class → Leave"
- **THEN** cloth items stay on the corpse

### Requirement: Actions
The actions SHALL be Pick up, Leave, Auto-sell and Highlight. Highlight SHALL mark the corpse with a visible
beam (an existing client visual) until it is looted.

#### Scenario: Rare highlighted
- **WHEN** a corpse holds an item with a T5 affix and a Highlight rule matches it
- **THEN** the corpse shows a beam and the item is picked up when looted

### Requirement: Protected items
Quest items, Uniques and gold SHALL always be picked up, whatever the rules say.

#### Scenario: Bad rule
- **WHEN** a rule says "orange quality → Auto-sell"
- **THEN** a Unique is still picked up

### Requirement: Applies to all looting
The filter SHALL apply both to pet looting and to the player's own looting.

#### Scenario: Looting by hand
- **WHEN** the player opens a corpse's loot window and a rule says Leave for an item
- **THEN** that item is not moved into the bags by auto-loot
