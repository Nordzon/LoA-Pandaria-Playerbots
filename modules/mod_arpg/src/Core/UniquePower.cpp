#include "UniquePower.h"

#include <algorithm>
#include <cmath>

namespace Arpg::Core
{
    namespace
    {
        int RollStep(int lo, int hi, int step, double r)
        {
            if (hi < lo)
                std::swap(lo, hi);
            int steps = (hi - lo) / step;                  // number of steps above lo
            int k = std::min(steps, int(r * double(steps + 1)));
            return lo + k * step;
        }
    }

    RolledPower RollPower(PowerTierRange const& range, PowerTrigger trigger, PowerRandFn const& rand)
    {
        RolledPower out;
        double r1 = rand(), r2 = rand();
        out.trigger = RollStep(range.triggerMin, range.triggerMax, 1, r1);
        out.magnitude = RollStep(range.magnitudeMin, range.magnitudeMax, 5, r2);
        // Every Nth hit: a lower N is better, so flip the roll (best roll = triggerMin).
        if (trigger == PowerTrigger::EveryNthHit)
            out.trigger = range.triggerMin + range.triggerMax - out.trigger;
        int bestTrigger = trigger == PowerTrigger::EveryNthHit ? std::min(range.triggerMin, range.triggerMax)
                                                               : std::max(range.triggerMin, range.triggerMax);
        int bestMagnitude = RollStep(range.magnitudeMin, range.magnitudeMax, 5, 0.999999);
        out.perfect = out.trigger == bestTrigger && out.magnitude == bestMagnitude;
        return out;
    }

    uint32_t PowerStat(uint32_t attackPower, uint32_t spellPower)
    {
        return std::max(attackPower, spellPower);
    }

    uint32_t PowerAmount(PowerKind kind, int magnitudePct, uint32_t power, uint32_t maxHealth)
    {
        if (magnitudePct <= 0)
            return 0;
        uint64_t base = kind == PowerKind::HealAbsorb ? maxHealth : power;
        return uint32_t(base * uint64_t(magnitudePct) / 100u);
    }

    bool PowerReady(uint32_t now, uint32_t readyAt)
    {
        return readyAt == 0 || int32_t(now - readyAt) >= 0;
    }
}
