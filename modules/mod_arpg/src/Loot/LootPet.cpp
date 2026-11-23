#include "Loot/LootPet.h"

#include "ArpgConfig.h"
#include "ArpgHooks.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "Creature.h"
#include "DatabaseEnv.h"
#include "LootMgr.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include "Timer.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"

#include <cstdio>
#include <deque>
#include <mutex>
#include <unordered_map>

namespace
{
    uint32 const PetEntry = 7000001;

    struct PetState
    {
        bool loaded = false;
        bool on = false;
        uint64 pet = 0;                 // creature guid
        std::deque<uint64> queue;       // corpses to visit
        uint64 target = 0;
        uint32 moveStart = 0;
        uint32 lastTick = 0;
        uint32 looted = 0;              // corpses emptied (since login)
    };

    std::mutex g_lock;
    std::unordered_map<uint32, PetState> g_state;   // player guid -> state

    PetState& State(uint32 guid)
    {
        PetState& s = g_state[guid];
        if (!s.loaded)
        {
            s.loaded = true;
            if (QueryResult r = CharacterDatabase.PQuery("SELECT enabled FROM character_arpg_lootpet WHERE guid = %u", guid))
                s.on = r->Fetch()[0].GetUInt8() != 0;
        }
        return s;
    }

    void Despawn(Player* p, uint64 guid)
    {
        if (Creature* c = guid ? ObjectAccessor::GetCreature(*p, ObjectGuid(guid)) : nullptr)
            c->DespawnOrUnsummon();
    }

    Creature* Summon(Player* p)
    {
        Position pos = p->GetNearPosition(2.0f, float(M_PI) * 0.75f);
        TempSummon* c = p->SummonCreature(PetEntry, pos, TEMPSUMMON_MANUAL_DESPAWN);
        if (!c)
            return nullptr;
        c->SetFaction(p->GetFaction());
        c->GetMotionMaster()->MoveFollow(p, 2.0f, float(M_PI) * 0.75f);
        return c;
    }

    // Moves the corpse's gold and items to the player (core store path: affixes, planned names, loot filter, quest items).
    void Transfer(Player* p, Creature* corpse)
    {
        ::Loot& loot = corpse->loot;
        uint32 gold = loot.gold;
        if (gold)
        {
            if (ArpgHooks::LootMoney)
                ArpgHooks::LootMoney(p, gold);   // gold find affixes, as with the core's money looting
            p->ModifyMoney(int64(gold));
            loot.gold = 0;
        }
        uint32 items = 0;
        ObjectGuid const oldLoot = p->GetLootGUID();
        p->SetLootGUID(corpse->GetGUID());
        for (uint8 i = 0; i < uint8(loot.items.size()); ++i)
            if (!loot.items[i].is_looted && !loot.items[i].currency && !loot.items[i].is_blocked)
            {
                p->StoreLootItem(i, &loot, corpse);
                if (loot.items[i].is_looted)
                    ++items;
            }
        p->SetLootGUID(oldLoot);
        if (loot.isLooted())
        {
            corpse->RemoveFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            corpse->AllLootRemovedFromCorpse();
        }
        Arpg::TestEvents::Record(p->GetGUID().GetCounter(), "lootpet looted corpse=" + std::to_string(corpse->GetEntry()) +
            " items=" + std::to_string(items) + " gold=" + std::to_string(gold));
    }

    void Tick(Player* p)
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        uint32 key = p->GetGUID().GetCounter();
        uint32 now = getMSTime();
        uint64 petGuid = 0, target = 0;
        uint32 moveStart = 0;
        bool on = false;
        {
            std::lock_guard<std::mutex> l(g_lock);
            PetState& st = State(key);
            if (st.lastTick && getMSTimeDiff(st.lastTick, now) < 500)
                return;
            st.lastTick = now;
            on = st.on && s.lootPetEnabled;
            petGuid = st.pet;
            target = st.target;
            moveStart = st.moveStart;
        }
        if (!on)
        {
            if (petGuid)
            {
                Despawn(p, petGuid);
                std::lock_guard<std::mutex> l(g_lock);
                PetState& st = State(key);
                st.pet = 0;
                st.queue.clear();
                st.target = 0;
            }
            return;
        }
        if (!p->IsAlive() || p->IsInFlight())
            return;
        // Keep the pet with the player (zoning, teleports, death of the pet).
        Creature* pet = petGuid ? ObjectAccessor::GetCreature(*p, ObjectGuid(petGuid)) : nullptr;
        if (!pet || !pet->IsAlive() || pet->GetMapId() != p->GetMapId() || !pet->IsWithinDistInMap(p, 80.0f))
        {
            if (pet)
                pet->DespawnOrUnsummon();
            pet = Summon(p);
            std::lock_guard<std::mutex> l(g_lock);
            PetState& st = State(key);
            st.pet = pet ? pet->GetGUID().GetRawValue() : 0;
            st.target = 0;
            if (!pet)
                return;
            target = 0;
        }
        // Companion roles (vendor, repair, bank, mail, reforge, transmog, void): native NPC flags, cleared in combat.
        {
            uint32 want = (p->IsInCombat() && !s.companionInCombat) ? 0u : s.companionNpcFlags;
            if (pet->GetUInt32Value(UNIT_FIELD_NPC_FLAGS) != want)
                pet->SetUInt32Value(UNIT_FIELD_NPC_FLAGS, want);
        }
        if (s.lootPetWaitForCombatEnd && p->IsInCombat())
        {
            Arpg::TestEvents::Record(key, "lootpet waits (combat)");
            return;
        }
        // Next corpse.
        if (!target)
        {
            std::lock_guard<std::mutex> l(g_lock);
            PetState& st = State(key);
            while (!st.queue.empty() && !target)
            {
                target = st.queue.front();
                st.queue.pop_front();
                Creature* c = ObjectAccessor::GetCreature(*p, ObjectGuid(target));
                if (!c || c->IsAlive() || c->loot.isLooted())
                {
                    Arpg::TestEvents::Record(key, std::string("lootpet skip ") + (!c ? "gone" : c->IsAlive() ? "alive" : "empty"));
                    target = 0;
                }
            }
            st.target = target;
            st.moveStart = now;
            moveStart = now;
            if (target)
                if (Creature* c = ObjectAccessor::GetCreature(*p, ObjectGuid(target)))
                    pet->GetMotionMaster()->MovePoint(1, c->GetPositionX(), c->GetPositionY(), c->GetPositionZ());
        }
        if (!target)
            return;
        Creature* corpse = ObjectAccessor::GetCreature(*p, ObjectGuid(target));
        bool done = !corpse || corpse->IsAlive() || corpse->loot.isLooted();
        if (!done && (pet->IsWithinDistInMap(corpse, 3.0f) || getMSTimeDiff(moveStart, now) > s.lootPetReachTimeoutMs))
        {
            Transfer(p, corpse);   // reached it, or unreachable: loot from where it is
            done = true;
            std::lock_guard<std::mutex> l(g_lock);
            ++State(key).looted;
        }
        if (done)
        {
            bool more;
            {
                std::lock_guard<std::mutex> l(g_lock);
                PetState& st = State(key);
                st.target = 0;
                more = !st.queue.empty();
            }
            if (!more)
                pet->GetMotionMaster()->MoveFollow(p, 2.0f, float(M_PI) * 0.75f);
        }
    }

    // Right-click on the companion: its owner gets a menu of the enabled roles; each opens the game's own window.
    enum CompanionAction : uint32 { ACT_VENDOR = 1, ACT_BANK = 2, ACT_MAIL = 3 };

    class ArpgCompanionCreatureScript : public CreatureScript
    {
    public:
        ArpgCompanionCreatureScript() : CreatureScript("arpg_companion") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            ClearGossipMenuFor(player);
            if (creature->GetOwnerGUID() != player->GetGUID() && creature->ToTempSummon() &&
                creature->ToTempSummon()->GetSummonerGUID() != player->GetGUID())
            {
                CloseGossipMenuFor(player);
                return true;   // someone else's companion
            }
            uint32 f = creature->GetUInt32Value(UNIT_FIELD_NPC_FLAGS);
            if (f & UNIT_NPC_FLAG_VENDOR)
                AddGossipItemFor(player, GOSSIP_ICON_VENDOR, (f & UNIT_NPC_FLAG_REPAIR) ? "Sell and repair" : "Sell", GOSSIP_SENDER_MAIN, ACT_VENDOR);
            if (f & UNIT_NPC_FLAG_BANKER)
                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Bank", GOSSIP_SENDER_MAIN, ACT_BANK);
            if (f & UNIT_NPC_FLAG_MAILBOX)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Mailbox", GOSSIP_SENDER_MAIN, ACT_MAIL);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            ClearGossipMenuFor(player);
            CloseGossipMenuFor(player);
            uint32 f = creature->GetUInt32Value(UNIT_FIELD_NPC_FLAGS);
            switch (action)
            {
                case ACT_VENDOR: if (f & UNIT_NPC_FLAG_VENDOR)  player->GetSession()->SendListInventory(creature->GetGUID()); break;
                case ACT_BANK:   if (f & UNIT_NPC_FLAG_BANKER)  player->GetSession()->SendShowBank(creature->GetGUID()); break;
                case ACT_MAIL:   if (f & UNIT_NPC_FLAG_MAILBOX) player->GetSession()->SendShowMailBox(creature->GetGUID()); break;
                default: break;
            }
            return true;
        }
    };

    class ArpgLootPetPlayerScript : public PlayerScript
    {
    public:
        ArpgLootPetPlayerScript() : PlayerScript("arpg_lootpet") { }

        void OnUpdate(Player* player, uint32 /*diff*/) override
        {
            if (player->IsInWorld() && Arpg::Scaling::IsRealPlayer(player))
                Tick(player);
        }

        void OnCreatureKill(Player* killer, Creature* killed) override
        {
            if (!killer || !killed || !Arpg::Scaling::IsRealPlayer(killer))
                return;
            std::lock_guard<std::mutex> l(g_lock);
            PetState& st = State(killer->GetGUID().GetCounter());
            if (st.on && st.queue.size() < 50)
            {
                st.queue.push_back(killed->GetGUID().GetRawValue());
                Arpg::TestEvents::Record(killer->GetGUID().GetCounter(), "lootpet queued corpse=" + std::to_string(killed->GetEntry()) +
                    " gold=" + std::to_string(killed->loot.gold) + " items=" + std::to_string(killed->loot.items.size()));
            }
        }

        void OnLogout(Player* player) override
        {
            uint64 pet = 0;
            {
                std::lock_guard<std::mutex> l(g_lock);
                auto it = g_state.find(player->GetGUID().GetCounter());
                if (it != g_state.end())
                {
                    pet = it->second.pet;
                    g_state.erase(it);
                }
            }
            Despawn(player, pet);
        }
    };
}

namespace Arpg::Loot
{
    void InstallLootPet()
    {
        new ArpgLootPetPlayerScript();
        new ArpgCompanionCreatureScript();
    }

    void SetLootPet(Player* player, bool on)
    {
        uint32 key = player->GetGUID().GetCounter();
        {
            std::lock_guard<std::mutex> l(g_lock);
            State(key).on = on;
            State(key).lastTick = 0;   // act on the next update
        }
        CharacterDatabase.PExecute("REPLACE INTO character_arpg_lootpet (guid, enabled) VALUES (%u, %u)", key, on ? 1u : 0u);
    }

    bool LootPetOn(Player* player)
    {
        std::lock_guard<std::mutex> l(g_lock);
        return State(player->GetGUID().GetCounter()).on;
    }

    std::string DescribeLootPet(Player* player)
    {
        std::lock_guard<std::mutex> l(g_lock);
        PetState& st = State(player->GetGUID().GetCounter());
        char flags[16];
        uint32 f = 0;
        if (Creature* c = st.pet ? ObjectAccessor::GetCreature(*player, ObjectGuid(st.pet)) : nullptr)
            f = c->GetUInt32Value(UNIT_FIELD_NPC_FLAGS);
        std::snprintf(flags, sizeof(flags), "0x%X", f);
        return std::string("lootpet ") + (st.on ? "on" : "off") + " pet=" + std::to_string(uint32(ObjectGuid(st.pet).GetCounter())) +
            " queue=" + std::to_string(st.queue.size() + (st.target ? 1 : 0)) + " looted=" + std::to_string(st.looted) + " npcflags=" + flags;
    }
}
