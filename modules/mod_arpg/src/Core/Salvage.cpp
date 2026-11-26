#include "Salvage.h"

namespace Arpg::Core
{
    bool CanSalvage(SalvageItem const& item, SalvageRules const& rules)
    {
        if (item.equipped || item.protectedByFilter)
            return false;
        if (item.unique)
            return rules.includeUniques;
        if (item.affixCount <= 0)
            return false;
        return item.quality <= rules.maxQuality;
    }

    int EssencesFor(SalvageItem const& item, SalvageRules const& rules)
    {
        if (!CanSalvage(item, rules))
            return 0;
        int n = item.affixCount * rules.essencePerAffix;
        if (item.unique)
            n += rules.uniqueBonus;
        if (item.ascended)
            n += rules.ascendedBonus;
        return n > 0 ? n : 0;
    }

    int CatalystBandForTier(int tier)
    {
        if (tier < 1)
            return 0;
        if (tier <= 2)
            return 1;
        if (tier <= 4)
            return 2;
        return 3;
    }

    int CatalystBandMaxTier(int band)
    {
        switch (band)
        {
            case 1: return 2;
            case 2: return 4;
            case 3: return 99;
            default: return 0;
        }
    }
}
