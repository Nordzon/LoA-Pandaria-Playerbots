// Bigger stacks (change m9-stacks-trainer, owner 2026-11-28 "any stackable item actually go to 999"): every item whose stock
// stack size is 2 .. Arpg.Stacks.Max-1 stacks to Arpg.Stacks.Max in memory; the client learns the new size from the
// Item-sparse hotfix rows (sql/world 2026_11_28_02_arpg_stacks.sql), which the server answers from the in-memory template.
#ifndef ARPG_ITEMS_STACKS_H
#define ARPG_ITEMS_STACKS_H

#include "Define.h"

#include <string>

namespace Arpg::Items
{
    // Applies Arpg.Stacks.Max to the item templates (0 restores stock sizes). Returns "stacks max=.. raised=..".
    std::string ApplyStacks();
    // "stack item=.. max=.. stock=.." (0 when the item does not exist).
    std::string DescribeStack(uint32 itemId);
}

#endif
