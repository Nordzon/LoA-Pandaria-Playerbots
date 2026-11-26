# Tasks - m6-custom-items

## 1. Items
- [x] 1.1 SQL: `item_template` rows 700001-700003 (catalysts), 700010 (essence) + `hotfix_data` both types; idempotent
- [x] 1.2 GM test: give each item, it lands in the bags with the right stack size

## 2. Catalysts (m1b 2b.3)
- [x] 2.1 Config `Arpg.Catalyst.*`; drops from Champion leaders / Superuniques by tier band
- [x] 2.2 Craft uses up one fitting catalyst automatically (+1 tier, x2 Unique chance); `28-crafting` covers it

## 3. Salvage
- [x] 3.1 `Core/Salvage`: essences for an item (affix count, Unique, Ascended) + unit test
- [x] 3.2 Companion dialogue: Salvage (preview + confirm), Buy catalysts (essence cost per band)
- [x] 3.3 `.arpg test salvage <char>` + GM test

## 4. Owner + docs
- [ ] 4.1 Owner: the items show name/icon/tooltip in the real client; salvage + buy through the companion dialogue
- [x] 4.2 SPEC 4.8, CODEMAP, PROGRESS, OWNER-CHECKS
