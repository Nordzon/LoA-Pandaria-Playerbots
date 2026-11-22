# Tasks — m1-world-scaling

## 1. Module skeleton
- [x] 1.1 `modules/mod_arpg` CMake target, loads `arpg.conf` (own parser `Core/ConfigFile`; copied by write-confs.ps1 if missing)
- [x] 1.2 Commands: `.arpg version`, `.arpg reload`, `.arpg config set`, `.arpg test setup` (+ `arpg_test_spot` SQL)
- [x] 1.3 Unit-test target wired into ctest (`arpg/unittests`, root CMake `# ARPG:` line)

## 2. Scaling
- [x] 2.1 `Core/Scaling.{h,cpp}`: pure formula (level, health and damage multipliers) + unit tests
- [x] 2.2 Hook creature visibility/update near the real player; apply the scaled level and stats (PlayerScript tick + `ArpgHooks::CreatureLevel` in SelectLevel) — code done, live check passed
- [x] 2.3 Lock creatures in combat with bots; revert on reset/respawn (in-combat creatures never change; respawn hook + sweep revert when no real player near; evade-reset reverts on the next sweep) — live check passed
- [x] 2.4 `.arpg scale probe <char> nearest|<spawnId>`
- [x] 2.5 SQL: quest level -1 for scaling quests (reversible script) — 13743 zone quests; originals in `arpg_quest_level_backup`

## 3. Tests
- [x] 3.1 Unit: formula edge cases (offset, upward-only, level caps) — 10 cases pass
- [x] 3.2 `arpg/tests/10-world-scaling.gmtest` passes (live, 2026-11-22 00:57: levels 30/60/85/90, offsets ±2, cap, upward-only, kill switch)
- [ ] 3.3 Playtest: level 85 in Northshire, Westfall and Jade Forest; note kill times; tune HealthMod/DamageMod
- [ ] 3.4 Unit/GM test of the bot lock using a creature put "in combat with a bot" by test command; the live check with real bots happens in m7

## 4. Docs
- [ ] 4.1 Update docs/CODEMAP.md (hooks used, upstream touches)
- [ ] 4.2 Update docs/PROGRESS.md
