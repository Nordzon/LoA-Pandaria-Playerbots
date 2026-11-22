#include "Monsters/Superuniques.h"
#include "Monsters/Champions.h"

#include "ArpgConfig.h"
#include "Core/LootSubstitution.h"
#include "Core/MonsterBands.h"
#include "Core/Pity.h"
#include "Core/SuperuniqueRotation.h"
#include "Loot/BossPity.h"
#include "Loot/Substitution.h"
#include "Loot/Uniques.h"
#include "Loot/ZoneLevels.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "Creature.h"
#include "DatabaseEnv.h"
#include "LootMgr.h"
#include "DBCStores.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "TemporarySummon.h"
#include "Timer.h"
#include "World.h"

#include <algorithm>
#include <map>
#include <mutex>
#include <unordered_map>

namespace
{
    struct Def { uint32 id, zone, entry, kit, weight; uint32 uniqueId = 0; };
    struct Point { uint32 map; float x, y, z, o; };
    struct Ability { uint32 spell; uint8 kind; uint32 everyMs; uint32 damagePct; uint8 minLevel; };

    struct Live
    {
        uint32 id = 0;
        uint64 guid = 0;
        Arpg::Core::MonsterBand band;
        std::vector<Ability> abilities;
        std::vector<uint32> lastUse;       // getMSTime per ability
    };

    struct ZoneState
    {
        std::map<uint32, Live> alive;      // superunique id -> live creature
        uint32 respawnAt = 0;              // getMSTime; 0 = now
        uint32 lastKilled = 0;
        uint32 lastPlayerSeen = 0;
    };

    std::mutex g_lock;
    std::unordered_map<uint32, Def> g_defs;                          // id -> def
    std::unordered_map<uint32, std::vector<uint32>> g_pools;         // zone -> ids
    std::unordered_map<uint32, std::vector<Point>> g_points;         // id -> spawn points
    std::unordered_map<uint32, std::vector<Ability>> g_kits;         // kit -> abilities by slot
    std::vector<Arpg::Core::MonsterBand> g_bands;
    std::unordered_map<uint32, ZoneState> g_zones;                   // zone -> state
    std::unordered_map<uint64, std::pair<uint32, uint32>> g_byGuid;  // creature guid -> (zone, id)
    std::unordered_map<uint32, uint64> g_lastKilledBy;               // player guid -> corpse guid
    std::unordered_map<uint32, uint32> g_lastTick;                   // player guid -> getMSTime

    uint32 Rand(uint32 n) { return n ? urand(0, n - 1) : 0u; }

    void Rec(Player* p, std::string const& line)
    {
        if (p && Arpg::TestEvents::Enabled())
            Arpg::TestEvents::Record(p->GetGUID().GetCounter(), line);
    }

    // Builds the live state and spawns the creature at `pos` (grid loaded first; kept active so it isn't unloaded).
    bool Spawn(Player* p, uint32 zone, uint32 id, Position const& pos, std::string& out)
    {
        Def def;
        std::vector<Ability> kit;
        Arpg::Core::MonsterBand band;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto d = g_defs.find(id);
            if (d == g_defs.end()) { out = "unknown superunique " + std::to_string(id); return false; }
            def = d->second;
            kit = g_kits[def.kit];
            Arpg::Core::MonsterBand const* b = Arpg::Core::BandFor(g_bands, Arpg::Core::MonsterKind::Superunique, int(p->GetLevel()));
            if (!b) { out = "no superunique band"; return false; }
            band = *b;
        }
        Map* map = p->GetMap();
        map->LoadGrid(pos.GetPositionX(), pos.GetPositionY());
        // Tag (owner 2026-11-22): its "Unique <name>" copy (Rare Elite portrait) when the copy exists.
        Arpg::Settings const& ts = sArpgConfig.Get();
        uint32 entry = ts.superuniqueEntryOffset && sObjectMgr->GetCreatureTemplate(def.entry + ts.superuniqueEntryOffset)
            ? def.entry + ts.superuniqueEntryOffset : def.entry;
        TempSummon* c = map->SummonCreature(entry, pos);
        if (!c) { out = "could not spawn entry " + std::to_string(def.entry); return false; }
        c->setActive(true, ActiveFlags::Script);
        // Patrol (m3 3.4): roams around its spawn point instead of standing still.
        Arpg::Settings const& ps = sArpgConfig.Get();
        if (ps.superuniquePatrolRadius > 0.0f)
        {
            c->SetWanderDistance(ps.superuniquePatrolRadius);
            c->SetDefaultMovementType(RANDOM_MOTION_TYPE);
            c->GetMotionMaster()->MoveRandom(ps.superuniquePatrolRadius);
        }
        Arpg::Settings const& s = sArpgConfig.Get();
        int effective = Arpg::Core::LootLevel(int(p->GetLevel()), Arpg::Loot::ZoneLevelsFor(zone, c->GetAreaId()), s.scaleZoneCapAbove, false);
        uint8 level = uint8(std::max<int>(c->GetLevel(), Arpg::Core::MonsterLevel(band, effective, int(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)))));
        Arpg::Scaling::Pin(c, level, band.healthMult, band.damageMult);
        if (s.superuniqueScale > 0.0f)
            c->SetObjectScale(c->GetObjectScale() * s.superuniqueScale);
        Live live;
        live.id = id;
        live.guid = c->GetGUID().GetRawValue();
        live.band = band;
        int count = band.affixMin + int(Rand(uint32(std::max(0, band.affixMax - band.affixMin) + 1)));
        for (Ability const& a : kit)
            if (int(live.abilities.size()) < count && a.minLevel <= p->GetLevel())
                live.abilities.push_back(a);
        live.lastUse.assign(live.abilities.size(), 0u);    // 0 = ready: the first ability goes off as the fight starts
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_zones[zone].alive[id] = live;
            g_byGuid[live.guid] = { zone, id };
        }
        out = "superunique id=" + std::to_string(id) + " entry=" + std::to_string(def.entry) + " level=" + std::to_string(level) +
            " band=" + std::to_string(band.minPlayerLevel) + " hp=" + std::to_string(c->GetMaxHealth()) + " abilities=" +
            std::to_string(live.abilities.size());
        Rec(p, "spawn " + out);
        // Zone announce (m3 3.4): everyone in the zone learns a Superunique is about.
        if (sArpgConfig.Get().superuniqueAnnounce)
        {
            std::string text = "|cffff8000" + c->GetName() + "|r stalks " + (c->GetAreaId() && sAreaTableStore.LookupEntry(c->GetAreaId())
                ? std::string(sAreaTableStore.LookupEntry(c->GetAreaId())->area_name) : std::string("the zone")) + "!";
            sWorld->SendZoneText(zone, text.c_str());
            Rec(p, "superunique announce " + text);
        }
        return true;
    }

    uint32 PickNext(uint32 zone)
    {
        std::vector<Arpg::Core::SuperuniqueCandidate> pool;
        std::vector<uint32> alive;
        uint32 last = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            for (uint32 id : g_pools[zone])
                pool.push_back({ id, g_defs[id].weight });
            ZoneState& z = g_zones[zone];
            for (auto const& kv : z.alive)
                alive.push_back(kv.first);
            last = z.lastKilled;
        }
        return Arpg::Core::NextSuperunique(pool, last, alive, &Rand);
    }

    bool RandomPoint(uint32 id, uint32 mapId, Position& pos)
    {
        std::lock_guard<std::mutex> l(g_lock);
        std::vector<Point> pts;
        for (Point const& pt : g_points[id])
            if (pt.map == mapId)
                pts.push_back(pt);
        if (pts.empty())
            return false;
        Point const& pt = pts[Rand(uint32(pts.size()))];
        pos.Relocate(pt.x, pt.y, pt.z, pt.o);
        return true;
    }

    void Despawn(Player* p, uint64 guid)
    {
        if (Creature* c = ObjectAccessor::GetCreature(*p, ObjectGuid(guid)))
            if (c->IsAlive())
            {
                c->setActive(false, ActiveFlags::Script);
                c->DespawnOrUnsummon();
            }
        Arpg::Scaling::Forget(guid);
    }

    // Per real player, once a second: keep the player's zone populated, despawn the ones of zones on this map nobody is in,
    // and run the abilities of a Superunique fighting this player.
    void Tick(Player* p)
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        if (!s.superuniqueEnabled || p->GetMap()->Instanceable())
            return;
        uint32 now = getMSTime();
        uint32 key = p->GetGUID().GetCounter();
        uint32 zone = p->GetZoneId();
        int target = 0;
        int aliveCount = 0;
        bool due = false;
        std::vector<std::pair<uint32, uint64>> check;    // (zone, guid) of everything we track on this map
        {
            std::lock_guard<std::mutex> l(g_lock);
            uint32& last = g_lastTick[key];
            if (last && getMSTimeDiff(last, now) < 1000)
                return;
            last = now;
            auto pool = g_pools.find(zone);
            if (pool != g_pools.end())
            {
                std::vector<Arpg::Core::SuperuniqueCandidate> c;
                for (uint32 id : pool->second)
                    c.push_back({ id, g_defs[id].weight });
                target = Arpg::Core::TargetAliveCount(c, s.superuniquePerZone);
                ZoneState& z = g_zones[zone];
                z.lastPlayerSeen = now;
                aliveCount = int(z.alive.size());
                due = z.respawnAt == 0 || int32(now - z.respawnAt) >= 0;
            }
            for (auto& zs : g_zones)
                for (auto& kv : zs.second.alive)
                    check.push_back({ zs.first, kv.second.guid });
        }
        // Drop what vanished (despawned, grid unloaded, killed without a player kill) and empty zones nobody visits.
        for (auto const& zc : check)
        {
            Creature* c = ObjectAccessor::GetCreature(*p, ObjectGuid(zc.second));
            bool gone = !c || !c->IsAlive();
            bool abandoned = false;
            {
                std::lock_guard<std::mutex> l(g_lock);
                ZoneState& z = g_zones[zc.first];
                abandoned = zc.first != zone && z.lastPlayerSeen && getMSTimeDiff(z.lastPlayerSeen, now) > 120000;
            }
            if (c && c->GetMapId() != p->GetMapId())
                continue;
            if (gone || abandoned)
            {
                if (abandoned && !gone)
                    Despawn(p, zc.second);
                std::lock_guard<std::mutex> l(g_lock);
                auto it = g_byGuid.find(zc.second);
                if (it != g_byGuid.end())
                {
                    ZoneState& gz = g_zones[it->second.first];
                    gz.alive.erase(it->second.second);
                    if (gone)   // killed by a guard/bot/other creature: same rotation and delay as a player kill
                    {
                        gz.lastKilled = it->second.second;
                        gz.respawnAt = now + sArpgConfig.Get().superuniqueRespawnDelaySec * IN_MILLISECONDS;
                    }
                    g_byGuid.erase(it);
                }
            }
        }
        {
            std::lock_guard<std::mutex> l(g_lock);
            aliveCount = int(g_zones[zone].alive.size());
        }
        if (target > 0 && aliveCount < target && due)
            if (uint32 id = PickNext(zone))
            {
                Position pos;
                std::string out;
                if (RandomPoint(id, p->GetMapId(), pos) && Spawn(p, zone, id, pos, out))
                {
                    std::lock_guard<std::mutex> l(g_lock);
                    g_zones[zone].respawnAt = 0;
                }
            }
        // Abilities: one per tick for each Superunique fighting this player.
        std::vector<Live> fighting;
        {
            std::lock_guard<std::mutex> l(g_lock);
            for (auto& zs : g_zones)
                for (auto& kv : zs.second.alive)
                    fighting.push_back(kv.second);
        }
        for (Live const& lv : fighting)
        {
            Creature* c = ObjectAccessor::GetCreature(*p, ObjectGuid(lv.guid));
            // Abilities go at whoever holds its aggro (the player, a bot tank in the party, a pet, a guard), as long as a real
            // player is near (no fights run in empty zones).
            Unit* target = c ? c->GetVictim() : nullptr;
            if (!c || !c->IsAlive() || !target || !target->IsAlive() || !c->IsWithinDistInMap(p, 100.0f) ||
                !c->IsWithinDistInMap(target, 40.0f) || c->HasUnitState(UNIT_STATE_CASTING))
                continue;
            for (size_t i = 0; i < lv.abilities.size(); ++i)
            {
                Ability const& a = lv.abilities[i];
                if (lv.lastUse[i] && getMSTimeDiff(lv.lastUse[i], now) < a.everyMs)
                    continue;
                {
                    std::lock_guard<std::mutex> l(g_lock);
                    auto it = g_byGuid.find(lv.guid);
                    if (it != g_byGuid.end())
                        g_zones[it->second.first].alive[it->second.second].lastUse[i] = now;
                }
                // Casts keep their cast time (interruptible); instants are fully triggered (no weapon/range checks).
                c->CastSpell(target, a.spell, a.kind == 0 ? TRIGGERED_FULL_MASK : TRIGGERED_IGNORE_POWER_AND_REAGENT_COST);
                Rec(p, "superunique ability spell=" + std::to_string(a.spell) + " kind=" + std::to_string(a.kind) + " casting=" +
                    (c->HasUnitState(UNIT_STATE_CASTING) ? "1" : "0") + " target=" + (target == p ? "me" :
                    target->GetTypeId() == TYPEID_PLAYER ? "player" : "creature"));
                break;
            }
        }
    }

    bool LiveOf(Unit* u, Live& out)
    {
        Creature* c = u ? u->ToCreature() : nullptr;
        if (!c)
            return false;
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_byGuid.find(c->GetGUID().GetRawValue());
        if (it == g_byGuid.end())
            return false;
        out = g_zones[it->second.first].alive[it->second.second];
        return true;
    }

    class ArpgSuperuniqueUnitScript : public UnitScript
    {
    public:
        ArpgSuperuniqueUnitScript() : UnitScript("arpg_superuniques_unit") { }

        // Kit abilities hit for a % of the target's max health (the spell's own number is ignored).
        void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
        {
            if (!spellInfo || !target)
                return;
            Live lv;
            if (!LiveOf(attacker, lv))
                return;
            for (Ability const& a : lv.abilities)
                if (a.spell == spellInfo->Id)
                {
                    uint32 base = Arpg::Core::AffixDamage(lv.band, target->GetMaxHealth());
                    damage = int32(std::min<uint64>(uint64(base) * a.damagePct / 100, target->GetMaxHealth() - 1));
                    if (Player* tp = target->ToPlayer())   // bots are players too: a party tank sees the same % hit
                        Rec(tp, "superunique hit spell=" + std::to_string(a.spell) + " dmg=" + std::to_string(damage));
                    return;
                }
        }
    };

    class ArpgSuperuniquePlayerScript : public PlayerScript
    {
    public:
        ArpgSuperuniquePlayerScript() : PlayerScript("arpg_superuniques") { }

        void OnUpdate(Player* player, uint32 /*diff*/) override
        {
            if (player->IsInWorld() && Arpg::Scaling::IsRealPlayer(player))
                Tick(player);
        }

        void OnLogout(Player* player) override
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_lastTick.erase(player->GetGUID().GetCounter());
        }

        void OnCreatureKill(Player* killer, Creature* killed) override
        {
            if (!killer || !killed)
                return;
            uint64 raw = killed->GetGUID().GetRawValue();
            Live lv;
            uint32 zone = 0;
            {
                std::lock_guard<std::mutex> l(g_lock);
                auto it = g_byGuid.find(raw);
                if (it == g_byGuid.end())
                    return;
                zone = it->second.first;
                ZoneState& z = g_zones[zone];
                lv = z.alive[it->second.second];
                z.alive.erase(it->second.second);
                z.lastKilled = lv.id;
                z.respawnAt = getMSTime() + sArpgConfig.Get().superuniqueRespawnDelaySec * IN_MILLISECONDS;
                g_byGuid.erase(it);
                g_lastKilledBy[killer->GetGUID().GetCounter()] = raw;
            }
            Arpg::Scaling::Forget(raw);
            killed->setActive(false, ActiveFlags::Script);
            Arpg::Settings const& s = sArpgConfig.Get();
            // Health orb (40%).
            uint32 heal = Arpg::Core::OrbAmount(lv.band, killer->GetMaxHealth());
            killer->ModifyHealth(int32(heal));
            Powers pt = killer->GetPowerType();
            killer->ModifyPower(pt, int32(Arpg::Core::OrbAmount(lv.band, uint32(std::max(0, killer->GetMaxPower(pt))))));
            // Guaranteed blue.
            uint32 blue = Arpg::Monsters::FloorItem(killer, ITEM_QUALITY_RARE);
            if (blue)
                Arpg::Monsters::AddLootItem(killed->loot, blue);
            // Unique ladder per character and Superunique (creature entry): 10% + 5% per kill without one.
            uint32 const baseEntry = Arpg::Monsters::BaseEntry(killed->GetEntry());   // pity is per original creature
            int misses = Arpg::Loot::UniqueMisses(killer, baseEntry);
            Arpg::Core::PityLadder ladder;
            ladder.base = s.superuniqueUniqueChancePct / 100.0f;
            ladder.step = s.superuniqueUniqueStepPct / 100.0f;
            bool won = Arpg::Core::RollWithPity(ladder, misses, rand_norm());
            Arpg::Loot::SetUniqueMisses(killer, baseEntry, misses);
            uint32 uniqueBase = 0;
            if (won)
            {
                // The Unique sits on a base of its slot added to the loot; picking it up turns it into the Unique.
                uint32 uid = 0;
                {
                    std::lock_guard<std::mutex> l(g_lock);
                    auto d = g_defs.find(lv.id);
                    uid = d != g_defs.end() ? d->second.uniqueId : 0;
                }
                if (!uid)
                    uid = Arpg::Loot::RandomUniqueId();
                uint32 inv = 0, armor = 0;
                if (uid && Arpg::Loot::UniqueSlot(uid, inv, armor))
                    uniqueBase = Arpg::Loot::FindBaseItem(inv, armor, ITEM_QUALITY_RARE, Arpg::Loot::LootLevelFor(killer));
                if (uniqueBase)
                {
                    Arpg::Monsters::AddLootItem(killed->loot, uniqueBase);
                    Arpg::Loot::SetPendingUnique(killer, killed->GetEntry(), uid);   // matched against the loot's source (tag entry)
                }
            }
            if (blue || uniqueBase)
                killed->SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            Rec(killer, "superunique killed id=" + std::to_string(lv.id) + " entry=" + std::to_string(baseEntry) + " tag=" + killed->GetName() +
                " orb_health=" + std::to_string(heal) + " floor_blue=" + std::to_string(blue) + " unique " + (won ? "won" : "missed") +
                " misses=" + std::to_string(misses) + " unique_base=" + std::to_string(uniqueBase));
        }
    };
}

namespace Arpg::Monsters
{
    std::string LoadSuperuniqueData()
    {
        std::unordered_map<uint32, Def> defs;
        std::unordered_map<uint32, std::vector<uint32>> pools;
        std::unordered_map<uint32, std::vector<Point>> points;
        std::unordered_map<uint32, std::vector<Ability>> kits;
        std::vector<Core::MonsterBand> bands;
        if (QueryResult r = WorldDatabase.Query("SELECT id, zone_id, creature_entry, kit_id, weight FROM arpg_superunique"))
            do
            {
                Field* f = r->Fetch();
                Def d{ f[0].GetUInt32(), f[1].GetUInt32(), f[2].GetUInt32(), f[3].GetUInt32(), f[4].GetUInt32(), 0 };
                if (!sObjectMgr->GetCreatureTemplate(d.entry))
                    continue;
                defs[d.id] = d;
                pools[d.zone].push_back(d.id);
            } while (r->NextRow());
        // Each Superunique's own Unique (arpg_superunique_unique); without a row it drops a random one.
        if (QueryResult r = WorldDatabase.Query("SELECT superunique_id, unique_id FROM arpg_superunique_unique"))
            do
            {
                Field* f = r->Fetch();
                auto it = defs.find(f[0].GetUInt32());
                if (it != defs.end())
                    it->second.uniqueId = f[1].GetUInt32();
            } while (r->NextRow());
        if (QueryResult r = WorldDatabase.Query("SELECT superunique_id, map, x, y, z, o FROM arpg_superunique_spawn"))
            do
            {
                Field* f = r->Fetch();
                points[f[0].GetUInt32()].push_back({ f[1].GetUInt16(), f[2].GetFloat(), f[3].GetFloat(), f[4].GetFloat(), f[5].GetFloat() });
            } while (r->NextRow());
        if (QueryResult r = WorldDatabase.Query("SELECT kit_id, spell_id, kind, every_ms, damage_pct, min_player_level FROM arpg_superunique_ability ORDER BY kit_id, slot"))
            do
            {
                Field* f = r->Fetch();
                kits[f[0].GetUInt32()].push_back({ f[1].GetUInt32(), f[2].GetUInt8(), f[3].GetUInt32(), f[4].GetUInt16(), f[5].GetUInt8() });
            } while (r->NextRow());
        if (QueryResult r = WorldDatabase.Query("SELECT kind, min_player_level, level_offset, health_mult, damage_mult, affix_min, affix_max, "
            "minions_min, minions_max, orb_pct, affix_damage_pct FROM arpg_monster_band WHERE kind = 1"))
            do
            {
                Field* f = r->Fetch();
                Core::MonsterBand b;
                b.kind = Core::MonsterKind(f[0].GetUInt8());
                b.minPlayerLevel = f[1].GetUInt8();
                b.levelOffset = f[2].GetInt8();
                b.healthMult = f[3].GetFloat();
                b.damageMult = f[4].GetFloat();
                b.affixMin = f[5].GetUInt8();
                b.affixMax = f[6].GetUInt8();
                b.minionsMin = f[7].GetUInt8();
                b.minionsMax = f[8].GetUInt8();
                b.orbPct = f[9].GetUInt8();
                b.affixDamagePct = f[10].GetUInt8();
                bands.push_back(b);
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_defs = defs;
        g_pools = pools;
        g_points = points;
        g_kits = kits;
        g_bands = bands;
        return std::to_string(defs.size()) + " superuniques in " + std::to_string(pools.size()) + " zones";
    }

    void InstallSuperuniques()
    {
        new ArpgSuperuniquePlayerScript();
        new ArpgSuperuniqueUnitScript();
    }

    bool SpawnSuperuniqueNear(Player* player, uint32 id, std::string& out)
    {
        uint32 zone = player->GetZoneId();
        std::vector<uint64> old;
        {
            std::lock_guard<std::mutex> l(g_lock);
            if (!g_pools.count(zone)) { out = "no superunique pool for zone " + std::to_string(zone); return false; }
            if (id && (!g_defs.count(id) || g_defs[id].zone != zone)) { out = "superunique " + std::to_string(id) + " is not in zone " + std::to_string(zone); return false; }
            for (auto const& kv : g_zones[zone].alive)
                old.push_back(kv.second.guid);
        }
        for (uint64 g : old)
        {
            Despawn(player, g);
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_byGuid.find(g);
            if (it != g_byGuid.end())
            {
                g_zones[zone].alive.erase(it->second.second);
                g_byGuid.erase(it);
            }
        }
        if (!id)
            id = PickNext(zone);
        Position pos = player->GetNearPosition(5.0f, 0.0f);
        return Spawn(player, zone, id, pos, out);
    }

    void SkipSuperuniqueDelay(Player* player)
    {
        std::lock_guard<std::mutex> l(g_lock);
        g_zones[player->GetZoneId()].respawnAt = 0;
    }

    Creature* ZoneSuperunique(Player* player)
    {
        uint64 guid = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto z = g_zones.find(player->GetZoneId());
            if (z == g_zones.end() || z->second.alive.empty())
                return nullptr;
            guid = z->second.alive.begin()->second.guid;
        }
        return ObjectAccessor::GetCreature(*player, ObjectGuid(guid));
    }

    Creature* LastKilledSuperunique(Player* player)
    {
        uint64 guid = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_lastKilledBy.find(player->GetGUID().GetCounter());
            if (it == g_lastKilledBy.end())
                return nullptr;
            guid = it->second;
        }
        return ObjectAccessor::GetCreature(*player, ObjectGuid(guid));
    }

    bool IsNativeSuperuniqueRare(Creature* c)
    {
        if (!c || c->IsSummon() || !sArpgConfig.Get().superuniqueEnabled)
            return false;
        std::lock_guard<std::mutex> l(g_lock);
        if (g_byGuid.count(c->GetGUID().GetRawValue()))
            return false;
        for (auto const& kv : g_defs)
            if (kv.second.entry == c->GetEntry())
                return true;
        return false;
    }

    std::string DescribeSuperuniques(Player* player)
    {
        uint32 zone = player->GetZoneId();
        std::lock_guard<std::mutex> l(g_lock);
        auto pool = g_pools.find(zone);
        ZoneState& z = g_zones[zone];
        uint32 now = getMSTime();
        std::string out = "superunique zone=" + std::to_string(zone) + " pool=" + std::to_string(pool == g_pools.end() ? 0 : pool->second.size()) +
            " alive=" + std::to_string(z.alive.size());
        for (auto const& kv : z.alive)
        {
            out += " [id=" + std::to_string(kv.first) + " entry=" + std::to_string(g_defs[kv.first].entry) + " abilities=" +
                std::to_string(kv.second.abilities.size());
            if (Creature* c = ObjectAccessor::GetCreature(*player, ObjectGuid(kv.second.guid)))
                out += " level=" + std::to_string(c->GetLevel()) + " hp=" + std::to_string(c->GetMaxHealth()) + " dist=" +
                    std::to_string(int(player->GetDistance(c))) + " combat=" + (c->IsInCombat() ? "1" : "0") + " victim=" +
                    (c->GetVictim() ? (c->GetVictim() == player ? "me" : "other") : "none") + " casting=" +
                    (c->HasUnitState(UNIT_STATE_CASTING) ? "1" : "0") + " ai=" + c->GetAIName();
            out += "]";
        }
        int32 left = z.respawnAt ? int32(z.respawnAt - now) : 0;
        out += " respawn_in=" + std::to_string(std::max(0, left / 1000)) + " last_killed=" + std::to_string(z.lastKilled);
        return out;
    }
}
