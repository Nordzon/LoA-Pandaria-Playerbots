# m9-rifts tasks

## 1. Data
- [x] 1.1 `sql/world/2026_11_28_01_arpg_shrines_goblin_rifts.sql`: `arpg_rift_dungeon` (MoP dungeons' par times); characters
  `character_arpg_rift`

## 2. Logic
- [x] 2.1 `Core/Rifts`: multipliers, key upgrade from time, reward counts, Fortified/Tyrannical + unit tests
- [x] 2.2 `Dungeons/Rifts`: key load/save, open (instance reset), start on entering a 5-man dungeon, scaling via the
  instance-scale hook, deaths, par announces, final boss -> finish, rewards; `Arpg.Rift.*`
- [x] 2.3 Companion menu "Open a Rift"; player `.rift` commands; `ARPGR|` line; ArpgUI timer line
- [x] 2.4 `.arpg rift open|info|finish|key|close|death`; GM test `61-rifts`

## 3. Owner
- [ ] 3.1 In game: open a level-1 rift at the companion, run a dungeon, beat the clock, key goes up (OWNER-CHECKS)

## 4. Docs
- [x] 4.1 SPEC 4.6b, CODEMAP, PROGRESS
