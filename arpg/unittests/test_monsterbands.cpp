// Champion / Superunique level bands (SPEC 4.3, BALANCE §14): early characters can kill them; full numbers from 35/60.
#include "TestHarness.h"
#include "MonsterBands.h"

#include <algorithm>
#include <random>
#include <set>

using namespace Arpg::Core;

namespace
{
    // Same rows as modules/mod_arpg/sql/world/2026_11_22_03_arpg_monster_band.sql
    std::vector<MonsterBand> Bands()
    {
        using K = MonsterKind;
        return {
            { K::Champion, 1, 0, 2.0f, 1.15f, 1, 1, 1, 1, 20, 15 },
            { K::Champion, 15, 1, 2.5f, 1.30f, 1, 2, 2, 3, 20, 15 },
            { K::Champion, 35, 2, 3.0f, 1.50f, 1, 2, 2, 4, 20, 25 },
            { K::Champion, 60, 2, 3.0f, 1.50f, 2, 2, 2, 4, 20, 25 },
            { K::Superunique, 1, 1, 3.0f, 1.20f, 2, 2, 0, 0, 40, 15 },
            { K::Superunique, 15, 2, 4.0f, 1.35f, 3, 3, 0, 0, 40, 15 },
            { K::Superunique, 35, 3, 5.0f, 1.50f, 3, 4, 0, 0, 40, 25 },
            { K::Superunique, 60, 3, 6.0f, 1.60f, 4, 4, 0, 0, 40, 25 },
        };
    }

    // Same rows as arpg_monster_affix: 1-3 soft, 4-6 Enchanted (group 1) from 15, 7 Arcane Caster from 25.
    std::vector<MonsterAffixDef> Affixes()
    {
        return { { 1, 1, 100, 0 }, { 2, 1, 100, 0 }, { 3, 1, 100, 0 },
                 { 4, 15, 100, 1 }, { 5, 15, 100, 1 }, { 6, 15, 100, 1 }, { 7, 25, 60, 0 } };
    }
}

TEST(monster_band_lookup)
{
    auto b = Bands();
    CHECK(BandFor(b, MonsterKind::Champion, 0) == nullptr);                 // below the first row
    CHECK_EQ(BandFor(b, MonsterKind::Champion, 1)->minPlayerLevel, 1);
    CHECK_EQ(BandFor(b, MonsterKind::Champion, 14)->minPlayerLevel, 1);
    CHECK_EQ(BandFor(b, MonsterKind::Champion, 15)->minPlayerLevel, 15);
    CHECK_EQ(BandFor(b, MonsterKind::Champion, 59)->minPlayerLevel, 35);
    CHECK_EQ(BandFor(b, MonsterKind::Champion, 90)->minPlayerLevel, 60);
    CHECK(BandFor(b, MonsterKind::Superunique, 8)->kind == MonsterKind::Superunique);
    CHECK_NEAR(BandFor(b, MonsterKind::Superunique, 90)->healthMult, 6.0, 1e-6);
    CHECK(BandFor({}, MonsterKind::Champion, 50) == nullptr);              // empty table: feature off
}

TEST(monster_level_early_is_at_level)
{
    auto b = Bands();
    CHECK_EQ(MonsterLevel(*BandFor(b, MonsterKind::Champion, 5), 5, 90), 5);      // fresh character: +0
    CHECK_EQ(MonsterLevel(*BandFor(b, MonsterKind::Superunique, 5), 5, 90), 6);   // +1
    CHECK_EQ(MonsterLevel(*BandFor(b, MonsterKind::Champion, 40), 40, 90), 42);
    CHECK_EQ(MonsterLevel(*BandFor(b, MonsterKind::Superunique, 89), 89, 90), 90); // capped at max level
    MonsterBand negative; negative.levelOffset = -3;
    CHECK_EQ(MonsterLevel(negative, 20, 90), 20);                                 // never below the player
}

TEST(monster_affixes_respect_unlocks_and_groups)
{
    auto b = Bands();
    auto pool = Affixes();
    std::mt19937 rng(7);
    auto rand = [&](uint32_t n) { return n ? uint32_t(rng() % n) : 0u; };
    for (int i = 0; i < 2000; ++i)
    {
        // Below 15: exactly 1 affix, soft only.
        auto early = PickMonsterAffixes(pool, *BandFor(b, MonsterKind::Champion, 8), 8, rand);
        CHECK_EQ(int(early.size()), 1);
        CHECK(early[0] >= 1 && early[0] <= 3);

        // 20: 1-2, Enchanted allowed but never two of them, never Arcane (25).
        auto mid = PickMonsterAffixes(pool, *BandFor(b, MonsterKind::Champion, 20), 20, rand);
        CHECK(mid.size() >= 1 && mid.size() <= 2);
        int enchanted = 0;
        for (uint32_t id : mid) { CHECK(id != 7); enchanted += (id >= 4 && id <= 6); }
        CHECK(enchanted <= 1);
        CHECK_EQ(std::set<uint32_t>(mid.begin(), mid.end()).size(), mid.size());

        // 60+: exactly 2.
        auto top = PickMonsterAffixes(pool, *BandFor(b, MonsterKind::Champion, 90), 90, rand);
        CHECK_EQ(int(top.size()), 2);
    }
}

TEST(monster_affixes_pool_runs_out)
{
    MonsterBand band; band.affixMin = 4; band.affixMax = 4;
    std::vector<MonsterAffixDef> pool = { { 1, 1, 10, 0 }, { 2, 1, 10, 0 } };
    auto rand = [](uint32_t n) { return n ? n - 1 : 0u; };
    CHECK_EQ(int(PickMonsterAffixes(pool, band, 10, rand).size()), 2);
    CHECK_EQ(int(PickMonsterAffixes(pool, band, 0, rand).size()), 0);   // nothing unlocked
}

TEST(monster_minions_by_band)
{
    auto b = Bands();
    auto lo = [](uint32_t) { return 0u; };
    auto hi = [](uint32_t n) { return n ? n - 1 : 0u; };
    CHECK_EQ(MinionCount(*BandFor(b, MonsterKind::Champion, 5), lo), 1);
    CHECK_EQ(MinionCount(*BandFor(b, MonsterKind::Champion, 5), hi), 1);
    CHECK_EQ(MinionCount(*BandFor(b, MonsterKind::Champion, 20), lo), 2);
    CHECK_EQ(MinionCount(*BandFor(b, MonsterKind::Champion, 20), hi), 3);
    CHECK_EQ(MinionCount(*BandFor(b, MonsterKind::Champion, 70), hi), 4);
}

TEST(monster_affix_damage_is_share_of_health_never_oneshot)
{
    auto b = Bands();
    CHECK_EQ(AffixDamage(*BandFor(b, MonsterKind::Champion, 10), 200), 30u);       // 15% below 35
    CHECK_EQ(AffixDamage(*BandFor(b, MonsterKind::Champion, 85), 400000), 100000u); // 25% from 35
    MonsterBand brutal; brutal.affixDamagePct = 100;
    CHECK_EQ(AffixDamage(brutal, 500), 499u);                                       // never a one-shot
    CHECK_EQ(AffixDamage(brutal, 1), 1u);
    CHECK_EQ(AffixDamage(*BandFor(b, MonsterKind::Champion, 10), 0), 0u);
    MonsterBand tiny; tiny.affixDamagePct = 1;
    CHECK_EQ(AffixDamage(tiny, 50), 1u);                                            // at least 1
}

TEST(monster_health_orbs)
{
    auto b = Bands();
    CHECK_EQ(OrbAmount(*BandFor(b, MonsterKind::Champion, 30), 1000), 200u);      // 20%
    CHECK_EQ(OrbAmount(*BandFor(b, MonsterKind::Superunique, 30), 1000), 400u);   // 40%
}
