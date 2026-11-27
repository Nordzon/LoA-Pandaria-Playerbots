#include "Loot/Blanks.h"

#include "ArpgConfig.h"
#include "ArpgHooks.h"
#include "Core/Blanks.h"
#include "Loot/AffixRoller.h"
#include "Loot/Crafting.h"
#include "Loot/ZoneLevels.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "DatabaseEnv.h"
#include "Item.h"
#include "LootMgr.h"
#include "ObjectMgr.h"
#include "Player.h"

#include <map>
#include <mutex>
#include <unordered_set>

namespace
{
    // (slot, class, subclass, quality) -> bracket -> main stat -> blank entry
    using Key = std::tuple<uint32, uint32, uint32, uint32>;
    std::mutex g_lock;
    std::map<Key, std::map<int, std::map<uint32, uint32>>> g_blanks;
    std::unordered_set<uint32> g_ids;

    uint32 const SalvagedEssence = 700010;

    uint32 SlotOf(uint32 inventoryType)
    {
        return inventoryType == INVTYPE_ROBE ? uint32(INVTYPE_CHEST) : inventoryType;   // robes and chests share blanks
    }

    bool Eligible(ItemTemplate const* p)
    {
        return p && (p->Class == ITEM_CLASS_WEAPON || p->Class == ITEM_CLASS_ARMOR) && p->Quality >= ITEM_QUALITY_UNCOMMON &&
            p->Quality <= ITEM_QUALITY_EPIC && !p->RandomProperty && !p->RandomSuffix;
    }

    void Rec(Player* p, std::string const& s)
    {
        if (p)
            Arpg::TestEvents::Record(p->GetGUID().GetCounter(), s);
    }

    // (b) Spell::DoCreateItem: crafted gear comes out as its blank (crafter's loot level and spec).
    uint32 CraftedItemHook(Player* player, uint32 spellId, uint32 itemId)
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        if (!s.blankEnabled || !s.blankCrafted || !player || !Arpg::Scaling::IsRealPlayer(player))
            return itemId;
        if (!Arpg::Loot::IsCraftSpell(spellId))
            return itemId;
        uint32 blank = Arpg::Loot::BlankForItem(player, itemId);
        if (!blank)
            return itemId;
        Rec(player, "blank crafted from=" + std::to_string(itemId) + " to=" + std::to_string(blank));
        return blank;
    }
}

namespace Arpg::Loot
{
    std::string LoadBlanks()
    {
        std::map<Key, std::map<int, std::map<uint32, uint32>>> blanks;
        std::unordered_set<uint32> ids;
        size_t n = 0;
        if (QueryResult r = WorldDatabase.Query("SELECT entry, inventory_type, item_class, subclass, quality, bracket, main_stat FROM arpg_blank"))
            do
            {
                Field* f = r->Fetch();
                uint32 entry = f[0].GetUInt32();
                if (!sObjectMgr->GetItemTemplate(entry))
                    continue;
                Key k{ SlotOf(f[1].GetUInt8()), f[2].GetUInt8(), f[3].GetUInt8(), f[4].GetUInt8() };
                blanks[k][int(f[5].GetUInt8())][f[6].GetUInt8()] = entry;
                ids.insert(entry);
                ++n;
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_blanks = std::move(blanks);
        g_ids = std::move(ids);
        return "blank bases=" + std::to_string(n);
    }

    void InstallBlanks()
    {
        ArpgHooks::CraftedItem = &CraftedItemHook;
    }

    bool IsBlank(uint32 itemId)
    {
        std::lock_guard<std::mutex> l(g_lock);
        return g_ids.count(itemId) != 0;
    }

    uint32 BlankFor(uint32 inventoryType, uint32 itemClass, uint32 subclass, uint32 quality, int level, uint32 mainStat)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_blanks.find(Key{ SlotOf(inventoryType), itemClass, subclass, quality });
        if (it == g_blanks.end())
            return 0;
        std::vector<int> brackets;
        for (auto const& b : it->second)
            brackets.push_back(b.first);
        auto const& variants = it->second.at(Core::PickBlankBracket(brackets, level));
        std::vector<uint32_t> stats;
        for (auto const& v : variants)
            stats.push_back(v.first);
        return variants.at(Core::PickBlankMainStat(stats, mainStat));
    }

    uint32 BlankForItem(Player* player, uint32 itemId)
    {
        ItemTemplate const* p = sObjectMgr->GetItemTemplate(itemId);
        if (!player || !Eligible(p) || IsBlank(itemId))
            return 0;
        return BlankFor(p->InventoryType, p->Class, p->SubClass, p->Quality, LootLevelFor(player), SpecMainStat(player));
    }

    void MaybeBlankLoot(Player* owner, ::Loot& loot)
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        if (!s.blankEnabled || !s.blankDropChance || !owner || !Arpg::Scaling::IsRealPlayer(owner))
            return;
        for (LootItem& li : loot.items)
        {
            if (li.currency || li.needs_quest || li.is_looted || li.randomPropertyId)
                continue;
            if (urand(1, 100) > s.blankDropChance)
                continue;
            if (uint32 blank = BlankForItem(owner, li.itemid))
            {
                Rec(owner, "blank drop from=" + std::to_string(li.itemid) + " to=" + std::to_string(blank));
                li.itemid = blank;
                li.randomSuffix = 0;
            }
        }
    }

    std::vector<BlankShopSlot> const& BlankShopSlots()
    {
        static std::vector<BlankShopSlot> const slots =
        {
            { "Head", INVTYPE_HEAD, ITEM_CLASS_ARMOR, -1 }, { "Shoulders", INVTYPE_SHOULDERS, ITEM_CLASS_ARMOR, -1 },
            { "Chest", INVTYPE_CHEST, ITEM_CLASS_ARMOR, -1 }, { "Waist", INVTYPE_WAIST, ITEM_CLASS_ARMOR, -1 },
            { "Legs", INVTYPE_LEGS, ITEM_CLASS_ARMOR, -1 }, { "Feet", INVTYPE_FEET, ITEM_CLASS_ARMOR, -1 },
            { "Wrists", INVTYPE_WRISTS, ITEM_CLASS_ARMOR, -1 }, { "Hands", INVTYPE_HANDS, ITEM_CLASS_ARMOR, -1 },
            { "Back", INVTYPE_CLOAK, ITEM_CLASS_ARMOR, ITEM_SUBCLASS_ARMOR_CLOTH },
            { "Neck", INVTYPE_NECK, ITEM_CLASS_ARMOR, ITEM_SUBCLASS_ARMOR_MISCELLANEOUS },
            { "Finger", INVTYPE_FINGER, ITEM_CLASS_ARMOR, ITEM_SUBCLASS_ARMOR_MISCELLANEOUS },
            { "Shield", INVTYPE_SHIELD, ITEM_CLASS_ARMOR, ITEM_SUBCLASS_ARMOR_SHIELD },
            { "Off-hand (held)", INVTYPE_HOLDABLE, ITEM_CLASS_ARMOR, ITEM_SUBCLASS_ARMOR_MISCELLANEOUS },
            { "One-hand sword", INVTYPE_WEAPON, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_SWORD },
            { "One-hand axe", INVTYPE_WEAPON, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_AXE },
            { "One-hand mace", INVTYPE_WEAPON, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_MACE },
            { "Dagger", INVTYPE_WEAPON, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_DAGGER },
            { "Fist weapon", INVTYPE_WEAPON, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_FIST_WEAPON },
            { "Two-hand sword", INVTYPE_2HWEAPON, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_SWORD2 },
            { "Two-hand axe", INVTYPE_2HWEAPON, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_AXE2 },
            { "Two-hand mace", INVTYPE_2HWEAPON, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_MACE2 },
            { "Polearm", INVTYPE_2HWEAPON, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_POLEARM },
            { "Staff", INVTYPE_2HWEAPON, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_STAFF },
            { "Bow", INVTYPE_RANGED, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_BOW },
            { "Gun", INVTYPE_RANGEDRIGHT, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_GUN },
            { "Wand", INVTYPE_RANGEDRIGHT, ITEM_CLASS_WEAPON, ITEM_SUBCLASS_WEAPON_WAND },
        };
        return slots;
    }

    int BlankEssenceCost(uint32 quality)
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        switch (quality)
        {
            case ITEM_QUALITY_UNCOMMON: return int(s.blankEssenceGreen);
            case ITEM_QUALITY_RARE: return int(s.blankEssenceBlue);
            case ITEM_QUALITY_EPIC: return int(s.blankEssenceEpic);
            default: return 0;
        }
    }

    bool BuyBlank(Player* player, int slot, uint32 quality, std::string& out)
    {
        std::vector<BlankShopSlot> const& slots = BlankShopSlots();
        if (!player || slot < 0 || slot >= int(slots.size()) || quality < ITEM_QUALITY_UNCOMMON || quality > ITEM_QUALITY_EPIC)
        {
            out = "no such blank";
            return false;
        }
        BlankShopSlot const& ss = slots[size_t(slot)];
        int level = LootLevelFor(player);
        uint32 sub = ss.subclass >= 0 ? uint32(ss.subclass) : uint32(Core::ArmorSubclassForClass(player->GetClass(), int(player->GetLevel())));
        uint32 blank = BlankFor(ss.inventoryType, ss.itemClass, sub, quality, level, SpecMainStat(player));
        if (!blank && ss.inventoryType == INVTYPE_CHEST)
            blank = BlankFor(INVTYPE_ROBE, ss.itemClass, sub, quality, level, SpecMainStat(player));
        int cost = BlankEssenceCost(quality);
        if (!blank)
        {
            out = std::string("no ") + ss.name + " blank of that quality";
            return false;
        }
        if (!player->HasItemCount(SalvagedEssence, uint32(std::max(0, cost))))
        {
            out = "not enough essences: need " + std::to_string(cost) + ", have " + std::to_string(player->GetItemCount(SalvagedEssence));
            return false;
        }
        ItemPosCountVec dest;
        if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, blank, 1) != EQUIP_ERR_OK)
        {
            out = "no room in the bags";
            return false;
        }
        if (cost > 0)
            player->DestroyItemCount(SalvagedEssence, uint32(cost), true);
        Item* item = player->StoreNewItem(dest, blank, true);
        if (!item)
        {
            out = "could not create the item";
            return false;
        }
        std::string why;
        int n = RollForItem(player, item, ItemPickupSourceType::Command, why);
        player->SendNewItem(item, 1, true, false);
        out = "bought blank item=" + std::to_string(blank) + " for essences=" + std::to_string(cost) + " affixes=" + std::to_string(n);
        Rec(player, out);
        return true;
    }
}
