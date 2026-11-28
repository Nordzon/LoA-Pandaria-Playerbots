#include "Character/ProfessionShop.h"

#include "ArpgConfig.h"
#include "Test/TestEvents.h"

#include "ArpgHooks.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "WorldSession.h"

#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace Arpg::Character
{
    namespace
    {
        uint32 const CompanionEntry = 1000001;
        uint32 const RecipeCopyOffset = 1100000;   // gen_recipe_shop.py COPY_OFFSET

        // Menu order: the 11 primary professions, then Cooking, First Aid, Fishing.
        std::pair<uint32, char const*> const Skills[] = {
            { SKILL_ALCHEMY, "Alchemy" }, { SKILL_BLACKSMITHING, "Blacksmithing" }, { SKILL_ENCHANTING, "Enchanting" },
            { SKILL_ENGINEERING, "Engineering" }, { SKILL_HERBALISM, "Herbalism" }, { SKILL_INSCRIPTION, "Inscription" },
            { SKILL_JEWELCRAFTING, "Jewelcrafting" }, { SKILL_LEATHERWORKING, "Leatherworking" }, { SKILL_MINING, "Mining" },
            { SKILL_SKINNING, "Skinning" }, { SKILL_TAILORING, "Tailoring" }, { SKILL_COOKING, "Cooking" },
            { SKILL_FIRST_AID, "First Aid" }, { SKILL_FISHING, "Fishing" } };

        struct Shop
        {
            std::unordered_map<uint32, TrainerSpellData> trainers;                    // skill -> trainer list
            std::map<uint32, std::map<uint32, std::pair<uint32, uint32>>> pages;     // skill -> page -> (vendor, count)
        };

        std::mutex g_lock;
        std::shared_ptr<Shop> g_shop = std::make_shared<Shop>();
        std::vector<std::shared_ptr<Shop>> g_retired;   // a reload keeps old lists alive (a window may still point at one)
        std::unordered_map<uint32, uint32> g_selected;  // player guid -> skill whose trainer list the companion serves

        std::shared_ptr<Shop> Current()
        {
            std::lock_guard<std::mutex> l(g_lock);
            return g_shop;
        }

        // Same rules as ObjectMgr::AddSpellToTrainer (valid spell, learn-spell effects give the learned spells).
        bool AddSpell(TrainerSpellData& data, uint32 spell, uint32 cost, uint32 reqSkill, uint32 reqSkillValue, uint32 reqLevel)
        {
            SpellInfo const* info = sSpellMgr->GetSpellInfo(spell);
            if (!info || !SpellMgr::IsSpellValid(info, nullptr, false))
                return false;
            TrainerSpell& t = data.spellList[spell];
            t.spell = spell;
            t.spellCost = cost;
            t.reqSkill = reqSkill;
            t.reqSkillValue = reqSkillValue;
            t.reqLevel = reqLevel ? reqLevel : info->SpellLevel;
            t.learnedSpell[0] = spell;
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                if (info->Effects[i].Effect != SPELL_EFFECT_LEARN_SPELL)
                    continue;
                if (t.learnedSpell[0] == spell)
                    t.learnedSpell[0] = 0;
                uint32 const target = info->Effects[i].TargetA.GetTarget();
                if (target != 0 && target != TARGET_UNIT_TARGET_ALLY && target != TARGET_UNIT_TARGET_ANY && target != TARGET_UNIT_CASTER)
                    continue;
                t.learnedSpell[i] = info->Effects[i].TriggerSpell;
            }
            data.trainerType = 2;   // profession trainer window
            return true;
        }

        TrainerSpellData const* ServeTrainer(Player* player, Creature* trainer)
        {
            if (!player || !trainer || trainer->GetEntry() != CompanionEntry)
                return nullptr;
            std::lock_guard<std::mutex> l(g_lock);
            auto sel = g_selected.find(player->GetGUID().GetCounter());
            if (sel == g_selected.end())
                return nullptr;
            auto it = g_shop->trainers.find(sel->second);
            return it == g_shop->trainers.end() ? nullptr : &it->second;
        }
    }

    std::string LoadProfessionShop()
    {
        auto shop = std::make_shared<Shop>();
        std::string skills;
        for (auto const& s : Skills)
            skills += (skills.empty() ? "" : ",") + std::to_string(s.first);
        uint32 spells = 0;
        // Every trainer row (direct or in a reference list) of these professions; a spell taught by several trainers once,
        // at its lowest price and requirement.
        if (QueryResult r = WorldDatabase.PQuery("SELECT spell, MIN(spellcost), reqskill, MIN(reqskillvalue), MIN(reqlevel) "
            "FROM npc_trainer WHERE spell > 0 AND reqskill IN (%s) GROUP BY spell, reqskill", skills.c_str()))
            do
            {
                Field* f = r->Fetch();
                uint32 const skill = f[2].GetUInt16();
                if (AddSpell(shop->trainers[skill], uint32(f[0].GetInt32()), f[1].GetUInt32(), skill, f[3].GetUInt16(),
                        f[4].GetUInt8()))
                    ++spells;
            } while (r->NextRow());
        uint32 recipes = 0;
        if (QueryResult r = WorldDatabase.Query("SELECT skill, page, vendor, COUNT(*) FROM arpg_recipe_shop GROUP BY skill, page, vendor"))
            do
            {
                Field* f = r->Fetch();
                shop->pages[f[0].GetUInt16()][f[1].GetUInt8()] = { f[2].GetUInt32(), uint32(f[3].GetUInt64()) };
                recipes += uint32(f[3].GetUInt64());
            } while (r->NextRow());
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_retired.push_back(g_shop);
            g_shop = shop;
        }
        return std::to_string(spells) + " profession trainer spells, " + std::to_string(recipes) + " shop recipes";
    }

    void InstallProfessionShop()
    {
        ArpgHooks::TrainerSpells = &ServeTrainer;
    }

    std::vector<uint32> ShopSkills(Player* player)
    {
        std::vector<uint32> out;
        for (auto const& s : Skills)
            if (player->HasSkill(s.first))
                out.push_back(s.first);
        return out;
    }

    std::string SkillName(uint32 skill)
    {
        for (auto const& s : Skills)
            if (s.first == skill)
                return s.second;
        return "Skill " + std::to_string(skill);
    }

    void SelectTrainer(Player* player, uint32 skill)
    {
        std::lock_guard<std::mutex> l(g_lock);
        g_selected[player->GetGUID().GetCounter()] = skill;
    }

    bool OpenProfessionTrainer(Player* player, Creature* companion, uint32 skill, std::string& out)
    {
        auto shop = Current();
        auto it = shop->trainers.find(skill);
        if (it == shop->trainers.end() || it->second.spellList.empty())
        {
            out = "No trainer for " + SkillName(skill) + ".";
            return false;
        }
        SelectTrainer(player, skill);
        Arpg::TestEvents::Record(player->GetGUID().GetCounter(), "profession trainer skill=" + std::to_string(skill) +
            " spells=" + std::to_string(it->second.spellList.size()));
        player->GetSession()->SendTrainerList(companion->GetGUID(), SkillName(skill) + " Trainer");
        return true;
    }

    std::vector<std::pair<uint32, uint32>> RecipePages(uint32 skill)
    {
        std::vector<std::pair<uint32, uint32>> out;
        auto shop = Current();
        auto it = shop->pages.find(skill);
        if (it != shop->pages.end())
            for (auto const& p : it->second)
                out.emplace_back(p.first, p.second.second);
        return out;
    }

    bool OpenRecipePage(Player* player, Creature* companion, uint32 skill, uint32 page, std::string& out)
    {
        auto shop = Current();
        auto it = shop->pages.find(skill);
        if (it == shop->pages.end() || !it->second.count(page))
        {
            out = "No recipes for " + SkillName(skill) + ".";
            return false;
        }
        uint32 const vendor = it->second.at(page).first;
        Arpg::TestEvents::Record(player->GetGUID().GetCounter(), "recipe shop skill=" + std::to_string(skill) + " page=" +
            std::to_string(page) + " vendor=" + std::to_string(vendor));
        player->GetSession()->SendListInventory(companion->GetGUID(), vendor);
        return true;
    }

    std::string DescribeProfessionShop(uint32 skill)
    {
        auto shop = Current();
        auto t = shop->trainers.find(skill);
        uint32 recipes = 0, pages = 0;
        if (auto p = shop->pages.find(skill); p != shop->pages.end())
            for (auto const& kv : p->second)
            {
                recipes += kv.second.second;
                ++pages;
            }
        return "profshop skill=" + std::to_string(skill) + " trainer_spells=" +
            std::to_string(t == shop->trainers.end() ? 0 : t->second.spellList.size()) + " recipes=" + std::to_string(recipes) +
            " pages=" + std::to_string(pages) + " copy_offset=" + std::to_string(RecipeCopyOffset);
    }

    std::string DescribeServedTrainer(Player* player)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto sel = g_selected.find(player->GetGUID().GetCounter());
        if (sel == g_selected.end())
            return "served none";
        auto it = g_shop->trainers.find(sel->second);
        return "served skill=" + std::to_string(sel->second) + " spells=" +
            std::to_string(it == g_shop->trainers.end() ? 0 : it->second.spellList.size());
    }
}
