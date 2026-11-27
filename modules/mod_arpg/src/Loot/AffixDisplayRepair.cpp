#include "Loot/AffixDisplayRepair.h"

#include "ArpgConfig.h"
#include "Loot/Uniques.h"

#include "DatabaseEnv.h"
#include "Item.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <map>
#include <vector>

namespace Arpg::Loot
{
    namespace
    {
        class AffixDisplayRepairScript : public PlayerScript
        {
        public:
            AffixDisplayRepairScript() : PlayerScript("arpg_affix_display_repair") { }

            void OnLogin(Player* player) override
            {
                if (int fixed = RepairAffixDisplay(player))
                    TC_LOG_INFO("module", "mod_arpg: %s - affix lines rewritten on %d item(s)", player->GetName().c_str(), fixed);
            }
        };
    }

    int RepairAffixDisplay(Player* player)
    {
        if (!player || !sArpgConfig.Get().displayEnabled)
            return 0;
        // display ids in position order per item (rows without one - native enchants, too many lines - take no slot)
        std::map<uint32, std::vector<uint32>> lines;
        if (QueryResult r = CharacterDatabase.PQuery(
                "SELECT a.item_guid, a.display_id FROM item_instance_arpg_affix a JOIN character_inventory ci ON ci.item = a.item_guid "
                "WHERE ci.guid = %u ORDER BY a.item_guid, a.position", player->GetGUID().GetCounter()))
            do
            {
                Field* f = r->Fetch();
                std::vector<uint32>& v = lines[f[0].GetUInt32()];
                if (uint32 d = f[1].GetUInt32())
                    v.push_back(d);
            } while (r->NextRow());
        int fixed = 0;
        for (auto const& [guid, ids] : lines)
        {
            Item* item = player->GetItemByGuid(ObjectGuid(HighGuid::Item, guid));
            if (!item)
                continue;
            // Expected: affix lines in slots 8.. in order (AffixRoller), the Unique power line in slot 12 (Uniques).
            std::map<uint32, uint32> want;
            uint32 slot = PROP_ENCHANTMENT_SLOT_0;
            for (uint32 id : ids)
                if (slot <= PROP_ENCHANTMENT_SLOT_4)
                    want[slot++] = id;
            if (uint32 u = UniqueDisplayId(UniqueOf(guid)))
                want[PROP_ENCHANTMENT_SLOT_4] = u;
            bool changed = false;
            for (auto const& [s, id] : want)
                if (item->GetEnchantmentId(EnchantmentSlot(s)) != id)
                {
                    item->SetEnchantment(EnchantmentSlot(s), id, 0, 0);
                    changed = true;
                }
            if (changed)
            {
                item->SetState(ITEM_CHANGED, player);
                ++fixed;
            }
        }
        return fixed;
    }

    void InstallAffixDisplayRepair()
    {
        new AffixDisplayRepairScript();
    }
}
