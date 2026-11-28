# Owner checks (in game) - character: Lokond

Work top-down; write the result under each section (`> Result: ...`; "ok" or "no: what happened" is enough). Always start
the client as **`Wow-64-all.exe`** (stock `Wow-64.exe` ignores our patch: plain blues, no new race-class pairs). Commands go in
the chat box exactly as written.

**Confirmed and removed:** companion + dialogue, salvage/catalysts, Champion/Unique tags and drops, Paragon, professions panel,
any weapon, character creation, catalysts, potion effects, mount purchase, client patch retest (0b), loot beam + Champion
glow (2), mount vendor window (4), potion tooltip (5), blank bases use (7: "all of these" - built, see 1 below).

## 0. Handy commands
| What | Command |
| --- | --- |
| Set level (1-90) | `.character level Lokond 20` |
| Gold (6000 g) | target yourself, then `.modify money 60000000` |
| Teleport | `.tele goldshire`, `.tele stormwind`, `.tele ValeofEternalBlossoms` |

## D. New 2026-11-28 afternoon - Champion camps, Superuniques in every zone
**D1. Camps:** walk through a few monster camps (Wandering Isle is fine): each camp (about 100 yards across, 4+ monsters)
has one Champion pack; after you kill it, the camp gets a new one about 10 minutes later. Even enough? Too many / too few?
(Numbers: `Arpg.Champion.CampSize` 100, `CampMin` 4, `CampRespawn` 600.)
**D2. Never Champions:** quest givers, sparring trainees, Training Targets, story NPCs. If you still see one, tell me its name
(I add it to the exclusion list, no rebuild needed).
**D3. Superuniques:** on the Wandering Isle "Unique Pei-Wu Tiger", "Unique Deepscale Ravager", "Unique Darkened Horror" and
two more show up (up to 3 at a time per zone; the zone chat announces them). Every zone now has at least 5 in its pool.

> Result:

## C. New 2026-11-28 midday - 999 stacks, companion class trainer, professions, camera
Live after the next server restart (I'll ask before restarting - you're online). The camera part works now: type `/reload`.

**C1. Camera** (works now): `/reload`, then scroll out: about twice as far as before. `/arpgcam` shows the values;
`/arpgcam 3` (or 2, 4...) sets another distance factor if 4 feels too far.

**C2. Stacks to 999:** loot cloth/herbs/ore: stacks grow past 20 up to 999; merging two stacks works by dragging. If the
client still says a stack is full at 20, tell me (it means the client didn't take the new size).

**C3. Companion class trainer:** right-click the companion -> **Class trainer**: reset specialization, reset talents (the
game's own confirm box), Learn Dual Specialization (level 30+, 10 g), Proving Grounds (90), **Learn a new profession**.

**C4. Professions:**
1. Companion -> Class trainer -> Learn a new profession -> pick any you don't have (also a 4th, 5th...): you keep the others.
   Morda: Leatherworking was saved as a skill without its spell (that's why its icon is a "?"); it shows in that list too -
   learning it there fixes it and keeps the points.
2. The game's own "abandon profession" button is now refused (chat says how to really drop one: `.professions abandon <name>`).
3. Higher ranks (Journeyman at 50 and up) and recipes: the normal profession trainer, for every profession.
4. **Question:** which professions did Morda lose, and roughly when? (Morda's last saves only ever had Leatherworking; from now
   on every learned/removed profession is logged, so if it happens again I can see exactly what did it.)

> Result:

## B. New 2026-11-28 (built overnight) - charms, shrines, Treasure Goblins, rifts
Start `Wow-64-all.exe` fresh: ArpgUI got a new file (rift timer), and the new items come from the server (no patch needed).
Any character works; Nordana is fine. Commands below use Nordana; swap in your character's name.

**B1. Charm Pouch + charms**
1. Log in: you get a **Charm Pouch** (blue bag, 8 slots), straight into a free bag slot, otherwise into your bags. If it's
   in your bags, drag it into a bag slot.
2. Get charms: `.arpg charm give Nordana grand`, `.arpg charm give Nordana small`, `.arpg charm give Nordana coin`. They go
   straight into the pouch. Open your character sheet: stats / gold find go up.
3. Drag a charm out of the pouch into a normal bag: its stats go away within a second. Drag it back: they return. Only
   charms fit in the pouch.
4. Tooltip: does a charm show its lines ("+N Stamina", "x% Magic Find"...)? If not, tell me. The lines still work, they're
   just not shown on a non-gear item yet.
5. Feel: are charm stats too small or too big? (A Grand Charm line is about a third to half of a ring's affix line.)
6. Drops: charms drop now and then (Champions 6%, Superuniques 20%, bosses 12%, normal mobs 0.3%).

**B2. Shrines**
1. `.arpg shrine spawn Nordana` (random) or `.arpg shrine spawn Nordana 6` (Fortune). A Pandaren shrine appears in front of you.
2. Click it: you get a buff for 2 minutes and the shrine disappears. Check the buff tooltip shows the right number
   (Frenzied 20% haste, Combat 15% damage, Protection 20% less damage, Fleeting 30% speed, Enlightened 25% XP, Fortune).
3. While clearing, roughly every 100 kills a shrine appears by itself at a corpse (at most one per 4 minutes).

**B3. Treasure Goblin**
1. `.arpg goblin spawn Nordana`: a Treasure Goblin appears about 15 yards away ("A Treasure Goblin appears!").
2. Hit it: it runs. Let it run 30 seconds: "escaped through a portal", gone. Spawn another and kill it fast: gold, 4+ gear
   items, essences, maybe a charm or catalyst.
3. In the wild it shows up about once per 250 kills (at most every 10 minutes). Does it run sensibly, or get stuck on walls?

**B4. Rifts** (any level; the Deadmines or a Pandaria dungeon)
1. Right-click your companion -> **Open a Rift (your key: 1)** -> "Rift level 1". (Or type `.rift open 1`.)
2. Enter a 5-man dungeon: chat says "Rift level 1 opened: par time 25:00 ...", and a timer line shows at the top of the
   screen (ArpgUI).
3. Kill the last boss: "Rift level 1 complete in mm:ss: key +N". Reward items go into your bags. `.rift info` shows your key.
4. Try a higher level: monsters get tougher (+10% health and +7% damage per level, compounding); from level 2 a run is
   Fortified (tougher trash) or Tyrannical (tougher bosses).
5. Feel: par times OK? Rewards worth it?

> Result:

## A2. New 2026-11-27 22:05 - starting quests for new race/class pairs (Nordana)
1. Nordana: go to Ilthalaine (or Melithar Staghelm): "Simple Sigil" is offered - take it, it's done at once.
2. Hand it in to Alyissia (the Warrior trainer in Aldrassil): "Learning New Techniques" - also done on accept; hand it in.
3. "Priestess of the Moon" follows, and the rest of Shadowglen as usual.
4. Any other new race/class character later (e.g. a Human Shaman in Northshire): Marshal McBride should offer "Beating Them
   Back!" at level 1.

> Result:

## A. New 2026-11-27 20:45 - class abilities with any weapon (patch installed 21:12: start `Wow-64-all.exe` fresh)
1. A Rogue with a **two-handed sword** (`.additem 2489`): Backstab, Mutilate (with no off-hand weapon), Dispatch, poisons and
   Shiv all work; none is red on the action bar.
2. A Shaman with a **staff** (`.additem 2132`): Lava Lash works without an off-hand weapon.
3. Still needed: Shield Slam needs a shield; Hunter shots need a bow, gun or crossbow.
4. ~~Hunter shots without a ranged weapon?~~ - answered "keep the requirement".

> Result:

## 1. New 2026-11-27 08:15 - start `Wow-64-all.exe` fresh (the addon got new files)
1. **Corpse loot preview** (replaces the old "loot names on the corpse" check): companion off (L), kill something with a
   green/blue: hovering the item in the loot window shows its affix lines; after looting, the bag item has exactly the same lines.
2. **Blank bases - drops:** `.arpg config set Arpg.Blank.DropChance 100`, `.tele goldshire`, `.arpg champion make Lokund 299`,
   kill the pack: the green/blue items are blanks - tooltip: main stat + Stamina, then the affix lines (green 2, blue 3), and
   the yellow line "Blank base: always rolls the most affixes for its quality." Afterwards `.arpg config set Arpg.Blank.DropChance 10`.
3. **Blank bases - companion:** `.additem 700010 50` (essences), right-click the companion -> "Blank bases for essences" ->
   Chest -> Blue: 15 essences are taken and a leather Agility chest (rogue) with 3 affixes appears.
4. Crafting (only if Lokund has a profession that makes gear): crafted gear comes out as its blank.

> Result:

## 2. Superuniques everywhere and more Champions (keep as is - your note: "need to play more to know")
Any zone has a Superunique of its own rares ("Unique <name>"); Champion packs are 5% of monsters. Note anything that feels off.

> Result:

## 3. Companion windows (one check left from the companion change)
Right-click the companion -> Reforge, Transmogrify, Void Storage: does each open its normal window?

> Result:

## 4. Questions for you (no testing)
1. ~~Bots' loot rolls~~ - answered "bots never Need is perfect" (already so via playerbots `LootNeedRollLevel` 1; now pinned).
2. ~~Unique bad-luck counter~~ - answered "one shared counter" (built; `Arpg.Superunique.SharedBadLuck` 1).
> Answers: 1. Bots never need is perfect. 2. One shared counter.

## 5. Friends: your part (when you want to invite mates; guide: `docs/FRIENDS.md`)
The sign-up page, the network settings and the client kit are built and tested. What only you can do (system changes):
1. Install Tailscale, note your 100.x address, share your PC with your mates.
2. As administrator: the firewall rule for 3724/8085/8090 from Tailscale, and **limit the existing "Port 3307" rule** (MySQL is
   reachable from your network today - see FRIENDS.md step 2).
3. Fill in `network.local.ps1` and `signup.local.json` (copy the .example files), then tell me: I'll do the one restart with you.

> Result:

## 6. ~~Rules with several real players~~ - answered "as recommended", Vale tier "highest for now"
Built (live after the next restart): highest level scales a monster; personal loot (your own corpse, mates get their own roll
in their bags; groups with 2+ real players switch to round robin); one companion each; own Champion/Superunique drops and Unique
counters; 300 bots in friends mode. **Paragon tier switch, no rebuild needed:**
`.arpg config set Arpg.Paragon.WorldTierMode average` (or `lowest` / `highest`); to keep it after restarts, set the same line in
`arpg.conf` next to the server (build/bin/RelWithDebInfo).

## Later: auction house
`.tele stormwind`, Trade District auctioneer: mostly materials, some gear, about a third of the green-epic gear with affixes.

## Playtests (standing list - write notes whenever you've played; no rush)
Moved here from the change folders so finished features can be archived. Tuning goes into `docs/BALANCE.md`.
- World scaling: at 85 in Northshire, Westfall, Jade Forest - kill times feel right?
- Loot: an evening of levelling - are drops exciting?
- Champion packs: an at-level character beats a pack in 30-60 s?
- Paragon: 5 or more levels in a 2-hour session at the start?
- Dungeons: solo a 5-man as tank, DPS and healer.
- Companion: clear 3 packs in Jade Forest - it never gets stuck and feels fast enough.

> Notes:
