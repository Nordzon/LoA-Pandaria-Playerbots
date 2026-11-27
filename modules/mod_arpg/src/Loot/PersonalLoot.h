// Personal loot with friends (m8-friends, owner 2026-11-27 "as recommended"): when a group with several real players kills
// a monster, the killer gets the corpse as usual and every OTHER real player of the group nearby gets their own roll of it -
// own loot-table roll (level-appropriate for them), own affixes / blank chance, own Champion and Superunique extras with their
// own Unique counters - straight into the bags (mailed when full). No rolling against mates. Arpg.Party.PersonalLoot.
#ifndef ARPG_LOOT_PERSONALLOOT_H
#define ARPG_LOOT_PERSONALLOOT_H

#include "Define.h"
#include <vector>

class Creature;
class Player;

namespace Arpg::Loot
{
    // The other real players of the killer's group who get their own loot from this kill (empty: solo, bots only, feature off,
    // the killer is a bot). Kill scripts use it to give each of them their own extras.
    std::vector<Player*> PersonalRecipients(Player* killer, Creature* killed);
    // An extra item for `member` from this kill (Champion/Superunique floor item, owed Unique's base, potion, catalyst): it is
    // delivered with their loot-table roll by the last kill script.
    void AddPersonalItem(Player* member, Creature* killed, uint32 itemId);
    // Registers the kill script that rolls and delivers personal loot. Call after every other kill script is installed.
    void InstallPersonalLoot();
}

#endif
