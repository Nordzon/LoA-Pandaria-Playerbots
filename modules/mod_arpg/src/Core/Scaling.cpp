#include "Scaling.h"

#include <algorithm>

namespace Arpg::Core
{
    uint8_t ScaledLevel(ScaleSettings const& s, int creatureLevel, int playerLevel)
    {
        if (!s.enabled || playerLevel <= 0)
            return 0;
        int cap = std::max(1, std::min(s.maxLevel, 255));
        int target = std::clamp(playerLevel + s.levelOffset, 1, cap);
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
