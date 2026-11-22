#include "AffixValue.h"

#include <algorithm>
#include <cmath>

namespace Arpg::Core
{
    namespace
    {
        double Lerp(TierRange const& r, double roll01)
        {
            double t = std::clamp(roll01, 0.0, 1.0);
            double lo = std::min(r.min, r.max), hi = std::max(r.min, r.max);
            return lo + (hi - lo) * t;
        }
    }

    uint32_t FlatValue(uint32_t budgetB, TierRange const& range, double roll01)
    {
        double v = double(budgetB) * Lerp(range, roll01) / 100.0;
        return uint32_t(std::max(1.0, std::round(v)));
    }

    int32_t PercentValue(TierRange const& range, double roll01)
    {
        return int32_t(std::lround(Lerp(range, roll01)));
    }

    uint64_t SellValue(uint64_t templatePriceCopper, int affixCount, int playerLevel, uint32_t copperPerAffixLevel)
    {
        if (affixCount <= 0 || playerLevel <= 0)
            return templatePriceCopper;
        return templatePriceCopper + uint64_t(affixCount) * uint64_t(playerLevel) * uint64_t(copperPerAffixLevel);
    }

    TierRange AscendedRange(TierRange const& t5, int pct)
    {
        float f = float(pct) / 100.0f;
        return { t5.min * f, t5.max * f };
    }
}
