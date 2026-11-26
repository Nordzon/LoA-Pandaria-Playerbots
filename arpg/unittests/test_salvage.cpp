// Salvage and catalyst bands (change m6-custom-items).
#include "TestHarness.h"
#include "Salvage.h"

using namespace Arpg::Core;

TEST(salvage_essences_by_affix_count)                    // Scenario: salvage a 2-affix and a 3-affix item -> 5 essences
{
    SalvageRules r;
    SalvageItem a; a.quality = 2; a.affixCount = 2;
    SalvageItem b; b.quality = 3; b.affixCount = 3;
    CHECK_EQ(EssencesFor(a, r) + EssencesFor(b, r), 5);
}

TEST(salvage_skips_equipped_protected_plain_and_epic)
{
    SalvageRules r;
    SalvageItem e; e.quality = 2; e.affixCount = 2; e.equipped = true;
    CHECK(!CanSalvage(e, r));
    SalvageItem p; p.quality = 2; p.affixCount = 2; p.protectedByFilter = true;
    CHECK(!CanSalvage(p, r));
    SalvageItem plain; plain.quality = 2;
    CHECK(!CanSalvage(plain, r));
    SalvageItem epic; epic.quality = 4; epic.affixCount = 4;
    CHECK(!CanSalvage(epic, r));
    CHECK_EQ(EssencesFor(epic, r), 0);
}

TEST(salvage_unique_and_ascended_bonus)
{
    SalvageRules r;
    SalvageItem u; u.quality = 4; u.affixCount = 3; u.unique = true;
    CHECK(!CanSalvage(u, r));                                             // kept by default
    r.includeUniques = true;                                              // then any quality
    CHECK(CanSalvage(u, r));
    CHECK_EQ(EssencesFor(u, r), 3 + 5);
    SalvageItem a; a.quality = 3; a.affixCount = 2; a.ascended = true;
    CHECK_EQ(EssencesFor(a, r), 2 + 3);
}

TEST(catalyst_band_by_tier)
{
    CHECK_EQ(CatalystBandForTier(0), 0);
    CHECK_EQ(CatalystBandForTier(1), 1);
    CHECK_EQ(CatalystBandForTier(2), 1);
    CHECK_EQ(CatalystBandForTier(3), 2);
    CHECK_EQ(CatalystBandForTier(4), 2);
    CHECK_EQ(CatalystBandForTier(5), 3);
    CHECK_EQ(CatalystBandForTier(8), 3);
    CHECK_EQ(CatalystBandMaxTier(1), 2);
    CHECK_EQ(CatalystBandMaxTier(3), 99);
}
