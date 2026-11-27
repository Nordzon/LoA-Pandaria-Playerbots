// Boss loot rules (SPEC 4.4, BALANCE §13, change m2-affix-loot task 4.4): items from a boss roll at the highest tier the level
// allows, and every Nth boss kill without a 5-affix item forces one (god-roll pity, per character, character_arpg_pity
// row boss_entry 0). The Unique ladder (20% +20% per run) comes with the Uniques (m3).
#ifndef ARPG_LOOT_BOSSPITY_H
#define ARPG_LOOT_BOSSPITY_H

#include "Define.h"

#include <string>

class Player;

namespace Arpg::Loot
{
    // Registers the boss-kill PlayerScript.
    void InstallBossPity();

    // Creature template is a boss: dungeon boss (encounter data) or world-boss rank.
    bool IsBossEntry(uint32 creatureEntry);

    // Kills since the character's last 5-affix boss item (god-roll pity counter).
    int GodRollKills(Player* player);
    // A 5-affix item is owed on the next epic from boss loot.
    bool GodRollPending(Player* player);
    // Called when a boss-loot item rolled 5 affixes: counter back to 0.
    void GodRollGranted(Player* player);
    // ".arpg pity set <char> godroll <n>" (tests).
    void SetGodRollKills(Player* player, int kills);
    // Unique ladder (BALANCE §13): 20% on the first kill of a boss, +20% per kill without its Unique (guaranteed by the 5th).
    int UniqueMisses(Player* player, uint32 bossEntry);
    // Counter key of the one Unique ladder all Superuniques share (owner 2026-11-27; creature entry 1 is never a boss).
    constexpr uint32 SharedSuperuniquePityKey = 1;
    void SetUniqueMisses(Player* player, uint32 bossEntry, int misses);
    // A boss kill won the ladder: the next item from that boss's loot whose slot has a Unique becomes one. Returns the boss
    // entry owed (0 = none) and clears it when `take` is true.
    uint32 PendingUnique(Player* player, bool take);
    // A kill (boss ladder, Superunique ladder) owes this player a Unique on the next item of that creature's loot.
    void SetPendingUnique(Player* player, uint32 creatureEntry, uint32 uniqueId = 0);
    // The specific Unique owed (Superunique's own), 0 = any that fits the item's slot.
    uint32 PendingUniqueId(Player* player);

    // ".arpg pity info <char>"
    std::string DescribePity(Player* player);
}

#endif
