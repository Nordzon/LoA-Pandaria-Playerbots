// Pure: Unique powers (BALANCE §8b, SPEC 4.4, change m3 task 3.6). No game headers - unit-tested in arpg/unittests.
// A Unique's signature power has a tier (T1-T8) and rolls two numbers inside the tier's ranges: a trigger (proc chance in %,
// or N for "every Nth hit") and a magnitude (% of Power for damage, % of max health for heal/absorb).
#ifndef ARPG_CORE_UNIQUEPOWER_H
#define ARPG_CORE_UNIQUEPOWER_H

#include <cstdint>
#include <functional>

namespace Arpg::Core
{
    enum class PowerTrigger : uint8_t { ChanceOnHit = 0, ChanceOnHeal = 1, WhenStruck = 2, EveryNthHit = 3, Passive = 4 };
    enum class PowerKind : uint8_t { Damage = 0, HealAbsorb = 1, Flat = 2 };

    // One row of arpg_unique_power_tier (BALANCE §8b template, or a Unique's own shift of it).
    struct PowerTierRange
    {
        int triggerMin = 0, triggerMax = 0;     // chance % (ChanceOn*, WhenStruck) or N (EveryNthHit: lower is better)
        int magnitudeMin = 0, magnitudeMax = 0; // % of Power (Damage) or % of max health (HealAbsorb)
        uint32_t cooldownMs = 0;                // internal cooldown
    };

    struct RolledPower
    {
        int trigger = 0;
        int magnitude = 0;
        bool perfect = false;                   // both numbers at their best end of the range
    };

    using PowerRandFn = std::function<double()>;   // uniform in [0, 1)

    // Rolls both numbers. Chance rolls in 1% steps, magnitude in 5% steps (the tooltip rows use the same steps); for
    // EveryNthHit the best value is the LOWEST N.
    RolledPower RollPower(PowerTierRange const& range, PowerTrigger trigger, PowerRandFn const& rand);

    // Power = max(attack power, spell power) (class-neutral).
    uint32_t PowerStat(uint32_t attackPower, uint32_t spellPower);

    // Proc amount: magnitude % of Power (Damage) or of max health (HealAbsorb).
    uint32_t PowerAmount(PowerKind kind, int magnitudePct, uint32_t power, uint32_t maxHealth);

    // Internal cooldown: ready when `now` is at or past `readyAt` (wrap-safe ms clock).
    bool PowerReady(uint32_t now, uint32_t readyAt);
}

#endif
