// Treasure Goblins (change m9-goblins, SPEC 4.3): a real player's open-world kill sometimes summons a goblin (creature
// 1000010) that never fights, flees when hit or approached and escapes Arpg.Goblin.EscapeSeconds after the first hit;
// killed, it drops gold, affixed gear, essences, maybe a catalyst, a charm and the Unique charm Goblin's Lucky Coin.
#ifndef ARPG_MONSTERS_TREASUREGOBLIN_H
#define ARPG_MONSTERS_TREASUREGOBLIN_H

#include "Define.h"

#include <string>

class Creature;
class Player;

namespace Arpg::Monsters
{
    uint32 const TreasureGoblinEntry = 1000010;

    void InstallTreasureGoblins();
    // Summons a goblin for the player (15 yd away). Null + why on failure.
    Creature* SpawnGoblin(Player* player, std::string& why);
    // The player's last goblin (alive or its corpse) on the player's map, or null.
    Creature* LastGoblinOf(Player* player);
    // "goblin alive=.. fleeing=.. engaged=.. escape_in=..s level=.. hp=.." or "goblin alive=0 escaped=.. killed=.."
    std::string DescribeGoblin(Player* player);
}

#endif
