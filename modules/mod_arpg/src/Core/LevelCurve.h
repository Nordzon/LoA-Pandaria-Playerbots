// Pure: piecewise-linear "level -> value" curves from config text, e.g. Arpg.Loot.EpicIlvlCurve =
// "1:6,59:64,60:66,70:115,80:213,85:359,90:463" (player level -> item level of an epic at that level; B uses it).
#ifndef ARPG_CORE_LEVELCURVE_H
#define ARPG_CORE_LEVELCURVE_H

#include <string>
#include <utility>
#include <vector>

namespace Arpg::Core
{
    class LevelCurve
    {
    public:
        // "x:y,x:y,..." with increasing x. Returns false (curve unchanged) on malformed text.
        bool Parse(std::string const& text);
        // Linear between points, clamped to the first/last value outside them. 0 if empty.
        int ValueAt(int x) const;
        bool Empty() const { return _points.empty(); }

    private:
        std::vector<std::pair<int, int>> _points;
    };
}

#endif
