// Pure: level-appropriate loot with a zone ceiling (SPEC 4.1, owner 2026-11-22, change m2-affix-loot task 4.8).
// No game headers - unit-tested in arpg/unittests.
#ifndef ARPG_CORE_LOOTSUBSTITUTION_H
#define ARPG_CORE_LOOTSUBSTITUTION_H

#include <cstdint>
#include <functional>
#include <vector>

namespace Arpg::Core
{
    struct ZoneLevels
    {
        int minLevel = 0;
        int maxLevel = 0;      // 0 = no ceiling (cities, mixed zones, unknown zones)
    };

    // The level loot is made for: the player's level, capped at the zone's max level + margin. Instances: no ceiling
    // (the dungeon level selector, m5, sets theirs).
    int LootLevel(int playerLevel, ZoneLevels const& zone, int margin, bool inInstance);

    // Swap a drop when its required level is more than `gap` below the loot level (never swap upwards past it).
    bool ShouldSubstitute(int itemRequiredLevel, int lootLevel, int gap);

    struct SubstituteCandidate
    {
        uint32_t itemId = 0;
        int requiredLevel = 0;
    };

    using SubstituteRandFn = std::function<uint32_t(uint32_t n)>;   // uniform in [0, n)

    // Pick from candidates (same slot/type/quality, chosen by the caller): those whose required level is closest to the
    // loot level without exceeding it; a random one among ties (variety). 0 if none are <= loot level.
    uint32_t PickSubstitute(std::vector<SubstituteCandidate> const& candidates, int lootLevel, SubstituteRandFn const& rand);
}

#endif
