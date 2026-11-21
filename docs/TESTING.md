# Testing

Four layers, fastest first. Claude runs layers 1–3 itself after every build. Layer 4 is you, in game.

| Layer | What it checks | Needs | Run |
| --- | --- | --- | --- |
| 1. Unit tests | Pure math: affix rolls and tiers, scaling formula, Paragon curve, party multipliers | Build only | `ctest --test-dir build -C RelWithDebInfo -R arpg` |
| 2. GM tests over SOAP | Real server behaviour: a mob's scaled level, items rolled, a Champion spawned, Paragon XP | worldserver running, no client | `arpg\scripts\run-gmtests.ps1` |
| 3. Bot scenarios | Bots grouping, queueing, running a dungeon with our scaling | worldserver + bots | `.botscenario …` (playerbots) via `gm.ps1` |
| 4. Playtest | Feel: is it fun, too hard, are kicks needed, does loot excite | You + client | Checklist in each change's `tasks.md` |

## 1. Unit tests
Anything with a formula lives in `modules/mod_arpg/src/Core/` and has no game headers, so it can be tested
in milliseconds. Example: 10,000 simulated rolls of a blue item at level 60 must give 2–3 affixes, never
an affix above the tier unlocked at 60, and a T5 rate within ±1% of the configured weight.

## 2. GM tests over SOAP (no client needed)
SOAP lets scripts send GM commands to the running worldserver. Enable it in `worldserver.conf`
(`SOAP.Enabled = 1`, `SOAP.IP = "127.0.0.1"`, `SOAP.Port = 7878`) and put a GM account in
`arpg\scripts\soap.local.ps1`. A `.gmtest` file is a list of commands and expected replies:

```
> .arpg test setup Arpgtest 85 elwynn-northshire
> .arpg scale probe Arpgtest nearest
? scaled=85\b
```

SOAP has no selected target, so our debug commands always take a player name or GUID. The test
character `Arpgtest` is created and moved by `.arpg test setup`, so tests don't depend on where you logged off.

## 3. Bot scenarios
LoA-Pandaria-Playerbots ships `.botscenario` GM commands to place, group, queue and probe bots in
repeatable setups (`modules/mod_playerbots/src/Scenario/`). We use them for party-size scaling and the
Dungeon Finder hook: e.g. queue `Arpgtest` (level 50) for Deadmines and check the bots matched are 47–53.

## 4. Playtesting in game
Log in with your GM account. Useful core commands (TrinityCore family; confirm names in this fork with `.help`):

| Goal | Command |
| --- | --- |
| GM mode / can't die | `.gm on`, `.cheat god` |
| Go somewhere | `.tele <name>` (`.lookup tele <part>` to find names), `.go xyz <x> <y> <z> <map>` |
| Change your level | `.character level <name> <lvl>` or `.levelup <n>` |
| Move fast | `.modify speed 5` |
| Kill / hurt target | `.die` (kills selected), `.damage <amount>` |
| Spawn / inspect a mob | `.npc add <entry>`, `.npc info`, `.respawn` |
| Items | `.additem <id> [count]`, `.lookup item <part>` |
| Server health | `.server info` (watch update time with 1000 bots) |

Our `.arpg` commands, added per change (each takes an explicit player name so it also works over SOAP):

| Feature | Commands |
| --- | --- |
| General | `.arpg version`, `.arpg reload`, `.arpg config set <key> <value>`, `.arpg test setup <char> <lvl> <spot>` |
| Scaling (m1) | `.arpg scale probe <char> nearest\|<guid>` (template vs scaled level and multipliers), `.arpg scale radius <char>` |
| Loot (m2) | `.arpg roll <char> <itemId> [n]` (creates items), `.arpg loot sim <itemId> <lvl> 1000` (stats only, no items) |
| Professions (m1b) | `.arpg test learnall <char>` (all 11 primary professions) |
| Loot pet and filter (m2b) | `.arpg lootpet on\|off\|info <char>`, `.arpg filter list\|add\|remove\|move <char> …`, `.arpg test pack <char> <entry> <n>` (spawn and kill a pack next to the character) |
| Monsters (m3) | `.arpg champion spawn <char> [affix]`, `.arpg superunique spawn <char> <id>`, `.arpg superunique list` |
| Paragon (m4) | `.arpg paragon addxp <char> <xp>`, `.arpg paragon info <char>`, `.arpg paragon reset <char>` |
| Dungeons (m5) | `.arpg dungeon info <char>` (party size, multipliers, instance level), `.arpg lfg probe <char> <dungeon>` |

A typical playtest of scaling: `.character level Me 85` → `.tele Northshire` → pull a wolf and check its
level and how long it takes to kill → `.arpg scale probe Me nearest` to see the numbers → tune
`Arpg.Scale.HealthMod` → `.arpg reload` → pull again. No rebuild, no restart.
