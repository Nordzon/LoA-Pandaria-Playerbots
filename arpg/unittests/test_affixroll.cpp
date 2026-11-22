// m2-affix-loot task 2.2: affix roll RULES (not rates): counts, limits, duplicates, exclusive groups, slots, tiers, smart loot.
#include "TestHarness.h"
#include "AffixRoll.h"

#include <memory>
#include <random>
#include <set>

using namespace Arpg::Core;

namespace
{
    uint32_t const SLOT_WEAPON = 1, SLOT_CHEST = 2, SLOT_BOOTS = 4, SLOT_ANY = 0xFFFFFFFF;
    uint32_t const STAT_AGI = 3, STAT_STR = 4;

    std::vector<AffixDef> Pool()
    {
        return {
            { 1, AffixKind::Prefix, SLOT_ANY, 100, 0, 0 },        // Deadly
            { 2, AffixKind::Prefix, SLOT_ANY, 100, 0, 0 },        // Swift
            { 3, AffixKind::Prefix, SLOT_ANY, 100, 0, 0 },        // Masterful
            { 4, AffixKind::Prefix, SLOT_WEAPON, 60, 0, 0 },      // Cruel (weapon only)
            { 5, AffixKind::Prefix, SLOT_BOOTS, 20, 7, 0 },       // Lucky (group 7)
            { 10, AffixKind::Suffix, SLOT_ANY, 100, 0, STAT_STR },// of the Titan
            { 11, AffixKind::Suffix, SLOT_ANY, 100, 0, STAT_AGI },// of the Wind
            { 12, AffixKind::Suffix, SLOT_ANY, 100, 0, 0 },       // of the Whale
            { 13, AffixKind::Suffix, SLOT_BOOTS, 40, 0, 0 },      // of Speed (boots only)
            { 14, AffixKind::Suffix, SLOT_BOOTS, 30, 7, 0 },      // of Fortune (group 7)
        };
    }
    std::vector<TierRule> Tiers() { return { { 1, 1 }, { 2, 15 }, { 3, 35 }, { 4, 60 }, { 5, 85 } }; }

    RandFn Rng(uint32_t seed)
    {
        auto g = std::make_shared<std::mt19937>(seed);
        return [g](uint32_t n) { return n == 0 ? 0u : uint32_t((*g)() % n); };
    }

    std::vector<AffixDef>::const_iterator Find(std::vector<AffixDef> const& p, uint32_t id)
    {
        for (auto it = p.begin(); it != p.end(); ++it) if (it->id == id) return it;
        return p.end();
    }
}

TEST(affix_count_by_quality_always_in_range)
{
    auto pool = Pool(); auto tiers = Tiers();
    int const expectMin[] = { 0, 0, 1, 2, 3 }, expectMax[] = { 0, 0, 2, 3, 5 };
    for (int q = QUALITY_POOR; q <= QUALITY_EPIC; ++q)
        for (uint32_t seed = 1; seed <= 300; ++seed)
        {
            RollRequest r; r.quality = q; r.slotBit = SLOT_BOOTS; r.playerLevel = 90;
            auto got = RollAffixes(pool, tiers, r, Rng(seed));
            CHECK(int(got.size()) >= expectMin[q]);
            CHECK(int(got.size()) <= expectMax[q]);
        }
}

TEST(affix_limits_duplicates_groups_slots)
{
    auto pool = Pool(); auto tiers = Tiers();
    for (uint32_t seed = 1; seed <= 500; ++seed)
    {
        RollRequest r; r.quality = QUALITY_EPIC; r.slotBit = (seed % 2) ? SLOT_BOOTS : SLOT_CHEST; r.playerLevel = 90;
        auto got = RollAffixes(pool, tiers, r, Rng(seed));
        std::set<uint32_t> ids, groups; int pre = 0, suf = 0;
        for (auto const& a : got)
        {
            auto def = Find(pool, a.affixId);
            CHECK(def != pool.end());
            CHECK(ids.insert(a.affixId).second);                         // no duplicates
            if (def->exclusiveGroup) CHECK(groups.insert(def->exclusiveGroup).second);  // Lucky / of Fortune exclusive
            CHECK((def->slotMask & r.slotBit) != 0);                     // allowed on this slot
            (def->kind == AffixKind::Prefix ? pre : suf)++;
        }
        CHECK(pre <= 3);
        CHECK(suf <= 3);
    }
}

TEST(affix_tier_capped_by_level_and_bonus)
{
    auto pool = Pool(); auto tiers = Tiers();
    for (uint32_t seed = 1; seed <= 300; ++seed)
    {
        RollRequest r; r.quality = QUALITY_EPIC; r.slotBit = SLOT_CHEST;
        r.playerLevel = 60;                                              // T4 unlocked at 60
        for (auto const& a : RollAffixes(pool, tiers, r, Rng(seed))) { CHECK(a.tier >= 1); CHECK(a.tier <= 4); }
        r.playerLevel = 34;                                              // T2 only (T3 at 35)
        for (auto const& a : RollAffixes(pool, tiers, r, Rng(seed))) CHECK(a.tier <= 2);
        r.tierBonus = 1;                                                 // Champion kill: one tier higher
        for (auto const& a : RollAffixes(pool, tiers, r, Rng(seed))) CHECK(a.tier <= 3);
        r.playerLevel = 90; r.tierBonus = 3;                             // never above absoluteMaxTier (T5)
        for (auto const& a : RollAffixes(pool, tiers, r, Rng(seed))) CHECK(a.tier <= 5);
    }
    CHECK_EQ(MaxTierForLevel(tiers, 1), 1);
    CHECK_EQ(MaxTierForLevel(tiers, 85), 5);
}

TEST(affix_smart_loot_forced_on_and_off)
{
    auto pool = Pool(); auto tiers = Tiers();
    for (uint32_t seed = 1; seed <= 200; ++seed)
    {
        RollRequest r; r.quality = QUALITY_UNCOMMON; r.slotBit = SLOT_CHEST; r.playerLevel = 50;
        r.playerMainStat = STAT_AGI; r.smartLootPct = 100;               // forced on: first affix is of the Wind
        auto got = RollAffixes(pool, tiers, r, Rng(seed));
        CHECK(!got.empty());
        CHECK_EQ(got.front().affixId, 11u);
        r.smartLootPct = 0; r.quality = QUALITY_UNCOMMON;                // forced off: smart loot never applies first
        r.forceCount = 1;
        pool[6].weight = 0;                                              // remove of the Wind from the random pool
        got = RollAffixes(pool, tiers, r, Rng(seed));
        CHECK(got.empty() || got.front().affixId != 11u);
        pool[6].weight = 100;
    }
}

TEST(affix_boss_loot_rolls_at_the_level_max_tier)
{
    auto pool = Pool(); auto tiers = Tiers();
    for (uint32_t seed = 1; seed <= 200; ++seed)
    {
        RollRequest r; r.quality = QUALITY_EPIC; r.slotBit = SLOT_BOOTS; r.playerLevel = 20; r.tierAtMax = true;
        for (RolledAffix const& a : RollAffixes(pool, tiers, r, Rng(seed)))
            CHECK_EQ(a.tier, 2);                     // level 20 allows T2 (unlock 15): exactly T2, never above
        r.playerLevel = 90;
        for (RolledAffix const& a : RollAffixes(pool, tiers, r, Rng(seed)))
            CHECK_EQ(a.tier, 5);
    }
}

TEST(affix_max_count_caps_uniques_at_four)
{
    auto pool = Pool(); auto tiers = Tiers();
    for (uint32_t seed = 1; seed <= 200; ++seed)
    {
        RollRequest r; r.quality = QUALITY_EPIC; r.slotBit = SLOT_BOOTS; r.playerLevel = 90; r.maxCount = 4;
        CHECK(RollAffixes(pool, tiers, r, Rng(seed)).size() <= 4u);
        r.forceCount = 5;                                // a god roll on a Unique still keeps the power slot free
        CHECK(RollAffixes(pool, tiers, r, Rng(seed)).size() <= 4u);
    }
}

TEST(affix_god_roll_forces_five)
{
    auto pool = Pool(); auto tiers = Tiers();
    for (uint32_t seed = 1; seed <= 200; ++seed)
    {
        RollRequest r; r.quality = QUALITY_EPIC; r.slotBit = SLOT_BOOTS; r.playerLevel = 90; r.forceCount = 5;
        CHECK_EQ(int(RollAffixes(pool, tiers, r, Rng(seed)).size()), 5);
    }
}

TEST(affix_small_pool_never_loops)
{
    std::vector<AffixDef> tiny = { { 1, AffixKind::Prefix, SLOT_WEAPON, 100, 0, 0 } };
    RollRequest r; r.quality = QUALITY_EPIC; r.slotBit = SLOT_WEAPON; r.playerLevel = 90;
    CHECK_EQ(int(RollAffixes(tiny, Tiers(), r, Rng(7)).size()), 1);    // stops when nothing else fits
    r.slotBit = SLOT_CHEST;
    CHECK_EQ(int(RollAffixes(tiny, Tiers(), r, Rng(7)).size()), 0);
}
