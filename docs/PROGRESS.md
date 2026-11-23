# Progress

The single source of truth for where work stands. Every session starts by reading **Now** and ends by
updating **Now** and adding a **Log** line. If this file and memory disagree, this file wins.

## Now
- **Active changes:** `m2-affix-loot` (roll, save, calibration to real blues, stats on equip: done; next 4.6 MPQ spike,
  4.5 sell, 4.4 pity, 4.3 effect affixes), `m3-champions-superuniques` (Core bands + SQL pulled forward).
  Done but not archived: `m0-server-setup` (except owner's real-client login + first upstream sync), `m1-world-scaling`,
  `m1b-all-professions` (server side).
- **Where we stopped (2026-11-22 20:08):** owner away (playing with his son). Owner's build queue DONE and tested: **all 16
  GM files pass** (00-32), 65 unit tests. (1) splash/chain procs skip neutral bystanders; guard/bot Superunique kills wait
  the respawn delay; (2) loot rolled at the kill, loot window shows the final name, pickup applies the plan; (3) Champion /
  Superunique tags (creature copies, "Champion <name>" Rare, "Unique <name>" Rare Elite); (4) potions (m3b): % heal/resource
  for every class, drops by monster kind, 999 stacks, level 1, 10 s shared cooldown via item script + Item-sparse hotfixes.
- **Next step:** owner checks in game (headless can't): Champion/Unique names + silver / winged-silver portraits, loot-window
  names before pickup, potions (999 stack shown, 10 s sweep on all potion buttons, mana potion on a rage class), Superunique
  casts at a bot tank. m3 left: glow aura + 4.3 playtest (owner). NOW: m4 Paragon (change `m4-paragon`): Core, tables, XP at 90 -> Paragon, `.arpg paragon info|set|xp`, test 40 done; points + bonuses + ArpgUI Paragon window done (owner to try in game: `/paragon`); world tier in the Vale done (test 40). m4 left: 4.3 playtest (>= 5 levels per 2-hour session), loot tier T6-T8 gating comes with m5 raids. m3/m3b/m4 stay open until the owner's playtests. NOW m5: party table x raid tier x solo role (AutoBalance hook), solo tank/healer damage, level selector done (test 50). Ascended T6-T8 done (test 50). Raid probe done (Molten Core solo: 0.115 / 0.288, Instance.IgnoreRaid = 1 via write-confs). m5 left: 4.3 playtest (owner). NOW m2b: loot filter done (rules, commands, applied at loot planning; test 33). loot pet v1 done (test 34). companion roles v1 done (gossip menu: sell+repair, bank, mailbox; reforge/transmog/void flags set - owner to check in game if those windows open). NEXT m2b: overflow mail (full bags), 3b.3 companion appearance + reagent vendor list, 2.4 highlight beam, 4.x addon (pet toggle, filter panel) Later: m6 custom items via Item-sparse hotfixes - ask owner before m6 design choices, world tier in the Vale, `.paragon` player commands. Idea to raise with owner: custom items via Item-sparse
  hotfixes (typed blank bases, catalysts, essences) before m6. Parked: sell price.
- **Blockers:** owner (m2b, in game): right-click the loot companion - do Sell/repair, Bank, Mailbox open? do reforge/transmog/void windows open at all (flags set, no server opener)? owner (m2b): filter "Leave" removes the item from the corpse (the client auto-loots what it sees); keep, or add a core hook so it truly stays? owner: potions (change `m3b-potions` drafted) - cooldown 10 s? carry stack 20 or cap ~10 per type? owner question: catalysts and essences (m1b 2b.3) need item ids - reuse unused existing items or wait for custom items (Item-sparse patch, m6)? Champion glow aura: pick a spell id in game. Client: `Wow-64-all.exe` (unlocked; logs in AND loads our client patch `wow-update-enUS-20000.MPQ`).
- **Bots:** on for play (300) via start-stack / final stage; off for ARPG feature tests. Bot rules in m7.

## Roadmap (one change folder each; propose the backlog ones from the spec Doc when their turn comes)
| # | Change | Status |
| --- | --- | --- |
| 0 | m0-server-setup | done except owner's real-client login + first upstream sync |
| 1 | m1-world-scaling (module skeleton, `.arpg` commands, test harness, scaling) | live + tested; archive pending |
| 1 | m1b-all-professions | server side done |
| 2 | m2-affix-loot (+ MPQ-loading spike) | **active**: roll/save/calibration done, stats on equip in test |
| 2 | m2b-loot-pet-filter | proposed |
| 3 | m3-champions-superuniques (a different Superunique respawns 5 min after a kill) | started: Core bands + SQL |
| 4 | m4-paragon-addon | backlog |
| 5 | m5-dungeon-scaling (party size 1–5, leader's level, difficulty knob) | backlog |
| 6 | m6-client-patch (native affix lines + names, Uniques, charms, race/class combos) | backlog |
| 7 | m7-bots (enable bots, verify bot rules, Dungeon Finder hook, optional crowding) | proposed |
| – | multiplayer with friends (client updater, Tailscale, group rules) | idea |
| – | loot pet gathers herbs/ore | idea |

## Upstream sync
- Branch tracked: `playerbot` (beta: `playerbot-beta`, cherry-pick only) · Last sync: _(none yet)_ · Cadence: every 2–4 weeks, or when a fix we need lands

## Environment facts (keep current)
| Item | Value |
| --- | --- |
| Repo | `D:\WoWMoPProject\LoA-Pandaria-Playerbots` |
| MySQL | 8.0.45, Server only, service `MySQL80`, port **3307**, StartType Manual |
| OpenSSL for the build | `C:/Program Files/OpenSSL-Win64-3.0` (3.0.22 full; libs in `lib/VC/x64/MD`); 4.0.0 in `OpenSSL-Win64` for other work — don't use. System32 has OpenSSL 3.5.2 DLLs (other app) → copy 3.0 DLLs next to server exes |
| Boost | 1.87.0 at `C:/local/boost_1_87_0` (= upstream CI; static libs; `BOOST_ROOT` only in `.vscode/settings.json`). 1.84 at `C:/local/boost_1_84_0` unused |
| Python | 3.14.10 via Python install manager (`py`) |
| DB GUI | DBeaver |
| Server DBs | `auth`, `characters`, `world`, `playerbots`; MySQL user `arpg` (password only in gitignored `arpg/scripts/mysql.local.cnf`) |
| Server data | `D:\WoWMoPProject\ServerData\` (db-dumps now; extracted dbc/maps/vmaps/mmaps later) |
| WoW client | `D:\WoWMoPProject\MOP-5.4.8.18414-enUS-Repack`, 5.4.8 build 18414 enUS. **Run `Wow-64-all.exe`** (unlocked: custom MPQs, from wowmodding.net file 367, audited) - fallback `Wow-64.exe` (from the owner's `D:\Downloads\Client-…-001.zip`; small private-server patch, classic login). `Wow-SF*.exe` = Stormforge builds, force Battle.net login → don't work. `Config.wtf`: only `SET realmlist "127.0.0.1"`, no portal (start-stack enforces it). Spare exes: `D:\WoWMoPProject\client-exes` |
| Toolchain | MSVC 14.44 (VS Build Tools 17.14), Windows SDK 10.0.22621 + 10.0.26100 |
| Machine | 24 logical CPUs, 32 GB RAM, fixed 26 GB pagefile. Build capped: `cmake --build build --config RelWithDebInfo -- /m:2 /p:CL_MPCount=6 /nodeReuse:false` (`--parallel` → out of memory, C3859/C1076) |
| Git remotes | origin = `git@github.com:Nordzon/LoA-Pandaria-Playerbots.git`, upstream = 1LineAtaTime (https) |
| SSH | Git uses Windows OpenSSH (`core.sshCommand`); github.com host key accepted |

## Log (newest first, one line each)
- 2026-11-23 04:01 — m2b companion roles: NPC flags from `Arpg.Companion.*` (cleared in combat), gossip menu opening the native vendor/repair, bank and mailbox windows. **All 20 GM files pass.**
- 2026-11-23 03:49 — m2b loot pet v1: `Loot/LootPet` (follow, kill queue, corpse walk + reach timeout, transfer via the core store path, re-summon, persists), `.arpg lootpet`, test 34; test pack `kill loot`. **All 20 GM files pass.**
- 2026-11-23 03:00 — m2b loot filter: `Loot/LootFilter` (+ Core), `character_arpg_lootfilter`, `.arpg filter list|add|remove|move|reset`, applied when loot is planned; test 33. **All 19 GM files pass**, 79 unit tests.
- 2026-11-23 02:05 — m2b started: `Core/LootFilter` (ordered rules, protected items, text form; 79 unit tests).
- 2026-11-23 01:57 — m5 raids: solo raid entry (`Instance.IgnoreRaid = 1`), test 50 enters Molten Core (raid table x tier 1). **All 18 GM files pass.**
- 2026-11-23 01:30 — m5 Ascended affix tiers T6-T8 in raids (raid tier + Paragon gate, values 110/120/130% of T5, `.arpg test give … raid <tier>`). **All 18 GM files pass**, 74 unit tests.
- 2026-11-23 01:06 — m5 dungeon scaling: AutoBalance on, `// ARPG:` hooks (InstanceScale, InstanceLevel), party table/raid tiers/solo roles, level selector, test 50 (solo Deadmines 30%/30%). **All 18 GM files pass.**
- 2026-11-23 00:05 — m5 started: change folder (party table, raid tiers, solo roles, level selector, Ascended gate) + `Core/InstanceScale` (73 unit tests).
- 2026-11-22 23:53 — m4 Vale world tier committed; test 23 revives the character before the XP step (aggro'd worgs could kill it). **All 17 GM files pass.**
- 2026-11-22 23:11 — Vale world tier works (test 40), test 31 skips a bot-killed Superunique delay; test 23 XP affix check failed once - paused for the owner's usage reset (23:40), uncommitted.
- 2026-11-22 22:40 — Paragon Core tab: any of strength/agility/intellect/stamina (owner: no main-stat guess); addon shows the four. **All 17 GM files pass.**
- 2026-11-22 22:24 — ArpgUI addon: Paragon window (XP bar, 4 tabs, + buttons, free respec, P launcher) over `.paragon` player commands and a hidden `ARPGP|` data line; installed to the client. **All 17 GM files pass.**
- 2026-11-22 22:07 — m4 Paragon points: D3 rotation, spend/respec (free), bonuses Core/Offense/Defense/Utility applied on login and spend; test-unique equip leaves combat first (test 26 flake). **All 17 GM files pass.**
- 2026-11-22 21:29 — m4 Paragon step 1: kill/quest XP at 90 -> Paragon XP (`Paragon/Paragon`, `character_paragon`), curve rounded (float conf), test 40. **All 17 GM files pass**, 70 unit tests. Commits now on branch `arpg`, pushed to origin (owner).
- 2026-11-22 20:29 — Superuniques drop their own Unique (`arpg_superunique_unique`, 6 Uniques shared by theme), native copies of pool rares despawn on sight; test 23 pack `aggro` (neutral worgs). **All 16 GM files pass.** Started m4: change folder + `Core/Paragon` (70 unit tests).
- 2026-11-22 20:15 — m3 3.4 done: Superunique patrol (roam 15 yd) + zone announce; test 30 covers Champion bands 5/20/90. **All 16 GM files pass.**
- 2026-11-22 20:08 — Owner queue done, **all 16 GM files pass**: proc target filter, Superunique respawn delay on any death, roll-at-kill loot plans (names in the loot window), Champion/Unique tag copies (`gen_monster_tags.py`), potions m3b (item script, hotfixes, test 32). Found: Item-sparse hotfixes deliver item changes without a client patch.
- 2026-11-22 19:40 — Owner in game: Uniques OK (Long Road speed, Stormcaller tooltip + Static Field procs), Superuniques fight and drop blues, Unique ladder working (won on Gruff, base in corpse). Wants: roll visible at the kill, Champion/Unique name tags + rare portraits, potions (999 stacks, mana/rejuv). Sell price parked.
- 2026-11-22 19:15 — Drafted change `m3b-potions` (proposal, spec delta, tasks): % of max health heals 10-20% by monster tier, short shared cooldown via client+server DBC. Waiting on owner for cooldown/carry rule.
- 2026-11-22 19:08 — Superunique/Champion casts go at the aggro holder (guards in Goldshire showed it works); test tool `fight` fixates on the character. **All 15 GM files pass.** Owner idea pending: ARPG potions (% heal by monster tier, short cooldown) - change folder after his answers on cooldown and carry cap.
- 2026-11-22 18:58 — m3 3.4 Superuniques (`Monsters/Superuniques`, `.arpg superunique …`, test 31): **all 15 GM files pass**. Owner rule: abilities go at the aggro holder (bot tank included), SPEC 4.3.
- 2026-11-22 15:47 — m3 2.2 Superunique data: 13 Superuniques in Elwynn/Westfall/Dun Morogh (existing rares), 28 spawn points from their creature rows, 2 ability kits (brute/caster); SQL applied to the world DB.
- 2026-11-22 15:41 — m3 3.2 monster affixes: Cold (Chilled aura), Lightning (sparks), Fire (delayed dodgeable death blast), Arcane Caster (interruptible cast, damage = % max health); `.arpg champion make … affix <ids>`, `.arpg champion strike`. **All 14 GM files pass.**
- 2026-11-22 15:28 — **All 14 GM files pass.** Unique drops (boss ladder 20%+20%, test 27), crafting wheel (recipe colour tiers, max-skill +1 affix, crafted Uniques 2%+1%/craft per profession, catalyst flag; `Core/Craft`, `Loot/Crafting`, test 28), Champions step 1 (first-sight 2% roll, band level/mults pinned, minions, soft affixes, orb, green+blue floor, +1 tier loot; `Monsters/Champions`, test 30). clearbags strips worn gear; test 05 starts at 15; DescribeEffects shows auras with no affixes.
- 2026-11-22 14:41 — m3 Uniques step 2: powers fire in combat (Static Field on hit, Long Road +10% above the cap, when-struck absorb, every-Nth-hit, on-heal; per-item cooldowns; Power = max(AP, SP)). Test 26 passes; all 11 GM files pass. Next: Unique drops (boss/Superunique ladder 20%+20%, BALANCE §13) then the crafting wheel.
- 2026-11-22 14:33 — m3 Uniques step 1: `Loot/Uniques` (load, MakeUnique on a droppable base of the slot via the substitution index, power tier + rolled numbers stored in `item_instance_arpg_unique`), 1,422 power tooltip rows + 6 name rows in the client patch, `.arpg test unique`, max 4 affixes on Uniques (`RollRequest.maxCount`, unit test). **All 11 GM files pass.** Next: step 2 = powers fire in combat (effect machinery).
- 2026-11-22 14:25 — **All 10 GM files pass.** m2 4.4 boss loot: items from bosses roll exactly at the loot level's max tier (`RollRequest.tierAtMax`, unit test), god-roll pity every 3rd boss kill (test 25). m3 Unique SQL written (6 powers, T1-T8 template, 6 Uniques, item table); next: Unique game wiring.
- 2026-11-22 14:50 — Owner decided typed blank bases (SPEC 4.1, BALANCE §8c). World loop froze 14:17-14:45 (console QuickEdit selection pause, no crash): new `arpg/scripts/no-quickedit.ps1`, `restart-world.ps1` turns QuickEdit off for the two server windows only.
- 2026-11-22 14:30 — Owner refined blank bases → typed bases (armour type's main stat + Stamina as item data, up to 5 affix lines; Uniques 4 affixes + power line): BALANCE §8c. m3 Core/UniquePower (3 tests, 55 total).
- 2026-11-22 14:20 — Owner saw named 5-line items in game ("... of the Sage/Wind/Swiftness"). m2 4.5 sell value done (`ArpgHooks::ItemSellPrice` for sale AND buyback; test 21 exact). 4.4 boss pity written (`Loot/BossPity`, test 25, rebuild running). Blank-base proposal → BALANCE §8c (needs owner decision).
- 2026-11-22 13:57 — m2 4.10 done: affixed items get a random-suffix id (30000 + highest-tier affix) → 5 lines (slots 8-12) + D2 names ("... of the Titan"); suffix rows in client patch + server DBC (original kept as .blizzard). Test 10 made deterministic (fixed spawns, no far teleport). All 9 GM files pass. Next: 4.5 sell hook (one hook for sell AND buyback price - no gold loop).
- 2026-11-22 13:45 — In game: effect lines show ("Cheat death: heals 20%..."). Suffix probe: a custom random-suffix id makes the client show all 5 slots 8-12 AND append the suffix to the name → task 4.10 (5 lines + D2 names) replaces the slot-7 idea.
- 2026-11-22 13:31 — Resumed. Run from 10:49: **all 9 GM files pass** (zone cap: Northshire scales to 20 for a level 60; loot level: Northshire keeps, Westfall swaps to req 28-33). m2 4.8/4.9 done. Owner: crafting wheel → SPEC 4.8 + m1b tasks 2b.x.
- 2026-11-22 10:50 — Effects test 23 passes in full (procs, lifesteal, thorns, speed aura cap, XP). 4.8/4.9 built: `arpg_zone_level` (263 zones from original quest levels, `gen_zone_levels.py`), scaling cap zone max + 15 (Vale 5840 uncapped), loot level ceiling, loot substitution in the loot window (`ArpgHooks::LootFilled`, `Loot/Substitution`, index from items that really drop), test 24. Unique power design written (BALANCE §8b). Test tools: setup revives dead test chars, handles in-transfer chars. Paused for usage reset.
- 2026-11-22 10:15 — m2 4.3: effect affixes implemented (`Core/AffixEffects` 4 tests, `Loot/AffixEffects`: leech, heal/power on hit, Fire Nova/Chain Lightning/Frost Nova/poison/bleed/crushing blow procs, thorns, struck chain, elite dmg/DR, martyr, Warding, Phoenix, regen, move/cast speed auras, armour %, healer heal->damage, atonement, XP, gold find (new `ArpgHooks::LootMoney`), magic find); tuning in `arpg_affix_effect`; 6 effects not yet active get weight 0; effect display rows in the client patch (57,955 rows).
- 2026-11-22 10:00 — m2 4.7 done: **affixes show natively** - client patch with 23,988 "+N stat" rows (`build-client-patch.py`, ids 60001-83999), roller writes exact display ids into slots 9-12, stats still from AffixStats. Owner confirmed in game (+147 Parry/+168 Stamina/+302 Crit, match=yes). SPEC 4.1: loot substitution with zone ceiling (owner decision) → task 4.8.
- 2026-11-22 09:45 — **Client patch works**: owner found an unlocked 5.4.8 x64 exe (wowmodding.net file 367); audited (18 bytes in 6 spots, no new code, Defender clean). With a plain DBC in `wow-update-enUS-20000.MPQ` the changed row 74 and new rows 5300/60000 all show on tooltips. start-stack launches `Wow-64-all.exe`. m6 unblocked.
- 2026-11-22 09:30 — MPQ spike parked after 9 in-game rounds: archive now matches Blizzard's in every measurable property and StormLib applies it, but Wow-64.exe crashes reading any non-Blizzard archive data → needs an unlocked exe. StormLib built from source (arpg/tools/bin/StormLib). Probe removed. Next: m2 4.5 sell hook.
- 2026-11-22 08:55 — MPQ spike, part 2 (with owner in game): Wow-64 loads `wow-update-enUS-18299.MPQ` (plain DBC crashed it on Enter World = it reads our file); Blizzard update entries are `PTCH` patches; WDBX's StormLib can't write them. Found + fixed: client hides enchant slot 8 (reforge) → native affixes use slots 9-12, probeitem uses 9; all 7 GM files pass.
- 2026-11-22 08:05 — m2 4.6 spike done: Wow-64.exe loads no custom MPQ (wow-update-*/patch-* names, v2/v4; verified with Restart Manager - earlier "loaded" hits were Defender scans). In game: native affix lines show (+1 Agility/+1 Stamina on a rolled robe). Probe files removed from the client.
- 2026-11-22 07:38 — **Owner logged in with the real client** (m0 4.1): `Wow-64.exe` from his Client download (20-byte private-server patch) uses classic login (capture: challenge build 18414, proof OK). start-stack now launches it and rewrites Config.wtf to realmlist-only before each start.
- 2026-11-22 07:35 — Real-client login diagnosed with a logging relay: Stormforge exe forces Battle.net 2.0 login (`arpgadmin@`), authserver speaks classic only → blocker (stock 5.4.8 exe needed, or a bnet2 port). Config.wtf back to realmlist only; authserver log level restored.
- 2026-11-22 07:25 — Real client: Battle.net error #2. Cause: Config.wtf portal had no port (SF client defaults elsewhere; Stormforge used host:1118). Fixed: SET portal "127.0.0.1:3724".
- 2026-11-22 07:15 — m2 4.6 spike: client update MPQs are binary-diff chains → patches carry full files. Probe `wow-update-enUS-18414.MPQ` (patched SpellItemEnchantment.dbc + row 60000) built + installed in the client; `.arpg test probeitem` added; all 7 GM files pass. m3 Core `SuperuniqueRotation` (4 tests, 41 total). SPEC 4.10: living AH plan.
- 2026-11-22 07:06 — m2 4.1d done: affix stats count on the character (hook `ArpgHooks::ItemBonuses` in `Player::_ApplyItemBonuses`, `Loot/AffixStats`, in-memory cache, per-character applied set). New `.arpg test equip/unequip`, `.arpg loot applied`; 22-affix-stats passes (exact). All 7 GM files pass.
- 2026-11-22 07:06 — m3 pulled forward: change folder, `Core/MonsterBands` (7 unit tests, 37 total pass), `arpg_monster_band`/`arpg_monster_affix` SQL applied. Bot-affix plan added to SPEC 4.10.
- 2026-11-22 06:50 — Rebuild with `.arpg test clearbags` OK; all 6 GM files pass; live rolls match BALANCE §16 (L30 Titan T2=7, L90 Swift T4=420). World running, bots on.
- 2026-11-22 06:50 — Owner: calibrate affixes to real items. `blue_stats.py` measured blues per level: main line = 43% of B, secondary 34%, stamina 50% (steady at all levels). Stat affixes now T1 30–45% → T5 85–100% of a blue line (was 6–9% → 18–22% of B, ~2× weaker); SQL regenerated + applied, GM tests pass. Low-level floor question closed. Added `.arpg test clearbags` (Arpgtest's bags were full). SPEC 4.1 + BALANCE §16 updated.
- 2026-11-22 05:38 — m2 4.1c: affixes roll on new items (hook live). **6 GM files pass** (new 21-affix-loot). Sample: L90 epic robe → of the Titan T3=239, of the Wind T3=236, Deadly T3=219; L12 green → +1 values (floor proposed in BALANCE §15).
- 2026-11-22 04:37 — m2 4.1b: native enchant index (30 unit tests, 5 GM files pass). Measured client coverage: main-stat enchants only 1-46 → decision: all affix stats applied server-side at exact value, native slot only within ±5%, tooltips via ArpgUI/patch; MPQ spike (4.6) priority up. design.md + SPEC updated.
- 2026-11-22 03:2x — m2 4.1a: design.md (roll in OnItemPickup for loot/quest/craft sources; native stat affixes via nearest SpellItemEnchantment in PROP slots, all affixes in item_instance_arpg_affix). Upstream hook `ArpgHooks::ItemPickup` added in ScriptMgr::OnItemPickup (// ARPG:, CODEMAP updated). Not built yet.
- 2026-11-22 03:07 — m2 4.2 done: **5 GM test files pass** (new 20-affix-budget). B at 90 chest 1710 → T5 main-stat affix ≈ 310-375 (≈1/3 of a 463 chest's base stat).
- 2026-11-22 03:0x — m2 4.2 B budget: Core/LevelCurve (3 tests, 28 total pass), Loot/Budget, `.arpg loot budget`, 20-affix-budget.gmtest. First build failed on a header-order error (ItemPrototype.h not self-contained -> include ObjectMgr.h); old binary restarted by the script; retrying (compile-check first).
- 2026-11-22 01:4x — m2 3.1: affix tables (48 affixes, 240 tiers) from a generator (`modules/mod_arpg/sql/tools/gen_affix_sql.py`), pity table; applied. Next: 4.2 B budget, 4.1 item-creation hook.
- 2026-11-22 01:34 — rebuild-and-test: build + unit + server + **4 GM test files pass** (smoke, test tools live, world scaling, professions). Fixed kit bugs: gm.ps1 crashed on non-XML SOAP faults; runner waited a fixed 25 s for login (now waits for LOGIN-OK); tests require `(online)`.
- 2026-11-22 — Added rebuild-and-test.ps1 (unattended dev loop) + 05-test-tools.gmtest; Core/AffixValue (4 tests; 25 unit tests pass). First rebuild-and-test run in progress.
- 2026-11-22 00:58 — Overnight pipeline finished: 301 characters online in 51 zones; all stages OK.
- 2026-11-22 — Early game (BALANCE §14, SPEC 4.3, TESTING C1-C4): Champion/Superunique level bands (gentle at 1-14, full at 35/60), soft affixes only below 15, affix damage as % max health, health orbs 20%/40%.
- 2026-11-22 01:1x — m2-affix-loot change folder created. Event recorder + `.arpg test events/pack` written (compile check running). Core/AffixRoll + 6 rule tests: 21 unit tests pass.
- 2026-11-22 00:57 — Pipeline: arpg build OK, final OK (bots on, navmesh walk), **all GM tests pass** (smoke, world scaling live, professions 11/11). Bots stage counting.
- 2026-11-22 — mmaps done in 28.6 min (6744 tiles). Pipeline building mod_arpg; my parallel build collided with it (tlog lock) - pipeline's build continues. 10-world-scaling.gmtest extended (levels 30/60/85/90, offsets, cap); TESTING scaling matrix W/D/R/P/S.
- 2026-11-22 — TESTING §3c test catalogue (owner rules: trust engine procs, no statistical rate tests, force outcomes via config/counters, every feature ships unit + gmtest + scenario where it matters; coverage table per feature).
- 2026-11-22 — CLAUDE.md: "Autonomous work and the scheduler" rules (resumable scripts, CronCreate check-ins every session, no blocking, one build at a time). m2 started test-first: Core/Pity (ladder, god-roll pity, Ascended tiers) — 15 unit tests pass (standalone build). BALANCE: Superunique pity = avg 5-6 kills, max 19.
- 2026-11-22 — BALANCE §10-13 + SPEC: healers solo (knob 1.6, Radiant/Zealous/Atonement/Martyrdom, healer Uniques), dungeon level selector, Paragon-gated Ascended tiers T6-T8, pity ladder 20%+20%/run (guaranteed run 5). TESTING §3b: dungeon/raid scenario matrix (headless + SOAP probes, loot sim). Fixed m1 compile error (getMSTimeDiff).
- 2026-11-22 — docs/BALANCE.md (BrM vs Rogue god rolls, solo dungeon math, raids as Paragon-gated endgame tiers, tiered Uniques, gold). Condensed rates in write-confs (move 1.15, drops 2/2/1.5, gold 1.5, skill gain 2, gather XP 3, 11 professions). SPEC updated. m1b server part done (config) + `.arpg test learnall`. Smoke passed (headless LOGIN-OK).
- 2026-11-22 — m1 started: mod_arpg skeleton, arpg.conf, .arpg commands, world scaling, unit tests (pass). Upstream touches: ArpgHooks.h, Creature.cpp SelectLevel hook, root CMake test line. SPEC: XP 1.5x, affix pool v2, companion roles.
- 2026-11-22 — First full build OK (worldserver + authserver, 0 errors) with capped parallelism.
- 2026-11-22 — Decision (owner): bots ON for play now (300), off for ARPG tests; SPEC + CLAUDE.md updated. Added start-stack.ps1 + desktop shortcuts (Start / Stop). Pipeline relaunched with final stage bots on + bots stage.
- 2026-11-21 — Added restart-world.ps1 (4.3), new-account.ps1, overnight.ps1; client junctions dbc/maps/vmaps/mmaps/cameras/Buildings → ServerData\data; Config.wtf → 127.0.0.1. Overnight run launched, Claude check-ins every 30 min.
- 2026-11-21 — Owner closed apps to free memory. MySQL user `arpg` + 4 DBs created; ci-playerbot dumps (sha256 OK) imported. Added write-confs.ps1.
- 2026-11-21 — Kit committed (dc18b649, not pushed); upstream dry run clean. Build command capped in CLAUDE.md, update-upstream.ps1, VS Code F7.
- 2026-11-21 — First configure OK (OpenSSL 3.0.22, Boost 1.87, MySQL, Playerbots). First build `--parallel` failed: out of memory (C3859 PCH); rerun capped at /m:2 × /MP 6.
- 2026-11-21 — Tools verified (sigs/hashes, Defender scan, Boost+OpenSSL+MySQL smoke build). Decision: Boost 1.84 → 1.87.0 (upstream CI); SPEC updated. Fixed `,vscode`→`.vscode`, OpenSSL lib paths, setup-tools.ps1 (OpenSSL/Boost/client detection), non-ASCII in update-upstream.ps1 (PS 5.1 parse error); tools → `arpg/tools/bin`; upstream remote added.
- 2026-11-21 — Handover to Claude Code. Bots moved to the end (m7). Boost 1.84 chosen. setup-tools.ps1 Python detection fixed.
- 2026-11-21 — Repo cloned, kit copied in. Tools check: Git, CMake, MSVC, MySQL 8.0.45, Node, OpenSpec OK.
- 2026-11-18 — Affix pool v1 drafted in the spec Doc; loot pet + filter, all professions, 5-min Superunique respawn added.
- 2026-11-18 — Added update-upstream.ps1 and the upstream sync rules.
- 2026-11-15 — Spec agreed; kit created (setup script, OpenSpec layout, CODEMAP, TESTING, SOAP test runner).
