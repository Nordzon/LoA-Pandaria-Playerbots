#include "Loot/LootFilter.h"

#include "ArpgConfig.h"
#include "Core/AffixValue.h"
#include "Core/LootFilter.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "DatabaseEnv.h"
#include "Item.h"
#include "LootMgr.h"
#include "ObjectMgr.h"
#include "Player.h"

#include <mutex>
#include <unordered_map>

namespace
{
    std::mutex g_lock;
    std::unordered_map<uint32, std::vector<std::string>> g_rules;   // player guid -> rule texts in order

    char const* const DefaultRule = "quality<=0 -> sell";

    std::vector<std::string> Load(uint32 guid)
    {
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_rules.find(guid);
            if (it != g_rules.end())
                return it->second;
        }
        std::vector<std::string> rules;
        bool any = false;
        if (QueryResult r = CharacterDatabase.PQuery("SELECT rule FROM character_arpg_lootfilter WHERE guid = %u ORDER BY position", guid))
        {
            any = true;
            do
            {
                std::string rule = r->Fetch()[0].GetString();
                if (!rule.empty())   // "" = the player deleted every rule on purpose (no default then)
                    rules.push_back(rule);
            } while (r->NextRow());
        }
        if (!any)
            rules.push_back(DefaultRule);
        std::lock_guard<std::mutex> l(g_lock);
        g_rules[guid] = rules;
        return rules;
    }

    void Save(uint32 guid, std::vector<std::string> const& rules)
    {
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_rules[guid] = rules;
        }
        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        trans->PAppend("DELETE FROM character_arpg_lootfilter WHERE guid = %u", guid);
        // No rules at all: one empty row, so the next load doesn't hand out the default rule again (owner 2026-11-27: after
        // deleting every rule, a restart brought back "Grey and below: sell" and low-level corpses came up empty).
        if (rules.empty())
            trans->PAppend("INSERT INTO character_arpg_lootfilter (guid, position, rule) VALUES (%u, 0, '')", guid);
        for (size_t i = 0; i < rules.size(); ++i)
        {
            std::string esc = rules[i];
            CharacterDatabase.EscapeString(esc);
            trans->PAppend("INSERT INTO character_arpg_lootfilter (guid, position, rule) VALUES (%u, %u, '%s')", guid, uint32(i), esc.c_str());
        }
        CharacterDatabase.CommitTransaction(trans);
    }

    // Item level of what the character wears in the slot this item would go to (-1 = nothing / not gear).
    int EquippedItemLevel(Player* p, ItemTemplate const* proto)
    {
        if (!proto || (proto->Class != ITEM_CLASS_ARMOR && proto->Class != ITEM_CLASS_WEAPON))
            return -1;
        uint8 slot = p->FindEquipSlot(proto, NULL_SLOT, true);   // the slot it would go to (swap allowed)
        if (slot == NULL_SLOT)
            return -1;
        Item* it = p->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        return it ? int(it->GetTemplate()->ItemLevel) : -1;
    }
}

namespace Arpg::Loot
{
    std::vector<std::string> FilterRules(Player* player)
    {
        return Load(player->GetGUID().GetCounter());
    }

    bool AddFilterRule(Player* player, std::string const& text, std::string& error)
    {
        Core::FilterRule r;
        if (!Core::RuleFromText(text, r))
        {
            error = "bad rule: " + text + " (e.g. quality<=0 -> sell; keys quality class sub usable affixes tier vs ids; actions pickup leave sell highlight)";
            return false;
        }
        auto rules = Load(player->GetGUID().GetCounter());
        if (rules.size() >= 50)
        {
            error = "at most 50 rules";
            return false;
        }
        rules.push_back(Core::RuleToText(r));
        Save(player->GetGUID().GetCounter(), rules);
        return true;
    }

    bool RemoveFilterRule(Player* player, int position, std::string& error)
    {
        auto rules = Load(player->GetGUID().GetCounter());
        if (position < 1 || position > int(rules.size()))
        {
            error = "no rule " + std::to_string(position);
            return false;
        }
        rules.erase(rules.begin() + (position - 1));
        Save(player->GetGUID().GetCounter(), rules);
        return true;
    }

    bool MoveFilterRule(Player* player, int from, int to, std::string& error)
    {
        auto rules = Load(player->GetGUID().GetCounter());
        int n = int(rules.size());
        if (from < 1 || from > n || to < 1 || to > n)
        {
            error = "positions must be 1-" + std::to_string(n);
            return false;
        }
        std::string r = rules[from - 1];
        rules.erase(rules.begin() + (from - 1));
        rules.insert(rules.begin() + (to - 1), r);
        Save(player->GetGUID().GetCounter(), rules);
        return true;
    }

    void ResetFilter(Player* player)
    {
        Save(player->GetGUID().GetCounter(), { DefaultRule });
    }

    bool ApplyFilter(Player* player, ::Loot& loot, LootItem& li, int affixCount, int bestTier, bool unique)
    {
        if (!player || li.currency || li.is_looted)
            return true;
        if (!Arpg::Scaling::IsRealPlayer(player))
            return true;
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(li.itemid);
        if (!proto)
            return true;
        std::vector<Core::FilterRule> rules;
        for (std::string const& text : Load(player->GetGUID().GetCounter()))
        {
            Core::FilterRule r;
            if (Core::RuleFromText(text, r))
                rules.push_back(r);
        }
        Core::ItemFacts f;
        f.itemId = proto->ItemId;
        f.quality = int(proto->Quality);
        f.usable = player->CanUseItem(proto) == EQUIP_ERR_OK;
        f.itemClass = int(proto->Class);
        f.itemSubClass = int(proto->SubClass);
        f.affixCount = affixCount;
        f.bestTier = bestTier;
        f.itemLevel = int(proto->ItemLevel);
        f.equippedItemLevel = EquippedItemLevel(player, proto);
        f.quest = li.needs_quest || proto->StartQuest || proto->Class == ITEM_CLASS_QUEST;
        f.unique = unique;
        int matched = -1;
        Core::FilterAction a = Core::EvaluateFilter(rules, f, matched);
        if (a == Core::FilterAction::Pickup || a == Core::FilterAction::Highlight)
        {
            if (a == Core::FilterAction::Highlight)
                Arpg::TestEvents::Record(player->GetGUID().GetCounter(), "filter item=" + std::to_string(li.itemid) + " action=highlight rule=" + std::to_string(matched + 1));
            return true;
        }
        uint32 count = std::max<uint32>(1, li.count);
        uint64 gold = 0;
        if (a == Core::FilterAction::AutoSell)
        {
            gold = Core::SellValue(uint64(proto->SellPrice) * count, affixCount, int(proto->RequiredLevel),
                sArpgConfig.Get().sellCopperPerAffixLevel);
            if (gold)
                player->ModifyMoney(int64(gold));
        }
        li.is_looted = true;
        if (loot.unlootedCount)
            --loot.unlootedCount;
        Arpg::TestEvents::Record(player->GetGUID().GetCounter(), "filter item=" + std::to_string(li.itemid) + " action=" +
            (a == Core::FilterAction::AutoSell ? "sell gold=" + std::to_string(gold) : std::string("leave")) + " rule=" + std::to_string(matched + 1));
        return false;
    }
}
