#include "World/Shrines.h"

#include "ArpgConfig.h"
#include "Core/AffixEffects.h"
#include "Core/Charms.h"
#include "Loot/AffixEffects.h"
#include "Loot/AffixRoller.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "CellImpl.h"
#include "Chat.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "Map.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "Timer.h"

#include <cmath>
#include <ctime>
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace
{
    struct ShrineDef
    {
        uint32 id = 0;
        std::string name;
        uint32 goEntry = 0;
        uint32 spell = 0;
        int32 amount = 0;
        std::vector<Arpg::Loot::EffectLine> effects;
        uint32 durationS = 120;
        int weight = 0;
    };
    struct ActiveEffects
    {
        uint32 spell = 0;
        uint32 untilMs = 0;
        std::vector<Arpg::Loot::EffectLine> lines;
    };

    std::mutex g_lock;
    std::vector<ShrineDef> g_defs;
    std::unordered_map<uint32, uint32> g_lastSpawnS;                          // player low -> time(nullptr) of last spawn
    std::unordered_map<uint32, std::map<uint32, ActiveEffects>> g_active;     // player low -> shrine id -> effect lines
    std::unordered_map<uint32, uint32> g_nextTick;                             // player low -> ms

    void Rec(Player* p, std::string const& line)
    {
        if (p && Arpg::TestEvents::Enabled())
            Arpg::TestEvents::Record(p->GetGUID().GetCounter(), line);
    }

    bool DefById(uint32 id, ShrineDef& out)
    {
        std::lock_guard<std::mutex> l(g_lock);
        for (ShrineDef const& d : g_defs)
            if (d.id == id)
            {
                out = d;
                return true;
            }
        return false;
    }

    bool DefByEntry(uint32 entry, ShrineDef& out)
    {
        std::lock_guard<std::mutex> l(g_lock);
        for (ShrineDef const& d : g_defs)
            if (d.goEntry == entry)
            {
                out = d;
                return true;
            }
        return false;
    }

    // Buff for one character: the spell with our amount/duration; effect lines tracked until it ends.
    void GiveBuff(Player* p, ShrineDef const& d)
    {
        p->RemoveAurasDueToSpell(d.spell);
        int32 const ms = int32(d.durationS * 1000);
        if (Aura* a = p->AddAura(d.spell, p))
        {
            a->SetMaxDuration(ms);
            a->SetDuration(ms);
            if (d.amount)
                if (AuraEffect* e = a->GetEffect(0))
                    e->ChangeAmount(float(d.amount));
        }
        if (d.effects.empty())
            return;
        uint32 const pg = p->GetGUID().GetCounter();
        bool fresh = false;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto& mine = g_active[pg];
            auto it = mine.find(d.id);
            if (it == mine.end())
            {
                mine[d.id] = { d.spell, getMSTime() + uint32(ms), d.effects };
                fresh = true;
            }
            else
                it->second.untilMs = getMSTime() + uint32(ms);   // refresh, never twice
        }
        if (fresh)
            Arpg::Loot::ApplyEffectLines(p, d.effects, true);
    }

    void ExpireEffects(Player* p, bool all)
    {
        uint32 const pg = p->GetGUID().GetCounter();
        uint32 const now = getMSTime();
        std::vector<std::pair<uint32, std::vector<Arpg::Loot::EffectLine>>> gone;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto pit = g_active.find(pg);
            if (pit == g_active.end())
                return;
            for (auto it = pit->second.begin(); it != pit->second.end();)
            {
                if (all || now >= it->second.untilMs || !p->HasAura(it->second.spell))
                {
                    gone.push_back({ it->first, it->second.lines });
                    it = pit->second.erase(it);
                }
                else
                    ++it;
            }
        }
        for (auto const& g : gone)
        {
            Arpg::Loot::ApplyEffectLines(p, g.second, false);
            Rec(p, "shrine effects end id=" + std::to_string(g.first));
        }
    }

    std::string Article(std::string const& name)
    {
        char c = name.empty() ? 'x' : char(std::tolower(name[0]));
        return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') ? "An " : "A ";
    }

    class ArpgShrineGameObjectScript : public GameObjectScript
    {
    public:
        ArpgShrineGameObjectScript() : GameObjectScript("arpg_shrine") { }

        bool OnGossipHello(Player* player, GameObject* go) override
        {
            std::string out;
            Arpg::World::UseShrine(player, go, out);
            return true;
        }
    };

    class ArpgShrinePlayerScript : public PlayerScript
    {
    public:
        ArpgShrinePlayerScript() : PlayerScript("arpg_shrines") { }

        void OnLogin(Player* player) override
        {
            // Buffs saved with the character lost their effect lines and our amount; start clean.
            std::vector<uint32> spells;
            {
                std::lock_guard<std::mutex> l(g_lock);
                for (ShrineDef const& d : g_defs)
                    spells.push_back(d.spell);
            }
            for (uint32 s : spells)
                player->RemoveAurasDueToSpell(s);
        }

        void OnLogout(Player* player) override
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_active.erase(player->GetGUID().GetCounter());   // effect totals are forgotten by AffixStats at logout
            g_nextTick.erase(player->GetGUID().GetCounter());
        }

        void OnUpdate(Player* player, uint32 /*diff*/) override
        {
            uint32 const pg = player->GetGUID().GetCounter();
            uint32 const now = getMSTime();
            {
                std::lock_guard<std::mutex> l(g_lock);
                if (!g_active.count(pg))
                    return;
                uint32& next = g_nextTick[pg];
                if (now < next)
                    return;
                next = now + 1000;
            }
            ExpireEffects(player, false);
        }

        void OnCreatureKill(Player* killer, Creature* killed) override
        {
            if (!killer || !killed || !Arpg::Scaling::IsRealPlayer(killer))
                return;
            Arpg::M9Settings const m = sArpgConfig.Get().m9;
            if (!m.shrineEnabled)
                return;
            Map* map = killed->GetMap();
            if (!map || map->IsBattlegroundOrArena() || (map->Instanceable() && !m.shrineInDungeons) || map->IsRaid())
                return;
            if (killed->GetEntry() == 1000010)
                return;   // Treasure Goblins drop loot, not shrines
            uint32 const pg = killer->GetGUID().GetCounter();
            uint32 const nowS = uint32(time(nullptr));
            bool hasLast;
            uint32 last;
            {
                std::lock_guard<std::mutex> l(g_lock);
                auto it = g_lastSpawnS.find(pg);
                hasLast = it != g_lastSpawnS.end();
                last = hasLast ? it->second : 0;
            }
            if (!Arpg::Core::SpawnGate(m.shrineKillChancePct, urand(0, 9999), nowS, hasLast, last, m.shrineCooldownS))
                return;
            std::string why;
            Arpg::World::SpawnShrine(killer, 0, killed->GetPositionX(), killed->GetPositionY(), killed->GetPositionZ(), why);
        }
    };
}

namespace Arpg::World
{
    std::string LoadShrineData()
    {
        std::vector<ShrineDef> defs;
        if (QueryResult r = WorldDatabase.Query("SELECT id, name, go_entry, spell, amount, effect1, value1, effect2, value2, "
            "duration_s, weight FROM arpg_shrine ORDER BY id"))
            do
            {
                Field* f = r->Fetch();
                ShrineDef d;
                d.id = f[0].GetUInt32();
                d.name = f[1].GetString();
                d.goEntry = f[2].GetUInt32();
                d.spell = f[3].GetUInt32();
                d.amount = f[4].GetInt32();
                for (int i = 0; i < 2; ++i)
                {
                    Core::Effect e = Core::EffectFromName(f[5 + i * 2].GetString());
                    if (e != Core::Effect::None)
                        d.effects.push_back({ int(e), f[6 + i * 2].GetInt32() });
                }
                d.durationS = std::max<uint32>(1, f[9].GetUInt32());
                d.weight = int(f[10].GetUInt32());
                defs.push_back(d);
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_defs = defs;
        return "shrines=" + std::to_string(defs.size());
    }

    void InstallShrines()
    {
        new ArpgShrineGameObjectScript();
        new ArpgShrinePlayerScript();
    }

    GameObject* SpawnShrine(Player* player, uint32 id, float x, float y, float z, std::string& why)
    {
        ShrineDef d;
        if (!id)
        {
            std::vector<ShrineDef> defs;
            {
                std::lock_guard<std::mutex> l(g_lock);
                defs = g_defs;
            }
            std::vector<int> w;
            for (ShrineDef const& s : defs)
                w.push_back(s.weight);
            int i = Core::PickWeighted(w, urand(0, 0x7FFFFFFF));
            if (i < 0)
            {
                why = "no shrines in arpg_shrine";
                return nullptr;
            }
            d = defs[size_t(i)];
        }
        else if (!DefById(id, d))
        {
            why = "unknown shrine id " + std::to_string(id);
            return nullptr;
        }
        float const ang = player->GetOrientation();
        G3D::Quat rot(0.0f, 0.0f, std::sin(ang / 2.0f), std::cos(ang / 2.0f));
        uint32 const life = sArpgConfig.Get().m9.shrineLifetimeS;
        GameObject* go = player->SummonGameObject(d.goEntry, x, y, z, ang, rot, life);
        if (!go)
        {
            why = "could not spawn";
            return nullptr;
        }
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_lastSpawnS[player->GetGUID().GetCounter()] = uint32(time(nullptr));
        }
        ChatHandler(player->GetSession()).PSendSysMessage("|cffffd200%s%s appears nearby!|r", Article(d.name).c_str(), d.name.c_str());
        Rec(player, "shrine spawn id=" + std::to_string(d.id) + " entry=" + std::to_string(d.goEntry));
        why = "shrine id=" + std::to_string(d.id) + " entry=" + std::to_string(d.goEntry) + " name=" + d.name;
        return go;
    }

    GameObject* SpawnShrineNear(Player* player, uint32 id, std::string& why)
    {
        Position pos = player->GetNearPosition(3.0f, 0.0f);
        return SpawnShrine(player, id, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), why);
    }

    GameObject* NearestShrine(Player* player, float range)
    {
        std::vector<uint32> entries;
        {
            std::lock_guard<std::mutex> l(g_lock);
            for (ShrineDef const& d : g_defs)
                entries.push_back(d.goEntry);
        }
        GameObject* best = nullptr;
        for (uint32 e : entries)
            if (GameObject* go = player->FindNearestGameObject(e, range))
                if (go->isSpawned() && (!best || player->GetDistance(go) < player->GetDistance(best)))
                    best = go;
        return best;
    }

    bool UseShrine(Player* player, GameObject* go, std::string& out)
    {
        ShrineDef d;
        if (!player || !go || !DefByEntry(go->GetEntry(), d))
        {
            out = "not a shrine";
            return false;
        }
        if (!go->isSpawned())
        {
            out = "shrine already used";
            return false;
        }
        std::vector<Player*> targets{ player };
        float const range = sArpgConfig.Get().m9.shrineGroupRange;
        if (Group* g = player->GetGroup())
            for (GroupReference* ref = g->GetFirstMember(); ref; ref = ref->next())
                if (Player* m = ref->GetSource())
                    if (m != player && m->IsInMap(player) && m->IsAlive() && m->IsWithinDistInMap(player, range))
                        targets.push_back(m);
        for (Player* t : targets)
            GiveBuff(t, d);
        go->SetRespawnTime(0);
        go->Delete();
        ChatHandler(player->GetSession()).PSendSysMessage("|cffffd200%s: %u seconds.|r", d.name.c_str(), d.durationS);
        Rec(player, "shrine used id=" + std::to_string(d.id) + " targets=" + std::to_string(targets.size()));
        out = "used id=" + std::to_string(d.id) + " name=" + d.name + " targets=" + std::to_string(targets.size());
        return true;
    }

    std::string Describe(Player* player)
    {
        std::vector<ShrineDef> defs;
        {
            std::lock_guard<std::mutex> l(g_lock);
            defs = g_defs;
        }
        std::string out;
        size_t nearby = 0;
        for (ShrineDef const& d : defs)
        {
            std::list<GameObject*> list;
            player->GetGameObjectListWithEntryInGrid(list, d.goEntry, 40.0f);
            for (GameObject* go : list)
                if (go->isSpawned())
                    ++nearby;
        }
        out = "shrines nearby=" + std::to_string(nearby);
        if (GameObject* n = NearestShrine(player, 40.0f))
            out += " nearest=" + std::to_string(n->GetEntry()) + " dist=" + std::to_string(int(player->GetDistance(n)));
        for (ShrineDef const& d : defs)
            if (Aura* a = player->GetAura(d.spell))
            {
                AuraEffect* e = a->GetEffect(0);
                out += " | buff id=" + std::to_string(d.id) + " amount=" + std::to_string(e ? int32(e->GetAmount()) : 0) +
                    " left=" + std::to_string(a->GetDuration() / 1000) + "s";
            }
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_active.find(player->GetGUID().GetCounter());
            if (it != g_active.end())
                for (auto const& kv : it->second)
                    out += " | effects id=" + std::to_string(kv.first) + " lines=" + std::to_string(kv.second.lines.size());
        }
        return out;
    }

    int ClearBuffs(Player* player)
    {
        std::vector<uint32> spells;
        {
            std::lock_guard<std::mutex> l(g_lock);
            for (ShrineDef const& d : g_defs)
                spells.push_back(d.spell);
        }
        int n = 0;
        for (uint32 s : spells)
            if (player->HasAura(s))
            {
                player->RemoveAurasDueToSpell(s);
                ++n;
            }
        return n;
    }
}
