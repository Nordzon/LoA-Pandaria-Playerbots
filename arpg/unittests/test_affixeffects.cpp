// Effect affixes (task 4.3): names, totals with caps, proc roll, crushing blow, phoenix, absorb pool.
#include "TestHarness.h"
#include "AffixEffects.h"

using namespace Arpg::Core;

TEST(effects_names_round_trip)
{
    for (int i = 1; i < int(Effect::Count); ++i)
        CHECK(EffectFromName(EffectName(Effect(i))) == Effect(i));   // every effect has a unique name
    CHECK(EffectFromName("strength") == Effect::None);              // flat stats are not effects
    CHECK(EffectFromName("leech_pct") == Effect::LeechPct);
    CHECK(EffectFromName("proc_frost_nova") == Effect::ProcFrostNova);
}

TEST(effects_totals_and_caps)
{
    EffectTotals t;
    t.Add(Effect::MoveSpeedPct, 1000); t.Add(Effect::MoveSpeedPct, 1000); t.Add(Effect::MoveSpeedPct, 1500);
    CHECK_EQ(t.Raw(Effect::MoveSpeedPct), 3500);
    CHECK_EQ(t.Get(Effect::MoveSpeedPct), 3000);                    // 30% cap
    t.Remove(Effect::MoveSpeedPct, 1500);
    CHECK_EQ(t.Get(Effect::MoveSpeedPct), 2000);
    t.Add(Effect::LeechPct, 300);
    CHECK_EQ(t.Get(Effect::LeechPct), 300);
    t.Add(Effect::LeechPct, 5000);
    CHECK_EQ(t.Get(Effect::LeechPct), 2000);                        // 20% cap
    CHECK(t.Any());
    EffectTotals e;
    CHECK(!e.Any());
    e.Add(Effect::None, 5);
    CHECK(!e.Any());
}

TEST(effects_math)
{
    CHECK_EQ(PctOf(1000, 250), 25u);                                // 2.5% of 1000
    CHECK_EQ(PctOf(1000, -5), 0u);
    CHECK(ProcRoll(500, 499, false));
    CHECK(!ProcRoll(500, 500, false));
    CHECK(ProcRoll(1, 9999, true));                                 // forced (tests)
    CHECK(!ProcRoll(0, 0, true));                                   // no affix, no proc even when forced
    CHECK_EQ(CrushingBlowDamage(100000, false, 50000), 10000u);     // 10% of current health
    CHECK_EQ(CrushingBlowDamage(1000000, true, 50000), 25000u);     // bosses: 2.5%
    CHECK_EQ(CrushingBlowDamage(1000000, false, 100), 300u);        // capped at 3x the hit
}

TEST(effects_phoenix_and_absorb)
{
    PhoenixResult p = Phoenix(5000, 3000, 10000, 2000, true);       // lethal, ready
    CHECK(p.triggered);
    CHECK_EQ(p.damage, 2999u);                                      // leaves 1 HP
    CHECK_EQ(p.heal, 2000u);                                        // 20% of max
    CHECK(!Phoenix(2000, 3000, 10000, 2000, true).triggered);       // not lethal
    CHECK(!Phoenix(5000, 3000, 10000, 2000, false).triggered);      // on cooldown
    uint32_t pool = 300;
    CHECK_EQ(AbsorbFromPool(200, pool), 0u);
    CHECK_EQ(pool, 100u);
    CHECK_EQ(AbsorbFromPool(250, pool), 150u);
    CHECK_EQ(pool, 0u);
}
