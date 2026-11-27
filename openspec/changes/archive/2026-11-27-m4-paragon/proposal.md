# m4-paragon

## Why
At 90 the character stops growing. Paragon (SPEC 4.5, BALANCE §6.1/§12) keeps every kill and quest meaningful after the
cap: endless levels, a point per level into small permanent bonuses, and the key that unlocks the top loot tiers (T6-T8)
in raids. The open world at 90 (Vale of Eternal Blossoms) becomes the endless solo farm through a world tier that grows
with Paragon (SPEC 4.1 "Paragon grind").

## What changes
- **Paragon XP:** from level 90, all XP (kills, quests, Champions/Superuniques) goes to `character_paragon` instead of being
  lost. XP to the next level = `Arpg.Paragon.XpBase × Arpg.Paragon.XpGrowth^level` (target: ≥ 5 levels in a 2-hour session
  at the start, then slower).
- **Points, Diablo 3 style (proposed):** each Paragon level gives one point to the next tab in rotation
  (Core → Offense → Defense → Utility → Core …); within a tab the player picks the stat. Bonuses per point (BALANCE §6.1):
  Core +0.5% main stat or +0.5% stamina; Offense +0.4% crit / haste / mastery; Defense +0.4% armour / avoidance;
  Utility: move speed cap +1% (max +10%), magic find +5% (max +100%), gold find +5%.
- **Applied** as stat modifiers on login and on spend; free respec (optional gold cost, config).
- **World tier (Vale):** monsters in uncapped zones get +`Arpg.Paragon.WorldTierPct` (2)% health and damage per Paragon level
  (capped by `Arpg.Paragon.WorldTierCapPct`) and give the same % more Paragon XP; more Champions there.
- **Loot:** Paragon gates the Ascended affix tiers in raids (BALANCE §12) - wiring lands with m5 raids; m4 stores the level.
- **UI:** `.paragon` chat commands (info, spend <tab> <stat>, respec) now; the ArpgUI bar and panel later (addon).
- Test tools: `.arpg paragon set <char> <level> [xp]`, `.arpg paragon info <char>`.

## Open questions (owner)
1. Points: Diablo 3 rotation (one tab per level, pick the stat inside) or free choice of tab every level?
2. Respec: free, or a gold cost that grows with Paragon level?
3. Should quests at 90 give full Paragon XP (fast) or half?

## Milestone
4 (after m3 Champions/Superuniques, before m5 dungeon/raid scaling).
