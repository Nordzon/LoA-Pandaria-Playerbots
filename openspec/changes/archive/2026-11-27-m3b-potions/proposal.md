# m3b-potions

## Why
Owner idea (2026-11-22): Diablo-style potions. He plays the tank; fights against Champions and Superuniques need a
mid-fight answer that isn't a drink break. A potion heals a share of max health (never outdated by levelling), stronger
from stronger monsters, on a short cooldown. The health orb on a Champion/Superunique kill stays as the free burst.

## What changes
- **ARPG potions** = six existing healing potion items (Minor -> Major Healing Potion). Each heals a % of max health instead
  of its flat amount: 10 / 12 / 14 / 16 / 18 / 20%.
- **Drops by monster tier** (extra loot, chance per kill): normal mob 10% potion ~5%, elite 12% ~15%, Champion 15% 50%,
  Superunique 18% always, dungeon/world boss 20% always. One potion per kill at most.
- **Short shared cooldown** (`Arpg.Potion.Cooldown`, 10 s proposed) instead of WoW's 1-minute potion lockout. The client
  enforces cooldowns from its own DBC, so the potion spells' cooldown rows go into the client patch AND the server DBC (same
  pipeline as the tooltip rows, SPEC 5).
- Carry limit: open (stack 20 as WoW, or a belt-style cap per type).
- All numbers in `arpg.conf` / `arpg_potion` (SQL); `.arpg reload` picks them up.
- Test tools: `.arpg test potion <char> <tier>` (gives + drinks), event recorder lines `potion heal=… pct=…`.

## Out of scope
Mana/energy potions (later, same mechanism), rejuvenation potions, potion affixes on gear (could come with m2 effects).

## Open questions (owner)
1. Cooldown length: 10 s proposed.
2. Carry: stack 20, or cap ~10 per type (Diablo belt)?

## Milestone
3b (after m3 Champions/Superuniques, before m4 Paragon).
