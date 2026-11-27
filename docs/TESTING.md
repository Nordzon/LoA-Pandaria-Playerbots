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
in milliseconds. Example: rolls of a blue item at level 60 always give 2–3 affixes, never an affix above the
tier unlocked at 60, never two of the same affix (rules, not rates: see §3c).

## 2. GM tests over SOAP (no client needed)
SOAP lets scripts send GM commands to the running worldserver. Enable it in `worldserver.conf`
(`SOAP.Enabled = 1`, `SOAP.IP = "127.0.0.1"`, `SOAP.Port = 7878`) and put a GM account in
`arpg\scripts\soap.local.ps1`. A `.gmtest` file is a list of commands and expected replies:

```
> .arpg test setup Arpgtest 85 elwynn-northshire
> .arpg scale probe Arpgtest nearest
? scaled=85\b
```
`!` = must not match, `~ 500` = wait 500 ms, `*` = like `?` but re-sends the last command every 2 s until it matches (20 s max;
for things that happen on a timer, such as the scaling tick).

SOAP has no selected target, so our debug commands always take a player name or GUID. The test
character `Arpgtest` is created and moved by `.arpg test setup`, so tests don't depend on where you logged off. A second real test character, `Arpgtesttwo` (account ARPGTEST2, password in `headless.local.json` account2/password2), is held online too for the multiplayer tests (55-friends, m8).

## 3. Bot scenarios
LoA-Pandaria-Playerbots ships `.botscenario` GM commands to place, group, queue and probe bots in
repeatable setups (`modules/mod_playerbots/src/Scenario/`). We use them for party-size scaling and the
Dungeon Finder hook: e.g. queue `Arpgtest` (level 50) for Deadmines and check the bots matched are 47–53.

## 3b. Scenario tests: dungeons, raids, power levels (m5, m4, m2)
Instance scaling and loot pity are the riskiest systems, so they are tested **without a person**: the headless client holds a test
character in the world (`Arpgtest`, created by the overnight pipeline), GM commands over SOAP set the scene, probes read the
numbers, and headless "play" verbs fight for real. Files: `arpg/tests/5x-*.gmtest` (state checks) and
`arpg/scenarios/*.steps` (headless fight scripts, run by `arpg\scripts\run-scenarios.ps1`, which writes a CSV per run).

**Building blocks (added with the change that needs them):**
| Command | What it does | Change |
| --- | --- | --- |
| `.arpg test setup <char> <lvl> <spot>` | level + teleport; spots include dungeon/raid **interiors** (`arpg_test_spot`, e.g. `dungeon-deadmines-entrance`, `raid-msv-stoneguard`), which creates the instance for that character | m1 (done), spots added in m5 |
| `.arpg test gear <char> <preset>` | equips a preset (`brm-t3`, `rogue-t5-godroll`, `holy-t1`…) from `arpg_test_gear`, so power is reproducible | m2 |
| `.arpg test spec <char> <spec>` | switches talent spec (tank/healer/DPS paths) | m5 |
| `.arpg test paragon <char> <level>` | sets Paragon level + auto-spends points by a named build | m4 |
| `.arpg dungeon level <char> <n>` | the level selector, also used by players | m5 |
| `.arpg dungeon probe <char> nearest` | party size, chosen level, tier, health/damage multipliers, solo-role knob in effect, for the nearest creature | m5 |
| `.arpg loot sim <char> <boss> <runs>` | simulates N boss kills with the real loot + pity code (no items created): Unique run, affix counts, tiers | m2/m5 |
| `.botscenario <char> …` | upstream: groups bots with the character for party sizes 2–5 | m7 / used in m5 |

**Matrix the scenarios cover** (each cell = one scenario run, ~2–5 min headless):
| Axis | Values |
| --- | --- |
| Content | 1 classic dungeon, 1 MoP dungeon, 1 raid per tier band (1, 3, 5, 7) |
| Party size | 1 (solo), 3 (bots), 5 (bots) |
| Chosen level | dungeon minimum, middle, player level (selector) |
| Spec | Brewmaster, Rogue, a healer (Holy Paladin), one caster |
| Power | gear preset × Paragon (0, 25, 60, 110) |

**What each scenario asserts** (CSV columns; thresholds live in the scenario file and start loose):
- Scaling: probe shows the expected level and multipliers for that party size, tier and role (exact match, from the spec tables).
- Viability: the headless character clears a trash pack and the first boss (`kill ENTRY` verbs): **time-to-kill**, **deaths**,
  lowest health %. Example: solo Brewmaster at the dungeon's own level clears pack 1 with 0 deaths in < 3 min.
- Progression: the same raid at Paragon 0 vs 60 → kill time and deaths must drop (Paragon must matter).
- Selector: running at a lower chosen level → creatures probe at that level, loot sim shows matching affix tiers.
- Pity: unit tests prove the ladder (20/40/60/80/100%); in game, `.arpg pity set <char> <boss> 4` then one boss kill must
  drop the Unique, and the counter must read 0 afterwards (deterministic, no repeated runs).
- Loot tier: at raid tier 5, Paragon 60 → a forced boss drop carries T7 affixes; Paragon 59 → highest tier T6.

The full matrix is too long for every build; `run-scenarios.ps1 -Quick` runs one cell per axis (about 10 minutes) after builds,
and the full matrix runs overnight (it fits next to bots and mmaps on this machine). Results feed `docs/BALANCE.md` tuning.

## 3c. Test catalogue: rules and coverage (owner, 2026-11-22)
**Rules**
- We trust the engine: random procs, loot-table rolls and spell maths are not re-measured. No large kill samples or
  statistical rate tests. (Our own item chances start at ≥ 10% with pity anyway; nothing we add is a 2% drop.)
- Tests check **our** behaviour: wiring (does our code run on the real event?), numbers (right level/multiplier/tier/value),
  state (saved, reverted, persisted across relog) and edge cases (caps, kill switches, combat, instances).
- **Force outcomes instead of sampling:** a test sets chances to 0% or 100% (`.arpg config set …`, restored at the end of the
  file), sets counters directly (`.arpg pity set`), or spawns exactly what it needs (`.arpg test pack`). One kill, one check.
- **Every feature ships with:** unit tests for its `Core/` logic, one `.gmtest` for its state checks, and, if it changes how play
  feels or flows, one headless scenario. Tests are added with the feature, in the same change.
- **Two shared tools come first in m2** because most tests need them: `.arpg test pack <char> <entry> <n>` (spawn next to the
  character; `kill` = killed and credited to the character) and the **event recorder** (`Arpg.Test.RecordEvents = 1`;
  `.arpg test events <char> [clear]` lists what our code did, e.g. `unique=KegBreaker removed=41150 healed=12345`).

**Coverage per feature** (✓ = exists now)
| Feature | Unit (Core) | GM test (SOAP state checks) | Headless scenario (real play) |
| --- | --- | --- | --- |
| Config / commands | ✓ parser | ✓ version, reload, config set | – |
| World scaling (m1) | ✓ formula | ✓ low zone → player level; high zone untouched; kill switch reverts; bot lock: a creature fighting a non-player stand-in keeps its level, rescales after (`.arpg scale combat`) [`10-world-scaling`] | kill a scaled mob at 85 in Northshire; time-to-kill sane |
| Quest scaling (m1) | – | quest level reads -1 for a zone quest; backup table holds the original | accept + turn in a low quest at 85, XP > 0 |
| Professions (m1b) | – | ✓ learnall → 11 of 11 | gather one node, skill rises by 2 |
| Rates (XP, move, drops) | – | config values loaded (`.server info` / probe) | walk a fixed path: time ≈ 1/1.15; kill a known mob: XP = 1.5 × base |
| Affixes on items (m2) | roll rules: counts per quality, ≤3/≤3/≤5, no dupes, tiers by level, smart loot | forced roll on a created item: affix slots filled, values in tier range, persists after relog | loot pet picks up an item, it has affixes |
| Affix stats (m2) | – | `.arpg test equip/unequip`: measured stat change = template + affix lines (exact), swap removes the old item's affixes, unequip removes all [`22-affix-stats`] | equip a rolled item in the real client: character sheet goes up |
| Pity + loot tiers (m2/m5) | ✓ ladder, god-roll, Ascended | ✓ god-roll every 3rd boss kill [`25-boss-pity`]; ✓ pity set 4 → next boss kill owes the Unique, the next boss item of its slot becomes it, counter 0 [`27-unique-drops`]; T7 only at Paragon 60 + raid tier 5 | – |
| Sell value / gold (m2) | ✓ price formula | ✓ sell price shown by `.arpg loot last` [`21-affix-loot`] | sell one 3-affix item: gold delta = formula |
| Uniques (m2+) | ✓ power tiers, steps, cooldown | ✓ tier ranges, max 4 affixes, powers fire (forced procs), passive speed above the cap [`26-uniques`] | trigger the mechanic once (cast/attack a dummy) → event recorder shows the effect with the right numbers |
| Loot pet + filter (m2b) | ✓ filter rule matching, order, protected items, text form | ✓ rules add/move/remove/reset, data line, Champion floor: green sold for gold, blue left [`33-loot-filter`]; ✓ pet empties 3 corpses, full bags -> mailed, looks persist [`34-loot-pet`] | kill a 5-pack: bags/gold/mail as the rules say; pet doesn't get stuck |
| Companion roles (m2b) | – | ✓ npc flags match config (0x3C021081), cleared in combat, look change [`34-loot-pet`] | open vendor/bank/mail on the pet (gossip); do reforge/transmog/void open? (owner) |
| Champions (m3) | ✓ bands, affix pick rules, minions, orb | ✓ `.arpg champion make`: band level, minions, affixes; pinned against the scaling tick; kill → orb + green + blue floor; looted items marked +1 tier [`30-champions`] | fight one: death explosion recorded |
| Crafting wheel (m1b) | ✓ recipe colour, mods, Unique ladder, extra affix/tier cap in the roll | ✓ `.arpg test craft` with skill override: orange/green/grey rules, catalyst +1, max skill +1 affix, crafted Unique at full pity [`28-crafting`] | craft a recipe in the real client |
| Superuniques (m3) | ✓ rotation | ✓ spawn, abilities at the aggro holder, kill → orb, blue, own Unique, a **different** one after the delay, announce [`31-superuniques`] | – |
| Paragon (m4) | ✓ XP curve, rotation, caps, world tier | ✓ XP at 90 → levels (exact curve), spend → exact rating/stamina, respec, addon data line, Vale world tier 120%/200% [`40-paragon`] | ≥ 5 levels in a 2-hour session (owner) |
| Dungeon scaling (m5) | ✓ party-size table, interpolation, solo role knobs | ✓ solo Deadmines 0.30/0.30 (non-tank) [`50-dungeons`]; 3 bots vs 5 not headless | solo pack + first boss per role (BrM, Rogue, healer): deaths, time, lowest HP % (owner) |
| Level selector (m5) | – | ✓ Deadmines at 20 applied on entry [`50-dungeons`] | – |
| Raid tiers (m5) | ✓ tier multiplier, Ascended gate, Ascended range | ✓ solo Molten Core 0.115/0.288; T8 at raid 7 + Paragon 110, T6 at raid 5 + Paragon 59, T5 outside [`50-dungeons`] | Paragon 0 vs 60 in one tier-5 raid: kill time drops |
| Potions (m3b) | ✓ % heal, resource rules, tier/kind pick | ✓ exact % heals, shared 10 s cooldown, mana potion on a rage class, Champion drop [`32-potions`] | 999 stacks + cooldown sweep shown by the client (owner) |
| Crafting wheel (m1b) | ✓ recipe colour, mods, Unique ladder | ✓ colour rules, catalyst, max skill, crafted Unique [`28-crafting`] | craft in the real client |
| Bots (m7) | – | bots online count; a creature in combat with a bot keeps its level | 300 bots + headless for 30 min: update time stays low, no crash |
| Upstream sync | all | all | quick set |

**Scaling matrix (world, dungeon, raid × level choice × Paragon)** - owner request 2026-11-22. Every cell is an exact check
(probe or one forced drop), expected values straight from SPEC 4.2/4.6 and BALANCE §5-6, §11-12. Files in brackets.
| # | Content | Setup (player level / chosen level / party / spec / Paragon) | Expect |
| --- | --- | --- | --- |
| W1 | World, Northshire (1-5) | 30, 60, 85, 90 | nearest mob `scaled=` 30/60/85/90 [`10-world-scaling` ✓] |
| W2 | World | 60 with `LevelOffset` +2 / -2; 90 with +5 | 62 / 58 / 90 (cap) [`10-world-scaling` ✓] |
| W3 | World, Jade Forest (85-86) | 80 | `scaled=none` (upward only) [`10-world-scaling` ✓] |
| W4 | World | kill switch 0 | `scaled=none`, scaled mobs revert [`10-world-scaling` ✓] |
| D1 | Dungeon (Deadmines) | 90 / chosen = dungeon min, 30, 60, 90 / solo | mob level = chosen; forced drop's affix values = B at chosen level [`50-dungeon-scaling`] |
| D2 | Dungeon | 90 / 90 / party 1-5 (bots) | health/damage = 30/40, 45/55, 65/70, 80/85, 100/100% [`50-dungeon-scaling`] |
| D3 | Dungeon | solo as BrM / Rogue / Holy Paladin | non-tank damage-taken knob 0.75 on Rogue + Holy; tank damage dealt ×1.25 on BrM; healer ×1.6 on Holy [`51-dungeon-roles`] |
| D4 | Dungeon | chosen level remembered | leave, re-enter: same chosen level [`50-dungeon-scaling`] |
| D5 | Dungeon (MoP, e.g. Temple of the Jade Serpent) | 90 / 90 / solo | level 90, solo multipliers; boss scripts still run (boss fight starts, no error in Server.log) [`50-dungeon-scaling`] |
| R1 | Raid tier 1 (Molten Core) and tier 5 (Mogu'shan Vaults) | 90 / solo / Paragon 0 | mobs at 90; health = 10% × (1 + 0.15 × tier), damage = 25% × (1 + 0.15 × tier) [`60-raid-tiers`] |
| R2 | Raid | party 1, 2, 3, 5, 10 (bots) | 10/25, 16/32, 22/40, 35/55, 100/100% before the tier multiplier [`60-raid-tiers`] |
| R3 | Raid tiers 3 / 5 / 7 | Paragon just below / at 25, 60, 110 | forced drop max tier: T5→T6 at 25 (tier 3+), T6→T7 at 60 (tier 5+), T7→T8 at 110 (tier 7) [`61-paragon-loot-tier`] |
| R4 | Raid tier 2 | Paragon 200 | max tier stays T5 (raid tier gates Ascended tiers) [`61-paragon-loot-tier`] |
| R5 | Raid | Unique tier by raid tier + Paragon | Unique I/II/III as BALANCE §8/§12 [`61-paragon-loot-tier`] |
| P1 | Paragon power | Paragon 0 / 25 / 60 / 110, auto-spent "Core" build | main stat +0.5% per point in Core (probe stats) [`40-paragon`] |
| C1 | Champion bands | `.arpg champion spawn` next to a level 5 / 20 / 40 / 85 character | level offset, health and damage multipliers, affix count and allowed affixes match the band row (5 → +0 / 2.0x / 1.15x / soft only) [`30-champions`] |
| C2 | Superunique bands | spawn by command at level 5 / 20 / 40 / 85 | band row values; no must-interrupt ability below 15 [`31-superuniques`] |
| C3 | Affix damage | Fire Enchanted forced, kill it at level 5 and 85 (event recorder) | explosion = 15% / 25% of max health, never lethal from full |
| C4 | Early fight (headless) | level-5 warrior, starter gear, Northshire Champion pack | kill within 90 s, 0 deaths; health orb restores 20% [`scenarios/early-champion.steps`] |
| S1 | Solo viability (headless) | BrM, Rogue, Holy at Deadmines chosen 90 and MSV at Paragon 0 vs 60 | records deaths / time / lowest HP; MSV at Paragon 60 must be faster than at 0 [`scenarios/raid-paragon.steps`] |
Rows marked ✓ exist now and run in tonight's GM-test stage; the rest are written with m4/m5 (their commands are listed in §3b).

**When they run:** unit tests after every build (seconds); GM tests + a quick headless set after server builds (~10 min,
`run-gmtests.ps1`, `run-scenarios.ps1 -Quick`); the full scenario matrix overnight.

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
| Scaling (m1) | `.arpg scale probe <char> nearest\|<guid>` (template vs scaled level and multipliers), `.arpg scale combat <char> <spawnId> on\|off\|status` (bot lock stand-in), `.arpg scale radius <char>` |
| Loot (m2) | `.arpg roll <char> <itemId> [n]` (creates items), `.arpg loot sim <itemId> <lvl> 1000` (stats only, no items) |
| Professions (m1b) | `.arpg test learnall <char>` (all 11 primary professions) |
| Loot pet and filter (m2b) | `.arpg lootpet on\|off\|info <char>`, `.arpg filter list\|add\|remove\|move <char> …`, `.arpg test pack <char> <entry> <n>` (spawn and kill a pack next to the character) |
| Monsters (m3) | `.arpg champion spawn <char> [affix]`, `.arpg superunique spawn <char> <id>`, `.arpg superunique list` |
| Paragon (m4) | `.arpg paragon info|set|xp|spend|respec|data <char>`; player: `.paragon data|info|spend|respec` |
| Dungeons (m5) | `.arpg dungeon info <char>` (party size, multipliers, instance level), `.arpg lfg probe <char> <dungeon>` |

A typical playtest of scaling: `.character level Me 85` → `.tele Northshire` → pull a wolf and check its
level and how long it takes to kill → `.arpg scale probe Me nearest` to see the numbers → tune
`Arpg.Scale.HealthMod` → `.arpg reload` → pull again. No rebuild, no restart.
