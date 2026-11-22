// Champions (SPEC 4.3, BALANCE §14, change m3 tasks 3.1/3.3): an open-world creature first seen by a real player becomes a
// Champion with Arpg.Champion.Chance %: level / health / damage / monster affixes / minions from the player's level band
// (arpg_monster_band, arpg_monster_affix, Core/MonsterBands). Kill: health orb, loot floor (>= 1 green, Arpg.Champion.
// LootBlueChance % blue), items looted from it roll +1 affix tier.
#ifndef ARPG_MONSTERS_CHAMPIONS_H
#define ARPG_MONSTERS_CHAMPIONS_H

#include "Define.h"

#include <string>
#include <vector>

class Creature;
class Player;
struct Loot;

namespace Arpg::Monsters
{
    // Startup / .arpg reload: bands and monster affixes. Returns a one-line summary.
    std::string LoadChampionData();
    // Kill script (orb, loot floor) + the scaling tick's new-candidate roll.
    void InstallChampions();

    // Turns `c` into a Champion for `player` (band by the player's level) and summons its minions.
    // forcedAffixes (tests): use exactly these arpg_monster_affix ids instead of the band's roll.
    bool MakeChampion(Creature* c, Player* player, std::string& out, std::vector<uint32> const& forcedAffixes = {});

    bool IsChampion(Creature* c);
    // The original creature entry of a tag copy (Champion / Superunique), or the entry itself.
    uint32 BaseEntry(uint32 entry);
    // Tests: the last Champion made for this player (alive or its corpse), on the player's map; null if gone.
    Creature* LastChampionOf(Player* player);
    // The player is looting a killed Champion (its items roll +1 tier).
    bool IsLootingChampion(Player* player);
    // This creature just died as a Champion (its loot gets +1 tier).
    bool WasChampionKill(Creature* c);

    // Shared with Superuniques: armour type the class wears, a droppable item of a random slot at the loot level, and an
    // extra item added to a corpse's loot (before the loot window opens).
    uint32 ArmorSubclassFor(Player* p);
    uint32 FloorItem(Player* p, uint32 quality);
    void AddLootItem(::Loot& loot, uint32 itemId);

    // "champion entry=… level=… band=… hp=… minions=… affixes=…" or "not a champion".
    std::string DescribeChampion(Creature* c);
}

#endif
