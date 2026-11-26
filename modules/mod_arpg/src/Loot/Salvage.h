// Salvage and the catalyst shop (change m6-custom-items tasks 3.2/3.3, SPEC 4.8): the companion's dialogue turns affixed
// items in the bags (not equipped; greens/blues; Uniques only with Arpg.Salvage.IncludeUniques) into Salvaged Essence
// (700010), and trades essences for catalysts (Arpg.Catalyst.EssenceCost*).
#ifndef ARPG_LOOT_SALVAGE_H
#define ARPG_LOOT_SALVAGE_H

#include "Define.h"

#include <string>

class Player;

namespace Arpg::Loot
{
    struct SalvagePreview
    {
        int items = 0;
        int essences = 0;
    };

    // What "Salvage" would take now.
    SalvagePreview PreviewSalvage(Player* player);
    // Destroys those items, adds the essences. `out` = one-line summary.
    SalvagePreview DoSalvage(Player* player, std::string& out);
    // Essence price of a catalyst band (1-3), from config.
    int CatalystEssenceCost(int band);
    // Buys one catalyst of the band with essences. False (and why in `out`) if not enough essences or no room.
    bool BuyCatalyst(Player* player, int band, std::string& out);
}

#endif
