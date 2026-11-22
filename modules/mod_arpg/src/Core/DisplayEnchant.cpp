#include "DisplayEnchant.h"

namespace Arpg::Core
{
    uint32_t DisplayEnchantId(DisplayRule const& rule, DisplayStat stat, int32_t value)
    {
        int s = int(stat);
        if (s < 0 || s >= int(DisplayStat::Count) || value < 1 || rule.stride < 2 || uint32_t(value) >= rule.stride)
            return 0;
        return rule.base + uint32_t(s) * rule.stride + uint32_t(value);
    }

    uint32_t EffectDisplayId(uint32_t effectBase, int effectIndex, int32_t valueHundredths, bool isFlag)
    {
        if (effectIndex < 1 || effectIndex > 999)
            return 0;
        if (isFlag)
            return effectBase + uint32_t(effectIndex) * 1000u;
        int32_t tenths = (valueHundredths + 5) / 10;
        if (valueHundredths <= 0 || tenths < 1 || tenths > 999)
            return 0;
        return effectBase + uint32_t(effectIndex) * 1000u + uint32_t(tenths);
    }

    int32_t RoundToTenths(int32_t valueHundredths)
    {
        return valueHundredths <= 0 ? valueHundredths : ((valueHundredths + 5) / 10) * 10;
    }

    int DisplayStatFromItemMod(uint32_t itemMod)
    {
        // ITEM_MOD_* ids of the 5.4.8 core (ItemPrototype.h): 3 agi, 4 str, 5 int, 6 spi, 7 sta, 13 dodge, 14 parry,
        // 31 hit, 32 crit, 36 haste, 37 expertise, 49 mastery.
        switch (itemMod)
        {
            case 4:  return int(DisplayStat::Strength);
            case 3:  return int(DisplayStat::Agility);
            case 5:  return int(DisplayStat::Intellect);
            case 6:  return int(DisplayStat::Spirit);
            case 7:  return int(DisplayStat::Stamina);
            case 32: return int(DisplayStat::Crit);
            case 36: return int(DisplayStat::Haste);
            case 31: return int(DisplayStat::Hit);
            case 37: return int(DisplayStat::Expertise);
            case 49: return int(DisplayStat::Mastery);
            case 13: return int(DisplayStat::Dodge);
            case 14: return int(DisplayStat::Parry);
            default: return -1;
        }
    }
}
