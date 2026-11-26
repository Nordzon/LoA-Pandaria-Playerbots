// mod_arpg entry point: Pandaria ARPG layer (see docs/SPEC.md). Called by the generated modules/ModulesLoader.cpp.
#include "ArpgConfig.h"
#include "Scaling/WorldScaling.h"
#include "Loot/NativeEnchants.h"
#include "Loot/AffixRoller.h"
#include "Loot/AffixStats.h"
#include "Loot/AffixEffects.h"
#include "Loot/ZoneLevels.h"
#include "Loot/Substitution.h"
#include "Loot/BossPity.h"
#include "Loot/Uniques.h"
#include "Monsters/Champions.h"
#include "Monsters/Superuniques.h"
#include "Loot/Potions.h"
#include "Paragon/Paragon.h"
#include "Dungeons/InstanceScaling.h"
#include "Loot/LootPet.h"
#include "Character/AnyWeapon.h"
#include "Loot/Crafting.h"
#include "Loot/MountShop.h"
#include "Bots/BotItemScore.h"

#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"

void AddSC_arpg_commandscript();

namespace
{
    class ArpgWorldScript : public WorldScript
    {
    public:
        ArpgWorldScript() : WorldScript("arpg_world") { }

        // Startup and ".reload config": (re)read arpg.conf.
        void OnConfigLoad(bool /*reload*/) override
        {
            std::string msg;
            sArpgConfig.Load(msg);
        }

        // DBC stores are loaded by now.
        void OnStartup() override
        {
            Arpg::Loot::BuildNativeEnchants();
            Arpg::Loot::LoadAffixData();
            Arpg::Loot::CleanOrphanAffixes();
            Arpg::Loot::LoadStatCache();
            Arpg::Loot::LoadEffectData();
            Arpg::Loot::LoadZoneLevels();
            Arpg::Loot::LoadSubstitutionIndex();
            Arpg::Loot::LoadUniques();
            Arpg::Monsters::LoadChampionData();
            Arpg::Monsters::LoadSuperuniqueData();
            Arpg::Loot::LoadPotionData();
            Arpg::Dungeons::LoadInstanceData();
            Arpg::Loot::LoadCompanionModels();
            Arpg::Loot::LoadMountShop();
        }
    };
}

void AddSC_mod_arpg()
{
    new ArpgWorldScript();
    Arpg::Scaling::Install();
    Arpg::Loot::InstallPickupHook();
    Arpg::Loot::InstallAffixStats();
    Arpg::Loot::InstallAffixEffects();
    Arpg::Loot::InstallSubstitution();
    Arpg::Loot::InstallBossPity();
    Arpg::Monsters::InstallChampions();
    Arpg::Monsters::InstallSuperuniques();
    Arpg::Loot::InstallPotions();
    Arpg::Paragon::InstallParagon();
    Arpg::Dungeons::InstallInstanceScaling();
    Arpg::Loot::InstallLootPet();
    Arpg::Character::InstallAnyWeapon();
    Arpg::Loot::InstallCatalystDrops();
    Arpg::Bots::InstallBotItemScore();
    Arpg::Loot::InstallLootPlanner();   // last kill script: plans corpse loot after the others added items and pity
    AddSC_arpg_commandscript();
}
