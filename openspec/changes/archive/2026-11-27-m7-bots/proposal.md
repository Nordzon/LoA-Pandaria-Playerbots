# m7-bots

## Why
Bots make the world feel alive and fill dungeon groups. They come last so m0–m6 are built and tuned on a
quiet, predictable server, and so bot problems can't mask bugs in our own features.

## What changes
- Enable playerbots: `MaxRandomBots` 1000 (the authors' usual count); watch world update time with `.server info`.
- Verify the bot-related rules already written into earlier features:
  - World scaling: a creature already fighting a bot keeps its level; only creatures near the real player scale (m1).
  - Party-size dungeon scaling counts bots as party members (m5).
  - Bot loot: bots roll only on items the player's class can't use; bot gear gets no affixes (`Arpg.Loot.AffixBotItems = 0`) (m2).
- Dungeon Finder hook: when the real player queues, any dungeon is allowed at any level and bots are matched to
  the player's level ±`Arpg.LFG.BotLevelWindow` (3).
- Optional: bot crowding — move idle bots of the zone's level range to the player's zone for more life.

## Out of scope
Changing bot AI itself.

## Milestone
7 (last)
