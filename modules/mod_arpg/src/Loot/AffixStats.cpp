#include "Loot/AffixStats.h"

#include "ArpgHooks.h"
#include "Loot/AffixRoller.h"
#include "Loot/AffixEffects.h"
#include "Test/TestEvents.h"

#include "Item.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace
{
    using Arpg::Loot::StatLine;

    std::mutex g_lock;
    struct Applied
    {
        std::vector<StatLine> stats;
        std::vector<Arpg::Loot::EffectLine> effects;
    };
    // player guid -> (item guid -> lines applied)
    std::unordered_map<uint64, std::unordered_map<uint32, Applied>> g_applied;

    void ApplyStat(Player* p, uint32 stat, int32 v, bool apply)
    {
        float const f = float(v);
        switch (stat)
        {
            case ITEM_MOD_STRENGTH:  p->HandleStatModifier(UNIT_MOD_STAT_STRENGTH, BASE_VALUE, f, apply);  p->ApplyStatBuffMod(STAT_STRENGTH, f, apply); break;
            case ITEM_MOD_AGILITY:   p->HandleStatModifier(UNIT_MOD_STAT_AGILITY, BASE_VALUE, f, apply);   p->ApplyStatBuffMod(STAT_AGILITY, f, apply); break;
            case ITEM_MOD_INTELLECT: p->HandleStatModifier(UNIT_MOD_STAT_INTELLECT, BASE_VALUE, f, apply); p->ApplyStatBuffMod(STAT_INTELLECT, f, apply); break;
            case ITEM_MOD_SPIRIT:    p->HandleStatModifier(UNIT_MOD_STAT_SPIRIT, BASE_VALUE, f, apply);    p->ApplyStatBuffMod(STAT_SPIRIT, f, apply); break;
            case ITEM_MOD_STAMINA:   p->HandleStatModifier(UNIT_MOD_STAT_STAMINA, BASE_VALUE, f, apply);   p->ApplyStatBuffMod(STAT_STAMINA, f, apply); break;
            case ITEM_MOD_CRIT_RATING:
                p->ApplyRatingMod(CR_CRIT_MELEE, v, apply); p->ApplyRatingMod(CR_CRIT_RANGED, v, apply); p->ApplyRatingMod(CR_CRIT_SPELL, v, apply); break;
            case ITEM_MOD_HASTE_RATING:
                p->ApplyRatingMod(CR_HASTE_MELEE, v, apply); p->ApplyRatingMod(CR_HASTE_RANGED, v, apply); p->ApplyRatingMod(CR_HASTE_SPELL, v, apply); break;
            case ITEM_MOD_HIT_RATING:
                p->ApplyRatingMod(CR_HIT_MELEE, v, apply); p->ApplyRatingMod(CR_HIT_RANGED, v, apply); p->ApplyRatingMod(CR_HIT_SPELL, v, apply); break;
            case ITEM_MOD_EXPERTISE_RATING: p->ApplyRatingMod(CR_EXPERTISE, v, apply); break;
            case ITEM_MOD_MASTERY_RATING:   p->ApplyRatingMod(CR_MASTERY, v, apply); break;
            case ITEM_MOD_DODGE_RATING:     p->ApplyRatingMod(CR_DODGE, v, apply); break;
            case ITEM_MOD_PARRY_RATING:     p->ApplyRatingMod(CR_PARRY, v, apply); break;
            default: break;
        }
    }

    void ItemBonusesHook(Player* player, Item const* item, uint8 /*slot*/, bool apply)
    {
        if (!player || !item)
            return;
        uint64 pg = player->GetGUID().GetCounter();
        uint32 ig = item->GetGUID().GetCounter();
        Arpg::Loot::ApplyUniquePower(player, ig, apply);   // idempotent per item; no-op for non-Uniques
        Applied lines;
        if (apply)
        {
            lines.stats = Arpg::Loot::ItemStatLines(ig, false);   // own lock; bots' items simply have none
            lines.effects = Arpg::Loot::ItemEffectLines(ig);
            if (lines.stats.empty() && lines.effects.empty())
                return;
            std::lock_guard<std::mutex> l(g_lock);
            auto& items = g_applied[pg];
            if (items.count(ig))
                return;                                      // already applied: never twice
            items[ig] = lines;
        }
        else
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto p = g_applied.find(pg);
            if (p == g_applied.end())
                return;
            auto it = p->second.find(ig);
            if (it == p->second.end())
                return;                                      // nothing of ours on it
            lines = std::move(it->second);
            p->second.erase(it);
        }
        for (StatLine const& sl : lines.stats)
            ApplyStat(player, sl.stat, sl.value, apply);
        Arpg::Loot::ApplyEffectLines(player, lines.effects, apply);
        if (Arpg::TestEvents::Enabled())
            Arpg::TestEvents::Record(pg, std::string(apply ? "affix-stats apply" : "affix-stats remove") + " guid=" + std::to_string(ig) +
                " lines=" + std::to_string(lines.stats.size() + lines.effects.size()));
    }

    class ArpgAffixStatsPlayerScript : public PlayerScript
    {
    public:
        ArpgAffixStatsPlayerScript() : PlayerScript("arpg_affix_stats") { }

        // The Player object goes away; a later login re-applies from scratch.
        void OnLogout(Player* player) override
        {
            {
                std::lock_guard<std::mutex> l(g_lock);
                g_applied.erase(player->GetGUID().GetCounter());
            }
            Arpg::Loot::ForgetEffects(player->GetGUID().GetCounter());
        }
    };

    char const* StatName(uint32 stat)
    {
        switch (stat)
        {
            case ITEM_MOD_STRENGTH: return "str"; case ITEM_MOD_AGILITY: return "agi"; case ITEM_MOD_INTELLECT: return "int";
            case ITEM_MOD_SPIRIT: return "spi"; case ITEM_MOD_STAMINA: return "sta"; case ITEM_MOD_CRIT_RATING: return "crit";
            case ITEM_MOD_HASTE_RATING: return "haste"; case ITEM_MOD_HIT_RATING: return "hit"; case ITEM_MOD_EXPERTISE_RATING: return "exp";
            case ITEM_MOD_MASTERY_RATING: return "mastery"; case ITEM_MOD_DODGE_RATING: return "dodge"; case ITEM_MOD_PARRY_RATING: return "parry";
            default: return "?";
        }
    }
}

namespace Arpg::Loot
{
    void ApplyStatLine(Player* player, uint32 itemMod, int32 value, bool apply)
    {
        if (player && value)
            ApplyStat(player, itemMod, value, apply);
    }

    uint32 StatModFromName(std::string const& name)
    {
        static std::map<std::string, uint32> const m =
        {
            { "strength", ITEM_MOD_STRENGTH }, { "agility", ITEM_MOD_AGILITY }, { "intellect", ITEM_MOD_INTELLECT },
            { "stamina", ITEM_MOD_STAMINA }, { "spirit", ITEM_MOD_SPIRIT }, { "crit_rating", ITEM_MOD_CRIT_RATING },
            { "haste_rating", ITEM_MOD_HASTE_RATING }, { "mastery_rating", ITEM_MOD_MASTERY_RATING },
            { "hit_rating", ITEM_MOD_HIT_RATING }, { "expertise_rating", ITEM_MOD_EXPERTISE_RATING },
            { "dodge_rating", ITEM_MOD_DODGE_RATING }, { "parry_rating", ITEM_MOD_PARRY_RATING },
        };
        auto it = m.find(name);
        return it == m.end() ? 0 : it->second;
    }

    void InstallAffixStats()
    {
        ArpgHooks::ItemBonuses = &ItemBonusesHook;
        new ArpgAffixStatsPlayerScript();
    }

    std::string DescribeApplied(Player* player)
    {
        std::map<std::string, int64> totals;
        size_t items = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto p = g_applied.find(player->GetGUID().GetCounter());
            if (p != g_applied.end())
            {
                items = p->second.size();
                for (auto const& kv : p->second)
                    for (StatLine const& sl : kv.second.stats)
                        totals[StatName(sl.stat)] += sl.value;
            }
        }
        std::string out = "applied items=" + std::to_string(items);
        for (auto const& kv : totals)
            out += " " + kv.first + "=+" + std::to_string(kv.second);
        return out;
    }
}
