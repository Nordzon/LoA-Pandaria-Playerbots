#include "Dungeons/InstanceScaling.h"
#include "Dungeons/Rifts.h"

#include "ArpgConfig.h"
#include "ArpgHooks.h"
#include "Core/InstanceScale.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "DatabaseEnv.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"

#include <cstdio>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace
{
    std::mutex g_lock;
    std::vector<Arpg::Core::PartyScaleRow> g_fiveMan, g_raid;
    std::unordered_map<uint32, int> g_raidTier;                                 // map -> tier
    std::unordered_map<uint32, std::unordered_map<uint32, int>> g_chosen;      // player guid -> map -> level

    Arpg::Core::SoloKnobs Knobs()
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        return { s.dungeonSoloNonTankDamage, s.dungeonSoloTankDamageDealt, s.dungeonSoloHealerDamageDealt };
    }

    // The only real (non-GM) player in a 5-man instance, else null (a party with bots is not "solo").
    Player* SoloPlayer(Map* map)
    {
        if (!map || !map->IsDungeon() || map->IsRaid())
            return nullptr;
        Player* only = nullptr;
        Map::PlayerList const& players = map->GetPlayers();
        for (auto itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* p = itr->GetSource();
            if (!p || p->IsGameMaster())
                continue;
            if (only)
                return nullptr;
            only = p;
        }
        return only;
    }

    void Rec(Map* map, std::string const& line)
    {
        if (!Arpg::TestEvents::Enabled() || !map)
            return;
        Map::PlayerList const& players = map->GetPlayers();
        for (auto itr = players.begin(); itr != players.end(); ++itr)
            if (Player* p = itr->GetSource())
                Arpg::TestEvents::Record(p->GetGUID().GetCounter(), line);
    }

    void InstanceScaleHook(Map* map, uint32 maxPlayers, uint32 playerCount, bool boss, float& health, float& damage)
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        if (!s.dungeonEnabled || !map || !map->Instanceable())
            return;
        bool const raid = map->IsRaid() || maxPlayers > 5;
        std::vector<Arpg::Core::PartyScaleRow> rows;
        int tier = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            rows = raid ? g_raid : g_fiveMan;
            auto it = g_raidTier.find(map->GetId());
            tier = it == g_raidTier.end() ? 0 : it->second;
        }
        if (rows.empty())
            return;
        float h = 1.0f, d = 1.0f;
        Arpg::Core::PartyScale(rows, int(playerCount), h, d);
        float const tierMult = raid ? Arpg::Core::RaidTierMultiplier(tier, s.dungeonRaidTierStep) : 1.0f;
        float role = 1.0f;
        if (!raid && playerCount <= 1)
            if (Player* p = SoloPlayer(map))
                role = Arpg::Core::SoloCreatureDamage(Arpg::Core::RoleForSpec(p->GetTalentSpecialization()), true, Knobs());
        health = h * tierMult;
        damage = d * tierMult * role;
        int const rift = raid ? 0 : Arpg::Dungeons::RiftScale(map, boss, health, damage);   // m9: rift level multipliers
        char buf[200];
        std::snprintf(buf, sizeof(buf), "instance scale map=%u players=%u boss=%d health=%.3f damage=%.3f tier=%d role=%.2f rift=%d",
            map->GetId(), playerCount, boss ? 1 : 0, health, damage, raid ? tier : 0, role, rift);
        // One line per change per instance (every creature of the instance goes through here: the recorder keeps 30).
        static std::unordered_map<uint32, std::string> lastLine;
        bool fresh;
        {
            std::lock_guard<std::mutex> l(g_lock);
            std::string& last = lastLine[map->GetInstanceId()];
            fresh = last != buf;
            if (fresh)
                last = buf;
        }
        if (fresh)
            Rec(map, buf);
    }

    void InstanceLevelHook(Map* map, uint8& level)
    {
        if (!sArpgConfig.Get().dungeonEnabled || !map || map->IsRaid())
            return;
        // The first real player who chose a level for this dungeon decides (party leader in practice); never above level.
        Map::PlayerList const& players = map->GetPlayers();
        for (auto itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* p = itr->GetSource();
            if (!p || p->IsGameMaster() || !Arpg::Scaling::IsRealPlayer(p))
                continue;
            int chosen = Arpg::Dungeons::ChosenLevel(p, map->GetId());
            if (chosen > 0 && chosen < int(level))
            {
                level = uint8(chosen);
                Rec(map, "instance level map=" + std::to_string(map->GetId()) + " level=" + std::to_string(chosen));
            }
            return;
        }
    }

    // Alone in a 5-man: a tank or healer deals more damage (BALANCE §5/§10).
    float SoloDealtMult(Unit* attacker)
    {
        Player* p = attacker ? attacker->ToPlayer() : nullptr;
        if (!p || !sArpgConfig.Get().dungeonEnabled)
            return 1.0f;
        Map* map = p->GetMap();
        if (SoloPlayer(map) != p)
            return 1.0f;
        return Arpg::Core::SoloPlayerDamageDealt(Arpg::Core::RoleForSpec(p->GetTalentSpecialization()), true, Knobs());
    }

    class ArpgInstanceUnitScript : public UnitScript
    {
    public:
        ArpgInstanceUnitScript() : UnitScript("arpg_instance_scaling_unit") { }

        void ModifyMeleeDamage(Unit* /*target*/, Unit* attacker, uint32& damage) override
        {
            float m = SoloDealtMult(attacker);
            if (m != 1.0f)
                damage = uint32(float(damage) * m);
        }

        void ModifySpellDamageTaken(Unit* /*target*/, Unit* attacker, int32& damage, SpellInfo const* /*spellInfo*/) override
        {
            float m = SoloDealtMult(attacker);
            if (m != 1.0f && damage > 0)
                damage = int32(float(damage) * m);
        }

        void ModifyPeriodicDamageAurasTick(Unit* /*target*/, Unit* attacker, int32& damage, SpellInfo const* /*spellInfo*/) override
        {
            float m = SoloDealtMult(attacker);
            if (m != 1.0f && damage > 0)
                damage = int32(float(damage) * m);
        }
    };
}

namespace Arpg::Dungeons
{
    std::string LoadInstanceData()
    {
        std::vector<Core::PartyScaleRow> five, raid;
        std::unordered_map<uint32, int> tiers;
        if (QueryResult r = WorldDatabase.Query("SELECT kind, players, health, damage FROM arpg_party_scale"))
            do
            {
                Field* f = r->Fetch();
                Core::PartyScaleRow row{ int(f[1].GetUInt8()), f[2].GetFloat(), f[3].GetFloat() };
                (f[0].GetUInt8() == 0 ? five : raid).push_back(row);
            } while (r->NextRow());
        if (QueryResult r = WorldDatabase.Query("SELECT map, tier FROM arpg_raid_tier"))
            do
            {
                Field* f = r->Fetch();
                tiers[f[0].GetUInt32()] = int(f[1].GetUInt8());
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_fiveMan = five;
        g_raid = raid;
        g_raidTier = tiers;
        return std::to_string(five.size() + raid.size()) + " party scale rows, " + std::to_string(tiers.size()) + " raid tiers";
    }

    void InstallInstanceScaling()
    {
        ArpgHooks::InstanceScale = &InstanceScaleHook;
        ArpgHooks::InstanceLevel = &InstanceLevelHook;
        new ArpgInstanceUnitScript();
    }

    int RaidTierOf(uint32 mapId)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_raidTier.find(mapId);
        return it == g_raidTier.end() ? 0 : it->second;
    }

    int ChosenLevel(Player* player, uint32 mapId)
    {
        uint32 guid = player->GetGUID().GetCounter();
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_chosen.find(guid);
            if (it != g_chosen.end())
            {
                auto m = it->second.find(mapId);
                return m == it->second.end() ? 0 : m->second;
            }
        }
        std::unordered_map<uint32, int> mine;
        if (QueryResult r = CharacterDatabase.PQuery("SELECT map, level FROM character_arpg_dungeon_level WHERE guid = %u", guid))
            do
            {
                Field* f = r->Fetch();
                mine[f[0].GetUInt32()] = int(f[1].GetUInt8());
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_chosen[guid] = mine;
        auto m = mine.find(mapId);
        return m == mine.end() ? 0 : m->second;
    }

    void SetChosenLevel(Player* player, uint32 mapId, int level)
    {
        uint32 guid = player->GetGUID().GetCounter();
        ChosenLevel(player, mapId);   // load
        {
            std::lock_guard<std::mutex> l(g_lock);
            if (level > 0)
                g_chosen[guid][mapId] = level;
            else
                g_chosen[guid].erase(mapId);
        }
        if (level > 0)
            CharacterDatabase.PExecute("REPLACE INTO character_arpg_dungeon_level (guid, map, level) VALUES (%u, %u, %u)", guid, mapId, uint32(level));
        else
            CharacterDatabase.PExecute("DELETE FROM character_arpg_dungeon_level WHERE guid = %u AND map = %u", guid, mapId);
    }

    std::string Describe(Player* player)
    {
        Map* map = player->GetMap();
        uint32 count = 0;
        Map::PlayerList const& players = map->GetPlayers();
        for (auto itr = players.begin(); itr != players.end(); ++itr)
            if (Player* p = itr->GetSource())
                if (!p->IsGameMaster())
                    ++count;
        Core::SpecRole role = Core::RoleForSpec(player->GetTalentSpecialization());
        char const* roleName = role == Core::SpecRole::Tank ? "tank" : role == Core::SpecRole::Healer ? "healer" : "damage";
        return "dungeon map=" + std::to_string(map->GetId()) + " instance=" + (map->Instanceable() ? "1" : "0") + " raid=" +
            (map->IsRaid() ? "1" : "0") + " players=" + std::to_string(count) + " role=" + roleName + " solo=" +
            (SoloPlayer(map) == player ? "1" : "0") + " chosen_level=" + std::to_string(ChosenLevel(player, map->GetId())) +
            " raid_tier=" + std::to_string(RaidTierOf(map->GetId())) + " dealt_mult=" + std::to_string(SoloDealtMult(player));
    }
}
