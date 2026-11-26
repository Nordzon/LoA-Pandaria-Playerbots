// Bots understand affixes (m7, owner 2026-11-26, SPEC 4.10): playerbots scores items from the template only; through the
// one hook we added to its StatsWeightCalculator (// ARPG:, ExtraItemStats) a party bot (a bot in a real player's group)
// also counts the affix stats of the item instance it carries or wears, with its own spec weights.
// v1: items the bot owns (equip decisions). Not yet: loot still on a corpse (its plan has affix ids + tiers, values roll
// at pickup).
#ifndef ARPG_BOTS_BOTITEMSCORE_H
#define ARPG_BOTS_BOTITEMSCORE_H

#include <string>

class Player;

namespace Arpg::Bots
{
    void InstallBotItemScore();
    // Test: the affix stats the hook would add for this bot's item (sum of values; 0 = none / not a party bot).
    int AffixScoreInput(Player* bot, unsigned itemId, int randomPropertyId, std::string& why);
}

#endif
