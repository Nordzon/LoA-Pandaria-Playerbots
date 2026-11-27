#include "Scaling/WorldScaling.h"
#include "Loot/ZoneLevels.h"
#include "Core/Paragon.h"
#include "Paragon/Paragon.h"

#include "ArpgConfig.h"
#include "ArpgHooks.h"
#include "Test/TestEvents.h"
#include "CellImpl.h"
#include "Creature.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Timer.h"
#include "WorldSession.h"

#include <cmath>
#include <cstdio>
#include <list>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace
{
    struct ScaledEntry
    {
        uint8 target = 0;     // level the hook returns
        uint8 original = 0;   // level before scaling (the core's own roll)
        bool pinned = false;  // Champion / Superunique / minion (m3): level and multipliers fixed, the tick leaves it alone
        float tier = 1.0f;    // m4 world tier (uncapped zones): health and damage x this on top of the scaling mods
        float healthMult = 1.0f, damageMult = 1.0f;   // pinned only, on top of the scaling mods
    };

    std::unordered_map<uint64, bool> g_seen;           // creature guid -> already offered to the Champion roll this life
    Arpg::Scaling::NewCandidateFn g_onNewCandidate = nullptr;

    // Map threads update in parallel: every access goes through the mutex.
    std::mutex g_lock;
    std::unordered_map<uint64, ScaledEntry> g_scaled;          // creature guid -> scaling
    std::unordered_map<uint64, uint32> g_lastTick;             // player guid -> getMSTime of last check
    std::unordered_map<uint64, uint32> g_sweepCounter;         // player guid -> ticks since last sweep

    bool FindEntry(Creature* c, ScaledEntry& out)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_scaled.find(c->GetGUID().GetRawValue());
        if (it == g_scaled.end())
            return false;
        out = it->second;
        return true;
    }

    bool AnyRealPlayerNear(Creature* c, float radius)
    {
        Map::PlayerList const& players = c->GetMap()->GetPlayers();
        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* p = itr->GetSource();
            if (p && p->IsInWorld() && Arpg::Scaling::IsRealPlayer(p) && c->IsWithinDist(p, radius))
                return true;
        }
        return false;
    }

    // Creature::SelectLevel hook (map thread). Spawns/respawns: drop the scaling when no real player is near.
    uint8 CreatureLevelHook(Creature* creature, uint8 /*level*/)
    {
        ScaledEntry e;
        if (!FindEntry(creature, e))
            return 0;
        // Called by our own Scale() (entry just written, the player is near) or by the core on respawn/re-init.
        Arpg::Settings s = sArpgConfig.Get();
        if (e.pinned && creature->IsInWorld() && creature->IsAlive())
            return e.target;
        if (!e.pinned && s.scale.enabled && creature->IsInWorld() && AnyRealPlayerNear(creature, s.scaleRadius * 1.2f))
            return e.target;
        std::lock_guard<std::mutex> l(g_lock);
        g_scaled.erase(creature->GetGUID().GetRawValue());
        return 0;
    }

    bool IsCandidateType(Creature* c)
    {
        if (!c->IsAlive() || c->IsPet() || c->IsSummon() || c->IsTotem() || c->IsGuardian())
            return false;
        if (!c->GetOwnerGUID().IsEmpty() || c->IsTrigger() || c->IsCivilian())
            return false;
        CreatureTemplate const* t = c->GetCreatureTemplate();
        return t && t->type != CREATURE_TYPE_CRITTER;
    }

    void ApplyModsAndRefresh(Creature* c, Arpg::Settings const& s, bool scaled, float extraHealth = 1.0f, float extraDamage = 1.0f)
    {
        float hm = Arpg::Core::HealthMultiplier(s.scale, scaled) * extraHealth;
        float dm = Arpg::Core::DamageMultiplier(s.scale, scaled) * extraDamage;
        if (hm != 1.0f)
        {
            uint32 hp = uint32(c->GetMaxHealth() * hm);
            c->SetCreateHealth(hp);
            c->SetModifierValue(UNIT_MOD_HEALTH, BASE_VALUE, float(hp));
        }
        if (dm != 1.0f)
            for (WeaponAttackType t : { BASE_ATTACK, OFF_ATTACK, RANGED_ATTACK })
            {
                c->SetBaseWeaponDamage(t, MINDAMAGE, c->GetWeaponDamageRange(t, MINDAMAGE) * dm);
                c->SetBaseWeaponDamage(t, MAXDAMAGE, c->GetWeaponDamageRange(t, MAXDAMAGE) * dm);
            }
        c->UpdateAllStats();
        c->SetHealth(c->GetMaxHealth());
    }

    void Scale(Creature* c, uint8 target, uint8 original, Arpg::Settings const& s, float tier = 1.0f)
    {
        {
            std::lock_guard<std::mutex> l(g_lock);
            ScaledEntry e;
            e.target = target;
            e.original = original;
            e.tier = tier;
            g_scaled[c->GetGUID().GetRawValue()] = e;
        }
        c->SelectLevel(c->GetCreatureTemplate(), true); // hook returns `target`; core computes stats for it
        ApplyModsAndRefresh(c, s, true, tier, tier);
    }

    void Revert(Creature* c, Arpg::Settings const& s)
    {
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_scaled.erase(c->GetGUID().GetRawValue());
        }
        c->SelectLevel(c->GetCreatureTemplate(), true); // keepLevel: back to the core's original roll
        ApplyModsAndRefresh(c, s, false);
    }

    // Reverts out-of-combat scaled creatures on this player's map that no real player is near (or all, if disabled).
    void Sweep(Player* player, Arpg::Settings const& s)
    {
        std::vector<uint64> guids;
        {
            std::lock_guard<std::mutex> l(g_lock);
            guids.reserve(g_scaled.size());
            for (auto const& kv : g_scaled)
                guids.push_back(kv.first);
        }
        for (uint64 raw : guids)
        {
            ObjectGuid guid(raw);
            Creature* c = ObjectAccessor::GetCreature(*player, guid); // same map only
            ScaledEntry pe;
            if (c && FindEntry(c, pe) && pe.pinned)
            {
                if (!c->IsAlive())   // died without a player kill (bots, other mobs): the respawn starts clean
                {
                    std::lock_guard<std::mutex> l(g_lock);
                    g_scaled.erase(raw);
                    g_seen.erase(raw);
                }
                continue;
            }
            if (!c || !c->IsAlive() || c->IsInCombat())
                continue;
            if (!s.scale.enabled || !AnyRealPlayerNear(c, s.scaleRadius * 1.2f))
                Revert(c, s);
        }
    }

    void Tick(Player* player)
    {
        Arpg::Settings s = sArpgConfig.Get();
        Map* map = player->GetMap();
        if (!map || map->Instanceable())
            return; // open world only (dungeons: m5)

        uint64 pg = player->GetGUID().GetRawValue();
        bool sweep;
        {
            std::lock_guard<std::mutex> l(g_lock);
            sweep = (++g_sweepCounter[pg] % 5) == 0 || !s.scale.enabled;
        }
        if (sweep)
            Sweep(player, s);
        if (!s.scale.enabled || !player->IsAlive())
            return;

        std::list<Unit*> units;
        Trinity::AnyUnitInObjectRangeCheck check(player, s.scaleRadius);
        Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(player, units, check);
        Cell::VisitAllObjects(player, searcher, s.scaleRadius);
        // m8 (owner 2026-11-27): with several real players, a monster follows the highest level among those near it, and
        // its world tier follows Arpg.Paragon.WorldTierMode over their Paragon levels. Real players of this map, once per tick.
        std::vector<Player*> real;
        Map::PlayerList const& mapPlayers = map->GetPlayers();
        for (Map::PlayerList::const_iterator itr = mapPlayers.begin(); itr != mapPlayers.end(); ++itr)
            if (Player* rp = itr->GetSource())
                if (rp != player && rp->IsInWorld() && Arpg::Scaling::IsRealPlayer(rp) && rp->IsWithinDist(player, s.scaleRadius * 2.0f))
                    real.push_back(rp);

        for (Unit* u : units)
        {
            Creature* c = u->ToCreature();
            if (!c || !IsCandidateType(c) || c->IsInCombat()) // in combat (e.g. with a bot): keep its level
                continue;
            if (!player->IsValidAttackTarget(c))
                continue; // vendors, guards of your faction, quest givers stay as they are
            ScaledEntry e;
            bool known = FindEntry(c, e);
            if (known && e.pinned)
                continue;
            if (g_onNewCandidate)
            {
                bool first;
                {
                    std::lock_guard<std::mutex> l(g_lock);
                    first = g_seen.emplace(c->GetGUID().GetRawValue(), true).second;
                }
                if (first && g_onNewCandidate(c, player))
                    continue;   // it became a Champion (pinned)
            }
            uint8 original = known ? e.original : c->GetLevel();
            int playerLevel = int(player->GetLevel());
            std::vector<int> paragons{ Arpg::Paragon::ParagonLevel(player) };
            for (Player* rp : real)
                if (c->IsWithinDist(rp, s.scaleRadius))
                {
                    playerLevel = std::max(playerLevel, int(rp->GetLevel()));
                    paragons.push_back(Arpg::Paragon::ParagonLevel(rp));
                }
            // Zone cap (task 4.9): scale to at most the zone's max level + Arpg.Scale.ZoneCapAbove (Vale: no cap).
            int effective = Arpg::Core::LootLevel(playerLevel, Arpg::Loot::ZoneLevelsFor(c->GetZoneId(), c->GetAreaId()),
                s.scaleZoneCapAbove, false);
            uint8 target = Arpg::Core::ScaledLevel(s.scale, original, effective);
            // World tier (m4, SPEC 4.1 "Paragon grind"): in uncapped zones (the Vale) monsters grow with the player's Paragon.
            float tier = 1.0f;
            if (s.uncappedZones.count(c->GetZoneId()))
                tier = Arpg::Core::WorldTierMultiplier(Arpg::Core::GroupParagonLevel(paragons, s.paragonWorldTierMode),
                    s.paragonWorldTierPct, s.paragonWorldTierCapPct);
            if (target == 0 && tier > 1.0f)
                target = original;   // no level change, but the tier still applies
            if (target == 0)
            {
                if (known)
                {
                    Revert(c, s);
                    Arpg::TestEvents::Record(player->GetGUID().GetCounter(), "revert entry=" + std::to_string(c->GetEntry()) +
                        " spawn=" + std::to_string(c->GetDBTableGUIDLow()) + " level=" + std::to_string(c->GetLevel()));
                }
                continue;
            }
            if (!known || e.target != target || std::fabs(e.tier - tier) > 0.001f)
            {
                Scale(c, target, original, s, tier);
                Arpg::TestEvents::Record(player->GetGUID().GetCounter(), "scale entry=" + std::to_string(c->GetEntry()) +
                    " spawn=" + std::to_string(c->GetDBTableGUIDLow()) + " from=" + std::to_string(original) +
                    " to=" + std::to_string(target) + " hp=" + std::to_string(c->GetMaxHealth()) +
                    (tier > 1.0f ? " tier=" + std::to_string(int(tier * 100 + 0.5f)) : ""));
            }
        }
    }

    class ArpgScalingPlayerScript : public PlayerScript
    {
    public:
        ArpgScalingPlayerScript() : PlayerScript("arpg_world_scaling") { }

        void OnUpdate(Player* player, uint32 /*diff*/) override
        {
            if (!player->IsInWorld() || !Arpg::Scaling::IsRealPlayer(player))
                return;
            uint32 now = getMSTime();
            uint64 pg = player->GetGUID().GetRawValue();
            {
                std::lock_guard<std::mutex> l(g_lock);
                uint32& last = g_lastTick[pg];
                if (last != 0 && getMSTimeDiff(last, now) < sArpgConfig.Get().scaleTickMs)
                    return;
                last = now;
            }
            Tick(player);
        }

        void OnLogout(Player* player) override
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_lastTick.erase(player->GetGUID().GetRawValue());
            g_sweepCounter.erase(player->GetGUID().GetRawValue());
        }
    };
}

namespace Arpg::Scaling
{
    void Install()
    {
        ArpgHooks::CreatureLevel = &CreatureLevelHook;
        new ArpgScalingPlayerScript();
    }

    float TierOf(Creature* creature)
    {
        ScaledEntry e;
        return creature && FindEntry(creature, e) ? std::max(1.0f, e.tier) : 1.0f;
    }

    bool IsRealPlayer(Player* player)
    {
        WorldSession* session = player ? player->GetSession() : nullptr;
        return session && !session->IsBot();
    }

    Creature* FindNearestCandidate(Player* player, float radius)
    {
        std::list<Unit*> units;
        Trinity::AnyUnitInObjectRangeCheck check(player, radius);
        Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(player, units, check);
        Cell::VisitAllObjects(player, searcher, radius);
        Creature* best = nullptr;
        float bestDist = radius + 1.0f;
        for (Unit* u : units)
        {
            Creature* c = u->ToCreature();
            if (!c || !IsCandidateType(c) || !player->IsValidAttackTarget(c))
                continue;
            float d = player->GetDistance(c);
            if (d < bestDist)
            {
                best = c;
                bestDist = d;
            }
        }
        return best;
    }

    Creature* FindNearestScaled(Player* player, float radius, bool uncappedOnly)
    {
        Arpg::Settings const s = sArpgConfig.Get();
        std::list<Unit*> units;
        Trinity::AnyUnitInObjectRangeCheck check(player, radius);
        Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(player, units, check);
        Cell::VisitAllObjects(player, searcher, radius);
        Creature* best = nullptr;
        float bestDist = radius + 1.0f;
        for (Unit* u : units)
        {
            Creature* c = u->ToCreature();
            ScaledEntry e;
            if (!c || !c->IsAlive() || c->IsInCombat() || (uncappedOnly && !s.uncappedZones.count(c->GetZoneId())) || !FindEntry(c, e) || e.pinned)
                continue;
            float d = player->GetDistance(c);
            if (d < bestDist)
            {
                best = c;
                bestDist = d;
            }
        }
        return best;
    }

    std::string Describe(Creature* c)
    {
        ScaledEntry e;
        bool scaled = FindEntry(c, e);
        CreatureTemplate const* t = c->GetCreatureTemplate();
        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "%s entry=%u spawn=%u level=%u template=%u-%u original=%u scaled=%s hp=%u tier_pct=%d",
            c->GetName().c_str(), c->GetEntry(), c->GetDBTableGUIDLow(), uint32(c->GetLevel()),
            t ? uint32(t->minlevel) : 0u, t ? uint32(t->maxlevel) : 0u, scaled ? uint32(e.original) : uint32(c->GetLevel()),
            scaled ? std::to_string(e.target).c_str() : "none", c->GetMaxHealth(), scaled ? int(e.tier * 100 + 0.5f) : 100);
        return buf;
    }

    void Pin(Creature* c, uint8 level, float healthMult, float damageMult)
    {
        Arpg::Settings s = sArpgConfig.Get();
        uint8 original = c->GetLevel();
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_scaled.find(c->GetGUID().GetRawValue());
            if (it != g_scaled.end())
                original = it->second.original;
            ScaledEntry e;
            e.target = level;
            e.original = original;
            e.pinned = true;
            e.healthMult = healthMult;
            e.damageMult = damageMult;
            g_scaled[c->GetGUID().GetRawValue()] = e;
            g_seen[c->GetGUID().GetRawValue()] = true;
        }
        c->SelectLevel(c->GetCreatureTemplate(), true);
        ApplyModsAndRefresh(c, s, true, healthMult, damageMult);
    }

    bool IsPinned(Creature* c)
    {
        ScaledEntry e;
        return FindEntry(c, e) && e.pinned;
    }

    void Forget(uint64 rawGuid)
    {
        std::lock_guard<std::mutex> l(g_lock);
        g_scaled.erase(rawGuid);
        g_seen.erase(rawGuid);
    }

    void SetNewCandidateHandler(NewCandidateFn fn)
    {
        g_onNewCandidate = fn;
    }

    size_t ScaledCount()
    {
        std::lock_guard<std::mutex> l(g_lock);
        return g_scaled.size();
    }
}
