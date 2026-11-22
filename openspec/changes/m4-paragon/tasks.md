# Tasks — m4-paragon

## 0. Decisions
- [x] 0.1 Owner 2026-11-22: Diablo 3 rotation, free respec, full quest XP

## 1. Core (pure, unit-tested)
- [x] 1.1 `Core/Paragon` (5 unit tests, 2026-11-22): XP to next level (base × growth^level), add XP (multi-level), tab rotation, points per tab,
  bonus per point with caps, world tier multiplier

## 2. Data
- [x] 2.1 characters `character_paragon` (guid, level, xp) + `character_paragon_point` (guid, tab, stat, points)
- [x] 2.2 `arpg.conf`: Arpg.Paragon.Enabled, XpBase, XpGrowth, WorldTierPct, WorldTierCapPct, RespecGold

## 3. Game wiring
- [x] 3.1 XP at max level -> Paragon XP (OnGiveXP for kills, OnQuestRewarded for quests x QuestXpPct), level-ups with a message
- [x] 3.2 Stat bonuses applied on login / spend / respec (HandleStatModifier TOTAL_PCT, ApplyRatingMod, effect lines, move speed above the cap)
- [ ] 3.3 World tier in uncapped zones (health/damage on scaling, XP bonus)
- [x] 3.4 `.paragon data|info|spend|respec` (any account) + ArpgUI Paragon window (`arpg/addons/ArpgUI`, `install-addons.ps1`), `.arpg paragon set|info|xp|spend|respec|data <char>`

## 4. Tests
- [ ] 4.1 Unit: curve, multi-level gain, rotation, caps, world tier
- [ ] 4.2 `40-paragon.gmtest`: XP at 90 -> Paragon level; spend -> stat change exact; respec; world tier multiplier in the Vale
- [ ] 4.3 Playtest: ≥ 5 levels in a 2-hour session at the start

## 5. Docs
- [ ] 5.1 SPEC 4.5 decisions, BALANCE §6.1 numbers, CODEMAP, PROGRESS
