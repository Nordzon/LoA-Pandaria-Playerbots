#include "Loot/Salvage.h"

#include "ArpgConfig.h"
#include "Core/Salvage.h"
#include "Loot/Crafting.h"
#include "Loot/Uniques.h"
#include "Test/TestEvents.h"

#include "Bag.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Player.h"

#include <unordered_map>
#include <vector>

namespace
{
    uint32 const SalvagedEssence = 700010;
    int const AscendedTier = 6;   // T6-T8 are Ascended

    struct Candidate
    {
        uint8 bag;
        uint8 slot;
        Item* item;
        int essences;
    };

    Arpg::Core::SalvageRules Rules()
    {
        Arpg::Settings s = sArpgConfig.Get();
        Arpg::Core::SalvageRules r;
        r.essencePerAffix = s.salvageEssencePerAffix;
        r.uniqueBonus = s.salvageUniqueBonus;
        r.ascendedBonus = s.salvageAscendedBonus;
        r.maxQuality = s.salvageMaxQuality;
        r.includeUniques = s.salvageIncludeUniques;
        return r;
    }

    // Every item in the backpack and the equipped bags (equipped gear and the bank are never touched).
    std::vector<Candidate> Collect(Player* player)
    {
        std::vector<Candidate> all;
        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
            if (Item* it = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                all.push_back({ INVENTORY_SLOT_BAG_0, i, it, 0 });
        for (uint8 b = INVENTORY_SLOT_BAG_START; b < INVENTORY_SLOT_BAG_END; ++b)
            if (Bag* bag = player->GetBagByPos(b))
                for (uint32 j = 0; j < bag->GetBagSize(); ++j)
                    if (Item* it = player->GetItemByPos(b, uint8(j)))
                        all.push_back({ b, uint8(j), it, 0 });
        if (all.empty())
            return all;

        // Affix count and highest tier per item, one query.
        std::string ids;
        for (Candidate const& c : all)
            ids += (ids.empty() ? "" : ",") + std::to_string(c.item->GetGUID().GetCounter());
        std::unordered_map<uint32, std::pair<int, int>> affixes;   // guid -> (count, max tier)
        if (QueryResult r = CharacterDatabase.PQuery("SELECT item_guid, COUNT(*), MAX(tier) FROM item_instance_arpg_affix "
            "WHERE item_guid IN (%s) GROUP BY item_guid", ids.c_str()))
            do
            {
                Field* f = r->Fetch();
                affixes[f[0].GetUInt32()] = { int(f[1].GetInt64()), int(f[2].GetUInt8()) };
            } while (r->NextRow());

        Arpg::Core::SalvageRules rules = Rules();
        std::vector<Candidate> out;
        for (Candidate c : all)
        {
            uint32 guid = c.item->GetGUID().GetCounter();
            auto a = affixes.find(guid);
            Arpg::Core::SalvageItem si;
            si.quality = int(c.item->GetTemplate()->Quality);
            si.affixCount = a != affixes.end() ? a->second.first : 0;
            si.ascended = a != affixes.end() && a->second.second >= AscendedTier;
            si.unique = Arpg::Loot::UniqueOf(guid).uniqueId != 0;
            c.essences = Arpg::Core::EssencesFor(si, rules);
            if (c.essences > 0)
                out.push_back(c);
        }
        return out;
    }
}

namespace Arpg::Loot
{
    SalvagePreview PreviewSalvage(Player* player)
    {
        SalvagePreview p;
        for (Candidate const& c : Collect(player))
        {
            ++p.items;
            p.essences += c.essences;
        }
        return p;
    }

    SalvagePreview DoSalvage(Player* player, std::string& out)
    {
        SalvagePreview p;
        for (Candidate const& c : Collect(player))
        {
            ++p.items;
            p.essences += c.essences;
            player->DestroyItem(c.bag, c.slot, true);
        }
        if (p.essences > 0 && !player->AddItem(SalvagedEssence, uint32(p.essences)))
            out = "salvaged, but the essences didn't fit (bags full)";
        else
            out = "salvaged items=" + std::to_string(p.items) + " essences=" + std::to_string(p.essences);
        Arpg::TestEvents::Record(player->GetGUID().GetCounter(), out);
        return p;
    }

    int CatalystEssenceCost(int band)
    {
        Arpg::Settings s = sArpgConfig.Get();
        switch (band)
        {
            case 1: return s.catalystCostLesser;
            case 2: return s.catalystCostGreater;
            case 3: return s.catalystCostGrand;
            default: return 0;
        }
    }

    bool BuyCatalyst(Player* player, int band, std::string& out)
    {
        uint32 item = CatalystItemForBand(band);
        int cost = CatalystEssenceCost(band);
        if (!item || cost <= 0)
        {
            out = "no such catalyst";
            return false;
        }
        if (!player->HasItemCount(SalvagedEssence, uint32(cost)))
        {
            out = "not enough essences: need " + std::to_string(cost) + ", have " +
                std::to_string(player->GetItemCount(SalvagedEssence));
            return false;
        }
        ItemPosCountVec dest;
        if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item, 1) != EQUIP_ERR_OK)
        {
            out = "no room in the bags";
            return false;
        }
        player->DestroyItemCount(SalvagedEssence, uint32(cost), true);
        player->AddItem(item, 1);
        out = "bought catalyst item=" + std::to_string(item) + " for essences=" + std::to_string(cost);
        Arpg::TestEvents::Record(player->GetGUID().GetCounter(), out);
        return true;
    }
}
