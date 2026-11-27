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
- Bots: **on for play from m0 (300 random bots; owner decision 2026-11-22), off when testing ARPG features** (`write-confs.ps1 -Bots off`). Bot rules, 1000 bots and the LFG hook stay in milestone 7.
- **Condensed experience** (owner 2026-11-22; reasoning in `docs/BALANCE.md`), set by `write-confs.ps1`: XP **1.5×** (kill/quest/explore), gathering XP 3×, **+15% movement** for all players incl. mounts (`Rate.MoveSpeed` 1.15), drops greens/blues 2×, epics 1.5×, gold 1.5×, skill-ups give 2 points, skill-up chance yellow 100% / green 50%, all 11 professions.

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
- **Typed blank bases** (owner decision 2026-11-22, BALANCE §8c, with the Item-sparse patch in m6): affixed drops become stat-less copies
  that keep only the armour type's main stat + Stamina; all other power comes from up to 5 affix lines (Uniques: 4 + power line).
  **Use (owner 2026-11-27, "all of these", change m6-blank-bases):** `Arpg.Blank.DropChance` (10%) of gear drops come as their
  blank, crafted gear comes out as its blank (crafting-wheel roll), the companion sells blanks for essences; a dropped or bought
  blank rolls the most affixes for its quality. 1,495 blanks (slot x type x quality x 10-level bracket x main stat).
- **Corpse loot preview** (owner 2026-11-27): affix values are fixed when corpse loot is planned; the loot window tooltip shows
  the lines (ArpgUI, `.lootinfo data`), identical to the picked-up item.
- **Level-appropriate loot with a zone ceiling** (owner 2026-11-22, to build in m2): when a drop's required level is more than
  `Arpg.Loot.SubstituteGap` (5) below the loot level, it is swapped - before affixes roll - for an item of the same slot,
  quality and armour type whose required level is closest to the loot level. **Loot level = min(player level, zone max level
  + `Arpg.Loot.ZoneCeilingMargin` (15, = the scaling cap))**, and affix values follow the same loot level. So a level-60 in Elwynn (zone 3-10)
  gets level-25 loot: farming low zones stops paying, which prompts moving on. Quest reward items use the quest zone's
  ceiling the same way. Catch-up for players who are far behind: the scaled dungeons (m5, level selector). Dungeons and
  raids use their chosen/scaled level instead of the zone ceiling. Zone level ranges come from a data table
  (`arpg_zone_level`, filled from the client's AreaTable/quest levels), editable in SQL.
- **World scaling cap per zone** (owner 2026-11-22): open-world monsters scale to the player's level but at most
  `Arpg.Scale.ZoneCapAbove` (15) levels above the zone's normal max level (`arpg_zone_level`). **Vale of Eternal Blossoms
  (zone 5840), the endgame hub, has no cap** (`Arpg.Scale.UncappedZones` = 5840, comma list). Upward-only rule unchanged.
- **Paragon grind** (owner question 2026-11-22, plan for m4/m5): monster levels stay at 90 (above it the level-based
  formulas - hit chance, XP, stats - break). Instead (a) **open world (Vale): a world tier** that grows with the player's
  Paragon level: +`Arpg.Paragon.WorldTierPct` (2)% monster health and damage per Paragon level up to a cap, more Paragon XP
  and more Champions/Superuniques - the endless solo farm; (b) **raids and scaled dungeons: the structured ladder** (tier
  multipliers, Paragon-gated Ascended affix tiers T6-T8, best loot; BALANCE §6/§12), with per-boss controls.
- **Paragon and the open world** (proposal, m4): quests at 90 give Paragon XP; Paragon raises the open-world loot tier the way
  it does in raids (BALANCE §12).
- Tables: `arpg_affix` (name, kind, stat/effect, allowed slots, weight), `arpg_affix_tier` (T1–T5 ranges, unlock level).
- Per item: ≤ 3 prefixes, ≤ 3 suffixes, ≤ 5 total, no duplicates.
- Smart loot: main-stat suffix matches the player's spec `Arpg.Loot.SmartLootPct` (70%) of the time.
- Tiers unlock at player level: T1 1, T2 15, T3 35, T4 60, T5 85. Higher quality, Champion kills (+1 tier), magic find shift rolls up.
- Flat stat values are a share of **B** = stat budget of an epic item in that slot at the player's level (from the core's item stat tables).
  **Calibrated to real items** (2026-11-22, BALANCE §16): a stat affix is a share of the matching stat line on a real blue of the
  player's level and slot (blue main line = 43% of B, best secondary 34%, stamina 50%). T1 = 30–45% of that line, T5 = 85–100%
  (a T5 roll equals a full blue stat line); Ascended T6–T8 go above it. Tiers in between are linear.
- Sell value: `template price + affixCount × playerLevel × Arpg.Loot.SellCopperPerAffixLevel` (200) → ~350–500 g/h of normal play at 90 (needs a small `// ARPG:` hook in the vendor sell handler).
- Bots: `Arpg.Loot.AffixBotItems = 0` (bot gear vanilla). In a group, bots never roll Need (owner 2026-11-27; `AiPlayerbot.LootNeedRollLevel` 1 turns their Need into Greed), so the player wins any Need roll.
- **Display:** all affix effects are applied server-side at their exact value. A stat affix also shows natively when the client
  has a fixed-amount `SpellItemEnchantment` within ±5% of the roll (measured: client rows only reach ~46 main stat, so mostly low levels). Other affixes work server-side from day one; ArpgUI adds their tooltip lines (server sends
  affixes via addon messages). With the client patch (m6, or m2 if the MPQ-loading spike succeeds), the pipeline generates
  one enchant row per (affix, value) from `arpg_affix_tier` → every affix is a native green tooltip line; items get D2-style names.

**Affix pool v1** (24 affixes; "Native" = shows without addon/patch):

| Affix | Kind | Effect | Slots | T1 → T5 | Weight | Before patch |
| --- | --- | --- | --- | --- | --- | --- |
| Deadly | Prefix | Critical strike | Any | 30–45% → 85–100% of a blue's secondary line | 100 | Native |
| Swift | Prefix | Haste | Any | 30–45% → 85–100% of a blue's secondary line | 100 | Native |
| Masterful | Prefix | Mastery | Any | 30–45% → 85–100% of a blue's secondary line | 100 | Native |
| Precise | Prefix | Hit | Any | 30–45% → 85–100% of a blue's secondary line | 60 | Native |
| Keen | Prefix | Expertise | Any | 30–45% → 85–100% of a blue's secondary line | 60 | Native |
| Vampiric | Prefix | Life leech, % of damage dealt | Weapon, ring | 0.5% → 3% | 20 | Addon |
| Thirsty | Prefix | Health on hit, % of max health | Weapon, gloves | 0.2% → 1% | 40 | Addon |
| Slayer's | Prefix | Damage vs Champions and Superuniques | Weapon, amulet, ring | 2% → 12% | 40 | Addon |
| Blazing / Shocking / Freezing | Prefix | Chance on hit: Fire Nova / Chain Lightning / Frost Nova (existing spells, level-scaled) | Weapon | 2% → 6% | 20 | Addon |
| of the Titan | Suffix | Strength | Any | 30–45% → 85–100% of a blue's line | 100 | Native |
| of the Wind | Suffix | Agility | Any | 30–45% → 85–100% of a blue's line | 100 | Native |
| of the Sage | Suffix | Intellect | Any | 30–45% → 85–100% of a blue's line | 100 | Native |
| of the Whale | Suffix | Stamina | Any | 30–45% → 85–100% of a blue's line | 100 | Native |
| of the Spring | Suffix | Spirit | Cloth, leather, jewellery | 30–45% → 85–100% of a blue's secondary line | 40 | Native |
| of Evasion | Suffix | Dodge | Armour, jewellery | 30–45% → 85–100% of a blue's secondary line | 40 | Native |
| of Deflection | Suffix | Parry | Plate, weapon, jewellery | 30–45% → 85–100% of a blue's secondary line | 40 | Native |
| of Fortitude | Suffix | Bonus armour | Armour, shield | 3% → 15% | 60 | Addon |
| of Thorns | Suffix | Reflects % of melee damage taken | Chest, shield, shoulders | 2% → 12% | 20 | Addon |
| of Resolve | Suffix | Shorter stun/fear/root | Helm, chest, amulet | 4% → 20% | 20 | Addon |
| of Speed | Suffix | Movement speed (total cap `Arpg.Affix.MoveSpeedCap` 30%) | Boots only | 2% → 10% | 40 | Addon |
| of Greed | Suffix | Gold find | Jewellery, helm, cloak | 5% → 40% | 40 | Addon |
| of Fortune | Suffix | Magic find | Jewellery, helm, cloak | 3% → 25% | 30 | Addon |
| of Wisdom | Suffix | XP and Paragon XP | Helm, ring | 2% → 10% | 30 | Addon |
| of Plenty | Suffix | Gathering/crafting skill-up chance | Gloves, cloak | 5% → 25% | 20 | Addon |

Tuning watch-list: the 30% move-speed cap; T5 flat stats (a god-roll epic: base stats + one full blue main line + up to four blue secondary lines; see BALANCE §16).

**Affix pool v2** (proposed 2026-11-22, Diablo II-inspired; joins v1 in m2, numbers tuned in playtests). D2 names where they fit,
mapped to what MoP can do. "Spell" effects reuse existing client spells (level-scaled) so they need no client patch to work.

| Affix | Kind | Effect (D2 origin) | Slots | T1 → T5 | Weight | Before patch |
| --- | --- | --- | --- | --- | --- | --- |
| Cruel | Prefix | +% weapon damage (Enhanced Damage) | Weapon | 5% → 25% | 60 | Addon |
| Socketed | Prefix | +1 prismatic gem socket (Socketed; uses the core's prismatic socket slot) | Weapon, chest, helm, legs, belt | — (1 socket) | 15 | Native |
| Venomous | Prefix | Chance on hit: poison DoT (Poison Damage) | Weapon | 3% → 8% | 20 | Addon |
| Siphoning | Prefix | Mana/resource on hit, % of max (Mana Leech) | Weapon, ring | 0.2% → 1% | 25 | Addon |
| Fanatic's | Prefix | Faster cooldown recovery (Increased Attack Speed / skill feel) | Gloves, amulet | 2% → 8% | 15 | Addon |
| Arcing | Prefix | Chance when struck: Chain Lightning on attacker (Charged Bolt "when struck") | Chest, shield | 3% → 8% | 15 | Addon |
| Lucky | Prefix | Magic find (D2 prefix form of "of Fortune"; never both on one item) | Jewellery, boots | 3% → 20% | 20 | Addon |
| Champion's | Prefix | Damage taken from Champions/Superuniques reduced (Damage Reduced) | Armour, shield | 2% → 10% | 30 | Addon |
| of the Bat | Suffix | Mana/resource leech, % of damage dealt (Mana Stolen per Hit) | Weapon, amulet | 0.5% → 2% | 20 | Addon |
| of Crushing | Suffix | Chance on hit: extra damage = % of target's current health; not on bosses/Superuniques (Crushing Blow) | Weapon | 2% → 6% (cap 10% of max HP per hit) | 15 | Addon |
| of Gore | Suffix | Chance on hit: bleed DoT (Open Wounds) | Weapon, gloves | 3% → 10% | 20 | Addon |
| of Regeneration | Suffix | Health regen per 5 s, % of max (Replenish Life) | Armour, jewellery | 0.2% → 1% | 40 | Addon |
| of Freedom | Suffix | Shorter slows/snares (Cannot Be Frozen, partial) | Boots, legs | 5% → 25% | 20 | Addon |
| of the Phoenix | Suffix | Below 35% health: heal for % of max, once per 60 s (Phoenix "when struck") | Chest, amulet | 8% → 20% | 10 | Addon |
| of Warding | Suffix | Entering combat: absorb shield, % of max health (Shield/"Bone Armor" feel) | Shield, chest, cloak | 3% → 12% | 15 | Addon |
| of Light | Suffix | +loot pet range and Highlight beam radius (Light Radius) | Helm, ring | 10% → 40% | 20 | Addon |
| of the Magus | Suffix | +% spell damage and healing (+Skills feel for casters) | Weapon, off-hand, amulet | 2% → 10% | 30 | Addon |
| of Alacrity | Suffix | +% cast speed only (Faster Cast Rate) | Cloth, jewellery | 2% → 8% | 25 | Addon |

Rare names (D2 style, m6): items with 3+ affixes get a random two-word name from `arpg_rare_name` (prefix word + suffix word,
e.g. "Doom Grasp", "Storm Mark"); items with 1–2 affixes use "<Prefix> <Item> <Suffix>". Gold/yellow name colour only with the client patch.
Rules carried over: ≤ 3 prefixes, ≤ 3 suffixes, no duplicates; Lucky and of Fortune exclusive; every "chance on hit/when struck"
affix has a per-affix internal cooldown (`Arpg.Affix.ProcIcdMs`, 1000) so fast weapons don't spam procs.

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
  *Implemented 2026-11-22 (step 1):* the roll happens once per creature life, the first time a real player's scaling tick sees
  it (open world, attackable, not rare/boss); the Champion's level/multipliers are pinned (the scaling tick leaves it alone);
  minions are copies of the creature at its level following it; soft affixes (Extra Strong/Fast, Stoneskin) work, the
  Enchanted/Arcane ones are picked but their spells come with task 3.2; no glow aura yet (`Arpg.Champion.VisualAura` 0 until
  one is chosen in game).
- **Level bands** (owner 2026-11-22, `arpg_monster_band`): Champion and Superunique level offset, health, damage, affix count and ability set grow with the player's level so early characters can kill them (1-14: Champion +0 / 2.0x / 1.15x / 1 soft affix; Superunique +1 / 3.0x / 1.2x, no must-interrupt; full numbers from 35/60). Affix damage is a % of the player's max health. A Champion kill restores 20% health and power (`Arpg.Champion.HealthOrbPct`), a Superunique 40%. Table: `docs/BALANCE.md` §14.
- **Superuniques:** 1–2 per zone, **every zone**. Most data-driven (existing model + SQL ability kit, one generic C++ script);
  Pandaria's hand-scripted (MoP rare elites are good candidates). Player level +3, patrol path (`waypoint_data`),
  3–4 abilities incl. ≥ 1 must-interrupt cast. **5 minutes after one dies (`Arpg.Superunique.RespawnDelay` 300 s) a different
  Superunique from the zone's pool spawns elsewhere in the zone.** Zone + addon announce spawns.
  **Tags (owner 2026-11-22):** Champions and Superuniques are their own creature types (copies made by
  `sql/tools/gen_monster_tags.py`): "Champion <name>" with the Rare portrait (silver dragon), "Unique <name>" with the Rare
  Elite portrait; kill credit points at the original. A Champion switches to its copy in place and back on respawn.
  **Roll at the kill (owner 2026-11-22):** loot is rolled when the corpse/chest is filled; the loot window shows the final name
  ("X of the Whale", "X - <Unique>"); picking it up applies exactly that roll (values roll at pickup, they aren't visible before).
  Splash/chain procs only hit hostile, unfriendly or fighting units (neutral bystanders are left alone).
  **Targeting (owner 2026-11-22):** abilities and caster affixes go at whoever holds the monster's aggro: the player, a bot tank
  in the player's party, a pet. The full kit is used on the tank-at-the-time, damage always as a % of *that* target's max
  health. They only act while a real player is near (~100 yd).
  Loot: guaranteed blue + own Unique at **10%** with bad-luck protection (+5% per kill without it, reset on drop; `Arpg.Superunique.UniqueChance`, `.UniqueBadLuckStep`). **One counter shared by all Superuniques** (owner 2026-11-27, `.SharedBadLuck` 1): any Superunique kill without a Unique raises it, so a Unique comes within 19 Superunique kills whichever ones they are. Boss ladders stay per boss.

### 4.3b Potions (m3b, owner 2026-11-22)
- Existing potion items (Minor .. Major Healing / Mana, Minor/Major Rejuvenation, Alchemist's Rejuvenation) heal / restore a
  **% of max health / resource**: healing 10-20% by tier; mana potions restore mana %, **energy and focus at half that %,
  rage and runic power at that %** (every class gets something); rejuvenation does both.
- **Drops by monster kind** (`arpg_potion_drop`): normal 5% tier 1, elite 15% tier 2, Champion 50% tier 3, Superunique
  always tier 5, boss always tier 6; at most one per kill; mix healing 50-60 / mana 30 / rejuvenation 10-20.
- **Stacks of 999, usable at any level, one 10 s shared cooldown** (owner: no needless inventory management). Done through
  `item_template` + Item-sparse hotfixes (`hotfix_data`, type 2442913102): the client re-reads these items from the server,
  no client patch. *Finding:* the same route can deliver custom items (typed blank bases, catalysts, essences) before m6.

### 4.4 Uniques and charms (m2/m6)
- `arpg_unique`: base item, fixed affixes, **1–2 random suffixes rolled at drop**, and a **signature mechanic** (server-side spell/aura script that changes how a class plays, e.g. Purifying Brew heals, finishers refund combo points). Unique tiers I/II/III (D2 normal/exceptional/elite) by source: Superuniques and raid tiers 1–2 → I; raid tiers 3–5 → II; 6–7 → III (more random suffixes, higher fixed tier). Drops from its Superunique (10% +5%/miss) or dungeon/raid boss (**pity ladder 20% +20% per run, guaranteed by run 5**, `character_arpg_pity`). Every boss drops ≥1 item with 3+ affixes; every 3rd kill without a 5-affix item guarantees one. Examples in `docs/BALANCE.md` §8, §13.
- Custom names need custom item entries: test server-sent DB2 `Item-sparse` hotfixes in m2; otherwise client patch.
- **Unique powers** (owner 2026-11-22, numbers in `docs/BALANCE.md` §8b): `arpg_unique_power` (id, name, trigger: chance-on-hit /
  chance-on-heal / when-struck / every-Nth-hit, kind: damage / heal-absorb / flat, school, radius, targets, spell for the log)
  + `arpg_unique_power_tier` (power, tier, chance or N min-max, magnitude min-max, internal cooldown). Scaling stat **Power =
  max(attack power, spell power)**; heal/absorb powers scale on max health. Each Unique item instance stores its power tier
  and both rolled numbers (`item_instance_arpg_unique`). Tooltip line from client patch rows per (power, tier, chance step 1%,
  magnitude step 5%), "- Perfect" when both numbers roll their maximum. Runs on the effect-affix machinery (Loot/AffixEffects).
- Charms: stats while in bags (scan on login/bag change → auras); same charm doesn't stack.

### 4.5 Paragon (m4)
- From 90, XP goes to `character_paragon`. XP for next level = `Base × Growth^level` (config).
  **Target: ≥ 5 levels per 2-hour session at the start**, low Growth so it slows gradually.
- 1 point/level in 4 tabs: Core (strength, agility, intellect or stamina - **any of them, no main-stat guess**, owner 2026-11-22), Offense (crit, haste, mastery), Defense (armour, dodge/parry, avoidance),
  Utility (move speed cap +10%, magic find cap +100%, gold find). Applied via stat modifiers on login/spend. Free respec at NPC (optional gold cost).
- ArpgUI draws the Paragon bar + panel (client has no XP bar at 90); `.paragon` commands as fallback.
- **Decided (owner 2026-11-22):** Diablo 3 rotation (each level's point goes to the next tab, Core -> Offense -> Defense ->
  Utility; the player picks the stat inside the tab), **free respec**, quests at 90 give **full** Paragon XP.

### 4.6 Dungeon scaling (m5)
- On entering a 5-player instance: N = party members incl. bots (1–5) → multipliers, then `Arpg.Dungeon.Difficulty` (1.0).
  Creature level follows the party leader. Boss mechanics unchanged. Built on the core's AutoBalance (instance scaling, already ported).
- Solo roles: `Arpg.Dungeon.SoloNonTankDamage` (0.75, extra creature-damage multiplier when the lone player isn't a tank spec) and `Arpg.Dungeon.SoloTankDamageDealt` (1.25), so both a Brewmaster and a Rogue can solo (analysis: `docs/BALANCE.md` §5).
- **Healers solo too:** `Arpg.Dungeon.SoloHealerDamageDealt` (1.6) + healing-to-damage affixes (Radiant, Zealous, of Atonement, of Martyrdom) and healer Uniques (`docs/BALANCE.md` §10).
- **Dungeon level selector:** the player picks the level a dungeon runs at (dungeon min level … own level; companion gossip or `.arpg dungeon level <char> <n>`, remembered per dungeon); creatures and loot (B, affix tiers) follow the chosen level.
- **Paragon raises raid loot tier:** Ascended affix tiers T6/T7/T8 (110/120/130% of T5) need raid tier 3/5/7 **and** Paragon 25/60/110 (`docs/BALANCE.md` §12).
- **Raids are the endgame tier** (owner 2026-11-22): every raid scales to the leader's level, then party size (solo: 10% health, 25% damage of 10-man normal), then a per-raid tier multiplier `1 + 0.15 × tier` from `arpg_raid_tier` (tiers 1–7: Classic → Siege of Orgrimmar, with recommended Paragon). Paragon power is how you climb tiers; loot affixes roll for your level so old raids stay useful. Details: `docs/BALANCE.md` §6.

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
- **Companion roles** (owner request 2026-11-22): the loot pet is a multi-role companion. One creature template carries native
  NPC flags, so right-click opens the game's own windows - no new UI: **vendor** (sell; buys reagents from `arpg_companion_vendor`),
  **repair**, **bank**, **mailbox**, **reforge**, **transmogrification** (appearance changer) and **void storage**. Each role is a
  config switch (`Arpg.Companion.Vendor`, `.Repair`, `.Bank`, `.Mail`, `.Reforge`, `.Transmog`, `.VoidStorage`; all 1).
  A gossip menu offers **appearance**: pick the companion's model from `arpg_companion_model` (unlocked list; `.arpg companion model`).
  Roles never work in combat or in instances' boss areas (`Arpg.Companion.InCombat` 0). Later roles (not v1): gathering herbs/ore,
  stable master, a "stash" shared between characters.
- *Implemented 2026-11-23 (m2b):* filter rules in text form (`quality<=0 usable=0 affixes>=3 tier>=5 vs=0 class=4 ids=1,2 ->
  sell|leave|pickup|highlight`), applied when a corpse/chest's loot is rolled (sell = gold at once; v1 leave = out of the loot);
  loot companion = creature 1000001 (follow, queue, walk to corpse, 3 s reach timeout, core loot store path, mail on full bags,
  9 looks); roles via gossip menu (Sell and repair, Bank, Mailbox open the native windows; reforge/transmog/void flags set,
  in-game check pending); ArpgUI L button + filter panel over `.lootpet` / `.lootfilter` player commands.

### 4.8 All professions (m1b)
- Primary-profession limit 2 → 11: `MaxPrimaryTradeSkill = 11` (config, no core patch; set by `write-confs.ps1`). ArpgUI professions panel
  (spellbook only shows 2 primaries).
- **The crafting wheel** (owner 2026-11-22): quest and farm → bags full of materials and drops → back in town: craft, sell, filter,
  keep the good rolls → out again. Crafting is a real source of chase items, not a side activity:
  - **Crafted gear rolls affixes** (already live: crafting is an item source; values follow the crafter's loot level).
  - **Skill matters:** recipe colour sets the roll: orange/yellow recipes +1 affix tier, and a crafter at the profession's max skill
    rolls one extra affix (within the quality's limits). Grey recipes roll at T1 (no exploit by spamming trivial recipes).
    *Implemented 2026-11-22:* colour thresholds as the client (yellow = SkillLineAbility min, green = (min+max)/2, grey = max);
    "max skill" = `Arpg.Craft.MaxSkill` (600); the extra affix never goes past the quality's max count (a green at max skill
    always rolls 2). Unique ladder per character and profession in `character_arpg_craft_pity` (`Core/Craft`, `Loot/Crafting`).
  - **Crafted Uniques:** each craft of a green/blue/epic item has a `Arpg.Craft.UniqueChance` (2%) chance to become a **Unique of
    the same slot and armour type** (BALANCE §8b powers, tier from the loot level), with a per-character, per-profession pity
    (+1% per craft without one, `character_arpg_pity`). Orange/yellow recipes only.
  - **Catalysts:** Champion and Superunique kills drop **ARPG catalysts** (one item type per tier band); using one in a craft
    raises the roll by +1 tier and doubles the Unique chance for that craft. Champions finally feed the crafting loop.
  - **Salvage:** unwanted affixed items break down (at a vendor or the companion) into **essences** per affix count; essences buy
    catalysts and later the Mystic's rerolls (gold and material sink, BALANCE §7).
  - Loot filter and companion (m2b) do the sorting: auto-sell below a rarity, keep by affix/tier rules.
  - Tests: forced craft of a known recipe for the test character (`.arpg test craft <char> <spellId> [catalyst]`) → affix
    count/tier rules, Unique pity forced to fire, catalyst +1 tier; no rate tests (TESTING §3c).

- *Implemented 2026-11-26 (change `m6-custom-items`):* custom items 700001-700003 (Lesser/Greater/Grand Catalyst), 700010
  (Salvaged Essence) via `item_template` + `hotfix_data` (no client patch). Champion leaders (25%) and Superuniques (100%) drop a
  catalyst of their band (by the monster level's affix tier); a craft that rolls affixes uses up the lowest fitting one (+1 tier,
  x2 Unique chance). Companion dialogue: Salvage (affixed greens/blues in the bags, confirm; 1 essence per affix, Uniques kept
  unless `Arpg.Salvage.IncludeUniques`) and Buy catalysts (10/30/90 essences). All numbers in `Arpg.Catalyst.*` / `Arpg.Salvage.*`.

- *Mount shop (owner 2026-11-26, change `m2b-mount-shop`):* the companion's dialogue sells the 64 reputation mounts for gold
  (`Arpg.MountShop.Price` 5000, per-row override in `arpg_mount_shop`), no reputation needed. First of the planned gold sinks.

### 4.9 Character creation (m6)
- Any race with any class (e.g. Undead Monk): server SQL (playercreateinfo, spells, skills) + client `CharBaseInfo` patch.
- *Implemented 2026-11-23:* `sql/tools/gen_any_race_class.py` adds the 43 missing pairs (165 total): start at the race's own start
  (Death Knights: shared DK start), class action bar and start items from a reference race of the same faction family, level stats =
  reference race's class stats + (race warrior - reference warrior) per level, race masks ORed into class skill/spell rows.
  Client: `build-client-patch.py` packs `CharBaseInfo.dbc` (all 165 pairs) into the patch MPQ and copies it to the server DBC folder.
  Check: `arpg/scripts/test-char-create.ps1` (headless create of an Undead Monk, DB check, erase).
- **Any weapon** (owner 2026-11-26, change `m6-any-weapon`): any class can equip any weapon type and a shield. Every real
  player gets every weapon proficiency + Shield (spell for the client, skill at the level max for the server's equip check)
  at login and on level change. `Arpg.AnyWeapon.Enabled` (1), `.Bots` (0: bots keep their class's weapons), `.DualWield`
  (0). Not covered: items limited to a class by the item itself (`AllowableClass`) and class abilities tied to a weapon.

### 4.10 Bots (m7, last)
- *Implemented 2026-11-26:* 500 random bots (write-confs default). Dungeon Finder any level = upstream's switches, set by
  write-confs: `LFG.AnyLevel.Enable/Random/Scenarios` = 1, `AiPlayerbot.LfgAnyLevel.Enable` = 1, `.LevelWindow` = 3 (no code of
  ours; out-of-range dungeons scale through AutoBalance + our m5 hook). Raid Finder stays off (raids are entered solo, m5).
- *Bot crowding (2026-11-26):* upstream's dynamic level brackets (`AiPlayerbot.LevelBrackets.*` via write-confs, real-player
  weight 10, factions synced) put many more random bots at the real player's level; they quest and grind in that level's zones.
- *Bots understand affixes (2026-11-26, v1):* playerbots `StatsWeightCalculator` got one hook (`ExtraItemStats`, // ARPG:);
  mod_arpg adds the affix stats of the item a party bot (a bot in a real player's group) owns, weighted by the bot's spec, so
  it equips the better roll. `Arpg.Bots.AffixScoring` (1). When anyone joins a group that has a real player, each bot's
  equipped greens-epics without affixes roll once, `Arpg.Bots.PartyAffixTierOffset` (-1) tiers from the player's
  (`Arpg.Bots.PartyGear` 1). Not yet: corpse loot (Need rolls) scored by planned affixes.
- Enable playerbots (300 → 1000). Verify: scaling bot-lock, party-size scaling with bots, bot loot rule.
- Dungeon Finder hook: real player queuing → any dungeon at any level; bots matched to player level ±`Arpg.LFG.BotLevelWindow` (3).
- Optional: bot crowding (idle bots of the zone's level range move to the player's zone).
- **Bots understand affixes** (owner 2026-11-22): playerbots scores items from the template only
  (`mod_playerbots/src/Mgr/Item/StatsCollector::CollectItemStats(ItemTemplate const*)`, used by `StatsWeightCalculator`
  and `BotFactory::InitEquipment`). Add an instance-aware score = template + mod_arpg `ItemStatLines(itemGuid)` with the same
  spec weights, so bots equip better rolls and roll Need sensibly. Bots in the real player's party (and companions) get affixed
  auto-gear (`Arpg.Bots.PartyAffixTierOffset`, default -1 tier); random world bots stay vanilla (DB and CPU stay lean).
- **Living auction house** (owner 2026-11-22): bots only *visit* auctioneers today (town errands); the core's AuctionHouseBot is
  off and lists/buys by template only (no affixes - no player pickup). Plan: (1) AHBot seller on for profession materials;
  (2) a share of AHBot gear listings rolls affixes (moderate tiers), priced by the sell formula × `Arpg.AH.AffixMarkup`;
  (3) AHBot buyer values affixes (count × tier), so a good roll sells better there than at a vendor.
- Idea (owner to decide): crafter at max skill → +1 affix tier on crafted items (crafting already rolls affixes).

### 4.11 Playing with friends (m8, owner 2026-11-27)
- Mates connect over **Tailscale** (private network; no port forwarding). Servers bind per `network.local.ps1` (`write-confs
  -BindIP`); the realm address is the owner's Tailscale address (`set-realm-address.ps1`), loopback for the owner's own client.
- **Sign-up page** (`arpg/signup/signup.py`, port 8090): name + password twice -> a normal player account (never GM, never over
  an existing account); invite code, account cap, rate limit; settings in `signup.local.json`. Test: `test-signup.ps1`.
- **Client kit** (`make-client-kit.ps1`): patch MPQ + ArpgUI + installer (realmlist); mates bring their own 5.4.8 client.
- **Backups:** auth + characters before every rebuild (`backup-db.ps1`, last 30); rebuild-and-test won't restart while a real
  player is online. Guide: `docs/FRIENDS.md`.
- **Rules with several real players** (owner 2026-11-27, "as recommended"): a monster scales to the highest level among the real
  players near it; the Vale world tier follows `Arpg.Paragon.WorldTierMode` (highest, change any time: average | lowest).
  **Personal loot** (`Arpg.Party.PersonalLoot`): the killer keeps the corpse (round robin; groups with 2+ real players switch to
  it), every other real player of the group within `PersonalLootRange` (100 yd) gets their own roll of the loot table + their own
  Champion/Superunique/boss extras and Unique counters, straight into the bags (mailed when full). One companion per player.
  Bots with friends: `$FriendsBotCount` (300) in network.local.ps1.

## 5. Client patch pipeline (m2 spike, m6 full)
- Git tracks only sources: `arpg/client-patch/dbc/*.csv` (rows we add/override, reserved high ID range), `icons/`, `arpg/addons/ArpgUI/`.
- Build: merge CSV rows into the extracted base DBC/DB2 → **copy the same files to the server's DBC dir** → pack MPQ
  (Ladik's MPQ Editor CLI) with a version in the name → deploy to the client `Data\` and delete the client `Cache\`.
- Server tells the addon the expected patch version on login; addon warns if outdated.
- Open question (m2 spike): exact MPQ names/folders the 5.4.8 client loads custom patches from.
  **Spike result (2026-11-22): WORKS** with the unlocked exe **`Wow-64-all.exe`** (wowmodding.net file 367 "WoW MoP 5.4.8
  cracked exe 64-bit", "all" variant; audited: same size/sections/imports as the original, 18 bytes changed in 6 places,
  no new code, Defender clean). It keeps the classic login and loads **plain** files from **`Data\enUS\wow-update-enUS-20000.MPQ`**
  (no patch wrapper, no metadata; numbers above Blizzard's are fine). In game: a changed Blizzard row, a new row 5300 and a
  new row **60000** all show on tooltips → our enchant ids start at 60000. The stock-ish `Wow-64.exe` rejects any
  non-Blizzard archive data (9 rounds, `arpg/client-patch/TEST-SCENARIOS.md`). The desktop shortcut starts `Wow-64-all.exe`.
  **Rule:** rows we add to a client DBC go into the server's DBC dir too (server and client must agree).
  Display: the client never shows enchant slot 8 (MoP reforge) unless the item has a random-suffix id. **Probe 2026-11-22: an
  item with a (custom) random-suffix id shows all five slots 8-12 with our rows' exact values and gets the suffix in its name**
  ("Gloomshroud Armor of the ARPG Probe"). So affixed items get 5 lines + a D2-style name: suffix rows 30000 + affix id in
  ItemRandomSuffix (client patch AND server copy - chat links validate the id), name from the highest-tier affix.
  Tools: `arpg/client-patch/build-probe-patch.py --plain`, StormLib from source (`arpg/tools/bin/StormLib`),
  `.arpg test probeitem <char> <item> [enchant]` (slot 9).

## 6. Workflow
- OpenSpec-style changes in `openspec/changes/<id>/` (proposal, tasks, spec deltas with SHALL + WHEN/THEN); archive into `openspec/specs/` when done.
- Testing: unit tests (pure logic in `mod_arpg/src/Core/`), `.gmtest` over SOAP, `.botscenario` (m7), playtests — see `docs/TESTING.md`.
- Upstream sync every 2–4 weeks with `arpg/scripts/update-upstream.ps1` (stable branch, merge not rebase).

## 7. Later ideas (not scheduled)
- Multiplayer with friends: GitHub Releases + `update-client.ps1` for the patch/addon; Tailscale/ZeroTier instead of port forwarding;
  group rules (scale to highest-level real player, personal loot, a pet per player, shared Superunique spawns).
- Reforge-style "Mystic" NPC; Challenge-Mode-based keys; loot pet gathering herbs/ore; daily world affixes on Champion packs.
- Isolation from work setup (optional): VS Code "WoW" profile, MySQL on 3307 with manual start, no PATH changes, Git `includeIf` identity.
