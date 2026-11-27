#include "WeaponReq.h"

namespace Arpg::Core
{
    bool IsClassSpellFamily(uint32_t family)
    {
        switch (family)
        {
            case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: case 11: case 15: case 53:
                return true;
            default:
                return false;
        }
    }

    bool WidenWeaponRequirement(uint32_t family, WeaponReq& req)
    {
        if (!IsClassSpellFamily(family) || req.itemClass != ItemClassWeapon)
            return false;
        bool anySubclass = req.subClassMask == 0 || req.subClassMask == -1;
        if (!anySubclass && (req.subClassMask & MeleeWeaponMask) == 0)
            return false;                                   // ranged only (Hunter shots, wands)
        WeaponReq before = req;
        if (!anySubclass)
            req.subClassMask |= MeleeWeaponMask;
        req.invTypeMask = 0;
        return req.subClassMask != before.subClassMask || req.invTypeMask != before.invTypeMask;
    }
}
