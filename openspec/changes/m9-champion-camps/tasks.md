# Tasks: m9-champion-camps

## 1. Camps
- [x] 1.1 `Core/ChampionCamps` (CampKey, CampWantsChampion) + unit tests
- [x] 1.2 Champions: `ChampionEligible` (no menu/quest/scripted/friendly/critter/rare/boss/excluded), `TryCampChampion` in
      the first-sight roll; settings `Arpg.Champion.Camps/CampSize/CampMin/CampRespawn`, Chance default 0
- [x] 1.3 SQL `arpg_champion_exclude` (2026_11_28_03)
- [x] 1.4 `.arpg champion camp|eligible`, GM test 63; tests 00/10/30 keep camps off during the run

## 2. Superuniques in every zone
- [x] 2.1 `gen_superuniques.py` stand-ins (5 per zone minimum, ids 5000+), regenerated SQL
- [x] 2.2 Native rare despawn only for real rares; `Arpg.Superunique.PerZone` 3
- [x] 2.3 Apply SQL + `gen_monster_tags.py` ("Unique <name>" copies for the stand-ins), restart, full run

## 2b. Health cap (owner 2026-11-28: 232k-hp Unique Darkened Horror, template Health_mod 272)
- [x] 2b.1 Stand-ins skip Health_mod > 3; Superuniques/Champions count Health_mod as at most `Arpg.Superunique.MaxHealthMod` (10)

## 3. Owner checks
- [ ] 3.1 OWNER-CHECKS D: Wandering Isle camps feel even, no quest NPC Champions, Superuniques show up
