#include "LevelCurve.h"

#include <cmath>
#include <cstdlib>
#include <sstream>

namespace Arpg::Core
{
    bool LevelCurve::Parse(std::string const& text)
    {
        std::vector<std::pair<int, int>> pts;
        std::stringstream ss(text);
        std::string item;
        while (std::getline(ss, item, ','))
        {
            size_t colon = item.find(':');
            if (colon == std::string::npos)
                return false;
            char* end1 = nullptr;
            char* end2 = nullptr;
            std::string xs = item.substr(0, colon), ys = item.substr(colon + 1);
            long x = std::strtol(xs.c_str(), &end1, 10);
            long y = std::strtol(ys.c_str(), &end2, 10);
            if (xs.empty() || ys.empty() || (end1 && *end1 != '\0' && *end1 != ' ') || (end2 && *end2 != '\0' && *end2 != ' '))
                return false;
            if (!pts.empty() && x <= pts.back().first)
                return false; // x must increase
            pts.emplace_back(int(x), int(y));
        }
        if (pts.empty())
            return false;
        _points = std::move(pts);
        return true;
    }

    int LevelCurve::ValueAt(int x) const
    {
        if (_points.empty())
            return 0;
        if (x <= _points.front().first)
            return _points.front().second;
        if (x >= _points.back().first)
            return _points.back().second;
        for (size_t i = 1; i < _points.size(); ++i)
            if (x <= _points[i].first)
            {
                auto const& a = _points[i - 1];
                auto const& b = _points[i];
                double t = double(x - a.first) / double(b.first - a.first);
                return int(std::lround(a.second + (b.second - a.second) * t));
            }
        return _points.back().second;
    }
}
