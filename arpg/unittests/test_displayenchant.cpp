// Display rows for affixes (SPEC 5): id = base + stat * stride + value; server and client patch must agree.
#include "TestHarness.h"
#include "DisplayEnchant.h"

using namespace Arpg::Core;

TEST(display_enchant_ids)
{
    DisplayRule r;   // 60000 / 2000
    CHECK_EQ(DisplayEnchantId(r, DisplayStat::Strength, 1), 60001u);
    CHECK_EQ(DisplayEnchantId(r, DisplayStat::Strength, 735), 60735u);
    CHECK_EQ(DisplayEnchantId(r, DisplayStat::Agility, 1), 62001u);
    CHECK_EQ(DisplayEnchantId(r, DisplayStat::Parry, 1999), 60000u + 11u * 2000u + 1999u);   // last row of the range
    CHECK_EQ(DisplayEnchantId(r, DisplayStat::Parry, 2000), 0u);    // above the range: no row (addon shows it)
    CHECK_EQ(DisplayEnchantId(r, DisplayStat::Crit, 0), 0u);
    CHECK_EQ(DisplayEnchantId(r, DisplayStat::Crit, -5), 0u);
    CHECK_EQ(DisplayEnchantId(r, DisplayStat::Count, 5), 0u);
}

TEST(display_enchant_ranges_never_overlap)
{
    DisplayRule r;
    for (int s = 0; s + 1 < int(DisplayStat::Count); ++s)
        CHECK(DisplayEnchantId(r, DisplayStat(s), int32_t(r.stride - 1)) < DisplayEnchantId(r, DisplayStat(s + 1), 1));
    CHECK(DisplayEnchantId(r, DisplayStat::Strength, 1) > 5267u);   // above Blizzard's highest id
}

TEST(display_effect_ids)
{
    CHECK_EQ(EffectDisplayId(100000, 1, 150), 101015u);     // leech 1.5% -> effect 1, 15 tenths
    CHECK_EQ(EffectDisplayId(100000, 11, 600), 111060u);    // Frost Nova 6%
    CHECK_EQ(EffectDisplayId(100000, 1, 1537), 101154u);    // 15.37% shows as 15.4%
    CHECK_EQ(EffectDisplayId(100000, 1, 0), 0u);
    CHECK_EQ(EffectDisplayId(100000, 1, 10000), 0u);        // 100% would need tenths 1000: no row
    CHECK_EQ(EffectDisplayId(100000, 0, 150), 0u);
    CHECK_EQ(EffectDisplayId(100000, 35, 0, true), 135000u); // flag (Socketed)
    CHECK_EQ(RoundToTenths(1537), 1540);
    CHECK_EQ(RoundToTenths(1534), 1530);
    CHECK_EQ(RoundToTenths(200), 200);
}

TEST(display_enchant_item_mods)
{
    CHECK_EQ(DisplayStatFromItemMod(4), int(DisplayStat::Strength));
    CHECK_EQ(DisplayStatFromItemMod(3), int(DisplayStat::Agility));
    CHECK_EQ(DisplayStatFromItemMod(49), int(DisplayStat::Mastery));
    CHECK_EQ(DisplayStatFromItemMod(14), int(DisplayStat::Parry));
    CHECK_EQ(DisplayStatFromItemMod(45), -1);   // spell power: no rows
}
