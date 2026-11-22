// Pure: client-side display rows for affixes (SPEC 5, change m2-affix-loot task 4.7). No game headers - unit-tested.
// Our client patch (arpg/client-patch/build-client-patch.py) adds one SpellItemEnchantment row per (stat, value):
//     id = base + statIndex * stride + value        text "+$k1 <Stat>", amount = value
// The server never loads these rows: it only writes the id into an item's display slot (9-12); the stats themselves are
// applied by Loot/AffixStats. Both sides compute ids from the same rule and the same Arpg.Display.* numbers.
#ifndef ARPG_CORE_DISPLAYENCHANT_H
#define ARPG_CORE_DISPLAYENCHANT_H

#include <cstdint>

namespace Arpg::Core
{
    // Display stat order: must match STATS in arpg/client-patch/build-client-patch.py.
    enum class DisplayStat : int
    {
        Strength = 0, Agility, Intellect, Spirit, Stamina, Crit, Haste, Hit, Expertise, Mastery, Dodge, Parry,
        Count
    };

    struct DisplayRule
    {
        uint32_t base = 60000;      // Arpg.Display.EnchantBase (first id of our range; Blizzard's max here is 5267)
        uint32_t stride = 2000;     // Arpg.Display.EnchantStride (ids per stat; values 1..stride-1 have a row)
    };

    // Row id for "+value <stat>", or 0 if there is no row (value < 1, value >= stride, bad stat).
    uint32_t DisplayEnchantId(DisplayRule const& rule, DisplayStat stat, int32_t value);

    // Effect affix rows: id = effectBase + effectIndex * 1000 + value in tenths of a percent (1..999); 0 = no row.
    // effectIndex = Core::Effect value (1-based). Flag effects (no value) use tenths = 0: pass valueHundredths = 0 and
    // isFlag = true.
    uint32_t EffectDisplayId(uint32_t effectBase, int effectIndex, int32_t valueHundredths, bool isFlag = false);

    // Effect values roll in hundredths of a percent; displayed (and therefore kept) in tenths: 1537 -> 1540.
    int32_t RoundToTenths(int32_t valueHundredths);

    // Core ITEM_MOD_* stat id -> display stat; -1 if the stat has no display rows.
    int DisplayStatFromItemMod(uint32_t itemMod);
}

#endif
