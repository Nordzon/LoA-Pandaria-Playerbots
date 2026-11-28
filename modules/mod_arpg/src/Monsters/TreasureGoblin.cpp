#include "Monsters/TreasureGoblin.h"

#include "ArpgConfig.h"
#include "Charms/Charms.h"
#include "Core/Charms.h"
#include "Loot/PersonalLoot.h"
#include "Monsters/Champions.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "Chat.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "LootMgr.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"

#include <ctime>
#include <mutex>
#include <unordered_map>

namespace
{
    uint32 const LootAura = 139792;    // "Loot! This opponent is hauling treasure!" (client buff, dummy)
    uint32 const Essence = 700010;

    struct GoblinInfo
    {
        uint32 owner = 0;           // player low guid it was summoned for
        bool escaped = false;
        bool killed = false;
        bool engaged = false;
        bool fleeing = false;
        uint32 escapeLeftMs = 0;
    };

    std::mutex g_lock;
    std::unordered_map<uint64, GoblinInfo> g_goblins;          // creature raw guid
    std::unordered_map<uint32, uint64> g_lastOf;               // player low -> goblin raw guid
    std::unordered_map<uint32, uint32> g_lastSpawnS;           // player low -> time(nullptr)

    void Rec(uint32 playerLow, std::string const& line)
    {
        if (playerLow && Arpg::TestEvents::Enabled())
            Arpg::TestEvents::Record(playerLow, line);
    }

    void Tell(Creature* me, uint32 ownerLow, char const* text)
    {
        if (Player* p = ObjectAccessor::GetPlayer(*me, ObjectGuid(HighGuid::Player, ownerLow)))
            ChatHandler(p->GetSession()).PSendSysMessage("|cffffd200%s|r", text);
    }

    struct TreasureGoblinAI : public CreatureAI
    {
        explicit TreasureGoblinAI(Creature* c) : CreatureAI(c) { }

        uint32 lifeMs = 0;
        uint32 refleeMs = 0;

        GoblinInfo* Info()
        {
            auto it = g_goblins.find(me->GetGUID().GetRawValue());
            return it == g_goblins.end() ? nullptr : &it->second;
        }

        void InitializeAI() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->AddAura(LootAura, me);
        }

        void AttackStart(Unit* /*who*/) override { }     // never fights
        void EnterEvadeMode(EvadeReason /*why*/) override { }

        void Flee(Unit* from)
        {
            if (!from || !me->IsAlive())
                return;
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveFleeing(from, 0);
            std::lock_guard<std::mutex> l(g_lock);
            if (GoblinInfo* gi = Info())
                gi->fleeing = true;
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (!who || who->GetTypeId() != TYPEID_PLAYER || !me->IsAlive() || !me->IsWithinDistInMap(who, 10.0f))
                return;
            bool fleeing;
            {
                std::lock_guard<std::mutex> l(g_lock);
                GoblinInfo* gi = Info();
                fleeing = gi && gi->fleeing;
            }
            if (!fleeing)
                Flee(who);
        }

        void DamageTaken(Unit* attacker, uint32& /*damage*/) override
        {
            bool start = false;
            uint32 owner = 0;
            {
                std::lock_guard<std::mutex> l(g_lock);
                if (GoblinInfo* gi = Info())
                {
                    if (!gi->engaged)
                    {
                        gi->engaged = true;
                        gi->escapeLeftMs = sArpgConfig.Get().m9.goblinEscapeS * 1000;
                        start = true;
                        owner = gi->owner;
                    }
                }
            }
            if (start)
            {
                Tell(me, owner, "The Treasure Goblin runs for it! Kill it before it escapes.");
                Rec(owner, "goblin engaged");
            }
            if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != FLEEING_MOTION_TYPE)
                Flee(attacker);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!me->IsAlive())
                return;
            bool escape = false, expire = false;
            uint32 owner = 0;
            {
                std::lock_guard<std::mutex> l(g_lock);
                GoblinInfo* gi = Info();
                if (!gi)
                    return;
                owner = gi->owner;
                if (gi->engaged)
                {
                    if (gi->escapeLeftMs <= diff)
                    {
                        gi->escapeLeftMs = 0;
                        gi->escaped = true;
                        escape = true;
                    }
                    else
                        gi->escapeLeftMs -= diff;
                }
                else
                {
                    lifeMs += diff;
                    expire = lifeMs >= sArpgConfig.Get().m9.goblinLifetimeS * 1000;
                    if (expire)
                        gi->escaped = true;
                }
            }
            if (escape || expire)
            {
                if (escape)
                    Tell(me, owner, "The Treasure Goblin escaped through a portal!");
                Rec(owner, std::string("goblin escaped") + (expire ? " untouched" : ""));
                me->DespawnOrUnsummon();
                return;
            }
            // Stunned, rooted or feared and back: run again from the nearest player.
            if (refleeMs <= diff)
            {
                refleeMs = 2000;
                bool fleeing;
                {
                    std::lock_guard<std::mutex> l(g_lock);
                    GoblinInfo* gi = Info();
                    fleeing = gi && gi->fleeing;
                }
                if (fleeing && me->GetMotionMaster()->GetCurrentMovementGeneratorType() != FLEEING_MOTION_TYPE &&
                    !me->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_ROOT | UNIT_STATE_CONFUSED))
                    if (Player* p = me->SelectNearestPlayer(40.0f))
                        Flee(p);
            }
            else
                refleeMs -= diff;
        }
    };

    class ArpgTreasureGoblinCreatureScript : public CreatureScript
    {
    public:
        ArpgTreasureGoblinCreatureScript() : CreatureScript("arpg_treasure_goblin") { }
        CreatureAI* GetAI(Creature* c) const override { return new TreasureGoblinAI(c); }
    };

    void AddLoot(::Loot& loot, uint32 itemId, uint32 count)
    {
        if (!itemId)
            return;
        LootItem li;
        li.itemid = itemId;
        li.count = std::max<uint32>(1, count);
        li.is_world_drop = false;
        li.personal = false;
        loot.items.push_back(li);
        ++loot.unlootedCount;
    }

    uint32 CatalystFor(Player* p)
    {
        uint8 lvl = p->GetLevel();
        return lvl < 35 ? 700001 : lvl < 70 ? 700002 : 700003;
    }

    class ArpgTreasureGoblinPlayerScript : public PlayerScript
    {
    public:
        ArpgTreasureGoblinPlayerScript() : PlayerScript("arpg_treasure_goblins") { }

        void OnCreatureKill(Player* killer, Creature* killed) override
        {
            if (!killer || !killed)
                return;
            Arpg::M9Settings const m = sArpgConfig.Get().m9;
            if (killed->GetEntry() == Arpg::Monsters::TreasureGoblinEntry)
            {
                Loot(killer, killed, m);
                return;
            }
            if (!m.goblinEnabled || !Arpg::Scaling::IsRealPlayer(killer))
                return;
            Map* map = killed->GetMap();
            if (!map || map->Instanceable())
                return;   // open world only
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
            if (!Arpg::Core::SpawnGate(m.goblinKillChancePct, urand(0, 9999), nowS, hasLast, last, m.goblinCooldownS))
                return;
            std::string why;
            Arpg::Monsters::SpawnGoblin(killer, why);
        }

        void Loot(Player* killer, Creature* killed, Arpg::M9Settings const& m)
        {
            uint32 owner = 0;
            {
                std::lock_guard<std::mutex> l(g_lock);
                auto it = g_goblins.find(killed->GetGUID().GetRawValue());
                if (it != g_goblins.end())
                {
                    it->second.killed = true;
                    owner = it->second.owner;
                }
            }
            Arpg::Scaling::Forget(killed->GetGUID().GetRawValue());
            uint32 const level = std::max<uint32>(1, killed->GetLevel());
            uint32 gold = uint32(uint64(m.goblinGoldPerLevel) * level * urand(80, 120) / 100);
            killed->loot.gold += gold;
            auto items = [&](Player* who) -> std::vector<uint32>
            {
                std::vector<uint32> out;
                for (int i = 0; i < m.goblinItems; ++i)
                {
                    uint32 q = float(rand_chance()) < m.goblinEpicPct ? ITEM_QUALITY_EPIC :
                        float(rand_chance()) < m.goblinBluePct ? ITEM_QUALITY_RARE : ITEM_QUALITY_UNCOMMON;
                    if (uint32 id = Arpg::Monsters::FloorItem(who, q))
                        out.push_back(id);
                }
                if (float(rand_chance()) < m.goblinCatalystPct)
                    out.push_back(CatalystFor(who));
                if (float(rand_chance()) < m.goblinCharmPct)
                    out.push_back(Arpg::Charms::RandomCharmEntry());
                if (float(rand_chance()) < m.goblinUniqueCharmPct && !Arpg::Charms::HasCharm(who, Arpg::Charms::GoblinsLuckyCoin))
                    out.push_back(Arpg::Charms::GoblinsLuckyCoin);
                return out;
            };
            std::vector<uint32> mine = items(killer);
            for (uint32 id : mine)
                AddLoot(killed->loot, id, 1);
            AddLoot(killed->loot, Essence, uint32(m.goblinEssences));
            for (Player* mate : Arpg::Loot::PersonalRecipients(killer, killed))
                for (uint32 id : items(mate))
                    Arpg::Loot::AddPersonalItem(mate, killed, id);
            killed->SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            ChatHandler(killer->GetSession()).PSendSysMessage("|cffffd200The Treasure Goblin bursts open!|r");
            std::string ids;
            for (uint32 id : mine)
                ids += (ids.empty() ? "" : ",") + std::to_string(id);
            Rec(killer->GetGUID().GetCounter(), "goblin killed gold=" + std::to_string(gold) + " items=" + std::to_string(mine.size()) +
                " essences=" + std::to_string(m.goblinEssences) + " ids=" + ids + " owner=" + std::to_string(owner));
        }
    };
}

namespace Arpg::Monsters
{
    void InstallTreasureGoblins()
    {
        new ArpgTreasureGoblinCreatureScript();
        new ArpgTreasureGoblinPlayerScript();
    }

    Creature* SpawnGoblin(Player* player, std::string& why)
    {
        Arpg::M9Settings const m = sArpgConfig.Get().m9;
        float const angle = float(rand_norm()) * 2.0f * float(M_PI);
        Position pos = player->GetNearPosition(15.0f, angle);
        Creature* c = player->SummonCreature(TreasureGoblinEntry, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
            pos.GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, Milliseconds(120000));
        if (!c)
        {
            why = "could not summon";
            return nullptr;
        }
        uint32 const pg = player->GetGUID().GetCounter();
        {
            std::lock_guard<std::mutex> l(g_lock);
            GoblinInfo gi;
            gi.owner = pg;
            g_goblins[c->GetGUID().GetRawValue()] = gi;
            g_lastOf[pg] = c->GetGUID().GetRawValue();
            g_lastSpawnS[pg] = uint32(time(nullptr));
            if (g_goblins.size() > 256)   // corpses and escapes are long gone by then
                for (auto it = g_goblins.begin(); it != g_goblins.end();)
                    it = (it->second.killed || it->second.escaped) ? g_goblins.erase(it) : std::next(it);
        }
        Arpg::Scaling::Pin(c, uint8(player->GetLevel()), m.goblinHealthMult, 1.0f);
        ChatHandler(player->GetSession()).PSendSysMessage("|cffffd200A Treasure Goblin appears!|r");
        Rec(pg, "goblin spawn level=" + std::to_string(c->GetLevel()) + " hp=" + std::to_string(c->GetMaxHealth()));
        why = "goblin level=" + std::to_string(c->GetLevel()) + " hp=" + std::to_string(c->GetMaxHealth());
        return c;
    }

    Creature* LastGoblinOf(Player* player)
    {
        uint64 raw;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_lastOf.find(player->GetGUID().GetCounter());
            if (it == g_lastOf.end())
                return nullptr;
            raw = it->second;
        }
        return ObjectAccessor::GetCreature(*player, ObjectGuid(raw));
    }

    std::string DescribeGoblin(Player* player)
    {
        uint64 raw = 0;
        GoblinInfo gi;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_lastOf.find(player->GetGUID().GetCounter());
            if (it == g_lastOf.end())
                return "goblin none";
            raw = it->second;
            auto g = g_goblins.find(raw);
            if (g != g_goblins.end())
                gi = g->second;
        }
        Creature* c = ObjectAccessor::GetCreature(*player, ObjectGuid(raw));
        if (c && c->IsAlive())
            return "goblin alive=1 fleeing=" + std::to_string(gi.fleeing ? 1 : 0) + " engaged=" + std::to_string(gi.engaged ? 1 : 0) +
                " escape_in=" + std::to_string(gi.escapeLeftMs / 1000) + "s level=" + std::to_string(c->GetLevel()) +
                " hp=" + std::to_string(c->GetHealth()) + "/" + std::to_string(c->GetMaxHealth()) +
                " dist=" + std::to_string(int(player->GetDistance(c)));
        return "goblin alive=0 escaped=" + std::to_string(gi.escaped ? 1 : 0) + " killed=" + std::to_string(gi.killed ? 1 : 0);
    }
}
