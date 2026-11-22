#include "Loot/AffixEffects.h"

#include "ArpgConfig.h"
#include "ArpgHooks.h"
#include "Test/TestEvents.h"
#include "Loot/Uniques.h"

#include "CellImpl.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "Log.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Timer.h"

#include <algorithm>
#include <list>
#include <map>
#include <mutex>
#include <unordered_map>

namespace
{
    using Arpg::Core::Effect;
    using Arpg::Core::EffectTotals;
    using Arpg::Core::PctOf;

    struct EffectTuning
    {
        uint32 spellId = 0;
        uint32 hitPct = 0;
        float radius = 0.0f;
        uint32 maxTargets = 1;
        uint32 durationMs = 0;
        uint32 cooldownMs = 0;
        uint32 schoolMask = SPELL_SCHOOL_MASK_NORMAL;
    };

    struct PlayerFx
    {
        EffectTotals totals;
        uint32 absorbPool = 0;      // Warding: refilled when a fight starts
        bool inCombat = false;
        uint32 phoenixReadyAt = 0;  // getMSTime() when Phoenix may trigger again
        uint32 lastRegen = 0;
        bool aurasDirty = true;     // move/cast speed auras need a refresh (done in OnUpdate, when in world)
        std::vector<Arpg::Loot::ActivePower> powers;   // equipped Uniques' powers (m3)
        std::map<uint32, uint32> powerReadyAt;          // item guid -> getMSTime() when the power may fire again
        std::map<uint32, int> powerHits;                 // item guid -> hits counted (every Nth hit)
        int32 paragonMove = 0;                           // Paragon Utility: move speed above the cap (hundredths of %)
    };

    std::mutex g_lock;
    std::map<Effect, EffectTuning> g_tuning;
    std::unordered_map<uint64, PlayerFx> g_fx;

    // >0 while we deal damage or heal ourselves: our hooks then do nothing (no proc chains, no thorns ping-pong).
    thread_local int t_depth = 0;
    struct Guard { Guard() { ++t_depth; } ~Guard() { --t_depth; } };

    EffectTuning Tuning(Effect e)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_tuning.find(e);
        return it == g_tuning.end() ? EffectTuning() : it->second;
    }

    bool TotalsOf(Unit* u, EffectTotals& out)
    {
        Player* p = u ? u->ToPlayer() : nullptr;
        if (!p)
            return false;
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_fx.find(p->GetGUID().GetCounter());
        if (it == g_fx.end() || !it->second.totals.Any())
            return false;
        out = it->second.totals;
        return true;
    }

    void Record(Player* p, std::string const& line)
    {
        if (Arpg::TestEvents::Enabled())
            Arpg::TestEvents::Record(p->GetGUID().GetCounter(), "fx " + line);
    }

    bool ForceProcs() { return sArpgConfig.Get().forceProcs; }

    bool IsEliteOrBoss(Unit* u)
    {
        Creature* c = u ? u->ToCreature() : nullptr;
        return c && (c->isElite() || c->isWorldBoss() || c->IsDungeonBoss());
    }

    uint32 DealProcDamage(Unit* attacker, Unit* target, uint32 amount, uint32 spellId, uint32 schoolMask)
    {
        if (!amount || !target || !target->IsAlive() || !attacker->IsValidAttackTarget(target))
            return 0;
        Guard g;
        SpellNonMeleeDamage d(attacker, target, spellId, schoolMask);
        d.damage = amount;
        attacker->DealSpellDamage(&d, false);
        attacker->SendSpellNonMeleeDamageLog(&d);
        return amount;
    }

    void Heal(Player* p, Unit* who, uint32 amount, uint32 spellId)
    {
        if (!amount || !who || !who->IsAlive())
            return;
        Guard g;
        if (SpellInfo const* si = spellId ? sSpellMgr->GetSpellInfo(spellId) : nullptr)
            p->HealBySpell(who, si, amount);
        else
            who->ModifyHealth(int32(amount));
    }

    std::list<Unit*> EnemiesNear(Unit* attacker, WorldObject* center, float radius)
    {
        std::list<Unit*> units;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(center, attacker, radius);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(center, units, check);
        Cell::VisitAllObjects(center, searcher, radius);
        // Owner 2026-11-22: splash/chain procs only hit what is hostile or already fighting (no neutral turkeys, no critters,
        // no pulling neutral packs). The primary target of a proc is added by the caller, not filtered here.
        units.remove_if([attacker](Unit* u)
        {
            if (!attacker->IsValidAttackTarget(u))
                return true;
            if (Creature* c = u->ToCreature())
                if (c->GetCreatureTemplate() && c->GetCreatureTemplate()->type == CREATURE_TYPE_CRITTER)
                    return true;
            Unit* v = u->GetVictim();
            bool fighting = u->IsInCombatWith(attacker) || (v && (v == attacker || v->IsFriendlyTo(attacker)));
            return !fighting && u->GetReactionTo(attacker) >= REP_NEUTRAL;   // neutral bystanders (turkeys, deer) are left alone
        });
        units.sort([center](Unit* a, Unit* b) { return center->GetExactDist(a) < center->GetExactDist(b); });
        return units;
    }

    void ApplyDot(Player* p, Unit* target, EffectTuning const& t, uint32 total)
    {
        if (!total || !target->IsAlive())
            return;
        Aura* a = nullptr;
        {
            Guard g;
            a = t.spellId ? p->AddAura(t.spellId, target) : nullptr;
        }
        if (!a)
        {
            DealProcDamage(p, target, total, t.spellId, t.schoolMask);   // no aura: deal it at once
            return;
        }
        if (t.durationMs)
        {
            a->SetMaxDuration(int32(t.durationMs));
            a->SetDuration(int32(t.durationMs));
        }
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (AuraEffect* e = a->GetEffect(i))
                if (e->GetAuraType() == SPELL_AURA_PERIODIC_DAMAGE)
                {
                    uint32 amplitude = e->GetAmplitude() > 0 ? uint32(e->GetAmplitude()) : 1000u;
                    uint32 ticks = std::max<uint32>(1, (t.durationMs ? t.durationMs : uint32(a->GetMaxDuration())) / amplitude);
                    e->ChangeAmount(float(std::max<uint32>(1, total / ticks)));
                }
    }

    using Arpg::Loot::ActivePower;
    using Arpg::Core::PowerTrigger;
    using Arpg::Core::PowerKind;

    std::vector<ActivePower> PowersOf(Player* p, PowerTrigger trigger)
    {
        std::vector<ActivePower> out;
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_fx.find(p->GetGUID().GetCounter());
        if (it != g_fx.end())
            for (ActivePower const& a : it->second.powers)
                if (a.trigger == trigger)
                    out.push_back(a);
        return out;
    }

    // Cooldown + (for every-Nth) the hit counter. True = fire now.
    bool PowerTriggers(Player* p, ActivePower const& a)
    {
        uint32 now = getMSTime();
        std::lock_guard<std::mutex> l(g_lock);
        PlayerFx& fx = g_fx[p->GetGUID().GetCounter()];
        if (!Arpg::Core::PowerReady(now, fx.powerReadyAt[a.itemGuid]))
            return false;
        bool fire;
        if (a.trigger == PowerTrigger::EveryNthHit)
        {
            int& hits = fx.powerHits[a.itemGuid];
            fire = ++hits >= std::max(1, a.triggerValue);
            if (fire)
                hits = 0;
        }
        else
            fire = Arpg::Core::ProcRoll(a.triggerValue * 100, urand(0, 9999), sArpgConfig.Get().forceProcs);
        if (fire && a.cooldownMs)
            fx.powerReadyAt[a.itemGuid] = now + a.cooldownMs;
        return fire;
    }

    uint32 PowerOf(Player* p)
    {
        return Arpg::Core::PowerStat(uint32(std::max(0.0f, p->GetTotalAttackPowerValue(BASE_ATTACK))),
            uint32(std::max(0, p->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ALL))));
    }

    // A damage power hits the target (radius 0) or the target + enemies around it, up to max_targets.
    void FireDamagePower(Player* p, Unit* target, ActivePower const& a)
    {
        uint32 dmg = Arpg::Core::PowerAmount(PowerKind::Damage, a.magnitude, PowerOf(p), p->GetMaxHealth());
        uint32 n = 0;
        if (a.radius > 0.0f)
        {
            std::list<Unit*> around = EnemiesNear(p, target, a.radius);
            around.remove(target);
            around.push_front(target);
            for (Unit* u : around)
                if (n < a.maxTargets && DealProcDamage(p, u, dmg, a.spellId, a.schoolMask))
                    ++n;
        }
        else if (DealProcDamage(p, target, dmg, a.spellId, a.schoolMask))
            n = 1;
        Record(p, "unique " + a.name + " dmg=" + std::to_string(dmg) + " targets=" + std::to_string(n));
    }

    // Attacker side, every landed hit (melee or spell): chance-on-hit and every-Nth-hit powers.
    void FireUniquePowersOnHit(Player* p, Unit* victim)
    {
        if (t_depth || !victim || !victim->IsAlive())
            return;
        for (PowerTrigger trig : { PowerTrigger::ChanceOnHit, PowerTrigger::EveryNthHit })
            for (ActivePower const& a : PowersOf(p, trig))
                if (a.kind == PowerKind::Damage && PowerTriggers(p, a))
                    FireDamagePower(p, victim, a);
    }

    // On-hit procs: Fire Nova (area), Chain Lightning (chain), Frost Nova (area + root), poison/bleed (DoT), crushing blow.
    void FireProcs(Player* p, Unit* victim, uint32 hit, EffectTotals const& t)
    {
        if (t_depth || !hit || !victim || !victim->IsAlive())
            return;
        bool const force = ForceProcs();
        auto roll = [&](Effect e) { return Arpg::Core::ProcRoll(t.Get(e), urand(0, 9999), force); };

        // Crushing blow first: it is a share of the target's current health, so it must land before the other procs.
        if (roll(Effect::CrushingBlowPct))
        {
            uint32 dmg = Arpg::Core::CrushingBlowDamage(victim->GetHealth(), IsEliteOrBoss(victim), hit);
            DealProcDamage(p, victim, dmg, 0, SPELL_SCHOOL_MASK_NORMAL);
            Record(p, "proc crushing_blow dmg=" + std::to_string(dmg));
        }

        if (roll(Effect::ProcFireNova))
        {
            EffectTuning tn = Tuning(Effect::ProcFireNova);
            uint32 n = 0, dmg = PctOf(hit, int32(tn.hitPct * 100));
            for (Unit* u : EnemiesNear(p, victim, tn.radius ? tn.radius : 8.0f))
                if (n < tn.maxTargets && DealProcDamage(p, u, dmg, tn.spellId, tn.schoolMask))
                    ++n;
            Record(p, "proc fire_nova dmg=" + std::to_string(dmg) + " targets=" + std::to_string(n));
        }
        if (roll(Effect::ProcChainLightning))
        {
            EffectTuning tn = Tuning(Effect::ProcChainLightning);
            uint32 n = 0, dmg = PctOf(hit, int32(tn.hitPct * 100));
            std::list<Unit*> chain = EnemiesNear(p, victim, tn.radius ? tn.radius : 10.0f);
            chain.remove(victim);
            chain.push_front(victim);
            for (Unit* u : chain)
                if (n < tn.maxTargets && DealProcDamage(p, u, dmg, tn.spellId, tn.schoolMask))
                    ++n;
            Record(p, "proc chain_lightning dmg=" + std::to_string(dmg) + " targets=" + std::to_string(n));
        }
        if (roll(Effect::ProcFrostNova))
        {
            EffectTuning tn = Tuning(Effect::ProcFrostNova);
            uint32 n = 0, dmg = PctOf(hit, int32(tn.hitPct * 100));
            for (Unit* u : EnemiesNear(p, p, tn.radius ? tn.radius : 8.0f))
            {
                if (n >= tn.maxTargets)
                    break;
                if (DealProcDamage(p, u, dmg, tn.spellId, tn.schoolMask) && u->IsAlive())
                {
                    Guard g;
                    if (tn.spellId)
                        p->AddAura(tn.spellId, u);   // Frost Nova's aura = the root
                    ++n;
                }
            }
            Record(p, "proc frost_nova dmg=" + std::to_string(dmg) + " targets=" + std::to_string(n));
        }
        if (roll(Effect::ProcPoison))
        {
            EffectTuning tn = Tuning(Effect::ProcPoison);
            uint32 total = PctOf(hit, int32(tn.hitPct * 100));
            ApplyDot(p, victim, tn, total);
            Record(p, "proc poison total=" + std::to_string(total));
        }
        if (roll(Effect::ProcBleed))
        {
            EffectTuning tn = Tuning(Effect::ProcBleed);
            uint32 total = PctOf(hit, int32(tn.hitPct * 100));
            ApplyDot(p, victim, tn, total);
            Record(p, "proc bleed total=" + std::to_string(total));
        }
    }

    // Move speed and cast speed come from one passive aura each (arpg_affix_effect.spell_id), amount = capped total %.
    void RefreshAura(Player* p, Effect e, int32 totalHundredths)
    {
        EffectTuning tn = Tuning(e);
        if (!tn.spellId || !sSpellMgr->GetSpellInfo(tn.spellId))
            return;
        Guard g;
        if (totalHundredths <= 0)
        {
            p->RemoveAurasDueToSpell(tn.spellId);
            return;
        }
        Aura* a = p->GetAura(tn.spellId);
        if (!a)
            a = p->AddAura(tn.spellId, p);
        if (!a)
            return;
        a->SetMaxDuration(-1);
        a->SetDuration(-1);
        if (AuraEffect* eff = a->GetEffect(0))
            eff->ChangeAmount(float(totalHundredths) / 100.0f);
    }

    void LootMoneyHook(Player* player, uint32& gold)
    {
        EffectTotals t;
        if (!TotalsOf(player, t))
            return;
        if (int32 g = t.Get(Effect::GoldFindPct))
        {
            uint32 extra = PctOf(gold, g);
            gold += extra;
            Record(player, "gold_find extra=" + std::to_string(extra));
        }
    }

    class ArpgEffectsUnitScript : public UnitScript
    {
    public:
        ArpgEffectsUnitScript() : UnitScript("arpg_affix_effects_unit") { }

        // White melee hits (before armour): weapon damage %, heal on hit, power on hit, procs; victim: thorns, struck chain.
        void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
        {
            if (t_depth || !target || !attacker)
                return;
            EffectTotals t;
            if (TotalsOf(attacker, t))
            {
                Player* p = attacker->ToPlayer();
                if (int32 w = t.Get(Effect::WeaponDmgPct))
                    damage += PctOf(damage, w);
                if (int32 h = t.Get(Effect::HealOnHitPct))
                {
                    uint32 heal = PctOf(p->GetMaxHealth(), h);
                    Heal(p, p, heal, 0);
                    Record(p, "heal_on_hit heal=" + std::to_string(heal));
                }
                if (int32 pw = t.Get(Effect::PowerOnHitPct))
                {
                    Powers pt = p->GetPowerType();
                    p->ModifyPower(pt, int32(PctOf(uint32(std::max(0, p->GetMaxPower(pt))), pw)));
                }
                FireProcs(p, target, damage, t);
            }
            if (Player* pa = attacker->ToPlayer())
                FireUniquePowersOnHit(pa, target);
            if (Player* pv = target->ToPlayer())
                for (ActivePower const& a : PowersOf(pv, PowerTrigger::WhenStruck))
                    if (a.kind == PowerKind::HealAbsorb && PowerTriggers(pv, a))
                    {
                        uint32 shield = Arpg::Core::PowerAmount(PowerKind::HealAbsorb, a.magnitude, 0, pv->GetMaxHealth());
                        {
                            std::lock_guard<std::mutex> l(g_lock);
                            g_fx[pv->GetGUID().GetCounter()].absorbPool += shield;
                        }
                        Record(pv, "unique " + a.name + " shield=" + std::to_string(shield));
                    }
            if (TotalsOf(target, t))
            {
                Player* v = target->ToPlayer();
                if (int32 th = t.Get(Effect::ThornsPct))
                {
                    uint32 dmg = DealProcDamage(v, attacker, PctOf(damage, th), 0, SPELL_SCHOOL_MASK_NORMAL);
                    Record(v, "thorns dmg=" + std::to_string(dmg));
                }
                if (Arpg::Core::ProcRoll(t.Get(Effect::ProcStruckChain), urand(0, 9999), ForceProcs()))
                {
                    EffectTuning tn = Tuning(Effect::ProcStruckChain);
                    uint32 n = 0, dmg = PctOf(damage, int32(tn.hitPct * 100));
                    std::list<Unit*> chain = EnemiesNear(v, attacker, tn.radius ? tn.radius : 10.0f);
                    chain.remove(attacker);
                    chain.push_front(attacker);
                    for (Unit* u : chain)
                        if (n < tn.maxTargets && DealProcDamage(v, u, dmg, tn.spellId, tn.schoolMask))
                            ++n;
                    Record(v, "proc struck_chain dmg=" + std::to_string(dmg) + " targets=" + std::to_string(n));
                }
            }
        }

        // Spell and ability hits: spell power % (non-physical), procs.
        void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
        {
            if (t_depth || !target || !attacker || damage <= 0)
                return;
            if (Player* pa = attacker->ToPlayer())
                FireUniquePowersOnHit(pa, target);
            EffectTotals t;
            if (!TotalsOf(attacker, t))
                return;
            Player* p = attacker->ToPlayer();
            if (spellInfo && !(spellInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_NORMAL))
                if (int32 sp = t.Get(Effect::SpellPowerPct))
                    damage += int32(PctOf(uint32(damage), sp));
            FireProcs(p, target, uint32(damage), t);
        }

        // Every damage (after absorbs), proc damage included: attacker leech / power leech / atonement / elite and martyr
        // bonus; victim elite reduction, Warding absorb, Phoenix. No t_depth check here: nothing below starts new procs
        // (procs only start in the hit hooks above, which do check it), so there is no loop.
        void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
        {
            if (!attacker || !victim || !damage)
                return;
            EffectTotals t;
            if (TotalsOf(attacker, t))
            {
                Player* p = attacker->ToPlayer();
                if (IsEliteOrBoss(victim))
                    damage += PctOf(damage, t.Get(Effect::DmgVsElitePct));
                if (p->GetHealthPct() < 50.0f)
                    damage += PctOf(damage, t.Get(Effect::MartyrDmgPct));
                if (int32 l = t.Get(Effect::LeechPct))
                {
                    uint32 heal = PctOf(damage, l);
                    Heal(p, p, heal, Tuning(Effect::LeechPct).spellId);
                    Record(p, "leech heal=" + std::to_string(heal));
                }
                if (int32 ml = t.Get(Effect::PowerLeechPct))
                    if (p->GetMaxPower(POWER_MANA) > 0)
                        p->ModifyPower(POWER_MANA, int32(PctOf(damage, ml)));
                if (int32 at = t.Get(Effect::AtonementPct))
                {
                    Unit* lowest = p;
                    if (Group* g = p->GetGroup())
                        for (GroupReference* ref = g->GetFirstMember(); ref; ref = ref->next())
                            if (Player* m = ref->GetSource())
                                if (m->IsAlive() && m->IsInMap(p) && p->IsWithinDistInMap(m, 40.0f) && m->GetHealthPct() < lowest->GetHealthPct())
                                    lowest = m;
                    Heal(p, lowest, PctOf(damage, at), 0);
                }
            }
            if (TotalsOf(victim, t))
            {
                Player* v = victim->ToPlayer();
                if (IsEliteOrBoss(attacker))
                    damage -= PctOf(damage, t.Get(Effect::DrVsElitePct));
                uint32 before = damage;
                {
                    std::lock_guard<std::mutex> l(g_lock);
                    auto it = g_fx.find(v->GetGUID().GetCounter());
                    if (it != g_fx.end() && it->second.absorbPool)
                        damage = Arpg::Core::AbsorbFromPool(damage, it->second.absorbPool);
                }
                if (before != damage)
                    Record(v, "warding absorbed=" + std::to_string(before - damage));
                if (int32 ph = t.Get(Effect::PhoenixHealPct))
                {
                    uint32 now = getMSTime();
                    bool ready;
                    {
                        std::lock_guard<std::mutex> l(g_lock);
                        PlayerFx& fx = g_fx[v->GetGUID().GetCounter()];
                        ready = !fx.phoenixReadyAt || int32(now - fx.phoenixReadyAt) >= 0;
                    }
                    Arpg::Core::PhoenixResult r = Arpg::Core::Phoenix(damage, v->GetHealth(), v->GetMaxHealth(), ph, ready);
                    if (r.triggered)
                    {
                        damage = r.damage;
                        {
                            std::lock_guard<std::mutex> l(g_lock);
                            g_fx[v->GetGUID().GetCounter()].phoenixReadyAt = now + std::max<uint32>(1000, Tuning(Effect::PhoenixHealPct).cooldownMs);
                        }
                        Heal(v, v, r.heal, 0);
                        Record(v, "phoenix heal=" + std::to_string(r.heal));
                    }
                }
            }
        }

        // Healers (BALANCE §10): overheal -> damage (Radiant), healing -> damage (Zealous) to the healer's enemy target.
        void OnHeal(Unit* healer, Unit* receiver, uint32& gain) override
        {
            if (t_depth || !healer || !receiver || !gain)
                return;
            Player* p = healer->ToPlayer();
            if (!p)
                return;
            EffectTotals t;
            TotalsOf(healer, t);   // may be empty: Unique powers still apply
            Unit* enemy = p->GetSelectedUnit();
            if (!enemy || !p->IsValidAttackTarget(enemy))
                enemy = p->GetVictim();
            if (!enemy || !p->IsValidAttackTarget(enemy))
                return;
            for (ActivePower const& a : PowersOf(p, PowerTrigger::ChanceOnHeal))
                if (a.kind == PowerKind::Damage && PowerTriggers(p, a))
                    FireDamagePower(p, enemy, a);
            uint32 missing = receiver->GetMaxHealth() - receiver->GetHealth();
            uint32 overheal = gain > missing ? gain - missing : 0;
            uint32 dmg = PctOf(overheal, t.Get(Effect::OverhealToDmgPct)) + PctOf(gain, t.Get(Effect::HealToDmgPct));
            if (dmg)
            {
                DealProcDamage(p, enemy, dmg, 0, SPELL_SCHOOL_MASK_HOLY);
                Record(p, "heal_to_damage dmg=" + std::to_string(dmg));
            }
        }
    };

    class ArpgEffectsPlayerScript : public PlayerScript
    {
    public:
        ArpgEffectsPlayerScript() : PlayerScript("arpg_affix_effects_player") { }

        void OnGiveXP(Player* player, uint32& amount, Unit* /*victim*/) override
        {
            EffectTotals t;
            if (TotalsOf(player, t))
                if (uint32 extra = PctOf(amount, t.Get(Effect::XpPct)))
                {
                    amount += extra;
                    Record(player, "xp extra=" + std::to_string(extra));
                }
        }

        // Regeneration (every 5 s), Warding pool on entering combat, aura refresh once in the world.
        void OnUpdate(Player* player, uint32 /*diff*/) override
        {
            if (!player->IsInWorld())
                return;
            uint64 key = player->GetGUID().GetCounter();
            bool dirty = false, refill = false, regen = false;
            int32 move = 0, cast = 0, absorb = 0, regenPct = 0;
            uint32 now = getMSTime();
            {
                std::lock_guard<std::mutex> l(g_lock);
                auto it = g_fx.find(key);
                if (it == g_fx.end())
                    return;
                PlayerFx& fx = it->second;
                dirty = fx.aurasDirty;
                fx.aurasDirty = false;
                move = fx.totals.Get(Effect::MoveSpeedPct);
                for (ActivePower const& a : fx.powers)   // "Long Road": flat move speed ABOVE the affix cap
                    if (a.trigger == PowerTrigger::Passive && a.kind == PowerKind::Flat)
                        move += a.magnitude * 100;
                move += fx.paragonMove;                  // Paragon Utility, also above the cap
                cast = fx.totals.Get(Effect::CastSpeedPct);
                absorb = fx.totals.Get(Effect::CombatAbsorbPct);
                regenPct = fx.totals.Get(Effect::RegenPct);
                bool combat = player->IsInCombat();
                if (combat && !fx.inCombat && absorb > 0)
                {
                    fx.absorbPool = PctOf(player->GetMaxHealth(), absorb);
                    refill = true;
                }
                if (!combat)
                    fx.absorbPool = 0;
                fx.inCombat = combat;
                if (regenPct > 0 && getMSTimeDiff(fx.lastRegen, now) >= 5000)
                {
                    fx.lastRegen = now;
                    regen = true;
                }
            }
            if (dirty)
            {
                RefreshAura(player, Effect::MoveSpeedPct, move);
                RefreshAura(player, Effect::CastSpeedPct, cast);
            }
            if (refill)
                Record(player, "warding pool=" + std::to_string(PctOf(player->GetMaxHealth(), absorb)));
            if (regen && player->IsAlive() && player->GetHealth() < player->GetMaxHealth())
                Heal(player, player, PctOf(player->GetMaxHealth(), regenPct), 0);
        }
    };
}

namespace Arpg::Loot
{
    std::string LoadEffectData()
    {
        std::map<Effect, EffectTuning> tuning;
        uint32 missingSpells = 0;
        if (QueryResult r = WorldDatabase.Query("SELECT effect, spell_id, hit_pct, radius, max_targets, duration_ms, cooldown_ms, school_mask FROM arpg_affix_effect"))
            do
            {
                Field* f = r->Fetch();
                Effect e = Core::EffectFromName(f[0].GetString());
                if (e == Effect::None)
                    continue;
                EffectTuning t;
                t.spellId = f[1].GetUInt32();
                t.hitPct = f[2].GetUInt8();
                t.radius = f[3].GetFloat();
                t.maxTargets = std::max<uint32>(1, f[4].GetUInt8());
                t.durationMs = f[5].GetUInt32();
                t.cooldownMs = f[6].GetUInt32();
                t.schoolMask = f[7].GetUInt32();
                if (t.spellId && !sSpellMgr->GetSpellInfo(t.spellId))
                {
                    TC_LOG_ERROR("server.loading", "mod_arpg: arpg_affix_effect %s uses missing spell %u", f[0].GetCString(), t.spellId);
                    ++missingSpells;
                }
                tuning[e] = t;
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_tuning = std::move(tuning);
        std::string msg = "effects=" + std::to_string(g_tuning.size()) + " missing_spells=" + std::to_string(missingSpells);
        TC_LOG_INFO("server.loading", "mod_arpg: affix effect data %s", msg.c_str());
        return msg;
    }

    void InstallAffixEffects()
    {
        ArpgHooks::LootMoney = &LootMoneyHook;
        new ArpgEffectsUnitScript();
        new ArpgEffectsPlayerScript();
    }

    void ApplyEffectLines(Player* player, std::vector<EffectLine> const& lines, bool apply)
    {
        if (!player || lines.empty())
            return;
        {
            std::lock_guard<std::mutex> l(g_lock);
            PlayerFx& fx = g_fx[player->GetGUID().GetCounter()];
            for (EffectLine const& el : lines)
            {
                if (apply)
                    fx.totals.Add(Effect(el.effect), el.value);
                else
                    fx.totals.Remove(Effect(el.effect), el.value);
            }
            fx.aurasDirty = true;
        }
        // Armour %: a multiplier per line, symmetric with the remove (no game call needs the lock).
        for (EffectLine const& el : lines)
            if (Effect(el.effect) == Effect::ArmorPct && el.value > 0)
                player->HandleStatModifier(UNIT_MOD_ARMOR, TOTAL_PCT, float(el.value) / 100.0f, apply);
    }

    void ApplyUniquePower(Player* player, uint32 itemGuid, bool apply)
    {
        ActivePower a;
        if (!player || !PowerFor(itemGuid, a))
            return;
        std::lock_guard<std::mutex> l(g_lock);
        PlayerFx& fx = g_fx[player->GetGUID().GetCounter()];
        auto it = std::find_if(fx.powers.begin(), fx.powers.end(), [itemGuid](ActivePower const& x) { return x.itemGuid == itemGuid; });
        if (apply && it == fx.powers.end())
            fx.powers.push_back(a);
        else if (!apply && it != fx.powers.end())
            fx.powers.erase(it);
        fx.aurasDirty = true;
    }

    void ForgetEffects(uint64 playerGuid)
    {
        std::lock_guard<std::mutex> l(g_lock);
        g_fx.erase(playerGuid);
    }

    int32 EffectTotal(Player* player, Core::Effect e)
    {
        EffectTotals t;
        return TotalsOf(player, t) ? t.Get(e) : 0;
    }

    std::string DescribeEffects(Player* player)
    {
        EffectTotals t;
        std::string out = "effects";
        if (TotalsOf(player, t))   // a Unique with no affixes still has its power's aura: no early "none"
            for (int i = 1; i < int(Effect::Count); ++i)
                if (int32 v = t.Get(Effect(i)))
                    out += std::string(" ") + Core::EffectName(Effect(i)) + "=" + std::to_string(v);
        // The live aura amounts (refreshed on the next player update after a change).
        for (Effect e : { Effect::MoveSpeedPct, Effect::CastSpeedPct })
            if (uint32 spell = Tuning(e).spellId)
                if (Aura* a = player->GetAura(spell))
                    if (AuraEffect* eff = a->GetEffect(0))
                        out += std::string(" aura_") + Core::EffectName(e) + "=" + std::to_string(eff->GetAmount());
        return out == "effects" ? out + " none" : out;
    }

    void SetParagonMoveSpeed(Player* player, int32 hundredths)
    {
        std::lock_guard<std::mutex> l(g_lock);
        PlayerFx& fx = g_fx[player->GetGUID().GetCounter()];
        fx.paragonMove = std::max(0, hundredths);
        fx.aurasDirty = true;
    }

    bool AddTestEffect(Player* player, std::string const& name, int32 value, std::string& message)
    {
        Effect e = Core::EffectFromName(name);
        if (e == Effect::None || value == 0)
        {
            message = "unknown effect " + name;
            return false;
        }
        ApplyEffectLines(player, { EffectLine{ int(e), value > 0 ? value : -value } }, value > 0);
        message = DescribeEffects(player);
        return true;
    }
}
