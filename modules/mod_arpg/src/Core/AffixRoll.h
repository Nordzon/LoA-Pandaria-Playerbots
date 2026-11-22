// Pure: which affixes an item gets (SPEC 4.1, change m2-affix-loot). No game headers - unit-tested in arpg/unittests.
#ifndef ARPG_CORE_AFFIXROLL_H
#define ARPG_CORE_AFFIXROLL_H

#include <cstdint>
#include <functional>
#include <vector>

namespace Arpg::Core
{
    enum class AffixKind : uint8_t { Prefix, Suffix };

    // Item qualities as the core numbers them.
    enum : int { QUALITY_POOR = 0, QUALITY_NORMAL = 1, QUALITY_UNCOMMON = 2, QUALITY_RARE = 3, QUALITY_EPIC = 4 };

    struct AffixDef
    {
        uint32_t id = 0;
        AffixKind kind = AffixKind::Prefix;
        uint32_t slotMask = 0;      // bit per item slot group the affix may appear on
        uint32_t weight = 0;        // relative chance (arpg_affix.weight)
        uint32_t exclusiveGroup = 0;// 0 = none; affixes sharing a group never appear together (Lucky / of Fortune)
        uint32_t mainStat = 0;      // 0 = not a main-stat affix; else the stat id (smart loot matches it)
    };

    struct TierRule { int tier; int unlockLevel; };   // T1 1, T2 15, T3 35, T4 60, T5 85

    struct RollRequest
    {
        int quality = QUALITY_UNCOMMON;
        uint32_t slotBit = 0;        // the item's slot group bit
        int playerLevel = 1;
        int tierBonus = 0;           // Champion kill +1, magic find shift, ...
        int absoluteMaxTier = 5;     // T5 normally; Ascended tiers raise it (m5)
        uint32_t playerMainStat = 0; // spec's main stat id for smart loot (0 = none)
        int smartLootPct = 70;       // Arpg.Loot.SmartLootPct
        int forceCount = 0;          // >0: exactly this many (god-roll pity forces 5), still within the 3/3 limits
        bool tierAtMax = false;      // every affix at the highest tier allowed (boss loot quality floor, BALANCE §13)
        int maxCount = 0;            // >0: at most this many (Uniques: 4 random affixes + the power line)
        int extraCount = 0;          // added to the rolled count, never past the quality's max (crafter at max skill +1)
        int tierCap = 0;             // >0: no affix above this tier (grey recipes: T1)
    };

    struct RolledAffix { uint32_t affixId; int tier; };

    // Uniform integer in [0, n). Game code passes urand; tests pass a seeded generator.
    using RandFn = std::function<uint32_t(uint32_t n)>;

    // Affix count range for a quality: green 1-2, blue 2-3, purple 3-5, else 0-0.
    void CountRange(int quality, int& minCount, int& maxCount);

    // Highest tier unlocked at a level by the rules (0 if none).
    int MaxTierForLevel(std::vector<TierRule> const& tiers, int level);

    // Rolls the affix list. Never more than 3 prefixes or 3 suffixes, never the same affix or exclusive group twice,
    // only affixes allowed on the slot, tiers 1..min(levelMax + tierBonus, absoluteMaxTier).
    std::vector<RolledAffix> RollAffixes(std::vector<AffixDef> const& pool, std::vector<TierRule> const& tiers,
                                         RollRequest const& req, RandFn const& rand);
}

#endif
