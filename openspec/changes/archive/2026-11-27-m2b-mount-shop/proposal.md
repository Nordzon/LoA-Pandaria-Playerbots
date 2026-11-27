# m2b-mount-shop

## Why
Owner 2026-11-26: the companion's dialogue should offer gold sinks, starting with "the rep grind mounts available for gold"
at a few thousand gold each (5,000 to start, tuned later).

## What changes
- `arpg_mount_shop` (world): one row per mount spell the game sells for reputation (64; built from item_template mount items
  with a required faction), optional per-row price. Faction-only mounts are open to everyone.
- Companion dialogue: "Mounts for gold" -> pages of 15 mounts the character doesn't know, each with a price popup; buying takes
  the gold and teaches the spell (no reputation check). `Arpg.MountShop.Enabled` (1), `Arpg.MountShop.Price` (5000 gold).
- `.arpg test mount <char> list|buy|forget`, `.arpg test money <char> <gold>`.
- Not included: riding skill (a mount needs the riding skill it always needed).
