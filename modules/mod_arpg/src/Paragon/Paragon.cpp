#include "Paragon/Paragon.h"

#include "ArpgConfig.h"
#include "Core/Paragon.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "Chat.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldSession.h"

#include <mutex>
#include <unordered_map>

namespace
{
    struct Progress { int level = 0; uint64 xp = 0; bool loaded = false; };

    std::mutex g_lock;
    std::unordered_map<uint32, Progress> g_progress;   // player guid -> Paragon

    Arpg::Core::ParagonCurve Curve()
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        return { s.paragonXpBase, s.paragonXpGrowth };
    }

    Progress Load(uint32 guid)
    {
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_progress.find(guid);
            if (it != g_progress.end() && it->second.loaded)
                return it->second;
        }
        Progress p;
        if (QueryResult r = CharacterDatabase.PQuery("SELECT level, xp FROM character_paragon WHERE guid = %u", guid))
        {
            p.level = int(r->Fetch()[0].GetUInt32());
            p.xp = r->Fetch()[1].GetUInt64();
        }
        p.loaded = true;
        std::lock_guard<std::mutex> l(g_lock);
        g_progress[guid] = p;
        return p;
    }

    void Save(uint32 guid, Progress const& p)
    {
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_progress[guid] = p;
        }
        CharacterDatabase.PExecute("REPLACE INTO character_paragon (guid, level, xp) VALUES (%u, %u, " UI64FMTD ")", guid, uint32(p.level), p.xp);
    }

    bool AtMaxLevel(Player* p)
    {
        return p->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
    }

    class ArpgParagonPlayerScript : public PlayerScript
    {
    public:
        ArpgParagonPlayerScript() : PlayerScript("arpg_paragon") { }

        // Kill XP at the max level (the core would drop it): it becomes Paragon XP.
        void OnGiveXP(Player* player, uint32& amount, Unit* /*victim*/) override
        {
            Arpg::Settings const& s = sArpgConfig.Get();
            if (!s.paragonEnabled || !amount || !AtMaxLevel(player) || !Arpg::Scaling::IsRealPlayer(player))
                return;
            Arpg::Paragon::AddXp(player, uint64(amount), "kill");
        }

        // Quest XP at the max level (the core pays gold instead): Arpg.Paragon.QuestXpPct of it as Paragon XP too.
        void OnQuestRewarded(Player* player, Quest const* quest) override
        {
            Arpg::Settings const& s = sArpgConfig.Get();
            if (!s.paragonEnabled || !quest || !AtMaxLevel(player) || !Arpg::Scaling::IsRealPlayer(player))
                return;
            uint64 xp = uint64(double(quest->XPValue(player)) * sWorld->getRate(RATE_XP_QUEST) * s.paragonQuestXpPct / 100.0);
            if (xp)
                Arpg::Paragon::AddXp(player, xp, "quest");
        }

        void OnLogin(Player* player) override
        {
            Load(player->GetGUID().GetCounter());
        }

        void OnLogout(Player* player) override
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_progress.erase(player->GetGUID().GetCounter());
        }
    };
}

namespace Arpg::Paragon
{
    void InstallParagon()
    {
        new ArpgParagonPlayerScript();
    }

    int ParagonLevel(Player* player)
    {
        return Load(player->GetGUID().GetCounter()).level;
    }

    int AddXp(Player* player, uint64 xp, std::string const& why)
    {
        uint32 guid = player->GetGUID().GetCounter();
        Progress p = Load(guid);
        int gained = Core::AddParagonXp(Curve(), p.level, p.xp, xp);
        Save(guid, p);
        Arpg::TestEvents::Record(guid, "paragon xp=" + std::to_string(xp) + " (" + why + ") level=" + std::to_string(p.level) +
            (gained ? " up=" + std::to_string(gained) : ""));
        if (gained && player->GetSession())
            ChatHandler(player->GetSession()).PSendSysMessage("|cffffd100Paragon level %d!|r", p.level);
        return gained;
    }

    void SetParagon(Player* player, int level, uint64 xp)
    {
        Progress p;
        p.level = std::max(0, level);
        p.xp = xp;
        p.loaded = true;
        Save(player->GetGUID().GetCounter(), p);
    }

    std::string Describe(Player* player)
    {
        Progress p = Load(player->GetGUID().GetCounter());
        Core::ParagonCurve c = Curve();
        std::string out = "paragon level=" + std::to_string(p.level) + " xp=" + std::to_string(p.xp) + "/" +
            std::to_string(Core::ParagonXpToNext(c, p.level)) + " points";
        char const* names[] = { "core", "offense", "defense", "utility" };
        for (int t = 0; t < 4; ++t)
            out += std::string(" ") + names[t] + "=" + std::to_string(Core::PointsInTab(p.level, Core::ParagonTab(t)));
        return out;
    }
}
