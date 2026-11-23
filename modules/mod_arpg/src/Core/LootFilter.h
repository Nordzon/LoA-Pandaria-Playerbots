// Pure: loot filter rules (SPEC 4.7, change m2b task 2.1). No game headers - unit-tested.
// Ordered rules per character; the first matching rule decides; no match = pick up. Quest items and Uniques are protected
// (always picked up); gold never goes through the filter.
#ifndef ARPG_CORE_LOOTFILTER_H
#define ARPG_CORE_LOOTFILTER_H

#include <cstdint>
#include <string>
#include <vector>

namespace Arpg::Core
{
    enum class FilterAction : uint8_t { Pickup = 0, Leave = 1, AutoSell = 2, Highlight = 3 };

    // What the filter knows about one looted item (filled by the game side).
    struct ItemFacts
    {
        uint32_t itemId = 0;
        int quality = 0;              // 0 grey .. 5 legendary
        bool usable = true;           // the character's class can use it (armour type, weapon skill, class mask)
        int itemClass = 0;
        int itemSubClass = 0;
        int affixCount = 0;           // rolled (planned) affixes
        int bestTier = 0;             // highest affix tier on it
        int itemLevel = 0;
        int equippedItemLevel = -1;   // item level of what the character wears in that slot (-1 = nothing / not gear)
        bool quest = false;
        bool unique = false;          // an ARPG Unique
    };

    // A rule: every condition left at "any" (-1, or an empty id list) matches everything.
    struct FilterRule
    {
        int minQuality = -1, maxQuality = -1;
        int usable = -1;              // 0 = only items the class can't use, 1 = only usable ones
        int itemClass = -1, itemSubClass = -1;
        int minAffixes = -1, maxAffixes = -1;
        int minBestTier = -1;
        int vsEquipped = -1;          // 0 = lower item level than equipped, 1 = equal or higher
        std::vector<uint32_t> itemIds;
        FilterAction action = FilterAction::Pickup;
    };

    bool RuleMatches(FilterRule const& r, ItemFacts const& f);

    // The action for an item: protected items -> Pickup; else the first matching rule; else Pickup.
    // `matchedRule` = index of the deciding rule (-1 = none / protected).
    FilterAction EvaluateFilter(std::vector<FilterRule> const& rules, ItemFacts const& f, int& matchedRule);

    // Text form for storage and commands, e.g. "quality<=0 usable=0 class=4 sub=1 affixes>=3 tier>=5 vs=0 ids=1,2 -> sell".
    std::string RuleToText(FilterRule const& r);
    bool RuleFromText(std::string const& text, FilterRule& out);
}

#endif
