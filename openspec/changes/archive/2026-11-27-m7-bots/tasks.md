# Tasks — m7-bots

## 1. Enable
- [x] 1.1 playerbots conf: enable, `MaxRandomBots` 300 first, then 1000; first start creates bot characters (slow) - owner 2026-11-26: 500 (write-confs default)
- [x] 1.2 `.server info` update time acceptable with the client running; record numbers in PROGRESS (500 bots, no client: mean 44 ms, median 13, p95 119; 4.8 GB)

## 2. Verify earlier features with bots
- [x] 2.1 Scaling: level-85 player arrives where a low-level bot is fighting; the bot's mob keeps its level (m1 requirement "Bots are not disturbed") - 54-bots.gmtest (2026-11-27)
- [x] 2.2 Dungeons: party of player + bots scales by party size (m5) - 54-bots.gmtest (2026-11-27)
- [x] 2.3 Loot: bots' own items have no affixes (m2) - `54-bots.gmtest` - 54-bots.gmtest (2026-11-27)
- [x] 2.4 Bot Need rolls in the player's group: owner 2026-11-27 "bots never Need" = playerbots `LootNeedRollLevel` 1 (a Need vote becomes Greed, LootRollAction.cpp), pinned in write-confs

## 3. Dungeon Finder hook
- [x] 3.1 Real player queuing: dungeon level limits lifted; bots matched to player level ±`Arpg.LFG.BotLevelWindow` (upstream's LFG.AnyLevel + AiPlayerbot.LfgAnyLevel, window 3, via write-confs - no code)
- [x] 3.2 `.botscenario` test: level-50 test character queues Deadmines; matched bots are 47–53; instance scales to 50 (`51-dungeon-finder`: queued, bots fill -> proposal; scaling to the group = m5 test 50)

## 3b. Bots understand affixes (owner 2026-11-26)
- [x] 3b.1 One hook in playerbots `StatsWeightCalculator::CalculateItem` (`ExtraItemStats`, // ARPG:, 5 lines in 2 files); mod_arpg `Bots/BotItemScore` adds the affix stats of the item instance a party bot owns
- [x] 3b.2 `52-bot-affixes.gmtest`: a party bot's score for an affixed blue is higher with the hook; `Arpg.Bots.AffixScoring` 0 turns it off
- [x] 3b.3 Loot on a corpse (Need rolls): score by the planned affixes (plan has ids + tiers; values roll at pickup)
- [x] 3b.4 Party bots wear affixed gear one tier below the player (`Arpg.Bots.PartyAffixTierOffset`)

## 4. Optional
- [x] 4.1 Bot crowding (idle bots of the zone's level range move to the player's zone) - upstream dynamic level brackets (write-confs, weight 10, factions synced): 215 of 500 bots in the player's bracket after one run

## 5. Docs
- [x] 5.1 Update docs/CODEMAP.md
- [x] 5.2 Update docs/PROGRESS.md
