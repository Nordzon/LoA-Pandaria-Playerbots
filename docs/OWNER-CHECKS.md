# Owner checks (in game)

Things only a person in the real client can confirm. Work top-down when you have 20-30 minutes; write the result under each
section (`> Result: ...`; "ok" or "no: what happened" is enough). Client: `Wow-64-all.exe` (the addon is reinstalled by every
rebuild; type `/reload` once after login). GM commands go in the chat box; replace `<you>` with your character's name.

Done and removed (2026-11-26): visuals picked (loot beam 120720, Champion/Superunique glow 117230 - being built), any weapon
on a caster (works), character creation (works; findings below are being fixed), catalyst item looks.

## 0. Handy commands
| What | Command |
| --- | --- |
| Set your level (1-90) | `.character level <you> 90` |
| One level up / several | `.levelup` (yourself), `.levelup <you> 5` |
| Paragon levels (at 90) | `.arpg paragon set <you> 10` (10 levels = 10 points) |
| Gold | `.modify money 60000000` with yourself targeted (= 6000 g) |
| Teleport | `.tele goldshire`, `.tele stormwind`, `.tele orgrimmar`, `.tele ValeofEternalBlossoms` |
| Stuck lying on the ground (characters made before 2026-11-26 22:00) | `.unaura 73523` with no target |

## 1. Professions panel
Click the **T** button by the minimap (or `/prof`). Every profession you know is listed; clicking a crafting one opens its
window. Learn a few first if you have none: `.arpg test learnall <you>`.

> Result: These and the box with professions is working well! More to test during gameplay. 

## 2. Loot companion and its dialogue
Click the **L** button: the companion appears and follows you; kill a few things - it walks to the corpses and loots them into
your bags. Right-click the **L** button: the loot filter panel. Right-click the companion itself: a dialogue with Sell and
repair, Bank, Mailbox, Salvage, Buy catalysts, Mounts for gold, Change appearance. Answer for each window: opens / doesn't
(also: do Reforge, Transmog, Void Storage exist anywhere?).

> Result: Loot companion is working, but there are no visuals so I cannot click on it and use the dialogue. I do have auto-loot though!

## 3. Salvage and catalysts (companion dialogue)
Get two affixed greens (kill things, or `.arpg test give <you> 2317` twice). Right-click the companion: "Salvage 2 affixed
item(s) for N essence(s)" asks to confirm and turns them into Salvaged Essence; "Buy catalysts" lists Lesser/Greater/Grand with
prices. Answer: ok / what's off.

> Result: Cannot see companion

## 4. Mount shop (companion dialogue)
Give yourself gold (section 0), right-click the companion -> "Mounts for gold". Pages of 15 with "More mounts..."; clicking one
shows a price popup; after buying, the mount is in your mount journal and gone from the list. Answer: ok / what's off (and
whether 5000 g feels right).

> Result: Cannot see companion

## 5. Auction house (new)
Visit an auctioneer in a capital (`.tele stormwind`, Trade District). It fills over the first minutes after a server start:
mostly materials (cloth, ore, herbs, gems, glyphs, recipes, potions), some gear; about a third of the green-epic gear carries
affixes (affix names in the item name, prices well above plain items). Answer: ok / what's off (prices, amounts).

> Result: Not important now

## 6. Champions, Superuniques, potions, Paragon
Do these in Elwynn Forest (`.tele goldshire`): Superuniques only live in Elwynn, Dun Morogh and Westfall.

- **Champion tag.** `.arpg champion make <you> 299` spawns a Young Wolf pack next to you with a Champion leader. Target the
  leader: "Champion Young Wolf" with the silver dragon (Rare) portrait (only creatures that spawn in the world have a tag copy). `.arpg champion kill <you> all` clears the pack.
- **Unique tag.** `.arpg superunique spawn <you>` puts the zone's Superunique next to you. Target it: "Unique <name>" with the
  gold dragon (Rare Elite) portrait. `.arpg superunique fight <you>` makes it attack you; `.arpg superunique kill <you>`
  removes it.
- **Potions.** `.additem 118 50`, `.additem 2455 50`, `.additem 2456 50` (healing, mana, rejuvenation). They stack to 999 in
  one slot; drinking one starts the 10 s cooldown on all three.
- **Paragon.** At 90 (section 0): the **P** button or `/paragon`; spend points, the stats change.

> Result: Champions have no fancy visuals or portraits so far as I can tell. .arpg superunique spawn Lokond spawned an invisible creature in Goldshire that I cannot seem to target. Potions are working well, but the tooltip is incorrectly showing the usual healing potion tooltip (functionality is correct)

## 7. Loot names on the corpse
Kill something near your level. Affixed greens/blues already show their final name in the loot window ("... of the Bear")
before you pick them up, and the item in the bags is that same item.

> Result:
