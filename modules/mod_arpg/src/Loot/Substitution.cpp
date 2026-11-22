#include "Loot/Substitution.h"
#include "Loot/AffixRoller.h"

#include "ArpgConfig.h"
#include "ArpgHooks.h"
#include "Core/LootSubstitution.h"
#include "Loot/ZoneLevels.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "DatabaseEnv.h"
#include "Log.h"
#include "LootMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    std::mutex g_lock;
    // key (class, subclass, normalised inventory type, quality) -> candidates
    std::unordered_map<uint64, std::vector<Arpg::Core::SubstituteCandidate>> g_index;

    uint32 NormalInvType(uint32 inv)
    {
        return inv == INVTYPE_ROBE ? uint32(INVTYPE_CHEST) : inv;   // robes and chests are the same slot
    }

    uint64 Key(uint32 cls, uint32 subclass, uint32 inv, uint32 quality)
    {
        return (uint64(cls) << 48) | (uint64(subclass) << 32) | (uint64(NormalInvType(inv)) << 8) | uint64(quality);
    }

    bool Eligible(ItemTemplate const* p)
    {
        return p && (p->Class == ITEM_CLASS_WEAPON || p->Class == ITEM_CLASS_ARMOR) && p->Quality >= ITEM_QUALITY_UNCOMMON &&
            p->Quality <= ITEM_QUALITY_EPIC && p->RequiredLevel >= 1 && p->InventoryType != INVTYPE_NON_EQUIP;
    }

    // The core of the swap: 0 = keep the item.
    uint32 Choose(Player* player, ItemTemplate const* proto, std::string& why)
    {
        Arpg::Settings s = sArpgConfig.Get();
        if (!Eligible(proto))
        {
            why = "not an equippable green/blue/epic";
            return 0;
        }
        int lootLevel = Arpg::Loot::LootLevelFor(player);
        if (!Arpg::Core::ShouldSubstitute(int(proto->RequiredLevel), lootLevel, s.lootSubstituteGap))
        {
            why = "close enough to loot level " + std::to_string(lootLevel);
            return 0;
        }
        std::vector<Arpg::Core::SubstituteCandidate> c;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_index.find(Key(proto->Class, proto->SubClass, proto->InventoryType, proto->Quality));
            if (it != g_index.end())
                c = it->second;
        }
        uint32 pick = Arpg::Core::PickSubstitute(c, lootLevel, [](uint32 n) { return n ? urand(0, n - 1) : 0u; });
        if (!pick || pick == proto->ItemId)
        {
            why = "no closer candidate for loot level " + std::to_string(lootLevel);
            return 0;
        }
        why = "loot level " + std::to_string(lootLevel);
        return pick;
    }

    void LootFilledHook(Loot* loot, Player* owner)
    {
        if (!loot || !owner || !sArpgConfig.Get().lootEnabled)
            return;
        if (!Arpg::Scaling::IsRealPlayer(owner) && !sArpgConfig.Get().affixBotItems)
            return;   // bots' loot stays vanilla
        for (LootItem& li : loot->items)
        {
            if (li.type != 0 || li.needs_quest)
                continue;
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(li.itemid);
            std::string why;
            if (uint32 to = Choose(owner, proto, why))
            {
                Arpg::TestEvents::Record(owner->GetGUID().GetCounter(), "substitute from=" + std::to_string(li.itemid) +
                    " to=" + std::to_string(to) + " " + why);
                li.itemid = to;
                li.randomSuffix = 0;          // candidates never have Blizzard random stats
                li.randomPropertyId = 0;
            }
        }
        // Roll at loot time (owner 2026-11-22): chests and containers are planned here; creature corpses by the last kill script
        // (it knows Champion kills and owed Uniques), see Arpg::Loot::InstallLootPlanner.
        if (loot->sourceTypeId == TYPEID_GAMEOBJECT)
            Arpg::Loot::PlanLoot(owner, *loot, ItemPickupSourceType::GameObjectLoot, loot->sourceEntry, false);
        else if (loot->sourceTypeId == TYPEID_ITEM || loot->sourceTypeId == TYPEID_CONTAINER)
            Arpg::Loot::PlanLoot(owner, *loot, ItemPickupSourceType::ItemLoot, loot->sourceEntry, false);
    }
}

namespace Arpg::Loot
{
    std::string LoadSubstitutionIndex()
    {
        std::unordered_set<uint32> drops;
        for (char const* table : { "creature_loot_template", "reference_loot_template", "gameobject_loot_template" })
            if (QueryResult r = WorldDatabase.PQuery("SELECT DISTINCT item FROM %s", table))
                do
                    drops.insert(r->Fetch()[0].GetUInt32());
                while (r->NextRow());

        std::unordered_map<uint64, std::vector<Core::SubstituteCandidate>> index;
        size_t n = 0;
        for (auto const& kv : *sObjectMgr->GetItemTemplateStore())
        {
            ItemTemplate const& p = kv.second;
            if (!Eligible(&p) || !drops.count(p.ItemId) || p.RandomProperty || p.RandomSuffix || p.ScalingStatDistribution)
                continue;
            if (p.AllowableClass != -1 && p.AllowableClass != 0)
                continue;             // class-restricted (tier tokens, class items)
            std::string const& name = p.Name1;
            if (name.find("Test") != std::string::npos || name.find("Deprecated") != std::string::npos ||
                name.find("[PH]") != std::string::npos || name.rfind("OLD", 0) == 0 || name.find("Monster -") != std::string::npos)
                continue;
            index[Key(p.Class, p.SubClass, p.InventoryType, p.Quality)].push_back({ p.ItemId, int(p.RequiredLevel) });
            ++n;
        }
        std::lock_guard<std::mutex> l(g_lock);
        g_index = std::move(index);
        std::string msg = "substitution candidates=" + std::to_string(n) + " groups=" + std::to_string(g_index.size());
        TC_LOG_INFO("server.loading", "mod_arpg: %s", msg.c_str());
        return msg;
    }

    void InstallSubstitution()
    {
        ArpgHooks::LootFilled = &LootFilledHook;
    }

    uint32 FindBaseItem(uint32 inventoryType, uint32 armorSubclass, uint32 quality, int level)
    {
        std::vector<Core::SubstituteCandidate> c;
        {
            std::lock_guard<std::mutex> l(g_lock);
            for (auto const& kv : g_index)
            {
                uint32 cls = uint32(kv.first >> 48), sub = uint32((kv.first >> 32) & 0xFFFF);
                uint32 inv = uint32((kv.first >> 8) & 0xFFFFFF), q = uint32(kv.first & 0xFF);
                if (inv != NormalInvType(inventoryType) || q != quality)
                    continue;
                if (armorSubclass && !(cls == ITEM_CLASS_ARMOR && sub == armorSubclass))
                    continue;
                c.insert(c.end(), kv.second.begin(), kv.second.end());
            }
        }
        return Core::PickSubstitute(c, level, [](uint32 n) { return n ? urand(0, n - 1) : 0u; });
    }

    uint32 SubstituteFor(Player* player, uint32 itemId, std::string& why)
    {
        return Choose(player, sObjectMgr->GetItemTemplate(itemId), why);
    }
}
