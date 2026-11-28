#include "Dungeons/Rifts.h"

#include "ArpgConfig.h"
#include "Charms/Charms.h"
#include "Core/Rifts.h"
#include "Loot/GiveItem.h"
#include "Monsters/Champions.h"
#include "Paragon/Paragon.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "Chat.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Map.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "Timer.h"
#include "World.h"

#include <cstdio>
#include <mutex>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    struct Key
    {
        int key = 1;
        int best = 0;
        uint32 runs = 0;
        bool loaded = false;
    };
    struct Run
    {
        uint32 owner = 0;
        int level = 0;
        uint32 map = 0;
        uint32 startMs = 0;
        uint32 parS = 0;
        int deaths = 0;
        Arpg::Core::RiftAffix affix = Arpg::Core::RiftAffix::None;
        int lastMilestone = 0;
        bool done = false;
    };

    std::mutex g_lock;
    std::unordered_map<uint32, Key> g_keys;              // player low
    std::unordered_map<uint32, int> g_pending;           // player low -> level
    std::unordered_map<uint32, Run> g_runs;              // instance id
    std::unordered_map<uint32, uint32> g_par;            // map -> par seconds
    std::unordered_set<uint32> g_finalBosses;            // creature entries
    std::unordered_map<uint32, uint32> g_nextTick;       // player low -> ms

    void Rec(Player* p, std::string const& line)
    {
        if (p && Arpg::TestEvents::Enabled())
            Arpg::TestEvents::Record(p->GetGUID().GetCounter(), line);
    }

    void Say(Player* p, std::string const& text)
    {
        if (p && p->GetSession())
            ChatHandler(p->GetSession()).PSendSysMessage("|cff66ccff%s|r", text.c_str());
    }

    std::string Clock(uint32 s)
    {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%u:%02u", s / 60, s % 60);
        return buf;
    }

    Key& KeyOf(uint32 low)   // g_lock held
    {
        Key& k = g_keys[low];
        if (!k.loaded)
        {
            k.loaded = true;
            if (QueryResult r = CharacterDatabase.PQuery("SELECT key_level, best_level, runs FROM character_arpg_rift WHERE guid = %u", low))
            {
                Field* f = r->Fetch();
                k.key = std::max(1, int(f[0].GetUInt16()));
                k.best = int(f[1].GetUInt16());
                k.runs = f[2].GetUInt32();
            }
        }
        return k;
    }

    void SaveKey(uint32 low, Key const& k)
    {
        CharacterDatabase.PExecute("REPLACE INTO character_arpg_rift (guid, key_level, best_level, runs) VALUES (%u, %u, %u, %u)",
            low, uint32(k.key), uint32(k.best), k.runs);
    }

    bool IsFiveMan(Map* map)
    {
        return map && map->IsDungeon() && !map->IsRaid() && !map->IsBattlegroundOrArena();
    }

    uint32 ParFor(uint32 mapId)   // g_lock held
    {
        auto it = g_par.find(mapId);
        return it != g_par.end() ? it->second : sArpgConfig.Get().m9.riftDefaultParMin * 60;
    }

    // The run of the instance the player is in (copy); false if none.
    bool RunOf(Player* p, Run& out, uint32& instanceId)
    {
        Map* map = p->GetMap();
        if (!IsFiveMan(map))
            return false;
        instanceId = map->GetInstanceId();
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_runs.find(instanceId);
        if (it == g_runs.end() || it->second.done)
            return false;
        out = it->second;
        return true;
    }

    void Rewards(Player* p, int level, bool late, uint32 bossEntry, std::string& summary)
    {
        Arpg::M9Settings const m = sArpgConfig.Get().m9;
        int const n = Arpg::Core::RiftRewardItems(level, m.riftRewardItems, m.riftRewardItemsMax, late);
        float const epic = Arpg::Core::RiftEpicChance(level, m.riftEpicBasePct, m.riftEpicStepPct, m.riftEpicCapPct);
        int given = 0, mailedN = 0;
        for (int i = 0; i < n; ++i)
        {
            uint32 q = float(rand_chance()) < epic ? ITEM_QUALITY_EPIC : ITEM_QUALITY_RARE;
            uint32 id = Arpg::Monsters::FloorItem(p, q);
            bool mailed = false;
            if (id && Arpg::Loot::GiveItem(p, id, 1, bossEntry, "Rift reward", mailed))
            {
                ++given;
                mailedN += mailed ? 1 : 0;
            }
        }
        uint64 gold = uint64(m.riftGoldPerLevel) * uint64(level) * uint64(p->GetLevel());
        if (late)
            gold /= 2;
        if (gold)
            p->ModifyMoney(int64(gold));
        uint64 xp = 0;
        if (p->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) && m.riftParagonXpPerLevel)
        {
            xp = uint64(m.riftParagonXpPerLevel) * uint64(level) / (late ? 2 : 1);
            Arpg::Paragon::AddXp(p, xp, "rift");
        }
        std::string extra;
        if (!late && float(rand_chance()) < m.riftCharmPct + m.riftCharmStepPct * float(level))
        {
            bool mailed = false;
            if (Arpg::Loot::GiveItem(p, Arpg::Charms::RandomCharmEntry(), 1, 0, "Rift reward", mailed))
                extra += " charm=1";
        }
        if (!late && level >= m.riftUniqueCharmLevel && float(rand_chance()) < m.riftUniqueCharmPct &&
            !Arpg::Charms::HasCharm(p, Arpg::Charms::RiftwalkersSigil))
        {
            bool mailed = false;
            if (Arpg::Loot::GiveItem(p, Arpg::Charms::RiftwalkersSigil, 1, 0, "Rift reward", mailed))
                extra += " sigil=1";
        }
        summary = "items=" + std::to_string(given) + " mailed=" + std::to_string(mailedN) + " gold=" + std::to_string(gold) +
            " paragon_xp=" + std::to_string(xp) + extra;
    }

    // Ends a run: key, best, rewards for every real player in the instance.
    void Finish(Map* map, uint32 instanceId, uint32 elapsedS, uint32 bossEntry, std::string& out)
    {
        Run run;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_runs.find(instanceId);
            if (it == g_runs.end() || it->second.done)
            {
                out = "no active rift";
                return;
            }
            run = it->second;
            g_runs.erase(it);   // done: the instance goes back to normal (it may be re-used later)
        }
        Arpg::M9Settings const m = sArpgConfig.Get().m9;
        uint32 const eff = Arpg::Core::RiftEffectiveSeconds(elapsedS, run.deaths, m.riftDeathPenaltyS);
        int const up = Arpg::Core::RiftUpgrade(eff, run.parS);
        bool const late = up == 0;
        Map::PlayerList const& players = map->GetPlayers();
        for (auto itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* p = itr->GetSource();
            if (!p || !Arpg::Scaling::IsRealPlayer(p))
                continue;
            uint32 const low = p->GetGUID().GetCounter();
            Key k;
            {
                std::lock_guard<std::mutex> l(g_lock);
                Key& kk = KeyOf(low);
                if (up > 0)
                    kk.key = std::max(kk.key, run.level + up);
                if (!late)
                    kk.best = std::max(kk.best, run.level);
                ++kk.runs;
                k = kk;
            }
            SaveKey(low, k);
            std::string rewards;
            Rewards(p, run.level, late, bossEntry, rewards);
            std::string line = "Rift level " + std::to_string(run.level) + " complete in " + Clock(eff) + " (par " + Clock(run.parS) +
                (run.deaths ? ", " + std::to_string(run.deaths) + " deaths" : "") + "): " +
                (late ? std::string("too late - the key stays at ") + std::to_string(k.key) + "." :
                        "key +" + std::to_string(up) + " -> " + std::to_string(k.key) + ".");
            Say(p, line);
            std::string ev = "rift finish level=" + std::to_string(run.level) + " elapsed=" + std::to_string(elapsedS) + " effective=" +
                std::to_string(eff) + " par=" + std::to_string(run.parS) + " deaths=" + std::to_string(run.deaths) + " upgrade=" +
                std::to_string(up) + " key=" + std::to_string(k.key) + " best=" + std::to_string(k.best) + " " + rewards;
            Rec(p, ev);
            if (low == run.owner)
                out = ev;
        }
        if (out.empty())
            out = "rift finish level=" + std::to_string(run.level) + " upgrade=" + std::to_string(up) + " (owner not in the instance)";
    }

    void StartIfPending(Player* p)
    {
        Map* map = p->GetMap();
        if (!IsFiveMan(map) || !Arpg::Scaling::IsRealPlayer(p) || !sArpgConfig.Get().m9.riftEnabled)
            return;
        uint32 const low = p->GetGUID().GetCounter();
        uint32 const inst = map->GetInstanceId();
        Run run;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto pend = g_pending.find(low);
            if (pend == g_pending.end())
                return;
            if (g_runs.count(inst) && !g_runs[inst].done)
                return;   // already a rift here (re-entry after a death keeps the clock running)
            run.owner = low;
            run.level = pend->second;
            run.map = map->GetId();
            run.startMs = getMSTime();
            run.parS = ParFor(map->GetId());
            Arpg::M9Settings const m = sArpgConfig.Get().m9;
            if (run.level >= m.riftAffixLevel)
                run.affix = urand(0, 1) ? Arpg::Core::RiftAffix::Tyrannical : Arpg::Core::RiftAffix::Fortified;
            g_runs[inst] = run;
            g_pending.erase(pend);
        }
        Arpg::M9Settings const m = sArpgConfig.Get().m9;
        Arpg::Core::RiftMults mm = Arpg::Core::RiftMultipliers(run.level, m.riftHealthGrowth, m.riftDamageGrowth);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "+%d%% health, +%d%% damage", int((mm.health - 1.0f) * 100.0f + 0.5f), int((mm.damage - 1.0f) * 100.0f + 0.5f));
        Say(p, "Rift level " + std::to_string(run.level) + " opened: par time " + Clock(run.parS) + ". Monsters " + buf +
            (run.affix != Arpg::Core::RiftAffix::None ? std::string(", ") + Arpg::Core::RiftAffixName(run.affix) : std::string()) +
            ". Kill the last boss in time to raise your key.");
        Rec(p, "rift start level=" + std::to_string(run.level) + " map=" + std::to_string(run.map) + " par=" + std::to_string(run.parS) +
            " affix=" + Arpg::Core::RiftAffixName(run.affix));
    }

    class ArpgRiftPlayerScript : public PlayerScript
    {
    public:
        ArpgRiftPlayerScript() : PlayerScript("arpg_rifts") { }

        void OnMapChanged(Player* player) override
        {
            StartIfPending(player);
        }

        void OnLogout(Player* player) override
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_nextTick.erase(player->GetGUID().GetCounter());
        }

        void OnPlayerKilledByCreature(Creature* /*killer*/, Player* killed) override
        {
            Run run;
            uint32 inst = 0;
            if (!killed || !RunOf(killed, run, inst))
                return;
            {
                std::lock_guard<std::mutex> l(g_lock);
                ++g_runs[inst].deaths;
            }
            Say(killed, "Rift: a death costs " + std::to_string(sArpgConfig.Get().m9.riftDeathPenaltyS) + " seconds.");
            Rec(killed, "rift death deaths=" + std::to_string(run.deaths + 1));
        }

        void OnUpdate(Player* player, uint32 /*diff*/) override
        {
            Map* map = player->GetMap();
            if (!IsFiveMan(map))
                return;
            uint32 const low = player->GetGUID().GetCounter();
            uint32 const now = getMSTime();
            Run run;
            bool pending = false;
            {
                std::lock_guard<std::mutex> l(g_lock);
                pending = g_pending.count(low) > 0;
                if (g_runs.empty() && !pending)
                    return;
                uint32& next = g_nextTick[low];
                if (now < next)
                    return;
                next = now + 2000;
            }
            if (pending)
                StartIfPending(player);   // fallback when the map-change hook didn't start it
            {
                std::lock_guard<std::mutex> l(g_lock);
                auto it = g_runs.find(map->GetInstanceId());
                if (it == g_runs.end() || it->second.done || it->second.owner != low)
                    return;
                uint32 elapsed = getMSTimeDiff(it->second.startMs, now) / 1000;
                uint32 eff = Arpg::Core::RiftEffectiveSeconds(elapsed, it->second.deaths, sArpgConfig.Get().m9.riftDeathPenaltyS);
                int ms = Arpg::Core::RiftMilestone(eff, it->second.parS, it->second.lastMilestone);
                if (!ms)
                    return;
                it->second.lastMilestone = ms;
                run = it->second;
                run.lastMilestone = int(eff);   // reuse: effective seconds for the message
            }
            Say(player, "Rift: " + Clock(uint32(run.lastMilestone)) + " of " + Clock(run.parS) + " used.");
        }

        void OnCreatureKill(Player* killer, Creature* killed) override
        {
            if (!killer || !killed)
                return;
            bool final;
            {
                std::lock_guard<std::mutex> l(g_lock);
                final = g_finalBosses.count(killed->GetEntry()) > 0;
            }
            if (!final)
                return;
            Map* map = killed->GetMap();
            if (!IsFiveMan(map))
                return;
            uint32 inst = map->GetInstanceId();
            uint32 elapsed;
            {
                std::lock_guard<std::mutex> l(g_lock);
                auto it = g_runs.find(inst);
                if (it == g_runs.end() || it->second.done)
                    return;
                elapsed = getMSTimeDiff(it->second.startMs, getMSTime()) / 1000;
            }
            std::string out;
            Finish(map, inst, elapsed, killed->GetEntry(), out);
        }
    };
}

namespace Arpg::Dungeons
{
    std::string LoadRiftData()
    {
        std::unordered_map<uint32, uint32> par;
        std::unordered_set<uint32> bosses;
        if (QueryResult r = WorldDatabase.Query("SELECT map, par_s FROM arpg_rift_dungeon"))
            do
            {
                Field* f = r->Fetch();
                par[f[0].GetUInt32()] = f[1].GetUInt32();
            } while (r->NextRow());
        if (QueryResult r = WorldDatabase.Query("SELECT creditEntry FROM instance_encounters WHERE lastEncounterDungeon <> 0 AND creditType = 0"))
            do
            {
                bosses.insert(r->Fetch()[0].GetUInt32());
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_par = par;
        g_finalBosses = bosses;
        return "rift dungeons=" + std::to_string(par.size()) + " final_bosses=" + std::to_string(bosses.size());
    }

    void InstallRifts()
    {
        new ArpgRiftPlayerScript();
    }

    int RiftScale(Map* map, bool boss, float& health, float& damage)
    {
        if (!IsFiveMan(map))
            return 0;
        Run run;
        bool found = false, anyPending = false;
        {
            std::lock_guard<std::mutex> l(g_lock);
            anyPending = !g_pending.empty();
            auto it = g_runs.find(map->GetInstanceId());
            if (it != g_runs.end())
            {
                run = it->second;   // active run (finished/closed runs are erased)
                found = true;
            }
        }
        if (anyPending)
        {
            // The instance scales its creatures as the character arrives, before OnMapChanged: start the rift now.
            Map::PlayerList const& players = map->GetPlayers();
            for (auto itr = players.begin(); itr != players.end(); ++itr)
                if (Player* p = itr->GetSource())
                    if (p->GetMap() == map)
                        StartIfPending(p);
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_runs.find(map->GetInstanceId());
            if (it != g_runs.end())
            {
                run = it->second;
                found = true;
            }
        }
        if (!found)
            return 0;
        Arpg::M9Settings const m = sArpgConfig.Get().m9;
        Core::RiftMults mm = Core::RiftMultipliers(run.level, m.riftHealthGrowth, m.riftDamageGrowth);
        float h = mm.health, d = mm.damage;
        Core::ApplyRiftAffix(run.affix, boss, h, d);
        health *= h;
        damage *= d;
        return run.level;
    }

    bool OpenRift(Player* player, int level, std::string& out)
    {
        if (!sArpgConfig.Get().m9.riftEnabled)
        {
            out = "rifts are off";
            return false;
        }
        if (IsFiveMan(player->GetMap()))
        {
            out = "leave the dungeon first";
            return false;
        }
        uint32 const low = player->GetGUID().GetCounter();
        int key;
        {
            std::lock_guard<std::mutex> l(g_lock);
            key = KeyOf(low).key;
        }
        if (level < 1 || level > key)
        {
            out = "level " + std::to_string(level) + " is above your key (" + std::to_string(key) + ")";
            return false;
        }
        player->ResetInstances(INSTANCE_RESET_ALL, false);
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_pending[low] = level;
        }
        Say(player, "A level " + std::to_string(level) + " rift is open: the next dungeon you enter is the rift.");
        Rec(player, "rift open level=" + std::to_string(level));
        out = "rift open level=" + std::to_string(level) + " key=" + std::to_string(key);
        return true;
    }

    std::string CloseRift(Player* player)
    {
        uint32 const low = player->GetGUID().GetCounter();
        int closed = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            closed += int(g_pending.erase(low));
            for (auto it = g_runs.begin(); it != g_runs.end();)
                if (it->second.owner == low)
                {
                    it = g_runs.erase(it);   // monsters back to normal
                    ++closed;
                }
                else
                    ++it;
        }
        return "rift closed=" + std::to_string(closed);
    }

    bool FinishRift(Player* player, int elapsedS, std::string& out)
    {
        Run run;
        uint32 inst = 0;
        if (!RunOf(player, run, inst))
        {
            out = "no active rift here";
            return false;
        }
        Finish(player->GetMap(), inst, uint32(std::max(0, elapsedS)), 0, out);
        return true;
    }

    bool AddRiftDeath(Player* player, std::string& out)
    {
        Run run;
        uint32 inst = 0;
        if (!RunOf(player, run, inst))
        {
            out = "no active rift here";
            return false;
        }
        std::lock_guard<std::mutex> l(g_lock);
        out = "rift deaths=" + std::to_string(++g_runs[inst].deaths);
        return true;
    }

    int KeyLevel(Player* player)
    {
        std::lock_guard<std::mutex> l(g_lock);
        return KeyOf(player->GetGUID().GetCounter()).key;
    }

    void SetKeyLevel(Player* player, int key)
    {
        uint32 const low = player->GetGUID().GetCounter();
        Key k;
        {
            std::lock_guard<std::mutex> l(g_lock);
            Key& kk = KeyOf(low);
            kk.key = std::max(1, key);
            k = kk;
        }
        SaveKey(low, k);
    }

    std::string DescribeRift(Player* player)
    {
        uint32 const low = player->GetGUID().GetCounter();
        Key k;
        int pending = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            k = KeyOf(low);
            auto p = g_pending.find(low);
            pending = p == g_pending.end() ? 0 : p->second;
        }
        std::string out = "rift key=" + std::to_string(k.key) + " best=" + std::to_string(k.best) + " runs=" + std::to_string(k.runs) +
            " pending=" + std::to_string(pending);
        Run run;
        uint32 inst = 0;
        if (!RunOf(player, run, inst))
            return out + " active=0";
        Arpg::M9Settings const m = sArpgConfig.Get().m9;
        Core::RiftMults mm = Core::RiftMultipliers(run.level, m.riftHealthGrowth, m.riftDamageGrowth);
        char buf[200];
        std::snprintf(buf, sizeof(buf), " active=1 level=%d map=%u elapsed=%u par=%u deaths=%d affix=%s health=%.3f damage=%.3f",
            run.level, run.map, getMSTimeDiff(run.startMs, getMSTime()) / 1000, run.parS, run.deaths, Core::RiftAffixName(run.affix),
            mm.health, mm.damage);
        return out + buf;
    }

    std::string DataLine(Player* player)
    {
        uint32 const low = player->GetGUID().GetCounter();
        Key k;
        int pending = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            k = KeyOf(low);
            auto p = g_pending.find(low);
            pending = p == g_pending.end() ? 0 : p->second;
        }
        Run run;
        uint32 inst = 0;
        bool active = RunOf(player, run, inst);
        uint32 elapsed = active ? getMSTimeDiff(run.startMs, getMSTime()) / 1000 : 0;
        return "ARPGR|" + std::to_string(k.key) + "|" + std::to_string(k.best) + "|" + std::to_string(pending) + "|" +
            (active ? "1" : "0") + "|" + std::to_string(active ? run.level : 0) + "|" + std::to_string(elapsed) + "|" +
            std::to_string(active ? run.parS : 0) + "|" + std::to_string(active ? run.deaths : 0) + "|" +
            (active ? Core::RiftAffixName(run.affix) : "none");
    }

    std::string OfferedLevels(Player* player)
    {
        int key = KeyLevel(player);
        std::set<int, std::greater<int>> lv{ key, key - 1, key - 2, key - 5, 1 };
        std::string out;
        for (int v : lv)
            if (v >= 1)
                out += (out.empty() ? "" : ",") + std::to_string(v);
        return out;
    }
}
