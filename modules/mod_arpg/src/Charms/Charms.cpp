#include "Charms/Charms.h"

#include "ArpgConfig.h"
#include "Core/Charms.h"
#include "Core/AffixEffects.h"
#include "Core/DisplayEnchant.h"
#include "Loot/AffixRoller.h"
#include "Loot/AffixStats.h"
#include "Loot/AffixEffects.h"
#include "Loot/BossPity.h"
#include "Loot/Budget.h"
#include "Loot/GiveItem.h"
#include "Loot/PersonalLoot.h"
#include "Loot/ZoneLevels.h"
#include "Monsters/Champions.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "Bag.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "LootMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "Timer.h"

#include <map>
#include <mutex>
#include <set>
#include <unordered_map>

namespace
{
    struct CharmAffix
    {
        uint32 id = 0;
        std::string name;
        int kind = 0;           // 0 flat stat, 1 effect
        std::string what;
        int weight = 0;
        int minLevel = 1;
        int32 lo = 0, hi = 0;
    };
    struct UniqueLine
    {
        int kind = 0;
        std::string what;
        int32 lo = 0, hi = 0;
    };
    struct Line
    {
        int kind = 0;
        std::string what;
        int32 value = 0;
    };
    struct Applied
    {
        std::vector<Line> lines;
    };
    struct PlayerState
    {
        uint32 nextScan = 0;
        std::map<uint32, Applied> applied;   // charm item low guid -> lines applied
    };

    std::mutex g_lock;
    std::vector<CharmAffix> g_affixes;
    std::map<uint32, std::vector<UniqueLine>> g_unique;          // item entry -> fixed lines
    std::unordered_map<uint32, std::vector<Line>> g_lines;       // charm item low guid -> its lines
    std::unordered_map<uint32, PlayerState> g_players;           // player low guid
    ItemTemplate const* g_ringProto = nullptr;                    // B reference: an epic ring

    void Rec(Player* p, std::string const& line)
    {
        if (p && Arpg::TestEvents::Enabled())
            Arpg::TestEvents::Record(p->GetGUID().GetCounter(), line);
    }

    int CharmSize(uint32 entry)
    {
        switch (entry)
        {
            case Arpg::Charms::SmallCharm: return 1;
            case Arpg::Charms::LargeCharm: return 2;
            case Arpg::Charms::GrandCharm: return 3;
            default: return 0;
        }
    }

    bool IsUniqueCharm(uint32 entry)
    {
        return entry == Arpg::Charms::HeartOfThePandaren || entry == Arpg::Charms::GoblinsLuckyCoin ||
            entry == Arpg::Charms::RiftwalkersSigil;
    }

    // Short names for Describe ("sta", "mf"...).
    std::string ShortName(Line const& l)
    {
        static std::map<std::string, std::string> const m =
        {
            { "strength", "str" }, { "agility", "agi" }, { "intellect", "int" }, { "stamina", "sta" },
            { "crit_rating", "crit" }, { "haste_rating", "haste" }, { "mastery_rating", "mastery" },
            { "magic_find_pct", "mf" }, { "gold_find_pct", "gf" }, { "xp_pct", "xp" }, { "move_speed_pct", "speed" },
            { "armor_pct", "armor" }, { "dmg_vs_elite_pct", "elite_dmg" }, { "dr_vs_elite_pct", "elite_dr" },
        };
        auto it = m.find(l.what);
        return it == m.end() ? l.what : it->second;
    }

    void ApplyLines(Player* p, std::vector<Line> const& lines, bool apply)
    {
        std::vector<Arpg::Loot::EffectLine> effects;
        for (Line const& l : lines)
        {
            if (l.kind == 0)
            {
                if (uint32 mod = Arpg::Loot::StatModFromName(l.what))
                    Arpg::Loot::ApplyStatLine(p, mod, l.value, apply);
            }
            else if (Arpg::Core::Effect e = Arpg::Core::EffectFromName(l.what); e != Arpg::Core::Effect::None)
                effects.push_back({ int(e), l.value });
        }
        if (!effects.empty())
            Arpg::Loot::ApplyEffectLines(p, effects, apply);
    }

    // Charms inside the character's equipped pouch(es).
    std::set<uint32> CharmsInPouch(Player* p, bool& pouchEquipped)
    {
        std::set<uint32> out;
        pouchEquipped = false;
        for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
        {
            Bag* bag = p->GetBagByPos(slot);
            if (!bag || bag->GetEntry() != Arpg::Charms::PouchEntry)
                continue;
            pouchEquipped = true;
            for (uint32 i = 0; i < bag->GetBagSize(); ++i)
                if (Item* it = bag->GetItemByPos(uint8(i)))
                    if (Arpg::Charms::IsCharm(it->GetEntry()))
                        out.insert(it->GetGUID().GetCounter());
        }
        return out;
    }

    void Scan(Player* p)
    {
        bool equipped = false;
        std::set<uint32> want = CharmsInPouch(p, equipped);
        uint32 const pg = p->GetGUID().GetCounter();
        std::vector<std::pair<uint32, std::vector<Line>>> toRemove, toAdd;
        {
            std::lock_guard<std::mutex> l(g_lock);
            PlayerState& st = g_players[pg];
            for (auto it = st.applied.begin(); it != st.applied.end();)
            {
                if (!want.count(it->first))
                {
                    toRemove.push_back({ it->first, it->second.lines });
                    it = st.applied.erase(it);
                }
                else
                    ++it;
            }
            for (uint32 g : want)
            {
                if (st.applied.count(g))
                    continue;
                auto li = g_lines.find(g);
                if (li == g_lines.end())
                    continue;   // not rolled (yet)
                st.applied[g].lines = li->second;
                toAdd.push_back({ g, li->second });
            }
        }
        for (auto const& r : toRemove)
        {
            ApplyLines(p, r.second, false);
            Rec(p, "charm remove guid=" + std::to_string(r.first) + " lines=" + std::to_string(r.second.size()));
        }
        for (auto const& a : toAdd)
        {
            ApplyLines(p, a.second, true);
            Rec(p, "charm apply guid=" + std::to_string(a.first) + " lines=" + std::to_string(a.second.size()));
        }
    }

    void SaveLines(uint32 itemLow, std::vector<Line> const& lines, std::vector<uint32> const& displays)
    {
        CharacterDatabase.DirectPExecute("DELETE FROM item_instance_arpg_charm WHERE item_guid = %u", itemLow);
        for (size_t i = 0; i < lines.size(); ++i)
            CharacterDatabase.DirectPExecute("INSERT INTO item_instance_arpg_charm (item_guid, position, kind, what, value, display_id) "
                "VALUES (%u, %u, %u, '%s', %d, %u)", itemLow, uint32(i), uint32(lines[i].kind), lines[i].what.c_str(), lines[i].value,
                i < displays.size() ? displays[i] : 0u);
    }

    // Kind of a kill for the drop table (same rules as potions).
    int KindOf(Creature* killed)
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        uint32 entry = killed->GetEntry();
        if (Arpg::Loot::IsBossEntry(Arpg::Monsters::BaseEntry(entry)))
            return int(Arpg::Core::KillKind::Boss);
        if (s.superuniqueEntryOffset && entry >= s.superuniqueEntryOffset && entry < s.superuniqueEntryOffset + 300000)
            return int(Arpg::Core::KillKind::Superunique);
        if (Arpg::Monsters::WasChampionKill(killed))
            return int(Arpg::Core::KillKind::Champion);
        CreatureTemplate const* t = killed->GetCreatureTemplate();
        if (t && (t->rank == CREATURE_ELITE_ELITE || t->rank == CREATURE_ELITE_RAREELITE))
            return int(Arpg::Core::KillKind::Elite);
        return int(Arpg::Core::KillKind::Normal);
    }

    class ArpgCharmsPlayerScript : public PlayerScript
    {
    public:
        ArpgCharmsPlayerScript() : PlayerScript("arpg_charms") { }

        void OnLogin(Player* player) override
        {
            Arpg::M9Settings const m = sArpgConfig.Get().m9;
            if (!m.charmsEnabled || !Arpg::Scaling::IsRealPlayer(player))
                return;
            if (m.charmsGrantPouch)
                Arpg::Charms::EnsurePouch(player);
        }

        void OnLogout(Player* player) override
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_players.erase(player->GetGUID().GetCounter());   // the Player object (and its stats) goes away
        }

        void OnUpdate(Player* player, uint32 /*diff*/) override
        {
            if (!player->IsInWorld())
                return;
            Arpg::M9Settings const& m = sArpgConfig.Get().m9;
            if (!m.charmsEnabled)
                return;
            uint32 const now = getMSTime();
            {
                std::lock_guard<std::mutex> l(g_lock);
                auto it = g_players.find(player->GetGUID().GetCounter());
                if (it == g_players.end())
                {
                    if (!Arpg::Scaling::IsRealPlayer(player))
                        return;   // bots carry no charms; never track them
                }
                PlayerState& st = g_players[player->GetGUID().GetCounter()];
                if (now < st.nextScan)
                    return;
                st.nextScan = now + m.charmScanMs;
            }
            Scan(player);
        }

        void OnCreatureKill(Player* killer, Creature* killed) override
        {
            if (!killer || !killed)
                return;
            Arpg::Settings const s = sArpgConfig.Get();
            if (!s.m9.charmsEnabled || !s.lootEnabled || !Arpg::Scaling::IsRealPlayer(killer))
                return;
            if (killed->GetEntry() == 1000010)
                return;   // the Treasure Goblin has its own loot
            int const kind = KindOf(killed);
            float const chance = s.m9.charmDrop[kind];
            auto roll = [&](Player* who) -> std::vector<uint32>
            {
                std::vector<uint32> out;
                if (float(rand_chance()) < chance)
                    out.push_back(Arpg::Charms::RandomCharmEntry());
                if (kind == int(Arpg::Core::KillKind::Superunique) && float(rand_chance()) < s.m9.charmUniqueSuperuniquePct &&
                    !Arpg::Charms::HasCharm(who, Arpg::Charms::HeartOfThePandaren))
                    out.push_back(Arpg::Charms::HeartOfThePandaren);
                return out;
            };
            for (Player* m : Arpg::Loot::PersonalRecipients(killer, killed))
                for (uint32 id : roll(m))
                    Arpg::Loot::AddPersonalItem(m, killed, id);
            std::vector<uint32> mine = roll(killer);
            for (uint32 id : mine)
            {
                Arpg::Monsters::AddLootItem(killed->loot, id);
                Rec(killer, "charm drop item=" + std::to_string(id) + " kind=" + std::to_string(kind));
            }
            if (!mine.empty())
                killed->SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
        }
    };
}

namespace Arpg::Charms
{
    std::string LoadCharmData()
    {
        std::vector<CharmAffix> affixes;
        std::map<uint32, std::vector<UniqueLine>> uniques;
        std::unordered_map<uint32, std::vector<Line>> lines;
        if (QueryResult r = WorldDatabase.Query("SELECT id, name, kind, what, weight, min_level, lo, hi FROM arpg_charm_affix ORDER BY id"))
            do
            {
                Field* f = r->Fetch();
                CharmAffix a;
                a.id = f[0].GetUInt32();
                a.name = f[1].GetString();
                a.kind = int(f[2].GetUInt8());
                a.what = f[3].GetString();
                a.weight = int(f[4].GetUInt32());
                a.minLevel = int(f[5].GetUInt8());
                a.lo = f[6].GetInt32();
                a.hi = f[7].GetInt32();
                affixes.push_back(a);
            } while (r->NextRow());
        if (QueryResult r = WorldDatabase.Query("SELECT item, kind, what, lo, hi FROM arpg_charm_unique ORDER BY item, position"))
            do
            {
                Field* f = r->Fetch();
                uniques[f[0].GetUInt32()].push_back({ int(f[1].GetUInt8()), f[2].GetString(), f[3].GetInt32(), f[4].GetInt32() });
            } while (r->NextRow());
        // Lines of charms that no longer exist go away (deleted, sold, destroyed).
        CharacterDatabase.DirectExecute("DELETE c FROM item_instance_arpg_charm c LEFT JOIN item_instance i ON i.guid = c.item_guid WHERE i.guid IS NULL");
        if (QueryResult r = CharacterDatabase.Query("SELECT item_guid, kind, what, value FROM item_instance_arpg_charm ORDER BY item_guid, position"))
            do
            {
                Field* f = r->Fetch();
                lines[f[0].GetUInt32()].push_back({ int(f[1].GetUInt8()), f[2].GetString(), f[3].GetInt32() });
            } while (r->NextRow());
        ItemTemplate const* ring = nullptr;
        for (auto const& kv : *sObjectMgr->GetItemTemplateStore())
        {
            ItemTemplate const& t = kv.second;
            if (t.InventoryType == INVTYPE_FINGER && t.Quality == ITEM_QUALITY_EPIC && t.ItemLevel >= 400)
            {
                ring = &t;
                break;
            }
        }
        std::lock_guard<std::mutex> l(g_lock);
        g_affixes = affixes;
        g_unique = uniques;
        // Keep lines rolled since startup that the DB query raced with; the DB is the truth for everything else.
        for (auto const& kv : lines)
            g_lines[kv.first] = kv.second;
        g_ringProto = ring;
        return "charms affixes=" + std::to_string(affixes.size()) + " uniques=" + std::to_string(uniques.size()) +
            " items=" + std::to_string(lines.size()) + " ring=" + std::to_string(ring ? ring->ItemId : 0);
    }

    void InstallCharms()
    {
        new ArpgCharmsPlayerScript();
    }

    bool IsCharm(uint32 entry)
    {
        return CharmSize(entry) > 0 || IsUniqueCharm(entry);
    }

    uint32 RandomCharmEntry()
    {
        M9Settings const m = sArpgConfig.Get().m9;
        std::vector<int> w{ m.charmWeight[0], m.charmWeight[1], m.charmWeight[2] };
        int i = Core::PickWeighted(w, urand(0, 0x7FFFFFFF));
        return i == 2 ? GrandCharm : i == 1 ? LargeCharm : SmallCharm;
    }

    bool HasCharm(Player* player, uint32 entry)
    {
        return player && player->HasItemCount(entry, 1, true);
    }

    int RollCharm(Player* player, Item* item, std::string& summary)
    {
        if (!player || !item || !IsCharm(item->GetEntry()))
            return 0;
        uint32 const low = item->GetGUID().GetCounter();
        std::vector<CharmAffix> affixes;
        std::vector<UniqueLine> fixed;
        ItemTemplate const* ring;
        {
            std::lock_guard<std::mutex> l(g_lock);
            if (g_lines.count(low))
                return 0;   // rolled already (moved between bags, mailed...)
            affixes = g_affixes;
            auto u = g_unique.find(item->GetEntry());
            if (u != g_unique.end())
                fixed = u->second;
            ring = g_ringProto;
        }
        int const level = std::max(1, Arpg::Loot::LootLevelFor(player));
        uint32 const budget = ring ? Arpg::Loot::BudgetB(ring, uint8(level)).budget : uint32(level * 10);
        std::vector<Line> lines;
        if (IsUniqueCharm(item->GetEntry()))
        {
            for (UniqueLine const& u : fixed)
            {
                uint32 r = urand(0, 9999);
                lines.push_back({ u.kind, u.what, u.kind == 0 ? Core::CharmStatValue(budget, u.lo, u.hi, r) : Core::RangeValue(u.lo, u.hi, r) });
            }
        }
        else
        {
            std::vector<CharmAffix> pool;
            std::vector<int> weights;
            for (CharmAffix const& a : affixes)
                if (level >= a.minLevel)
                {
                    pool.push_back(a);
                    weights.push_back(a.weight);
                }
            std::vector<uint32> rolls{ urand(0, 0x7FFFFFFF), urand(0, 0x7FFFFFFF), urand(0, 0x7FFFFFFF) };
            for (int idx : Core::PickDistinct(weights, Core::CharmLineCount(CharmSize(item->GetEntry())), rolls))
            {
                CharmAffix const& a = pool[size_t(idx)];
                uint32 r = urand(0, 9999);
                lines.push_back({ a.kind, a.what, a.kind == 0 ? Core::CharmStatValue(budget, a.lo, a.hi, r) : Core::RangeValue(a.lo, a.hi, r) });
            }
        }
        if (lines.empty())
            return 0;
        // Tooltip lines: our client patch's display rows in slots 9-12 (the client hides slot 8 on items without a suffix).
        Arpg::Settings const s = sArpgConfig.Get();
        std::vector<uint32> displays;
        uint32 slot = PROP_ENCHANTMENT_SLOT_1;
        for (Line const& l : lines)
        {
            uint32 id = 0;
            if (s.displayEnabled)
            {
                if (l.kind == 0)
                {
                    int ds = Core::DisplayStatFromItemMod(Arpg::Loot::StatModFromName(l.what));
                    Core::DisplayRule rule{ s.displayBase, s.displayStride };
                    id = ds < 0 ? 0 : Core::DisplayEnchantId(rule, Core::DisplayStat(ds), l.value);
                }
                else if (Core::Effect e = Core::EffectFromName(l.what); e != Core::Effect::None)
                    id = Core::EffectDisplayId(s.displayEffectBase, int(e), l.value, false);
            }
            if (id && slot <= PROP_ENCHANTMENT_SLOT_4)
                item->SetEnchantment(EnchantmentSlot(slot++), id, 0, 0);
            else
                id = 0;
            displays.push_back(id);
        }
        if (item->IsInWorld())
            item->SetState(ITEM_CHANGED, player);
        SaveLines(low, lines, displays);
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_lines[low] = lines;
        }
        summary = "lines=" + std::to_string(lines.size());
        for (Line const& l : lines)
            summary += " " + ShortName(l) + "=" + std::to_string(l.value);
        Rec(player, "charm rolled item=" + std::to_string(item->GetEntry()) + " guid=" + std::to_string(low) + " " + summary);
        return int(lines.size());
    }

    std::string EnsurePouch(Player* player)
    {
        if (!player)
            return "granted=0";
        if (player->HasItemCount(PouchEntry, 1, true))
            return "granted=0";
        for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
        {
            if (player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                continue;
            uint16 dest = 0;
            if (player->CanEquipNewItem(slot, dest, PouchEntry, false) == EQUIP_ERR_OK)
                if (Item* it = player->EquipNewItem(dest, PouchEntry, true))
                {
                    player->SendNewItem(it, 1, true, false, false);
                    return "granted=1 where=bagslot";
                }
        }
        bool mailed = false;
        if (!Arpg::Loot::GiveItem(player, PouchEntry, 1, 0, "Your Charm Pouch", mailed, false))
            return "granted=0 error=create";
        return mailed ? "granted=1 where=mail" : "granted=1 where=bags";
    }

    bool EquipPouch(Player* player, std::string& why)
    {
        for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
            if (Bag* b = player->GetBagByPos(slot))
                if (b->GetEntry() == PouchEntry)
                {
                    why = "equipped slot=" + std::to_string(slot);
                    return true;
                }
        Item* pouch = nullptr;
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END && !pouch; ++slot)
            if (Item* it = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                if (it->GetEntry() == PouchEntry)
                    pouch = it;
        for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END && !pouch; ++bagSlot)
            if (Bag* bag = player->GetBagByPos(bagSlot))
                for (uint32 i = 0; i < bag->GetBagSize() && !pouch; ++i)
                    if (Item* it = bag->GetItemByPos(uint8(i)))
                        if (it->GetEntry() == PouchEntry)
                            pouch = it;
        if (!pouch)
        {
            why = "no pouch in the bags";
            return false;
        }
        for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
            if (!player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                player->SwapItem(pouch->GetPos(), uint16((INVENTORY_SLOT_BAG_0 << 8) | slot));
                why = "equipped slot=" + std::to_string(slot);
                return player->GetBagByPos(slot) != nullptr;
            }
        why = "no free bag slot";
        return false;
    }

    bool MoveCharm(Player* player, uint32 itemLow, bool in, std::string& why)
    {
        Item* item = player->GetItemByGuid(ObjectGuid(HighGuid::Item, itemLow));
        if (!item)
        {
            why = "no such item";
            return false;
        }
        uint8 targetBag = NULL_BAG;
        if (in)
        {
            for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
                if (Bag* b = player->GetBagByPos(slot))
                    if (b->GetEntry() == PouchEntry)
                        targetBag = slot;
            if (targetBag == NULL_BAG)
            {
                why = "no equipped pouch";
                return false;
            }
        }
        else
            targetBag = INVENTORY_SLOT_BAG_0;
        ItemPosCountVec dest;
        InventoryResult r = player->CanStoreItem(targetBag, NULL_SLOT, dest, item, false);
        if (r != EQUIP_ERR_OK || dest.empty())
        {
            why = "can't store there (error " + std::to_string(uint32(r)) + ")";
            return false;
        }
        player->SwapItem(item->GetPos(), dest.front().pos);
        Scan(player);
        why = std::string(in ? "in" : "out") + " pos=" + std::to_string(dest.front().pos);
        return true;
    }

    void Rescan(Player* player)
    {
        if (player)
            Scan(player);
    }

    std::string Describe(Player* player)
    {
        bool equipped = false;
        CharmsInPouch(player, equipped);
        std::map<std::string, int64> totals;
        size_t active = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_players.find(player->GetGUID().GetCounter());
            if (it != g_players.end())
            {
                active = it->second.applied.size();
                for (auto const& kv : it->second.applied)
                    for (Line const& ln : kv.second.lines)
                        totals[ShortName(ln)] += ln.value;
            }
        }
        std::string out = "charms pouch=" + std::string(player->HasItemCount(PouchEntry, 1, true) ? "1" : "0") +
            " equipped=" + (equipped ? "1" : "0") + " active=" + std::to_string(active);
        for (auto const& kv : totals)
            out += " " + kv.first + "=+" + std::to_string(kv.second);
        return out;
    }

    std::string DescribeItem(uint32 itemLow)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_lines.find(itemLow);
        if (it == g_lines.end())
            return "lines=0";
        std::string out = "lines=" + std::to_string(it->second.size());
        for (Line const& ln : it->second)
            out += " " + ShortName(ln) + "=" + std::to_string(ln.value);
        return out;
    }
}
