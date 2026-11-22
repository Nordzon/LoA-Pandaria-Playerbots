#include "Paragon/Paragon.h"

#include "ArpgConfig.h"
#include "Core/Paragon.h"
#include "Loot/AffixEffects.h"
#include "Core/AffixEffects.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "Chat.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "Timer.h"
#include "World.h"
#include "WorldSession.h"

#include <map>
#include <cstdio>
#include <mutex>
#include <unordered_map>

namespace
{
    struct Progress { int level = 0; uint64 xp = 0; bool loaded = false; };

    // What is applied on the character right now (so it can be removed exactly).
    struct Applied
    {
        bool on = false;
        float strPct = 0, agiPct = 0, intPct = 0, staPct = 0, armorPct = 0;
        int32 crit = 0, haste = 0, mastery = 0, dodge = 0, parry = 0;
        int32 move = 0, mf = 0, gf = 0;
    };

    std::mutex g_lock;
    std::unordered_map<uint32, Progress> g_progress;   // player guid -> Paragon
    std::unordered_map<uint32, std::map<std::pair<int, int>, int>> g_points;   // player guid -> (tab, stat) -> points spent
    std::unordered_map<uint32, Applied> g_applied;     // player guid -> bonuses applied

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

    std::map<std::pair<int, int>, int> LoadPoints(uint32 guid)
    {
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_points.find(guid);
            if (it != g_points.end())
                return it->second;
        }
        std::map<std::pair<int, int>, int> pts;
        if (QueryResult r = CharacterDatabase.PQuery("SELECT tab, stat, points FROM character_paragon_point WHERE guid = %u", guid))
            do
            {
                Field* f = r->Fetch();
                pts[{ int(f[0].GetUInt8()), int(f[1].GetUInt8()) }] = int(f[2].GetUInt16());
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_points[guid] = pts;
        return pts;
    }

    int SpentInTab(std::map<std::pair<int, int>, int> const& pts, int tab)
    {
        int n = 0;
        for (auto const& kv : pts)
            if (kv.first.first == tab)
                n += kv.second;
        return n;
    }

    void Ratings(Player* p, CombatRating const* list, int n, int32 value, bool apply)
    {
        if (value)
            for (int i = 0; i < n; ++i)
                p->ApplyRatingMod(list[i], value, apply);
    }

    void ApplyAll(Player* p, Applied const& a, bool apply)
    {
        if (!a.on)
            return;
        if (a.strPct)
            p->HandleStatModifier(UNIT_MOD_STAT_STRENGTH, TOTAL_PCT, a.strPct, apply);
        if (a.agiPct)
            p->HandleStatModifier(UNIT_MOD_STAT_AGILITY, TOTAL_PCT, a.agiPct, apply);
        if (a.intPct)
            p->HandleStatModifier(UNIT_MOD_STAT_INTELLECT, TOTAL_PCT, a.intPct, apply);
        if (a.staPct)
            p->HandleStatModifier(UNIT_MOD_STAT_STAMINA, TOTAL_PCT, a.staPct, apply);
        if (a.armorPct)
            p->HandleStatModifier(UNIT_MOD_ARMOR, TOTAL_PCT, a.armorPct, apply);
        static CombatRating const crit[] = { CR_CRIT_MELEE, CR_CRIT_RANGED, CR_CRIT_SPELL };
        static CombatRating const haste[] = { CR_HASTE_MELEE, CR_HASTE_RANGED, CR_HASTE_SPELL };
        static CombatRating const mastery[] = { CR_MASTERY };
        static CombatRating const dodge[] = { CR_DODGE };
        static CombatRating const parry[] = { CR_PARRY };
        Ratings(p, crit, 3, a.crit, apply);
        Ratings(p, haste, 3, a.haste, apply);
        Ratings(p, mastery, 1, a.mastery, apply);
        Ratings(p, dodge, 1, a.dodge, apply);
        Ratings(p, parry, 1, a.parry, apply);
        std::vector<Arpg::Loot::EffectLine> lines;
        if (a.mf)
            lines.push_back({ int(Arpg::Core::Effect::MagicFindPct), a.mf });
        if (a.gf)
            lines.push_back({ int(Arpg::Core::Effect::GoldFindPct), a.gf });
        if (!lines.empty())
            Arpg::Loot::ApplyEffectLines(p, lines, apply);
        Arpg::Loot::SetParagonMoveSpeed(p, apply ? a.move : 0);
    }

    // Removes what is applied, computes the bonuses from the spent points and applies them.
    void Reapply(Player* p)
    {
        uint32 guid = p->GetGUID().GetCounter();
        Applied old;
        {
            std::lock_guard<std::mutex> l(g_lock);
            old = g_applied[guid];
        }
        ApplyAll(p, old, false);
        Arpg::Settings const& s = sArpgConfig.Get();
        auto pts = LoadPoints(guid);
        auto at = [&](int tab, int stat) { auto it = pts.find({ tab, stat }); return it == pts.end() ? 0 : it->second; };
        Applied a;
        a.on = true;
        // Core (owner 2026-11-22: any stat, no "main stat" guess): 0 strength, 1 stamina, 2 agility, 3 intellect.
        a.strPct = s.paragonStatPctPerPoint * at(0, 0);
        a.staPct = s.paragonStatPctPerPoint * at(0, 1);
        a.agiPct = s.paragonStatPctPerPoint * at(0, 2);
        a.intPct = s.paragonStatPctPerPoint * at(0, 3);
        a.crit = s.paragonRatingPerPoint * at(1, 0);
        a.haste = s.paragonRatingPerPoint * at(1, 1);
        a.mastery = s.paragonRatingPerPoint * at(1, 2);
        a.armorPct = s.paragonArmorPctPerPoint * at(2, 0);
        a.dodge = s.paragonRatingPerPoint * at(2, 1);
        a.parry = s.paragonRatingPerPoint * at(2, 2);
        a.move = Arpg::Core::ParagonBonus({ s.paragonMoveSpeedPerPoint, s.paragonMoveSpeedCap }, at(3, 0));
        a.mf = Arpg::Core::ParagonBonus({ s.paragonFindPerPoint, s.paragonMagicFindCap }, at(3, 1));
        a.gf = Arpg::Core::ParagonBonus({ s.paragonFindPerPoint, 0 }, at(3, 2));
        ApplyAll(p, a, true);
        p->UpdateAllStats();
        std::lock_guard<std::mutex> l(g_lock);
        g_applied[guid] = a;
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
            uint64 xp = amount;
            if (s.uncappedZones.count(player->GetZoneId()))   // world tier: harder monsters, more Paragon XP
                xp = uint64(double(xp) * Arpg::Core::WorldTierMultiplier(Arpg::Paragon::ParagonLevel(player), s.paragonWorldTierPct,
                    s.paragonWorldTierCapPct) + 0.5);
            Arpg::Paragon::AddXp(player, xp, "kill");
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
            if (sArpgConfig.Get().paragonEnabled)
            {
                Reapply(player);   // bonuses from spent points
                Arpg::Paragon::SendData(player);
            }
        }

        void OnLogout(Player* player) override
        {
            std::lock_guard<std::mutex> l(g_lock);
            uint32 guid = player->GetGUID().GetCounter();
            g_progress.erase(guid);
            g_points.erase(guid);
            g_applied.erase(guid);
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
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cffffd100Paragon level %d!|r", p.level);
            SendData(player);   // the addon window updates
        }
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

    bool ParseTabStat(std::string const& tab, std::string const& stat, int& tabOut, int& statOut)
    {
        static char const* const tabs[] = { "core", "offense", "defense", "utility" };
        static char const* const stats[4][4] = { { "strength", "stamina", "agility", "intellect" }, { "crit", "haste", "mastery", nullptr },
                                                 { "armor", "dodge", "parry", nullptr }, { "speed", "magicfind", "goldfind", nullptr } };
        for (int t = 0; t < 4; ++t)
            if (tab == tabs[t])
                for (int i = 0; i < 4; ++i)
                    if (stats[t][i] && stat == stats[t][i])
                    {
                        tabOut = t;
                        statOut = i;
                        return true;
                    }
        return false;
    }

    bool Spend(Player* player, int tab, int stat, int n, std::string& out)
    {
        uint32 guid = player->GetGUID().GetCounter();
        int level = Load(guid).level;
        auto pts = LoadPoints(guid);
        int free = Core::PointsInTab(level, Core::ParagonTab(tab)) - SpentInTab(pts, tab);
        if (n < 1 || n > free)
        {
            out = "not enough points: tab has " + std::to_string(std::max(0, free)) + " unspent";
            return false;
        }
        // Measure the stat the points go to (before / after).
        auto measure = [&]() -> int64
        {
            switch (tab * 10 + stat)
            {
                case 0: return int64(player->GetStat(STAT_STRENGTH));
                case 1: return int64(player->GetStat(STAT_STAMINA));
                case 2: return int64(player->GetStat(STAT_AGILITY));
                case 3: return int64(player->GetStat(STAT_INTELLECT));
                case 10: return int64(player->GetUInt32Value(PLAYER_FIELD_COMBAT_RATINGS + CR_CRIT_MELEE));
                case 11: return int64(player->GetUInt32Value(PLAYER_FIELD_COMBAT_RATINGS + CR_HASTE_MELEE));
                case 12: return int64(player->GetUInt32Value(PLAYER_FIELD_COMBAT_RATINGS + CR_MASTERY));
                case 20: return int64(player->GetArmor());
                case 21: return int64(player->GetUInt32Value(PLAYER_FIELD_COMBAT_RATINGS + CR_DODGE));
                case 22: return int64(player->GetUInt32Value(PLAYER_FIELD_COMBAT_RATINGS + CR_PARRY));
                default: return Arpg::Loot::EffectTotal(player, stat == 1 ? Core::Effect::MagicFindPct : Core::Effect::GoldFindPct);
            }
        };
        int64 before = measure();
        int now = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            now = (g_points[guid][{ tab, stat }] += n);
        }
        CharacterDatabase.PExecute("REPLACE INTO character_paragon_point (guid, tab, stat, points) VALUES (%u, %u, %u, %u)",
            guid, uint32(tab), uint32(stat), uint32(now));
        Reapply(player);
        int64 after = measure();
        out = "spent tab=" + std::to_string(tab) + " stat=" + std::to_string(stat) + " points=" + std::to_string(now) +
            " before=" + std::to_string(before) + " after=" + std::to_string(after) + " diff=" + std::to_string(after - before);
        if (before > 0)
        {
            char pct[32];
            std::snprintf(pct, sizeof(pct), " change_pct=%.1f", double(after - before) * 100.0 / double(before));
            out += pct;
        }
        Arpg::TestEvents::Record(guid, "paragon " + out);
        return true;
    }

    void Respec(Player* player)
    {
        uint32 guid = player->GetGUID().GetCounter();
        Applied old;
        {
            std::lock_guard<std::mutex> l(g_lock);
            old = g_applied[guid];
            g_points[guid].clear();
            g_applied[guid] = Applied();
        }
        ApplyAll(player, old, false);
        player->UpdateAllStats();
        CharacterDatabase.PExecute("DELETE FROM character_paragon_point WHERE guid = %u", guid);
    }

    std::string DescribeApplied(Player* player)
    {
        Applied a;
        {
            std::lock_guard<std::mutex> l(g_lock);
            a = g_applied[player->GetGUID().GetCounter()];
        }
        char buf[256];
        std::snprintf(buf, sizeof(buf), "applied str_pct=%.1f agi_pct=%.1f int_pct=%.1f stamina_pct=%.1f crit=%d haste=%d mastery=%d armor_pct=%.1f dodge=%d parry=%d move=%d mf=%d gf=%d",
            a.strPct, a.agiPct, a.intPct, a.staPct, a.crit, a.haste, a.mastery, a.armorPct, a.dodge, a.parry, a.move, a.mf, a.gf);
        return buf;
    }

    std::string DataLine(Player* player)
    {
        uint32 guid = player->GetGUID().GetCounter();
        Progress p = Load(guid);
        auto pts = LoadPoints(guid);
        auto at = [&](int tab, int stat) { auto it = pts.find({ tab, stat }); return it == pts.end() ? 0 : it->second; };
        Arpg::Settings const& s = sArpgConfig.Get();
        std::string out = "ARPGP|" + std::to_string(p.level) + "|" + std::to_string(p.xp) + "|" +
            std::to_string(Core::ParagonXpToNext(Curve(), p.level)) + "|";
        for (int t = 0; t < 4; ++t)
            out += (t ? "," : "") + std::to_string(std::max(0, Core::PointsInTab(p.level, Core::ParagonTab(t)) - SpentInTab(pts, t)));
        int const counts[4] = { 4, 3, 3, 3 };   // core: strength, stamina, agility, intellect
        for (int t = 0; t < 4; ++t)
        {
            out += "|";
            for (int i = 0; i < counts[t]; ++i)
                out += (i ? "," : "") + std::to_string(at(t, i));
        }
        char cfg[128];
        std::snprintf(cfg, sizeof(cfg), "|%.2f,%d,%.2f,%d,%d,%d,%d", s.paragonStatPctPerPoint, s.paragonRatingPerPoint,
            s.paragonArmorPctPerPoint, s.paragonMoveSpeedPerPoint, s.paragonMoveSpeedCap, s.paragonFindPerPoint, s.paragonMagicFindCap);
        return out + cfg;
    }

    void SendData(Player* player)
    {
        if (player && player->GetSession())
            ChatHandler(player->GetSession()).SendSysMessage(DataLine(player).c_str());
    }
}
