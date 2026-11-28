// Pure: rift math (change m9-rifts). No game headers - unit-tested in arpg/unittests.
#ifndef ARPG_CORE_RIFTS_H
#define ARPG_CORE_RIFTS_H

#include <cstdint>

namespace Arpg::Core
{
    struct RiftMults
    {
        float health = 1.0f;
        float damage = 1.0f;
    };

    // Health growth^level, damage growth^level (level < 1: no change).
    RiftMults RiftMultipliers(int level, float healthGrowth, float damageGrowth);

    enum class RiftAffix : int { None = 0, Fortified = 1, Tyrannical = 2 };
    // Fortified: trash +20% health, +30% damage. Tyrannical: bosses +30% health, +15% damage. Multiplies health and damage.
    void ApplyRiftAffix(RiftAffix affix, bool boss, float& health, float& damage);
    char const* RiftAffixName(RiftAffix affix);

    // Elapsed seconds plus the death penalty.
    uint32_t RiftEffectiveSeconds(uint32_t elapsedS, int deaths, uint32_t penaltyS);
    // Key change for a finished run: +3 at <= 60% of par, +2 at <= 80%, +1 at <= 100%, 0 when late (par 0: +1).
    int RiftUpgrade(uint32_t effectiveS, uint32_t parS);

    // Reward gear items: base + level / 3, capped; half (rounded up, at least 1) when late.
    int RiftRewardItems(int level, int base, int maxItems, bool late);
    // Chance (percent) that a reward item is epic: base + step x level, capped.
    float RiftEpicChance(int level, float basePct, float stepPct, float capPct);

    // The par milestone (50, 75, 90) passed now and not announced yet (lastPct = the last one announced), or 0.
    int RiftMilestone(uint32_t elapsedS, uint32_t parS, int lastPct);
}

#endif
