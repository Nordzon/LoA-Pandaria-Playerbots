# Tasks — m7-bots

## 1. Enable
- [ ] 1.1 playerbots conf: enable, `MaxRandomBots` 300 first, then 1000; first start creates bot characters (slow)
- [ ] 1.2 `.server info` update time acceptable with the client running; record numbers in PROGRESS

## 2. Verify earlier features with bots
- [ ] 2.1 Scaling: level-85 player arrives where a low-level bot is fighting; the bot's mob keeps its level (m1 requirement "Bots are not disturbed")
- [ ] 2.2 Dungeons: party of player + bots scales by party size (m5)
- [ ] 2.3 Loot: bots roll only on items the player's class can't use; bot items have no affixes (m2)

## 3. Dungeon Finder hook
- [ ] 3.1 Real player queuing: dungeon level limits lifted; bots matched to player level ±`Arpg.LFG.BotLevelWindow`
- [ ] 3.2 `.botscenario` test: level-50 test character queues Deadmines; matched bots are 47–53; instance scales to 50

## 4. Optional
- [ ] 4.1 Bot crowding (idle bots of the zone's level range move to the player's zone)

## 5. Docs
- [ ] 5.1 Update docs/CODEMAP.md
- [ ] 5.2 Update docs/PROGRESS.md
