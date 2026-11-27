# m6-custom-items (+ m1b 2b.3 catalysts and salvage)

## Why
Owner 2026-11-26 (all "as recommended"): crafting gets catalysts (from Champions/Superuniques, +1 affix tier and x2 Unique
chance on the next craft, used up automatically) and salvage (affixed items -> essences -> buy catalysts). These need their own
items; custom item ids carry our names without a client patch.

## What changes
- **Custom items (ids 700000+):** `item_template` rows + `hotfix_data` (Item 1344507586 and Item-sparse 2442913102). The server
  already answers the client's item queries from `item_template`, so a new id shows its name, icon and tooltip in the real
  client; models/icons are reused from existing items.
- **Items in v1:** Catalysts by tier band (Lesser T1-T2, Greater T3-T4, Grand T5+; the band of the dropping monster's tier),
  one Essence item (stack 999).
- **Drops:** a Champion leader `Arpg.Catalyst.ChampionChance` (25%), a Superunique `Arpg.Catalyst.SuperuniqueChance` (100%).
- **Craft:** a crafted item that rolls affixes uses up one catalyst from the bags (highest band the crafter has that fits the
  item's level) -> +1 tier, x2 Unique chance (the existing catalyst flag in `Core/Craft`).
- **Salvage (companion dialogue):** "Salvage" lists what would be salvaged (affixed, not equipped, not protected by the loot
  filter, grey-blue) with a confirm box; each affix gives `Arpg.Salvage.EssencePerAffix` (1) essence, Uniques/Ascended give
  more. "Buy catalysts" options trade essences (`Arpg.Catalyst.EssenceCost` 10 / 30 / 90 per band) - gossip, not the vendor
  window (custom costs aren't in the client's ItemExtendedCost).
- Later (same mechanism): typed blank bases.

## Milestone
6 (owner decisions 2026-11-26).
