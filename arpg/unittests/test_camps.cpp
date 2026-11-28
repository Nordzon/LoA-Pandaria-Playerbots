// Champion camps (change m9-champion-camps): squares and the "wants a Champion" rule.
#include "TestHarness.h"
#include "ChampionCamps.h"

using namespace Arpg::Core;

TEST(camp_key_squares)
{
    CHECK_EQ(CampKey(0, 10.0f, 10.0f, 100.0f), CampKey(0, 99.0f, 1.0f, 100.0f));       // same square
    CHECK(CampKey(0, 10.0f, 10.0f, 100.0f) != CampKey(0, 101.0f, 10.0f, 100.0f));     // next square east
    CHECK(CampKey(0, -1.0f, 10.0f, 100.0f) != CampKey(0, 1.0f, 10.0f, 100.0f));       // across 0
    CHECK(CampKey(0, 10.0f, 10.0f, 100.0f) != CampKey(1, 10.0f, 10.0f, 100.0f));      // other map
    CHECK_EQ(CampKey(870, -5000.0f, 3000.0f, 0.0f), CampKey(870, -5000.0f, 3000.0f, 1.0f));   // size 0 -> 1
}

TEST(camp_wants_champion)
{
    CHECK(CampWantsChampion(false, 100, 0, 4, 4));
    CHECK(!CampWantsChampion(true, 100, 0, 9, 4));      // already has one
    CHECK(!CampWantsChampion(false, 100, 200, 9, 4));   // respawn delay running
    CHECK(CampWantsChampion(false, 200, 200, 9, 4));    // delay over
    CHECK(!CampWantsChampion(false, 100, 0, 3, 4));     // too few monsters
    CHECK(CampWantsChampion(false, 100, 0, 1, 0));      // min 0 counts as 1
}
