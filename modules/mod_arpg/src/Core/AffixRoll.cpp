#include "AffixRoll.h"

#include <algorithm>
#include <set>

namespace Arpg::Core
{
    void CountRange(int quality, int& minCount, int& maxCount)
    {
        switch (quality)
        {
            case QUALITY_UNCOMMON: minCount = 1; maxCount = 2; break;
            case QUALITY_RARE:     minCount = 2; maxCount = 3; break;
            case QUALITY_EPIC:     minCount = 3; maxCount = 5; break;
            default:               minCount = 0; maxCount = 0; break;
        }
    }

    int MaxTierForLevel(std::vector<TierRule> const& tiers, int level)
    {
        int best = 0;
        for (TierRule const& t : tiers)
            if (level >= t.unlockLevel)
                best = std::max(best, t.tier);
        return best;
    }

    namespace
    {
        bool Fits(AffixDef const& a, RollRequest const& req, int prefixes, int suffixes,
                  std::set<uint32_t> const& usedIds, std::set<uint32_t> const& usedGroups)
        {
            if (a.weight == 0 || (a.slotMask & req.slotBit) == 0 || usedIds.count(a.id))
                return false;
            if (a.exclusiveGroup && usedGroups.count(a.exclusiveGroup))
                return false;
            if (a.kind == AffixKind::Prefix && prefixes >= 3)
                return false;
            if (a.kind == AffixKind::Suffix && suffixes >= 3)
                return false;
            return true;
        }

        // Tier 1..maxTier, weighted toward higher tiers (weight = tier).
        int RollTier(int maxTier, RandFn const& rand)
        {
            if (maxTier <= 1)
                return 1;
            uint32_t total = uint32_t(maxTier * (maxTier + 1) / 2);
            uint32_t r = rand(total);
            for (int t = 1; t <= maxTier; ++t)
            {
                if (r < uint32_t(t))
                    return t;
                r -= uint32_t(t);
            }
            return maxTier;
        }
    }

    std::vector<RolledAffix> RollAffixes(std::vector<AffixDef> const& pool, std::vector<TierRule> const& tiers,
                                         RollRequest const& req, RandFn const& rand)
    {
        std::vector<RolledAffix> out;
        int minCount = 0, maxCount = 0;
        CountRange(req.quality, minCount, maxCount);
        int count = req.forceCount > 0 ? std::min(req.forceCount, 6) : minCount + int(rand(uint32_t(maxCount - minCount + 1)));
        if (req.forceCount <= 0 && req.extraCount > 0)
            count = std::min(count + req.extraCount, maxCount);
        if (req.maxCount > 0)
            count = std::min(count, req.maxCount);
        if (count <= 0)
            return out;

        int levelMax = MaxTierForLevel(tiers, req.playerLevel);
        int maxTier = std::min(std::max(1, levelMax + req.tierBonus), req.absoluteMaxTier);
        if (req.tierCap > 0)
            maxTier = std::min(maxTier, req.tierCap);

        std::set<uint32_t> usedIds, usedGroups;
        int prefixes = 0, suffixes = 0;
        auto take = [&](AffixDef const& a)
        {
            out.push_back({ a.id, req.tierAtMax ? maxTier : RollTier(maxTier, rand) });
            usedIds.insert(a.id);
            if (a.exclusiveGroup)
                usedGroups.insert(a.exclusiveGroup);
            (a.kind == AffixKind::Prefix ? prefixes : suffixes)++;
        };

        // Smart loot: first affix is the player's main stat, smartLootPct % of the time (if the slot allows it).
        if (req.playerMainStat && req.smartLootPct > 0 && int(rand(100)) < req.smartLootPct)
            for (AffixDef const& a : pool)
                if (a.mainStat == req.playerMainStat && Fits(a, req, prefixes, suffixes, usedIds, usedGroups))
                {
                    take(a);
                    break;
                }

        while (int(out.size()) < count)
        {
            uint32_t total = 0;
            for (AffixDef const& a : pool)
                if (Fits(a, req, prefixes, suffixes, usedIds, usedGroups))
                    total += a.weight;
            if (total == 0)
                break; // pool exhausted for this slot
            uint32_t r = rand(total);
            for (AffixDef const& a : pool)
            {
                if (!Fits(a, req, prefixes, suffixes, usedIds, usedGroups))
                    continue;
                if (r < a.weight)
                {
                    take(a);
                    break;
                }
                r -= a.weight;
            }
        }
        return out;
    }
}
