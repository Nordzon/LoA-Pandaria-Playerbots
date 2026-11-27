# Tasks — m3b-potions

## 0. Decisions
- [x] 0.1 Owner: stacks of 999, mana potions give energy/focus at half and rage/runic power; rejuvenation too; cooldown 10 s (proposed, not objected)

## 1. Core (pure, unit-tested)
- [x] 1.1 `Core/Potions`: heal amount = pct of max health (min 1), tier by monster kind/rank, drop roll per kind

## 2. Data
- [x] 2.1 `arpg_potion` + `arpg_potion_drop` (tier, item_id, spell_id, heal_pct, source kind, drop_pct) — the six healing potions
- [x] 2.2 Cooldown/stack/level via item_template + Item-sparse hotfixes (no DBC patch needed): category 4, 10 s

## 3. Game wiring
- [x] 3.1 Heal override: the potion spell heals pct of max health (UnitScript ModifyHealReceived / OnHeal by spell id)
- [x] 3.2 Drops: kill -> extra loot item by monster tier (normal/elite/Champion/Superunique/boss), at most one per kill
- [x] 3.3 Carry rule: none (stack 999, owner)
- [x] 3.4 `.arpg test potion <char> <itemId>`

## 4. Tests
- [x] 4.1 Unit: heal pct, tier by kind, drop chance table
- [x] 4.2 `32-potions.gmtest` (passes 2026-11-22; the use is an item script, `item_script_names` 'arpg_potion'): drink at 50% health -> +pct exactly; Champion kill with drop forced 100% -> tier-3 potion in loot;
  cooldown blocks a second drink, allows after the cooldown
- [x] 4.3 In game (owner): potion tooltip/cooldown sweep matches the server - owner confirmed 2026-11-27 (effects + tooltip)

## 5. Docs
- [x] 5.1 SPEC 4.3b potions section, CODEMAP, PROGRESS (BALANCE numbers: in SPEC + SQL)
