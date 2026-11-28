#include "Character/ProfessionGuard.h"

#include "ArpgConfig.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "ArpgHooks.h"
#include "Chat.h"
#include "DBCStores.h"
#include "Log.h"
#include "Player.h"
#include "SharedDefines.h"
#include "SpellMgr.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace Arpg::Character
{
    namespace
    {
        // The 11 primary professions of MoP (game ids, not tunables).
        std::pair<char const*, uint32> const Professions[] = {
            { "alchemy", SKILL_ALCHEMY }, { "blacksmithing", SKILL_BLACKSMITHING }, { "enchanting", SKILL_ENCHANTING },
            { "engineering", SKILL_ENGINEERING }, { "herbalism", SKILL_HERBALISM }, { "inscription", SKILL_INSCRIPTION },
            { "jewelcrafting", SKILL_JEWELCRAFTING }, { "leatherworking", SKILL_LEATHERWORKING }, { "mining", SKILL_MINING },
            { "skinning", SKILL_SKINNING }, { "tailoring", SKILL_TAILORING } };

        // Apprentice spell of each primary profession (game ids): learning it teaches the skill.
        std::pair<uint32, uint32> const Apprentice[] = {
            { SKILL_ALCHEMY, 2259 }, { SKILL_BLACKSMITHING, 2018 }, { SKILL_ENCHANTING, 7411 }, { SKILL_ENGINEERING, 4036 },
            { SKILL_HERBALISM, 2366 }, { SKILL_INSCRIPTION, 45357 }, { SKILL_JEWELCRAFTING, 25229 },
            { SKILL_LEATHERWORKING, 2108 }, { SKILL_MINING, 2575 }, { SKILL_SKINNING, 8613 }, { SKILL_TAILORING, 3908 } };

        uint32 ApprenticeOf(uint32 skill)
        {
            for (auto const& a : Apprentice)
                if (a.first == skill)
                    return a.second;
            return 0;
        }

        char const* NameOf(uint32 skill)
        {
            for (auto const& p : Professions)
                if (p.second == skill)
                    return p.first;
            return nullptr;
        }

        void OnSkillChanged(Player* player, uint32 skill, uint32 oldValue, uint32 newValue)
        {
            if (oldValue && newValue)
                return;   // a skill-up, not a learn / removal
            char const* name = NameOf(skill);
            if (!name || !player)
                return;
            std::string const line = std::string("profession ") + (newValue ? "learned " : "removed ") + name + " skill=" +
                std::to_string(skill) + " value=" + std::to_string(newValue ? newValue : oldValue);
            Arpg::TestEvents::Record(player->GetGUID().GetCounter(), line);
            if (Arpg::Scaling::IsRealPlayer(player))
                TC_LOG_INFO("server.worldserver", "[ARPG] %s %s", player->GetName().c_str(), line.c_str());
        }

        bool g_abandoning = false;   // set while AbandonProfession removes a skill

        bool OnUnlearnSkill(Player* player, uint32 skill)
        {
            if (g_abandoning || !NameOf(skill) || sArpgConfig.Get().m9.allowClientAbandon)
                return true;
            Arpg::TestEvents::Record(player->GetGUID().GetCounter(), std::string("profession abandon refused ") + NameOf(skill));
            if (Arpg::Scaling::IsRealPlayer(player))
                TC_LOG_INFO("server.worldserver", "[ARPG] %s profession abandon refused %s", player->GetName().c_str(), NameOf(skill));
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffff8800Kept your %s.|r The game's abandon button is switched off here (you can have all 11 professions). "
                "To really drop it, type: .professions abandon %s", NameOf(skill), NameOf(skill));
            return false;
        }
    }

    void InstallProfessionGuard()
    {
        ArpgHooks::SkillChanged = &OnSkillChanged;
        ArpgHooks::UnlearnSkillAllowed = &OnUnlearnSkill;
    }

    uint32 ProfessionByName(std::string name)
    {
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        if (name == "jc")
            name = "jewelcrafting";
        if (name == "lw")
            name = "leatherworking";
        if (name == "bs")
            name = "blacksmithing";
        if (name.size() < 3)
            return 0;
        for (auto const& p : Professions)
            if (std::string(p.first).rfind(name, 0) == 0)
                return p.second;
        return 0;
    }

    bool AbandonProfession(Player* player, uint32 skill, std::string& out)
    {
        char const* name = NameOf(skill);
        if (!player || !name)
        {
            out = "not a profession (alchemy, blacksmithing, enchanting, engineering, herbalism, inscription, jewelcrafting, "
                  "leatherworking, mining, skinning, tailoring)";
            return false;
        }
        if (!player->HasSkill(skill))
        {
            out = std::string("you don't know ") + name;
            return false;
        }
        g_abandoning = true;
        player->SetSkill(uint16(skill), 0, 0, 0);
        g_abandoning = false;
        out = std::string("abandoned ") + name;
        return true;
    }

    std::vector<std::pair<std::string, uint32>> LearnableProfessions(Player* player)
    {
        std::vector<std::pair<std::string, uint32>> out;
        for (auto const& p : Professions)
        {
            uint32 const spell = ApprenticeOf(p.second);
            if (spell && sSpellMgr->GetSpellInfo(spell) && (!player->HasSkill(p.second) || !player->HasSpell(spell)))
                out.emplace_back(p.first, p.second);
        }
        return out;
    }

    bool LearnProfession(Player* player, uint32 skill, std::string& out)
    {
        char const* name = NameOf(skill);
        uint32 const spell = ApprenticeOf(skill);
        if (!player || !name || !spell || !sSpellMgr->GetSpellInfo(spell))
        {
            out = "not a profession";
            return false;
        }
        if (player->HasSkill(skill) && player->HasSpell(spell))
        {
            out = std::string("you already know ") + name;
            return false;
        }
        M9Settings const& s = sArpgConfig.Get().m9;
        if (player->GetLevel() < s.professionLevel)
        {
            out = "professions come at level " + std::to_string(s.professionLevel);
            return false;
        }
        if (!player->HasEnoughMoney(uint64(s.professionCost)))
        {
            out = "not enough money";
            return false;
        }
        player->ModifyMoney(-int64(s.professionCost));
        uint32 const before = player->HasSkill(skill) ? player->GetPureSkillValue(skill) : 0;
        player->LearnSpell(spell, false);
        if (before > 1 && player->GetPureSkillValue(skill) < before)   // a bare skill keeps its points
            player->SetSkill(uint16(skill), player->GetSkillStep(uint16(skill)), uint16(before), player->GetPureMaxSkillValue(skill));
        out = std::string("learned ") + name;
        return player->HasSkill(skill);
    }

    std::string DescribeProfessions(Player* player)
    {
        std::string s;
        int n = 0;
        for (auto const& p : Professions)
            if (player->HasSkill(p.second))
            {
                ++n;
                s += std::string(" ") + p.first + "=" + std::to_string(player->GetPureSkillValue(p.second)) + "/" +
                    std::to_string(player->GetPureMaxSkillValue(p.second));
            }
        return "professions count=" + std::to_string(n) + s;
    }
}
