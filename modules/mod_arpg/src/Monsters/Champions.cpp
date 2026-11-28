#include "Monsters/Champions.h"
#include "Loot/PersonalLoot.h"

#include "ArpgConfig.h"
#include "Core/LootSubstitution.h"
#include "Core/MonsterBands.h"
#include "Core/ChampionCamps.h"
#include "Loot/Substitution.h"
#include "Loot/ZoneLevels.h"
#include "Scaling/WorldScaling.h"
#include "Core/Scaling.h"
#include "Monsters/Superuniques.h"
#include "SpellInfo.h"
#include "Timer.h"
#include "Test/TestEvents.h"

#include "Creature.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "LootMgr.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include "World.h"
#include "CellImpl.h"
#include "GameTime.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"

#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace
{
    struct AffixMeta { std::string name, effect; };

    struct ChampionInfo
    {
        Arpg::Core::MonsterBand band;
        std::vector<uint32> affixes;
        std::vector<uint64> minions;
        uint32 level = 0;
        std::vector<std::string> effects;   // arpg_monster_affix.effect of each picked affix
        uint32 lastNuke = 0;                 // getMSTime of the last Arcane Caster cast
        bool Has(char const* e) const { return std::find(effects.begin(), effects.end(), e) != effects.end(); }
    };

    struct PendingExplosion { uint32 mapId; float x, y, z; uint32 at; uint32 amount; uint32 entry; };

    std::mutex g_lock;
    std::vector<Arpg::Core::MonsterBand> g_bands;
    std::vector<Arpg::Core::MonsterAffixDef> g_affixPool;
    std::unordered_map<uint32, AffixMeta> g_affixMeta;
    std::unordered_map<uint64, ChampionInfo> g_champions;   // creature guid -> Champion (alive)
    std::unordered_set<uint64> g_killedChampions;            // dead Champions whose loot is still up (tier +1 on pickup)
    std::unordered_map<uint32, uint64> g_lastChampion;       // player guid -> last Champion made for / killed by them (tests)
    std::unordered_map<uint32, std::vector<PendingExplosion>> g_explosions;   // player guid -> Fire Enchanted blasts to resolve
    std::unordered_map<uint32, uint32> g_lastPlayerTick;     // player guid -> getMSTime of the last affix tick
    thread_local int t_depth = 0;                            // our own damage must not trigger affixes again

    // Champion camps (m9-champion-camps): square key -> its Champion (creature guid, 0 = none) and when it may get a new one.
    struct Camp { uint64 champion = 0; uint32 readyAtS = 0; };
    std::unordered_map<uint64, Camp> g_camps;
    std::unordered_set<uint32> g_excluded;                   // arpg_champion_exclude: entries that never become Champions

    struct Guard { Guard() { ++t_depth; } ~Guard() { --t_depth; } };

    bool ChampionOf(Unit* u, ChampionInfo& out)
    {
        Creature* c = u ? u->ToCreature() : nullptr;
        if (!c)
            return false;
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_champions.find(c->GetGUID().GetRawValue());
        if (it == g_champions.end())
            return false;
        out = it->second;
        return true;
    }

    void Rec(Player* p, std::string const& line)
    {
        if (p && Arpg::TestEvents::Enabled())
            Arpg::TestEvents::Record(p->GetGUID().GetCounter(), line);
    }

    // Damage from a Champion affix: a spell-school hit with a combat log line (the source may be dead: then self-sourced).
    void AffixHit(Unit* source, Player* victim, uint32 amount, uint32 spellId, SpellSchoolMask school)
    {
        if (!amount || !victim || !victim->IsAlive())
            return;
        Guard g;
        Unit* from = source && source->IsAlive() ? source : victim;
        SpellNonMeleeDamage d(from, victim, spellId, school);
        d.damage = std::min<uint32>(amount, victim->GetHealth() > 1 ? victim->GetHealth() - 1 : 0);   // affixes never kill outright
        if (!d.damage)
            return;
        from->DealSpellDamage(&d, false);
        from->SendSpellNonMeleeDamageLog(&d);
    }

    uint32 Rand(uint32 n) { return n ? urand(0, n - 1) : 0u; }

}

namespace Arpg::Monsters
{
    // Armour type the player's class wears (mail/plate from level 40, as the trainers teach it).
    uint32 ArmorSubclassFor(Player* p)
    {
        bool forty = p->GetLevel() >= 40;
        switch (p->GetClass())
        {
            case CLASS_WARRIOR: case CLASS_PALADIN: case CLASS_DEATH_KNIGHT: return forty ? ITEM_SUBCLASS_ARMOR_PLATE : ITEM_SUBCLASS_ARMOR_MAIL;
            case CLASS_HUNTER: case CLASS_SHAMAN: return forty ? ITEM_SUBCLASS_ARMOR_MAIL : ITEM_SUBCLASS_ARMOR_LEATHER;
            case CLASS_ROGUE: case CLASS_DRUID: case CLASS_MONK: return ITEM_SUBCLASS_ARMOR_LEATHER;
            default: return ITEM_SUBCLASS_ARMOR_CLOTH;
        }
    }

    // A real, droppable item of a random slot at the loot level (armour of the class's type; jewellery/cloak/weapons any).
    uint32 FloorItem(Player* p, uint32 quality)
    {
        static uint32 const armorSlots[] = { INVTYPE_HEAD, INVTYPE_SHOULDERS, INVTYPE_CHEST, INVTYPE_WAIST, INVTYPE_LEGS,
                                             INVTYPE_FEET, INVTYPE_WRISTS, INVTYPE_HANDS };
        static uint32 const anySlots[] = { INVTYPE_NECK, INVTYPE_FINGER, INVTYPE_CLOAK, INVTYPE_WEAPON, INVTYPE_2HWEAPON };
        int level = Arpg::Loot::LootLevelFor(p);
        auto tryAt = [&](uint32 q, int lvl) -> uint32
        {
            for (int attempt = 0; attempt < 8; ++attempt)
            {
                uint32 pick = Rand(13);
                uint32 id = pick < 8 ? Arpg::Loot::FindBaseItem(armorSlots[pick], ArmorSubclassFor(p), q, lvl)
                                     : Arpg::Loot::FindBaseItem(anySlots[pick - 8], 0, q, lvl);
                if (id)
                    return id;
            }
            return 0;
        };
        if (uint32 id = tryAt(quality, level))
            return id;
        // Low levels have hardly any blues (owner 2026-11-27: a level-4 Superunique kill dropped only a catalyst): one
        // quality lower at this level, else the asked quality a little higher up (wearable soon).
        if (quality > ITEM_QUALITY_UNCOMMON)
            if (uint32 id = tryAt(quality - 1, level))
                return id;
        for (int up = 1; up <= 10; ++up)
            if (uint32 id = tryAt(quality, level + up))
                return id;
        return 0;
    }

    void AddLootItem(::Loot& loot, uint32 itemId)
    {
        LootItem li;
        li.itemid = itemId;
        li.count = 1;
        li.is_world_drop = false;
        li.personal = false;
        loot.items.push_back(li);
        ++loot.unlootedCount;
    }
}

namespace
{
    using Arpg::Monsters::FloorItem;
    using Arpg::Monsters::AddLootItem;

    void ApplySoftAffix(Creature* c, std::string const& effect)
    {
        if (effect == "damage_up")
            for (WeaponAttackType t : { BASE_ATTACK, OFF_ATTACK, RANGED_ATTACK })
            {
                c->SetBaseWeaponDamage(t, MINDAMAGE, c->GetWeaponDamageRange(t, MINDAMAGE) * 1.25f);
                c->SetBaseWeaponDamage(t, MAXDAMAGE, c->GetWeaponDamageRange(t, MAXDAMAGE) * 1.25f);
            }
        else if (effect == "speed_up")
        {
            c->SetSpeed(MOVE_RUN, c->GetSpeedRate(MOVE_RUN) * 1.3f, true);
            c->SetAttackTime(BASE_ATTACK, uint32(c->GetAttackTime(BASE_ATTACK) * 0.8f));
        }
        else if (effect == "armor_up")
            c->SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, c->GetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE) * 2.0f);
        c->UpdateAllStats();
        c->SetHealth(c->GetMaxHealth());
    }

    // Never Champions (owner 2026-11-28: quest NPCs became Champions): NPCs with a menu or quest, scripted story NPCs,
    // monsters friendly to the player (sparring partners), rares/bosses, Superunique rares, arpg_champion_exclude.
    bool ChampionEligible(Creature* c, Player* p)
    {
        if (!c || !c->IsAlive() || c->IsInCombat() || c->IsSummon() || !p->IsValidAttackTarget(c))
            return false;
        CreatureTemplate const* t = c->GetCreatureTemplate();
        if (!t || t->rank > CREATURE_ELITE_ELITE || c->IsDungeonBoss() || t->type == CREATURE_TYPE_CRITTER)
            return false;
        if (t->npcflag || t->npcflag2 || c->GetUInt32Value(UNIT_FIELD_NPC_FLAGS) || t->ScriptID)
            return false;
        if (p->GetReactionTo(c) >= REP_FRIENDLY || c->IsCivilian() || c->IsTrigger())
            return false;
        {
            std::lock_guard<std::mutex> l(g_lock);
            if (g_excluded.count(t->Entry) || g_champions.count(c->GetGUID().GetRawValue()))
                return false;
        }
        return !Arpg::Monsters::IsNativeSuperuniqueRare(c);
    }

    // A camp square without a live Champion (and past its respawn delay) with enough eligible monsters gets one: a random
    // eligible monster of the square of `c` (a monster, or the player for tests). Returns true when c itself became it.
    bool TryCampChampion(WorldObject* c, Player* p, Arpg::Settings const& s, std::string* info = nullptr)
    {
        float const size = s.m9.campSize;
        uint64 const key = Arpg::Core::CampKey(c->GetMapId(), c->GetPositionX(), c->GetPositionY(), size);
        uint32 const nowS = uint32(GameTime::GetGameTime());
        bool live;
        uint32 readyAt;
        {
            std::lock_guard<std::mutex> l(g_lock);
            Camp& camp = g_camps[key];
            if (camp.champion && !g_champions.count(camp.champion))
            {
                camp.champion = 0;   // gone (killed, despawned): the square waits CampRespawn seconds
                camp.readyAtS = nowS + s.m9.campRespawnS;
            }
            live = camp.champion != 0;
            readyAt = camp.readyAtS;
        }
        if (info)
            *info = "camp live=" + std::to_string(live ? 1 : 0) + " wait=" + std::to_string(readyAt > nowS ? readyAt - nowS : 0);
        if (live || nowS < readyAt)
            return false;
        std::list<Unit*> units;
        Trinity::AnyUnitInObjectRangeCheck check(c, size);
        Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(c, units, check);
        Cell::VisitAllObjects(c, searcher, size);
        std::vector<Creature*> eligible;
        for (Unit* u : units)
            if (Creature* m = u->ToCreature())
                if (Arpg::Core::CampKey(m->GetMapId(), m->GetPositionX(), m->GetPositionY(), size) == key && ChampionEligible(m, p))
                    eligible.push_back(m);
        if (info)
            *info += " eligible=" + std::to_string(eligible.size());
        if (!Arpg::Core::CampWantsChampion(false, nowS, readyAt, int(eligible.size()), s.m9.campMin))
            return false;
        Creature* pick = eligible[urand(0, uint32(eligible.size() - 1))];
        std::string out;
        if (!Arpg::Monsters::MakeChampion(pick, p, out))
            return false;
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_camps[key].champion = pick->GetGUID().GetRawValue();
        }
        Arpg::TestEvents::Record(p->GetGUID().GetCounter(), "champion camp entry=" + std::to_string(pick->GetEntry()) +
            " eligible=" + std::to_string(eligible.size()));
        if (info)
            *info += " made=1 champion_entry=" + std::to_string(Arpg::Monsters::BaseEntry(pick->GetEntry()));
        return pick == c->ToCreature();
    }

    bool RollOnFirstSight(Creature* c, Player* p)
    {
        // A rare that is in a Superunique pool: the zone shows only the Superunique version (no duplicates).
        if (Arpg::Monsters::IsNativeSuperuniqueRare(c))
        {
            Arpg::TestEvents::Record(p->GetGUID().GetCounter(), "superunique native despawned entry=" + std::to_string(c->GetEntry()));
            c->DespawnOrUnsummon();
            return true;
        }
        Arpg::Settings const& s = sArpgConfig.Get();
        if (!s.championEnabled || p->GetMap()->Instanceable() || !ChampionEligible(c, p))
            return false;   // rares, bosses, quest/story NPCs keep their own identity
        if (s.m9.campsEnabled && TryCampChampion(c, p, s))
            return true;
        if (s.championChancePct <= 0.0f || float(rand_chance()) >= s.championChancePct)
            return false;
        std::string out;
        return Arpg::Monsters::MakeChampion(c, p, out);
    }

    class ArpgChampionUnitScript : public UnitScript
    {
    public:
        ArpgChampionUnitScript() : UnitScript("arpg_champions_unit") { }

        // A Champion's hit (melee or spell) on a player: Cold Enchanted slows, Lightning Enchanted sparks.
        void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
        {
            if (t_depth || !damage || !victim || victim->GetTypeId() != TYPEID_PLAYER)
                return;
            ChampionInfo info;
            if (!ChampionOf(attacker, info))
                return;
            Player* p = victim->ToPlayer();
            Arpg::Settings const& s = sArpgConfig.Get();
            if (info.Has("chill_on_hit") && s.championChillSpell)
            {
                Guard g;
                attacker->AddAura(s.championChillSpell, p);
                Rec(p, "champion chill spell=" + std::to_string(s.championChillSpell));
            }
            if (info.Has("sparks_on_hit") && (s.forceProcs || float(rand_chance()) < s.championSparkChancePct))
            {
                uint32 amount = std::max<uint32>(1, Arpg::Core::AffixDamage(info.band, p->GetMaxHealth()) / 3);
                AffixHit(attacker, p, amount, 421, SPELL_SCHOOL_MASK_NATURE);
                Rec(p, "champion sparks dmg=" + std::to_string(amount));
            }
        }

        // Arcane Caster: the nuke's damage is a % of the target's max health (never the spell's own number).
        void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
        {
            if (!spellInfo || !target || spellInfo->Id != sArpgConfig.Get().championNukeSpell)
                return;
            ChampionInfo info;
            if (!ChampionOf(attacker, info) || !info.Has("interruptible_nuke"))
                return;
            damage = int32(Arpg::Core::AffixDamage(info.band, target->GetMaxHealth()));
            Rec(target->ToPlayer(), "champion nuke dmg=" + std::to_string(damage));   // Rec ignores non-players
        }
    };

    // Per player, twice a second: resolve Fire Enchanted blasts (dodgeable: only if still within the radius when it goes
    // off) and let Arcane Casters fighting this player start their interruptible cast.
    void AffixTick(Player* p)
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        uint32 now = getMSTime();
        uint32 key = p->GetGUID().GetCounter();
        std::vector<PendingExplosion> due;
        std::vector<std::pair<uint64, ChampionInfo>> casters;
        {
            std::lock_guard<std::mutex> l(g_lock);
            uint32& last = g_lastPlayerTick[key];
            if (last && getMSTimeDiff(last, now) < 500)
                return;
            last = now;
            auto it = g_explosions.find(key);
            if (it != g_explosions.end())
            {
                auto& v = it->second;
                for (auto e = v.begin(); e != v.end();)
                    if (int32(now - e->at) >= 0) { due.push_back(*e); e = v.erase(e); }
                    else ++e;
                if (v.empty())
                    g_explosions.erase(it);
            }
            for (auto const& kv : g_champions)
                if (kv.second.Has("interruptible_nuke"))
                    casters.push_back(kv);
        }
        for (PendingExplosion const& e : due)
        {
            if (p->GetMapId() == e.mapId && p->GetExactDist(e.x, e.y, e.z) <= s.championExplosionRadius)
            {
                AffixHit(nullptr, p, e.amount, 1535, SPELL_SCHOOL_MASK_FIRE);
                Rec(p, "champion explosion dmg=" + std::to_string(e.amount));
            }
            else
                Rec(p, "champion explosion dodged");
        }
        if (!s.championNukeSpell)
            return;
        for (auto const& kv : casters)
        {
            Creature* c = ObjectAccessor::GetCreature(*p, ObjectGuid(kv.first));
            // Casts at whoever holds its aggro (player, bot tank, pet), while a real player is near.
            Unit* target = c ? c->GetVictim() : nullptr;
            if (!c || !c->IsAlive() || !target || !target->IsAlive() || !c->IsWithinDistInMap(p, 100.0f) || c->HasUnitState(UNIT_STATE_CASTING))
                continue;
            if (kv.second.lastNuke && getMSTimeDiff(kv.second.lastNuke, now) < s.championNukeEveryMs)
                continue;
            {
                std::lock_guard<std::mutex> l(g_lock);
                auto it = g_champions.find(kv.first);
                if (it != g_champions.end())
                    it->second.lastNuke = now;
            }
            c->CastSpell(target, s.championNukeSpell, TRIGGERED_IGNORE_POWER_AND_REAGENT_COST);
            Rec(p, "champion nuke cast entry=" + std::to_string(Arpg::Monsters::BaseEntry(c->GetEntry())) + " casting=" + (c->HasUnitState(UNIT_STATE_CASTING) ? "1" : "0"));
        }
    }

    class ArpgChampionPlayerScript : public PlayerScript
    {
    public:
        ArpgChampionPlayerScript() : PlayerScript("arpg_champions") { }

        void OnUpdate(Player* player, uint32 /*diff*/) override
        {
            if (player->IsInWorld() && Arpg::Scaling::IsRealPlayer(player))
                AffixTick(player);
        }

        void OnLogout(Player* player) override
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_explosions.erase(player->GetGUID().GetCounter());
            g_lastPlayerTick.erase(player->GetGUID().GetCounter());
        }

        void OnCreatureKill(Player* killer, Creature* killed) override
        {
            if (!killer || !killed)
                return;
            uint64 raw = killed->GetGUID().GetRawValue();
            ChampionInfo info;
            {
                std::lock_guard<std::mutex> l(g_lock);
                auto it = g_champions.find(raw);
                if (it == g_champions.end())
                {
                    Arpg::Scaling::Forget(raw);   // minion or plain creature: next life starts clean
                    return;
                }
                info = it->second;
                g_champions.erase(it);
                if (g_killedChampions.size() > 512)   // corpses despawn long before this many pile up
                    g_killedChampions.clear();
                g_killedChampions.insert(raw);
            }
            Arpg::Scaling::Forget(raw);
            if (info.Has("death_explosion"))
            {
                Arpg::Settings const& es = sArpgConfig.Get();
                PendingExplosion e{ killed->GetMapId(), killed->GetPositionX(), killed->GetPositionY(), killed->GetPositionZ(),
                    getMSTime() + es.championExplosionDelayMs, Arpg::Core::AffixDamage(info.band, killer->GetMaxHealth()), killed->GetEntry() };
                {
                    std::lock_guard<std::mutex> l(g_lock);
                    g_explosions[killer->GetGUID().GetCounter()].push_back(e);
                }
                Rec(killer, "champion explosion pending dmg=" + std::to_string(e.amount));
            }
            // Health orb: % of max health and of the main power, at once (BALANCE §14).
            uint32 heal = Arpg::Core::OrbAmount(info.band, killer->GetMaxHealth());
            killer->ModifyHealth(int32(heal));
            Powers pt = killer->GetPowerType();
            int32 power = int32(Arpg::Core::OrbAmount(info.band, uint32(std::max(0, killer->GetMaxPower(pt)))));
            killer->ModifyPower(pt, power);
            // Loot floor: at least one green, and a blue with Arpg.Champion.LootBlueChance %.
            Arpg::Settings const& s = sArpgConfig.Get();
            uint32 green = FloorItem(killer, ITEM_QUALITY_UNCOMMON);
            uint32 blue = float(rand_chance()) < s.championLootBlueChancePct ? FloorItem(killer, ITEM_QUALITY_RARE) : 0;
            if (green)
                AddLootItem(killed->loot, green);
            if (blue)
                AddLootItem(killed->loot, blue);
            if (green || blue)
                killed->SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            Arpg::TestEvents::Record(killer->GetGUID().GetCounter(), "champion killed entry=" + std::to_string(Arpg::Monsters::BaseEntry(killed->GetEntry())) +
                " orb_health=" + std::to_string(heal) + " orb_power=" + std::to_string(power) + " floor_green=" + std::to_string(green) +
                " floor_blue=" + std::to_string(blue));
            // m8: every other real player of the group nearby gets their own loot floor (personal loot).
            for (Player* m : Arpg::Loot::PersonalRecipients(killer, killed))
            {
                uint32 g = FloorItem(m, ITEM_QUALITY_UNCOMMON);
                uint32 b = float(rand_chance()) < s.championLootBlueChancePct ? FloorItem(m, ITEM_QUALITY_RARE) : 0;
                Arpg::Loot::AddPersonalItem(m, killed, g);
                Arpg::Loot::AddPersonalItem(m, killed, b);
                Arpg::TestEvents::Record(m->GetGUID().GetCounter(), "champion share entry=" + std::to_string(Arpg::Monsters::BaseEntry(killed->GetEntry())) +
                    " floor_green=" + std::to_string(g) + " floor_blue=" + std::to_string(b));
            }
        }
    };
}

namespace Arpg::Monsters
{
    std::string LoadChampionData()
    {
        std::vector<Core::MonsterBand> bands;
        std::vector<Core::MonsterAffixDef> pool;
        std::unordered_map<uint32, AffixMeta> meta;
        if (QueryResult r = WorldDatabase.Query("SELECT kind, min_player_level, level_offset, health_mult, damage_mult, affix_min, affix_max, "
            "minions_min, minions_max, orb_pct, affix_damage_pct FROM arpg_monster_band"))
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
        if (QueryResult r = WorldDatabase.Query("SELECT id, name, effect, unlock_level, weight, exclusive_group FROM arpg_monster_affix"))
            do
            {
                Field* f = r->Fetch();
                Core::MonsterAffixDef a;
                a.id = f[0].GetUInt32();
                a.unlockLevel = f[3].GetUInt8();
                a.weight = f[4].GetUInt32();
                a.exclusiveGroup = f[5].GetUInt32();
                pool.push_back(a);
                meta[a.id] = { f[1].GetString(), f[2].GetString() };
            } while (r->NextRow());
        std::unordered_set<uint32> excluded;
        if (QueryResult r = WorldDatabase.Query("SELECT entry FROM arpg_champion_exclude"))
            do
                excluded.insert(r->Fetch()[0].GetUInt32());
            while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_excluded = excluded;
        g_bands = bands;
        g_affixPool = pool;
        g_affixMeta = meta;
        return std::to_string(bands.size()) + " monster bands, " + std::to_string(pool.size()) + " monster affixes, " +
            std::to_string(excluded.size()) + " champion exclusions";
    }

    static Creature* NearestCreature(Player* p, float radius, uint32 entry, bool eligibleOnly)
    {
        std::list<Unit*> units;
        Trinity::AnyUnitInObjectRangeCheck check(p, radius);
        Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(p, units, check);
        Cell::VisitAllObjects(p, searcher, radius);
        Creature* best = nullptr;
        for (Unit* u : units)
            if (Creature* m = u->ToCreature())
                if ((!entry || BaseEntry(m->GetEntry()) == entry) && (!eligibleOnly || ChampionEligible(m, p)) &&
                    (!best || p->GetExactDist2d(m) < p->GetExactDist2d(best)))
                    best = m;
        return best;
    }

    static std::unordered_map<uint32, uint64>* g_probeAnchors = nullptr;
    static void CampProbeForget() { if (g_probeAnchors) g_probeAnchors->clear(); }

    std::string CampProbe(Player* p)
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        // Tests: the square of the nearest eligible monster at the first probe, kept until "reset" (that monster may have
        // become the Champion and died since; its corpse still marks the square).
        static std::unordered_map<uint32, uint64> anchors;   // player guid -> anchor creature guid (world thread only)
        g_probeAnchors = &anchors;
        WorldObject* anchor = nullptr;
        auto it = anchors.find(p->GetGUID().GetCounter());
        if (it != anchors.end())
            anchor = ObjectAccessor::GetCreature(*p, ObjectGuid(it->second));
        if (!anchor)
            if (Creature* c = NearestCreature(p, s.m9.campSize, 0, true))
            {
                anchors[p->GetGUID().GetCounter()] = c->GetGUID().GetRawValue();
                anchor = c;
            }
        if (!anchor)
            anchor = p;
        std::string info;
        TryCampChampion(anchor, p, s, &info);
        if (info.find("made=1") == std::string::npos)
            info += " made=0";
        return info;
    }

    std::string EligibleProbe(Player* p, uint32 entry)
    {
        Creature* c = NearestCreature(p, 100.0f, entry, false);
        if (!c)
            return "eligible none entry=" + std::to_string(entry);
        return "eligible entry=" + std::to_string(entry) + " ok=" + std::to_string(ChampionEligible(c, p) ? 1 : 0);
    }

    void ResetCamps()
    {
        CampProbeForget();
        std::lock_guard<std::mutex> l(g_lock);
        g_camps.clear();
    }

    void InstallChampions()
    {
        new ArpgChampionPlayerScript();
        new ArpgChampionUnitScript();
        Arpg::Scaling::SetNewCandidateHandler(&RollOnFirstSight);
    }

    bool MakeChampion(Creature* c, Player* player, std::string& out, std::vector<uint32> const& forcedAffixes)
    {
        if (!c || !player || !c->IsAlive())
        {
            out = "no creature";
            return false;
        }
        Core::MonsterBand band;
        std::vector<Core::MonsterAffixDef> pool;
        std::unordered_map<uint32, AffixMeta> meta;
        {
            std::lock_guard<std::mutex> l(g_lock);
            if (g_champions.count(c->GetGUID().GetRawValue()))
            {
                out = "already a champion";
                return false;
            }
            Core::MonsterBand const* b = Core::BandFor(g_bands, Core::MonsterKind::Champion, int(player->GetLevel()));
            if (!b)
            {
                out = "no champion band for level " + std::to_string(player->GetLevel());
                return false;
            }
            band = *b;
            pool = g_affixPool;
            meta = g_affixMeta;
        }
        Arpg::Settings const& s = sArpgConfig.Get();
        // Level: the player's (zone-capped, as scaling) level + the band offset, never below the creature's own level.
        int effective = Core::LootLevel(int(player->GetLevel()), Arpg::Loot::ZoneLevelsFor(c->GetZoneId(), c->GetAreaId()),
            s.scaleZoneCapAbove, false);
        int maxLevel = int(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
        uint8 level = uint8(std::max<int>(c->GetLevel(), Core::MonsterLevel(band, effective, maxLevel)));
        // Tag (owner 2026-11-22): its "Champion <name>" copy (Rare portrait); the respawn goes back to the original entry.
        uint32 baseEntry = c->GetEntry();
        if (s.championEntryOffset && baseEntry < s.championEntryOffset && sObjectMgr->GetCreatureTemplate(baseEntry + s.championEntryOffset))
            c->UpdateEntry(baseEntry + s.championEntryOffset);
        // Same cap as Superuniques: a template health multiplier above Arpg.Superunique.MaxHealthMod counts as that cap.
        float const maxMod = sArpgConfig.Get().m9.specialMaxHealthMod;
        float const modCap = (maxMod > 0.0f && c->GetCreatureTemplate()->ModHealth > maxMod) ? maxMod / c->GetCreatureTemplate()->ModHealth : 1.0f;
        Arpg::Scaling::Pin(c, level, band.healthMult * modCap, band.damageMult);
        ChampionInfo info;
        info.band = band;
        info.level = level;
        info.affixes = forcedAffixes.empty() ? Core::PickMonsterAffixes(pool, band, int(player->GetLevel()), &Rand) : forcedAffixes;
        for (uint32 id : info.affixes)
        {
            info.effects.push_back(meta[id].effect);
            ApplySoftAffix(c, meta[id].effect);
        }
        if (s.championScale > 0.0f)
            c->SetObjectScale(c->GetObjectScale() * s.championScale);
        if (s.championVisualAura)
            c->AddAura(s.championVisualAura, c);
        // Minions: the plain creature (no tag), same level, normal health; they follow the Champion.
        int minions = Core::MinionCount(band, &Rand);
        for (int i = 0; i < minions; ++i)
        {
            float angle = float(M_PI) * 2.0f * float(i) / float(std::max(1, minions));
            Position pos = c->GetNearPosition(3.0f, angle);
            if (TempSummon* m = c->SummonCreature(baseEntry, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, Milliseconds(120000)))
            {
                m->SetHomePosition(pos);
                // Minions are trash (owner 2026-11-28): the player's level minus 2-4 like other monsters, never below their own.
                uint8 const trash = Arpg::Core::ScaledLevel(sArpgConfig.Get().scale, m->GetLevel(), int(player->GetLevel()),
                    m->GetGUID().GetCounter());
                Arpg::Scaling::Pin(m, trash ? trash : m->GetLevel(), 1.0f, 1.0f);
                m->GetMotionMaster()->MoveFollow(c, 3.0f, angle);
                info.minions.push_back(m->GetGUID().GetRawValue());
            }
        }
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_champions[c->GetGUID().GetRawValue()] = info;
            g_lastChampion[player->GetGUID().GetCounter()] = c->GetGUID().GetRawValue();
        }
        out = DescribeChampion(c);
        Arpg::TestEvents::Record(player->GetGUID().GetCounter(), out);
        return true;
    }

    Creature* LastChampionOf(Player* player)
    {
        uint64 raw = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_lastChampion.find(player->GetGUID().GetCounter());
            if (it == g_lastChampion.end())
                return nullptr;
            raw = it->second;
        }
        return ObjectAccessor::GetCreature(*player, ObjectGuid(raw));
    }

    uint32 BaseEntry(uint32 entry)
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        if (s.superuniqueEntryOffset && entry >= s.superuniqueEntryOffset && entry < s.superuniqueEntryOffset + 300000)
            return entry - s.superuniqueEntryOffset;
        if (s.championEntryOffset && entry >= s.championEntryOffset && entry < s.championEntryOffset + 300000)
            return entry - s.championEntryOffset;
        return entry;
    }

    std::vector<uint64> ChampionMinions(Creature* c)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = c ? g_champions.find(c->GetGUID().GetRawValue()) : g_champions.end();
        return it == g_champions.end() ? std::vector<uint64>() : it->second.minions;
    }

    bool UnmakeChampion(Creature* c, std::string& out)
    {
        if (!c)
        {
            out = "no creature";
            return false;
        }
        uint64 raw = c->GetGUID().GetRawValue();
        ChampionInfo info;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_champions.find(raw);
            if (it == g_champions.end())
            {
                out = "not a champion";
                return true;   // nothing to undo
            }
            info = it->second;
            g_champions.erase(it);
        }
        for (uint64 m : info.minions)
            if (Creature* minion = ObjectAccessor::GetCreature(*c, ObjectGuid(m)))
                minion->DespawnOrUnsummon();
        Arpg::Settings const& s = sArpgConfig.Get();
        if (s.championVisualAura)
            c->RemoveAurasDueToSpell(s.championVisualAura);
        Arpg::Scaling::Forget(raw);                  // before the entry change: the level hook must not see the pin
        c->UpdateEntry(BaseEntry(c->GetEntry()));    // template level, health and size again
        c->SetObjectScale(c->GetCreatureTemplate() ? c->GetCreatureTemplate()->scale : 1.0f);
        out = "unmade champion entry=" + std::to_string(c->GetEntry()) + " level=" + std::to_string(c->GetLevel());
        return true;
    }

    bool IsChampion(Creature* c)
    {
        std::lock_guard<std::mutex> l(g_lock);
        return c && g_champions.count(c->GetGUID().GetRawValue());
    }

    bool WasChampionKill(Creature* c)
    {
        if (!c)
            return false;
        std::lock_guard<std::mutex> l(g_lock);
        return g_killedChampions.count(c->GetGUID().GetRawValue()) > 0;
    }

    bool IsLootingChampion(Player* player)
    {
        if (!player)
            return false;
        std::lock_guard<std::mutex> l(g_lock);
        return g_killedChampions.count(player->GetLootGUID().GetRawValue()) > 0;
    }

    std::string DescribeChampion(Creature* c)
    {
        if (!c)
            return "not a champion";
        ChampionInfo info;
        std::unordered_map<uint32, AffixMeta> meta;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_champions.find(c->GetGUID().GetRawValue());
            if (it == g_champions.end())
                return "not a champion";
            info = it->second;
            meta = g_affixMeta;
        }
        std::string names;
        for (uint32 id : info.affixes)
            names += (names.empty() ? "" : ",") + meta[id].name;
        return "champion entry=" + std::to_string(BaseEntry(c->GetEntry())) + " tag=" + c->GetName() + " level=" + std::to_string(c->GetLevel()) + " band=" +
            std::to_string(info.band.minPlayerLevel) + " hp=" + std::to_string(c->GetMaxHealth()) + " minions=" +
            std::to_string(info.minions.size()) + " affixes=" + std::to_string(info.affixes.size()) + " [" + names + "]";
    }
}
