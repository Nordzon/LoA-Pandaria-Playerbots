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
        int maxLevel = 90;        // server level cap (CONFIG_MAX_PLAYER_LEVEL)
        float healthMod = 1.0f;   // Arpg.Scale.HealthMod
        float damageMod = 1.0f;   // Arpg.Scale.DamageMod
    };

    // Level the creature should have near a player: max(own level, player level + offset), clamped to
    // [1, maxLevel]. Never lowers a creature (upward-only). Returns 0 when nothing changes (disabled,
    // or the creature is already at/above the target).
    uint8_t ScaledLevel(ScaleSettings const& s, int creatureLevel, int playerLevel);

    // Multipliers applied on top of the core's stats for the scaled level. 1.0 when not scaled.
    float HealthMultiplier(ScaleSettings const& s, bool scaled);
    float DamageMultiplier(ScaleSettings const& s, bool scaled);
}

#endif
