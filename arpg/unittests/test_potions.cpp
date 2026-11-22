// ARPG potions (change m3b-potions): % heals, resource rules, tier pick, kind pick.
#include "TestHarness.h"
#include "Potions.h"

using namespace Arpg::Core;

TEST(potion_heal_is_pct_of_max_health)
{
    CHECK_EQ(PotionHeal(400000, 10), 40000u);   // tier 1 at level 90
    CHECK_EQ(PotionHeal(1000, 20), 200u);       // tier 6 at level 10
    CHECK_EQ(PotionHeal(5, 10), 1u);            // at least 1
}

TEST(potion_resources_every_class_gets_something)
{
    CHECK_EQ(PotionResource(Resource::Mana, 100000, 16), 16000u);
    CHECK_EQ(PotionResource(Resource::Energy, 100, 16), 8u);      // half the %
    CHECK_EQ(PotionResource(Resource::Focus, 100, 16), 8u);
    CHECK_EQ(PotionResource(Resource::Rage, 1000, 16), 160u);     // rage is stored x10: 16 rage
    CHECK_EQ(PotionResource(Resource::RunicPower, 1000, 16), 160u);
    CHECK_EQ(PotionResource(Resource::Other, 100, 16), 0u);
    CHECK_EQ(PotionResource(Resource::Mana, 0, 16), 0u);          // no mana bar
}

TEST(potion_tier_pick_uses_highest_at_or_below)
{
    std::vector<PotionDef> p = { { 118, PotionKind::Healing, 1, 10 }, { 3928, PotionKind::Healing, 5, 18 },
                                 { 2456, PotionKind::Rejuvenation, 1, 8 }, { 18253, PotionKind::Rejuvenation, 4, 12 } };
    CHECK_EQ(PotionForTier(p, PotionKind::Healing, 5), 3928u);
    CHECK_EQ(PotionForTier(p, PotionKind::Healing, 3), 118u);
    CHECK_EQ(PotionForTier(p, PotionKind::Rejuvenation, 6), 18253u);
    CHECK_EQ(PotionForTier(p, PotionKind::Mana, 6), 0u);
}

TEST(potion_kind_pick_by_weight)
{
    CHECK(PickPotionKind(60, 30, 10, 0) == PotionKind::Healing);
    CHECK(PickPotionKind(60, 30, 10, 59) == PotionKind::Healing);
    CHECK(PickPotionKind(60, 30, 10, 60) == PotionKind::Mana);
    CHECK(PickPotionKind(60, 30, 10, 95) == PotionKind::Rejuvenation);
}
