// Uniques (BALANCE §8b, SPEC 4.4, change m3 tasks 2.3/3.6): a Unique is a named power + up to 4 random affixes on a dropped
// base of the Unique's slot (D2-style). The power rolls a tier and two numbers (Core/UniquePower); it shows as the 5th
// tooltip line (client patch row per power, tier and roll step) and the item is named via a random-suffix row.
#ifndef ARPG_LOOT_UNIQUES_H
#define ARPG_LOOT_UNIQUES_H

#include "Core/UniquePower.h"

#include "Define.h"

#include <string>

class Item;
class Player;

namespace Arpg::Loot
{
    struct UniqueRoll
    {
        uint32 uniqueId = 0;
        uint32 powerId = 0;
        int tier = 0;
        Core::RolledPower power;
    };

    // Startup / .arpg reload: arpg_unique_power, arpg_unique_power_tier, arpg_unique + every Unique item instance.
    std::string LoadUniques();

    // Turns `item` (already in the player's bags, affixes rolled with at most 4) into Unique `uniqueId` at `tier`:
    // rolls the power, stores it, sets the power line (slot 12) and the Unique's name. False + message on error.
    bool MakeUnique(Player* player, Item* item, uint32 uniqueId, int tier, std::string& message);

    // The item's Unique roll (uniqueId 0 = not a Unique). Thread-safe.
    UniqueRoll UniqueOf(uint32 itemGuid);

    // An equipped Unique's power with everything the combat code needs.
    struct ActivePower
    {
        uint32 itemGuid = 0;
        uint32 powerId = 0;
        std::string name;
        Core::PowerTrigger trigger = Core::PowerTrigger::ChanceOnHit;
        Core::PowerKind kind = Core::PowerKind::Damage;
        int triggerValue = 0;          // chance % or N
        int magnitude = 0;             // % of Power / % of max health / flat value
        uint32 cooldownMs = 0;
        float radius = 0.0f;
        uint32 maxTargets = 1;
        uint32 schoolMask = 1;
        uint32 spellId = 0;
    };
    // False if the item is not a Unique.
    bool PowerFor(uint32 itemGuid, ActivePower& out);

    // One-line description for .arpg loot last / .arpg test unique.
    std::string DescribeUnique(UniqueRoll const& u);

    // A random Unique that can sit on this base (same InventoryType - robes count as chest - and armour subclass rule);
    // 0 if none.
    uint32 UniqueForBase(uint32 inventoryType, uint32 itemClass, uint32 itemSubClass);

    // A random Unique id (0 if none loaded) - Superunique drops until each has its own (m3).
    uint32 RandomUniqueId();

    // Slot of a Unique (core InventoryType, armour subclass 0 = any); false if unknown.
    bool UniqueSlot(uint32 uniqueId, uint32& inventoryType, uint32& armorSubclass);
}

#endif
