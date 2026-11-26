#include "Loot/AffixRoller.h"

#include "ArpgConfig.h"
#include "ArpgHooks.h"
#include "Core/AffixRoll.h"
#include "Core/AffixValue.h"
#include "Core/DisplayEnchant.h"
#include "Core/AffixEffects.h"
#include "Loot/AffixEffects.h"
#include "Loot/ZoneLevels.h"
#include "Loot/BossPity.h"
#include "Loot/Crafting.h"
#include "Loot/LootFilter.h"
#include "Monsters/Champions.h"
#include "Dungeons/InstanceScaling.h"
#include "Paragon/Paragon.h"
#include "Core/Pity.h"
#include "Map.h"
#include "Loot/Uniques.h"
#include "Loot/Budget.h"
#include "Loot/NativeEnchants.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Item.h"
#include "ItemEnchantmentMgr.h"
#include "Creature.h"
#include "LootMgr.h"
#include "ScriptMgr.h"
#include "Timer.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"

#include <cmath>
#include <deque>
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace
{
    // Slot-group bits: must match modules/mod_arpg/sql/tools/gen_affix_sql.py
    enum : uint32
    {
        G_WEAPON = 1 << 0, G_OFFHAND = 1 << 1, G_HEAD = 1 << 2, G_SHOULDER = 1 << 3, G_CHEST = 1 << 4, G_WRIST = 1 << 5,
        G_HANDS = 1 << 6, G_WAIST = 1 << 7, G_LEGS = 1 << 8, G_FEET = 1 << 9, G_NECK = 1 << 10, G_FINGER = 1 << 11, G_BACK = 1 << 12,
    };

    struct AffixInfo
    {
        Arpg::Core::AffixDef def;
        std::string name;
        std::string effect;
        std::string unit;        // "B", "pct", "flag"
        uint8 materialMask = 0;  // 1 cloth, 2 leather, 4 mail, 8 plate; 0 = any
    };

    std::mutex g_lock;
    std::vector<AffixInfo> g_affixes;
    std::map<std::pair<uint32, int>, Arpg::Core::TierRange> g_ranges;    // (affix, tier) -> range
    std::vector<Arpg::Core::TierRule> g_tiers;
    std::unordered_map<uint32, int> g_botRoll;   // bot guid -> tier offset for its next roll (party gear)
    std::unordered_map<uint64, uint64> g_lastItem;                      // player guid -> item guid
    std::map<std::pair<uint32, int32>, std::deque<Arpg::Loot::RollPlan>> g_plans;   // (item id, suffix id) -> loot plans
    std::vector<Arpg::Core::AscendedRule> g_ascended;    // T6-T8 gates (arpg_ascended_tier)
    std::map<int, int> g_ascendedPct;                    // tier -> % of the T5 range
    std::unordered_map<uint64, int> g_testRaidTier;      // player guid -> raid tier for the next roll (tests)

    // The raid tier the character's next drop counts for: the map's (arpg_raid_tier) or a test override.
    int RaidTierForRoll(Player* player)
    {
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_testRaidTier.find(player->GetGUID().GetCounter());
            if (it != g_testRaidTier.end())
            {
                int t = it->second;
                g_testRaidTier.erase(it);
                return t;
            }
        }
        Map* map = player->GetMap();
        return map && map->IsRaid() ? Arpg::Dungeons::RaidTierOf(map->GetId()) : 0;
    }
    std::unordered_map<uint32, std::vector<Arpg::Loot::StatLine>> g_itemStats; // item guid -> flat stat affixes
    std::unordered_map<uint32, std::vector<Arpg::Loot::EffectLine>> g_itemEffects; // item guid -> effect affixes

    uint32 SlotGroup(ItemTemplate const* p)
    {
        switch (p->InventoryType)
        {
            case INVTYPE_HEAD: return G_HEAD;
            case INVTYPE_NECK: return G_NECK;
            case INVTYPE_SHOULDERS: return G_SHOULDER;
            case INVTYPE_CHEST: case INVTYPE_ROBE: return G_CHEST;
            case INVTYPE_WAIST: return G_WAIST;
            case INVTYPE_LEGS: return G_LEGS;
            case INVTYPE_FEET: return G_FEET;
            case INVTYPE_WRISTS: return G_WRIST;
            case INVTYPE_HANDS: return G_HANDS;
            case INVTYPE_FINGER: return G_FINGER;
            case INVTYPE_CLOAK: return G_BACK;
            case INVTYPE_WEAPON: case INVTYPE_2HWEAPON: case INVTYPE_WEAPONMAINHAND: case INVTYPE_RANGED:
            case INVTYPE_RANGEDRIGHT: case INVTYPE_THROWN: return G_WEAPON;
            case INVTYPE_SHIELD: case INVTYPE_WEAPONOFFHAND: case INVTYPE_HOLDABLE: return G_OFFHAND;
            default: return 0; // shirts, tabards, trinkets, bags...: no affixes in v1
        }
    }

    uint8 Material(ItemTemplate const* p)
    {
        if (p->Class != ITEM_CLASS_ARMOR)
            return 0;
        switch (p->SubClass)
        {
            case ITEM_SUBCLASS_ARMOR_CLOTH: return 1;
            case ITEM_SUBCLASS_ARMOR_LEATHER: return 2;
            case ITEM_SUBCLASS_ARMOR_MAIL: return 4;
            case ITEM_SUBCLASS_ARMOR_PLATE: return 8;
            default: return 0;
        }
    }

    // Main stat of the player's spec (smart loot). MoP spec ids; class fallback below level 10 (no spec yet).
    uint32 MainStat(Player* p)
    {
        switch (p->GetTalentSpecialization())
        {
            case 71: case 72: case 73: case 66: case 70: case 250: case 251: case 252: return ITEM_MOD_STRENGTH;
            case 253: case 254: case 255: case 259: case 260: case 261: case 263: case 268: case 269: case 103: case 104:
                return ITEM_MOD_AGILITY;
            case 65: case 256: case 257: case 258: case 262: case 264: case 62: case 63: case 64: case 265: case 266: case 267:
            case 270: case 102: case 105: return ITEM_MOD_INTELLECT;
            default: break;
        }
        switch (p->GetClass())
        {
            case CLASS_WARRIOR: case CLASS_PALADIN: case CLASS_DEATH_KNIGHT: return ITEM_MOD_STRENGTH;
            case CLASS_HUNTER: case CLASS_ROGUE: case CLASS_MONK: return ITEM_MOD_AGILITY;
            default: return ITEM_MOD_INTELLECT;
        }
    }

    // Affix effect -> core stat for flat (B) affixes (used for native display; m2 4.1d applies them).
    uint32 EffectStat(std::string const& e)
    {
        static std::map<std::string, uint32> const m =
        {
            { "strength", ITEM_MOD_STRENGTH }, { "agility", ITEM_MOD_AGILITY }, { "intellect", ITEM_MOD_INTELLECT },
            { "stamina", ITEM_MOD_STAMINA }, { "spirit", ITEM_MOD_SPIRIT }, { "crit_rating", ITEM_MOD_CRIT_RATING },
            { "haste_rating", ITEM_MOD_HASTE_RATING }, { "mastery_rating", ITEM_MOD_MASTERY_RATING },
            { "hit_rating", ITEM_MOD_HIT_RATING }, { "expertise_rating", ITEM_MOD_EXPERTISE_RATING },
            { "dodge_rating", ITEM_MOD_DODGE_RATING }, { "parry_rating", ITEM_MOD_PARRY_RATING },
        };
        auto it = m.find(e);
        return it == m.end() ? 0 : it->second;
    }

    bool SourceRolls(ItemPickupSourceType s)
    {
        switch (s)
        {
            case ItemPickupSourceType::CreatureLoot: case ItemPickupSourceType::GameObjectLoot: case ItemPickupSourceType::ItemLoot:
            case ItemPickupSourceType::QuestReward: case ItemPickupSourceType::Spell: case ItemPickupSourceType::CreatureMail:
            case ItemPickupSourceType::Command:
                return true;
            default:
                return false;
        }
    }

    void PickupHook(Player* player, Item* item, ItemPickupSourceType source, uint32 sourceId)
    {
        std::string why;
        // Rolled when the loot was made (the loot window showed its name): apply exactly that plan.
        int32 rp = item ? item->GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) : 0;
        Arpg::Loot::RollPlan plan;
        if (rp < 0 && Arpg::Loot::TakePlan(item->GetEntry(), rp, plan))
        {
            Arpg::Loot::ApplyRoll(player, item, plan, why);
            return;
        }
        Arpg::Loot::RollForItem(player, item, source, why, sourceId);
    }

    // Last kill script (registered after BossPity, Champions, Superuniques): plans every item of the corpse with the kill's
    // full context (Champion +1 tier, owed Unique, Champion/Superunique floor items already added).
    class ArpgLootPlanPlayerScript : public PlayerScript
    {
    public:
        ArpgLootPlanPlayerScript() : PlayerScript("arpg_loot_plan") { }

        void OnCreatureKill(Player* killer, Creature* killed) override
        {
            if (!killer || !killed)
                return;
            Arpg::Loot::PlanLoot(killer, killed->loot, ItemPickupSourceType::CreatureLoot, killed->GetEntry(),
                Arpg::Monsters::WasChampionKill(killed));
        }
    };
}

namespace Arpg::Loot
{
    void LoadAscended()
    {
        std::vector<Core::AscendedRule> rules;
        std::map<int, int> pct;
        if (QueryResult r = WorldDatabase.Query("SELECT tier, min_raid_tier, min_paragon, value_pct FROM arpg_ascended_tier ORDER BY tier"))
            do
            {
                Field* f = r->Fetch();
                rules.push_back({ int(f[0].GetUInt8()), int(f[1].GetUInt8()), int(f[2].GetUInt16()) });
                pct[int(f[0].GetUInt8())] = int(f[3].GetUInt16());
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_ascended = rules;
        g_ascendedPct = pct;
    }

    void AllowNextBotRoll(Player* bot, int tierOffset)
    {
        std::lock_guard<std::mutex> l(g_lock);
        g_botRoll[bot->GetGUID().GetCounter()] = tierOffset;
    }

    int MaxAffixTierAtLevel(int level)
    {
        std::lock_guard<std::mutex> l(g_lock);
        return Core::MaxTierForLevel(g_tiers, level);
    }

    void SetTestRaidTier(Player* player, int tier)
    {
        std::lock_guard<std::mutex> l(g_lock);
        g_testRaidTier[player->GetGUID().GetCounter()] = tier;
    }

    std::string LoadAffixData()
    {
        LoadAscended();
        std::vector<AffixInfo> affixes;
        std::map<std::pair<uint32, int>, Core::TierRange> ranges;
        std::map<int, int> unlock;
        if (QueryResult r = WorldDatabase.Query("SELECT id, name, kind, effect, slot_mask, material_mask, weight, exclusive_group, main_stat, unit FROM arpg_affix"))
            do
            {
                Field* f = r->Fetch();
                AffixInfo a;
                a.def.id = f[0].GetUInt32();
                a.name = f[1].GetString();
                a.def.kind = f[2].GetString() == "suffix" ? Core::AffixKind::Suffix : Core::AffixKind::Prefix;
                a.effect = f[3].GetString();
                a.def.slotMask = f[4].GetUInt32();
                a.materialMask = f[5].GetUInt8();
                a.def.weight = f[6].GetUInt32();
                a.def.exclusiveGroup = f[7].GetUInt32();
                a.def.mainStat = f[8].GetUInt8();
                a.unit = f[9].GetString();
                affixes.push_back(a);
            } while (r->NextRow());
        if (QueryResult r = WorldDatabase.Query("SELECT affix_id, tier, unlock_level, min_value, max_value FROM arpg_affix_tier"))
            do
            {
                Field* f = r->Fetch();
                int tier = int(f[1].GetUInt8());
                ranges[{ f[0].GetUInt32(), tier }] = { f[3].GetFloat(), f[4].GetFloat() };
                unlock[tier] = int(f[2].GetUInt8());
            } while (r->NextRow());
        std::vector<Core::TierRule> tiers;
        for (auto const& kv : unlock)
            tiers.push_back({ kv.first, kv.second });

        std::lock_guard<std::mutex> l(g_lock);
        g_affixes = std::move(affixes);
        g_ranges = std::move(ranges);
        g_tiers = std::move(tiers);
        std::string msg = "affixes=" + std::to_string(g_affixes.size()) + " tiers=" + std::to_string(g_tiers.size());
        TC_LOG_INFO("server.loading", "mod_arpg: loot data %s", msg.c_str());
        return msg;
    }

    void CleanOrphanAffixes()
    {
        CharacterDatabase.DirectExecute("DELETE a FROM item_instance_arpg_affix a LEFT JOIN item_instance i ON i.guid = a.item_guid WHERE i.guid IS NULL");
    }

    void LoadStatCache()
    {
        std::map<uint32, uint32> statOf;
        std::map<uint32, int> effectOf;
        {
            std::lock_guard<std::mutex> l(g_lock);
            for (AffixInfo const& a : g_affixes)
            {
                if (a.unit == "B")
                {
                    if (uint32 stat = EffectStat(a.effect))
                        statOf[a.def.id] = stat;
                }
                else if (Core::Effect e = Core::EffectFromName(a.effect); e != Core::Effect::None)
                    effectOf[a.def.id] = int(e);
            }
        }
        std::unordered_map<uint32, std::vector<StatLine>> cache;
        std::unordered_map<uint32, std::vector<EffectLine>> effects;
        size_t lines = 0;
        if (QueryResult r = CharacterDatabase.Query("SELECT item_guid, affix_id, value, enchant_id FROM item_instance_arpg_affix ORDER BY item_guid, position"))
            do
            {
                Field* f = r->Fetch();
                auto fx = effectOf.find(f[1].GetUInt32());
                if (fx != effectOf.end())
                {
                    effects[f[0].GetUInt32()].push_back({ fx->second, f[2].GetInt32() });
                    ++lines;
                    continue;
                }
                auto it = statOf.find(f[1].GetUInt32());
                if (it == statOf.end())
                    continue;
                cache[f[0].GetUInt32()].push_back({ it->second, f[2].GetInt32(), f[3].GetUInt32() != 0 });
                ++lines;
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_itemStats = std::move(cache);
        g_itemEffects = std::move(effects);
        TC_LOG_INFO("server.loading", "mod_arpg: affix stats cached for %u items (%u lines)", uint32(g_itemStats.size()), uint32(lines));
    }

    std::vector<StatLine> ItemStatLines(uint32 itemGuid, bool includeNative)
    {
        std::vector<StatLine> out;
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_itemStats.find(itemGuid);
        if (it != g_itemStats.end())
            for (StatLine const& sl : it->second)
                if (includeNative || !sl.native)
                    out.push_back(sl);
        return out;
    }

    std::vector<EffectLine> ItemEffectLines(uint32 itemGuid)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_itemEffects.find(itemGuid);
        return it == g_itemEffects.end() ? std::vector<EffectLine>() : it->second;
    }

    uint64 AffixSellPrice(Item const* item, uint64 basePrice)
    {
        if (!item || !basePrice)
            return basePrice;   // unsellable stays unsellable
        uint32 guid = item->GetGUID().GetCounter();
        int affixes = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto s = g_itemStats.find(guid);
            if (s != g_itemStats.end())
                affixes += int(s->second.size());
            auto e = g_itemEffects.find(guid);
            if (e != g_itemEffects.end())
                affixes += int(e->second.size());
        }
        if (!affixes)
            return basePrice;
        int level = std::max<int>(1, int(item->GetTemplate()->RequiredLevel));
        return Core::SellValue(basePrice, affixes, level, sArpgConfig.Get().sellCopperPerAffixLevel);
    }

    void InstallLootPlanner()
    {
        new ArpgLootPlanPlayerScript();
    }

    void InstallPickupHook()
    {
        ArpgHooks::ItemPickup = &PickupHook;
        ArpgHooks::ItemSellPrice = &AffixSellPrice;
    }

    bool PlanRoll(Player* player, ItemTemplate const* proto, ItemPickupSourceType source, uint32 sourceId, int maxCount,
                  bool championLoot, RollPlan& plan, std::string& why)
    {
        Arpg::Settings s = sArpgConfig.Get();
        if (!s.lootEnabled) { why = "loot disabled"; return false; }
        if (!player || !proto) { why = "no player/item"; return false; }
        int botTierOffset = 0;
        bool botAllowed = false;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_botRoll.find(player->GetGUID().GetCounter());
            if (it != g_botRoll.end())
            {
                botAllowed = true;
                botTierOffset = it->second;
                g_botRoll.erase(it);
            }
        }
        if (!Arpg::Scaling::IsRealPlayer(player) && !s.affixBotItems && !botAllowed) { why = "bot"; return false; }
        if (!SourceRolls(source)) { why = "source does not roll"; return false; }
        if (proto->Quality < ITEM_QUALITY_UNCOMMON || proto->Quality > ITEM_QUALITY_EPIC) { why = "quality"; return false; }
        if (proto->RandomProperty || proto->RandomSuffix) { why = "has Blizzard random stats"; return false; }
        uint32 slot = SlotGroup(proto);
        if (!slot) { why = "slot has no affixes"; return false; }

        std::vector<Core::AffixDef> pool;
        std::vector<Core::TierRule> tiers;
        {
            std::lock_guard<std::mutex> l(g_lock);
            uint8 mat = Material(proto);
            for (AffixInfo const& a : g_affixes)
                if (!a.materialMask || !mat || (a.materialMask & mat))
                    pool.push_back(a.def);
            tiers = g_tiers;
        }

        plan = RollPlan();
        plan.itemId = proto->ItemId;
        plan.source = source;
        plan.sourceId = sourceId;
        plan.championLoot = championLoot;

        Core::RollRequest req;
        req.quality = int(proto->Quality);
        req.slotBit = slot;
        int const lootLevel = LootLevelFor(player);   // player level, capped by the zone ceiling (task 4.8)
        req.playerLevel = lootLevel;
        req.playerMainStat = MainStat(player);
        req.smartLootPct = s.smartLootPct;
        req.maxCount = maxCount;
        // Boss loot (task 4.4, BALANCE §13): highest tier the loot level allows; a pending god roll forces 5 affixes on an epic.
        bool const bossLoot = source == ItemPickupSourceType::CreatureLoot && IsBossEntry(sourceId);
        // A Unique owed by this creature (boss / Superunique ladder): the first of its items whose slot has a Unique becomes it.
        if (source == ItemPickupSourceType::CreatureLoot && sourceId && PendingUnique(player, false) == sourceId)
            if ((plan.makeUnique = UniqueForBase(proto->InventoryType, proto->Class, proto->SubClass)))
            {
                // A Superunique owes its own Unique: only an item of that Unique's slot takes it.
                if (uint32 own = PendingUniqueId(player))
                {
                    uint32 inv = 0, armor = 0;
                    bool fits = UniqueSlot(own, inv, armor) && (inv == proto->InventoryType ||
                        (inv == INVTYPE_CHEST && proto->InventoryType == INVTYPE_ROBE) || (inv == INVTYPE_ROBE && proto->InventoryType == INVTYPE_CHEST)) &&
                        (!armor || (proto->Class == ITEM_CLASS_ARMOR && proto->SubClass == armor));
                    plan.makeUnique = fits ? own : 0;
                }
            }
            if (plan.makeUnique)
            {
                req.maxCount = 4;  // 4 affixes + the power line
                PendingUnique(player, true);
                SetUniqueMisses(player, sourceId, 0);
            }
        // Champion loot (m3 task 3.3): +1 affix tier for items from a Champion's corpse.
        if (championLoot)
            req.tierBonus += 1;
        req.tierBonus += botTierOffset;   // party bot gear: below the player's tier
        // Crafting wheel (SPEC 4.8): a tradeskill recipe's colour sets the roll; orange/yellow may make a Unique.
        CraftInfo craft;
        if (source == ItemPickupSourceType::Spell && (craft = CraftFor(player, sourceId)).isCraft)
        {
            UseCatalyst(player, craft, Core::MaxTierForLevel(tiers, lootLevel));   // a catalyst in the bags is used up here
            req.tierBonus += craft.mods.tierBonus;
            req.extraCount = craft.mods.extraAffixes;
            req.tierCap = craft.mods.tierCap;
            if (uint32 u = UniqueForBase(proto->InventoryType, proto->Class, proto->SubClass))
                if (RollCraftUnique(player, craft))
                {
                    plan.makeUnique = u;
                    req.maxCount = 4;
                }
        }
        if (bossLoot)
        {
            req.tierAtMax = true;  // the highest tier the loot level allows (not above it)
            if (proto->Quality == ITEM_QUALITY_EPIC && GodRollPending(player))
            {
                req.forceCount = 5;
                plan.godRoll = true;
            }
        }
        // Ascended tiers (BALANCE §12): in a raid, raid tier + Paragon may lift the cap above T5 (T6-T8).
        {
            int raidTier = RaidTierForRoll(player);
            if (raidTier > 0)
            {
                std::vector<Core::AscendedRule> rules;
                {
                    std::lock_guard<std::mutex> l(g_lock);
                    rules = g_ascended;
                }
                int base = Core::MaxTierForLevel(tiers, lootLevel);
                int asc = Core::MaxAffixTier(base, raidTier, Arpg::Paragon::ParagonLevel(player), rules.data(), int(rules.size()));
                if (asc > base)
                {
                    req.absoluteMaxTier = asc;
                    req.tierBonus += asc - base;
                }
            }
        }
        // Magic find (of Fortune / Lucky): chance in hundredths of a percent for +1 tier (BALANCE §13 pity is separate).
        if (int32 mf = EffectTotal(player, Core::Effect::MagicFindPct))
            if (int32(urand(0, 9999)) < std::min<int32>(mf, 10000))
                req.tierBonus += 1;
        plan.rolled = Core::RollAffixes(pool, tiers, req, [](uint32_t n) { return n ? urand(0, n - 1) : 0u; });
        if (plan.rolled.empty()) { why = "nothing rolled"; return false; }
        if (bossLoot && plan.rolled.size() >= 5)
        {
            GodRollGranted(player);   // any 5-affix boss item (forced or lucky) resets the counter
            Arpg::TestEvents::Record(player->GetGUID().GetCounter(), std::string("godroll granted") + (plan.godRoll ? " (pity)" : ""));
        }
        int nameTier = 0;
        for (Core::RolledAffix const& r : plan.rolled)
            if (r.tier > nameTier)   // the item is named after its highest-tier affix (first one on ties)
            {
                nameTier = r.tier;
                plan.nameAffixId = r.affixId;
            }
        return true;
    }

    int32 PlanSuffixId(RollPlan const& plan)
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        if (!s.displayEnabled || !s.displaySuffixBase)
            return 0;
        if (plan.makeUnique)
            return -int32(s.displaySuffixBase + 1000 + plan.makeUnique);
        return plan.nameAffixId ? -int32(s.displaySuffixBase + plan.nameAffixId) : 0;
    }

    // The roll's values for `lootLevel`; `player` may be null (auction listings: no owner, no events, no Unique).
    int ApplyRollAt(Player* player, Item* item, RollPlan const& plan, int lootLevel, std::string& why)
    {
        Arpg::Settings s = sArpgConfig.Get();
        ItemTemplate const* proto = item ? item->GetTemplate() : nullptr;
        if (!proto) { why = "no item"; return 0; }
        std::vector<Core::RolledAffix> const& rolled = plan.rolled;
        std::map<uint32, AffixInfo> byId;
        std::map<std::pair<uint32, int>, Core::TierRange> ranges;
        std::vector<Core::TierRule> tiers;
        {
            std::lock_guard<std::mutex> l(g_lock);
            for (AffixInfo const& a : g_affixes)
                byId[a.def.id] = a;
            ranges = g_ranges;
            tiers = g_tiers;
        }
        uint32 budget = BudgetB(proto, uint8(lootLevel)).budget;
        uint32 itemGuid = item->GetGUID().GetCounter();
        std::string summary;
        std::vector<StatLine> statLines;
        std::vector<EffectLine> effectLines;
        // With our display rows the item also gets a random-suffix id (below), which makes the client show slot 8 too:
        // five lines. Without them (old behaviour) slot 8 is hidden (MoP reforge slot), so start at 9.
        uint32 nextNativeSlot = s.displayEnabled ? uint32(PROP_ENCHANTMENT_SLOT_0) : uint32(PROP_ENCHANTMENT_SLOT_1);
        CharacterDatabase.DirectPExecute("DELETE FROM item_instance_arpg_affix WHERE item_guid = %u", itemGuid);
        for (size_t i = 0; i < rolled.size() && i < 5; ++i)
        {
            if (!byId.count(rolled[i].affixId))
                continue;   // affix removed by a reload since the plan was made
            AffixInfo const& a = byId[rolled[i].affixId];
            Core::TierRange range = ranges[{ a.def.id, rolled[i].tier }];
            if (rolled[i].tier > 5 && range.max <= 0.0f)   // Ascended: T5 range x the tier's %
            {
                int pct = 100;
                {
                    std::lock_guard<std::mutex> l(g_lock);
                    auto it = g_ascendedPct.find(rolled[i].tier);
                    pct = it == g_ascendedPct.end() ? 100 : it->second;
                }
                range = Core::AscendedRange(ranges[{ a.def.id, 5 }], pct);
            }
            int32 value = 1;
            if (a.unit == "B")
                value = int32(Core::FlatValue(budget, range, rand_norm()));
            else if (a.unit == "pct")
                value = Core::RoundToTenths(Core::PercentValue(range, rand_norm()));   // shown to 0.1%

            uint32 enchantId = 0;    // Blizzard row that APPLIES the stat (core enchant code); 0 = AffixStats applies it
            uint32 displayId = 0;    // our client patch row that only SHOWS the line (server doesn't know it)
            uint32 const flatStat = (a.unit == "B") ? EffectStat(a.effect) : 0;
            if (flatStat && s.displayEnabled)
            {
                int ds = Core::DisplayStatFromItemMod(flatStat);
                Core::DisplayRule rule{ s.displayBase, s.displayStride };
                displayId = ds < 0 ? 0 : Core::DisplayEnchantId(rule, Core::DisplayStat(ds), value);
                if (displayId && nextNativeSlot <= PROP_ENCHANTMENT_SLOT_4)
                    item->SetEnchantment(EnchantmentSlot(nextNativeSlot++), displayId, 0, 0);
                else
                    displayId = 0;   // no row for this value, or 5 lines already: ArpgUI shows it
            }
            else if (!flatStat && s.displayEnabled && a.unit != "B")
            {
                // Effect affix line ("1.5% of damage dealt heals you"): our client patch row, display only.
                Core::Effect e = Core::EffectFromName(a.effect);
                displayId = e == Core::Effect::None ? 0 :
                    Core::EffectDisplayId(s.displayEffectBase, int(e), value, a.unit == "flag");
                if (displayId && nextNativeSlot <= PROP_ENCHANTMENT_SLOT_4)
                    item->SetEnchantment(EnchantmentSlot(nextNativeSlot++), displayId, 0, 0);
                else
                    displayId = 0;
            }
            else if (flatStat)
            {
                Core::EnchantMatch m = NativeEnchants().Nearest(flatStat, uint32(value));
                if (m.enchantId && std::abs(int32(m.amount) - value) * 20 <= value && nextNativeSlot <= PROP_ENCHANTMENT_SLOT_4)
                {
                    enchantId = m.enchantId;
                    value = int32(m.amount);
                    item->SetEnchantment(EnchantmentSlot(nextNativeSlot++), enchantId, 0, 0);
                }
            }
            if (a.effect == "prismatic_socket")   // Socketed: a real prismatic socket (the belt buckle's enchant)
                item->SetEnchantment(PRISMATIC_ENCHANTMENT_SLOT, 3729, 0, 0);
            if (flatStat)
                statLines.push_back({ flatStat, value, enchantId != 0 });
            else if (Core::Effect e = Core::EffectFromName(a.effect); e != Core::Effect::None)
                effectLines.push_back({ int(e), value });
            CharacterDatabase.DirectPExecute("INSERT INTO item_instance_arpg_affix (item_guid, position, affix_id, tier, value, enchant_id, display_id) "
                "VALUES (%u, %u, %u, %u, %d, %u, %u)", itemGuid, uint32(i), a.def.id, uint32(rolled[i].tier), value, enchantId, displayId);
            summary += (summary.empty() ? "" : ", ") + a.name + " T" + std::to_string(rolled[i].tier) + "=" + std::to_string(value) +
                (enchantId ? "(native)" : displayId ? "(shown)" : "");
        }
        if (s.displayEnabled && plan.nameAffixId && s.displaySuffixBase)
            item->SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID, -int32(s.displaySuffixBase + plan.nameAffixId));   // field only
        if (plan.makeUnique && player)
        {
            std::string msg;
            int tier = std::max(1, Core::MaxTierForLevel(tiers, lootLevel));
            if (MakeUnique(player, item, plan.makeUnique, tier, msg))
                summary += " | " + msg;
        }
        if (player && item->IsInWorld())   // in the bags; a mailed item (overflow) is saved by the mail
            item->SetState(ITEM_CHANGED, player);
        {
            std::lock_guard<std::mutex> l(g_lock);
            if (player)
                g_lastItem[player->GetGUID().GetCounter()] = itemGuid;
            g_itemStats[itemGuid] = statLines;   // new items are in bags, never equipped yet: no stats to re-apply
            g_itemEffects[itemGuid] = effectLines;
        }
        if (player)
            Arpg::TestEvents::Record(player->GetGUID().GetCounter(), "affix item=" + std::to_string(proto->ItemId) + " guid=" +
                std::to_string(itemGuid) + " q=" + std::to_string(proto->Quality) + " n=" + std::to_string(rolled.size()) +
                (plan.championLoot ? " champion" : "") + (plan.planned ? " planned" : "") + " " + summary);
        else
            why = summary;
        return int(rolled.size());
    }

    int ApplyRoll(Player* player, Item* item, RollPlan const& plan, std::string& why)
    {
        if (!player) { why = "no player"; return 0; }
        return ApplyRollAt(player, item, plan, LootLevelFor(player), why);
    }

    int RollForAuction(Item* item, int level, int tierOffset, std::string& why)
    {
        ItemTemplate const* proto = item ? item->GetTemplate() : nullptr;
        if (!proto) { why = "no item"; return 0; }
        if (proto->Quality < ITEM_QUALITY_UNCOMMON || proto->Quality > ITEM_QUALITY_EPIC) { why = "quality"; return 0; }
        if (proto->RandomProperty || proto->RandomSuffix) { why = "has Blizzard random stats"; return 0; }
        uint32 slot = SlotGroup(proto);
        if (!slot) { why = "slot has no affixes"; return 0; }
        std::vector<Core::AffixDef> pool;
        std::vector<Core::TierRule> tiers;
        {
            std::lock_guard<std::mutex> l(g_lock);
            uint8 mat = Material(proto);
            for (AffixInfo const& a : g_affixes)
                if (!a.materialMask || !mat || (a.materialMask & mat))
                    pool.push_back(a.def);
            tiers = g_tiers;
        }
        RollPlan plan;
        plan.itemId = proto->ItemId;
        Core::RollRequest req;
        req.quality = int(proto->Quality);
        req.slotBit = slot;
        req.playerLevel = std::max(1, level);
        req.smartLootPct = 0;           // no buyer to favour
        req.tierBonus = tierOffset;
        plan.rolled = Core::RollAffixes(pool, tiers, req, [](uint32_t n) { return n ? urand(0, n - 1) : 0u; });
        if (plan.rolled.empty()) { why = "nothing rolled"; return 0; }
        int nameTier = 0;
        for (Core::RolledAffix const& a : plan.rolled)
            if (a.tier > nameTier)
            {
                nameTier = a.tier;
                plan.nameAffixId = a.affixId;
            }
        return ApplyRollAt(nullptr, item, plan, req.playerLevel, why);
    }

    int RollForItem(Player* player, Item* item, ItemPickupSourceType source, std::string& why, uint32 sourceId, int maxCount)
    {
        RollPlan plan;
        bool const championLoot = source == ItemPickupSourceType::CreatureLoot && Arpg::Monsters::IsLootingChampion(player);
        if (!item || !PlanRoll(player, item->GetTemplate(), source, sourceId, maxCount, championLoot, plan, why))
            return 0;
        return ApplyRoll(player, item, plan, why);
    }

    void PlanLoot(Player* owner, ::Loot& loot, ItemPickupSourceType source, uint32 sourceId, bool championLoot)
    {
        if (!owner)
            return;
        uint32 now = getMSTime();
        for (LootItem& li : loot.items)
        {
            if (li.currency || li.is_looted || li.randomPropertyId)
                continue;
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(li.itemid);
            RollPlan plan;
            std::string why;
            if (!proto || !PlanRoll(owner, proto, source, sourceId, 0, championLoot, plan, why))
            {
                if (proto)
                    ApplyFilter(owner, loot, li, 0, 0, false);   // greys / whites go through the filter too
                continue;
            }
            int bestTier = 0;
            for (Core::RolledAffix const& r : plan.rolled)
                bestTier = std::max(bestTier, r.tier);
            if (!ApplyFilter(owner, loot, li, int(plan.rolled.size()), bestTier, plan.makeUnique != 0))
            {
                if (plan.makeUnique)
                    SetPendingUnique(owner, sourceId, plan.makeUnique);   // never happens (Uniques are protected); safety
                continue;   // sold or left: the plan is dropped
            }
            int32 rp = PlanSuffixId(plan);
            if (!rp)
                continue;   // no display rows: nothing to show in the loot window - roll at pickup as before
            plan.planned = true;
            li.randomPropertyId = rp;   // the loot window shows the final name ("X of the Whale", "X - <Unique>")
            // The suffix factor too, as the core gives an item with a suffix (Item::UpdateItemSuffixFactor): with 0 the client
            // shows the plain name in the loot window (owner 2026-11-27: the name only appeared in the bags).
            li.randomSuffix = GenerateEnchSuffixFactor(li.itemid);
            {
                std::lock_guard<std::mutex> l(g_lock);
                // Drop plans nobody picked up for an hour (corpses despawn long before), and cap the store.
                for (auto it = g_plans.begin(); it != g_plans.end();)
                {
                    auto& dq = it->second;
                    while (!dq.empty() && getMSTimeDiff(dq.front().madeAt, now) > 3600000)
                        dq.pop_front();
                    it = dq.empty() ? g_plans.erase(it) : std::next(it);
                }
                plan.madeAt = now;
                g_plans[{ li.itemid, rp }].push_back(plan);
            }
            Arpg::TestEvents::Record(owner->GetGUID().GetCounter(), "loot plan item=" + std::to_string(li.itemid) + " rp=" +
                std::to_string(rp) + " n=" + std::to_string(plan.rolled.size()) + " unique=" + std::to_string(plan.makeUnique) +
                (championLoot ? " champion" : ""));
        }
    }

    bool TakePlan(uint32 itemId, int32 rp, RollPlan& out)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_plans.find({ itemId, rp });
        if (it == g_plans.end() || it->second.empty())
            return false;
        out = it->second.front();
        it->second.pop_front();
        if (it->second.empty())
            g_plans.erase(it);
        return true;
    }

    std::string DescribeLastItem(Player* player)
    {
        uint64 itemGuid = 0;
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_lastItem.find(player->GetGUID().GetCounter());
            if (it == g_lastItem.end())
                return "no item rolled for this character since the server started";
            itemGuid = it->second;
        }
        std::map<uint32, std::string> names;
        {
            std::lock_guard<std::mutex> l(g_lock);
            for (AffixInfo const& a : g_affixes)
                names[a.def.id] = a.name;
        }
        QueryResult r = CharacterDatabase.PQuery("SELECT affix_id, tier, value, enchant_id, display_id FROM item_instance_arpg_affix WHERE item_guid = %u ORDER BY position", uint32(itemGuid));
        std::string out = "item guid=" + std::to_string(itemGuid);
        if (Item* it = player->GetItemByGuid(ObjectGuid(HighGuid::Item, uint32(itemGuid))))
            if (int32 rp = it->GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID); rp < 0)
                if (ItemRandomSuffixEntry const* sfx = sItemRandomSuffixStore.LookupEntry(uint32(-rp)))
                    out += std::string(" name=\"") + sfx->nameSuffix + "\"";
        if (Item* it = player->GetItemByGuid(ObjectGuid(HighGuid::Item, uint32(itemGuid))))
            out += " sell=" + std::to_string(AffixSellPrice(it, uint64(it->GetTemplate()->SellPrice) * it->GetCount()));
        if (UniqueOf(uint32(itemGuid)).uniqueId)
            out += " " + DescribeUnique(UniqueOf(uint32(itemGuid)));
        int n = 0;
        if (r)
            do
            {
                Field* f = r->Fetch();
                out += " | " + names[f[0].GetUInt32()] + " T" + std::to_string(f[1].GetUInt8()) + "=" + std::to_string(f[2].GetInt32()) +
                    (f[3].GetUInt32() ? " native" : f[4].GetUInt32() ? " shown=" + std::to_string(f[4].GetUInt32()) : "");
                ++n;
            } while (r->NextRow());
        return "affixes=" + std::to_string(n) + " " + out;
    }
}
