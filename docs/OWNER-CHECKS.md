# Owner checks (in game) - character: Lokond

Work top-down; write the result under each section (`> Result: ...`; "ok" or "no: what happened" is enough). Start the
client fresh (`Wow-64-all.exe`): the client patch changed overnight (potion tooltips), and the addon is reinstalled by every
rebuild. Commands go in the chat box exactly as written.

**Confirmed and removed:** companion + dialogue, salvage/catalysts, Champion/Unique tags and drops, Paragon, professions panel,
any weapon, character creation, catalysts, potion effects, mount purchase.

## 0. Handy commands
| What | Command |
| --- | --- |
| Set level (1-90) | `.character level Lokond 20` |
| Gold (6000 g) | target yourself, then `.modify money 60000000` |
| Teleport | `.tele goldshire`, `.tele stormwind`, `.tele ValeofEternalBlossoms` |

## 1. Loot names on the corpse (fixed overnight)
Companion off (click **L**). Kill things near your level: affixed greens/blues show their full name in the loot window
("... of the Bear") before you take them, and the bag item is the same.

> Result:

## 2. Loot beam and Champion glow (new - your picks)
1. `.tele goldshire`, `.arpg champion make Lokond 299`: the pack leader has the "Empowered" glow (117230).
2. Kill the pack: corpses holding a green or better item get a blue light beam (120720) until you loot them.
3. `.arpg superunique spawn Lokond`: the Superunique has the same glow.

> Result: (too loud / too quiet / fine?)

## 3. Superuniques everywhere and more Champions (new)
Go to any other zone (e.g. `.tele stormwind` then ride out, or `.tele ValeofEternalBlossoms`): a Superunique of that zone's
own rares appears somewhere in it ("Unique <name>"); Champion packs are now 5% of monsters (was 2%).

> Result: (enough / too many Champions?)

## 4. Mount shop as a vendor window (changed overnight)
Right-click the companion -> "Mounts for gold": a normal vendor window with all 64 mounts at 5000 g; buying teaches the mount.

> Result:

## 5. Potion tooltip (client patch, new)
`.additem 118 5`, hover: "Use: Restores 10% of your maximum health. Shares a 10 sec cooldown with other potions."

> Result:

## 6. New character starter gear (fixed overnight)
A new Goblin Monk (or another unusual pair) starts dressed in its class's starting outfit.

> Result:

## 7. Question for you (no testing): typed blank bases
What should they be for: (a) a drop that always rolls the most affixes for its quality, (b) the crafting wheel's target items
(craft a blank of your armour type, it rolls), (c) sold by the companion for essences, or (d) all of these?

> Answer:

## Later: auction house
`.tele stormwind`, Trade District auctioneer: mostly materials, some gear, about a third of the green-epic gear with affixes.
