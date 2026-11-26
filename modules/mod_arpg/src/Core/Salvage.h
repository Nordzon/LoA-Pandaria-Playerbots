// Salvage and catalysts (change m6-custom-items, SPEC 4.8): pure rules, no game headers.
#ifndef ARPG_CORE_SALVAGE_H
#define ARPG_CORE_SALVAGE_H

namespace Arpg::Core
{
    struct SalvageRules
    {
        int essencePerAffix = 1;     // Arpg.Salvage.EssencePerAffix
        int uniqueBonus = 5;         // Arpg.Salvage.UniqueBonus (a Unique's power counts extra)
        int ascendedBonus = 3;       // Arpg.Salvage.AscendedBonus (per item with any Ascended affix, T6+)
        int maxQuality = 3;          // Arpg.Salvage.MaxQuality (3 = up to blue; epics are kept)
        bool includeUniques = false; // Arpg.Salvage.IncludeUniques (Uniques are kept unless this is on)
    };

    struct SalvageItem
    {
        int quality = 0;             // 0 grey .. 4 epic
        int affixCount = 0;
        bool unique = false;
        bool ascended = false;
        bool equipped = false;
        bool protectedByFilter = false;
    };

    // Whether the companion's Salvage takes this item (affixed, not equipped, not protected, quality <= max).
    bool CanSalvage(SalvageItem const& item, SalvageRules const& rules);
    // Essences for one salvaged item (0 if it can't be salvaged).
    int EssencesFor(SalvageItem const& item, SalvageRules const& rules);

    // Catalyst band for a monster's affix tier: 1 = Lesser (T1-T2), 2 = Greater (T3-T4), 3 = Grand (T5+). 0 for tier < 1.
    int CatalystBandForTier(int tier);
    // The highest tier a band's catalyst works on (Lesser 2, Greater 4, Grand: no limit -> 99).
    int CatalystBandMaxTier(int band);
}

#endif
