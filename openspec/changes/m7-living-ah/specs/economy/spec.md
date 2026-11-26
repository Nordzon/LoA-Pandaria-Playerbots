## ADDED Requirements

### Requirement: The auction house lists materials and some affixed gear
With the AuctionHouseBot seller on, the auction house SHALL list profession materials, and `Arpg.AH.AffixShare` % of its
green-epic gear listings (without stock random stats) SHALL carry affixes rolled at the item's level tier plus
`Arpg.AH.TierOffset`, priced by the sell formula x `Arpg.AH.AffixMarkup`.

#### Scenario: An affixed listing
- WHEN the seller lists a level-20 green with the share forced to 100%
- THEN the listed item has 1-2 affixes of tier 2 or lower and a buyout of at least 4x its plain sell value
