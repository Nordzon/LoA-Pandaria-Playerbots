# Tasks — m3-champions-superuniques

## 1. Core (pure, unit-tested)
- [x] 1.1 `Core/MonsterBands`: band lookup, monster level (never below the player), affix pick with level unlocks and exclusive
  group, minion count, affix damage as % of max health (never a one-shot), health orb — 7 tests pass (2026-11-22)
- [x] 1.2 `Core/SuperuniqueRotation`: next pick from the zone pool (skips the killed one and live ones), respawn time, alive count — 4 tests pass (2026-11-22)

## 2. Data
- [x] 2.1 `arpg_monster_band` (BALANCE §14 rows) + `arpg_monster_affix` (7 affixes) — `2026_11_22_03_arpg_monster_band.sql`
- [x] 2.2 `arpg_superunique` (zone pool, creature entry, ability kit) — first 3 zones: done 2026-11-22 (`2026_11_22_07_arpg_superunique.sql`: Elwynn 5, Westfall 5, Dun Morogh 3 from the zones' rares; spawn points copied from their creature rows; 2 kits x 4 abilities, kind instant/cast/must-interrupt, damage as % of the band's affix damage)

- [x] 2.3 Uniques (BALANCE §8b, SPEC 4.4): `arpg_unique_power` + `_tier` (template ranges), `arpg_unique` (base item, power, fixed
  affixes), `item_instance_arpg_unique`; first 6 Uniques from the §8b examples — SQL written 2026-11-22 (a Unique = slot +
  armour type, D2-style: the dropped base of that slot becomes the Unique; `2026_11_22_06_arpg_unique.sql`, characters `_03`)

## 3. Game wiring
- [~] 3.1 Champion roll on open-world spawn (real-player zones), apply band level/health/damage, minions, visual (scale + aura) — done 2026-11-22 incl. name tag + Rare portrait; glow aura still to pick in game: first-sight roll, `Scaling::Pin`, minions follow, scale 1.25, soft affixes (`Monsters/Champions`)
- [x] 3.2 Monster affix effects (auras/spells; damage = % of the target's max health) — done 2026-11-22: soft affixes, Cold = Chilled aura, Lightning = sparks, Fire = delayed dodgeable death blast, Arcane = interruptible Fireball cast every 8 s; affix hits never kill (test 30)
- [x] 3.3 Loot: floor (≥ 1 green, 25% blue), affix tier +1; health orb on kill — done 2026-11-22 (test 30)
- [x] 3.4 Superunique spawn, patrol, ability kit (generic script), rotation after a kill, zone announce — done 2026-11-22 (patrol: random roam `Arpg.Superunique.PatrolRadius`; announce: zone text): `Monsters/Superuniques` (PerZone alive while a real player is in the zone, band kind 1 pinned, kit abilities at the aggro holder incl. bot tanks, damage % of target max health, different one after RespawnDelay, 40% orb, guaranteed blue, Unique 10% +5%)
- [x] 3.5 Test commands: `.arpg champion make|info|kill|loot|strike <char>`, `.arpg superunique spawn|next|info|kill|loot|fight <char>`

- [x] 3.6 Unique powers on the effect machinery — done 2026-11-22: `Core/UniquePower` (3 tests), `Loot/Uniques`, powers fire via `Loot/AffixEffects` (chance on hit / every Nth hit / when struck = absorb shield / on heal / passive above the speed cap, per-item cooldowns), tooltip rows + names in the client patch, `.arpg test unique <char> <id> <tier> [equip]`, test 26; (Power = max(AP, SP), tier + rolled numbers, ICD); client rows per
  (power, tier, steps) + names via Item-sparse patch; `.arpg test unique <char> <id> <tier>`

## 4. Tests
- [x] 4.1 `30-champions.gmtest`: bands at 5, 20 and 90 (level, minions, affix set), pinned, orb, floor, +1 tier, affixes, tags - passes 2026-11-22 (band 35-59 not covered: no test spot in that range)
- [x] 4.2 `31-superuniques.gmtest`: kill → a different one spawns after the (shortened) delay; Unique pity — passes 2026-11-22
- [ ] 4.3 Playtest: at-level character beats a pack in 30–60 s (BALANCE §14 targets)

## 5. Docs
- [ ] 5.1 CODEMAP, PROGRESS, BALANCE §14 tuning notes
