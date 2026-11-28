## ADDED Requirements
### Requirement: Profession trainers on the companion
The companion SHALL offer, for each profession the character knows, a trainer window with every rank and recipe any trainer
of that profession teaches, and learning from it SHALL work like a normal trainer.

#### Scenario: Learn from the companion
- WHEN a character with Leatherworking skill 20 buys recipe 2153 from the companion's Leatherworking trainer
- THEN it knows the recipe and paid the trainer price

### Requirement: Reputation recipes for gold
Each profession SHALL have vendor pages selling its reputation, token and daily-quest recipes for gold, without reputation
requirements, priced from `arpg_recipe_price`.

#### Scenario: Leatherworking pages
- WHEN the recipe shop is loaded
- THEN Leatherworking has 2 pages and Jewelcrafting 3
