#include "Rifts.h"

#include <algorithm>
#include <cmath>

namespace Arpg::Core
{
    RiftMults RiftMultipliers(int level, float healthGrowth, float damageGrowth)
    {
        RiftMults m;
        if (level < 1)
            return m;
        m.health = float(std::pow(double(std::max(1.0f, healthGrowth)), level));
        m.damage = float(std::pow(double(std::max(1.0f, damageGrowth)), level));
        return m;
    }

    void ApplyRiftAffix(RiftAffix affix, bool boss, float& health, float& damage)
    {
        if (affix == RiftAffix::Fortified && !boss)
        {
            health *= 1.20f;
            damage *= 1.30f;
        }
        else if (affix == RiftAffix::Tyrannical && boss)
        {
            health *= 1.30f;
            damage *= 1.15f;
        }
    }

    char const* RiftAffixName(RiftAffix affix)
    {
        switch (affix)
        {
            case RiftAffix::Fortified: return "Fortified";
            case RiftAffix::Tyrannical: return "Tyrannical";
            default: return "none";
        }
    }

    uint32_t RiftEffectiveSeconds(uint32_t elapsedS, int deaths, uint32_t penaltyS)
    {
        return elapsedS + uint32_t(std::max(0, deaths)) * penaltyS;
    }

    int RiftUpgrade(uint32_t effectiveS, uint32_t parS)
    {
        if (parS == 0)
            return 1;
        uint64_t const e = uint64_t(effectiveS) * 100;
        if (e <= uint64_t(parS) * 60) return 3;
        if (e <= uint64_t(parS) * 80) return 2;
        if (e <= uint64_t(parS) * 100) return 1;
        return 0;
    }

    int RiftRewardItems(int level, int base, int maxItems, bool late)
    {
        int n = std::min(maxItems, base + std::max(0, level) / 3);
        if (late)
            n = (n + 1) / 2;
        return std::max(1, n);
    }

    float RiftEpicChance(int level, float basePct, float stepPct, float capPct)
    {
        return std::clamp(basePct + stepPct * float(std::max(0, level)), 0.0f, capPct);
    }

    int RiftMilestone(uint32_t elapsedS, uint32_t parS, int lastPct)
    {
        if (parS == 0)
            return 0;
        int const steps[] = { 90, 75, 50 };
        for (int s : steps)
            if (s > lastPct && uint64_t(elapsedS) * 100 >= uint64_t(parS) * uint64_t(s))
                return s;
        return 0;
    }
}
