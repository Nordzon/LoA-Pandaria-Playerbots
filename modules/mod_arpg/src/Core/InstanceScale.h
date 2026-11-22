// Pure: dungeon / raid scaling by party size, raid tier and solo role (SPEC 4.6, BALANCE §5/§6/§10, change m5).
// No game headers - unit-tested.
#ifndef ARPG_CORE_INSTANCESCALE_H
#define ARPG_CORE_INSTANCESCALE_H

#include <cstdint>
#include <vector>

namespace Arpg::Core
{
    // One row of arpg_party_scale: creature health / damage (fractions of the full-group values) for this many players.
    struct PartyScaleRow { int players; float health; float damage; };

    // Health and damage for `players` (incl. bots): exact row, linear between rows, clamped to the first / last row.
    // Rows need not be sorted. Empty table -> 1.0 / 1.0.
    void PartyScale(std::vector<PartyScaleRow> rows, int players, float& health, float& damage);

    // Raid tier ladder: 1 + perTier × tier (tier 0 = no extra).
    float RaidTierMultiplier(int tier, float perTier);

    enum class SpecRole : uint8_t { Damage = 0, Tank = 1, Healer = 2 };

    // MoP specialization id -> role (tanks: Protection x2, Blood, Guardian, Brewmaster; healers: Holy x2, Discipline,
    // Restoration x2, Mistweaver; everything else damage).
    SpecRole RoleForSpec(uint32_t specId);

    struct SoloKnobs
    {
        float nonTankCreatureDamage = 0.75f;   // Arpg.Dungeon.SoloNonTankDamage
        float tankDamageDealt = 1.25f;         // Arpg.Dungeon.SoloTankDamageDealt
        float healerDamageDealt = 1.6f;        // Arpg.Dungeon.SoloHealerDamageDealt
    };

    // Alone in a 5-man: creatures hit a non-tank softer; tanks and healers deal more damage. 1.0 when not solo.
    float SoloCreatureDamage(SpecRole role, bool solo, SoloKnobs const& k);
    float SoloPlayerDamageDealt(SpecRole role, bool solo, SoloKnobs const& k);
}

#endif
