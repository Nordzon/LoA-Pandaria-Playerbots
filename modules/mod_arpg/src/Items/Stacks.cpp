#include "Items/Stacks.h"

#include "ArpgConfig.h"

#include "ObjectMgr.h"

#include <unordered_map>

namespace Arpg::Items
{
    namespace
    {
        std::unordered_map<uint32, int32> g_stock;   // entry -> stock Stackable, for every item we changed
    }

    std::string ApplyStacks()
    {
        int const max = sArpgConfig.Get().m9.stackMax;
        uint32 raised = 0;
        // The store is ObjectMgr's own map; only Stackable changes, and only at startup / ".arpg reload".
        ItemTemplateContainer& store = const_cast<ItemTemplateContainer&>(*sObjectMgr->GetItemTemplateStore());
        for (auto& kv : store)
        {
            ItemTemplate& proto = kv.second;
            auto it = g_stock.find(kv.first);
            int32 const stock = it != g_stock.end() ? it->second : proto.Stackable;
            if (stock > 1 && max > 1 && stock < max)
            {
                g_stock.emplace(kv.first, stock);
                proto.Stackable = max;
                ++raised;
            }
            else if (it != g_stock.end())
                proto.Stackable = stock;
        }
        return "stacks max=" + std::to_string(max) + " raised=" + std::to_string(raised);
    }

    std::string DescribeStack(uint32 itemId)
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            return "stack item=" + std::to_string(itemId) + " max=0 stock=0";
        auto it = g_stock.find(itemId);
        return "stack item=" + std::to_string(itemId) + " max=" + std::to_string(proto->Stackable) + " stock=" +
            std::to_string(it != g_stock.end() ? it->second : proto->Stackable);
    }
}
