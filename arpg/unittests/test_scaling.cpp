// m1-world-scaling task 3.1: formula edge cases (offset, upward-only, level caps, kill switch).
#include "TestHarness.h"
#include "Scaling.h"

using namespace Arpg::Core;

TEST(scaling_raises_low_creature_to_player)          // Scenario: low-level mob raised to the player
{
    ScaleSettings s;
    CHECK_EQ(int(ScaledLevel(s, 5, 85)), 85);
}

TEST(scaling_is_upward_only)                          // Scenario: higher-level zone stays higher
{
    ScaleSettings s;
    CHECK_EQ(int(ScaledLevel(s, 86, 80)), 0);
    CHECK_EQ(int(ScaledLevel(s, 80, 80)), 0);         // equal: nothing to do
}

TEST(scaling_applies_offset)
{
    ScaleSettings s;
    s.levelOffset = 2;
    CHECK_EQ(int(ScaledLevel(s, 10, 50)), 52);
    s.levelOffset = -3;
    CHECK_EQ(int(ScaledLevel(s, 10, 50)), 47);
    CHECK_EQ(int(ScaledLevel(s, 48, 50)), 0);         // negative offset never lowers a creature
}

TEST(scaling_clamps_to_level_cap)
{
    ScaleSettings s;
    s.levelOffset = 5;
    s.maxLevel = 90;
    CHECK_EQ(int(ScaledLevel(s, 10, 90)), 90);
    CHECK_EQ(int(ScaledLevel(s, 90, 90)), 0);
    s.levelOffset = -200;
    CHECK_EQ(int(ScaledLevel(s, 0, 10)), 1);          // never below 1
}

TEST(scaling_kill_switch)                             // Scenario: disabled
{
    ScaleSettings s;
    s.enabled = false;
    CHECK_EQ(int(ScaledLevel(s, 5, 85)), 0);
}

TEST(scaling_ignores_invalid_player_level)
{
    ScaleSettings s;
    CHECK_EQ(int(ScaledLevel(s, 5, 0)), 0);
}

TEST(scaling_multipliers)
{
    ScaleSettings s;
    s.healthMod = 1.5f;
    s.damageMod = 0.8f;
    CHECK_NEAR(HealthMultiplier(s, true), 1.5, 1e-6);
    CHECK_NEAR(DamageMultiplier(s, true), 0.8, 1e-6);
    CHECK_NEAR(HealthMultiplier(s, false), 1.0, 1e-6);   // unscaled creatures untouched
    s.healthMod = 0.0f;                                  // nonsense config falls back to 1
    CHECK_NEAR(HealthMultiplier(s, true), 1.0, 1e-6);
}
