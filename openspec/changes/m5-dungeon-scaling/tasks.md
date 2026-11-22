# Tasks — m5-dungeon-scaling

## 1. Core (pure, unit-tested)
- [x] 1.1 `Core/InstanceScale` (3 unit tests, 73 total, 2026-11-23): party-size table lookup (interpolate between rows, clamp), raid tier multiplier, solo role
  knobs by spec role, Ascended tier gate (reuse `Core::MaxAffixTier`)

## 2. Data
- [x] 2.1 `arpg_party_scale` (kind 5-man/raid, players, health %, damage %) and `arpg_raid_tier` (map id, tier, recommended Paragon)
- [x] 2.2 `arpg.conf`: Arpg.Dungeon.Enabled, SoloNonTankDamage, SoloTankDamageDealt, SoloHealerDamageDealt

## 3. Game wiring
- [x] 3.1 `// ARPG:` hook in AutoBalance (AutoBalanceLevelScaling.cpp, creature multipliers) -> mod_arpg table × tier × role
- [x] 3.2 Solo player damage dealt (tank / healer) in 5-man instances
- [x] 3.3 Dungeon level selector (hook on AutoBalance's target level; `character_arpg_dungeon_level`)
- [x] 3.4 Ascended affix tiers in raids (loot roll: raid tier + Paragon) — `arpg_ascended_tier`, test 50
- [x] 3.5 `.arpg dungeon info|level <char>`; AutoBalance on in the confs (`write-confs.ps1`)

## 4. Tests
- [x] 4.1 Unit: table, interpolation, tiers, roles, gate
- [x] 4.2 `50-dungeons.gmtest` (solo Deadmines, level selector, Molten Core raid, Ascended tiers; 3-bot party not covered headless): solo in a 5-man -> 30%/40% (× role), 3 bots -> 80%/85%; level selector changes creature level;
  a raid map -> raid table × tier
- [ ] 4.3 Playtest: solo a 5-man as tank, DPS and healer (BALANCE §5 targets)

## 5. Docs
- [ ] 5.1 SPEC 4.6 decisions, CODEMAP (upstream touch), PROGRESS
