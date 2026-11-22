// Pure: which Superunique of a zone spawns next, and when (SPEC 4.3, change m3). No game headers - unit-tested.
// "5 minutes after one dies, a DIFFERENT Superunique from the zone's pool spawns elsewhere in the zone."
#ifndef ARPG_CORE_SUPERUNIQUEROTATION_H
#define ARPG_CORE_SUPERUNIQUEROTATION_H

#include <cstdint>
#include <functional>
#include <vector>

namespace Arpg::Core
{
    struct SuperuniqueCandidate
    {
        uint32_t id = 0;        // arpg_superunique.id
        uint32_t weight = 0;    // relative chance; 0 = disabled
    };

    using RotationRandFn = std::function<uint32_t(uint32_t n)>;   // uniform in [0, n)

    // Next Superunique for the zone: weighted pick that skips the one just killed and the ones still alive.
    // If that leaves nothing (pool of 1, or all others alive), the just-killed one may return (unless it is alive).
    // Returns 0 if nothing can spawn.
    uint32_t NextSuperunique(std::vector<SuperuniqueCandidate> const& pool, uint32_t justKilledId,
                             std::vector<uint32_t> const& aliveIds, RotationRandFn const& rand);

    // Time (ms, same clock as killTimeMs) at which the replacement spawns.
    uint64_t RespawnAt(uint64_t killTimeMs, uint32_t delaySeconds);

    // How many Superuniques a zone keeps alive: min(configured, pool size with weight > 0).
    int TargetAliveCount(std::vector<SuperuniqueCandidate> const& pool, int perZone);
}

#endif
