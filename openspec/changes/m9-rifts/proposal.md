# m9-rifts: Rifts - timed, levelled dungeon runs (owner 2026-11-28, "challenge mode/rifts dungeons")

## Why
Paragon needs a goal that keeps getting harder: Diablo greater rifts / WoW Mythic+. Any 5-man dungeon becomes a timed run at
a rift level you push ever higher; beating the clock raises your key, deeper levels pay better.

## What
- **Key:** each character has a rift key level (starts at 1) and a best level (`character_arpg_rift`).
- **Open:** companion menu "Open a Rift" (or `.rift open <n>`) picks a level from 1 to the key. The next 5-man dungeon the
  character enters becomes that rift (the character's old dungeon instances are reset when opening, so it starts fresh).
- **Harder:** monsters x `Arpg.Rift.HealthGrowth`^level health (1.10) and x `Arpg.Rift.DamageGrowth`^level damage (1.07), on
  top of party-size scaling. From level `Arpg.Rift.AffixLevel` (2) every run also rolls **Fortified** (trash +20% health,
  +30% damage) or **Tyrannical** (bosses +30% health, +15% damage).
- **Clock:** par time per dungeon (`arpg_rift_dungeon`, default `Arpg.Rift.DefaultParMinutes` 25); each death adds
  `Arpg.Rift.DeathPenalty` (10 s). Chat at 50%/75%/90% of par.
- **Done:** killing the dungeon's final boss (`instance_encounters.lastEncounterDungeon`) ends the run.
  In time: key +1 (+2 under 80% of par, +3 under 60%). Late: the key stays. Best level recorded.
- **Rewards** (bags; mailed when full): `Arpg.Rift.RewardItems` (2) + level/3 gear items (max 8), rolled as boss loot (best
  tier for the level), epic chance 10% + 3% per level (max 60%); gold `Arpg.Rift.GoldPerLevel` x level x character level;
  Paragon XP at 90 (`Arpg.Rift.ParagonXpPerLevel`); a charm at 20% + 2% per level; the Unique charm "Riftwalker's Sigil" at
  level 10+ (10%). Late runs: half the items, no charm.
- `.rift` player commands for chat (`.rift`, `.rift open <n>`, `.rift close`) + `ARPGR|` data line for the addon timer.

## Out of scope
- Champion packs inside rifts (the Champion roll is open-world only today).
- Timer frame polish in ArpgUI beyond a simple text line; per-week affix rotation; leaderboards.
