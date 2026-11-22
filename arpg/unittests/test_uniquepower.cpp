// Unique powers (BALANCE §8b): tier ranges, display steps, perfect rolls, Power scaling, internal cooldown.
#include "TestHarness.h"
#include "UniquePower.h"

#include <random>

using namespace Arpg::Core;

TEST(unique_power_rolls_stay_in_range_on_steps)
{
    PowerTierRange t5{ 25, 30, 210, 260, 2000 };                 // BALANCE §8b T5 damage template
    std::mt19937 rng(11);
    std::uniform_real_distribution<double> u(0.0, 1.0);
    bool sawPerfect = false;
    for (int i = 0; i < 5000; ++i)
    {
        RolledPower p = RollPower(t5, PowerTrigger::ChanceOnHit, [&]() { return u(rng); });
        CHECK(p.trigger >= 25 && p.trigger <= 30);
        CHECK(p.magnitude >= 210 && p.magnitude <= 260);
        CHECK_EQ(p.magnitude % 5, 0);                           // 5% steps (tooltip rows)
        sawPerfect = sawPerfect || p.perfect;
        if (p.perfect)
            CHECK(p.trigger == 30 && p.magnitude == 260);
    }
    CHECK(sawPerfect);                                          // reachable
}

TEST(unique_power_extremes_and_every_nth)
{
    PowerTierRange t1{ 15, 20, 60, 90, 3000 };
    RolledPower lo = RollPower(t1, PowerTrigger::ChanceOnHit, []() { return 0.0; });
    CHECK_EQ(lo.trigger, 15); CHECK_EQ(lo.magnitude, 60); CHECK(!lo.perfect);
    RolledPower hi = RollPower(t1, PowerTrigger::ChanceOnHit, []() { return 0.999999; });
    CHECK_EQ(hi.trigger, 20); CHECK_EQ(hi.magnitude, 90); CHECK(hi.perfect);
    // Every Nth hit: T1 = 8th hit .. T5 = 5th; a lower N is the better roll.
    PowerTierRange nth{ 5, 8, 60, 90, 0 };
    RolledPower best = RollPower(nth, PowerTrigger::EveryNthHit, []() { return 0.999999; });
    CHECK_EQ(best.trigger, 5); CHECK(best.perfect);
    RolledPower worst = RollPower(nth, PowerTrigger::EveryNthHit, []() { return 0.0; });
    CHECK_EQ(worst.trigger, 8); CHECK(!worst.perfect);
}

TEST(unique_power_amounts_and_cooldown)
{
    CHECK_EQ(PowerStat(40000, 30000), 40000u);                   // Power = max(AP, SP)
    CHECK_EQ(PowerStat(5000, 32000), 32000u);
    CHECK_EQ(PowerAmount(PowerKind::Damage, 235, 40000, 500000), 94000u);    // 235% of Power
    CHECK_EQ(PowerAmount(PowerKind::HealAbsorb, 12, 40000, 500000), 60000u); // 12% of max health
    CHECK_EQ(PowerAmount(PowerKind::Damage, 0, 40000, 500000), 0u);
    CHECK(PowerReady(1000, 0));
    CHECK(!PowerReady(1000, 2000));
    CHECK(PowerReady(2000, 2000));
    CHECK(PowerReady(5u, 0xFFFFFFF0u));                          // clock wrap
}
