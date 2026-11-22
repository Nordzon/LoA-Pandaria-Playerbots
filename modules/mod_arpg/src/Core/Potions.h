// Pure: ARPG potion rules (owner 2026-11-22, change m3b-potions). No game headers - unit-tested.
#ifndef ARPG_CORE_POTIONS_H
#define ARPG_CORE_POTIONS_H

#include <cstdint>
#include <vector>

namespace Arpg::Core
{
    enum class PotionKind : uint8_t { Healing = 0, Mana = 1, Rejuvenation = 2 };

    // The drinker's resource types (as the core numbers them: mana 0, rage 1, focus 2, energy 3, runic power 6).
    enum class Resource : uint8_t { Mana = 0, Rage = 1, Focus = 2, Energy = 3, RunicPower = 6, Other = 255 };

    // Health healed: pct of max health, at least 1.
    uint32_t PotionHeal(uint32_t maxHealth, int pct);

    // Resource restored by a mana / rejuvenation potion: mana pct; energy and focus at half that pct; rage and runic power
    // pct (generated). At least 1 when the resource has a maximum.
    uint32_t PotionResource(Resource r, uint32_t maxValue, int pct);

    struct PotionDef { uint32_t itemId; PotionKind kind; int tier; int pct; };

    // The potion to drop for a kind at a drop tier: the highest tier <= dropTier of that kind (0 if none).
    uint32_t PotionForTier(std::vector<PotionDef> const& potions, PotionKind kind, int dropTier);

    // Weighted kind pick; roll in [0, wHeal + wMana + wRejuv).
    PotionKind PickPotionKind(uint32_t wHeal, uint32_t wMana, uint32_t wRejuv, uint32_t roll);
}

#endif
