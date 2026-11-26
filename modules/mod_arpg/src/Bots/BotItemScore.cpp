#include "Bots/BotItemScore.h"

#include "ArpgConfig.h"
#include "Loot/AffixRoller.h"
#include "Scaling/WorldScaling.h"

#include "Bag.h"
#include "Group.h"
#include "Item.h"
#include "ItemPrototype.h"
#include "Player.h"

#include "StatsCollector.h"
#include "StatsWeightCalculator.h"

#include <map>
#include <vector>

namespace
{
    bool IsPartyBot(Player* bot)
    {
        if (!bot || Arpg::Scaling::IsRealPlayer(bot))
            return false;
        Group* g = bot->GetGroup();
        if (!g)
            return false;
        for (GroupReference* ref = g->GetFirstMember(); ref; ref = ref->next())
            if (Player* m = ref->GetSource())
                if (m != bot && Arpg::Scaling::IsRealPlayer(m))
                    return true;
        return false;
    }

    // The bot's own item with this entry (and suffix, if any) that has affixes: equipped first, then the bags.
    Item* FindInstance(Player* bot, uint32 itemId, int32 rp)
    {
        auto fits = [&](Item* it) -> bool
        {
            return it && it->GetEntry() == itemId && (rp == 0 || it->GetItemRandomPropertyId() == rp) &&
                !Arpg::Loot::ItemStatLines(it->GetGUID().GetCounter(), true).empty();
        };
        for (uint8 s = EQUIPMENT_SLOT_START; s < EQUIPMENT_SLOT_END; ++s)
            if (Item* it = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, s); fits(it))
                return it;
        for (uint8 s = INVENTORY_SLOT_ITEM_START; s < INVENTORY_SLOT_ITEM_END; ++s)
            if (Item* it = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, s); fits(it))
                return it;
        for (uint8 b = INVENTORY_SLOT_BAG_START; b < INVENTORY_SLOT_BAG_END; ++b)
            if (Bag* bag = bot->GetBagByPos(b))
                for (uint32 j = 0; j < bag->GetBagSize(); ++j)
                    if (Item* it = bot->GetItemByPos(b, uint8(j)); fits(it))
                        return it;
        return nullptr;
    }

    // Affix stats summed by stat type.
    std::map<uint32, int32> AffixStats(Player* bot, uint32 itemId, int32 rp, std::string& why)
    {
        std::map<uint32, int32> sum;
        if (!sArpgConfig.Get().botAffixScoring) { why = "off"; return sum; }
        if (!IsPartyBot(bot)) { why = "not a party bot"; return sum; }
        Item* it = FindInstance(bot, itemId, rp);
        if (!it) { why = "no affixed instance owned"; return sum; }
        for (Arpg::Loot::StatLine const& l : Arpg::Loot::ItemStatLines(it->GetGUID().GetCounter(), true))
            sum[l.stat] += l.value;
        why = "ok";
        return sum;
    }

    void ExtraItemStatsHook(Player* bot, uint32 itemId, int32 randomPropertyId, StatsCollector& collector)
    {
        std::string why;
        std::map<uint32, int32> stats = AffixStats(bot, itemId, randomPropertyId, why);
        if (stats.empty())
            return;
        // A blank armour template that carries only the affix stats: the collector maps each ITEM_MOD_* with the bot's
        // own rules, then StatsWeightCalculator weighs them like the template's stats.
        ItemTemplate tmp{};
        tmp.Class = ITEM_CLASS_ARMOR;
        uint32 i = 0;
        for (auto const& [stat, value] : stats)
        {
            if (i == MAX_ITEM_PROTO_STATS)
            {
                collector.CollectItemStats(&tmp);
                tmp = ItemTemplate{};
                tmp.Class = ITEM_CLASS_ARMOR;
                i = 0;
            }
            tmp.ItemStat[i].ItemStatType = stat;
            tmp.ItemStat[i].ItemStatValue = value;
            ++i;
        }
        if (i)
            collector.CollectItemStats(&tmp);
    }
}

namespace Arpg::Bots
{
    void InstallBotItemScore()
    {
        StatsWeightCalculator::ExtraItemStats = &ExtraItemStatsHook;
    }

    int AffixScoreInput(Player* bot, unsigned itemId, int randomPropertyId, std::string& why)
    {
        int total = 0;
        for (auto const& [stat, value] : AffixStats(bot, itemId, randomPropertyId, why))
            total += value;
        return total;
    }
}
