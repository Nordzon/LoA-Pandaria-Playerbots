// Pure: loot pity rules (SPEC 4.4, docs/BALANCE.md §12-13). No game headers - unit-tested in arpg/unittests.
#ifndef ARPG_CORE_PITY_H
#define ARPG_CORE_PITY_H

namespace Arpg::Core
{
    // Chance = base + step x misses (capped at 1). Boss Uniques: 0.20 + 0.20 -> guaranteed on the 5th run.
    struct PityLadder
    {
        float base = 0.20f;
        float step = 0.20f;
    };

    float PityChance(PityLadder const& ladder, int misses);

    // One roll: roll01 in [0,1). Returns true on a drop; updates misses (reset on drop, +1 otherwise).
    bool RollWithPity(PityLadder const& ladder, int& misses, double roll01);

    // Worst case number of attempts until a guaranteed drop (e.g. 5 for 0.20/0.20). 0 if never guaranteed.
    int GuaranteedBy(PityLadder const& ladder);

    // Expected number of attempts until the first drop.
    double ExpectedAttempts(PityLadder const& ladder);

    // "God-roll pity": every Nth boss kill without a 5-affix item forces one. killsWithout = kills since the last one.
    bool ForceGodRoll(int killsWithout, int every);

    // Ascended affix tiers (T6-T8) need both the raid tier and the Paragon level (BALANCE §12).
    struct AscendedRule
    {
        int tier;          // affix tier unlocked
        int minRaidTier;   // arpg_raid_tier.tier
        int minParagon;    // character Paragon level
    };

    // Highest affix tier a drop may roll: baseMaxTier (T1-T5 by level) raised by every rule that is met.
    int MaxAffixTier(int baseMaxTier, int raidTier, int paragon, AscendedRule const* rules, int ruleCount);
}

#endif
