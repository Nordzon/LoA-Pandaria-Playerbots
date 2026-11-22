// Level-appropriate loot with a zone ceiling (SPEC 4.1, change m2-affix-loot task 4.8): when creature/chest loot is filled,
// a green/blue/epic weapon or armour piece far below the loot level is swapped for the closest same-slot, same-type,
// same-quality item that really drops somewhere (appears in a loot table). The loot window shows the swapped item.
#ifndef ARPG_LOOT_SUBSTITUTION_H
#define ARPG_LOOT_SUBSTITUTION_H

#include "Define.h"

#include <string>

class Player;

namespace Arpg::Loot
{
    // Startup (world DB loaded): builds the candidate index. Returns a one-line summary.
    std::string LoadSubstitutionIndex();
    // Sets ArpgHooks::LootFilled.
    void InstallSubstitution();

    // ".arpg loot substitute <char> <itemId>": what the item would become for this character here (0 = kept), and why.
    uint32 SubstituteFor(Player* player, uint32 itemId, std::string& why);

    // A real, droppable base item for a Unique: given InventoryType (robes count as chest), armour subclass (0 = any) and
    // quality, the candidate whose required level is closest to (not above) `level`. 0 if none.
    uint32 FindBaseItem(uint32 inventoryType, uint32 armorSubclass, uint32 quality, int level);
}

#endif
