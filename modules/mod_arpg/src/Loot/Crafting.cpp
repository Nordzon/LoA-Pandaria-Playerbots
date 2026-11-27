#include "Loot/Crafting.h"
#include "Loot/PersonalLoot.h"

#include "ArpgConfig.h"
#include "Core/Salvage.h"
#include "Loot/AffixRoller.h"
#include "Monsters/Champions.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Creature.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"

#include <mutex>
#include <unordered_map>

namespace
{
    struct Override { int skill; bool catalyst; };
    std::mutex g_lock;
    std::unordered_map<uint32, Override> g_override;   // player guid -> next craft
}

namespace Arpg::Loot
{
    void SetCraftOverride(Player* player, int skill, bool catalyst)
    {
        std::lock_guard<std::mutex> l(g_lock);
        g_override[player->GetGUID().GetCounter()] = { skill, catalyst };
    }

    bool IsCraftSpell(uint32 spellId)
    {
        SkillLineAbilityMapBounds b = sSpellMgr->GetSkillLineAbilityMapBounds(spellId);
        for (auto it = b.first; it != b.second; ++it)
            if (SkillLineEntry const* line = sSkillLineStore.LookupEntry(it->second->SkillLine))
                if (line->CategoryID == SKILL_CATEGORY_PROFESSION || line->CategoryID == SKILL_CATEGORY_SECONDARY)
                    return true;
        return false;
    }

    CraftInfo CraftFor(Player* player, uint32 spellId)
    {
        CraftInfo c;
        SkillLineAbilityMapBounds b = sSpellMgr->GetSkillLineAbilityMapBounds(spellId);
        for (auto it = b.first; it != b.second; ++it)
        {
            SkillLineAbilityEntry const* e = it->second;
            SkillLineEntry const* line = sSkillLineStore.LookupEntry(e->SkillLine);
            if (!line || (line->CategoryID != SKILL_CATEGORY_PROFESSION && line->CategoryID != SKILL_CATEGORY_SECONDARY))
                continue;
            c.isCraft = true;
            c.skillLine = e->SkillLine;
            c.yellowAt = int(e->min_value);
            c.greyAt = int(e->max_value);
            break;
        }
        if (!c.isCraft)
            return c;
        c.skill = int(player->GetSkillValue(c.skillLine));
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_override.find(player->GetGUID().GetCounter());
            if (it != g_override.end())
            {
                if (it->second.skill >= 0)
                    c.skill = it->second.skill;
                c.catalyst = it->second.catalyst;
                g_override.erase(it);
            }
        }
        c.atMaxSkill = c.skill >= sArpgConfig.Get().craftMaxSkill;
        c.colour = Core::RecipeColourFor(c.skill, c.yellowAt, c.greyAt);
        c.mods = Core::CraftModsFor(c.colour, c.atMaxSkill, c.catalyst);
        return c;
    }

    uint32 CatalystItemForBand(int band)
    {
        switch (band)
        {
            case 1: return 700001;   // Lesser Catalyst
            case 2: return 700002;   // Greater Catalyst
            case 3: return 700003;   // Grand Catalyst
            default: return 0;
        }
    }

    uint32 UseCatalyst(Player* player, CraftInfo& craft, int itemTier)
    {
        if (!craft.isCraft || craft.catalyst)
            return 0;
        for (int band = std::max(1, Core::CatalystBandForTier(itemTier)); band <= 3; ++band)
        {
            uint32 item = CatalystItemForBand(band);
            if (!player->HasItemCount(item, 1))
                continue;
            player->DestroyItemCount(item, 1, true);
            craft.catalyst = true;
            craft.mods = Core::CraftModsFor(craft.colour, craft.atMaxSkill, true);
            Arpg::TestEvents::Record(player->GetGUID().GetCounter(), "catalyst used item=" + std::to_string(item) +
                " tier=" + std::to_string(itemTier));
            return item;
        }
        return 0;
    }

    int CraftUniqueMisses(Player* player, uint32 skillLine)
    {
        QueryResult r = CharacterDatabase.PQuery("SELECT misses FROM character_arpg_craft_pity WHERE guid = %u AND skill_line = %u",
            player->GetGUID().GetCounter(), skillLine);
        return r ? int(r->Fetch()[0].GetUInt16()) : 0;
    }

    void SetCraftUniqueMisses(Player* player, uint32 skillLine, int misses)
    {
        misses = std::max(0, std::min(misses, 65535));
        CharacterDatabase.DirectPExecute("INSERT INTO character_arpg_craft_pity (guid, skill_line, misses) VALUES (%u, %u, %d) "
            "ON DUPLICATE KEY UPDATE misses = %d", player->GetGUID().GetCounter(), skillLine, misses, misses);
    }

    bool RollCraftUnique(Player* player, CraftInfo const& craft)
    {
        if (!craft.isCraft || !craft.mods.uniqueEligible)
            return false;
        Arpg::Settings const& s = sArpgConfig.Get();
        int misses = CraftUniqueMisses(player, craft.skillLine);
        bool won = Core::RollWithPity(Core::CraftUniqueLadder(s.craftUniqueChancePct, s.craftUniquePityStepPct, craft.catalyst),
            misses, rand_norm());
        SetCraftUniqueMisses(player, craft.skillLine, misses);
        Arpg::TestEvents::Record(player->GetGUID().GetCounter(), std::string("craft unique ") + (won ? "won" : "missed") +
            " skill_line=" + std::to_string(craft.skillLine) + " misses=" + std::to_string(misses));
        return won;
    }

    std::string DescribeCraft(Player* player, CraftInfo const& c)
    {
        if (!c.isCraft)
            return "craft=no";
        return std::string("craft colour=") + Core::RecipeColourName(c.colour) + " skill=" + std::to_string(c.skill) +
            " yellow=" + std::to_string(c.yellowAt) + " grey=" + std::to_string(c.greyAt) + " max_skill=" + (c.atMaxSkill ? "1" : "0") +
            " catalyst=" + (c.catalyst ? "1" : "0") + " tier_bonus=" + std::to_string(c.mods.tierBonus) + " extra=" +
            std::to_string(c.mods.extraAffixes) + " cap=" + std::to_string(c.mods.tierCap) + " unique_eligible=" +
            (c.mods.uniqueEligible ? "1" : "0") + " skill_line=" + std::to_string(c.skillLine) + " unique_misses=" +
            std::to_string(CraftUniqueMisses(player, c.skillLine));
    }
}

namespace
{
    class ArpgCatalystDropScript : public PlayerScript
    {
    public:
        ArpgCatalystDropScript() : PlayerScript("arpg_catalyst_drops") { }

        void OnCreatureKill(Player* killer, Creature* killed) override
        {
            if (!killer || !killed)
                return;
            Arpg::Settings const& s = sArpgConfig.Get();
            if (!s.lootEnabled || (!Arpg::Scaling::IsRealPlayer(killer) && !s.affixBotItems))
                return;
            uint32 entry = killed->GetEntry();
            float chance = 0.0f;
            if (s.superuniqueEntryOffset && entry >= s.superuniqueEntryOffset && entry < s.superuniqueEntryOffset + 300000)
                chance = s.catalystSuperuniqueChancePct;
            else if (Arpg::Monsters::WasChampionKill(killed))
                chance = s.catalystChampionChancePct;
            if (chance <= 0.0f)
                return;
            int tier = Arpg::Loot::MaxAffixTierAtLevel(int(killed->GetLevel()));
            // m8: friends nearby roll their own catalyst (personal loot).
            for (Player* m : Arpg::Loot::PersonalRecipients(killer, killed))
                if (float(rand_chance()) < chance)
                    Arpg::Loot::AddPersonalItem(m, killed, Arpg::Loot::CatalystItemForBand(Arpg::Core::CatalystBandForTier(tier)));
            if (float(rand_chance()) >= chance)
                return;
            uint32 item = Arpg::Loot::CatalystItemForBand(Arpg::Core::CatalystBandForTier(tier));
            if (!item)
                return;
            Arpg::Monsters::AddLootItem(killed->loot, item);
            killed->SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            Arpg::TestEvents::Record(killer->GetGUID().GetCounter(), "catalyst drop item=" + std::to_string(item) +
                " tier=" + std::to_string(tier));
        }
    };
}

namespace Arpg::Loot
{
    void InstallCatalystDrops()
    {
        new ArpgCatalystDropScript();
    }
}
