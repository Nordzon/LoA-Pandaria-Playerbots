# Owner checks (in game)

Things only a person in the real client can confirm. Work top-down when you have 20-30 minutes; tell Claude the result of
each line (a number or "ok" / "no: what happened" is enough). Client: `Wow-64-all.exe`. Type `/reload` once after login so
the addon picks up new files. GM commands go in the chat box; replace `<you>` with your character's name.

## 1. Pick two visuals (loot beam, Champion glow)
`.arpg test visual <you> <spellId>` puts that spell's visual on the creature nearest to you (stand next to any mob) for 60 s;
`.arpg test visual <you> clear` removes it. Try each and note the ones you like.

**Loot beam** (shows a corpse with something worth picking up; should read from a distance, not blind you):

| Spell | Name in the game data |
| --- | --- |
| 52407 | WotLK Prologue Light Beam |
| 52427 | WotLK Prologue Light Beam, minor |
| 57772 | Beam of Light |
| 114382 | Beam of Light |
| 147974 | Pillar of Light |
| 120720 | Light Beam: Feralas Blue |
| 135419 | Light Beam: Feralas Green |
| 58042 | Treasure Sparkle |
| 85560 | Loot Sparkles |

**Champion glow** (marks a Champion pack leader; visible in a fight, not too loud):

| Spell | Name in the game data |
| --- | --- |
| 66602 | Red Aura |
| 66601 | Blue Aura |
| 103053 | Blue Aura |
| 94657 | Red Glow Silent |
| 84702 | Blue Glow |
| 64161 | Empowered |
| 117230 | Empowered |
| 145050 | Empowered |

Answer: "beam = <id>, glow = <id>" (or "none of them, show me more").

## 2. Character creation: any race, any class
Log out to the character list -> Create. Every race should offer all 11 classes. Make e.g. an Undead Monk or a Tauren Rogue,
enter the world, cast a class spell, then delete it if you like. Answer: ok / which pair failed.

## 3. Any weapon (a caster with a shield)
On (or create) a mage or priest: `.additem 1201` (Dull Heater Shield) and `.additem 2489` (Two-Handed Sword). In the bags
neither should be red; both should equip. Answer: ok / which one refused (and the error text).

## 4. Professions panel
Click the **T** button by the minimap (or `/prof`). Every profession you know is listed; clicking a crafting one opens its
window. Answer: ok / which one didn't open.

## 5. Loot companion windows
Click the **L** button to summon the companion, right-click it. A dialogue offers Sell and repair, Bank, Mailbox (and later the
mount shop). Answer for each: opens / doesn't: Sell+repair, Bank, Mailbox, Reforge, Transmog, Void Storage.

## 6. Earlier items still unconfirmed
Do these in Elwynn Forest (`.tele goldshire`): Superuniques only live in Elwynn, Dun Morogh and Westfall.

**Champion tag.** `.arpg champion make <you> 6` spawns a Kobold Vermin next to you as a Champion pack (leader + minions; any
creature entry works, e.g. 299 Young Wolf). Target the leader: the name should read "Champion Kobold Vermin" with the silver
dragon (Rare) portrait border. `.arpg champion info <you>` lists its affixes; `.arpg champion kill <you> all` clears the pack.

**Unique tag.** `.arpg superunique spawn <you>` puts the zone's Superunique next to you (Elwynn: Morgaine the Sly, Gruff
Swiftbite, Mother Fang, Fedfennel or Narg the Taskmaster; add an id from `.arpg superunique info <you>` for a specific one).
Target it: "Unique <name>" with the gold dragon (Rare Elite) border. `.arpg superunique fight <you>` makes it attack you (its
abilities go at whoever holds aggro); `.arpg superunique kill <you>` removes it.

**Potions.** `.additem 118 50` (Minor Healing Potion), `.additem 2455 50` (Minor Mana), `.additem 2456 50` (Minor
Rejuvenation). They should stack to 999 in one slot; drink one: the 10 s cooldown sweep shows on all three (shared
cooldown). Heal = a % of your max health (higher tiers heal more).

**Paragon.** Only at level 90: `.character level <you> 90`, then the **P** button or `/paragon`. `.arpg paragon set <you> 10`
gives 10 Paragon levels (10 points) to spend.

Answer: Champion tag ok/no, Unique tag ok/no, potions 999 + cooldown ok/no, Paragon ok/no.
