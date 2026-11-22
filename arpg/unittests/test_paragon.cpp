// Paragon (change m4-paragon): curve, multi-level gain, D3 rotation, caps, world tier.
#include "TestHarness.h"
#include "Paragon.h"

using namespace Arpg::Core;

TEST(paragon_curve_grows_slowly)
{
    ParagonCurve c;   // 600k, 1.04
    CHECK_EQ(ParagonXpToNext(c, 0), 600000u);
    CHECK(ParagonXpToNext(c, 10) > 880000u && ParagonXpToNext(c, 10) < 890000u);   // 600k x 1.04^10 = 888k
    CHECK(ParagonXpToNext(c, 100) > ParagonXpToNext(c, 99));
    ParagonCurve f{ 600000.0, double(1.04f) };   // as read from the conf (float): still exact
    CHECK_EQ(ParagonXpToNext(f, 1), 624000u);
}

TEST(paragon_xp_carries_over_levels)
{
    ParagonCurve c{ 1000.0, 1.0 };
    int level = 0;
    uint64_t xp = 0;
    CHECK_EQ(AddParagonXp(c, level, xp, 3500), 3);
    CHECK_EQ(level, 3);
    CHECK_EQ(xp, 500u);
    CHECK_EQ(AddParagonXp(c, level, xp, 400), 0);
    CHECK_EQ(xp, 900u);
}

TEST(paragon_five_levels_in_a_session)
{
    // BALANCE target: >= 5 levels from 0 with a 2-hour session's XP (~3.4M at 90: ~300 kills/h x ~5.7k XP).
    ParagonCurve c;
    int level = 0;
    uint64_t xp = 0;
    AddParagonXp(c, level, xp, 3400000);
    CHECK(level >= 5);
}

TEST(paragon_rotation_and_points)
{
    CHECK(TabForLevel(1) == ParagonTab::Core);
    CHECK(TabForLevel(2) == ParagonTab::Offense);
    CHECK(TabForLevel(4) == ParagonTab::Utility);
    CHECK(TabForLevel(5) == ParagonTab::Core);
    CHECK_EQ(PointsInTab(6, ParagonTab::Core), 2);
    CHECK_EQ(PointsInTab(6, ParagonTab::Offense), 2);
    CHECK_EQ(PointsInTab(6, ParagonTab::Defense), 1);
    CHECK_EQ(PointsInTab(6, ParagonTab::Utility), 1);
    CHECK_EQ(PointsInTab(0, ParagonTab::Core), 0);
}

TEST(paragon_bonus_caps_and_world_tier)
{
    CHECK_EQ(ParagonBonus({ 100, 1000 }, 7), 700);     // move speed cap +7%
    CHECK_EQ(ParagonBonus({ 100, 1000 }, 25), 1000);   // capped at +10%
    CHECK_EQ(ParagonBonus({ 50, 0 }, 60), 3000);       // main stat +30% at 60 points, no cap
    float m = WorldTierMultiplier(10, 2.0f, 100.0f);
    CHECK(m > 1.199f && m < 1.201f);
    CHECK(WorldTierMultiplier(80, 2.0f, 100.0f) < 2.0001f);
}
