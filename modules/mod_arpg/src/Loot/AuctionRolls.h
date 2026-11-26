// Living auction house (m7, owner 2026-11-26): the core's AuctionHouseBot seller lists materials and gear; through the hook
// ArpgHooks::AuctionBotItem (// ARPG: in AuctionHouseBotSeller.cpp) Arpg.AH.AffixShare % of its green-epic gear listings roll
// affixes at the item's level tier + Arpg.AH.TierOffset, priced by the sell formula x Arpg.AH.AffixMarkup.
#ifndef ARPG_LOOT_AUCTIONROLLS_H
#define ARPG_LOOT_AUCTIONROLLS_H

#include "Define.h"

#include <string>

class Item;

namespace Arpg::Loot
{
    void InstallAuctionRolls();
    // What the hook does to one new listing; `force` ignores the share. Returns the affix count (0 = left plain).
    int RollAuctionListing(Item* item, uint32& buyout, uint32& bid, bool force, std::string& why);
}

#endif
