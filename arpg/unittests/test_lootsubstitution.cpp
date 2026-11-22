// Level-appropriate loot with a zone ceiling (SPEC 4.1, task 4.8): stay in a zone, but its loot stops growing.
#include "TestHarness.h"
#include "LootSubstitution.h"

using namespace Arpg::Core;

TEST(loot_level_zone_ceiling)
{
    ZoneLevels elwynn{ 3, 10 };
    CHECK_EQ(LootLevel(8, elwynn, 3, false), 8);      // on level: own level
    CHECK_EQ(LootLevel(60, elwynn, 3, false), 13);    // out-levelled: zone max + margin -> move on
    CHECK_EQ(LootLevel(60, elwynn, 3, true), 60);     // instances: no ceiling (m5 level selector)
    CHECK_EQ(LootLevel(60, ZoneLevels{}, 3, false), 60);   // city / unknown zone: no ceiling
    CHECK_EQ(LootLevel(0, elwynn, 3, false), 1);
}

TEST(loot_substitute_gap)
{
    CHECK(!ShouldSubstitute(10, 13, 5));   // close enough: keep
    CHECK(ShouldSubstitute(5, 13, 5));     // 8 below: swap
    CHECK(!ShouldSubstitute(20, 13, 5));   // above the loot level: keep (never swapped down)
}

TEST(loot_substitute_pick)
{
    std::vector<SubstituteCandidate> c = { { 101, 5 }, { 102, 12 }, { 103, 12 }, { 104, 15 }, { 105, 30 } };
    auto first = [](uint32_t) { return 0u; };
    auto second = [](uint32_t) { return 1u; };
    CHECK_EQ(PickSubstitute(c, 13, first), 102u);     // closest at or below 13 (ties: 102/103)
    CHECK_EQ(PickSubstitute(c, 13, second), 103u);
    CHECK_EQ(PickSubstitute(c, 15, first), 104u);
    CHECK_EQ(PickSubstitute(c, 4, first), 0u);        // nothing low enough
    CHECK_EQ(PickSubstitute({}, 20, first), 0u);
}
