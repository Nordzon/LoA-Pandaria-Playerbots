## ADDED Requirements
### Requirement: Rare-exclusive loot floor
Items that only a few creatures (at most 3) drop, listed directly and ungrouped in the loot of a rare in a Superunique pool,
SHALL drop at least 30% of the time; quest items and shared loot stay as they are.

#### Scenario: Mount from a Pandaria rare
- WHEN the SQL is applied
- THEN Reins of the Thundering Onyx Cloud Serpent (stock 1%) is at 30% and the stock value is kept in arpg_rare_loot_floor
