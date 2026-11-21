# Tasks — m2b-loot-pet-filter

## 1. Shared loot transfer
- [ ] 1.1 `Loot/LootTransfer`: move one corpse's loot to a player (gold, quest items, loot rights, mail on overflow); reuse the core's auto-loot path
- [ ] 1.2 Confirm affixes roll when an item is created (m2 hook), not when the loot window opens

## 2. Loot filter
- [ ] 2.1 `Core/LootFilter.{h,cpp}`: pure rule matching + unit tests (every match type, rule order, protected items)
- [ ] 2.2 `character_arpg_lootfilter` table, load/save, default rules (grey → Auto-sell)
- [ ] 2.3 Hook into LootTransfer and the player's own loot (auto-loot path)
- [ ] 2.4 Highlight: corpse beam using an existing client visual; removed when looted
- [ ] 2.5 Commands: `.arpg filter list|add|remove|move <char> …`

## 3. Loot pet
- [ ] 3.1 Creature template row (existing model, non-combat, no loot of its own) + summon/despawn per player
- [ ] 3.2 AI: follow → queue → move to corpse → dig animation → transfer → next; reach timeout; combat wait
- [ ] 3.3 Kill hook fills the queue; re-summon on login, zoning, teleport and death
- [ ] 3.4 `.arpg lootpet on|off|info <char>`; setting persists per character

## 4. Addon
- [ ] 4.1 ArpgUI: pet toggle button
- [ ] 4.2 ArpgUI: filter panel (list, add, reorder, delete rules) over addon messages

## 5. Tests
- [ ] 5.1 Unit: LootFilter
- [ ] 5.2 `arpg/tests/20-loot-pet.gmtest`: test character kills a spawned pack by command, then check the bags, gold and mail
- [ ] 5.3 `arpg/tests/21-loot-filter.gmtest`: grey → sold, protected Unique kept, Leave rule leaves item
- [ ] 5.4 Playtest: clear 3 packs in Jade Forest; the pet never gets stuck; it feels fast enough

## 6. Docs
- [ ] 6.1 Update docs/CODEMAP.md
- [ ] 6.2 Update docs/PROGRESS.md
