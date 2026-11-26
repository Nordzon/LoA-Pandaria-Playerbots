#include "Character/AnyWeapon.h"

#include "ArpgConfig.h"
#include "Scaling/WorldScaling.h"

#include "DBCStores.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"

namespace Arpg::Character
{
    namespace
    {
        // Proficiency spell -> its skill line. The spell makes the client show the item as usable; the skill is what the
        // server's equip check reads. Learning the spell alone doesn't add the skill for a class the race/class table
        // (SkillRaceClassInfo.dbc) doesn't list, so the skill is set here too.
        struct Proficiency { uint32 spell; uint32 skill; };
        Proficiency const Proficiencies[] =
        {
            { 196, 44 },    // One-Handed Axes
            { 197, 172 },   // Two-Handed Axes
            { 198, 54 },    // One-Handed Maces
            { 199, 160 },   // Two-Handed Maces
            { 200, 229 },   // Polearms
            { 201, 43 },    // One-Handed Swords
            { 202, 55 },    // Two-Handed Swords
            { 227, 136 },   // Staves
            { 1180, 173 },  // Daggers
            { 15590, 473 }, // Fist Weapons
            { 264, 45 },    // Bows
            { 266, 46 },    // Guns
            { 5011, 226 },  // Crossbows
            { 5009, 228 },  // Wands
            { 9116, 433 },  // Shield
        };
        uint32 const DualWieldSpell = 674;

        bool Applies(Player* player)
        {
            Arpg::Settings const& s = sArpgConfig.Get();
            if (!s.anyWeaponEnabled)
                return false;
            return s.anyWeaponBots || Arpg::Scaling::IsRealPlayer(player);
        }

        void SetSkillFull(Player* player, uint32 skillId)
        {
            SkillLineEntry const* line = sSkillLineStore.LookupEntry(skillId);
            if (!line)
                return;
            if (GetSkillRangeType(line, false) == SKILL_RANGE_MONO)
                player->SetSkill(skillId, 0, 1, 1);
            else
            {
                uint16 max = player->GetMaxSkillValueForLevel();
                player->SetSkill(skillId, 0, max, max);
            }
        }

        class AnyWeaponPlayerScript : public PlayerScript
        {
        public:
            AnyWeaponPlayerScript() : PlayerScript("arpg_any_weapon") { }

            void OnLogin(Player* player) override { ApplyAnyWeapon(player); }
            void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override { ApplyAnyWeapon(player); }
        };
    }

    int ApplyAnyWeapon(Player* player)
    {
        if (!player || !Applies(player))
            return 0;
        int added = 0;
        for (Proficiency const& p : Proficiencies)
        {
            if (!sSpellMgr->GetSpellInfo(p.spell))
                continue;
            if (!player->HasSpell(p.spell))
            {
                player->LearnSpell(p.spell, false);
                ++added;
            }
            if (!player->HasSkill(p.skill))
            {
                SetSkillFull(player, p.skill);
                ++added;
            }
        }
        if (sArpgConfig.Get().anyWeaponDualWield && sSpellMgr->GetSpellInfo(DualWieldSpell))
        {
            if (!player->HasSpell(DualWieldSpell))
            {
                player->LearnSpell(DualWieldSpell, false);
                ++added;
            }
            player->SetCanDualWield(true);
        }
        return added;
    }

    std::string DescribeAnyWeapon(Player* player)
    {
        int known = 0, total = 0;
        for (Proficiency const& p : Proficiencies)
        {
            ++total;
            if (player->HasSpell(p.spell) && player->HasSkill(p.skill))
                ++known;
        }
        return "proficiencies=" + std::to_string(known) + "/" + std::to_string(total)
            + " dualwield=" + std::to_string(player->CanDualWield() ? 1 : 0)
            + " applies=" + std::to_string(Applies(player) ? 1 : 0);
    }

    void InstallAnyWeapon()
    {
        new AnyWeaponPlayerScript();
    }
}
