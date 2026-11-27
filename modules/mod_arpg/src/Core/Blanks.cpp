#include "Blanks.h"
#include "AffixRoll.h"

#include <algorithm>

namespace Arpg::Core
{
    int PickBlankBracket(std::vector<int> const& brackets, int level)
    {
        int best = 0, lowest = 0;
        for (int b : brackets)
        {
            if (!lowest || b < lowest)
                lowest = b;
            if (b <= level && b > best)
                best = b;
        }
        return best ? best : lowest;
    }

    uint32_t PickBlankMainStat(std::vector<uint32_t> const& available, uint32_t wanted)
    {
        if (available.empty())
            return 0;
        return std::find(available.begin(), available.end(), wanted) != available.end() ? wanted : available.front();
    }

    int ArmorSubclassForClass(int classId, int level)
    {
        switch (classId)
        {
            case 1: case 2: return level >= 40 ? 4 : 3;
            case 6: return 4;
            case 3: case 7: return level >= 40 ? 3 : 2;
            case 4: case 10: case 11: return 2;
            default: return 1;
        }
    }

    int BlankAffixCount(int quality, int cap)
    {
        int lo = 0, hi = 0;
        CountRange(quality, lo, hi);
        return cap > 0 ? std::min(hi, cap) : hi;
    }
}
