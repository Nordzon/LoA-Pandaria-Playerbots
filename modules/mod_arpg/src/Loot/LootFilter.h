// Loot filter (SPEC 4.7, change m2b tasks 2.2/2.3/2.5): ordered rules per character (character_arpg_lootfilter,
// Core/LootFilter). Applied when a corpse/chest is planned (roll at loot time): Sell = the item leaves the loot and its
// vendor price (affix value included) is paid at once; Leave = the item leaves the loot (v1: the client auto-loots
// whatever the window shows, so "stays on the corpse" needs a core hook - owner to decide); Highlight/Pickup = kept.
// Quest items and Uniques are protected. A character without rules gets the default "quality<=0 -> sell".
#ifndef ARPG_LOOT_LOOTFILTER_H
#define ARPG_LOOT_LOOTFILTER_H

#include "Define.h"

#include <string>
#include <vector>

class Player;
struct Loot;
struct LootItem;

namespace Arpg::Loot
{
    std::vector<std::string> FilterRules(Player* player);
    // Errors in `error`; positions are 1-based in the commands.
    bool AddFilterRule(Player* player, std::string const& text, std::string& error);
    bool RemoveFilterRule(Player* player, int position, std::string& error);
    bool MoveFilterRule(Player* player, int from, int to, std::string& error);
    void ResetFilter(Player* player);

    // Runs the filter over one loot item (affix count / best tier from its plan). Returns false if the item was taken out
    // of the loot (sold or left).
    bool ApplyFilter(Player* player, ::Loot& loot, LootItem& li, int affixCount, int bestTier, bool unique);
}

#endif
