#include "Character/AnyWeaponSkills.h"

#include "ArpgConfig.h"
#include "Core/WeaponReq.h"

#include "Log.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <map>
#include <set>

namespace Arpg::Character
{
    namespace
    {
        struct Original { int32 itemClass, subMask, invMask; uint32 attrEx3; };
        // The loaded SpellInfo is shared and const; this module is its only writer after load (startup / .arpg reload,
        // on the world thread), and it keeps every original value so the switch can be turned off again.
        std::map<SpellInfo*, Original> Changed;

        void RestoreAll()
        {
            for (auto const& [info, o] : Changed)
            {
                info->EquippedItemClass = o.itemClass;
                info->EquippedItemSubClassMask = o.subMask;
                info->EquippedItemInventoryTypeMask = o.invMask;
                info->AttributesEx3 = o.attrEx3;
            }
            Changed.clear();
        }

        void Remember(SpellInfo* info)
        {
            if (!Changed.count(info))
                Changed[info] = { info->EquippedItemClass, info->EquippedItemSubClassMask, info->EquippedItemInventoryTypeMask,
                                  info->AttributesEx3 };
        }

        // Every loaded SpellInfo of a spell id (regular + difficulty variants), once each.
        template <class F> void ForEachVariant(uint32 id, F&& f)
        {
            std::set<SpellInfo const*> seen;
            for (uint32 d = 0; d < MAX_DIFFICULTY; ++d)
                if (SpellInfo const* info = sSpellMgr->GetSpellInfo(id, Difficulty(d)))
                    if (seen.insert(info).second)
                        f(const_cast<SpellInfo*>(info));
        }
    }

    std::string ApplyAnyWeaponSkills()
    {
        RestoreAll();
        Arpg::Settings const& s = sArpgConfig.Get();
        if (!s.anyWeaponSkills)
            return "weapon skills: off";
        uint32 widened = 0, mainHand = 0;
        for (uint32 id = 1; id < sSpellMgr->GetSpellInfoStoreSize(); ++id)
        {
            bool counted = false;
            ForEachVariant(id, [&](SpellInfo* info)
            {
                Arpg::Core::WeaponReq req{ info->EquippedItemClass, info->EquippedItemSubClassMask,
                                           info->EquippedItemInventoryTypeMask };
                if (!Arpg::Core::WidenWeaponRequirement(info->SpellFamilyName, req))
                    return;
                Remember(info);
                info->EquippedItemSubClassMask = req.subClassMask;
                info->EquippedItemInventoryTypeMask = req.invTypeMask;
                if (!counted)
                {
                    ++widened;
                    counted = true;
                }
            });
        }
        for (uint32 id : s.anyWeaponMainHandSpells)
        {
            bool counted = false;
            ForEachVariant(id, [&](SpellInfo* info)
            {
                if (!(info->AttributesEx3 & SPELL_ATTR3_REQ_OFFHAND))
                    return;
                Remember(info);
                info->AttributesEx3 &= ~uint32(SPELL_ATTR3_REQ_OFFHAND);   // Spell then hits with BASE_ATTACK (main hand)
                if (!counted)
                {
                    ++mainHand;
                    counted = true;
                }
            });
        }
        std::string msg = "weapon skills: widened=" + std::to_string(widened) + " mainhand=" + std::to_string(mainHand);
        TC_LOG_INFO("server.loading", "mod_arpg: %s", msg.c_str());
        return msg;
    }

    std::string DescribeSpellRequirement(uint32_t spellId, uint32_t itemId, bool& ok)
    {
        ok = false;
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
        if (!info)
            return "unknown spell " + std::to_string(spellId);
        std::string line = "spell=" + std::to_string(spellId) + " family=" + std::to_string(info->SpellFamilyName)
            + " class=" + std::to_string(info->EquippedItemClass) + " sub=" + std::to_string(info->EquippedItemSubClassMask)
            + " inv=" + std::to_string(info->EquippedItemInventoryTypeMask)
            + " offhand=" + std::to_string((info->AttributesEx3 & SPELL_ATTR3_REQ_OFFHAND) ? 1 : 0);
        if (itemId)
        {
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
            if (!proto)
                return "unknown item " + std::to_string(itemId);
            // Item::IsFitToSpellRequirements for an item in the main hand (no vellum case for class spells).
            bool fits = true;
            if (info->EquippedItemClass != -1)
            {
                if (info->EquippedItemClass != int32(proto->Class))
                    fits = false;
                else if (info->EquippedItemSubClassMask != 0 && !(info->EquippedItemSubClassMask & (1 << proto->SubClass)))
                    fits = false;
            }
            if (fits && info->EquippedItemInventoryTypeMask != 0)
            {
                bool mhOk = proto->InventoryType == INVTYPE_WEAPON
                    && (info->EquippedItemInventoryTypeMask & ((1 << INVTYPE_WEAPONMAINHAND) | (1 << INVTYPE_WEAPONOFFHAND)));
                if (!mhOk && info->EquippedItemInventoryTypeMask == (1 << INVTYPE_WEAPONOFFHAND))
                    fits = false;                                   // off hand only: the main hand never fits
                else if (!mhOk && !(info->EquippedItemInventoryTypeMask & (1 << proto->InventoryType)))
                    fits = false;
            }
            line += " item=" + std::to_string(itemId) + " fits=" + std::to_string(fits ? 1 : 0);
        }
        ok = true;
        return line;
    }
}
