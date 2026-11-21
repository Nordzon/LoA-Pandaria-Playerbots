# Pandaria ARPG — Spec (canonical)

This file is the source of truth for **what** we build. The claude.ai Doc
"Pandaria ARPG – Single-Player MoP Mod Spec" was the drafting space; if they disagree, **this file wins**.
Change this file when a decision changes, and log it in `docs/PROGRESS.md`.

Status: agreed with the owner 2026-11-15 → 2026-11-21. Numbers are starting values, tuned in play.

## 1. Vision
A relaxed single-player (later: a few friends) Mists of Pandaria 5.4.8 server on the owner's Windows PC, with a
Diablo II-style layer on top. Pillars:
1. **Loot is the reward loop.** Drops above grey roll random affixes; named Uniques come from named monsters.
2. **The world stays relevant.** Creatures near you scale *up* to your level; quests scale.
3. **Danger roams.** Champion packs and Superuniques in every zone from level 1, with must-interrupt casts.
4. **No cap on progress.** Paragon levels after 90.
5. **Dungeons scale to the party** (1–5 members, bots count) with a difficulty knob.
6. **Everything tunable** in config/SQL; `.arpg reload` applies it without a restart.
Owner is a working dad: sessions are ~2 hours; zone clearing should feel fast and rewarding.

## 2. Platform
- Core: fork of [LoA-Pandaria-Playerbots](https://github.com/1LineAtaTime/LoA-Pandaria-Playerbots)
  (TrinityCore-family C++, MoP 5.4.8 build 18414, GPL-2.0, includes `modules/mod_playerbots`).
- Native Windows build: MSVC Build Tools 2022, CMake (4.3.2 installed; repo needs ≥ 3.27.2), MySQL 8.0 (5.7/8.0–8.1 supported),
  OpenSSL 3.0–3.1.1 full (not 3.2+), Boost ≥ 1.78 static (we use 1.87.0 = upstream Windows CI; README asks ≥ 1.85; code builds with BOOST_ASIO_NO_DEPRECATED).
- No server-side Lua engine (no Eluna). Server logic = C++; tuning = SQL + config; UI = client Lua addon; visuals/data = client MPQ patch.
- Bots: **off until milestone 7.**

## 3. Architecture — where each change lives
| Layer | Where | What | How changes apply |
| --- | --- | --- | --- |
| Server module (C++) | `modules/mod_arpg/` | loot rolls, affixes, scaling, Champions/Superuniques, Paragon, dungeon scaling, loot pet/filter, GM commands | rebuild worldserver |
| Data | `modules/mod_arpg/sql/`, `arpg.conf` | affix pools/tiers, Uniques, Superunique creatures + paths, scaling tables, Paragon curve | `.arpg reload` |
| Client addon (Lua) | `arpg/addons/ArpgUI/` (junctioned into the client) | Paragon panel, affix tooltip lines (pre-patch), alerts, loot filter panel, professions panel | `/reload` in game |
| Client patch (MPQ) | `arpg/client-patch/` → built MPQ | native affix lines + names, Unique names, new spells/icons, race/class unlock | rebuild MPQ, restart client |

Hooks (TrinityCore `ScriptMgr` style; confirm in this fork): item creation / `OnLootItem` (affixes),
`AllCreatureScript::OnCreatureAddWorld` / creature update (scaling, Champions), `PlayerScript::OnGiveXP` + `OnLogin` (Paragon),
group/map-enter hooks (dungeon scaling), `CommandScript` (`.arpg …`). A missing hook = tiny `// ARPG:` patch in `src/`, listed in CODEMAP.

Every `.arpg` debug command takes an explicit character name or GUID (works over SOAP, which has no selected target).

## 4. Features

### 4.1 Loot and affixes (m2)
- Quality sets affix count: green 1–2, blue 2–3, purple 3–5. Uniques are orange (Legendary). White stays plain.
- Affixes stored in the item instance's 5 random-property enchant slots (persist automatically). Blizzard reforging untouched.
- **Affixes roll when an item is created** (not when a loot window opens) — required by the loot pet.
- Values follow the **player's** level at drop time, not the item's.
- Tables: `arpg_affix` (name, kind, stat/effect, allowed slots, weight), `arpg_affix_tier` (T1–T5 ranges, unlock level).
- Per item: ≤ 3 prefixes, ≤ 3 suffixes, ≤ 5 total, no duplicates.
- Smart loot: main-stat suffix matches the player's spec `Arpg.Loot.SmartLootPct` (70%) of the time.
- Tiers unlock at player level: T1 1, T2 15, T3 35, T4 60, T5 85. Higher quality, Champion kills (+1 tier), magic find shift rolls up.
- Flat stat values are a share of **B** = stat budget of an epic item in that slot at the player's level (from the core's item stat tables).
- Bots: `Arpg.Loot.AffixBotItems = 0` (bot gear vanilla). In a group, bots roll only on items the player's class can't use.
- **Display:** stat affixes the client knows show natively (snapped to nearest existing enchant value via an extracted
  `SpellItemEnchantment` lookup). Other affixes work server-side from day one; ArpgUI adds their tooltip lines (server sends
  affixes via addon messages). With the client patch (m6, or m2 if the MPQ-loading spike succeeds), the pipeline generates
  one enchant row per (affix, value) from `arpg_affix_tier` → every affix is a native green tooltip line; items get D2-style names.

**Affix pool v1** (24 affixes; "Native" = shows without addon/patch):

| Affix | Kind | Effect | Slots | T1 → T5 | Weight | Before patch |
| --- | --- | --- | --- | --- | --- | --- |
| Deadly | Prefix | Critical strike | Any | 4–6% → 16–20% of B | 100 | Native |
| Swift | Prefix | Haste | Any | 4–6% → 16–20% of B | 100 | Native |
| Masterful | Prefix | Mastery | Any | 4–6% → 16–20% of B | 100 | Native |
| Precise | Prefix | Hit | Any | 4–6% → 16–20% of B | 60 | Native |
| Keen | Prefix | Expertise | Any | 4–6% → 16–20% of B | 60 | Native |
| Vampiric | Prefix | Life leech, % of damage dealt | Weapon, ring | 0.5% → 3% | 20 | Addon |
| Thirsty | Prefix | Health on hit, % of max health | Weapon, gloves | 0.2% → 1% | 40 | Addon |
| Slayer's | Prefix | Damage vs Champions and Superuniques | Weapon, amulet, ring | 2% → 12% | 40 | Addon |
| Blazing / Shocking / Freezing | Prefix | Chance on hit: Fire Nova / Chain Lightning / Frost Nova (existing spells, level-scaled) | Weapon | 2% → 6% | 20 | Addon |
| of the Titan | Suffix | Strength | Any | 6–9% → 18–22% of B | 100 | Native |
| of the Wind | Suffix | Agility | Any | 6–9% → 18–22% of B | 100 | Native |
| of the Sage | Suffix | Intellect | Any | 6–9% → 18–22% of B | 100 | Native |
| of the Whale | Suffix | Stamina | Any | 6–9% → 18–22% of B | 100 | Native |
| of the Spring | Suffix | Spirit | Cloth, leather, jewellery | 4–6% → 16–20% of B | 40 | Native |
| of Evasion | Suffix | Dodge | Armour, jewellery | 4–6% → 16–20% of B | 40 | Native |
| of Deflection | Suffix | Parry | Plate, weapon, jewellery | 4–6% → 16–20% of B | 40 | Native |
| of Fortitude | Suffix | Bonus armour | Armour, shield | 3% → 15% | 60 | Addon |
| of Thorns | Suffix | Reflects % of melee damage taken | Chest, shield, shoulders | 2% → 12% | 20 | Addon |
| of Resolve | Suffix | Shorter stun/fear/root | Helm, chest, amulet | 4% → 20% | 20 | Addon |
| of Speed | Suffix | Movement speed (total cap `Arpg.Affix.MoveSpeedCap` 30%) | Boots only | 2% → 10% | 40 | Addon |
| of Greed | Suffix | Gold find | Jewellery, helm, cloak | 5% → 40% | 40 | Addon |
| of Fortune | Suffix | Magic find | Jewellery, helm, cloak | 3% → 25% | 30 | Addon |
| of Wisdom | Suffix | XP and Paragon XP | Helm, ring | 2% → 10% | 30 | Addon |
| of Plenty | Suffix | Gathering/crafting skill-up chance | Gloves, cloak | 5% → 25% | 20 | Addon |

Tuning watch-list: the 30% move-speed cap; T5 flat stats (5 × ~20% of B on one item may be too strong).

### 4.2 World level scaling (m1)
- Per creature, **upward only**: level = max(normal level, player level + `Arpg.Scale.LevelOffset` (0)); stats recalculated
  from creature base stats × `Arpg.Scale.HealthMod` / `DamageMod`.
- Only creatures within `Arpg.Scale.Radius` (view range) of a real player scale. High-level zones stay high level.
- A creature already in combat with a bot keeps its level until reset/respawn. Scaled creatures revert on reset/respawn when no real player is near.
- Quests scale with the player (quest level −1). Quest reward items don't scale in v1.
- Kill switch `Arpg.Scale.Enabled`. Multiplayer later: scale to the highest-level real player nearby.

### 4.3 Champions and Superuniques (m3)
- **Champions:** on open-world spawn, `Arpg.Champion.Chance` (2%) → Champion + 2–4 minions, every zone from level 1.
  Player level +2, ~3× health, 1.5× damage, slightly larger, glow visual. 1–2 monster affixes: Extra Strong, Extra Fast,
  Fire Enchanted (death explosion), Cold Enchanted (slow), Lightning Enchanted, Stoneskin, Arcane Caster (interruptible nuke).
  Loot: ≥ 1 green, 25% blue, affix tier +1. Identified by skull marker + addon alert (names need the patch).
  Variant: Treasure Goblin (flees, drops crafting materials, rare). Optional in dungeons: `Arpg.Champion.InDungeons`.
- **Superuniques:** 1–2 per zone, **every zone**. Most data-driven (existing model + SQL ability kit, one generic C++ script);
  Pandaria's hand-scripted (MoP rare elites are good candidates). Player level +3, patrol path (`waypoint_data`),
  3–4 abilities incl. ≥ 1 must-interrupt cast. **5 minutes after one dies (`Arpg.Superunique.RespawnDelay` 300 s) a different
  Superunique from the zone's pool spawns elsewhere in the zone.** Zone + addon announce spawns.
  Loot: guaranteed blue + own Unique at 30% (likely too generous with 5-min respawns — tune).

### 4.4 Uniques and charms (m2/m6)
- `arpg_unique`: base item, fixed affixes, 1 random affix, optional special-effect enchant; drops only from its Superunique.
- Custom names need custom item entries: test server-sent DB2 `Item-sparse` hotfixes in m2; otherwise client patch.
- Charms: stats while in bags (scan on login/bag change → auras); same charm doesn't stack.

### 4.5 Paragon (m4)
- From 90, XP goes to `character_paragon`. XP for next level = `Base × Growth^level` (config).
  **Target: ≥ 5 levels per 2-hour session at the start**, low Growth so it slows gradually.
- 1 point/level in 4 tabs: Core (main stat, stamina), Offense (crit, haste, mastery), Defense (armour, dodge/parry, avoidance),
  Utility (move speed cap +10%, magic find cap +100%, gold find). Applied via stat modifiers on login/spend. Free respec at NPC (optional gold cost).
- ArpgUI draws the Paragon bar + panel (client has no XP bar at 90); `.paragon` commands as fallback.

### 4.6 Dungeon scaling (m5)
- On entering a 5-player instance: N = party members incl. bots (1–5) → multipliers, then `Arpg.Dungeon.Difficulty` (1.0).
  Creature level follows the party leader. Boss mechanics unchanged.

| Party size | Health | Damage |
| --- | --- | --- |
| 1 | 30% | 40% |
| 2 | 45% | 55% |
| 3 | 65% | 70% |
| 4 | 80% | 85% |
| 5 | 100% | 100% |

### 4.7 Loot pet and loot filter (m2b)
- Pet: non-combat follower (existing model), `.arpg lootpet on|off`, persists, re-summons after zoning/death. Queues the player's
  kills; out of combat (`Arpg.LootPet.WaitForCombatEnd`) walks to each corpse, digs, transfers loot (gold at once, quest items always,
  overflow mailed). Unreachable after `Arpg.LootPet.ReachTimeout` (3 s) → loot from where it is. Only items with loot rights;
  group-roll items stay in the roll. Builds on MoP's native area looting.
- Filter (server-side, applies to pet and manual looting): ordered rules per character (`character_arpg_lootfilter`) matching
  quality, "not usable by my class", item class/subclass, affix count, best tier, ilvl vs equipped, item ID lists.
  Actions: Pick up / Leave / Auto-sell (vendor price as gold) / Highlight (beam on corpse). Quest items, Uniques, gold always kept.
  ArpgUI panel + `.arpg filter` commands.

### 4.8 All professions (m1b)
- Primary-profession limit 2 → 11 (likely `MaxPrimaryTradeSkill`; else tiny `// ARPG:` patch). ArpgUI professions panel
  (spellbook only shows 2 primaries).

### 4.9 Character creation (m6)
- Any race with any class (e.g. Undead Monk): server SQL (playercreateinfo, spells, skills) + client `CharBaseInfo` patch.

### 4.10 Bots (m7, last)
- Enable playerbots (300 → 1000). Verify: scaling bot-lock, party-size scaling with bots, bot loot rule.
- Dungeon Finder hook: real player queuing → any dungeon at any level; bots matched to player level ±`Arpg.LFG.BotLevelWindow` (3).
- Optional: bot crowding (idle bots of the zone's level range move to the player's zone).

## 5. Client patch pipeline (m2 spike, m6 full)
- Git tracks only sources: `arpg/client-patch/dbc/*.csv` (rows we add/override, reserved high ID range), `icons/`, `arpg/addons/ArpgUI/`.
- Build: merge CSV rows into the extracted base DBC/DB2 → **copy the same files to the server's DBC dir** → pack MPQ
  (Ladik's MPQ Editor CLI) with a version in the name → deploy to the client `Data\` and delete the client `Cache\`.
- Server tells the addon the expected patch version on login; addon warns if outdated.
- Open question (m2 spike): exact MPQ names/folders the 5.4.8 client loads custom patches from.

## 6. Workflow
- OpenSpec-style changes in `openspec/changes/<id>/` (proposal, tasks, spec deltas with SHALL + WHEN/THEN); archive into `openspec/specs/` when done.
- Testing: unit tests (pure logic in `mod_arpg/src/Core/`), `.gmtest` over SOAP, `.botscenario` (m7), playtests — see `docs/TESTING.md`.
- Upstream sync every 2–4 weeks with `arpg/scripts/update-upstream.ps1` (stable branch, merge not rebase).

## 7. Later ideas (not scheduled)
- Multiplayer with friends: GitHub Releases + `update-client.ps1` for the patch/addon; Tailscale/ZeroTier instead of port forwarding;
  group rules (scale to highest-level real player, personal loot, a pet per player, shared Superunique spawns).
- Reforge-style "Mystic" NPC; Challenge-Mode-based keys; loot pet gathering herbs/ore; daily world affixes on Champion packs.
- Isolation from work setup (optional): VS Code "WoW" profile, MySQL on 3307 with manual start, no PATH changes, Git `includeIf` identity.
