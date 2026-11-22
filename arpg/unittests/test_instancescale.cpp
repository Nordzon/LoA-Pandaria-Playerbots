// Dungeon / raid scaling (change m5): party table, interpolation, raid tiers, solo roles.
#include "TestHarness.h"
#include "InstanceScale.h"

using namespace Arpg::Core;

static bool Near(float a, float b) { return a > b - 0.0005f && a < b + 0.0005f; }

TEST(party_scale_rows_and_clamps)
{
    std::vector<PartyScaleRow> fiveMan = { { 1, 0.30f, 0.40f }, { 2, 0.45f, 0.55f }, { 3, 0.65f, 0.70f }, { 4, 0.80f, 0.85f }, { 5, 1.0f, 1.0f } };
    float h, d;
    PartyScale(fiveMan, 1, h, d);
    CHECK(Near(h, 0.30f) && Near(d, 0.40f));
    PartyScale(fiveMan, 4, h, d);
    CHECK(Near(h, 0.80f) && Near(d, 0.85f));
    PartyScale(fiveMan, 9, h, d);          // more than the table: the full-group row
    CHECK(Near(h, 1.0f) && Near(d, 1.0f));
    PartyScale(fiveMan, 0, h, d);          // nobody counted: the first row
    CHECK(Near(h, 0.30f));
    PartyScale({}, 3, h, d);
    CHECK(Near(h, 1.0f) && Near(d, 1.0f));
}

TEST(party_scale_interpolates_raid_rows)
{
    std::vector<PartyScaleRow> raid = { { 10, 1.0f, 1.0f }, { 1, 0.10f, 0.25f }, { 5, 0.35f, 0.55f } };   // unsorted
    float h, d;
    PartyScale(raid, 3, h, d);             // halfway between 1 and 5
    CHECK(Near(h, 0.225f) && Near(d, 0.40f));
    PartyScale(raid, 25, h, d);
    CHECK(Near(h, 1.0f));
}

TEST(raid_tiers_and_roles)
{
    CHECK(Near(RaidTierMultiplier(0, 0.15f), 1.0f));
    CHECK(Near(RaidTierMultiplier(5, 0.15f), 1.75f));
    CHECK(Near(RaidTierMultiplier(7, 0.15f), 2.05f));
    CHECK(RoleForSpec(268) == SpecRole::Tank);      // Brewmaster
    CHECK(RoleForSpec(270) == SpecRole::Healer);    // Mistweaver
    CHECK(RoleForSpec(259) == SpecRole::Damage);    // Assassination
    SoloKnobs k;
    CHECK(Near(SoloCreatureDamage(SpecRole::Damage, true, k), 0.75f));
    CHECK(Near(SoloCreatureDamage(SpecRole::Tank, true, k), 1.0f));
    CHECK(Near(SoloCreatureDamage(SpecRole::Damage, false, k), 1.0f));
    CHECK(Near(SoloPlayerDamageDealt(SpecRole::Tank, true, k), 1.25f));
    CHECK(Near(SoloPlayerDamageDealt(SpecRole::Healer, true, k), 1.6f));
    CHECK(Near(SoloPlayerDamageDealt(SpecRole::Healer, false, k), 1.0f));
}
