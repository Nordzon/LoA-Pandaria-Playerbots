# Design

**Rule** (`Core::WidenWeaponRequirement`, unit-tested; the client patch builder uses the same rule in Python):
input spell family, required item class, weapon subclass mask, inventory type mask.
- Not a class family (Mage 3, Warrior 4, Warlock 5, Priest 6, Druid 7, Rogue 8, Hunter 9, Paladin 10, Shaman 11, DK 15,
  Monk 53) -> unchanged. Item class not weapon (2) -> unchanged (shields, armour).
- Subclass mask without any melee type (bow/gun/crossbow/thrown/wand only) -> unchanged.
- Otherwise: subclass mask |= all melee types (axes, maces, swords 1H/2H, polearm, staff, fist, dagger, spear; 0/-1 = any
  stays), inventory type mask = 0 (any hand).

**Server:** `Character/AnyWeaponSkills.cpp` walks `sSpellMgr` at startup and on `.arpg reload`, keeps the original values of
every spell it changes (restore when the switch is off), and widens via `const_cast` on the loaded `SpellInfo` (no core
patch). Main-hand spells: `AttributesEx3 &= ~SPELL_ATTR3_REQ_OFFHAND` -> `Spell` picks `BASE_ATTACK` (Spell.cpp:582) and
`CheckItems` no longer asks for an off-hand weapon. Passive auras with a requirement (Seasoned Soldier, Single-Minded Fury)
are re-checked on equip changes by the core as before; they now always fit a melee weapon.

**Client:** `build-client-patch.py` writes `SpellEquippedItems.dbc` (rows of widened spells) and `SpellMisc.dbc` (REQ_OFFHAND
cleared for the main-hand spells). Spell -> row: Spell.dbc field 16 (SpellEquippedItemsId), field 14 (SpellClassOptions ->
family), field 24 (SpellMisc). Both sides print the number of widened spells; the GM test checks the server's count, the
builder prints its own.

**Balance note:** Fury gets Single-Minded Fury with two-handers too; Monks get both Way of the Monk bonuses. Tune later in
BALANCE.md if it's too much.
