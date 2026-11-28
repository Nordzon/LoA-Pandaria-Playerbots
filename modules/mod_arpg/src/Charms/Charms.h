// Charms and the Charm Pouch (change m9-charms, SPEC 4.4). Every character gets one Charm Pouch (a soul-bag-type container,
// BagFamily 4, so only charms fit); charms grant their lines only while they sit in an equipped pouch. Small/Large/Grand charms
// roll 1/2/3 lines from arpg_charm_affix when they reach a real player's bags; Unique charms have fixed lines
// (arpg_charm_unique). Lines are stored per item (item_instance_arpg_charm) and cached in memory.
#ifndef ARPG_CHARMS_CHARMS_H
#define ARPG_CHARMS_CHARMS_H

#include "Define.h"

#include <string>
#include <vector>

class Creature;
class Item;
class Player;

namespace Arpg::Charms
{
    uint32 const PouchEntry = 720000;
    uint32 const SmallCharm = 720001, LargeCharm = 720002, GrandCharm = 720003;
    uint32 const HeartOfThePandaren = 720010, GoblinsLuckyCoin = 720011, RiftwalkersSigil = 720012;

    // Startup and .arpg reload: line tables + every charm's lines into memory; removes lines of items that are gone.
    std::string LoadCharmData();
    // Pouch grant (login), pouch scan (update), charm drops (kills), logout cleanup.
    void InstallCharms();

    bool IsCharm(uint32 entry);
    // A random charm entry by the size weights (Arpg.Charms.Weight*).
    uint32 RandomCharmEntry();
    // Lines for a charm that just reached the player's bags (pickup hook). 0 = not a charm or already rolled.
    int RollCharm(Player* player, Item* item, std::string& summary);
    // The player has (or had: bags, bank, mail is not checked) this Unique charm.
    bool HasCharm(Player* player, uint32 entry);

    // Gives the pouch if the character has none. Returns "granted=0" or "granted=1 where=bagslot|bags|mail".
    std::string EnsurePouch(Player* player);
    // Tests: puts the character's pouch into a free bag slot. False + why if it can't.
    bool EquipPouch(Player* player, std::string& why);
    // Tests: moves a charm (low guid) into the equipped pouch (in = true) or to the backpack.
    bool MoveCharm(Player* player, uint32 itemLow, bool in, std::string& why);
    // Applies/removes lines now (normally every Arpg.Charms.ScanMs).
    void Rescan(Player* player);
    // "charms pouch=<0|1> equipped=<0|1> active=<n> sta=+N mf=+N ..." (stats in points, effects in hundredths of a percent).
    std::string Describe(Player* player);
    // "lines=<n> <name>=<value> ..." for one charm item (low guid), from the cache.
    std::string DescribeItem(uint32 itemLow);
}

#endif
