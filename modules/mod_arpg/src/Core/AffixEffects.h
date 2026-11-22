// Pure: effect affixes (SPEC 4.1, change m2-affix-loot task 4.3). No game headers - unit-tested in arpg/unittests.
// Effect values are in hundredths of a percent (250 = 2.5%), as stored in item_instance_arpg_affix.value.
#ifndef ARPG_CORE_AFFIXEFFECTS_H
#define ARPG_CORE_AFFIXEFFECTS_H

#include <array>
#include <cstdint>
#include <string>

namespace Arpg::Core
{
    enum class Effect : int
    {
        None = 0,
        // offense
        LeechPct, HealOnHitPct, DmgVsElitePct, WeaponDmgPct, SpellPowerPct, CrushingBlowPct, PowerOnHitPct, PowerLeechPct,
        ProcFireNova, ProcChainLightning, ProcFrostNova, ProcPoison, ProcBleed,
        // defense
        DrVsElitePct, ThornsPct, ArmorPct, RegenPct, PhoenixHealPct, CombatAbsorbPct, ProcStruckChain, CcReductionPct,
        SnareReductionPct, MoveSpeedPct, CastSpeedPct, CooldownRatePct,
        // healer (BALANCE §10)
        OverhealToDmgPct, HealToDmgPct, AtonementPct, MartyrDmgPct,
        // utility
        GoldFindPct, MagicFindPct, XpPct, SkillupPct, PetRangePct, PrismaticSocket,
        Count
    };

    // arpg_affix.effect string -> Effect (None if unknown or a flat stat).
    Effect EffectFromName(std::string const& name);
    char const* EffectName(Effect e);

    // Summed effect values of everything a character wears, with caps applied by Get().
    class EffectTotals
    {
    public:
        void Add(Effect e, int32_t value) { if (Valid(e)) _v[size_t(e)] += value; }
        void Remove(Effect e, int32_t value) { if (Valid(e)) _v[size_t(e)] -= value; }
        int32_t Raw(Effect e) const { return Valid(e) ? _v[size_t(e)] : 0; }
        // Capped total in hundredths of a percent (move speed 30%, cast speed 30%, cooldown 30%, CC/snare reduction 75%,
        // leech 20%, damage reduction 50%).
        int32_t Get(Effect e) const;
        bool Any() const;
    private:
        static bool Valid(Effect e) { return int(e) > 0 && int(e) < int(Effect::Count); }
        std::array<int32_t, size_t(Effect::Count)> _v{};
    };

    // Percent-of helper: amount * hundredths / 10000, rounded down, never negative.
    uint32_t PctOf(uint32_t amount, int32_t hundredths);

    // Proc check: chance in hundredths of a percent vs a roll in [0, 10000). Force = always.
    bool ProcRoll(int32_t chanceHundredths, uint32_t roll0to9999, bool force);

    // Crushing blow: extra damage = value-independent 10% of the target's CURRENT health, capped to 2.5% for elites/bosses
    // (D2's quarter for bosses) and to 3x the triggering hit so trash fights stay fast but bosses don't melt.
    uint32_t CrushingBlowDamage(uint32_t targetCurrentHealth, bool eliteOrBoss, uint32_t triggeringHit);

    // Phoenix (cheat death): if lethal and ready, the damage is reduced to leave 1 HP and the victim heals value% of max health.
    struct PhoenixResult { bool triggered; uint32_t damage; uint32_t heal; };
    PhoenixResult Phoenix(uint32_t damage, uint32_t currentHealth, uint32_t maxHealth, int32_t valueHundredths, bool ready);

    // Absorb pool: takes what it can from damage; returns the damage left, updates the pool.
    uint32_t AbsorbFromPool(uint32_t damage, uint32_t& pool);
}

#endif
