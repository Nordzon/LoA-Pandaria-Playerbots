// Shrines (change m9-shrines): a real player's kill sometimes raises a shrine at the corpse (Arpg.Shrine.*); using it gives a
// timed buff (arpg_shrine: an existing client spell with our amount and duration; Fortune adds magic/gold find lines) to the
// user and the group nearby. Game objects 800201-800206 (script arpg_shrine).
#ifndef ARPG_WORLD_SHRINES_H
#define ARPG_WORLD_SHRINES_H

#include "Define.h"

#include <string>

class GameObject;
class Player;

namespace Arpg::World
{
    std::string LoadShrineData();
    void InstallShrines();

    // Raises shrine `id` (0 = random by weight) next to the player. Returns the object or null (why says).
    GameObject* SpawnShrine(Player* player, uint32 id, float x, float y, float z, std::string& why);
    GameObject* SpawnShrineNear(Player* player, uint32 id, std::string& why);
    // The nearest of our shrines within `range` of the player, or null.
    GameObject* NearestShrine(Player* player, float range);
    // Uses a shrine (the object script calls this): buff to the user + group nearby, shrine removed. False if not a shrine.
    bool UseShrine(Player* player, GameObject* go, std::string& out);
    // "shrines nearby=<n> [nearest=<entry> dist=<yd>] | buff id=.. name=.. amount=.. left=..s | ..."
    std::string Describe(Player* player);
    // Tests: removes every shrine buff now (their effect lines go with the next tick).
    int ClearBuffs(Player* player);
}

#endif
