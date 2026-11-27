// Typed blank bases (change m6-blank-bases, BALANCE 8c, owner 2026-11-27 "all of these"): copies of real gear (ids 710000+,
// sql 2026_11_27_03 by gen_blank_bases.py) keeping only main stat + Stamina. (a) a share of gear drops comes as its blank,
// (b) crafted gear comes out as its blank (ArpgHooks::CraftedItem), (c) the companion sells blanks for essences. A dropped or
// bought blank rolls the most affixes for its quality (AffixRoller PlanRoll).
#ifndef ARPG_LOOT_BLANKS_H
#define ARPG_LOOT_BLANKS_H

#include "Define.h"

#include <string>
#include <vector>

class Player;
struct Loot;

namespace Arpg::Loot
{
    // Startup and .arpg reload: reads arpg_blank. Returns a one-line summary.
    std::string LoadBlanks();
    // Sets ArpgHooks::CraftedItem.
    void InstallBlanks();

    bool IsBlank(uint32 itemId);
    // The blank for a slot (InventoryType; robe and chest are one slot), item class/subclass and quality at `level`, with
    // the main stat `mainStat` when that variant exists. 0 = none.
    uint32 BlankFor(uint32 inventoryType, uint32 itemClass, uint32 subclass, uint32 quality, int level, uint32 mainStat);
    // The blank `player` would get for this item (its loot level, its spec's main stat). 0 = none.
    uint32 BlankForItem(Player* player, uint32 itemId);

    // (a) Loot filled for `owner`: each affix-eligible gear item becomes its blank with Arpg.Blank.DropChance.
    void MaybeBlankLoot(Player* owner, ::Loot& loot);

    // (c) The companion's "Blank bases" shop.
    struct BlankShopSlot
    {
        char const* name;
        uint32 inventoryType;
        uint32 itemClass;
        int subclass;          // -1 = the armour type the class wears at its level
    };
    std::vector<BlankShopSlot> const& BlankShopSlots();
    int BlankEssenceCost(uint32 quality);
    // Buys the blank of shop slot `slot` and `quality` (2-4) for essences; it rolls at once. False (why in `out`) if it fails.
    bool BuyBlank(Player* player, int slot, uint32 quality, std::string& out);
}

#endif
