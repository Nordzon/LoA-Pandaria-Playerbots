// Any weapon for class abilities (owner 2026-11-27, change m6-any-weapon-skills): class spells that need a melee weapon
// accept any melee weapon in any hand; Arpg.AnyWeapon.MainHandSpells work without an off-hand weapon. Shield, armour and
// ranged-only requirements stay. The client patch carries the same edits (build-client-patch.py).
#ifndef ARPG_CHARACTER_ANYWEAPONSKILLS_H
#define ARPG_CHARACTER_ANYWEAPONSKILLS_H

#include <cstdint>
#include <string>

class Player;

namespace Arpg::Character
{
    // Startup and .arpg reload: restores every spell it changed before, then widens again if Arpg.AnyWeapon.Skills = 1.
    // Returns "weapon skills: widened=<n> mainhand=<m>".
    std::string ApplyAnyWeaponSkills();
    // For .arpg test spellreq: "class= sub= inv= offhand= [item= fits=]" (fits = the item would satisfy the spell in the
    // main hand, as Item::IsFitToSpellRequirements checks it).
    std::string DescribeSpellRequirement(uint32_t spellId, uint32_t itemId, bool& ok);
}

#endif
