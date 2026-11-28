#include "Scaling.h"

#include <algorithm>

namespace Arpg::Core
{
    int LevelBelow(ScaleSettings const& s, uint32_t seed)
    {
        int lo = std::max(0, s.levelBelowMin);
        int hi = std::max(lo, s.levelBelowMax);
        return lo + int(seed % uint32_t(hi - lo + 1));
    }

    uint8_t ScaledLevel(ScaleSettings const& s, int creatureLevel, int playerLevel, uint32_t seed)
    {
        if (!s.enabled || playerLevel <= 0)
            return 0;
        int cap = std::max(1, std::min(s.maxLevel, 255));
        int target = std::clamp(playerLevel + s.levelOffset - LevelBelow(s, seed), 1, cap);
        if (creatureLevel >= target)
            return 0;
        return uint8_t(target);
    }

    float HealthMultiplier(ScaleSettings const& s, bool scaled)
    {
        return (scaled && s.healthMod > 0.0f) ? s.healthMod : 1.0f;
    }

    float DamageMultiplier(ScaleSettings const& s, bool scaled)
    {
        return (scaled && s.damageMod > 0.0f) ? s.damageMod : 1.0f;
    }
}
