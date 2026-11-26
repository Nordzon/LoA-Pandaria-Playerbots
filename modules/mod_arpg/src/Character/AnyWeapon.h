// Any weapon (SPEC 4.9, change m6-any-weapon): every real player knows every weapon proficiency and Shield, so any class
// can equip any weapon type or a shield. Granted at login and on level change (Arpg.AnyWeapon.*).
#ifndef ARPG_CHARACTER_ANYWEAPON_H
#define ARPG_CHARACTER_ANYWEAPON_H

#include <string>

class Player;

namespace Arpg::Character
{
    void InstallAnyWeapon();
    // Grants what is missing (if the feature applies to this player); returns how many spells/skills it added.
    int ApplyAnyWeapon(Player* player);
    // "proficiencies=<known>/<total> dualwield=<0/1>" for .arpg test weapons.
    std::string DescribeAnyWeapon(Player* player);
}

#endif
