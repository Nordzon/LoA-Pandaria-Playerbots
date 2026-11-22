# m2b-loot-pet-filter

## Why
Clearing a zone should be about fighting, not clicking corpses. A pet that collects loot and a filter
that throws away junk keep the loop fast, and Highlight beams make good drops stand out.

## What changes
- Loot pet: a non-combat follower that walks to each corpse you killed, digs, and moves the loot into
  your bags (gold at once, quest items always, overflow by mail). Builds on MoP's built-in area looting.
- Server-side loot filter per character: ordered rules, with the actions Pick up, Leave, Auto-sell and Highlight.
  Quest items, Uniques and gold are always kept.
- ArpgUI: a pet toggle and a filter panel. `.arpg lootpet` and `.arpg filter` commands.
- **Companion roles** (added 2026-11-22): the same pet is a vendor, repairer, banker, mailbox, reforger,
  transmogrifier (appearance changer) and void storage keeper, using the core's native NPC windows; its own
  look can be changed from a list. `.arpg companion` commands.

## Depends on
m2-affix-loot: affixes must roll when an item is created, so pet-looted items get them too.

## Config / SQL added
`Arpg.LootPet.Enabled` (1), `Arpg.LootPet.CreatureEntry`, `Arpg.LootPet.WaitForCombatEnd` (1),
`Arpg.LootPet.ReachTimeout` (3000 ms), `Arpg.LootPet.MaxQueue` (40), `Arpg.LootPet.Range` (60 yd),
`Arpg.LootFilter.Enabled` (1), `Arpg.Companion.{Vendor,Repair,Bank,Mail,Reforge,Transmog,VoidStorage}` (1),
`Arpg.Companion.InCombat` (0); tables `arpg_lootpet` (creature template row), `character_arpg_lootfilter`,
`arpg_companion_vendor`, `arpg_companion_model`, `character_arpg_companion` (chosen model, unlocks).

## Out of scope
The pet gathering herbs or ore, stable master, shared stash (possible later). Filter presets shared between characters.

## Milestone
2
