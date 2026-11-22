#include "InstanceScale.h"

#include <algorithm>

namespace Arpg::Core
{
    void PartyScale(std::vector<PartyScaleRow> rows, int players, float& health, float& damage)
    {
        health = damage = 1.0f;
        if (rows.empty())
            return;
        std::sort(rows.begin(), rows.end(), [](PartyScaleRow const& a, PartyScaleRow const& b) { return a.players < b.players; });
        if (players <= rows.front().players)
        {
            health = rows.front().health;
            damage = rows.front().damage;
            return;
        }
        if (players >= rows.back().players)
        {
            health = rows.back().health;
            damage = rows.back().damage;
            return;
        }
        for (size_t i = 1; i < rows.size(); ++i)
            if (players <= rows[i].players)
            {
                PartyScaleRow const& a = rows[i - 1];
                PartyScaleRow const& b = rows[i];
                float t = b.players == a.players ? 1.0f : float(players - a.players) / float(b.players - a.players);
                health = a.health + (b.health - a.health) * t;
                damage = a.damage + (b.damage - a.damage) * t;
                return;
            }
    }

    float RaidTierMultiplier(int tier, float perTier)
    {
        return 1.0f + std::max(0.0f, perTier) * float(std::max(0, tier));
    }

    SpecRole RoleForSpec(uint32_t specId)
    {
        switch (specId)
        {
            case 73: case 66: case 250: case 104: case 268:
                return SpecRole::Tank;
            case 65: case 256: case 257: case 264: case 105: case 270:
                return SpecRole::Healer;
            default:
                return SpecRole::Damage;
        }
    }

    float SoloCreatureDamage(SpecRole role, bool solo, SoloKnobs const& k)
    {
        return solo && role != SpecRole::Tank ? k.nonTankCreatureDamage : 1.0f;
    }

    float SoloPlayerDamageDealt(SpecRole role, bool solo, SoloKnobs const& k)
    {
        if (!solo)
            return 1.0f;
        if (role == SpecRole::Tank)
            return k.tankDamageDealt;
        if (role == SpecRole::Healer)
            return k.healerDamageDealt;
        return 1.0f;
    }
}
