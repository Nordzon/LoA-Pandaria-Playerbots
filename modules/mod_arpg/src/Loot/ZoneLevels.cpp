#include "Loot/ZoneLevels.h"

#include "ArpgConfig.h"

#include "DatabaseEnv.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"

#include <mutex>
#include <unordered_map>

namespace
{
    std::mutex g_lock;
    std::unordered_map<uint32, Arpg::Core::ZoneLevels> g_zones;
}

namespace Arpg::Loot
{
    std::string LoadZoneLevels()
    {
        std::unordered_map<uint32, Core::ZoneLevels> zones;
        if (QueryResult r = WorldDatabase.Query("SELECT zone_id, min_level, max_level FROM arpg_zone_level"))
            do
            {
                Field* f = r->Fetch();
                zones[f[0].GetUInt32()] = Core::ZoneLevels{ int(f[1].GetUInt8()), int(f[2].GetUInt8()) };
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_zones = std::move(zones);
        std::string msg = "zones=" + std::to_string(g_zones.size());
        TC_LOG_INFO("server.loading", "mod_arpg: zone levels %s", msg.c_str());
        return msg;
    }

    Core::ZoneLevels ZoneLevelsFor(uint32 zoneId, uint32 areaId)
    {
        Arpg::Settings s = sArpgConfig.Get();
        if (s.uncappedZones.count(zoneId) || (areaId && s.uncappedZones.count(areaId)))
            return Core::ZoneLevels{};
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_zones.find(zoneId);
        if ((it == g_zones.end() || it->second.maxLevel <= 0) && areaId)
            it = g_zones.find(areaId);
        return it == g_zones.end() ? Core::ZoneLevels{} : it->second;
    }

    int LootLevelFor(Player* player)
    {
        Arpg::Settings s = sArpgConfig.Get();
        bool instance = player->GetMap() && player->GetMap()->Instanceable();
        return Core::LootLevel(int(player->GetLevel()), ZoneLevelsFor(player->GetZoneId(), player->GetAreaId()),
            s.lootZoneCeilingMargin, instance);
    }
}
