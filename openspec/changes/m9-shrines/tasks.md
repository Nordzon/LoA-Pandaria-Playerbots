# m9-shrines tasks

## 1. Data
- [x] 1.1 `sql/world/2026_11_28_01_arpg_shrines_goblin_rifts.sql`: `arpg_shrine` (6 rows), gameobject_template 800201-800206 (goober,
  ScriptName `arpg_shrine`)

## 2. Logic
- [x] 2.1 `Core/Charms` (shared): weighted pick, spawn gate (chance, cooldown) + unit tests
- [x] 2.2 `World/Shrines`: kill roll + spawn + announce, GameObjectScript use -> buff (aura amount/duration, Fortune effect
  lines, group within 40 yd), expiry tick, login cleanup; `Arpg.Shrine.*`
- [x] 2.3 `.arpg shrine spawn|use|info`; GM test `59-shrines`

## 3. Owner
- [ ] 3.1 In game: shrines appear now and then while clearing; click gives the buff with the right tooltip (OWNER-CHECKS)

## 4. Docs
- [x] 4.1 SPEC 4.3c, CODEMAP, PROGRESS
