// Pure: widen class spells' weapon requirements (owner 2026-11-27, change m6-any-weapon-skills). No game headers -
// unit-tested; arpg/client-patch/build-client-patch.py applies the same rule to the client's SpellEquippedItems.dbc.
#ifndef ARPG_CORE_WEAPONREQ_H
#define ARPG_CORE_WEAPONREQ_H

#include <cstdint>

namespace Arpg::Core
{
    // Weapon subclasses as item_template numbers them: axe 0/1, mace 4/5, polearm 6, sword 7/8, staff 10, fist 13,
    // dagger 15, spear 17 (melee); bow 2, gun 3, thrown 16, crossbow 18, wand 19 (ranged).
    constexpr int32_t MeleeWeaponMask = (1 << 0) | (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8)
                                      | (1 << 10) | (1 << 13) | (1 << 15) | (1 << 17);
    constexpr int32_t ItemClassWeapon = 2;

    // True for the spell families of the 11 classes (Mage 3 ... Monk 53).
    bool IsClassSpellFamily(uint32_t family);

    struct WeaponReq
    {
        int32_t itemClass;      // -1 = none
        int32_t subClassMask;   // 0 or -1 = any subclass
        int32_t invTypeMask;    // 0 = any inventory type (one-hand 13, two-hand 17, main hand 21, off hand 22, ...)
    };

    // A class spell that needs a melee weapon accepts every melee weapon type in any hand. Shields/armour (item class
    // 4), ranged-only masks and non-class spells are returned unchanged. Returns true if anything changed.
    bool WidenWeaponRequirement(uint32_t family, WeaponReq& req);
}

#endif
