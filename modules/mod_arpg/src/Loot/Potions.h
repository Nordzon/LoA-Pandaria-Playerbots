// ARPG potions (owner 2026-11-22, change m3b-potions): existing potion items heal / restore a % of max health / resource
// (arpg_potion, Core/Potions), drop by monster kind (arpg_potion_drop), stack to 999 and share a 10 s cooldown (item_template +
// Item-sparse hotfixes, SQL 2026_11_22_10).
#ifndef ARPG_LOOT_POTIONS_H
#define ARPG_LOOT_POTIONS_H

#include "Define.h"

#include <string>

class Player;

namespace Arpg::Loot
{
    std::string LoadPotionData();
    // Heal override, resource restore on drink, drops on kill.
    void InstallPotions();

    // ".arpg test potion <char> <itemId>": puts one in the bags, sets health/power to half, drinks it through the real cast
    // path (cooldown included). One line: "drink item=… ok=1 heal=… power=… cooldown_ms=…" or "refused (cooldown …)".
    std::string TestDrink(Player* player, uint32 itemId);
}

#endif
