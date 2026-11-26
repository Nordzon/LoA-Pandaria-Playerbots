# m7-living-ah

## Why
SPEC 4.10 / owner 2026-11-26 ("living auction house: yes, materials first"): the auction house should have things to buy -
profession materials for the crafting wheel, and now and then an affixed piece of gear - without other players.

## What changes
- **Materials (config only):** the core's AuctionHouseBot seller on (write-confs), weighted to trade goods, gems, glyphs,
  consumables and recipes; fewer gear listings; prices from the core's own rules.
- **Affixed gear (one marked hook):** `ArpgHooks::AuctionBotItem(Item*, buyout&, bid&)` right after the seller prices a new
  listing (AuctionHouseBotSeller.cpp, // ARPG:). mod_arpg rolls affixes on `Arpg.AH.AffixShare` % (30) of green-epic gear
  listings without stock random stats, at the tier the item's required level unlocks plus `Arpg.AH.TierOffset` (-1), and
  prices it by the sell formula x `Arpg.AH.AffixMarkup` (4).
- Rolling without a player: a roll path that takes the level from the item (required level, else item level band) instead
  of a character; no owner, no test events.
- `.arpg test ah list|roll <itemId>` + a GM test.

## Not in scope
- The AuctionHouseBot buyer (bidding on the player's listings) - later, if the owner wants a market for his own loot.
