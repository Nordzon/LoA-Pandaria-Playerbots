// mod_arpg settings: arpg.conf (next to worldserver.exe), reloadable with .arpg reload, overridable with .arpg config set.
#ifndef ARPG_CONFIG_H
#define ARPG_CONFIG_H

#include "Core/ConfigFile.h"
#include "Core/LevelCurve.h"
#include "Core/Scaling.h"
#include "Core/Paragon.h"
#include "M9Settings.h"

#include <mutex>
#include <set>
#include <vector>
#include <string>

#define ARPG_VERSION "0.1.0"
#define ARPG_MILESTONE "m1"

namespace Arpg
{
    struct Settings
    {
        Core::ScaleSettings scale;
        float scaleRadius = 100.0f;      // Arpg.Scale.Radius (yards)
        uint32_t scaleTickMs = 1000;     // Arpg.Scale.TickMs
        bool recordEvents = false;       // Arpg.Test.RecordEvents (event recorder for tests)
        bool forceProcs = false;         // Arpg.Test.ForceProcs (every chance-on-hit affix procs; tests only)
        Core::LevelCurve epicIlvlCurve;  // Arpg.Loot.EpicIlvlCurve (player level -> epic item level, for B)
        bool lootEnabled = true;         // Arpg.Loot.Enabled
        bool affixBotItems = false;      // Arpg.Loot.AffixBotItems
        int smartLootPct = 70;           // Arpg.Loot.SmartLootPct
        int scaleZoneCapAbove = 15;      // Arpg.Scale.ZoneCapAbove (monsters scale to at most zone max + this)
        std::set<uint32_t> uncappedZones{ 5840 }; // Arpg.Scale.UncappedZones (comma list; Vale of Eternal Blossoms)
        int lootZoneCeilingMargin = 15;  // Arpg.Loot.ZoneCeilingMargin (loot level <= zone max + this)
        int lootSubstituteGap = 5;       // Arpg.Loot.SubstituteGap (swap drops more than this below the loot level)
        uint32_t sellCopperPerAffixLevel = 200; // Arpg.Loot.SellCopperPerAffixLevel (vendor value per affix per item level)
        uint32_t championEntryOffset = 400000;     // Arpg.Champion.EntryOffset (tag copies "Champion <name>", gen_monster_tags.py)
        uint32_t superuniqueEntryOffset = 700000;  // Arpg.Superunique.EntryOffset (tag copies "Unique <name>")
        bool championEnabled = true;     // Arpg.Champion.Enabled
        float championChancePct = 0.0f;  // Arpg.Champion.Chance (% extra random Champions on top of camps, rolled once per life on first sight)
        float championScale = 1.25f;     // Arpg.Champion.Scale (model size multiplier)
        uint32_t championVisualAura = 117230; // Arpg.Champion.VisualAura (glow aura spell; owner pick 2026-11-26 "Empowered"; 0 = none)
        uint32_t superuniqueVisualAura = 117230; // Arpg.Superunique.VisualAura (same glow on Superuniques; 0 = none)
        uint32_t lootBeamAura = 120720;    // Arpg.Loot.BeamAura (light beam on corpses worth looting; owner pick; 0 = none)
        uint32_t lootBeamMinQuality = 2;   // Arpg.Loot.BeamMinQuality (2 green, 3 blue ...)
        float championLootBlueChancePct = 25.0f; // Arpg.Champion.LootBlueChance (extra blue in the loot floor)
        uint32_t championChillSpell = 6136;      // Arpg.Champion.ChillSpell (Cold Enchanted slow aura)
        float championSparkChancePct = 25.0f;    // Arpg.Champion.SparkChance (Lightning Enchanted, per hit)
        uint32_t championNukeSpell = 133;        // Arpg.Champion.NukeSpell (Arcane Caster cast; damage replaced by % of max health)
        uint32_t championNukeEveryMs = 8000;     // Arpg.Champion.NukeEveryMs
        float championExplosionRadius = 8.0f;    // Arpg.Champion.ExplosionRadius (Fire Enchanted, yards)
        uint32_t championExplosionDelayMs = 1500; // Arpg.Champion.ExplosionDelayMs (time to step out)
        bool superuniqueEnabled = true;          // Arpg.Superunique.Enabled
        int superuniquePerZone = 3;              // Arpg.Superunique.PerZone
        uint32_t superuniqueRespawnDelaySec = 300; // Arpg.Superunique.RespawnDelay
        float superuniqueUniqueChancePct = 10.0f;  // Arpg.Superunique.UniqueChance
        float superuniqueUniqueStepPct = 5.0f;     // Arpg.Superunique.UniqueBadLuckStep
        bool superuniqueSharedBadLuck = true;      // Arpg.Superunique.SharedBadLuck (one counter for all Superuniques)
        float superuniqueScale = 1.4f;             // Arpg.Superunique.Scale
        float superuniquePatrolRadius = 15.0f;     // Arpg.Superunique.PatrolRadius (yards around its point; 0 = stand)
        bool superuniqueAnnounce = true;           // Arpg.Superunique.Announce (zone message on spawn)
        uint32_t companionNpcFlags = 0;        // computed from Arpg.Companion.* (vendor, repair, bank, mail, reforge, transmog, void)
        bool companionInCombat = false;        // Arpg.Companion.InCombat (roles also work in combat)
        bool anyWeaponEnabled = true;          // Arpg.AnyWeapon.Enabled (every weapon proficiency + Shield for real players)
        bool anyWeaponBots = false;            // Arpg.AnyWeapon.Bots
        bool anyWeaponDualWield = false;       // Arpg.AnyWeapon.DualWield
        bool startQuestsEnabled = true;        // Arpg.StartQuests.Enabled (new race/class pairs borrow the Warrior start quests)
        bool anyWeaponSkills = true;           // Arpg.AnyWeapon.Skills (class spells take any melee weapon; startup / .arpg reload)
        std::vector<uint32_t> anyWeaponMainHandSpells{ 1329, 60103, 100130 }; // Arpg.AnyWeapon.MainHandSpells (no off hand needed)
        bool lootPetEnabled = true;            // Arpg.LootPet.Enabled (players may turn their companion on)
        bool lootPetWaitForCombatEnd = true;   // Arpg.LootPet.WaitForCombatEnd
        uint32_t lootPetReachTimeoutMs = 3000; // Arpg.LootPet.ReachTimeoutMs (then loots from where it is)
        bool dungeonEnabled = true;            // Arpg.Dungeon.Enabled (m5 party table / raid tiers / solo roles / level selector)
        float dungeonSoloNonTankDamage = 0.75f;   // Arpg.Dungeon.SoloNonTankDamage (creature damage, alone and not a tank)
        float dungeonSoloTankDamageDealt = 1.25f; // Arpg.Dungeon.SoloTankDamageDealt
        float dungeonSoloHealerDamageDealt = 1.6f; // Arpg.Dungeon.SoloHealerDamageDealt
        float dungeonRaidTierStep = 0.15f;     // Arpg.Dungeon.RaidTierStep (raid extra multiplier 1 + step x tier)
        bool paragonEnabled = true;      // Arpg.Paragon.Enabled
        double paragonXpBase = 600000.0; // Arpg.Paragon.XpBase (XP from Paragon 0 to 1)
        double paragonXpGrowth = 1.04;   // Arpg.Paragon.XpGrowth (each level needs this much more)
        float paragonQuestXpPct = 100.0f; // Arpg.Paragon.QuestXpPct (share of a quest's XP at the max level)
        float paragonWorldTierPct = 2.0f;     // Arpg.Paragon.WorldTierPct (uncapped zones: monster health/damage and Paragon XP % per Paragon level)
        float paragonWorldTierCapPct = 100.0f; // Arpg.Paragon.WorldTierCapPct
        Arpg::Core::WorldTierMode paragonWorldTierMode = Arpg::Core::WorldTierMode::Highest; // Arpg.Paragon.WorldTierMode (m8)
        bool personalLoot = true;              // Arpg.Party.PersonalLoot (m8): other real players in the group get their own loot
        float personalLootRange = 100.0f;      // Arpg.Party.PersonalLootRange (yards from the corpse)
        float paragonStatPctPerPoint = 0.5f;   // Arpg.Paragon.StatPctPerPoint (Core: main stat / stamina %)
        int paragonRatingPerPoint = 240;       // Arpg.Paragon.RatingPerPoint (Offense crit/haste/mastery, Defense dodge/parry)
        float paragonArmorPctPerPoint = 0.4f;  // Arpg.Paragon.ArmorPctPerPoint (Defense)
        int paragonMoveSpeedPerPoint = 100;    // Arpg.Paragon.MoveSpeedPerPoint (hundredths of %; cap MoveSpeedCap)
        int paragonMoveSpeedCap = 1000;        // Arpg.Paragon.MoveSpeedCap (+10%)
        int paragonFindPerPoint = 500;         // Arpg.Paragon.FindPerPoint (magic/gold find, hundredths of %)
        int paragonMagicFindCap = 10000;       // Arpg.Paragon.MagicFindCap (+100%)
        float ahAffixSharePct = 30.0f;      // Arpg.AH.AffixShare (% of green-epic gear listings that carry affixes)
        int ahTierOffset = -1;              // Arpg.AH.TierOffset (tiers vs the item's level)
        float ahAffixMarkup = 4.0f;         // Arpg.AH.AffixMarkup (buyout = sell formula x this, never below the core's)
        bool botPartyGear = true;           // Arpg.Bots.PartyGear (bots in your group get affixed gear)
        int botPartyTierOffset = -1;        // Arpg.Bots.PartyAffixTierOffset (their tiers vs yours)
        bool botAffixScoring = true;        // Arpg.Bots.AffixScoring (party bots count affixes when choosing gear)
        bool mountShopEnabled = true;       // Arpg.MountShop.Enabled (companion dialogue)
        uint32_t mountShopPriceGold = 5000; // Arpg.MountShop.Price (gold; a row's price_gold overrides)
        // Typed blank bases (m6-blank-bases, owner 2026-11-27 "all of these")
        bool blankEnabled = true;           // Arpg.Blank.Enabled
        uint32_t blankDropChance = 10;      // Arpg.Blank.DropChance (% per affix-eligible gear drop)
        bool blankCrafted = true;           // Arpg.Blank.Crafted (crafted gear comes out as its blank)
        uint32_t blankEssenceGreen = 5;     // Arpg.Blank.EssenceCost.Green (companion shop)
        uint32_t blankEssenceBlue = 15;     // Arpg.Blank.EssenceCost.Blue
        uint32_t blankEssenceEpic = 45;     // Arpg.Blank.EssenceCost.Epic
        int salvageEssencePerAffix = 1;     // Arpg.Salvage.EssencePerAffix
        int salvageUniqueBonus = 5;         // Arpg.Salvage.UniqueBonus
        int salvageAscendedBonus = 3;       // Arpg.Salvage.AscendedBonus
        int salvageMaxQuality = 3;          // Arpg.Salvage.MaxQuality (3 = blue)
        bool salvageIncludeUniques = false; // Arpg.Salvage.IncludeUniques
        int catalystCostLesser = 10;        // Arpg.Catalyst.EssenceCostLesser (essences per catalyst at the companion)
        int catalystCostGreater = 30;       // Arpg.Catalyst.EssenceCostGreater
        int catalystCostGrand = 90;         // Arpg.Catalyst.EssenceCostGrand
        float catalystChampionChancePct = 25.0f;     // Arpg.Catalyst.ChampionChance (% per Champion leader kill)
        float catalystSuperuniqueChancePct = 100.0f; // Arpg.Catalyst.SuperuniqueChance
        float potionDropMult = 1.0f;     // Arpg.Potion.DropMult (x arpg_potion_drop.chance_pct; tests set 100 to force)
        int craftMaxSkill = 600;         // Arpg.Craft.MaxSkill (at or above: crafted items roll +1 affix)
        float craftUniqueChancePct = 2.0f;   // Arpg.Craft.UniqueChance (orange/yellow craft -> Unique of its slot)
        float craftUniquePityStepPct = 1.0f; // Arpg.Craft.UniquePityStep (+ per craft without one, per profession)
        int godRollPityEvery = 3;        // Arpg.Loot.GodRollPityEvery (every Nth boss kill without a 5-affix item forces one)
        bool displayEnabled = true;      // Arpg.Display.Enabled (affix lines from our client patch rows)
        uint32_t displayBase = 60000;    // Arpg.Display.EnchantBase   (must match the client patch)
        uint32_t displayStride = 2000;   // Arpg.Display.EnchantStride (must match the client patch)
        uint32_t displayEffectBase = 100000; // Arpg.Display.EffectBase (must match the client patch)
        uint32_t displaySuffixBase = 30000;  // Arpg.Display.SuffixBase (random-suffix rows: 5 lines + item name)
        uint32_t displayUniqueBase = 200000; // Arpg.Display.UniqueBase (Unique power lines: base + power*1000 + tier*100 + roll)
        M9Settings m9;                   // charms, shrines, Treasure Goblins, rifts (Arpg.Charms./Shrine./Goblin./Rift.*)
    };

    class Config
    {
    public:
        static Config& Instance();

        // (Re)reads arpg.conf, then re-applies .arpg config set overrides. Returns false (keeping old values) on error.
        bool Load(std::string& message);
        // .arpg config set: in-memory override (kept across .arpg reload, lost on restart). False if the key is unknown.
        bool SetOverride(std::string const& key, std::string const& value, std::string& message);
        // Drops every ".arpg config set" override and reloads arpg.conf (the test runner calls it at the end of a run).
        bool ClearOverrides(std::string& message);

        Settings Get() const { std::lock_guard<std::mutex> l(_lock); return _settings; }
        std::string Path() const { return _path; }

    private:
        void Apply(Core::ConfigFile const& file);

        mutable std::mutex _lock;
        Settings _settings;
        Core::ConfigFile _overrides;
        std::string _path = "arpg.conf";
    };
}

#define sArpgConfig Arpg::Config::Instance()

#endif
