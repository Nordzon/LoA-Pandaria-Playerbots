// m2-affix-loot task 2.3: affix values follow B and the tier range; sell value formula (spec scenario).
#include "TestHarness.h"
#include "AffixValue.h"

using namespace Arpg::Core;

TEST(affix_value_flat_inside_tier_range)
{
    TierRange t5{ 18.0f, 22.0f };                     // "of the Wind" T5: 18-22% of B
    CHECK_EQ(FlatValue(1000, t5, 0.0), 180u);
    CHECK_EQ(FlatValue(1000, t5, 1.0), 220u);
    CHECK_EQ(FlatValue(1000, t5, 0.5), 200u);
    CHECK_EQ(FlatValue(1000, t5, 7.0), 220u);         // roll clamped
    CHECK_EQ(FlatValue(3, TierRange{ 6.0f, 9.0f }, 0.0), 1u);   // never 0 at low level
}

TEST(affix_value_follows_player_level)              // Scenario: same item, different levels
{
    TierRange t2{ 9.0f, 12.0f };
    uint32_t low = FlatValue(120, t2, 0.5);           // B at level 20 (small)
    uint32_t high = FlatValue(2400, t2, 0.5);         // B at level 80 (large)
    CHECK(high > low);
    CHECK(low >= 11 && low <= 14);
    CHECK(high >= 216 && high <= 288);
}

TEST(affix_value_percent)
{
    TierRange leech{ 50.0f, 300.0f };                 // Vampiric T1..T5 in hundredths of a percent: 0.5% .. 3%
    CHECK_EQ(PercentValue(leech, 0.0), 50);
    CHECK_EQ(PercentValue(leech, 1.0), 300);
    CHECK_EQ(PercentValue(TierRange{ 300.0f, 50.0f }, 0.0), 50);   // reversed table rows still work
}

TEST(affix_sell_value)                               // Scenario: selling a 3-affix item at 90
{
    uint64_t tenGold = 100000;                        // 10 g in copper
    CHECK_EQ(SellValue(tenGold, 3, 90, 200), uint64_t(154000));   // 15 g 40 s
    CHECK_EQ(SellValue(tenGold, 0, 90, 200), tenGold);             // plain items unchanged
    CHECK_EQ(SellValue(0, 1, 1, 200), uint64_t(200));
}
