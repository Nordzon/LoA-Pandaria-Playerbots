// Effect affixes in the game (SPEC 4.1, change m2-affix-loot task 4.3): lifesteal, on-hit procs, thorns, move speed...
// Per-character totals are kept in step with equipped items by Loot/AffixStats (same equip/unequip/login hook).
// Tuning (spells for the combat log / roots / DoTs, % of the hit, radius, targets, cooldowns) is in arpg_affix_effect.
#ifndef ARPG_LOOT_AFFIXEFFECTS_H
#define ARPG_LOOT_AFFIXEFFECTS_H

#include "Core/AffixEffects.h"
#include "Loot/AffixRoller.h"

#include "Define.h"

#include <string>
#include <vector>

class Player;

namespace Arpg::Loot
{
    // Startup / .arpg reload: reads arpg_affix_effect. Returns a one-line summary.
    std::string LoadEffectData();
    // Registers the UnitScript/PlayerScript hooks and the loot-money hook.
    void InstallAffixEffects();

    // Called by AffixStats when an item's lines are applied (equip, login) or removed (unequip, break).
    void ApplyEffectLines(Player* player, std::vector<EffectLine> const& lines, bool apply);
    // Called by AffixStats on equip/unequip of a Unique: its power becomes active / inactive (idempotent per item).
    void ApplyUniquePower(Player* player, uint32 itemGuid, bool apply);

    // Logout: forget the character.
    void ForgetEffects(uint64 playerGuid);

    // Capped total of one effect for a character (0 if none).
    int32 EffectTotal(Player* player, Core::Effect e);
    // ".arpg loot effects <char>": every non-zero effect total.
    std::string DescribeEffects(Player* player);
    // ".arpg test effect <char> <name> <value>": adds (or with a negative value removes) a test effect line.
    bool AddTestEffect(Player* player, std::string const& name, int32 value, std::string& message);
}

#endif
