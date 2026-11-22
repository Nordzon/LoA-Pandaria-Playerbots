#include "SuperuniqueRotation.h"

#include <algorithm>

namespace Arpg::Core
{
    namespace
    {
        bool Contains(std::vector<uint32_t> const& v, uint32_t id)
        {
            return std::find(v.begin(), v.end(), id) != v.end();
        }

        uint32_t WeightedPick(std::vector<SuperuniqueCandidate> const& c, RotationRandFn const& rand)
        {
            uint32_t total = 0;
            for (SuperuniqueCandidate const& s : c)
                total += s.weight;
            if (!total)
                return 0;
            uint32_t r = rand(total);
            for (SuperuniqueCandidate const& s : c)
            {
                if (r < s.weight)
                    return s.id;
                r -= s.weight;
            }
            return c.back().id;
        }
    }

    uint32_t NextSuperunique(std::vector<SuperuniqueCandidate> const& pool, uint32_t justKilledId,
                             std::vector<uint32_t> const& aliveIds, RotationRandFn const& rand)
    {
        std::vector<SuperuniqueCandidate> open;
        for (SuperuniqueCandidate const& s : pool)
            if (s.weight && s.id != justKilledId && !Contains(aliveIds, s.id))
                open.push_back(s);
        if (open.empty())
            for (SuperuniqueCandidate const& s : pool)
                if (s.weight && s.id == justKilledId && !Contains(aliveIds, s.id))
                    open.push_back(s);
        return open.empty() ? 0 : WeightedPick(open, rand);
    }

    uint64_t RespawnAt(uint64_t killTimeMs, uint32_t delaySeconds)
    {
        return killTimeMs + uint64_t(delaySeconds) * 1000u;
    }

    int TargetAliveCount(std::vector<SuperuniqueCandidate> const& pool, int perZone)
    {
        int usable = int(std::count_if(pool.begin(), pool.end(), [](SuperuniqueCandidate const& s) { return s.weight > 0; }));
        return std::max(0, std::min(perZone, usable));
    }
}
