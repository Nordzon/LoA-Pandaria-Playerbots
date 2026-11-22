// Pure: Champion / Superunique power by the player's level band (SPEC 4.3, BALANCE §14, change m3).
// No game headers - unit-tested in arpg/unittests. Numbers come from arpg_monster_band / arpg_monster_affix.
#ifndef ARPG_CORE_MONSTERBANDS_H
#define ARPG_CORE_MONSTERBANDS_H

#include <cstdint>
#include <functional>
#include <vector>

namespace Arpg::Core
{
    enum class MonsterKind : uint8_t { Champion = 0, Superunique = 1 };

    // One row of arpg_monster_band: applies from minPlayerLevel up to the next row's minPlayerLevel - 1.
    struct MonsterBand
    {
        MonsterKind kind = MonsterKind::Champion;
        int minPlayerLevel = 1;
        int levelOffset = 0;        // added to the player's level (capped at the max level)
        float healthMult = 1.0f;
        float damageMult = 1.0f;
        int affixMin = 0;           // monster affixes (Champions) or abilities (Superuniques)
        int affixMax = 0;
        int minionsMin = 0;
        int minionsMax = 0;
        int orbPct = 0;             // health orb on kill: % of max health and power restored
        int affixDamagePct = 0;     // affix damage (e.g. Fire Enchanted death explosion) as % of the player's max health
    };

    // One row of arpg_monster_affix (Extra Strong, Fire Enchanted, ...).
    struct MonsterAffixDef
    {
        uint32_t id = 0;
        int unlockLevel = 1;        // player level from which it may appear (soft affixes 1, Fire/Cold/Lightning 15, Arcane 25)
        uint32_t weight = 0;
        uint32_t exclusiveGroup = 0;// 0 = none; the three Enchanted affixes share a group
    };

    using MonsterRandFn = std::function<uint32_t(uint32_t n)>;   // uniform in [0, n)

    // The band for this kind at this player level; nullptr if no row applies (table empty or level below the first row).
    MonsterBand const* BandFor(std::vector<MonsterBand> const& bands, MonsterKind kind, int playerLevel);

    // Level of the spawned monster: player level + offset, within [1, maxLevel]. Never below the player's level - so the
    // band offset is the only thing that makes it harder.
    int MonsterLevel(MonsterBand const& band, int playerLevel, int maxLevel);

    // Picks the monster affixes: count in [affixMin, affixMax], only affixes unlocked at this level, no duplicates and no
    // two from one exclusive group. Returns fewer if the pool runs out.
    std::vector<uint32_t> PickMonsterAffixes(std::vector<MonsterAffixDef> const& pool, MonsterBand const& band, int playerLevel,
                                             MonsterRandFn const& rand);

    // Number of minions in [minionsMin, minionsMax].
    int MinionCount(MonsterBand const& band, MonsterRandFn const& rand);

    // Damage an affix deals to the player: % of max health, at least 1. Never a one-shot: capped below max health.
    uint32_t AffixDamage(MonsterBand const& band, uint32_t playerMaxHealth);

    // Health orb on kill: amount restored (of `maxValue`, i.e. max health or max power).
    uint32_t OrbAmount(MonsterBand const& band, uint32_t maxValue);
}

#endif
