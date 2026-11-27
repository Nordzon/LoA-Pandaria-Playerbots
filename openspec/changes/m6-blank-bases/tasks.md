# m6-blank-bases tasks

## 1. Data
- [x] 1.1 `sql/tools/gen_blank_bases.py` -> `sql/world/2026_11_27_03_arpg_blank_bases.sql` (`arpg_blank`, item copies 710000+, hotfixes)
- [x] 1.2 Config `Arpg.Blank.Enabled|DropChance|Crafted|EssenceCost.Green|Blue|Epic` (+ conf.dist, reload)

## 2. Core
- [x] 2.1 `Core/Blanks`: bracket pick (highest bracket <= level - wearable - else the lowest), main-stat variant pick, armour type for a
  class at a level, max affix count for a quality; unit tests

## 3. Game
- [x] 3.1 `Loot/Blanks`: load `arpg_blank`, `IsBlank`, `BlankFor(slot/class/subclass/quality, level, main stat)`
- [x] 3.2 Roll rule: dropped / bought blanks force the quality's max affix count (crafted: crafting wheel rules)
- [x] 3.3 (a) Drop swap at the start of `PlanLoot` (floor items included), `Arpg.Blank.DropChance`
- [x] 3.4 (b) `ArpgHooks::CraftedItem` (upstream touch in `Spell::DoCreateItem`) -> blank of the crafted item
- [x] 3.5 (c) Companion "Blank bases" menu: slot -> quality -> buy for essences
- [x] 3.6 Commands: `.arpg blank for <char> <itemId>`, `.arpg blank buy <char> <slot> <quality>`

## 4. Tests
- [x] 4.1 Unit tests (Core/Blanks)
- [x] 4.2 `36-blank-bases.gmtest`: mapping, forced drop (chance 100) rolls max affixes, buy for essences, crafted blank
- [ ] 4.3 Owner check: tooltip (2 base lines + affixes, description), companion menu

## 5. Docs
- [x] 5.1 SPEC 4.1, BALANCE §8c, CODEMAP (files, upstream touch), PROGRESS
