// Pure: charms, shrines and goblin roll helpers (changes m9-charms, m9-shrines, m9-goblins). No game headers - unit-tested.
#ifndef ARPG_CORE_CHARMS_H
#define ARPG_CORE_CHARMS_H

#include <cstdint>
#include <vector>

namespace Arpg::Core
{
    // Lines a charm of this size rolls: 1 Small, 2 Large, 3 Grand (anything else: 0).
    int CharmLineCount(int size);

    // Index picked from weights for a roll (taken modulo the weight sum); -1 when every weight is 0. Weights <= 0 never win.
    int PickWeighted(std::vector<int> const& weights, uint32_t roll);
    int WeightSum(std::vector<int> const& weights);

    // Up to `count` distinct indices by weight. rolls[i] is a raw random number for the i-th pick, so the caller supplies
    // the randomness and tests are exact.
    std::vector<int> PickDistinct(std::vector<int> weights, int count, std::vector<uint32_t> const& rolls);

    // A value between lo and hi (inclusive, any order) for a roll in [0, 10000).
    int32_t RangeValue(int32_t lo, int32_t hi, uint32_t roll0to9999);
    // A flat-stat charm line: B x permille / 1000 (permille rolled between lo and hi), at least 1.
    int32_t CharmStatValue(uint32_t budget, int32_t permilleLo, int32_t permilleHi, uint32_t roll0to9999);

    // Monster kinds for drop tables.
    enum class KillKind : int { Normal = 0, Elite = 1, Champion = 2, Superunique = 3, Boss = 4 };

    // Spawn gate for shrines and goblins: chance in percent vs a roll in [0, 10000), and at least cooldownS seconds since the
    // last spawn for this player (hasLast = false: never spawned).
    bool SpawnGate(float chancePct, uint32_t roll0to9999, uint32_t nowS, bool hasLast, uint32_t lastS, uint32_t cooldownS);
}

#endif
