// Gives a new item straight to a character (rift rewards, charm pouch, test commands): into the bags, else by mail, with the
// same affix / charm roll a loot pickup gets (changes m9-*).
#ifndef ARPG_LOOT_GIVEITEM_H
#define ARPG_LOOT_GIVEITEM_H

#include "Define.h"

#include <string>

class Item;
class Player;

namespace Arpg::Loot
{
    // Creates `count` of itemId for the player; rolls it as creature loot of sourceEntry (0 = plain); mails it when the bags
    // are full (subject/body of that mail). Returns the item (in the bags or mailed), null if the item id is unknown.
    // mailed = true when it went by mail.
    Item* GiveItem(Player* player, uint32 itemId, uint32 count, uint32 sourceEntry, std::string const& mailSubject,
                   bool& mailed, bool roll = true);
}

#endif
