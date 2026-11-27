# Design — m2-affix-loot

## Where affixes roll (task 4.1)
`ScriptMgr::OnItemPickup(player, item, sourceType, sourceId)` is called from every place a player receives a **new** item
(Player.cpp: creature/gameobject/item loot, quest reward and quest source items, crafting via `Spell`, vendor, `.additem`,
refund, mail). Upstream only forwards it to the item's own `ItemScript`. We add one `// ARPG:` line there that calls
`ArpgHooks::ItemPickup` first (null unless mod_arpg is loaded). One patch instead of touching 59 `StoreNewItem` call sites.

Rolled for sources: `CreatureLoot`, `GameObjectLoot`, `ItemLoot`, `QuestReward`, `Spell` (crafting), `CreatureMail`
(loot-pet overflow, m2b), `Command` (only for the test character / GM `.additem`, so tests can create items).
Never for: `Vendor`, `ItemRefund`, `PlayerMail`, `PlayerLoot`, `QuestSource`, `Unknown`.
Skipped when: not a real player (bots: `Arpg.Loot.AffixBotItems` = 0), quality not green/blue/purple, the item already has
affixes, or the template has its own random property/suffix (keep Blizzard's "of the Bear" items as they are).

## Where affixes live (tasks 4.1, 3.2)
- **Source of truth:** `item_instance_arpg_affix` (characters DB): item guid, position (0-4), affix id, tier, value.
  Read on login into a per-item cache; written when rolled (same transaction as the item save).
- **Native display + stats** for stat affixes (Deadly, Swift, of the Wind, ...): the item's 5 random-property enchant slots
  (`PROP_ENCHANTMENT_SLOT_0..4`) get the id of an existing `SpellItemEnchantment` row "+N <stat>" with N closest to the
  rolled value (lookup built at startup from `sSpellItemEnchantmentStore`: single-effect `ITEM_ENCHANTMENT_TYPE_STAT` rows).
  The core then applies the stat and the client shows a green line - no addon, no patch. The stored value is snapped to that N.
- **Non-native affixes** (leech, procs, move speed, gold find...): only in our table; their effects are applied by our code
  (auras/hooks, task 4.3) while the item is equipped. Tooltip lines come from ArpgUI (m4) or the client patch (m6).
- Enchant slots used by Blizzard random properties are left alone (we skip those items), so no conflict.
- Orphan rows (deleted items) are removed by a cleanup query at startup:
  `DELETE a FROM item_instance_arpg_affix a LEFT JOIN item_instance i ON i.guid = a.item_guid WHERE i.guid IS NULL`.

## Measured: native enchant coverage (2026-11-22, `.arpg loot enchants`)
Fixed-amount stat enchantments in the 5.4.8 client: agi/int/sta/spi 1-46, str 1-400 (outlier), crit 2-70, hit 2-30,
dodge 1-38, haste 6-14, mastery 7-12, expertise 18, parry 2-3. Level-90 affixes roll ~300+ main stat, so **snapping would
wreck the endgame**. Decision (supersedes "native display" above):
- **Every affix stat is applied by our code at its exact rolled value** while the item is equipped (stat modifiers on equip/
  unequip/login), so gameplay is right at every level from day one.
- A native enchant slot is written **only if a row is within ±5% of the roll** (then the client shows a real green line and the
  core applies that row; our code skips that affix so nothing doubles). In practice: low levels.
- Everything else: tooltip lines from ArpgUI (m4) until the client patch (m6, or m2 if the MPQ spike - task 4.6 - succeeds)
  adds one enchantment row per (affix, value); then everything is native. **Priority of task 4.6 raised.**

## Risks
- If the client caches enchant tooltips oddly for random-property slots on non-random items, fall back to the regular
  `PERM`/`TEMP` slots is NOT an option (player enchants); instead test early with `.arpg loot last` + a real client look (owner).
- Snapping: low levels have few small "+N stat" enchants; values below the smallest existing row use that row (documented).
