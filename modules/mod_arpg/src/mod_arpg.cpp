// mod_arpg entry point: Pandaria ARPG layer (see docs/SPEC.md). Called by the generated modules/ModulesLoader.cpp.
#include "Items/Stacks.h"
#include "Character/ProfessionGuard.h"
#include "Character/ProfessionShop.h"
#include "ArpgConfig.h"
#include "Scaling/WorldScaling.h"
#include "Loot/PersonalLoot.h"
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
#include "Character/AnyWeaponSkills.h"
#include "Character/StartQuests.h"
#include "Loot/AffixDisplayRepair.h"
#include "Loot/Crafting.h"
#include "Loot/MountShop.h"
#include "Bots/BotItemScore.h"
#include "Loot/AuctionRolls.h"
#include "Loot/LootBeam.h"
#include "Loot/Blanks.h"
#include "Charms/Charms.h"
#include "World/Shrines.h"
#include "Monsters/TreasureGoblin.h"
#include "Dungeons/Rifts.h"
#include "Commands/M9Commands.h"

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
            Arpg::Loot::LoadBlanks();
            Arpg::Character::ApplyAnyWeaponSkills();
            Arpg::Character::LoadStartQuests();
            Arpg::Charms::LoadCharmData();
            Arpg::World::LoadShrineData();
            Arpg::Dungeons::LoadRiftData();
            Arpg::Items::ApplyStacks();
            Arpg::Character::LoadProfessionShop();
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
    Arpg::Loot::InstallBlanks();
    Arpg::Character::InstallAnyWeapon();
    Arpg::Character::InstallStartQuests();
    Arpg::Loot::InstallAffixDisplayRepair();
    Arpg::Loot::InstallCatalystDrops();
    Arpg::Bots::InstallBotItemScore();
    Arpg::Loot::InstallAuctionRolls();
    Arpg::Loot::InstallLootBeam();
    Arpg::Character::InstallProfessionGuard();
    Arpg::Character::InstallProfessionShop();     // m10: companion profession trainers + recipe shop   // m9-stacks-trainer: refuse the client abandon, log changes
    Arpg::Charms::InstallCharms();        // m9: charm drops are kill extras (before the planner)
    Arpg::World::InstallShrines();
    Arpg::Monsters::InstallTreasureGoblins();
    Arpg::Dungeons::InstallRifts();
    Arpg::Loot::InstallLootPlanner();   // plans corpse loot after the others added items and pity
    Arpg::Loot::InstallPersonalLoot();  // last kill script (m8): friends' own loot, after every extra was queued
    AddSC_arpg_commandscript();
    AddSC_arpg_m9_commandscript();
}
