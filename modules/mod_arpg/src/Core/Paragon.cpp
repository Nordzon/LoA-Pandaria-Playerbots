#include "Paragon.h"

#include <algorithm>
#include <cmath>

namespace Arpg::Core
{
    uint64_t ParagonXpToNext(ParagonCurve const& c, int level)
    {
        double v = std::max(1.0, c.base) * std::pow(std::max(1.0, c.growth), double(std::max(0, level)));
        return v >= 1.8e19 ? UINT64_MAX : std::max<uint64_t>(1, uint64_t(v));
    }

    int AddParagonXp(ParagonCurve const& c, int& level, uint64_t& xpIntoLevel, uint64_t gained)
    {
        int levels = 0;
        uint64_t xp = xpIntoLevel + gained;
        if (xp < xpIntoLevel)
            xp = UINT64_MAX;   // overflow guard
        for (uint64_t need = ParagonXpToNext(c, level); xp >= need && levels < 10000; need = ParagonXpToNext(c, level))
        {
            xp -= need;
            ++level;
            ++levels;
        }
        xpIntoLevel = xp;
        return levels;
    }

    ParagonTab TabForLevel(int paragonLevel)
    {
        return ParagonTab(((std::max(1, paragonLevel) - 1) % 4));
    }

    int PointsInTab(int paragonLevel, ParagonTab tab)
    {
        if (paragonLevel <= 0)
            return 0;
        int t = int(tab);
        int full = paragonLevel / 4;
        return full + ((paragonLevel % 4) > t ? 1 : 0);
    }

    int ParagonBonus(ParagonBonusRule const& rule, int points)
    {
        int v = rule.perPoint * std::max(0, points);
        return rule.cap > 0 ? std::min(v, rule.cap) : v;
    }

    float WorldTierMultiplier(int paragonLevel, float pctPerLevel, float capPct)
    {
        float pct = std::max(0.0f, pctPerLevel) * float(std::max(0, paragonLevel));
        if (capPct > 0.0f)
            pct = std::min(pct, capPct);
        return 1.0f + pct / 100.0f;
    }
}
