// Loot companion (SPEC 4.7, change m2b tasks 1.1/3.1-3.4): a non-combat follower (creature 7000001) that queues the
// player's kills and, out of combat, walks to each corpse and moves its loot into the player's bags through the core's
// own loot store path (affixes, planned names and the loot filter all apply). Unreachable after Arpg.LootPet.ReachTimeoutMs
// -> loots from where it is. Re-summoned after zoning, teleports and death. On/off persists (character_arpg_lootpet).
#ifndef ARPG_LOOT_LOOTPET_H
#define ARPG_LOOT_LOOTPET_H

#include "Define.h"

#include <string>

class Player;

namespace Arpg::Loot
{
    void InstallLootPet();
    // Startup / reload: arpg_companion_model.
    std::string LoadCompanionModels();
    // The companion look (arpg_companion_model.id); applied now and on every summon, saved per character.
    bool SetCompanionModel(Player* player, uint32 modelId, std::string& out);
    void SetLootPet(Player* player, bool on);
    bool LootPetOn(Player* player);
    // "lootpet on|off pet=<guid|0> queue=N looted=N"
    std::string DescribeLootPet(Player* player);
}

#endif
