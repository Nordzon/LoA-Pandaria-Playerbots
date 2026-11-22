// The crafting wheel (SPEC 4.8, change m1b tasks 2b.1-2b.2): a crafted item (Spell source from a tradeskill recipe) rolls
// with the recipe colour's rules (Core/Craft) and may become a Unique of its slot (per-profession pity,
// character_arpg_craft_pity).
#ifndef ARPG_LOOT_CRAFTING_H
#define ARPG_LOOT_CRAFTING_H

#include "Core/Craft.h"
#include "Define.h"

#include <string>

class Player;

namespace Arpg::Loot
{
    struct CraftInfo
    {
        bool isCraft = false;          // the spell is a tradeskill recipe (SkillLineAbility)
        uint32 skillLine = 0;
        int skill = 0;                 // the crafter's skill (or the test override)
        int yellowAt = 0, greyAt = 0;
        bool atMaxSkill = false;       // skill >= Arpg.Craft.MaxSkill
        bool catalyst = false;
        Core::RecipeColour colour = Core::RecipeColour::Yellow;
        Core::CraftMods mods;
    };

    // Recipe data + the crafter's skill. Consumes a pending test override for this player (one craft).
    CraftInfo CraftFor(Player* player, uint32 spellId);

    // Crafted-Unique ladder for (player, profession): rolls, stores the misses, returns true on a Unique.
    bool RollCraftUnique(Player* player, CraftInfo const& craft);
    int CraftUniqueMisses(Player* player, uint32 skillLine);
    void SetCraftUniqueMisses(Player* player, uint32 skillLine, int misses);

    // ".arpg test craft": the next craft by this player uses this skill (-1 = real) and catalyst flag.
    void SetCraftOverride(Player* player, int skill, bool catalyst);

    // "colour=yellow skill=… tier_bonus=… extra=… cap=… catalyst=… unique_misses=…"
    std::string DescribeCraft(Player* player, CraftInfo const& craft);
}

#endif
