#include "Loot/Uniques.h"

#include "ArpgConfig.h"
#include "Test/TestEvents.h"

#include "DatabaseEnv.h"
#include "Item.h"
#include "Log.h"
#include "Player.h"
#include "Random.h"

#include <map>
#include <mutex>
#include <unordered_map>

namespace
{
    struct PowerDef
    {
        uint32 id = 0;
        std::string name;
        Arpg::Core::PowerTrigger trigger = Arpg::Core::PowerTrigger::ChanceOnHit;
        Arpg::Core::PowerKind kind = Arpg::Core::PowerKind::Damage;
        uint32 schoolMask = 1;
        float radius = 0.0f;
        uint32 maxTargets = 1;
        uint32 spellId = 0;
        std::map<int, Arpg::Core::PowerTierRange> tiers;
    };

    struct UniqueDef
    {
        uint32 id = 0;
        std::string name;
        uint32 powerId = 0;
        uint32 inventoryType = 0;
        uint32 armorSubclass = 0;
    };

    std::mutex g_lock;
    std::map<uint32, PowerDef> g_powers;
    std::map<uint32, UniqueDef> g_uniques;
    std::unordered_map<uint32, Arpg::Loot::UniqueRoll> g_items;   // item guid -> roll

    char const* Roman(int t)
    {
        static char const* const r[] = { "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII" };
        return t >= 0 && t <= 8 ? r[t] : "?";
    }
}

namespace Arpg::Loot
{
    std::string LoadUniques()
    {
        std::map<uint32, PowerDef> powers;
        std::map<uint32, UniqueDef> uniques;
        std::unordered_map<uint32, UniqueRoll> items;
        if (QueryResult r = WorldDatabase.Query("SELECT id, name, `trigger`, kind, school_mask, radius, max_targets, spell_id FROM arpg_unique_power"))
            do
            {
                Field* f = r->Fetch();
                PowerDef p;
                p.id = f[0].GetUInt32();
                p.name = f[1].GetString();
                p.trigger = Core::PowerTrigger(f[2].GetUInt8());
                p.kind = Core::PowerKind(f[3].GetUInt8());
                p.schoolMask = f[4].GetUInt32();
                p.radius = f[5].GetFloat();
                p.maxTargets = std::max<uint32>(1, f[6].GetUInt8());
                p.spellId = f[7].GetUInt32();
                powers[p.id] = p;
            } while (r->NextRow());
        if (QueryResult r = WorldDatabase.Query("SELECT power_id, tier, trigger_min, trigger_max, magnitude_min, magnitude_max, cooldown_ms FROM arpg_unique_power_tier"))
            do
            {
                Field* f = r->Fetch();
                auto it = powers.find(f[0].GetUInt32());
                if (it != powers.end())
                    it->second.tiers[int(f[1].GetUInt8())] = Core::PowerTierRange{ int(f[2].GetInt16()), int(f[3].GetInt16()),
                        int(f[4].GetInt16()), int(f[5].GetInt16()), f[6].GetUInt32() };
            } while (r->NextRow());
        if (QueryResult r = WorldDatabase.Query("SELECT id, name, power_id, inventory_type, armor_subclass FROM arpg_unique"))
            do
            {
                Field* f = r->Fetch();
                UniqueDef u;
                u.id = f[0].GetUInt32();
                u.name = f[1].GetString();
                u.powerId = f[2].GetUInt32();
                u.inventoryType = f[3].GetUInt8();
                u.armorSubclass = f[4].GetUInt8();
                uniques[u.id] = u;
            } while (r->NextRow());
        CharacterDatabase.DirectExecute("DELETE u FROM item_instance_arpg_unique u LEFT JOIN item_instance i ON i.guid = u.item_guid WHERE i.guid IS NULL");
        if (QueryResult r = CharacterDatabase.Query("SELECT item_guid, unique_id, tier, trigger_value, magnitude, perfect FROM item_instance_arpg_unique"))
            do
            {
                Field* f = r->Fetch();
                UniqueRoll u;
                u.uniqueId = f[1].GetUInt32();
                auto d = uniques.find(u.uniqueId);
                u.powerId = d == uniques.end() ? 0 : d->second.powerId;
                u.tier = int(f[2].GetUInt8());
                u.power.trigger = int(f[3].GetInt16());
                u.power.magnitude = int(f[4].GetInt16());
                u.power.perfect = f[5].GetUInt8() != 0;
                items[f[0].GetUInt32()] = u;
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_powers = std::move(powers);
        g_uniques = std::move(uniques);
        g_items = std::move(items);
        std::string msg = "uniques=" + std::to_string(g_uniques.size()) + " powers=" + std::to_string(g_powers.size()) +
            " unique_items=" + std::to_string(g_items.size());
        TC_LOG_INFO("server.loading", "mod_arpg: %s", msg.c_str());
        return msg;
    }

    uint32 UniqueForBase(uint32 inventoryType, uint32 itemClass, uint32 itemSubClass)
    {
        uint32 inv = inventoryType == INVTYPE_ROBE ? uint32(INVTYPE_CHEST) : inventoryType;
        std::vector<uint32> fits;
        {
            std::lock_guard<std::mutex> l(g_lock);
            for (auto const& kv : g_uniques)
            {
                uint32 uinv = kv.second.inventoryType == INVTYPE_ROBE ? uint32(INVTYPE_CHEST) : kv.second.inventoryType;
                if (uinv != inv)
                    continue;
                if (kv.second.armorSubclass && !(itemClass == ITEM_CLASS_ARMOR && itemSubClass == kv.second.armorSubclass))
                    continue;
                fits.push_back(kv.first);
            }
        }
        return fits.empty() ? 0 : fits[urand(0, uint32(fits.size()) - 1)];
    }

    uint32 RandomUniqueId()
    {
        std::lock_guard<std::mutex> l(g_lock);
        if (g_uniques.empty())
            return 0;
        auto it = g_uniques.begin();
        std::advance(it, urand(0, uint32(g_uniques.size()) - 1));
        return it->first;
    }

    bool UniqueSlot(uint32 uniqueId, uint32& inventoryType, uint32& armorSubclass)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_uniques.find(uniqueId);
        if (it == g_uniques.end())
            return false;
        inventoryType = it->second.inventoryType;
        armorSubclass = it->second.armorSubclass;
        return true;
    }

    bool MakeUnique(Player* player, Item* item, uint32 uniqueId, int tier, std::string& message)
    {
        UniqueDef def;
        PowerDef power;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto u = g_uniques.find(uniqueId);
            if (u == g_uniques.end())
            {
                message = "unknown unique " + std::to_string(uniqueId);
                return false;
            }
            def = u->second;
            auto p = g_powers.find(def.powerId);
            if (p == g_powers.end() || !p->second.tiers.count(tier))
            {
                message = "power " + std::to_string(def.powerId) + " has no tier " + std::to_string(tier);
                return false;
            }
            power = p->second;
        }
        Core::PowerTierRange range = power.tiers[tier];
        UniqueRoll roll;
        roll.uniqueId = uniqueId;
        roll.powerId = power.id;
        roll.tier = tier;
        roll.power = Core::RollPower(range, power.trigger, []() { return rand_norm(); });

        uint32 guid = item->GetGUID().GetCounter();
        CharacterDatabase.DirectPExecute("REPLACE INTO item_instance_arpg_unique (item_guid, unique_id, tier, trigger_value, magnitude, perfect) "
            "VALUES (%u, %u, %u, %d, %d, %u)", guid, uniqueId, uint32(tier), roll.power.trigger, roll.power.magnitude, roll.power.perfect ? 1u : 0u);
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_items[guid] = roll;
        }

        Arpg::Settings s = sArpgConfig.Get();
        if (s.displayEnabled)
        {
            // Power line: id = UniqueBase + power*1000 + tier*100 + combo (trigger step x magnitude step), the order the
            // client generator writes (arpg/client-patch/build-client-patch.py).
            int lo = std::min(range.triggerMin, range.triggerMax);
            int nMag = (range.magnitudeMax - range.magnitudeMin) / 5 + 1;
            int combo = (roll.power.trigger - lo) * nMag + (roll.power.magnitude - range.magnitudeMin) / 5;
            if (combo >= 0 && combo < 100)
                item->SetEnchantment(PROP_ENCHANTMENT_SLOT_4, s.displayUniqueBase + power.id * 1000 + uint32(tier) * 100 + uint32(combo), 0, 0);
            // Name: "<base> - <Unique name>" until the Item-sparse patch gives Uniques their own names (m6).
            item->SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID, -int32(s.displaySuffixBase + 1000 + uniqueId));
        }
        item->SetState(ITEM_CHANGED, player);
        Arpg::TestEvents::Record(player->GetGUID().GetCounter(), "unique made " + DescribeUnique(roll));
        message = DescribeUnique(roll);
        return true;
    }

    bool PowerFor(uint32 itemGuid, ActivePower& out)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_items.find(itemGuid);
        if (it == g_items.end())
            return false;
        auto p = g_powers.find(it->second.powerId);
        if (p == g_powers.end())
            return false;
        auto t = p->second.tiers.find(it->second.tier);
        out.itemGuid = itemGuid;
        out.powerId = p->second.id;
        out.name = p->second.name;
        out.trigger = p->second.trigger;
        out.kind = p->second.kind;
        out.triggerValue = it->second.power.trigger;
        out.magnitude = it->second.power.magnitude;
        out.cooldownMs = t == p->second.tiers.end() ? 0 : t->second.cooldownMs;
        out.radius = p->second.radius;
        out.maxTargets = p->second.maxTargets;
        out.schoolMask = p->second.schoolMask;
        out.spellId = p->second.spellId;
        return true;
    }

    UniqueRoll UniqueOf(uint32 itemGuid)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_items.find(itemGuid);
        return it == g_items.end() ? UniqueRoll() : it->second;
    }

    std::string DescribeUnique(UniqueRoll const& u)
    {
        if (!u.uniqueId)
            return "unique=none";
        std::string name;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_uniques.find(u.uniqueId);
            name = it == g_uniques.end() ? "?" : it->second.name;
        }
        return "unique=" + std::to_string(u.uniqueId) + " \"" + name + "\" tier=" + Roman(u.tier) + " trigger=" +
            std::to_string(u.power.trigger) + " magnitude=" + std::to_string(u.power.magnitude) + (u.power.perfect ? " perfect" : "");
    }
}
