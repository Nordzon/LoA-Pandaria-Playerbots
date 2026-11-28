// Rifts (change m9-rifts): timed, levelled runs of any 5-man dungeon. A character opens a rift (level 1 .. its key) and the
// next 5-man dungeon it enters runs as that rift: monsters x growth^level (+ Fortified / Tyrannical), a par time
// (arpg_rift_dungeon), deaths add seconds; killing the final boss (instance_encounters.lastEncounterDungeon) finishes it,
// moves the key (+3/+2/+1/0 by time) and gives rewards. Key per character: character_arpg_rift.
#ifndef ARPG_DUNGEONS_RIFTS_H
#define ARPG_DUNGEONS_RIFTS_H

#include "Define.h"

#include <string>

class Map;
class Player;

namespace Arpg::Dungeons
{
    std::string LoadRiftData();
    void InstallRifts();

    // Called by the instance-scale hook: multiplies health/damage for an active rift on this map. Returns the rift level
    // (0 = no rift).
    int RiftScale(Map* map, bool boss, float& health, float& damage);

    // Opens a rift at `level` for the character (1 .. key). out = reply text.
    bool OpenRift(Player* player, int level, std::string& out);
    // Clears a pending rift and abandons the character's active run (no reward).
    std::string CloseRift(Player* player);
    // Finishes the character's active run as if the final boss died after elapsedS seconds (tests; real runs use the clock).
    bool FinishRift(Player* player, int elapsedS, std::string& out);
    // Tests: one more death in the character's active run.
    bool AddRiftDeath(Player* player, std::string& out);
    int KeyLevel(Player* player);
    void SetKeyLevel(Player* player, int key);
    // "rift key=.. best=.. runs=.. pending=.. active=<0|1> [level=.. map=.. elapsed=.. par=.. deaths=.. affix=.. health=.. damage=..]"
    std::string DescribeRift(Player* player);
    // Addon line "ARPGR|key|best|pending|active|level|elapsed|par|deaths|affix".
    std::string DataLine(Player* player);
    // Companion menu: the levels to offer (key first, then lower ones), up to 5.
    std::string OfferedLevels(Player* player);
}

#endif
