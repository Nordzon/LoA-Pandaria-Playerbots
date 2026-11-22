#include "LootSubstitution.h"

#include <algorithm>

namespace Arpg::Core
{
    int LootLevel(int playerLevel, ZoneLevels const& zone, int margin, bool inInstance)
    {
        if (playerLevel < 1)
            return 1;
        if (inInstance || zone.maxLevel <= 0)
            return playerLevel;
        return std::max(1, std::min(playerLevel, zone.maxLevel + std::max(0, margin)));
    }

    bool ShouldSubstitute(int itemRequiredLevel, int lootLevel, int gap)
    {
        return lootLevel - itemRequiredLevel > std::max(0, gap);
    }

    uint32_t PickSubstitute(std::vector<SubstituteCandidate> const& candidates, int lootLevel, SubstituteRandFn const& rand)
    {
        int best = -1;
        for (SubstituteCandidate const& c : candidates)
            if (c.requiredLevel <= lootLevel && c.requiredLevel > best)
                best = c.requiredLevel;
        if (best < 0)
            return 0;
        std::vector<uint32_t> ties;
        for (SubstituteCandidate const& c : candidates)
            if (c.requiredLevel == best)
                ties.push_back(c.itemId);
        return ties[ties.size() > 1 ? rand(uint32_t(ties.size())) % ties.size() : 0];
    }
}
