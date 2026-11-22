// B = the epic stat budget for an item's slot at a player level (SPEC 4.1, m2 task 4.2). Flat stat affixes are a share of B.
#ifndef ARPG_LOOT_BUDGET_H
#define ARPG_LOOT_BUDGET_H

#include "Define.h"

struct ItemTemplate;

namespace Arpg::Loot
{
    struct BudgetInfo
    {
        uint32 itemLevel = 0;   // epic item level for the player level (Arpg.Loot.EpicIlvlCurve)
        int32 slotOffset = -1;  // RandPropPoints column for the slot (core: ItemTemplate::GetRandomPointsOffset), -1 = none
        uint32 budget = 0;      // B; 0 when the item has no stat slot (bags, quest items...)
    };

    BudgetInfo BudgetB(ItemTemplate const* proto, uint8 playerLevel);
}

#endif
