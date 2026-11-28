// Pure: open-world level scaling formula (SPEC 4.2, change m1-world-scaling). No game headers.
#ifndef ARPG_CORE_SCALING_H
#define ARPG_CORE_SCALING_H

#include <cstdint>

namespace Arpg::Core
{
    struct ScaleSettings
    {
        bool enabled = true;
        int levelOffset = 0;      // Arpg.Scale.LevelOffset
        int levelBelowMin = 0;    // Arpg.Scale.LevelBelowMin  (owner 2026-11-28: trash 2-4 levels below the player;
        int levelBelowMax = 0;    // Arpg.Scale.LevelBelowMax   each creature picks one value in [min, max] by its seed)
        int maxLevel = 90;        // server level cap (CONFIG_MAX_PLAYER_LEVEL)
        float healthMod = 1.0f;   // Arpg.Scale.HealthMod
        float damageMod = 1.0f;   // Arpg.Scale.DamageMod
    };

    // Level the creature should have near a player: max(own level, player level + offset - below), clamped to
    // [1, maxLevel]; below = levelBelowMin + seed % (levelBelowMax - levelBelowMin + 1) (a creature keeps its value: the
    // seed is its guid). Never lowers a creature (upward-only). Returns 0 when nothing changes (disabled,
    // or the creature is already at/above the target).
    uint8_t ScaledLevel(ScaleSettings const& s, int creatureLevel, int playerLevel, uint32_t seed = 0);
    // The "below" part alone (0 when the range is empty).
    int LevelBelow(ScaleSettings const& s, uint32_t seed);

    // Multipliers applied on top of the core's stats for the scaled level. 1.0 when not scaled.
    float HealthMultiplier(ScaleSettings const& s, bool scaled);
    float DamageMultiplier(ScaleSettings const& s, bool scaled);
}

#endif
