#include "Potions.h"

#include <algorithm>

namespace Arpg::Core
{
    uint32_t PotionHeal(uint32_t maxHealth, int pct)
    {
        uint64_t v = uint64_t(maxHealth) * uint64_t(std::max(0, pct)) / 100;
        return uint32_t(std::max<uint64_t>(1, v));
    }

    uint32_t PotionResource(Resource r, uint32_t maxValue, int pct)
    {
        if (!maxValue || pct <= 0)
            return 0;
        int p = pct;
        if (r == Resource::Energy || r == Resource::Focus)
            p = pct / 2;
        else if (r == Resource::Other)
            return 0;
        uint64_t v = uint64_t(maxValue) * uint64_t(p) / 100;
        return uint32_t(std::max<uint64_t>(1, v));
    }

    uint32_t PotionForTier(std::vector<PotionDef> const& potions, PotionKind kind, int dropTier)
    {
        uint32_t best = 0;
        int bestTier = 0;
        for (PotionDef const& p : potions)
            if (p.kind == kind && p.tier <= dropTier && p.tier > bestTier)
            {
                best = p.itemId;
                bestTier = p.tier;
            }
        return best;
    }

    PotionKind PickPotionKind(uint32_t wHeal, uint32_t wMana, uint32_t wRejuv, uint32_t roll)
    {
        if (roll < wHeal)
            return PotionKind::Healing;
        if (roll < wHeal + wMana)
            return PotionKind::Mana;
        return wRejuv ? PotionKind::Rejuvenation : PotionKind::Healing;
    }
}
