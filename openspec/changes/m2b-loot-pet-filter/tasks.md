# Tasks — m2b-loot-pet-filter

## 1. Shared loot transfer
- [x] 1.1 `Loot/LootTransfer` (in `Loot/LootPet` Transfer: gold incl. gold find, items via Player::StoreLootItem; mail on overflow done 2026-11-23: rolled first, 'Loot Companion' mail, 12 items per mail): move one corpse's loot to a player (gold, quest items, loot rights, mail on overflow); reuse the core's auto-loot path
- [x] 1.2 Confirm affixes roll when an item is created (m2 hook), not when the loot window opens

## 2. Loot filter
- [x] 2.1 `Core/LootFilter.{h,cpp}` (5 unit tests, text form `quality<=0 usable=0 class=4 sub=1 affixes>=3 tier>=5 vs=0 ids=1,2 -> sell|leave|pickup|highlight`): pure rule matching + unit tests (every match type, rule order, protected items)
- [x] 2.2 `character_arpg_lootfilter` table, load/save, default rules (grey → Auto-sell)
- [x] 2.3 Applied when corpse/chest loot is planned (covers the player's own and later pet looting); v1 Leave = out of the loot (owner may want a core hook for 'stays on the corpse')
- [ ] 2.4 Highlight: corpse beam using an existing client visual; removed when looted
- [x] 2.5 Commands: `.arpg filter list|add|remove|move|reset <char> …`

## 3. Loot pet
- [x] 3.1 Creature template row (7000001 'Loot Companion', copy of 2671) (existing model, non-combat, no loot of its own) + summon/despawn per player
- [x] 3.2 AI (v1, no dig animation): follow → queue → move to corpse → dig animation → transfer → next; reach timeout; combat wait
- [x] 3.3 Kill hook fills the queue; re-summon on login, zoning, teleport and death
- [x] 3.4 `.arpg lootpet on|off|info <char>`; setting persists per character

## 3b. Companion roles
- [x] 3b.1 npcflag mask from `Arpg.Companion.*` (+ gossip menu `arpg_companion`: Sell and repair, Bank, Mailbox open the native windows) (VENDOR, REPAIR, BANKER, MAILBOX, REFORGER, TRANSMOGRIFIER, VAULTKEEPER); re-applied on reload
- [ ] 3b.2 Confirm each native window opens on the summoned pet (core `GetNPCIfCanInteractWith` accepts it); fix with a tiny `// ARPG:` patch only if one refuses
- [x] 3b.3 (vendor list as `npc_vendor` rows for 7000001; looks `arpg_companion_model`, 9 models) `arpg_companion_vendor` (reagents/consumables) and gossip "Change appearance" from `arpg_companion_model`; choice persists (`character_arpg_companion`)
- [x] 3b.4 Roles off in combat (`Arpg.Companion.InCombat` 0): flags cleared on combat start, restored on combat end
- [x] 3b.5 `.arpg companion info|model|roles <char> ...` (done as `.arpg lootpet info|model`: info shows the role flags)
- [x] 3b.6 `arpg/tests/22-companion.gmtest`: flags match config; model change persists; vendor list present (covered in `34-loot-pet.gmtest`)

## 4. Addon
- [x] 4.1 ArpgUI: pet toggle button (L by the minimap; left = on/off, right = filter panel)
- [x] 4.2 ArpgUI: filter panel (list in plain words, add box + quick rules, reorder, delete, reset) over `.lootfilter` player commands and the hidden `ARPGF|` line

## 5. Tests
- [x] 5.1 Unit: LootFilter
- [x] 5.2 `arpg/tests/34-loot-pet.gmtest` (passes 2026-11-23; `.arpg test pack … kill loot` = the kill counts as the player's damage so the core fills the loot): test character kills a spawned pack by command, then check the bags, gold and mail
- [x] 5.3 `arpg/tests/33-loot-filter.gmtest` (passes 2026-11-23): grey → sold, protected Unique kept, Leave rule leaves item
- [ ] 5.4 Playtest: clear 3 packs in Jade Forest; the pet never gets stuck; it feels fast enough

## 6. Docs
- [ ] 6.1 Update docs/CODEMAP.md
- [ ] 6.2 Update docs/PROGRESS.md
