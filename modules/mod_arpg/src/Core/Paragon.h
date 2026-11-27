// Pure: Paragon levels, points and bonuses (SPEC 4.5, BALANCE §6.1, change m4-paragon). No game headers - unit-tested.
#ifndef ARPG_CORE_PARAGON_H
#define ARPG_CORE_PARAGON_H

#include <cstdint>
#include <string>
#include <vector>

namespace Arpg::Core
{
    struct ParagonCurve
    {
        double base = 600000.0;   // Arpg.Paragon.XpBase: XP from Paragon 0 to 1
        double growth = 1.04;     // Arpg.Paragon.XpGrowth: each level needs this much more
    };

    // XP needed to go from `level` to level + 1 (at least 1).
    uint64_t ParagonXpToNext(ParagonCurve const& c, int level);

    // Adds XP; carries over into as many levels as it pays for. Returns the number of levels gained.
    int AddParagonXp(ParagonCurve const& c, int& level, uint64_t& xpIntoLevel, uint64_t gained);

    enum class ParagonTab : uint8_t { Core = 0, Offense = 1, Defense = 2, Utility = 3 };

    // Diablo 3 rotation: level 1 -> Core, 2 -> Offense, 3 -> Defense, 4 -> Utility, 5 -> Core ...
    ParagonTab TabForLevel(int paragonLevel);
    // Points a tab has earned at this Paragon level (rotation).
    int PointsInTab(int paragonLevel, ParagonTab tab);

    // Bonus per point in hundredths of a percent, with a cap (0 = none). Main stat / stamina 50 (0.5%), ratings 40,
    // armour / avoidance 40, move speed cap 100 (max 1000 = +10%), magic find 500 (max 10000 = +100%), gold find 500.
    struct ParagonBonusRule { int perPoint; int cap; };
    int ParagonBonus(ParagonBonusRule const& rule, int points);

    // World tier (Vale, uncapped zones): monster health/damage and Paragon XP multiplier,
    // 1 + pctPerLevel × level / 100, at most 1 + capPct / 100.
    float WorldTierMultiplier(int paragonLevel, float pctPerLevel, float capPct);

    // Several real players near one monster (m8, owner 2026-11-27): whose Paragon sets the world tier.
    // Arpg.Paragon.WorldTierMode: "highest" (default), "average" (rounded down), "lowest".
    enum class WorldTierMode { Highest, Average, Lowest };
    WorldTierMode WorldTierModeFromName(std::string const& name);   // unknown -> Highest
    char const* WorldTierModeName(WorldTierMode mode);
    int GroupParagonLevel(std::vector<int> const& levels, WorldTierMode mode);   // empty -> 0
}

#endif
