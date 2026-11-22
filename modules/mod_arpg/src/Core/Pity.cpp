#include "Pity.h"

#include <algorithm>

namespace Arpg::Core
{
    float PityChance(PityLadder const& ladder, int misses)
    {
        float c = ladder.base + ladder.step * float(std::max(0, misses));
        return std::clamp(c, 0.0f, 1.0f);
    }

    bool RollWithPity(PityLadder const& ladder, int& misses, double roll01)
    {
        if (roll01 < double(PityChance(ladder, misses)))
        {
            misses = 0;
            return true;
        }
        ++misses;
        return false;
    }

    int GuaranteedBy(PityLadder const& ladder)
    {
        for (int misses = 0; misses < 1000; ++misses)
            if (PityChance(ladder, misses) >= 1.0f)
                return misses + 1;
        return 0;
    }

    double ExpectedAttempts(PityLadder const& ladder)
    {
        double expected = 0.0, stillMissing = 1.0;
        for (int misses = 0; misses < 1000 && stillMissing > 1e-12; ++misses)
        {
            double c = PityChance(ladder, misses);
            expected += double(misses + 1) * stillMissing * c;
            stillMissing *= (1.0 - c);
        }
        return expected;
    }

    bool ForceGodRoll(int killsWithout, int every)
    {
        return every > 0 && killsWithout + 1 >= every;
    }

    int MaxAffixTier(int baseMaxTier, int raidTier, int paragon, AscendedRule const* rules, int ruleCount)
    {
        int best = baseMaxTier;
        for (int i = 0; i < ruleCount; ++i)
            if (raidTier >= rules[i].minRaidTier && paragon >= rules[i].minParagon)
                best = std::max(best, rules[i].tier);
        return best;
    }
}
