// Pure: the crafting wheel's roll rules (SPEC 4.8, change m1b task 2b.1-2b.2). No game headers - unit-tested.
#ifndef ARPG_CORE_CRAFT_H
#define ARPG_CORE_CRAFT_H

#include "Pity.h"

namespace Arpg::Core
{
    // Recipe colour as the client shows it (same thresholds as Player::UpdateCraftSkill):
    // orange below yellowAt (SkillLineAbility min_value), yellow below (yellowAt + greyAt) / 2, green below greyAt
    // (max_value), grey from greyAt. A recipe without thresholds (0/0) counts as yellow.
    enum class RecipeColour { Orange, Yellow, Green, Grey };

    RecipeColour RecipeColourFor(int skill, int yellowAt, int greyAt);
    char const* RecipeColourName(RecipeColour c);

    struct CraftMods
    {
        int tierBonus = 0;            // added to the level's max tier (orange/yellow +1, catalyst +1)
        int extraAffixes = 0;         // crafter at the profession's max skill: +1 (within the quality's count limit)
        int tierCap = 0;              // >0: no affix above this tier (grey recipes: T1)
        bool uniqueEligible = false;  // only orange/yellow recipes can become a Unique
    };

    // Rules: orange/yellow +1 tier and Unique-eligible; green plain; grey capped at T1 and no extra affix;
    // max skill +1 affix (not grey); a catalyst +1 tier (still capped by grey).
    CraftMods CraftModsFor(RecipeColour colour, bool atMaxSkill, bool catalyst);

    // Crafted-Unique ladder: base + step per craft without one (per character and profession); a catalyst doubles both.
    PityLadder CraftUniqueLadder(float basePct, float stepPct, bool catalyst);
}

#endif
