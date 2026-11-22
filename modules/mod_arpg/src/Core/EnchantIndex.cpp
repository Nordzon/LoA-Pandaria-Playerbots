#include "EnchantIndex.h"

namespace Arpg::Core
{
    void EnchantIndex::Add(uint32_t stat, uint32_t amount, uint32_t enchantId)
    {
        if (amount == 0 || enchantId == 0)
            return;
        auto& m = _byStat[stat];
        auto it = m.find(amount);
        if (it == m.end() || enchantId < it->second)
            m[amount] = enchantId;
    }

    EnchantMatch EnchantIndex::Nearest(uint32_t stat, uint32_t value) const
    {
        EnchantMatch r;
        auto s = _byStat.find(stat);
        if (s == _byStat.end() || s->second.empty())
            return r;
        auto const& m = s->second;
        auto hi = m.lower_bound(value);              // first amount >= value
        if (hi == m.end())
        {
            auto last = std::prev(m.end());
            return { last->second, last->first };
        }
        if (hi->first == value || hi == m.begin())
            return { hi->second, hi->first };
        auto lo = std::prev(hi);
        // ties go to the lower amount (never round a player up past the roll on equal distance)
        if (value - lo->first <= hi->first - value)
            return { lo->second, lo->first };
        return { hi->second, hi->first };
    }

    size_t EnchantIndex::Count(uint32_t stat) const
    {
        auto s = _byStat.find(stat);
        return s == _byStat.end() ? 0 : s->second.size();
    }

    uint32_t EnchantIndex::MinAmount(uint32_t stat) const
    {
        auto s = _byStat.find(stat);
        return (s == _byStat.end() || s->second.empty()) ? 0 : s->second.begin()->first;
    }

    uint32_t EnchantIndex::MaxAmount(uint32_t stat) const
    {
        auto s = _byStat.find(stat);
        return (s == _byStat.end() || s->second.empty()) ? 0 : s->second.rbegin()->first;
    }
}
