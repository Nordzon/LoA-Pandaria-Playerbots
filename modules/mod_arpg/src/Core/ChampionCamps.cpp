#include "ChampionCamps.h"

#include <cmath>

namespace Arpg::Core
{
    uint64_t CampKey(uint32_t mapId, float x, float y, float size)
    {
        if (!(size > 1.0f))
            size = 1.0f;
        // Squares are counted from -2^23 so negative coordinates get their own squares; 24 bits each, map on top.
        int64_t const cx = int64_t(std::floor(x / size)) + (int64_t(1) << 23);
        int64_t const cy = int64_t(std::floor(y / size)) + (int64_t(1) << 23);
        return (uint64_t(mapId & 0xFFFF) << 48) | ((uint64_t(cx) & 0xFFFFFF) << 24) | (uint64_t(cy) & 0xFFFFFF);
    }

    bool CampWantsChampion(bool hasLiveChampion, uint32_t nowS, uint32_t readyAtS, int eligible, int minEligible)
    {
        return !hasLiveChampion && nowS >= readyAtS && eligible >= (minEligible < 1 ? 1 : minEligible);
    }
}
