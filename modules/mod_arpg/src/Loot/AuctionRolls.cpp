#include "Loot/AuctionRolls.h"

#include "ArpgConfig.h"
#include "Core/AffixValue.h"
#include "Loot/AffixRoller.h"

#include "ArpgHooks.h"
#include "Item.h"
#include "ItemPrototype.h"
#include "Random.h"

#include <algorithm>

namespace
{
    void AuctionBotItemHook(Item* item, uint32& buyout, uint32& bid)
    {
        std::string why;
        Arpg::Loot::RollAuctionListing(item, buyout, bid, false, why);
    }
}

namespace Arpg::Loot
{
    int RollAuctionListing(Item* item, uint32& buyout, uint32& bid, bool force, std::string& why)
    {
        Arpg::Settings s = sArpgConfig.Get();
        ItemTemplate const* proto = item ? item->GetTemplate() : nullptr;
        if (!proto || !s.lootEnabled) { why = "off"; return 0; }
        if (proto->Class != ITEM_CLASS_WEAPON && proto->Class != ITEM_CLASS_ARMOR) { why = "not gear"; return 0; }
        if (!force && float(rand_chance()) >= s.ahAffixSharePct) { why = "share"; return 0; }
        int level = proto->RequiredLevel ? int(proto->RequiredLevel) : 1;
        int n = RollForAuction(item, level, s.ahTierOffset, why);
        if (n <= 0)
            return 0;
        uint64 sell = Core::SellValue(uint64(proto->SellPrice), n, level, s.sellCopperPerAffixLevel);
        uint64 wanted = uint64(double(sell) * s.ahAffixMarkup);
        uint32 newBuyout = uint32(std::min<uint64>(std::max<uint64>(wanted, buyout), 0xFFFFFFFFull));
        if (buyout && bid)
            bid = uint32(std::min<uint64>(uint64(bid) * newBuyout / buyout, newBuyout));
        buyout = newBuyout;
        return n;
    }

    void InstallAuctionRolls()
    {
        ArpgHooks::AuctionBotItem = &AuctionBotItemHook;
    }
}
