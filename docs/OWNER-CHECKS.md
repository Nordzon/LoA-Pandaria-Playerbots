# Owner checks (in game) - character: Lokond

Things only a person in the real client can confirm. Work top-down; write the result under each section (`> Result: ...`;
"ok" or "no: what happened" is enough). Client: `Wow-64-all.exe` (the addon is reinstalled by every rebuild; `/reload` once
after login). Commands go in the chat box exactly as written.

**Confirmed and removed:** visuals picked (loot beam 120720, Champion/Superunique glow 117230 - being built), any weapon on a
caster, character creation, catalyst item, professions panel (T button), potions work (heal/cooldown/stacking).
**Fixed since the last round (2026-11-27 00:03), needs a re-check:** the companion, Superuniques and Champion tags were
invisible / plain (creature ids were too large for the client) - sections 1-4; potion tooltip text - section 4.

## 0. Handy commands
| What | Command |
| --- | --- |
| Set level (1-90) | `.character level Lokond 20` |
| One level / several | `.levelup` (yourself), `.levelup Lokond 5` |
| Gold (6000 g) | target yourself, then `.modify money 60000000` |
| Teleport | `.tele goldshire`, `.tele stormwind`, `.tele ValeofEternalBlossoms` |
| Stuck lying on the ground (characters made before 2026-11-26 22:00) | `.unaura 73523` |

## 1. Loot companion: visible, loots, dialogue
1. Click the **L** button: the companion appears next to you (a small mechanical squirrel) and follows you.
2. Kill a few things: it walks to each corpse and the loot lands in your bags.
3. Right-click the companion itself: a dialogue opens. Try each option and note whether its window opens:
   Sell and repair, Bank, Mailbox, Change appearance (pick a look - the companion changes).
4. Right-click the **L** button: the loot filter panel opens.

> Result: Working very well!

## 2. Salvage and catalysts (companion dialogue)
1. `.arpg test give Lokond 2317` twice (two affixed greens in your bags).
2. Right-click the companion: "Salvage 2 affixed item(s) for N essence(s)" -> a confirm box -> both items turn into Salvaged
   Essence.
3. "Buy catalysts": Lesser / Greater / Grand with essence prices; buying one with enough essences works.

> Result: Works

## 3. Mount shop (companion dialogue)
1. Gold: target yourself, `.modify money 60000000`.
2. Right-click the companion -> "Mounts for gold": pages of 15, "More mounts..." at the bottom.
3. Click one: a price popup (5000 g); after buying, the mount is in your mount journal and gone from the list.
   (Riding needs the riding skill as always: `.character level Lokond 20` then a riding trainer, or just check the journal.)

> Result: (and: does 5000 g feel right?): Mounts work, let's leave 5000 for now. Can you make the UI a vendor one instead of a dialogue one?

## 4. Champion and Unique tags, potion tooltip (Goldshire)
1. `.tele goldshire`
2. `.arpg champion make Lokond 299` -> a Young Wolf pack next to you. Target the leader: "Champion Young Wolf" with the silver
   dragon (Rare) portrait. `.arpg champion kill Lokond all` clears it.
3. `.arpg superunique spawn Lokond` -> a visible, targetable "Unique <name>" with the winged gold dragon (Rare Elite) portrait.
   `.arpg superunique fight Lokond` makes it attack you; `.arpg superunique kill Lokond` removes it.
4. `.additem 118 5` and hover the potion: besides the stock "Use:" line, a yellow line "ARPG: heals 10% of your maximum health.
   10 sec shared cooldown." (if not: restart the client once - it may have cached the old text).

> Result: Champtions work very well! And drop nice greens. Superuniques also work and drop items as required. I am still noticing that the items get updated within my inventory instead of the corpses themselves. This is a small issue as the pet will loot for us anyway, but add to fix pile. Potions still have the incorrect tooltip, but do restore the correct amounts. (tooltip says restores some flat amount of health, using the potion restores % health as we planned, cooldown works.)

## 5. Loot names on the corpse
Kill a few things near your level (with the companion off: click **L** again). Affixed greens/blues already show their full
name in the loot window ("... of the Bear") before you take them; the item in your bags is the same.

> Result: Does not work correctly, loot gets updated in my bag,

## 6. New character starts normally
Make a new character of a pair stock MoP doesn't allow (e.g. Night Elf Monk or Tauren Rogue), enter the world: you can move
right away (no "lying on the ground"). Known, being fixed: such pairs start without their class's starter gear.

> Result: Characters start normally, but some class/race combos are naked (ex: Goblin Monk)

## 7. Paragon (needs level 90 - use another character if you want Lokond to stay low)
1. `.character level Lokond 90`, then `.arpg paragon set Lokond 10` (10 Paragon levels = 10 points).
2. The **P** button or `/paragon`: the window shows the points; spending one changes your stats.

> Result: Working as expected.

## 8. Later (not now): auction house
`.tele stormwind`, Trade District auctioneer: mostly materials, some gear, about a third of the green-epic gear with affixes.

> Result:
