# Code map

What's where, and what we've touched. Paths marked **?** are TrinityCore-family guesses not yet
verified against this fork — confirm in m0 and remove the mark.

## Our code
| Path | What |
| --- | --- |
| `modules/mod_arpg/` | Our module (m1). Entry `src/mod_arpg.cpp` (`AddSC_mod_arpg`, auto-registered by upstream's module glob) |
| `modules/mod_arpg/src/ArpgConfig.{h,cpp}` | Loads `arpg.conf` (next to worldserver.exe); `.arpg reload`, `.arpg config set` overrides |
| `modules/mod_arpg/src/Scaling/WorldScaling.{h,cpp}` | m1 open-world scaling: PlayerScript tick (real players only), registry, SelectLevel hook, sweep/revert; m3 `Pin` (fixed level + mults for Champions/minions) and the first-sight new-candidate callback |
| `modules/mod_arpg/src/Monsters/Champions.{h,cpp}` | m3 Champions: first-sight roll (`Arpg.Champion.Chance`), band level/health/damage via `Scaling::Pin`, soft monster affixes, minions, kill → orb + loot floor, Champion loot +1 tier (`IsLootingChampion` via the player's loot GUID), `.arpg champion make/info/kill/loot` |
| `modules/mod_arpg/src/Loot/Potions.{h,cpp}` + `Core/Potions` | m3b potions: % heal override (ModifyHealReceived by potion spell), resource restore (OnSpellCast), drops by monster kind (OnCreatureKill), `.arpg test potion`; data `arpg_potion`, `arpg_potion_drop`, item_template + Item-sparse hotfixes |
| `modules/mod_arpg/sql/tools/gen_monster_tags.py` | generates the Champion/Superunique tag copies of creature templates (entry +5,000,000 / +6,000,000) |
| `modules/mod_arpg/src/Monsters/Superuniques.{h,cpp}` | m3 Superuniques: zone pools (`arpg_superunique`, `_spawn`, `_ability`), PerZone alive while a real player is in the zone, band kind 1 via `Scaling::Pin`, kit abilities at the aggro holder (% of target max health), rotation `Core::NextSuperunique`, kill → orb/blue/Unique ladder, `.arpg superunique spawn/next/info/kill/loot/fight` |
| `modules/mod_arpg/src/Commands/cs_arpg.cpp` | `.arpg version/reload/config set/test setup/scale probe` |
| `modules/mod_arpg/src/Core/` | Pure logic, no game headers (unit-tested): `ConfigFile`, `Scaling` (m1), `Pity`, `AffixRoll`, `AffixValue`, `LevelCurve`, `EnchantIndex` (m2), `MonsterBands`, `SuperuniqueRotation` (m3), `DisplayEnchant` (m2 display ids), `AffixEffects` (m2 effect math) |
| `modules/mod_arpg/src/Loot/` | m2: `Budget` (B), `NativeEnchants`, `AffixRoller` (roll + save + stat cache), `AffixStats` (apply on equip), `AffixEffects` (effect affixes: hooks, procs, auras; tuning `arpg_affix_effect`), `ZoneLevels` (zone ranges: scaling cap + loot ceiling), `Substitution` (level-appropriate loot), `BossPity` (boss loot tier + god-roll pity + Unique ladder per boss), `Uniques` (load, MakeUnique, UniqueForBase), `AffixRoller` also plans loot at fill time (`PlanRoll`/`ApplyRoll`, `PlanLoot`, last kill script `InstallLootPlanner`; plan keyed by item id + suffix id), `Crafting` (recipe colour from SkillLineAbility, crafted-Unique pity `character_arpg_craft_pity`; Core rules in `Core/Craft`) |
| `modules/mod_arpg/src/Test/TestEvents.{h,cpp}` | Event recorder for tests (`Arpg.Test.RecordEvents`, `.arpg test events`) |
| `modules/mod_arpg/src/{Scaling,Loot,Champions,Paragon,Dungeon,Commands}/` | Game-facing code per feature |
| `modules/mod_arpg/sql/` | Our world/characters SQL (`apply-sql.ps1`); `sql/tools/gen_affix_sql.py` generates the affix tables - edit it, not the SQL; `sql/tools/blue_stats.py` measures real item stats per level (affix calibration, BALANCE §16) |
| `modules/mod_arpg/conf/arpg.conf.dist` | All tunables |
| `arpg/unittests/` | Unit tests (ctest `-R arpg`): tiny header-only harness, compiles `mod_arpg/src/Core/*.cpp` only. Outside the module because upstream globs every module `.cpp` into the server |
| `arpg/scripts/` | setup-tools, gm (SOAP), run-gmtests, restart-world, update-upstream, write-confs (server confs from .conf.dist + overrides; DB login from mysql.local.cnf), new-account (game account via SQL), overnight (unattended m0 pipeline; state in ServerData/pipeline), start-stack (desktop one-click: close apps, MySQL, servers, client; -Stop), rebuild-and-test (dev loop: stop, build, ctest, SQL, start, hold Arpgtest, all gmtests; log ServerData/pipeline/rebuild.log), apply-sql |
| `arpg/tests/*.gmtest` | GM-command tests run over SOAP |
| `arpg/client-patch/`, `arpg/addons/ArpgUI/` | Client patch sources (`build-client-patch.py` = the real patch: affix display rows → `Data\enUS\wow-update-enUS-20000.MPQ`; `build-probe-patch.py` + `TEST-SCENARIOS.md` = spike tools/test sheet; StormLib built from source in `arpg/tools/bin/StormLib`), client addon |
| `openspec/` | Specs (current behaviour) and changes (proposals) |

## Upstream landmarks
| Area | Path | Notes |
| --- | --- | --- |
| Script hooks | `src/server/game/Scripting/ScriptMgr.h` | PlayerScript, UnitScript, WorldScript, CommandScript… No AllCreatureScript. `PlayerScript::OnUpdate` fires **twice** per player update (Player.cpp 1428 + 1552) |
| Creature level/stats | `src/server/game/Entities/Creature/Creature.cpp` | `SelectLevel(cinfo, keepLevel)` computes health/damage/AP for a level; `UpdateAllStats` |
| Instance scaling | `src/server/game/Maps/AutoBalanceLevelScaling.{h,cpp}` | Port of AC mod-autobalance (instances only, off by default) - base for m5 dungeon scaling |
| Module build | `modules/CMakeLists.txt` | Every subfolder with .cpp/.h is a module; all its .cpp go into lib `modules`; entry `AddSC_<folder>()` |
| Player XP | `src/server/game/Entities/Player/Player.cpp` **?** | `GiveXP` → Paragon intercept at 90 |
| Item enchant slots | `src/server/game/Entities/Item/Item.{h,cpp}` **?** | `SetEnchantment`, PROP_ENCHANTMENT slots for affixes |
| Loot | `src/server/game/Loot/` **?** | item creation on loot |
| Dungeon Finder | `src/server/game/DungeonFinding/LFGMgr.cpp` **?** | level checks, bot matching hook |
| Chat commands | `src/server/scripts/Commands/` | `std::vector<ChatCommand>` tables; we use `rbac::RBAC_PERM_COMMAND_DEBUG` (GM 3 has it; no auth DB rows needed) |
| Playerbots | `modules/mod_playerbots/` | bot AI, `.botscenario` test commands in `src/Scenario/` |
| Headless client | `contrib/headless-client/` | scripted login smoke tests |
| Config | `worldserver.conf.dist`, `playerbots.conf` | SOAP, bot counts |

## Hooks we use
| Hook | Our handler | Change |
| --- | --- | --- |
| `ArpgHooks::CreatureLevel` (our hook in `Creature::SelectLevel`) | `WorldScaling.cpp` `CreatureLevelHook` | m1 |
| `ArpgHooks::ItemPickup` (in `ScriptMgr::OnItemPickup`) | `Loot/AffixRoller.cpp` `PickupHook` | m2 |
| `ArpgHooks::ItemBonuses` (in `Player::_ApplyItemBonuses`) + `PlayerScript::OnLogout` | `Loot/AffixStats.cpp` | m2 |
| `PlayerScript::OnUpdate`, `OnLogout` | `ArpgScalingPlayerScript` | m1 |
| `WorldScript::OnConfigLoad` | `ArpgWorldScript` (loads arpg.conf) | m1 |
| `CommandScript` | `arpg_commandscript` | m1 |

## Upstream touches
Every edit outside `modules/mod_arpg/`. Keep this list short; each entry is a merge-conflict risk.
| File | Why | Marker | Change |
| --- | --- | --- | --- |
| `src/server/game/Entities/Creature/ArpgHooks.h` (new) | Function-pointer hook declarations (CreatureLevel m1, ItemPickup m2, ItemBonuses m2, LootMoney m2, LootFilled m2, ItemSellPrice m2), null unless mod_arpg sets them | `ARPG:` | m1, m2 |
| `src/server/game/Entities/Creature/Creature.cpp` | include + hook definition (top); `SelectLevel` calls `ArpgHooks::CreatureLevel` after AutoBalance | `// ARPG:` | m1 |
| `src/server/game/Scripting/ScriptMgr.cpp` | include + `ArpgHooks::ItemPickup` definition (top); `OnItemPickup` calls it first (new item for a player: loot, quest, craft…) | `// ARPG:` | m2 |
| `src/server/game/Entities/Player/Player.cpp` | include + `ArpgHooks::ItemBonuses` definition (top); `_ApplyItemBonuses` calls it (not for the level-scale-only pass) | `// ARPG:` | m2 |
| `src/server/game/Handlers/LootHandler.cpp` | include + `ArpgHooks::LootMoney` definition (top, outside the Eluna block); `HandleLootMoneyOpcode` calls it after AutoBalance (gold find) | `// ARPG:` | m2 |
| `src/server/game/Loot/LootMgr.cpp` | include + `ArpgHooks::LootFilled` definition (top); `Loot::FillLoot` calls it after world drops, before group rights (loot substitution) | `// ARPG:` | m2 |
| `src/server/game/Handlers/ItemHandler.cpp` | include; `HandleSellItemOpcode` asks `ArpgHooks::ItemSellPrice` for the payout | `// ARPG:` | m2 |
| `src/server/game/Entities/Player/Player.cpp` (2nd touch) | `ArpgHooks::ItemSellPrice` definition; `AddItemToBuyBackSlot` uses it for the buyback price | `// ARPG:` | m2 |
| `CMakeLists.txt` (root) | `enable_testing()` + `add_subdirectory(arpg/unittests)` after modules | `# ARPG:` | m1 |
