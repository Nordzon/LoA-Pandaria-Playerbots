#include "Loot/Budget.h"

#include "ArpgConfig.h"
#include "DBCStores.h"
#include "ObjectMgr.h"   // includes ItemPrototype.h correctly (it isn't self-contained: needs <bitset>, DBC structs)

namespace Arpg::Loot
{
    BudgetInfo BudgetB(ItemTemplate const* proto, uint8 playerLevel)
    {
        BudgetInfo info;
        if (!proto)
            return info;
        Arpg::Settings s = sArpgConfig.Get();
        info.itemLevel = uint32(std::max(1, s.epicIlvlCurve.ValueAt(int(playerLevel))));
        info.slotOffset = proto->GetRandomPointsOffset();
        if (info.slotOffset < 0 || info.slotOffset > 4)
            return info;
        if (RandomPropertiesPointsEntry const* pts = sRandomPropertiesPointsStore.LookupEntry(info.itemLevel))
            info.budget = pts->EpicPropertiesPoints[info.slotOffset];
        return info;
    }
}
