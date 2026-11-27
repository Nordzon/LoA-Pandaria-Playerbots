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
