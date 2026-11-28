# m9-charms tasks

## 1. Data
- [x] 1.1 `sql/world/2026_11_28_00_arpg_charms.sql`: items 720000 (pouch), 720001-720003, 720010-720012 + hotfix_data;
  `arpg_charm_affix`, `arpg_charm_unique`; characters `item_instance_arpg_charm`

## 2. Logic
- [x] 2.1 `Core/Charms`: line count per size, value from B share, size pick by weight, drop chance by monster kind (+ unit tests)
- [x] 2.2 `Charms/Charms`: load tables + line cache, roll on pickup, pouch grant at login, pouch scan (apply/remove lines),
  drops on kill, `Arpg.Charms.*`
- [x] 2.3 `.arpg charm give|info|pouch|move`; GM test `58-charms`

## 3. Owner
- [ ] 3.1 In game: pouch in a bag slot, a charm moved in/out changes the character sheet; tooltip lines (OWNER-CHECKS)

## 4. Docs
- [x] 4.1 SPEC 4.4, CODEMAP, PROGRESS
