# m6-any-weapon-skills: class abilities work with any melee weapon

## Why
Owner 2026-11-27: "make sure no skills require specific weapons or a specific 1h or 2h weapon. Shield-requiring skills
stay as requiring a shield." Since `m6-any-weapon` any class can equip any weapon, but about 50 class spells still need a
weapon type (Backstab/Mutilate/Dispatch: daggers; poisons, Shiv, Envenom, Blade Flurry: one-handers; Lava Lash, Wild Strike,
Mutilate: an off-hand weapon; Seasoned Soldier, Might of the Frozen Wastes: two-handers; Single-Minded Fury, Titan's Grip,
Assassin's Resolve, Way of the Monk, ...).

## Scope
- Server: at startup (and `.arpg reload`) every class spell (spell family of a class) whose weapon requirement includes a
  melee weapon accepts **any melee weapon in any hand** (weapon type list widened to all melee types, one-/two-hand/main/off
  -hand limit removed). Ranged-only requirements (Hunter shots: bow/gun/crossbow) stay (owner confirmed 2026-11-27: "keep the requirement"). Shield and armour requirements stay.
- Spells that need an off-hand weapon to be cast at all (`Arpg.AnyWeapon.MainHandSpells`: Mutilate, Lava Lash, Wild Strike)
  hit with the main-hand weapon instead, so a two-hander can use them. Their off-hand follow-up hits (Mutilate Off-Hand,
  ...) still happen only with an off-hand weapon.
- Client: the same edits in the client patch (`SpellEquippedItems.dbc`, `SpellMisc.dbc`), so the action bar doesn't grey
  out the spell and the client doesn't refuse it.
- Switch: `Arpg.AnyWeapon.Skills` (1). Applies to everyone (spell data is shared): players, bots.

## Not in scope
Damage bonuses inside spell scripts stay (Ambush/Hemorrhage +45% with a dagger, Heroic Strike +40% with a one-hander,
Monk/DK two-hander numbers): they don't block a spell. Armour specialisations (Leather Specialization) stay.
