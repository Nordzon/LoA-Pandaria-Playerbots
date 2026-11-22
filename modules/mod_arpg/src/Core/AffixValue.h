// Pure: affix values and sell value (SPEC 4.1, change m2-affix-loot task 2.3). No game headers - unit-tested.
#ifndef ARPG_CORE_AFFIXVALUE_H
#define ARPG_CORE_AFFIXVALUE_H

#include <cstdint>

namespace Arpg::Core
{
    // A tier's range (arpg_affix_tier.min_value / max_value). For flat stats it is a percentage of B (the epic stat budget
    // for the slot at the player's level); for percent affixes (leech, move speed...) it is in hundredths of a percent.
    struct TierRange { float min; float max; };

    // Flat stat affix: B x (range at roll01) / 100, rounded, at least 1. roll01 in [0,1].
    uint32_t FlatValue(uint32_t budgetB, TierRange const& range, double roll01);

    // Ascended tiers (T6-T8, BALANCE §12): the T5 range scaled by pct (110 / 120 / 130).
    TierRange AscendedRange(TierRange const& t5, int pct);

    // Percent affix: the range at roll01, rounded to an integer in the table's unit.
    int32_t PercentValue(TierRange const& range, double roll01);

    // Vendor price with affixes: templatePrice + affixCount x playerLevel x copperPerAffixLevel (copper).
    uint64_t SellValue(uint64_t templatePriceCopper, int affixCount, int playerLevel, uint32_t copperPerAffixLevel);
}

#endif
