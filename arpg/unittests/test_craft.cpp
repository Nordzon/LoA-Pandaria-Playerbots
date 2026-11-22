// Crafting wheel (SPEC 4.8): recipe colour thresholds, roll modifiers, crafted-Unique ladder.
#include "TestHarness.h"
#include "AffixRoll.h"
#include "Craft.h"

#include <random>

using namespace Arpg::Core;

TEST(craft_recipe_colour_matches_skill_gain_thresholds)
{
    // yellow at 100, grey at 140 -> green from 120 (Player::UpdateCraftSkill uses (min+max)/2).
    CHECK(RecipeColourFor(90, 100, 140) == RecipeColour::Orange);
    CHECK(RecipeColourFor(100, 100, 140) == RecipeColour::Yellow);
    CHECK(RecipeColourFor(119, 100, 140) == RecipeColour::Yellow);
    CHECK(RecipeColourFor(120, 100, 140) == RecipeColour::Green);
    CHECK(RecipeColourFor(140, 100, 140) == RecipeColour::Grey);
    CHECK(RecipeColourFor(600, 0, 0) == RecipeColour::Yellow);      // no thresholds
}

TEST(craft_mods_by_colour_skill_and_catalyst)
{
    CraftMods o = CraftModsFor(RecipeColour::Orange, false, false);
    CHECK_EQ(o.tierBonus, 1); CHECK(o.uniqueEligible); CHECK_EQ(o.extraAffixes, 0); CHECK_EQ(o.tierCap, 0);
    CraftMods g = CraftModsFor(RecipeColour::Green, true, false);
    CHECK_EQ(g.tierBonus, 0); CHECK(!g.uniqueEligible); CHECK_EQ(g.extraAffixes, 1);
    CraftMods grey = CraftModsFor(RecipeColour::Grey, true, true);
    CHECK_EQ(grey.tierCap, 1); CHECK_EQ(grey.extraAffixes, 0); CHECK(!grey.uniqueEligible);
    CraftMods yc = CraftModsFor(RecipeColour::Yellow, true, true);
    CHECK_EQ(yc.tierBonus, 2); CHECK_EQ(yc.extraAffixes, 1);
}

TEST(craft_unique_ladder_and_catalyst_doubles)
{
    PityLadder l = CraftUniqueLadder(2.0f, 1.0f, false);
    CHECK(PityChance(l, 0) > 0.0199f && PityChance(l, 0) < 0.0201f);
    CHECK(PityChance(l, 10) > 0.1199f && PityChance(l, 10) < 0.1201f);
    CHECK(GuaranteedBy(l) >= 99 && GuaranteedBy(l) <= 100);       // 2% + 98 x 1% (float rounding may need one more)
    PityLadder c = CraftUniqueLadder(2.0f, 1.0f, true);
    CHECK(PityChance(c, 0) > 0.0399f && PityChance(c, 0) < 0.0401f);
}

TEST(craft_extra_affix_and_tier_cap_in_the_roll)
{
    std::vector<AffixDef> pool;
    for (uint32_t i = 1; i <= 12; ++i)
        pool.push_back({ i, i % 2 ? AffixKind::Prefix : AffixKind::Suffix, 0xFFFFFFFFu, 100, 0, 0 });
    std::vector<TierRule> tiers = { { 1, 1 }, { 2, 20 }, { 3, 40 }, { 4, 60 }, { 5, 80 } };
    std::mt19937 rng(5);
    RandFn rand = [&](uint32_t n) { return n ? uint32_t(rng() % n) : 0u; };
    RollRequest req;
    req.quality = QUALITY_UNCOMMON;     // 1-2 affixes
    req.slotBit = 1;
    req.playerLevel = 90;
    req.extraCount = 1;
    req.tierCap = 1;
    for (int i = 0; i < 300; ++i)
    {
        auto out = RollAffixes(pool, tiers, req, rand);
        CHECK_EQ(out.size(), size_t(2));                            // green 1-2, +1, but never past the quality's max
        for (auto const& a : out)
            CHECK_EQ(a.tier, 1);                                    // grey recipe: T1 only
    }
}
