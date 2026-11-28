// Champion camps (change m9-champion-camps, owner 2026-11-28 "each spawn location with mobs gets a champion"): the open world
// is cut into squares of Arpg.Champion.CampSize yards; a square with at least Arpg.Champion.CampMin eligible monsters holds
// one Champion, and gets a new one Arpg.Champion.CampRespawn seconds after the last one is gone. Pure logic, unit-tested.
#ifndef ARPG_CORE_CHAMPIONCAMPS_H
#define ARPG_CORE_CHAMPIONCAMPS_H

#include <cstdint>

namespace Arpg::Core
{
    // Square of a position: same map + same square -> same key. size <= 0 is treated as 1.
    uint64_t CampKey(uint32_t mapId, float x, float y, float size);

    // Whether a square should get a Champion now.
    bool CampWantsChampion(bool hasLiveChampion, uint32_t nowS, uint32_t readyAtS, int eligible, int minEligible);
}

#endif
