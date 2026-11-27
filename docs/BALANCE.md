# Balance notes (living document)

First-pass analysis, 2026-11-22. Numbers here are **starting values to tune in playtests**, not promises.
Canonical decisions go into `docs/SPEC.md`; this file explains the reasoning. MoP 5.4 combat facts marked *(verify)* should be
checked in game before we tune around them.

## 1. Design goals (owner, 2026-11-22)
- Condensed: faster levelling, faster gathering/crafting, more drops, little grinding, gold is not a worry.
- Chase loot: "god rolls" exist for every spec, and Uniques add new mechanics (with random suffixes so copies differ).
- Any spec, healers included, can solo a 5-man; raids are the endgame tier, solo-able once your Paragon is high enough; farm them and climb.
- Superuniques every ~5 minutes. Everyone moves faster.

## 2. Global rates (set by `write-confs.ps1`, live from the first start)
| Knob | Value | Why |
| --- | --- | --- |
| `Rate.XP.Kill/Quest/Explore` | 1.5 | Faster, not trivial. Zone quests scale with you (quest level -1), so quest XP stays full value |
| `Rate.XP.Gather` | 3.0 | Gathering levels you too; makes professions feel worth doing |
| `Rate.MoveSpeed` | 1.15 | +15% for every class, incl. mounts; players only |
| `Rate.Drop.Item.Uncommon/Rare` | 2.0 | Twice the greens/blues = twice the affix rolls |
| `Rate.Drop.Item.Epic` | 1.5 | Epics stay special |
| `Rate.Drop.Money` | 1.5 | With affix sell value (section 7), gold stops being a worry |
| `SkillGain.Gathering/Crafting` | 2 | 2 points per skill-up |
| `SkillChance.Yellow/Green` | 100 / 50 | (default 75 / 25) fewer wasted crafts |
| `MaxPrimaryTradeSkill` | 11 | All professions on one character |

Move-speed budget: 1.15 base x (1 + affixes, capped at 30%) x Paragon Utility (+10% cap) -> at most about 1.15 x 1.4 = 1.61x run speed.
That's fast but still readable; mounted speed scales the same way.

## 3. Stat basics at level 90 *(verify the ratings in game)*
| Stat | Rating per 1% at 90 | Notes |
| --- | --- | --- |
| Hit | 340 | 7.5% melee cap = 2550 |
| Expertise | 340 | 7.5% = dodge cap (2550); tanks also want parry cap 15% |
| Crit | 600 | |
| Haste | 425 | |
| Mastery | 600 | spec-specific |
| Dodge / Parry | ~885 *(verify)* | diminishing returns |

Our flat-stat affixes are a share of **B** (epic stat budget for the slot at your level). At T5 a "of the Wind" roll is 18-22% of B,
a secondary prefix 16-20% of B. Five T5 flat affixes on one item roughly double that item's stats: that is the "god roll" ceiling
and the reason for the watch-list note in SPEC 4.1.

## 4. Brewmaster vs Rogue with the affix pool (v1 + v2)

### 4.1 Brewmaster (tank, 2H staff/polearm)
How the stats work for a BrM *(verify)*: Agility -> attack power and dodge; Stamina -> health (and with it Thirsty/Phoenix/Warding
value, which scale with max health). Hit/Expertise to cap first (missed Jabs = less Chi = less Shuffle/Purify). Then Crit (Elusive
Brew stacks come from melee crits) and Haste (energy -> Chi) for active mitigation; Mastery for Stagger.

**Affix value for a BrM** (A = core, B = strong, C = situational):
| Affix | Grade | Why |
| --- | --- | --- |
| of the Wind (Agi), of the Whale (Sta) | A | main stats; Stamina also scales every %-of-max-health affix |
| Precise, Keen | A until capped, then dead | reroll/replace once at 2550 each |
| Deadly (crit) | A | Elusive Brew stacks |
| Thirsty (heal on hit, % max HP) | A | **tank's best sustain**: scales with your big health pool and attack rate |
| of Regeneration, of Warding, of the Phoenix | B | passive sustain / panic button; strong solo |
| Champion's (DR vs Champions/Superuniques) | B | exactly the fights that kill you solo |
| of Thorns, Arcing | B | damage while tanking packs -> faster solo clears |
| Swift (haste), Masterful | B | |
| Vampiric (leech, % of damage dealt) | C | a tank's low damage makes leech weak |
| Cruel (+weapon damage) | B for solo | BrM damage is weapon-based; speeds up solo play |

**BrM god rolls** (respecting max 3 prefixes / 3 suffixes / 5 total):
- Weapon: *Thirsty Deadly Cruel ... of the Wind of the Whale*
- Chest: *Champion's Arcing ... of the Whale of Warding of the Phoenix*
- Gloves: *Thirsty Deadly ... of the Wind of Gore of Regeneration* (Gore adds pack damage)
- Rings/amulet: *Precise/Keen until capped -> Deadly Swift ... of the Wind of the Whale of Regeneration*
- Helm: *Socketed Deadly ... of the Whale of Resolve of Light*

### 4.2 Rogue (dual wield / daggers)
Agility, Hit/Expertise caps (dual wield wants spell-hit-style 26% for white hits *(verify)*, but yellow cap 7.5% matters most), then
spec-dependent: Haste (Combat), Mastery (Assassination), Crit (Subtlety).

**Affix value for a Rogue:**
| Affix | Grade | Why |
| --- | --- | --- |
| of the Wind, Precise, Keen, Swift/Masterful/Deadly | A | damage |
| Vampiric (leech) | A | **a DPS's best sustain**: scales with high damage |
| Cruel | A | x2 weapons |
| Venomous, of Gore, Blazing/Shocking/Freezing | A | two fast weapons = many proc chances (internal cooldown 1 s keeps it sane) |
| of Crushing | A vs big trash | capped at 10% of max health per hit, off on bosses |
| of Evasion (dodge) | B | survivability without a tank |
| of the Whale | B | more health solo |
| Thirsty | C | small health pool |

**Rogue god rolls:**
- Main hand: *Vampiric Cruel Deadly ... of the Wind of Gore*
- Off hand: *Vampiric Venomous Swift ... of the Wind of Crushing*
- Chest: *Champion's ... of the Wind of the Whale of Warding*
- Boots: *Lucky Swift ... of the Wind of Speed of Freedom*

### 4.3 Takeaway
The two sustain families are deliberately split: **heal-on-hit (% max health) favours tanks, leech (% damage) favours DPS**. Each
spec has its own god rolls; neither depends on the other's affixes. Watch Thirsty on fast weapons: dual-wield rogues get more
hits but have less health, which is the intended trade-off.

## 5. Soloing 5-player dungeons (m5)
Party-size scaling (SPEC 4.6) at size 1: **30% health, 40% damage**, creature level = leader's. Rough model: a 5-man is tuned for
a tank + healer + 3 DPS, i.e. about 3.5 "DPS units" of damage output.

| Solo as | Damage output (DPS units) | Kill time vs a group | Damage taken | Verdict |
| --- | --- | --- | --- | --- |
| Rogue | ~1.0 | 0.30 x 3.5 / 1.0 = **~1.05x** | boss melee hits leather with no tank: about 2-2.5x what a tank takes -> at 40% = about what a tank would take | needs leech + Evasion/Feint/Cloak; viable with decent gear |
| Brewmaster | ~0.5 | 0.30 x 3.5 / 0.5 = **~2.1x** | designed for it; self-heals | comfortable but slower |

To keep **both** viable without making the tank boring, two knobs:
- `Arpg.Dungeon.SoloNonTankDamage` (proposed **0.75**): extra multiplier on creature damage when the lone player isn't in a tank
  spec. A DPS then takes about what a tank would; a tank gets no help it doesn't need.
- `Arpg.Dungeon.SoloTankDamageDealt` (proposed **1.25**): a lone tank deals +25% damage in dungeons, so clears aren't 2x slower.
Boss mechanics stay unchanged: you still have to move out of fire.

## 6. Raids as the endgame tier (extends m5)
**Every raid in the game becomes an endgame tier.** Raids scale to the leader's level (old raids rise to 90, through the core's AutoBalance level
scaling), then by party size, then by a per-raid **difficulty tier** that Paragon power is meant to overcome.

Solo raid multipliers (proposed; 10-man normal = 100%):
| Party size | Health | Damage |
| --- | --- | --- |
| 1 | 10% | 25% |
| 2 | 16% | 32% |
| 3 | 22% | 40% |
| 5 | 35% | 55% |
| 10 | 100% | 100% |

Tier ladder (`arpg_raid_tier`, recommended Paragon). The raid's extra multiplier is `1 + 0.15 x tier`, so higher tiers hit harder
and have more health, while Paragon power (section 6.1) grows to meet them:
| Tier | Raids (all scaled to 90) | Extra multiplier | Recommended Paragon |
| --- | --- | --- | --- |
| 1 | Molten Core, Blackwing Lair, Zul'Gurub (old), AQ20/40, Karazhan, Gruul, Magtheridon | 1.15 | 0-10 |
| 2 | SSC, Tempest Keep, Hyjal, Black Temple, Sunwell, Naxxramas, Eye of Eternity, Obsidian Sanctum | 1.30 | 10-25 |
| 3 | Ulduar, Trial of the Crusader, Icecrown Citadel, Ruby Sanctum | 1.45 | 25-40 |
| 4 | Cataclysm raids (BWD, BoT, ToFW, Firelands, Dragon Soul) | 1.60 | 40-60 |
| 5 | Mogu'shan Vaults, Heart of Fear, Terrace | 1.75 | 60-80 |
| 6 | Throne of Thunder | 1.90 | 80-110 |
| 7 | Siege of Orgrimmar | 2.05 | 110+ |

Loot: raid items keep their base stats, but **affixes roll for your level** (B = budget at your level), so an old raid's drop with
good affixes is still endgame-useful. Higher tiers: +1 affix tier per 2 raid tiers (cap T5) and higher Unique chances (section 8).

### 6.1 Paragon power (m4)
Per Paragon point (4 tabs, SPEC 4.5): Core +0.5% strength, agility, intellect or stamina (player's choice, owner 2026-11-22); Offense +0.4% crit/haste/mastery rating
budget; Defense +0.4% armour/avoidance; Utility as in SPEC. At about 1 point/level, Paragon 60 is roughly +30% main stat in your
chosen tab: enough to move a tier-5 raid from "wipe" to "farm" over a few weeks of play. **Target pace:** 5+ levels per 2-hour session
early (SPEC), then slower; a new tier every few sessions.

## 7. Gold without grinding (m2)
Affixed items get extra **sell value**, so normal play funds repairs, consumables and mounts:
`sell = template sell price + affixCount x playerLevel x Arpg.Loot.SellCopperPerAffixLevel` (proposed **200 copper**).
At 90, a 3-affix item sells for about 5.4 g on top of its base price. A typical hour of farming (60-90 affixed items, mostly auto-sold by
the loot filter) gives **about 350-500 g** plus 1.5x money drops. Repairs after a bad wipe are 20-40 g *(verify)*, so gold is
comfortable and never a reason to grind. Needs a small `// ARPG:` hook in the vendor sell handler (m2).

## 8. Uniques: tiered, with random suffixes (m2/m3)
Each Unique = **fixed signature mechanic + fixed affixes + 1-2 random suffixes**, rolled at drop, so two copies differ. Tiers like
D2's normal/exceptional/elite:
| Unique tier | Source | Random suffixes | Fixed-affix tier |
| --- | --- | --- | --- |
| I | Superuniques, raid tiers 1-2 | 1 | T3 |
| II | raid tiers 3-5 | 1-2 | T4 |
| III | raid tiers 6-7 | 2 | T5 |

Drop chance: a Superunique's own Unique **10%** per kill (SPEC said 30%; too generous with 5-minute respawns), with **bad-luck
protection**: +5% per kill without it, reset on drop: **about 5-6 kills on average (~30 min with 5-min respawns), never more than 19** (unit-tested). Dungeon and raid bosses: the 20%-ladder in §13 (guaranteed by run 5).

Signature mechanics are server-side spell/aura scripts (no client patch needed to work; the tooltip text needs ArpgUI or the patch).
Examples:
| Unique | For | Signature mechanic |
| --- | --- | --- |
| Keg-Breaker's Burden (staff) | Brewmaster | Purifying Brew heals you for 30% of the damage it removes |
| Ox-Soul Mantle (shoulders) | Brewmaster | Guard absorbs 40% more; when it breaks it explodes for the absorbed amount to nearby enemies |
| Drunken Haze (belt) | Brewmaster | Dodging an attack grants 1 Elusive Brew stack (ICD 1 s) |
| Twin Fangs of Venomspite (dagger) | Rogue | Finishers have a 20% chance per combo point to refund 1 combo point |
| Gloomstalker's Veil (cloak) | Rogue | Leaving stealth grants 4 s of 25% leech and 30% move speed |
| Bladestorm Treads (boots) | Rogue | Every 5th melee hit triggers Fan of Knives |
| Boots of the Long Road | anyone | +10% move speed **above** the affix cap |
| Band of the Plunderer (ring) | anyone | Loot pet range x2, +25% gold find |

## 8b. Unique powers: tiers, rolls and the chase (owner 2026-11-22)
A Unique's **signature power** has its own tier (T1-T5, Ascended T6-T8) and **rolls its numbers inside the tier's range**, like
D2's variable uniques. Two copies of the same Unique differ in tier, in the power's rolls and in the random affixes - that is the
chase. A "perfect" copy (both power numbers at the top of their range) is marked on the tooltip.

**Scaling stat: Power = the higher of attack power and spell power** (class-neutral: Intellect feeds spell power, Strength and
Agility feed attack power, so one rule works for every class and spec). Defensive powers scale on maximum health instead.

**Power tier comes from the drop** (same rules as affix tiers): player/loot level unlocks T1-T5 (1/15/35/60/85), Champion kills and
magic find add +1, raid tiers and Paragon unlock T6-T8 (§12). The Unique tier I/II/III from §8 (source) still decides how many
random affixes and the fixed-affix tier.

### Template ranges (per power type; every Unique uses one template and may shift it)
| Tier | Proc chance | Damage proc: % of Power | Heal/absorb proc: % of max health | Internal cooldown |
| --- | --- | --- | --- | --- |
| T1 | 15-20% | 60-90% | 3-5% | 3 s |
| T2 | 18-23% | 90-130% | 5-7% | 3 s |
| T3 | 20-25% | 130-170% | 7-9% | 2.5 s |
| T4 | 22-28% | 170-210% | 9-11% | 2 s |
| T5 | 25-30% | 210-260% | 11-13% | 2 s |
| T6 (Ascended) | 25-30% | 260-300% | 13-15% | 1.5 s |
| T7 | 28-32% | 300-340% | 15-17% | 1.5 s |
| T8 | 30-35% | 340-400% | 17-20% | 1 s |
"Every Nth hit" powers use N instead of a chance: T1 8th hit → T5 5th hit → T8 3rd hit.

**Why these numbers (level 90, MoP):** a geared character has Power about 40,000 and lands about 2.5 hits/s. A T5 damage proc with a
2 s cooldown fires at most 0.5 times/s → 0.5 × 2.35 × 40,000 ≈ 47,000 damage/s, about **20% of a 90 character's damage** - strong
enough to build around, not enough to replace the rotation. T1 at level 20 is the same share at that level (Power scales with level).
Heal/absorb powers: T5 = 12% max health per 2 s at most = 6% max health/s, about a third of what a tank takes in a hard pull.

### Display (client patch rows, one per power, tier and roll step)
`Equip: 27% chance on hit to unleash Keg Spill, dealing 215% of your Power as Fire damage to enemies within 8 yards. (Tier IV)`
Chance rolls in 1% steps and % of Power in 5% steps, so each power needs about 6 × 9 × 8 tiers ≈ 430 rows - 40 Uniques ≈ 17,000
rows, the same size as today's stat rows. A perfect roll appends ` - Perfect`. Names come from the Item-sparse patch (orange quality).

### Worked examples
| Unique (slot) | For | Power (template) | T1 | T5 | T8 |
| --- | --- | --- | --- | --- | --- |
| Keg-Breaker's Burden (staff) | Brewmaster | Keg Spill: chance on hit, Fire damage around the target (damage) | 15-20%, 60-90% | 25-30%, 210-260% | 30-35%, 340-400% |
| Ox-Soul Mantle (shoulders) | tanks | Ox Guard: when struck, absorb shield (heal/absorb); breaking it deals the absorbed amount to nearby enemies | 15-20%, 3-5% max health | 25-30%, 11-13% | 30-35%, 17-20% |
| Twin Fangs of Venomspite (dagger) | Rogue | Venom Burst: every Nth hit, Nature damage + 40% of it as poison over 6 s (every Nth) | 8th hit, 60-90% | 5th hit, 210-260% | 3rd hit, 340-400% |
| Censer of the Unseen Light (off-hand) | healers | Radiant Overflow: chance on heal, a Holy bolt at your target for % of Power (damage) - solo healers kill things | 15-20%, 60-90% | 25-30%, 210-260% | 30-35%, 340-400% |
| Stormcaller's Band (ring) | anyone | Static Field: chance on hit, Chain Lightning on 3 targets (damage, 70% of the template per target) | 15-20%, 42-63% | 25-30%, 147-182% | 30-35%, 238-280% |
| Boots of the Long Road (feet) | anyone | +10% movement speed above the affix cap (flat, no tiers) | +10% | +10% | +10% |

### The chase, layer by layer
1. **Find it:** boss pity ladder (guaranteed by run 5) or its Superunique (10% +5% per miss).
2. **Better tier:** higher level, Champion kills, magic find, then raid tiers and Paragon for T6-T8.
3. **Better rolls:** two numbers per power + the random affixes; "Perfect" marks the top roll.
4. Later (backlog): a Mystic NPC rerolls one random affix or one power number for gold + materials (gold sink).

## 8c. Typed blank bases (owner idea and DECISION 2026-11-22)
**Idea:** affixed items drop on a "blank" base - the item keeps its look, armour/weapon damage, sockets and quality colour, but
loses its template stats (the +855 Intellect / +474 Mastery lines); all of its power comes from our rolled affixes. D2-like:
a "Spelltwister's Grand Robe of the Phoenix" is only as good as its roll.

**What it fixes:** (1) every line on the tooltip is ours (clean, readable, no Blizzard secondaries we don't control),
(2) affix values become the whole item budget, so balance lives in one table (`gen_affix_sql.py`), (3) mismatched template
stats (Intellect robes for warriors) stop mattering.

**What it costs:**
- **Power:** today a blue = template stats (≈ main 43% + stamina 50% + 2 secondaries ≈ 60% of B) **plus** 2-3 affixes.
  Blank, it would only have the affixes → weaker. Fix: on blank bases, flat affixes get **x2.5** (T5 = 2.1-2.5 blue lines) and
  every affixed item gets **one guaranteed stamina affix** (survivability no longer depends on the template).
  Rough check, level 90 blue with 3 affixes at T4-T5: main 700×2.5 ≈ 1,750, stamina ≈ 1,500, one secondary ≈ 1,300 - about a
  raid epic's total, i.e. a good blue becomes a real upgrade; an epic with 5 affixes becomes the chase.
- **Tooltip:** template stat lines come from the client's item data (Item-sparse.db2), shared by every copy of that item.
  Hiding them for affixed copies only is not possible client-side, so blank bases need either **(a)** a client patch that blanks
  the stats of every equippable green/blue/epic (all copies - also bots' and vendors') with the server ignoring template stats
  for players, or **(b)** dedicated blank item entries (copies of popular bases with no stats, new item ids in our range) that
  drops are substituted with - more data, but untouched originals for bots, vendors and quests.
- **Bots:** keep vanilla gear either way (option b) or get stat-less gear (option a) - (b) is safer.

**DECIDED by the owner (2026-11-22) - typed bases:** the blank copy keeps **two base lines from the item
data: the armour type's main stat + Stamina** at the template's own values - plate: Strength or Intellect (two copies, for
paladins), mail and leather: Agility or Intellect, cloth: Intellect; weapons follow their usual users. Everything else (all
secondaries, extra main stats, effects) comes from up to **5 rolled affix lines** (slots 8-12). Base lines are item data and do
not use those slots, so an item shows 2 base lines + up to 5 affix lines. No spare slot on normal items; **Uniques roll at most 4
random affixes and use the 5th slot for their signature power line** (§8b).
Why better than fully blank: no x2.5 rebalance (main + Stamina ≈ a blue's own main and Stamina lines, so today's §16 affix values
fit as they are), Stamina is always there, and the base's main stat matches its armour type (settles the §15 question while
affixes may still add any main stat, D2-style). Smart loot then picks the base variant for the player's spec 70% of the time.

**Recommendation:** option **(b)** with typed bases, introduced with the Item-sparse patch (m6, needed anyway for Unique names): generate blank
copies of every droppable green/blue/epic base (~24,000 candidates already indexed by `Loot/Substitution`), map drops to their
blank copy in the same `LootFilled` hook, and switch the affix value table to the blank scale. Until then, today's "template +
affixes" stays. **Owner decides:** (a) or (b), and the x2.5 / guaranteed stamina numbers.

**Built (2026-11-27, change m6-blank-bases; owner: "all of these"):** option (b) as a *share* of drops, not all of them:
`Arpg.Blank.DropChance` 10% of gear drops, crafted gear, and the companion shop (essences 5 / 15 / 45 by quality). A dropped or
bought blank rolls the most affixes for its quality (green 2, blue 3, epic 5) - that is what makes it worth having over the
original with its template secondaries. Raise DropChance towards 100 to move fully to typed bases once it feels right.

## 9. Superuniques
`Arpg.Superunique.RespawnDelay` stays **300 s** (a different Superunique of the zone appears 5 minutes after a kill). With bad-luck
protection on Uniques, farming one zone for a specific Unique takes about 30 minutes on average. Good chase, no slog.

## 10. Healers solo too (owner 2026-11-22)
Goal: every spec is fun and solo-capable, even if it takes more grinding. Healers have the sustain but not the damage, so they get
**damage that comes from healing** (D2's "convert one strength into another" idea), via affixes, Uniques and a solo knob.

- Solo knob: `Arpg.Dungeon.SoloHealerDamageDealt` (proposed **1.6**) when the lone player is in a healing spec (dungeons and raids).
  Plus the non-tank damage reduction from §5 (0.75).
- New affixes (pool v2, healer-leaning; any spec can roll them):
| Affix | Kind | Effect | Slots | T1 → T5 | Weight |
| --- | --- | --- | --- | --- | --- |
| Radiant | Prefix | X% of your **overhealing** is dealt as Holy damage to the nearest enemy you're in combat with | Weapon, off-hand, amulet | 20% → 60% | 30 |
| Zealous | Prefix | Your heals also hit one nearby enemy for X% of the amount healed | Weapon, off-hand | 5% → 20% | 25 |
| of Atonement | Suffix | X% of the damage you deal heals you (works like leech, but also heals the lowest-health ally) | Weapon, ring, amulet | 2% → 8% | 25 |
| of Martyrdom | Suffix | While you're below 50% health, your heals on yourself also deal X% of the amount as damage around you | Chest, legs | 10% → 30% | 15 |

- Healer Uniques (signature mechanics, examples):
| Unique | For | Signature mechanic |
| --- | --- | --- |
| Penitent's Lightbringer (mace) | Holy Paladin | Holy Shock always hits the enemy too, and Beacon of Light copies 50% of your damage as healing |
| Thousand Gazes (staff) | Discipline Priest | Penance and Smite damage +60%; Atonement heals you as well |
| Thunderbloom Totem (off-hand) | Resto Shaman | Healing Stream Totem also casts Lightning Bolt at a nearby enemy each tick |
| Rootweaver's Coil (ring) | Resto Druid | Lifebloom blooming deals its heal as Nature damage to enemies within 8 yd |
| Mists of the Sunken Temple (staff) | Mistweaver | Soothing Mist channels on enemies as damage, healing you for 50% of it |

## 11. Dungeon level selector
You choose the **level a dungeon runs at**, between the dungeon's minimum level and your own level, at the entrance (companion
gossip, or `.arpg dungeon level <char> <n>`; remembered per dungeon). Creatures scale to that level (AutoBalance level scaling), and
**loot follows the chosen level**: affix values use B at that level and affix tiers unlock by it. Running a lower level means faster,
safer clears for lower-tier items, which is the way to work your way up. No XP above what the chosen level gives; no downscaling of
the player (it's meant to feel strong).

## 12. Paragon raises the loot tier in raids
Above T5, raids roll **Ascended affix tiers** depending on raid tier and your Paragon level (both must be met):
| Affix tier | Values (vs T5) | Raid tier | Paragon |
| --- | --- | --- | --- |
| T5 | 100% | any | any |
| T6 | 110% | 3+ | 25+ |
| T7 | 120% | 5+ | 60+ |
| T8 | 130% | 7 | 110+ |
Unique tier follows the same rule (Unique III needs raid tier 6+ and Paragon 80+). So Paragon is both your power **and** your loot
ceiling, and climbing tiers always pays off.

## 13. Drop rates and pity (no more than 5 runs for the good stuff)
Owner target: **you are always finding things**, and no dungeon or raid needs more than ~5 runs to get what you're after.

Per boss, per character, **pity counters** (`character_arpg_pity`):
- **Unique ladder:** a boss's Unique drops at **20% on the first run, +20% per run without it** → 20/40/60/80/**100%**. Guaranteed by the 5th
  run; the counter resets when it drops. (Replaces the flat 15% raid figure in §8.)
- **Quality floor:** every boss drops at least one item with **3+ affixes at the highest tier the run allows**. Every 3rd boss kill
  without a 5-affix item guarantees one ("god-roll pity"). Rerolling stays possible via crafting later (not in v1).
- **Smart loot:** 70% of drops match your spec's main stat (SPEC 4.1); boss Uniques for your class are picked over others 60% of the time.
- **Trash:** with 2x green/blue rates, a dungeon clear gives about 15-25 affixed items; most get auto-sold for gold by the loot filter.
- Superuniques keep their 10% + 5%/miss ladder (one counter for all Superuniques since 2026-11-27): ~5-6 kills on average, guaranteed by kill 19 (unit-tested; 5-min respawns → ~30 min typical).

Expected time to "done with a dungeon" (all its Uniques): 3-5 runs of ~15-25 minutes = roughly an evening. A raid tier: 5 runs per raid.

## 14. Early game: killable Champions and Superuniques (owner 2026-11-22)
Champions and Superuniques appear from level 1 in every zone, so they must be **beatable by a fresh character** with 2-3
buttons, no interrupt and often no self-heal. The full numbers (Champion +2 levels, 3x health, 1.5x damage; Superunique +3)
fit a character with a full kit. A level-5 character fighting a level-7 Champion would miss more often, get hit harder and
die. So their power comes in **level bands** (by the player's level, `arpg_monster_band`):

| Player level | Champion: level / health / damage / monster affixes | Superunique: level / health / damage / abilities |
| --- | --- | --- |
| 1-14 | +0 / 2.0x / 1.15x / 1 "soft" affix | +1 / 3.0x / 1.2x / 2 abilities, long telegraphed casts, no must-interrupt |
| 15-34 | +1 / 2.5x / 1.3x / 1-2 affixes | +2 / 4.0x / 1.35x / 3 abilities, 1 slow interruptible cast |
| 35-59 | +2 / 3.0x / 1.5x / 1-2 affixes | +3 / 5.0x / 1.5x / 3-4 abilities, 1 must-interrupt |
| 60+ | +2 / 3.0x / 1.5x / 2 affixes | +3 / 6.0x / 1.6x / 4 abilities, 1 must-interrupt |

- **Soft affixes only below 15:** Extra Strong, Extra Fast and Stoneskin. Fire, Cold and Lightning Enchanted unlock at 15
  (the death explosion and slow need a little kit to answer), Arcane Caster at 25.
- **Affix damage is a % of the player's max health**, not a flat number, so it scales fairly at any level: Fire Enchanted
  death explosion = 15% of max health below 35, 25% from 35. A one-shot from an affix is never possible.
- **Minions:** 1 at levels 1-14, 2-3 at 15-34, 2-4 from 35.
- **Health orbs** (ARPG flow, `Arpg.Champion.HealthOrbPct` 20): a Champion kill restores 20% health and mana/energy at once,
  so the pack fight doesn't end in a long drink break. Superuniques restore 40%.
- **Targets** (playtest): an at-level, normally geared character beats a Champion pack in **30-60 s** with **at most one
  ability rotation of trouble**, and a Superunique in **60-120 s**. Dying sometimes is fine; dying every time is not.
- **Rewards scale with the band:** a level-8 Champion still drops at least one green (with its 1 affix at T1+1 = T2) and
  25% blue, which is a big upgrade at that level. That's what makes them worth chasing early.
- **Bots:** Champion packs near a questing bot behave the same way (bots are real characters for the band), so low-level bot
  zones don't become death traps.

## 15. Open questions for playtests
- ~~Low-level affix values are tiny~~ → resolved by §16: values are now calibrated to real blues (level 15: +2–4 main stat,
  level 30: +4–8; a real level-15 blue chest has +7). No artificial floor needed; revisit after playtests.
- **Main-stat suffixes on any armour type** (a warrior got "of the Titan" + "of the Wind" on a cloth robe at 90). D2 allows it;
  option: weight main stats by the armour type's usual classes. Owner's call.


- Is 40% damage solo too easy for a geared BrM in heroics? (Then use the `Arpg.Dungeon.Difficulty` knob, per player.)
- T5 god rolls: is 5 flat affixes on one item too strong next to Paragon? Watch the main-stat totals.
- Thirsty on a fast BrM: cap total heal-on-hit at 1.5% max health per second?
- Raid tier ladder: check a few raids per tier for one-shot mechanics that don't scale (those need per-boss exceptions).

## 16. Affix values calibrated to real items (owner 2026-11-22)
Measured with `modules/mod_arpg/sql/tools/blue_stats.py` over `item_template` (no Blizzard random stats, no heirlooms),
stats scaled to chest size with the game's own slot budgets. Required-level checkpoints, median [10th–90th percentile]:

| Player level | Blue chest ilvl | Main stat | Stamina | Best secondary | B (our epic budget) | Main ÷ B | Secondary ÷ B |
|---|---|---|---|---|---|---|---|
| 15 | 18–21 | 7 [4–9] | 6 | 6 | 16 | 44% | 34% |
| 20 | 23–26 | 10 [6–12] | 7 | 6 | 20 | 49% | 30% |
| 30 | 31–37 | 11 [8–16] | 10 | 7 | 27 | 40% | 27% |
| 40 | 43–47 | 14 [11–20] | 14 | 14 | 34 | 41% | 40% |
| 50 | 53–57 | 16 [12–24] | 16 | 16 | 41 | 39% | 39% |
| 60 | 63–73 | 17 [11–26] | 27 | 16 | 48 | 36% | 33% |
| 70 | 114–115 | 31 [18–43] | 49 | 24 | 77 | 40% | 31% |
| 80 | 187–316 | 76 [62–229] | 114 | 68 | 191 | 40% | 36% |
| 85 | 333–424 | 283 [242–603] | 454 | 201 | 649 | 44% | 31% |
| 90 | 450–476 | 797 [698–907] | 1288 | 572 | 1710 | 47% | 34% |

(Greens: main ≈ 35% of B. Epics: main ≈ 55–85% of B. Below level 15 there are almost no blues; greens give +1–4.)

**Finding:** a blue's stat line is a steady share of B at every level, so B with our item-level curve already tracks the game.
**Rule now:** blue main line = 43% of B, best secondary = 34%, stamina = 50% (stamina is ~B×0.4 below 60, ~B×0.7 at 85+).
A stat affix rolls **T1 30–45% → T5 85–100% of that line**: a T5 roll equals a full blue line; Ascended T6–T8 (§12) go above.
Constants live in `gen_affix_sql.py` (`BLUE_LINE_PCT_OF_B`, `STAT_T1`, `STAT_T5`); re-run `blue_stats.py` after an upstream DB sync.

What a roll looks like (chest, Strength affix, min–max of the tier the level unlocks):

| Level | Unlocked tier | of the Titan | Real blue chest main stat |
|---|---|---|---|
| 15 | T2 | 3–4 | 7 |
| 30 | T2 | 5–7 | 11 |
| 60 | T4 | 15–18 | 17 |
| 90 | T5 | 625–735 | 797 |

**Power check (level 90, T5 god roll):** an epic keeps its own stats (≈1430 main) and adds up to one full blue main line (~735)
plus up to four secondary lines (~580 each), so roughly 1.5× a raid epic. That is the chase item; it needs T5 (level 85+), 5 affixes
(epic only) and no wasted main stat. Watch in playtests together with Paragon.
