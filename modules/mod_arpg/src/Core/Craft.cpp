#include "Craft.h"

#include <algorithm>

namespace Arpg::Core
{
    RecipeColour RecipeColourFor(int skill, int yellowAt, int greyAt)
    {
        if (yellowAt <= 0 && greyAt <= 0)
            return RecipeColour::Yellow;
        if (greyAt < yellowAt)
            greyAt = yellowAt;
        if (skill >= greyAt)
            return RecipeColour::Grey;
        if (skill >= (yellowAt + greyAt) / 2)
            return RecipeColour::Green;
        if (skill >= yellowAt)
            return RecipeColour::Yellow;
        return RecipeColour::Orange;
    }

    char const* RecipeColourName(RecipeColour c)
    {
        switch (c)
        {
            case RecipeColour::Orange: return "orange";
            case RecipeColour::Yellow: return "yellow";
            case RecipeColour::Green:  return "green";
            default:                   return "grey";
        }
    }

    CraftMods CraftModsFor(RecipeColour colour, bool atMaxSkill, bool catalyst)
    {
        CraftMods m;
        bool const hard = colour == RecipeColour::Orange || colour == RecipeColour::Yellow;
        if (hard)
        {
            m.tierBonus = 1;
            m.uniqueEligible = true;
        }
        if (colour == RecipeColour::Grey)
            m.tierCap = 1;
        else if (atMaxSkill)
            m.extraAffixes = 1;
        if (catalyst)
            m.tierBonus += 1;
        return m;
    }

    PityLadder CraftUniqueLadder(float basePct, float stepPct, bool catalyst)
    {
        float const mult = catalyst ? 2.0f : 1.0f;
        PityLadder l;
        l.base = std::max(0.0f, basePct) / 100.0f * mult;
        l.step = std::max(0.0f, stepPct) / 100.0f * mult;
        return l;
    }
}
