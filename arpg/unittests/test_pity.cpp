// Loot pity (SPEC 4.4, BALANCE §12-13): Unique by run 5, god-roll pity, Paragon-gated Ascended tiers.
#include "TestHarness.h"
#include "Pity.h"

#include <random>

using namespace Arpg::Core;

TEST(pity_ladder_chances)
{
    PityLadder l;   // 20% + 20% per miss
    CHECK_NEAR(PityChance(l, 0), 0.20, 1e-6);
    CHECK_NEAR(PityChance(l, 1), 0.40, 1e-6);
    CHECK_NEAR(PityChance(l, 3), 0.80, 1e-6);
    CHECK_NEAR(PityChance(l, 4), 1.00, 1e-6);
    CHECK_NEAR(PityChance(l, 9), 1.00, 1e-6);   // capped
    CHECK_EQ(GuaranteedBy(l), 5);
}

TEST(pity_expected_runs)
{
    PityLadder l;
    CHECK_NEAR(ExpectedAttempts(l), 2.5104, 1e-3);       // .2 + 2(.8)(.4) + 3(.8)(.6)(.6) + 4(.8)(.6)(.4)(.8) + 5(.8)(.6)(.4)(.2)
    PityLadder superunique{ 0.10f, 0.05f };              // Superunique: 10% + 5% per miss
    CHECK_EQ(GuaranteedBy(superunique), 19);
    CHECK(ExpectedAttempts(superunique) < 6.0);
}

TEST(pity_simulation_never_exceeds_five_runs)       // the ".arpg loot sim" property, in milliseconds
{
    PityLadder l;
    std::mt19937 rng(12345);
    std::uniform_real_distribution<double> u(0.0, 1.0);
    int worst = 0;
    double total = 0;
    int const trials = 20000;
    for (int t = 0; t < trials; ++t)
    {
        int misses = 0, runs = 0;
        bool got = false;
        while (!got && runs < 100)
        {
            ++runs;
            got = RollWithPity(l, misses, u(rng));
        }
        CHECK(got);
        CHECK_EQ(misses, 0);                              // counter resets on the drop
        worst = std::max(worst, runs);
        total += runs;
    }
    CHECK(worst <= 5);
    CHECK_NEAR(total / trials, 2.51, 0.05);
}

TEST(pity_god_roll_every_third_kill)
{
    CHECK(!ForceGodRoll(0, 3));
    CHECK(!ForceGodRoll(1, 3));
    CHECK(ForceGodRoll(2, 3));                            // 3rd kill without one
    CHECK(!ForceGodRoll(5, 0));                           // 0 = off
}

TEST(pity_ascended_tiers_need_raid_tier_and_paragon)
{
    AscendedRule const rules[] = { { 6, 3, 25 }, { 7, 5, 60 }, { 8, 7, 110 } };
    CHECK_EQ(MaxAffixTier(5, 5, 60, rules, 3), 7);        // raid tier 5 + Paragon 60 -> T7
    CHECK_EQ(MaxAffixTier(5, 5, 59, rules, 3), 6);        // Paragon 59 -> never T7
    CHECK_EQ(MaxAffixTier(5, 2, 200, rules, 3), 5);       // low raid tier caps it whatever the Paragon
    CHECK_EQ(MaxAffixTier(5, 7, 110, rules, 3), 8);
    CHECK_EQ(MaxAffixTier(3, 0, 0, rules, 3), 3);         // levelling: level-based tier stays
}
