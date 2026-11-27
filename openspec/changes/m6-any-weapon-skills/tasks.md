# m6-any-weapon-skills tasks

## 1. Rule
- [x] 1.1 `Core/WeaponReq.{h,cpp}` `WidenWeaponRequirement` + unit tests (dagger, 1H, 2H, off-hand, ranged kept, shield kept, non-class kept)

## 2. Server
- [x] 2.1 `Character/AnyWeaponSkills.cpp`: apply at startup + `.arpg reload`, restore when `Arpg.AnyWeapon.Skills` 0;
  `Arpg.AnyWeapon.MainHandSpells` (1329 60103 100130)
- [x] 2.2 `.arpg test spellreq <char> <spellId> [itemId]`: requirement + fits the item in main hand / needs off-hand
- [x] 2.3 GM test `56-weapon-skills`: Backstab/Mutilate/Dispatch with a 2H sword, poisons with a 2H, Lava Lash with a staff,
  Seasoned Soldier with a dagger, Shield Slam still needs a shield, Hunter shot still needs a ranged weapon

## 3. Client
- [x] 3.1 `build-client-patch.py`: `SpellEquippedItems.dbc` + `SpellMisc.dbc` with the same rule; counts match the server
- [ ] 3.2 Owner: a rogue with a two-handed sword uses Backstab/Mutilate; a shaman with a staff uses Lava Lash (OWNER-CHECKS)

## 4. Docs
- [x] 4.1 SPEC 4.9, CODEMAP, BALANCE note, PROGRESS
