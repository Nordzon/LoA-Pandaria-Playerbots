#include "MonsterBands.h"

#include <algorithm>
#include <set>

namespace Arpg::Core
{
    MonsterBand const* BandFor(std::vector<MonsterBand> const& bands, MonsterKind kind, int playerLevel)
    {
        MonsterBand const* best = nullptr;
        for (MonsterBand const& b : bands)
            if (b.kind == kind && b.minPlayerLevel <= playerLevel && (!best || b.minPlayerLevel > best->minPlayerLevel))
                best = &b;
        return best;
    }

    int MonsterLevel(MonsterBand const& band, int playerLevel, int maxLevel)
    {
        int level = playerLevel + std::max(0, band.levelOffset);
        return std::clamp(level, 1, std::max(1, maxLevel));
    }

    std::vector<uint32_t> PickMonsterAffixes(std::vector<MonsterAffixDef> const& pool, MonsterBand const& band, int playerLevel,
                                             MonsterRandFn const& rand)
    {
        std::vector<uint32_t> out;
        int lo = std::max(0, band.affixMin);
        int hi = std::max(lo, band.affixMax);
        int count = lo + int(rand(uint32_t(hi - lo + 1)));
        std::vector<MonsterAffixDef> open;
        for (MonsterAffixDef const& a : pool)
            if (a.weight > 0 && a.unlockLevel <= playerLevel)
                open.push_back(a);
        std::set<uint32_t> groups;
        while (int(out.size()) < count && !open.empty())
        {
            uint32_t total = 0;
            for (MonsterAffixDef const& a : open)
                total += a.weight;
            uint32_t r = rand(total);
            size_t pick = 0;
            for (; pick < open.size(); ++pick)
            {
                if (r < open[pick].weight)
                    break;
                r -= open[pick].weight;
            }
            if (pick >= open.size())
                pick = open.size() - 1;
            MonsterAffixDef chosen = open[pick];
            out.push_back(chosen.id);
            if (chosen.exclusiveGroup)
                groups.insert(chosen.exclusiveGroup);
            open.erase(std::remove_if(open.begin(), open.end(), [&](MonsterAffixDef const& a)
                { return a.id == chosen.id || (a.exclusiveGroup && groups.count(a.exclusiveGroup)); }), open.end());
        }
        return out;
    }

    int MinionCount(MonsterBand const& band, MonsterRandFn const& rand)
    {
        int lo = std::max(0, band.minionsMin);
        int hi = std::max(lo, band.minionsMax);
        return lo + int(rand(uint32_t(hi - lo + 1)));
    }

    uint32_t AffixDamage(MonsterBand const& band, uint32_t playerMaxHealth)
    {
        if (!playerMaxHealth)
            return 0;
        uint64_t d = uint64_t(playerMaxHealth) * uint64_t(std::clamp(band.affixDamagePct, 0, 100)) / 100;
        d = std::max<uint64_t>(d, 1);
        return uint32_t(std::min<uint64_t>(d, playerMaxHealth - 1 ? playerMaxHealth - 1 : 1));   // never a one-shot
    }

    uint32_t OrbAmount(MonsterBand const& band, uint32_t maxValue)
    {
        return uint32_t(uint64_t(maxValue) * uint64_t(std::clamp(band.orbPct, 0, 100)) / 100);
    }
}
