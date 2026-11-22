// m2 4.1b: snapping a rolled stat value to an existing fixed-amount enchantment (native display).
#include "TestHarness.h"
#include "EnchantIndex.h"

using namespace Arpg::Core;

TEST(enchant_nearest_snaps_and_clamps)
{
    EnchantIndex idx;
    uint32_t const AGI = 3;
    idx.Add(AGI, 10, 1001);
    idx.Add(AGI, 20, 1002);
    idx.Add(AGI, 40, 1003);
    CHECK_EQ(idx.Nearest(AGI, 20).amount, 20u);       // exact
    CHECK_EQ(idx.Nearest(AGI, 26).amount, 20u);       // closer to 20
    CHECK_EQ(idx.Nearest(AGI, 34).amount, 40u);       // closer to 40
    CHECK_EQ(idx.Nearest(AGI, 30).amount, 20u);       // tie -> lower
    CHECK_EQ(idx.Nearest(AGI, 3).amount, 10u);        // below the smallest row -> smallest
    CHECK_EQ(idx.Nearest(AGI, 400).amount, 40u);      // above the largest -> largest
    CHECK_EQ(idx.Nearest(AGI, 400).enchantId, 1003u);
    CHECK_EQ(idx.Nearest(7, 50).enchantId, 0u);       // stat without rows -> none
}

TEST(enchant_duplicates_keep_lowest_id_and_skip_zero)
{
    EnchantIndex idx;
    idx.Add(7, 15, 900);
    idx.Add(7, 15, 300);
    idx.Add(7, 15, 600);
    idx.Add(7, 0, 5);                                 // amount 0 = random-suffix row, unusable
    CHECK_EQ(idx.Count(7), size_t(1));
    CHECK_EQ(idx.Nearest(7, 15).enchantId, 300u);
    CHECK_EQ(idx.MinAmount(7), 15u);
    CHECK_EQ(idx.MaxAmount(7), 15u);
}
