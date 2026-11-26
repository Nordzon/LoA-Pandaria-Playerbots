#include "Loot/MountShop.h"

#include "ArpgConfig.h"
#include "Test/TestEvents.h"

#include "DatabaseEnv.h"
#include "Player.h"
#include "SpellMgr.h"

#include <mutex>

namespace
{
    struct Row { uint32 spellId; uint32 itemId; std::string name; uint32 priceGold; };
    std::mutex g_lock;
    std::vector<Row> g_rows;   // shop order

    uint32 PriceOf(Row const& r)
    {
        return r.priceGold ? r.priceGold : sArpgConfig.Get().mountShopPriceGold;
    }
}

namespace Arpg::Loot
{
    std::string LoadMountShop()
    {
        std::vector<Row> rows;
        if (QueryResult r = WorldDatabase.Query("SELECT spell_id, item_id, name, price_gold FROM arpg_mount_shop ORDER BY sort, spell_id"))
            do
            {
                Field* f = r->Fetch();
                Row row{ f[0].GetUInt32(), f[1].GetUInt32(), f[2].GetString(), f[3].GetUInt32() };
                if (sSpellMgr->GetSpellInfo(row.spellId))
                    rows.push_back(row);
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_rows = std::move(rows);
        return "mountshop=" + std::to_string(g_rows.size());
    }

    std::vector<ShopMount> MountsForSale(Player* player)
    {
        std::vector<Row> rows;
        {
            std::lock_guard<std::mutex> l(g_lock);
            rows = g_rows;
        }
        std::vector<ShopMount> out;
        for (Row const& r : rows)
            if (!player->HasSpell(r.spellId))
                out.push_back({ r.spellId, r.itemId, r.name, PriceOf(r) });
        return out;
    }

    bool BuyMount(Player* player, uint32 spellId, std::string& out)
    {
        if (!sArpgConfig.Get().mountShopEnabled)
        {
            out = "the mount shop is closed";
            return false;
        }
        Row row{};
        bool found = false;
        {
            std::lock_guard<std::mutex> l(g_lock);
            for (Row const& r : g_rows)
                if (r.spellId == spellId)
                {
                    row = r;
                    found = true;
                    break;
                }
        }
        if (!found)
        {
            out = "not sold here";
            return false;
        }
        if (player->HasSpell(spellId))
        {
            out = "you already have " + row.name;
            return false;
        }
        uint64 copper = uint64(PriceOf(row)) * 10000;
        if (!player->HasEnoughMoney(copper))
        {
            out = "not enough gold: " + row.name + " costs " + std::to_string(PriceOf(row)) + " gold";
            return false;
        }
        player->ModifyMoney(-int64(copper));
        player->LearnSpell(spellId, false);
        out = "bought mount spell=" + std::to_string(spellId) + " (" + row.name + ") for gold=" + std::to_string(PriceOf(row));
        Arpg::TestEvents::Record(player->GetGUID().GetCounter(), out);
        return true;
    }
}
