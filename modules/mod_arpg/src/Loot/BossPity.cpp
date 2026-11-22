#include "Loot/BossPity.h"

#include "ArpgConfig.h"
#include "Core/Pity.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "Creature.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"

#include <mutex>
#include <unordered_map>

namespace
{
    std::mutex g_lock;
    std::unordered_map<uint64, uint32> g_pendingUnique;   // player guid -> boss entry whose Unique is owed
    std::unordered_map<uint64, uint32> g_pendingUniqueId; // player guid -> the specific Unique owed (0 = any of the slot)

    void Store(uint64 guid, int kills)
    {
        CharacterDatabase.DirectPExecute("INSERT INTO character_arpg_pity (guid, boss_entry, unique_misses, godroll_kills) VALUES (%u, 0, 0, %d) "
            "ON DUPLICATE KEY UPDATE godroll_kills = %d", uint32(guid), kills, kills);
    }

    class ArpgBossPityPlayerScript : public PlayerScript
    {
    public:
        ArpgBossPityPlayerScript() : PlayerScript("arpg_boss_pity") { }

        void OnCreatureKill(Player* killer, Creature* killed) override
        {
            if (!killer || !killed || !Arpg::Scaling::IsRealPlayer(killer) || !Arpg::Loot::IsBossEntry(killed->GetEntry()))
                return;
            int kills = Arpg::Loot::GodRollKills(killer) + 1;
            Store(killer->GetGUID().GetCounter(), kills);
            Arpg::TestEvents::Record(killer->GetGUID().GetCounter(), "boss kill entry=" + std::to_string(killed->GetEntry()) +
                " godroll_kills=" + std::to_string(kills) + (Arpg::Loot::GodRollPending(killer) ? " pending" : ""));

            // Unique ladder for this boss (20% + 20% per miss, guaranteed by the 5th kill).
            int misses = Arpg::Loot::UniqueMisses(killer, killed->GetEntry());
            bool won = Arpg::Core::RollWithPity(Arpg::Core::PityLadder{}, misses, rand_norm());
            Arpg::Loot::SetUniqueMisses(killer, killed->GetEntry(), misses);
            if (won)
            {
                std::lock_guard<std::mutex> l(g_lock);
                g_pendingUnique[killer->GetGUID().GetCounter()] = killed->GetEntry();
            }
            Arpg::TestEvents::Record(killer->GetGUID().GetCounter(), std::string("unique ladder ") + (won ? "won" : "missed") +
                " misses=" + std::to_string(misses));
        }
    };
}

namespace Arpg::Loot
{
    void InstallBossPity()
    {
        new ArpgBossPityPlayerScript();
    }

    bool IsBossEntry(uint32 creatureEntry)
    {
        CreatureTemplate const* t = sObjectMgr->GetCreatureTemplate(creatureEntry);
        return t && ((t->flags_extra & CREATURE_FLAG_EXTRA_DUNGEON_BOSS) || t->rank == CREATURE_ELITE_WORLDBOSS);
    }

    int GodRollKills(Player* player)
    {
        QueryResult r = CharacterDatabase.PQuery("SELECT godroll_kills FROM character_arpg_pity WHERE guid = %u AND boss_entry = 0",
            player->GetGUID().GetCounter());
        return r ? int(r->Fetch()[0].GetUInt16()) : 0;
    }

    bool GodRollPending(Player* player)
    {
        // Counter = boss kills since the last 5-affix item, this kill included: the Nth one owes a god roll.
        int kills = GodRollKills(player);
        return kills > 0 && Core::ForceGodRoll(kills - 1, sArpgConfig.Get().godRollPityEvery);
    }

    void GodRollGranted(Player* player)
    {
        Store(player->GetGUID().GetCounter(), 0);
    }

    void SetGodRollKills(Player* player, int kills)
    {
        Store(player->GetGUID().GetCounter(), std::max(0, kills));
    }

    int UniqueMisses(Player* player, uint32 bossEntry)
    {
        QueryResult r = CharacterDatabase.PQuery("SELECT unique_misses FROM character_arpg_pity WHERE guid = %u AND boss_entry = %u",
            player->GetGUID().GetCounter(), bossEntry);
        return r ? int(r->Fetch()[0].GetUInt16()) : 0;
    }

    void SetUniqueMisses(Player* player, uint32 bossEntry, int misses)
    {
        CharacterDatabase.DirectPExecute("INSERT INTO character_arpg_pity (guid, boss_entry, unique_misses, godroll_kills) VALUES (%u, %u, %d, 0) "
            "ON DUPLICATE KEY UPDATE unique_misses = %d", player->GetGUID().GetCounter(), bossEntry, std::max(0, misses), std::max(0, misses));
    }

    void SetPendingUnique(Player* player, uint32 creatureEntry, uint32 uniqueId)
    {
        std::lock_guard<std::mutex> l(g_lock);
        g_pendingUnique[player->GetGUID().GetCounter()] = creatureEntry;
        g_pendingUniqueId[player->GetGUID().GetCounter()] = uniqueId;
    }

    uint32 PendingUniqueId(Player* player)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_pendingUniqueId.find(player->GetGUID().GetCounter());
        return it == g_pendingUniqueId.end() ? 0 : it->second;
    }

    uint32 PendingUnique(Player* player, bool take)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_pendingUnique.find(player->GetGUID().GetCounter());
        if (it == g_pendingUnique.end())
            return 0;
        uint32 boss = it->second;
        if (take)
        {
            g_pendingUnique.erase(it);
            g_pendingUniqueId.erase(player->GetGUID().GetCounter());
        }
        return boss;
    }

    std::string DescribePity(Player* player)
    {
        return "pity godroll_kills=" + std::to_string(GodRollKills(player)) + " every=" +
            std::to_string(sArpgConfig.Get().godRollPityEvery) + " pending=" + (GodRollPending(player) ? "yes" : "no") +
            " unique_owed_by=" + std::to_string(PendingUnique(player, false));
    }
}
