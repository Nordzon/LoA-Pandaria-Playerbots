# Tasks — m1-world-scaling

## 1. Module skeleton
- [ ] 1.1 `modules/mod_arpg` CMake target, loads `arpg.conf`
- [ ] 1.2 Commands: `.arpg version`, `.arpg reload`, `.arpg config set`, `.arpg test setup` (+ `arpg_test_spot` SQL)
- [ ] 1.3 Unit-test target wired into ctest

## 2. Scaling
- [ ] 2.1 `Core/Scaling.{h,cpp}`: pure formula (level, health and damage multipliers) + unit tests
- [ ] 2.2 Hook creature visibility/update near the real player; apply the scaled level and stats
- [ ] 2.3 Lock creatures in combat with bots; revert on reset/respawn
- [ ] 2.4 `.arpg scale probe <char> nearest|<guid>`
- [ ] 2.5 SQL: quest level -1 for scaling quests (reversible script)

## 3. Tests
- [ ] 3.1 Unit: formula edge cases (offset, upward-only, level caps)
- [ ] 3.2 `arpg/tests/10-world-scaling.gmtest` passes
- [ ] 3.3 Playtest: level 85 in Northshire, Westfall and Jade Forest; note kill times; tune HealthMod/DamageMod
- [ ] 3.4 Unit/GM test of the bot lock using a creature put "in combat with a bot" by test command; the live check with real bots happens in m7

## 4. Docs
- [ ] 4.1 Update docs/CODEMAP.md (hooks used, upstream touches)
- [ ] 4.2 Update docs/PROGRESS.md
