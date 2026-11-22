// Pure: "stat -> fixed-amount enchantments" index for native affix display (m2 tasks 3.2 / 4.1b, design.md).
// The game side feeds it the client's SpellItemEnchantment rows that add one stat by a fixed amount.
#ifndef ARPG_CORE_ENCHANTINDEX_H
#define ARPG_CORE_ENCHANTINDEX_H

#include <cstdint>
#include <map>
#include <vector>

namespace Arpg::Core
{
    struct EnchantMatch
    {
        uint32_t enchantId = 0;  // 0 = none for that stat
        uint32_t amount = 0;     // the value the client will show and the core will apply
    };

    class EnchantIndex
    {
    public:
        void Clear() { _byStat.clear(); }
        // Lower enchant id wins when two rows give the same amount (stable, deterministic).
        void Add(uint32_t stat, uint32_t amount, uint32_t enchantId);
        // Row whose amount is closest to `value` (ties: the lower amount). Values beyond the largest row use the largest.
        EnchantMatch Nearest(uint32_t stat, uint32_t value) const;
        size_t Count(uint32_t stat) const;
        uint32_t MinAmount(uint32_t stat) const;
        uint32_t MaxAmount(uint32_t stat) const;

    private:
        std::map<uint32_t, std::map<uint32_t, uint32_t>> _byStat;   // stat -> amount -> enchant id
    };
}

#endif
