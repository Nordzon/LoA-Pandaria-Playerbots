#include "AffixEffects.h"

#include <algorithm>
#include <map>

namespace Arpg::Core
{
    namespace
    {
        struct Named { Effect e; char const* name; };
        // Names = arpg_affix.effect (modules/mod_arpg/sql/tools/gen_affix_sql.py POOL)
        Named const kNames[] = {
            { Effect::LeechPct, "leech_pct" }, { Effect::HealOnHitPct, "heal_on_hit_pct" },
            { Effect::DmgVsElitePct, "dmg_vs_elite_pct" }, { Effect::WeaponDmgPct, "weapon_dmg_pct" },
            { Effect::SpellPowerPct, "spell_power_pct" }, { Effect::CrushingBlowPct, "crushing_blow_pct" },
            { Effect::PowerOnHitPct, "power_on_hit_pct" }, { Effect::PowerLeechPct, "power_leech_pct" },
            { Effect::ProcFireNova, "proc_fire_nova" }, { Effect::ProcChainLightning, "proc_chain_light" },
            { Effect::ProcFrostNova, "proc_frost_nova" }, { Effect::ProcPoison, "proc_poison" }, { Effect::ProcBleed, "proc_bleed" },
            { Effect::DrVsElitePct, "dr_vs_elite_pct" }, { Effect::ThornsPct, "thorns_pct" }, { Effect::ArmorPct, "armor_pct" },
            { Effect::RegenPct, "regen_pct" }, { Effect::PhoenixHealPct, "phoenix_heal_pct" },
            { Effect::CombatAbsorbPct, "combat_absorb_pct" }, { Effect::ProcStruckChain, "proc_struck_chain" },
            { Effect::CcReductionPct, "cc_reduction_pct" }, { Effect::SnareReductionPct, "snare_reduction_pct" },
            { Effect::MoveSpeedPct, "move_speed_pct" }, { Effect::CastSpeedPct, "cast_speed_pct" },
            { Effect::CooldownRatePct, "cooldown_rate_pct" }, { Effect::OverhealToDmgPct, "overheal_to_dmg_pct" },
            { Effect::HealToDmgPct, "heal_to_dmg_pct" }, { Effect::AtonementPct, "atonement_pct" },
            { Effect::MartyrDmgPct, "martyr_dmg_pct" }, { Effect::GoldFindPct, "gold_find_pct" },
            { Effect::MagicFindPct, "magic_find_pct" }, { Effect::XpPct, "xp_pct" }, { Effect::SkillupPct, "skillup_pct" },
            { Effect::PetRangePct, "pet_range_pct" }, { Effect::PrismaticSocket, "prismatic_socket" },
        };
    }

    Effect EffectFromName(std::string const& name)
    {
        for (Named const& n : kNames)
            if (name == n.name)
                return n.e;
        return Effect::None;
    }

    char const* EffectName(Effect e)
    {
        for (Named const& n : kNames)
            if (n.e == e)
                return n.name;
        return "none";
    }

    int32_t EffectTotals::Get(Effect e) const
    {
        int32_t v = std::max(0, Raw(e));
        switch (e)
        {
            case Effect::MoveSpeedPct: case Effect::CastSpeedPct: case Effect::CooldownRatePct: return std::min(v, 3000);
            case Effect::CcReductionPct: case Effect::SnareReductionPct: return std::min(v, 7500);
            case Effect::LeechPct: return std::min(v, 2000);
            case Effect::DrVsElitePct: return std::min(v, 5000);
            default: return v;
        }
    }

    bool EffectTotals::Any() const
    {
        for (int32_t v : _v)
            if (v)
                return true;
        return false;
    }

    uint32_t PctOf(uint32_t amount, int32_t hundredths)
    {
        if (hundredths <= 0)
            return 0;
        return uint32_t(uint64_t(amount) * uint64_t(hundredths) / 10000u);
    }

    bool ProcRoll(int32_t chanceHundredths, uint32_t roll0to9999, bool force)
    {
        if (chanceHundredths <= 0)
            return false;
        return force || roll0to9999 < uint32_t(chanceHundredths);
    }

    uint32_t CrushingBlowDamage(uint32_t targetCurrentHealth, bool eliteOrBoss, uint32_t triggeringHit)
    {
        uint32_t d = eliteOrBoss ? targetCurrentHealth / 40u : targetCurrentHealth / 10u;
        uint64_t cap = uint64_t(triggeringHit) * 3u;
        return uint32_t(std::min<uint64_t>(d, cap));
    }

    PhoenixResult Phoenix(uint32_t damage, uint32_t currentHealth, uint32_t maxHealth, int32_t valueHundredths, bool ready)
    {
        if (!ready || valueHundredths <= 0 || damage < currentHealth || currentHealth == 0)
            return { false, damage, 0 };
        return { true, currentHealth - 1, PctOf(maxHealth, valueHundredths) };
    }

    uint32_t AbsorbFromPool(uint32_t damage, uint32_t& pool)
    {
        uint32_t taken = std::min(damage, pool);
        pool -= taken;
        return damage - taken;
    }
}
