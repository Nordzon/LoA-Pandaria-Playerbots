// Applies flat stat affixes of equipped items to the player (m2-affix-loot task 4.1d).
// Runs from ArpgHooks::ItemBonuses, i.e. exactly when the core applies/removes the item's own stats (equip, unequip,
// login, break/repair). Native-slotted affixes are skipped: the core's enchantment code applies those.
// Each character's applied lines are remembered, so removal always undoes exactly what was added.
#ifndef ARPG_LOOT_AFFIXSTATS_H
#define ARPG_LOOT_AFFIXSTATS_H

#include "Define.h"

#include <string>

class Player;

namespace Arpg::Loot
{
    // Sets ArpgHooks::ItemBonuses and registers the logout cleanup.
    void InstallAffixStats();

    // ".arpg loot applied <char>": items and stat totals mod_arpg currently applies to this character.
    std::string DescribeApplied(Player* player);

    // Adds (apply) or removes one flat stat (ITEM_MOD_* id) on the player, the way equipped affix lines do (charms, m9).
    void ApplyStatLine(Player* player, uint32 itemMod, int32 value, bool apply);
    // ITEM_MOD_* id for an affix stat name ("stamina", "crit_rating"...), 0 if unknown.
    uint32 StatModFromName(std::string const& name);
}

#endif
