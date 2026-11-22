// Dungeon / raid scaling (SPEC 4.6, change m5): replaces AutoBalance's player-count curve with arpg_party_scale x raid tier
// (arpg_raid_tier) x solo role, gives a solo tank/healer more damage, and lets a character pick the level a dungeon runs
// at (character_arpg_dungeon_level). Hooks: ArpgHooks::InstanceScale / InstanceLevel in the core's AutoBalance.
#ifndef ARPG_DUNGEONS_INSTANCESCALING_H
#define ARPG_DUNGEONS_INSTANCESCALING_H

#include "Define.h"

#include <string>

class Player;

namespace Arpg::Dungeons
{
    std::string LoadInstanceData();
    void InstallInstanceScaling();

    // Raid tier of a map (0 = not in the ladder).
    int RaidTierOf(uint32 mapId);
    // Chosen level for a dungeon (0 = own level). Setting 0 clears it.
    int ChosenLevel(Player* player, uint32 mapId);
    void SetChosenLevel(Player* player, uint32 mapId, int level);
    // "dungeon map=… players=… role=… solo=… chosen_level=… raid_tier=…"
    std::string Describe(Player* player);
}

#endif
