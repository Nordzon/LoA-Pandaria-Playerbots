# Code map

What's where, and what we've touched. Paths marked **?** are TrinityCore-family guesses not yet
verified against this fork — confirm in m0 and remove the mark.

## Our code
| Path | What |
| --- | --- |
| `modules/mod_arpg/` | Our module (created in m1) |
| `modules/mod_arpg/src/Core/` | Pure logic, no game headers: rolls, curves, multipliers (unit-tested) |
| `modules/mod_arpg/src/{Scaling,Loot,Champions,Paragon,Dungeon,Commands}/` | Game-facing code per feature |
| `modules/mod_arpg/sql/` | Our world/characters SQL |
| `modules/mod_arpg/conf/arpg.conf.dist` | All tunables |
| `modules/mod_arpg/tests/` | Unit tests (ctest) |
| `arpg/scripts/` | setup-tools, gm (SOAP), run-gmtests, restart-world |
| `arpg/tests/*.gmtest` | GM-command tests run over SOAP |
| `arpg/client-patch/`, `arpg/addons/ArpgUI/` | Client patch sources, client addon |
| `openspec/` | Specs (current behaviour) and changes (proposals) |

## Upstream landmarks
| Area | Path | Notes |
| --- | --- | --- |
| Script hooks | `src/server/game/Scripting/ScriptMgr.h` **?** | PlayerScript, UnitScript, WorldScript, CommandScript… |
| Creature level/stats | `src/server/game/Entities/Creature/Creature.cpp` **?** | `SelectLevel`, `UpdateAllStats`, base stats lookup |
| Player XP | `src/server/game/Entities/Player/Player.cpp` **?** | `GiveXP` → Paragon intercept at 90 |
| Item enchant slots | `src/server/game/Entities/Item/Item.{h,cpp}` **?** | `SetEnchantment`, PROP_ENCHANTMENT slots for affixes |
| Loot | `src/server/game/Loot/` **?** | item creation on loot |
| Dungeon Finder | `src/server/game/DungeonFinding/LFGMgr.cpp` **?** | level checks, bot matching hook |
| Chat commands | `src/server/scripts/Commands/` **?** | examples for `.arpg` commands |
| Playerbots | `modules/mod_playerbots/` | bot AI, `.botscenario` test commands in `src/Scenario/` |
| Headless client | `contrib/headless-client/` | scripted login smoke tests |
| Config | `worldserver.conf.dist`, `playerbots.conf` | SOAP, bot counts |

## Hooks we use
| Hook | Our handler | Change |
| --- | --- | --- |
| _(none yet)_ | | |

## Upstream touches
Every edit outside `modules/mod_arpg/`. Keep this list short; each entry is a merge-conflict risk.
| File | Why | Marker | Change |
| --- | --- | --- | --- |
| _(none yet)_ | | | |
