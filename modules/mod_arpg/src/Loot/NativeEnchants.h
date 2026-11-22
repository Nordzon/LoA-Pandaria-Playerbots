// Index of the client's fixed-amount "+N <stat>" enchantments, built at startup (m2 4.1b, design.md "native display").
#ifndef ARPG_LOOT_NATIVEENCHANTS_H
#define ARPG_LOOT_NATIVEENCHANTS_H

#include "Core/EnchantIndex.h"

#include <string>

namespace Arpg::Loot
{
    // Scans sSpellItemEnchantmentStore once (WorldScript::OnStartup): single-effect ITEM_ENCHANTMENT_TYPE_STAT rows with a fixed
    // amount, no scaling, no gem, no condition, no skill requirement.
    void BuildNativeEnchants();
    Core::EnchantIndex const& NativeEnchants();
    // ".arpg loot enchants": count / min / max per affix stat.
    std::string DescribeNativeEnchants();
}

#endif
