// m2 task 4.2: player level -> epic item level curve (B is read at that item level).
#include "TestHarness.h"
#include "LevelCurve.h"

using namespace Arpg::Core;

TEST(curve_epic_ilvl_default_points)
{
    LevelCurve c;
    CHECK(c.Parse("1:6,59:64,60:66,70:115,80:213,85:359,90:463"));
    CHECK_EQ(c.ValueAt(1), 6);
    CHECK_EQ(c.ValueAt(60), 66);
    CHECK_EQ(c.ValueAt(85), 359);
    CHECK_EQ(c.ValueAt(90), 463);
    CHECK_EQ(c.ValueAt(65), 91);          // halfway 66..115 = 90.5 -> 91
    CHECK_EQ(c.ValueAt(0), 6);            // clamped below
    CHECK_EQ(c.ValueAt(100), 463);        // clamped above
}

TEST(curve_is_monotonic_over_all_levels)
{
    LevelCurve c;
    c.Parse("1:6,59:64,60:66,70:115,80:213,85:359,90:463");
    for (int lvl = 2; lvl <= 90; ++lvl)
        CHECK(c.ValueAt(lvl) >= c.ValueAt(lvl - 1));
}

TEST(curve_rejects_bad_text_and_keeps_old)
{
    LevelCurve c;
    c.Parse("1:10,90:100");
    CHECK(!c.Parse("1:10,abc"));
    CHECK(!c.Parse("5:10,5:20"));         // x must increase
    CHECK(!c.Parse(""));
    CHECK_EQ(c.ValueAt(90), 100);         // old curve kept
}
