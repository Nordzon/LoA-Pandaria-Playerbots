// Affix display repair (2026-11-27): the core used to clear two enchant slots of a weapon leaving the main hand ("held
// enchantments", Player::RemoveItem; now disabled there with an ARPG: guard). Our affix and Unique lines live in those
// slots (display rows only - stats come from item_instance_arpg_affix), so some items lost tooltip lines. At login every
// item with affix rows gets its lines rewritten from the tables, the same order AffixRoller wrote them.
#ifndef ARPG_LOOT_AFFIXDISPLAYREPAIR_H
#define ARPG_LOOT_AFFIXDISPLAYREPAIR_H

class Player;

namespace Arpg::Loot
{
    void InstallAffixDisplayRepair();
    // Rewrites missing/wrong affix and Unique lines on all of the player's items. Returns how many items were fixed.
    int RepairAffixDisplay(Player* player);
}

#endif
