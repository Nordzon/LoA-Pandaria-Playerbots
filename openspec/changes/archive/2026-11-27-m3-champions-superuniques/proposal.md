# m3-champions-superuniques

## Why
"Danger roams" (SPEC 1.3): every zone, from level 1, has Champion packs and Superuniques worth hunting. They give the open
world the Diablo pulse: a glowing pack on the horizon, a fight with a twist, a loot burst and a health orb. They must be
killable by a fresh character (owner 2026-11-22), so their power comes in level bands. See SPEC 4.3, BALANCE §9 and §14.

## What changes
- **Champions:** on open-world spawn, `Arpg.Champion.Chance` (2%) turns a creature into a Champion + minions. Level, health,
  damage, monster-affix count and minions come from the player's level band (`arpg_monster_band`).
- **Monster affixes** (`arpg_monster_affix`): Extra Strong, Extra Fast, Stoneskin (soft, from 1); Fire / Cold / Lightning
  Enchanted (from 15, at most one); Arcane Caster (from 25). Affix damage is a % of the player's max health, never a one-shot.
- **Superuniques:** 1–2 per zone from a per-zone pool; a different one spawns `Arpg.Superunique.RespawnDelay` (300 s) after a
  kill. Data-driven ability kits (one generic script), at least one must-interrupt cast from 35.
- **Rewards:** Champion ≥ 1 green, 25% blue, affix tier +1; Superunique guaranteed blue + own Unique 10% with bad-luck protection.
  Health orb on kill (20% / 40%).
- Test tools: `.arpg champion spawn <char>`, `.arpg superunique spawn|next <char>`.

## Config / SQL added
`Arpg.Champion.Chance` (2), `Arpg.Champion.InDungeons` (0), `Arpg.Superunique.RespawnDelay` (300),
`Arpg.Superunique.UniqueChance` (10), `Arpg.Superunique.UniqueBadLuckStep` (5); tables `arpg_monster_band`,
`arpg_monster_affix`, later `arpg_superunique` (zone pool, creature entry, ability kit).

## Out of scope
Champion names and glow from the client patch (m6), Treasure Goblin (later), Champions in dungeons (off by default).

## Milestone
3 (Core logic pulled forward 2026-11-22 while m2 builds run)
