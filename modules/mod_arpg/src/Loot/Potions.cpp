#include "Loot/Potions.h"

#include "ArpgConfig.h"
#include "Core/Potions.h"
#include "Loot/BossPity.h"
#include "Monsters/Champions.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "Creature.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "LootMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <mutex>
#include <unordered_map>

namespace
{
    struct Drop { int tier; float chancePct; uint32 wHeal, wMana, wRejuv; };

    std::mutex g_lock;
    std::vector<Arpg::Core::PotionDef> g_potions;
    std::unordered_map<uint32, Arpg::Core::PotionDef> g_bySpell;     // potion's use spell -> potion
    std::unordered_map<uint32, Drop> g_drops;                        // source kind -> drop rule

    bool PotionBySpell(uint32 spellId, Arpg::Core::PotionDef& out)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_bySpell.find(spellId);
        if (it == g_bySpell.end())
            return false;
        out = it->second;
        return true;
    }

    void Rec(Player* p, std::string const& line)
    {
        if (p && Arpg::TestEvents::Enabled())
            Arpg::TestEvents::Record(p->GetGUID().GetCounter(), line);
    }

    Arpg::Core::Resource ResourceOf(Powers pw)
    {
        switch (pw)
        {
            case POWER_MANA:        return Arpg::Core::Resource::Mana;
            case POWER_RAGE:        return Arpg::Core::Resource::Rage;
            case POWER_FOCUS:       return Arpg::Core::Resource::Focus;
            case POWER_ENERGY:      return Arpg::Core::Resource::Energy;
            case POWER_RUNIC_POWER: return Arpg::Core::Resource::RunicPower;
            default:                return Arpg::Core::Resource::Other;
        }
    }

    // Mana / rejuvenation: the drinker's main resource by the rule (and mana too when the main one isn't mana, e.g. a druid
    // in cat form). Returns the amount given to the main resource.
    uint32 RestoreResource(Player* p, int pct)
    {
        Powers main = p->GetPowerType();
        uint32 given = Arpg::Core::PotionResource(ResourceOf(main), uint32(std::max(0, p->GetMaxPower(main))), pct);
        if (given)
            p->ModifyPower(main, int32(given));
        if (main != POWER_MANA && p->GetMaxPower(POWER_MANA) > 0)
            p->ModifyPower(POWER_MANA, int32(Arpg::Core::PotionResource(Arpg::Core::Resource::Mana, uint32(p->GetMaxPower(POWER_MANA)), pct)));
        return given;
    }

    // The whole use of an ARPG potion (owner 2026-11-22): the core's potion spell would heal a flat amount and refuse a mana
    // potion to a rage/energy class, so the script does it: % heal and/or resource, one item used, the shared cooldown.
    bool Drink(Player* p, Item* item, uint8 castCount, std::string* report)
    {
        ItemTemplate const* proto = item->GetTemplate();
        Arpg::Core::PotionDef def{};
        uint32 spellId = proto->Spells[0].SpellId > 0 ? uint32(proto->Spells[0].SpellId) : 0;
        SpellInfo const* si = spellId ? sSpellMgr->GetSpellInfo(spellId) : nullptr;
        if (!si || !PotionBySpell(spellId, def))
            return false;   // not ours: the core casts it
        if (!p->IsAlive())
        {
            Spell::SendCastResult(p, si, castCount, SPELL_FAILED_CASTER_DEAD);
            return true;
        }
        if (!p->GetSpellHistory()->IsReady(si, proto->ItemId, false))
        {
            Spell::SendCastResult(p, si, castCount, SPELL_FAILED_NOT_READY);
            if (report)
                *report = "refused (cooldown " + std::to_string(p->GetSpellHistory()->GetCooldownDelay(spellId, proto->ItemId)) + " ms)";
            return true;
        }
        uint32 healed = 0, power = 0;
        if (def.kind != Arpg::Core::PotionKind::Mana)
        {
            healed = Arpg::Core::PotionHeal(p->GetMaxHealth(), def.pct);
            p->HealBySpell(p, si, healed);
        }
        if (def.kind != Arpg::Core::PotionKind::Healing)
            power = RestoreResource(p, def.pct);
        Rec(p, "potion drink item=" + std::to_string(def.itemId) + " pct=" + std::to_string(def.pct) + " heal=" + std::to_string(healed) +
            " power=" + std::to_string(uint32(p->GetPowerType())) + " amount=" + std::to_string(power));
        // Shared cooldown (server: the item's category cooldown from item_template; client: a cooldown packet per potion spell,
        // so every potion button shows the sweep).
        p->GetSpellHistory()->StartCooldown(si, proto->ItemId);
        uint32 cd = uint32(std::max(0, proto->Spells[0].SpellCategoryCooldown));
        std::vector<uint32> spells;
        {
            std::lock_guard<std::mutex> l(g_lock);
            for (auto const& kv : g_bySpell)
                spells.push_back(kv.first);
        }
        for (uint32 sp : spells)
            p->GetSpellHistory()->SendCooldown(sp, int32(cd));
        uint32 one = 1;
        p->DestroyItemCount(item, one, true);
        Spell::SendCastResult(p, si, castCount, SPELL_FAILED_DONT_REPORT);   // releases the item button, no message
        return true;
    }

    class ArpgPotionItemScript : public ItemScript
    {
    public:
        ArpgPotionItemScript() : ItemScript("arpg_potion") { }

        bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
        {
            return player && item && Drink(player, item, 0, nullptr);
        }
    };

    class ArpgPotionPlayerScript : public PlayerScript
    {
    public:
        ArpgPotionPlayerScript() : PlayerScript("arpg_potions") { }

        // Drops: one potion at most per kill, tier and chance by the monster's kind.
        void OnCreatureKill(Player* killer, Creature* killed) override
        {
            if (!killer || !killed)
                return;
            Arpg::Settings const& s = sArpgConfig.Get();
            if (!s.lootEnabled || (!Arpg::Scaling::IsRealPlayer(killer) && !s.affixBotItems))
                return;
            uint32 entry = killed->GetEntry();
            uint32 base = Arpg::Monsters::BaseEntry(entry);
            CreatureTemplate const* t = killed->GetCreatureTemplate();
            uint32 source = 0;
            if (Arpg::Loot::IsBossEntry(base))
                source = 4;
            else if (s.superuniqueEntryOffset && entry >= s.superuniqueEntryOffset && entry < s.superuniqueEntryOffset + 1000000)
                source = 3;
            else if (Arpg::Monsters::WasChampionKill(killed))
                source = 2;
            else if (t && (t->rank == CREATURE_ELITE_ELITE || t->rank == CREATURE_ELITE_RAREELITE))
                source = 1;
            Drop d;
            std::vector<Arpg::Core::PotionDef> potions;
            {
                std::lock_guard<std::mutex> l(g_lock);
                auto it = g_drops.find(source);
                if (it == g_drops.end())
                    return;
                d = it->second;
                potions = g_potions;
            }
            if (float(rand_chance()) >= d.chancePct * s.potionDropMult)
                return;
            uint32 total = d.wHeal + d.wMana + d.wRejuv;
            Arpg::Core::PotionKind kind = Arpg::Core::PickPotionKind(d.wHeal, d.wMana, d.wRejuv, total ? urand(0, total - 1) : 0);
            uint32 item = Arpg::Core::PotionForTier(potions, kind, d.tier);
            if (!item)
                item = Arpg::Core::PotionForTier(potions, Arpg::Core::PotionKind::Healing, d.tier);
            if (!item)
                return;
            Arpg::Monsters::AddLootItem(killed->loot, item);
            killed->SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            Rec(killer, "potion drop item=" + std::to_string(item) + " source=" + std::to_string(source) + " tier=" + std::to_string(d.tier));
        }
    };
}

namespace Arpg::Loot
{
    std::string LoadPotionData()
    {
        std::vector<Core::PotionDef> potions;
        std::unordered_map<uint32, Core::PotionDef> bySpell;
        std::unordered_map<uint32, Drop> drops;
        if (QueryResult r = WorldDatabase.Query("SELECT item_id, kind, tier, pct FROM arpg_potion"))
            do
            {
                Field* f = r->Fetch();
                Core::PotionDef d{ f[0].GetUInt32(), Core::PotionKind(f[1].GetUInt8()), int(f[2].GetUInt8()), int(f[3].GetUInt8()) };
                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(d.itemId);
                if (!proto)
                    continue;
                potions.push_back(d);
                for (auto const& sp : proto->Spells)
                    if (sp.SpellId > 0)
                        bySpell[uint32(sp.SpellId)] = d;
            } while (r->NextRow());
        if (QueryResult r = WorldDatabase.Query("SELECT source, tier, chance_pct, w_heal, w_mana, w_rejuv FROM arpg_potion_drop"))
            do
            {
                Field* f = r->Fetch();
                drops[f[0].GetUInt8()] = { int(f[1].GetUInt8()), f[2].GetFloat(), f[3].GetUInt16(), f[4].GetUInt16(), f[5].GetUInt16() };
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_potions = potions;
        g_bySpell = bySpell;
        g_drops = drops;
        return std::to_string(potions.size()) + " potions, " + std::to_string(drops.size()) + " potion drop rules";
    }

    void InstallPotions()
    {
        new ArpgPotionItemScript();
        new ArpgPotionPlayerScript();
    }

    std::string TestDrink(Player* p, uint32 itemId)
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto || proto->Spells[0].SpellId <= 0)
            return "error: not a usable item " + std::to_string(itemId);
        uint32 spellId = uint32(proto->Spells[0].SpellId);
        SpellInfo const* si = sSpellMgr->GetSpellInfo(spellId);
        if (!si)
            return "error: no spell " + std::to_string(spellId);
        Item* item = p->GetItemByEntry(itemId);
        if (!item)
        {
            ItemPosCountVec dest;
            if (p->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, 1) != EQUIP_ERR_OK)
                return "error: bags full";
            item = p->StoreNewItem(dest, itemId, true);
        }
        if (!item)
            return "error: could not create item";
        p->SetHealth(std::max<uint32>(1, p->GetMaxHealth() / 2));
        Powers pw = p->GetPowerType();
        p->SetPower(pw, p->GetMaxPower(pw) / 2);
        uint32 hp0 = p->GetHealth();
        int32 pw0 = p->GetPower(pw);
        std::string refused;
        if (!Drink(p, item, 0, &refused))
        {
            SpellCastTargets targets;   // not an ARPG potion: the core's own use path
            targets.SetUnitTarget(p);
            p->CastItemUseSpell(item, targets, 0, 0);
        }
        if (!refused.empty())
            return "drink item=" + std::to_string(itemId) + " " + refused;
        uint32 count = p->GetItemCount(itemId);
        // Expected heal by the rule (healing / rejuvenation potions), so a test can check it exactly.
        Core::PotionDef def{};
        bool known = PotionBySpell(spellId, def);
        uint32 expect = known && def.kind != Core::PotionKind::Mana ? Core::PotionHeal(p->GetMaxHealth(), def.pct) : 0;
        uint32 healed = p->GetHealth() - hp0;
        return "drink item=" + std::to_string(itemId) + " ok=1 heal=" + std::to_string(healed) + " expect=" + std::to_string(expect) +
            " match=" + (healed == expect ? "yes" : "no") + " max_health=" +
            std::to_string(p->GetMaxHealth()) + " power=" + std::to_string(p->GetPower(pw) - pw0) + " max_power=" +
            std::to_string(p->GetMaxPower(pw)) + " cooldown_ms=" + std::to_string(p->GetSpellHistory()->GetCooldownDelay(spellId, itemId)) +
            " left=" + std::to_string(count) + " stack=" + std::to_string(proto->GetMaxStackSize());
    }
}
