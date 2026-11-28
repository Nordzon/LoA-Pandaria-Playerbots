# m9-shrines: shrines while clearing (owner 2026-11-28)

## Why
Diablo-style shrines give short "ooh" moments in a 2-hour session: a glowing shrine appears while you clear, you click it,
and for two minutes you hit harder, run faster or find more.

## What
- A kill by a real player has `Arpg.Shrine.KillChance` (1%) to raise a shrine (game object 800201-800206) at the corpse,
  at most one per `Arpg.Shrine.Cooldown` (240 s) per player; it vanishes after `Arpg.Shrine.Lifetime` (180 s).
  Open world and dungeons/rifts (`Arpg.Shrine.InDungeons` 1).
- Six shrines (`arpg_shrine`, weights and numbers in SQL): **Frenzied** (+20% haste), **Combat** (+15% damage),
  **Protection** (-20% damage taken), **Fleeting** (+30% move speed), **Enlightened** (+25% kill XP), **Fortune**
  (+50% magic find, +50% gold find). Each is an existing client buff spell whose amount and duration (120 s) we set, so the
  buff icon and tooltip show the real numbers; Fortune adds our effect lines while its buff lasts.
- Clicking gives the buff to the user and the real players and bots of their group within 40 yards, then the shrine goes.
- An announce in chat ("A Shrine of Fortune appears nearby!").

## Out of scope
- Hand-placed shrines in the world; pylons with on-use spells (D3 rift pylons) - maybe later for rifts.
