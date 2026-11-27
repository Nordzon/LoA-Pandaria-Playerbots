# m6-start-quests tasks

## 1. Data
- [x] 1.1 `sql/tools/gen_start_quests.py` -> `sql/world/2026_11_27_05_arpg_start_quest_proxy.sql` (`arpg_quest_proxy`, 105 rows)

## 2. Server
- [x] 2.1 Upstream hook `ArpgHooks::QuestClassAllowed` in `Player::SatisfyQuestClass` (CODEMAP "Upstream touches")
- [x] 2.2 `Character/StartQuests`: load at startup + `.arpg reload`, hook, complete-on-accept for trainer steps; `Arpg.StartQuests.Enabled`
- [x] 2.3 `.arpg test questproxy`, `.arpg test cantake`; GM test `57-start-quests`
- [x] 2.4 Full run ALL OK (31 GM files, 22:48)

## 3. Owner
- [x] 3.1 Nordana (Night Elf Paladin): Ilthalaine offers "Simple Sigil" -> Alyissia "Learning New Techniques" (done on accept)
  -> "Priestess of the Moon" and the rest of Shadowglen

## 4. Docs
- [x] 4.1 SPEC 4.9, CODEMAP (module + upstream touch), PROGRESS
