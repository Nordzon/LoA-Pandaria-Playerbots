# m6-any-weapon

## Why
Owner 2026-11-26: "I want the weapon restrictions to be dropped (so any class can wear any weapon or shield)."
In an ARPG with random affixes the best roll should be wearable whatever its weapon type; a priest with a shield or a
mage with a two-handed axe is a valid choice.

## What changes
- Every real player knows every weapon proficiency (one/two-handed axes, maces, swords, polearms, staves, daggers, fist
  weapons, bows, guns, crossbows, wands) and Shield: the spell (client shows the item as usable) and the skill (server
  equip check). Granted at login and on level-up; nothing stored that a config flip can't undo at the next login.
- Config: `Arpg.AnyWeapon.Enabled` (1), `Arpg.AnyWeapon.Bots` (0: bots keep their class's weapons, so their gear choice
  stays sane), `Arpg.AnyWeapon.DualWield` (0: off-hand weapons stay with the classes that dual-wield).
- `.arpg test weapons <char> [itemId]`: proficiencies known, and whether the character can equip that item.
- Out of scope: items limited to a class by the item itself (`AllowableClass`, a few class quest rewards and legendaries);
  class abilities that need a weapon type (a warrior's Shield Slam still needs a warrior; a non-hunter has no shot with a bow).

## Milestone
6 (owner request; needs no core patch).
