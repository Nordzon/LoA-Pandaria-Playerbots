#include "Charms.h"

#include <algorithm>

namespace Arpg::Core
{
    int CharmLineCount(int size)
    {
        return (size >= 1 && size <= 3) ? size : 0;
    }

    int WeightSum(std::vector<int> const& weights)
    {
        int s = 0;
        for (int w : weights)
            if (w > 0)
                s += w;
        return s;
    }

    int PickWeighted(std::vector<int> const& weights, uint32_t roll)
    {
        int const total = WeightSum(weights);
        if (total <= 0)
            return -1;
        int r = int(roll % uint32_t(total));
        for (size_t i = 0; i < weights.size(); ++i)
        {
            if (weights[i] <= 0)
                continue;
            if (r < weights[i])
                return int(i);
            r -= weights[i];
        }
        return -1;
    }

    std::vector<int> PickDistinct(std::vector<int> weights, int count, std::vector<uint32_t> const& rolls)
    {
        std::vector<int> out;
        for (int i = 0; i < count; ++i)
        {
            uint32_t roll = i < int(rolls.size()) ? rolls[size_t(i)] : 0;
            int idx = PickWeighted(weights, roll);
            if (idx < 0)
                break;
            out.push_back(idx);
            weights[size_t(idx)] = 0;
        }
        return out;
    }

    int32_t RangeValue(int32_t lo, int32_t hi, uint32_t roll0to9999)
    {
        if (hi < lo)
            std::swap(lo, hi);
        int64_t span = int64_t(hi) - lo;
        return int32_t(lo + span * int64_t(std::min<uint32_t>(roll0to9999, 9999)) / 9999);
    }

    int32_t CharmStatValue(uint32_t budget, int32_t permilleLo, int32_t permilleHi, uint32_t roll0to9999)
    {
        int32_t pm = RangeValue(permilleLo, permilleHi, roll0to9999);
        int64_t v = int64_t(budget) * pm / 1000;
        return int32_t(std::max<int64_t>(1, v));
    }

    bool SpawnGate(float chancePct, uint32_t roll0to9999, uint32_t nowS, bool hasLast, uint32_t lastS, uint32_t cooldownS)
    {
        if (chancePct <= 0.0f)
            return false;
        if (hasLast && nowS < lastS + cooldownS)
            return false;
        return float(roll0to9999 % 10000) < chancePct * 100.0f;
    }
}
