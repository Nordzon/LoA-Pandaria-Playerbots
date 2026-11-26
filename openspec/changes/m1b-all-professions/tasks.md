# Tasks — m1b-all-professions

## 1. Server
- [x] 1.1 Find the primary-profession limit in this core (config key or constant); set to 11 — `MaxPrimaryTradeSkill` (World.cpp, no clamp), set by `write-confs.ps1 -MaxProfessions 11`
- [x] 1.2 If hard-coded: tiny `// ARPG:` patch — not needed (config key)

## 2. Addon
- [x] 2.1 ArpgUI professions panel: list learned professions + skill, click opens the profession window

## 2b. Crafting wheel (owner 2026-11-22, SPEC 4.8)
- [x] 2b.1 Recipe colour → tier bonus / affix count (orange/yellow +1 tier, max skill +1 affix, grey T1); craft source detection in the roll
- [x] 2b.2 Crafted Uniques: `Arpg.Craft.UniqueChance` + per-profession pity; needs m3 Uniques (BALANCE §8b)
- [~] 2b.3 Catalysts from Champions/Superuniques (+1 tier, x2 Unique chance); salvage → essences; vendor prices
- [x] 2b.4 `.arpg test craft <char> <spellId> [skill <n>] [catalyst]` + `28-crafting.gmtest` (2026-11-22: `Core/Craft` + 4 unit tests, `Loot/Crafting`; catalyst flag works, catalyst ITEMS come with 2b.3)

## 3. Tests
- [x] 3.1 `arpg/tests/15-professions.gmtest`: test character learns all 11 by command; the skill list shows 11
- [ ] 3.2 Playtest: learn a 3rd profession from a trainer in game, craft once, open it from the panel

## 4. Docs
- [ ] 4.1 Update docs/CODEMAP.md
- [ ] 4.2 Update docs/PROGRESS.md
