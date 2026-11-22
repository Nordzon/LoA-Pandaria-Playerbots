#include "Loot/NativeEnchants.h"

#include "DBCStores.h"
#include "Log.h"
#include "ObjectMgr.h"   // ItemPrototype.h (ITEM_MOD_*) through the core's own include order

#include <cstdio>

namespace
{
    Arpg::Core::EnchantIndex g_index;

    struct StatName { uint32 stat; char const* name; };
    StatName const AffixStats[] =
    {
        { ITEM_MOD_AGILITY, "agi" }, { ITEM_MOD_STRENGTH, "str" }, { ITEM_MOD_INTELLECT, "int" }, { ITEM_MOD_STAMINA, "sta" },
        { ITEM_MOD_SPIRIT, "spi" }, { ITEM_MOD_CRIT_RATING, "crit" }, { ITEM_MOD_HASTE_RATING, "haste" },
        { ITEM_MOD_MASTERY_RATING, "mastery" }, { ITEM_MOD_HIT_RATING, "hit" }, { ITEM_MOD_EXPERTISE_RATING, "exp" },
        { ITEM_MOD_DODGE_RATING, "dodge" }, { ITEM_MOD_PARRY_RATING, "parry" },
    };
}

namespace Arpg::Loot
{
    void BuildNativeEnchants()
    {
        g_index.Clear();
        uint32 used = 0;
        for (uint32 i = 0; i < sSpellItemEnchantmentStore.GetNumRows(); ++i)
        {
            SpellItemEnchantmentEntry const* e = sSpellItemEnchantmentStore.LookupEntry(i);
            if (!e || e->Type[0] != ITEM_ENCHANTMENT_TYPE_STAT || e->Type[1] != 0 || e->Type[2] != 0)
                continue;
            if (e->Amount[0] == 0 || e->ScalingLevel != 0 || e->GemID != 0 || e->EnchantmentCondition != 0 || e->RequiredSkill != 0)
                continue;
            g_index.Add(e->SpellID[0], e->Amount[0], e->ID);   // SpellID holds the ITEM_MOD_* stat for stat enchants
            ++used;
        }
        TC_LOG_INFO("server.loading", "mod_arpg: %u fixed-amount stat enchantments indexed for native affix display", used);
    }

    Core::EnchantIndex const& NativeEnchants()
    {
        return g_index;
    }

    std::string DescribeNativeEnchants()
    {
        std::string out;
        char buf[96];
        for (StatName const& s : AffixStats)
        {
            std::snprintf(buf, sizeof(buf), "%s n=%u %u-%u; ", s.name, uint32(g_index.Count(s.stat)),
                g_index.MinAmount(s.stat), g_index.MaxAmount(s.stat));
            out += buf;
        }
        return out;
    }
}
