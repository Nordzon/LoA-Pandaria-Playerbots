// The companion's mount shop (owner 2026-11-26, a gold sink): reputation mounts for gold (arpg_mount_shop; price
// Arpg.MountShop.Price unless a row sets its own). Buying teaches the mount spell; no reputation needed.
#ifndef ARPG_LOOT_MOUNTSHOP_H
#define ARPG_LOOT_MOUNTSHOP_H

#include "Define.h"

#include <string>
#include <vector>

class Player;

namespace Arpg::Loot
{
    struct ShopMount
    {
        uint32 spellId = 0;
        uint32 itemId = 0;
        std::string name;
        uint32 priceGold = 0;   // final price (row or config)
    };

    // Startup / .arpg reload. One-line summary.
    std::string LoadMountShop();
    // Mounts this character doesn't know yet, in shop order.
    std::vector<ShopMount> MountsForSale(Player* player);
    // Buys and teaches one. False (and why in `out`) if unknown, already known, or not enough gold.
    bool BuyMount(Player* player, uint32 spellId, std::string& out);
}

#endif
