# m9-goblins: Treasure Goblins (owner 2026-11-28; SPEC 4.3 "variant: Treasure Goblin")

## Why
The Diablo III loot piñata: a goblin with a sack appears, runs, and escapes through a portal unless you kill it fast.

## What
- A kill by a real player in the open world has `Arpg.Goblin.KillChance` (0.4%) to summon a **Treasure Goblin** (creature
  1000010, the client's own Treasure Goblin model) 15 yards away, at most one per `Arpg.Goblin.Cooldown` (600 s) per player.
- It never attacks. Hit or approached (10 yd), it flees; `Arpg.Goblin.EscapeSeconds` (30) after the first hit it opens a
  portal and is gone (no loot). Untouched, it leaves after `Arpg.Goblin.Lifetime` (180 s). Health: the player's level,
  x `Arpg.Goblin.HealthMult` (8) of a normal monster.
- Killed: gold (`Arpg.Goblin.GoldPerLevel` copper x level), `Arpg.Goblin.Items` (4) gear drops (green, blue at 50%, epic at
  10%, all with affixes), `Arpg.Goblin.Essences` salvage essences, a catalyst (25%), a charm (35%), the Unique charm
  "Goblin's Lucky Coin" (5%). Mates in the group get their own share (personal loot).
- Chat announce on spawn and escape.
