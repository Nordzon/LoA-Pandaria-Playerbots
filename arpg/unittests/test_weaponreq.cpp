// Class spells and weapons (change m6-any-weapon-skills): any melee weapon in any hand; shields, armour, ranged kept.
#include "TestHarness.h"
#include "WeaponReq.h"

using namespace Arpg::Core;

namespace
{
    int32_t const Dagger = 1 << 15;
    int32_t const OneHanders = (1 << 0) | (1 << 4) | (1 << 7) | (1 << 13) | (1 << 15);
    int32_t const TwoHanders = (1 << 1) | (1 << 5) | (1 << 6) | (1 << 8);
    int32_t const Ranged = (1 << 2) | (1 << 3) | (1 << 18);
}

TEST(weaponreq_dagger_spell_takes_any_melee_weapon)
{
    WeaponReq r{ 2, Dagger, 0 };                      // Backstab
    CHECK(WidenWeaponRequirement(8, r));
    CHECK_EQ(r.subClassMask, MeleeWeaponMask);
    CHECK_EQ(r.invTypeMask, 0);
}

TEST(weaponreq_one_and_two_hand_and_offhand_limits_removed)
{
    WeaponReq poison{ 2, OneHanders, 0 };             // poisons, Shiv
    CHECK(WidenWeaponRequirement(8, poison));
    CHECK_EQ(poison.subClassMask, MeleeWeaponMask);
    WeaponReq soldier{ 2, TwoHanders, 0 };            // Seasoned Soldier
    CHECK(WidenWeaponRequirement(4, soldier));
    CHECK_EQ(soldier.subClassMask, MeleeWeaponMask);
    WeaponReq lavaLash{ 2, MeleeWeaponMask, 1 << 22 }; // off hand only
    CHECK(WidenWeaponRequirement(11, lavaLash));
    CHECK_EQ(lavaLash.invTypeMask, 0);
    WeaponReq smf{ 2, OneHanders, (1 << 13) | (1 << 21) | (1 << 22) }; // Single-Minded Fury
    CHECK(WidenWeaponRequirement(4, smf));
    CHECK_EQ(smf.subClassMask, MeleeWeaponMask);
    CHECK_EQ(smf.invTypeMask, 0);
}

TEST(weaponreq_any_melee_already_is_unchanged)
{
    WeaponReq r{ 2, MeleeWeaponMask, 0 };             // most Warrior/Rogue/DK strikes
    CHECK(!WidenWeaponRequirement(4, r));
    WeaponReq fishingToo{ 2, MeleeWeaponMask | (1 << 20), 0 };
    CHECK(!WidenWeaponRequirement(11, fishingToo));
    CHECK_EQ(fishingToo.subClassMask, MeleeWeaponMask | (1 << 20));
}

TEST(weaponreq_ranged_shield_and_non_class_kept)
{
    WeaponReq shot{ 2, Ranged, 0 };                   // Hunter shots
    CHECK(!WidenWeaponRequirement(9, shot));
    CHECK_EQ(shot.subClassMask, Ranged);
    WeaponReq shield{ 4, 1 << 6, 0 };                 // Shield Slam
    CHECK(!WidenWeaponRequirement(4, shield));
    CHECK_EQ(shield.subClassMask, 1 << 6);
    WeaponReq npc{ 2, Dagger, 0 };                    // family 0: creature / item spells
    CHECK(!WidenWeaponRequirement(0, npc));
    CHECK_EQ(npc.subClassMask, Dagger);
    WeaponReq none{ -1, 0, 0 };
    CHECK(!WidenWeaponRequirement(8, none));
}

TEST(weaponreq_any_subclass_keeps_mask_but_drops_hand_limit)
{
    WeaponReq r{ 2, 0, 1 << 17 };                     // "a two-handed weapon"
    CHECK(WidenWeaponRequirement(15, r));
    CHECK_EQ(r.subClassMask, 0);
    CHECK_EQ(r.invTypeMask, 0);
    WeaponReq mixed{ 2, MeleeWeaponMask | Ranged, 0 }; // melee or ranged: nothing to widen
    CHECK(!WidenWeaponRequirement(8, mixed));
}

TEST(weaponreq_class_families)
{
    CHECK(IsClassSpellFamily(4));
    CHECK(IsClassSpellFamily(53));
    CHECK(IsClassSpellFamily(15));
    CHECK(!IsClassSpellFamily(0));
    CHECK(!IsClassSpellFamily(13));   // items
}
