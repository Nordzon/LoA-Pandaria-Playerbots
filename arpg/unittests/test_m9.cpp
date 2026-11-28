// Charms, shrines, goblins, rifts (changes m9-*): pure helpers.
#include "TestHarness.h"
#include "Charms.h"
#include "Rifts.h"

#include <cmath>

using namespace Arpg::Core;

TEST(charm_line_counts)
{
    CHECK_EQ(CharmLineCount(1), 1);
    CHECK_EQ(CharmLineCount(2), 2);
    CHECK_EQ(CharmLineCount(3), 3);
    CHECK_EQ(CharmLineCount(4), 0);
}

TEST(weighted_pick_and_distinct)
{
    std::vector<int> w{ 70, 25, 5 };
    CHECK_EQ(PickWeighted(w, 0), 0);
    CHECK_EQ(PickWeighted(w, 69), 0);
    CHECK_EQ(PickWeighted(w, 70), 1);
    CHECK_EQ(PickWeighted(w, 99), 2);
    CHECK_EQ(PickWeighted(w, 100), 0);          // modulo
    CHECK_EQ(PickWeighted({ 0, 0 }, 5), -1);
    std::vector<int> d = PickDistinct({ 10, 10, 10 }, 3, { 0, 0, 0 });
    CHECK_EQ(int(d.size()), 3);
    CHECK(d[0] != d[1] && d[1] != d[2] && d[0] != d[2]);
    CHECK_EQ(int(PickDistinct({ 10, 0 }, 3, { 0, 0, 0 }).size()), 1);   // only one line can roll
}

TEST(charm_values)
{
    CHECK_EQ(RangeValue(10, 20, 0), 10);
    CHECK_EQ(RangeValue(10, 20, 9999), 20);
    CHECK_EQ(RangeValue(20, 10, 9999), 20);
    CHECK_EQ(CharmStatValue(1000, 60, 120, 0), 60);
    CHECK_EQ(CharmStatValue(1000, 60, 120, 9999), 120);
    CHECK_EQ(CharmStatValue(5, 60, 120, 0), 1);            // never 0
}

TEST(spawn_gate)
{
    CHECK(SpawnGate(100.0f, 9999, 1000, false, 0, 240));
    CHECK(!SpawnGate(0.0f, 0, 1000, false, 0, 240));
    CHECK(SpawnGate(1.0f, 99, 1000, false, 0, 240));
    CHECK(!SpawnGate(1.0f, 100, 1000, false, 0, 240));
    CHECK(!SpawnGate(100.0f, 0, 1000, true, 900, 240));    // cooling down
    CHECK(SpawnGate(100.0f, 0, 1140, true, 900, 240));
}

TEST(rift_multipliers)
{
    RiftMults m = RiftMultipliers(3, 1.10f, 1.07f);
    CHECK(std::fabs(m.health - 1.331f) < 0.001f);
    CHECK(std::fabs(m.damage - 1.225f) < 0.001f);
    RiftMults z = RiftMultipliers(0, 1.10f, 1.07f);
    CHECK(z.health == 1.0f && z.damage == 1.0f);
    float h = 1.0f, d = 1.0f;
    ApplyRiftAffix(RiftAffix::Fortified, false, h, d);
    CHECK(std::fabs(h - 1.2f) < 0.001f && std::fabs(d - 1.3f) < 0.001f);
    h = d = 1.0f;
    ApplyRiftAffix(RiftAffix::Fortified, true, h, d);       // bosses untouched
    CHECK(h == 1.0f && d == 1.0f);
    ApplyRiftAffix(RiftAffix::Tyrannical, true, h, d);
    CHECK(std::fabs(h - 1.3f) < 0.001f && std::fabs(d - 1.15f) < 0.001f);
}

TEST(rift_upgrade_and_rewards)
{
    CHECK_EQ(RiftUpgrade(800, 1500), 3);
    CHECK_EQ(RiftUpgrade(900, 1500), 3);
    CHECK_EQ(RiftUpgrade(1200, 1500), 2);
    CHECK_EQ(RiftUpgrade(1500, 1500), 1);
    CHECK_EQ(RiftUpgrade(1700, 1500), 0);
    CHECK_EQ(RiftEffectiveSeconds(1000, 3, 10), 1030u);
    CHECK_EQ(RiftRewardItems(3, 2, 8, false), 3);
    CHECK_EQ(RiftRewardItems(30, 2, 8, false), 8);
    CHECK_EQ(RiftRewardItems(3, 2, 8, true), 2);
    CHECK_EQ(RiftRewardItems(0, 1, 8, true), 1);
    CHECK(std::fabs(RiftEpicChance(5, 10.0f, 3.0f, 60.0f) - 25.0f) < 0.01f);
    CHECK(std::fabs(RiftEpicChance(50, 10.0f, 3.0f, 60.0f) - 60.0f) < 0.01f);
}

TEST(rift_milestones)
{
    CHECK_EQ(RiftMilestone(100, 1000, 0), 0);
    CHECK_EQ(RiftMilestone(500, 1000, 0), 50);
    CHECK_EQ(RiftMilestone(800, 1000, 50), 75);
    CHECK_EQ(RiftMilestone(950, 1000, 0), 90);              // jumps straight to the highest passed
    CHECK_EQ(RiftMilestone(950, 1000, 90), 0);
}
