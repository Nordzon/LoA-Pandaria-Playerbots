// Superunique rotation (SPEC 4.3): a different one spawns after a kill, 300 s later; 1-2 alive per zone.
#include "TestHarness.h"
#include "SuperuniqueRotation.h"

#include <random>

using namespace Arpg::Core;

TEST(superunique_next_is_different)
{
    std::vector<SuperuniqueCandidate> pool = { { 1, 100 }, { 2, 100 }, { 3, 100 } };
    std::mt19937 rng(3);
    auto rand = [&](uint32_t n) { return n ? uint32_t(rng() % n) : 0u; };
    for (int i = 0; i < 1000; ++i)
    {
        uint32_t next = NextSuperunique(pool, 2, {}, rand);
        CHECK(next == 1 || next == 3);
        uint32_t other = NextSuperunique(pool, 1, { 3 }, rand);        // 3 is alive elsewhere in the zone
        CHECK_EQ(other, 2u);
    }
}

TEST(superunique_small_pools)
{
    auto rand = [](uint32_t) { return 0u; };
    CHECK_EQ(NextSuperunique({ { 7, 50 } }, 7, {}, rand), 7u);          // pool of 1: the same one returns
    CHECK_EQ(NextSuperunique({ { 7, 50 } }, 7, { 7 }, rand), 0u);       // ...unless it is somehow alive
    CHECK_EQ(NextSuperunique({ { 1, 10 }, { 2, 10 } }, 1, { 2 }, rand), 1u); // the other is alive: killed one returns
    CHECK_EQ(NextSuperunique({}, 0, {}, rand), 0u);
    CHECK_EQ(NextSuperunique({ { 1, 0 }, { 2, 0 } }, 0, {}, rand), 0u); // all disabled
}

TEST(superunique_weights)
{
    std::vector<SuperuniqueCandidate> pool = { { 1, 100 }, { 2, 0 }, { 3, 100 } };
    auto first = [](uint32_t) { return 0u; };
    auto last = [](uint32_t n) { return n ? n - 1 : 0u; };
    CHECK_EQ(NextSuperunique(pool, 0, {}, first), 1u);
    CHECK_EQ(NextSuperunique(pool, 0, {}, last), 3u);                   // weight 0 never picked
}

TEST(superunique_respawn_and_count)
{
    CHECK_EQ(RespawnAt(1000, 300), uint64_t(301000));
    CHECK_EQ(TargetAliveCount({ { 1, 1 }, { 2, 1 }, { 3, 1 } }, 2), 2);
    CHECK_EQ(TargetAliveCount({ { 1, 1 } }, 2), 1);
    CHECK_EQ(TargetAliveCount({ { 1, 0 } }, 2), 0);
}
