// Tunables of charms, shrines, Treasure Goblins and rifts (changes m9-*). Read from arpg.conf by ArpgConfig::Apply; every key
// starting with Arpg.Charms. / Arpg.Shrine. / Arpg.Goblin. / Arpg.Rift. is accepted by ".arpg config set".
#ifndef ARPG_M9SETTINGS_H
#define ARPG_M9SETTINGS_H

#include "Core/ConfigFile.h"

#include <algorithm>
#include <cstdint>

namespace Arpg
{
    struct M9Settings
    {
        // Charms
        bool charmsEnabled = true;          // Arpg.Charms.Enabled
        bool charmsGrantPouch = true;       // Arpg.Charms.GrantPouch (login: a character without a pouch gets one)
        float charmDrop[5] = { 0.3f, 1.5f, 6.0f, 20.0f, 12.0f };  // Arpg.Charms.DropNormal/Elite/Champion/Superunique/Boss (%)
        int charmWeight[3] = { 70, 25, 5 }; // Arpg.Charms.WeightSmall/Large/Grand
        float charmUniqueSuperuniquePct = 2.0f;  // Arpg.Charms.UniqueSuperunique (% Heart of the Pandaren per Superunique kill)
        uint32_t charmScanMs = 1000;        // Arpg.Charms.ScanMs (how often the pouch is checked)

        // Shrines
        bool shrineEnabled = true;          // Arpg.Shrine.Enabled
        float shrineKillChancePct = 1.0f;   // Arpg.Shrine.KillChance
        uint32_t shrineCooldownS = 240;     // Arpg.Shrine.Cooldown
        uint32_t shrineLifetimeS = 180;     // Arpg.Shrine.Lifetime
        bool shrineInDungeons = true;       // Arpg.Shrine.InDungeons
        float shrineGroupRange = 40.0f;     // Arpg.Shrine.GroupRange

        // Treasure Goblins
        bool goblinEnabled = true;          // Arpg.Goblin.Enabled
        float goblinKillChancePct = 0.4f;   // Arpg.Goblin.KillChance
        uint32_t goblinCooldownS = 600;     // Arpg.Goblin.Cooldown
        uint32_t goblinEscapeS = 30;        // Arpg.Goblin.EscapeSeconds (after the first hit)
        uint32_t goblinLifetimeS = 180;     // Arpg.Goblin.Lifetime (never engaged)
        float goblinHealthMult = 8.0f;      // Arpg.Goblin.HealthMult
        uint32_t goblinGoldPerLevel = 3000; // Arpg.Goblin.GoldPerLevel (copper x level)
        int goblinItems = 4;                // Arpg.Goblin.Items
        float goblinBluePct = 50.0f;        // Arpg.Goblin.BlueChance
        float goblinEpicPct = 10.0f;        // Arpg.Goblin.EpicChance
        int goblinEssences = 6;             // Arpg.Goblin.Essences
        float goblinCatalystPct = 25.0f;    // Arpg.Goblin.CatalystChance
        float goblinCharmPct = 35.0f;       // Arpg.Goblin.CharmChance
        float goblinUniqueCharmPct = 5.0f;  // Arpg.Goblin.UniqueCharmChance

        // Rifts
        bool riftEnabled = true;            // Arpg.Rift.Enabled
        float riftHealthGrowth = 1.10f;     // Arpg.Rift.HealthGrowth (x per level)
        float riftDamageGrowth = 1.07f;     // Arpg.Rift.DamageGrowth
        int riftAffixLevel = 2;             // Arpg.Rift.AffixLevel (Fortified / Tyrannical from this level)
        uint32_t riftDefaultParMin = 25;    // Arpg.Rift.DefaultParMinutes
        uint32_t riftDeathPenaltyS = 10;    // Arpg.Rift.DeathPenalty
        int riftRewardItems = 2;            // Arpg.Rift.RewardItems (+ level / 3)
        int riftRewardItemsMax = 8;         // Arpg.Rift.RewardItemsMax
        float riftEpicBasePct = 10.0f;      // Arpg.Rift.EpicChance
        float riftEpicStepPct = 3.0f;       // Arpg.Rift.EpicChancePerLevel
        float riftEpicCapPct = 60.0f;       // Arpg.Rift.EpicChanceCap
        uint32_t riftGoldPerLevel = 50;     // Arpg.Rift.GoldPerLevel (copper x rift level x character level)
        uint32_t riftParagonXpPerLevel = 40000; // Arpg.Rift.ParagonXpPerLevel
        float riftCharmPct = 20.0f;         // Arpg.Rift.CharmChance
        float riftCharmStepPct = 2.0f;      // Arpg.Rift.CharmChancePerLevel
        int riftUniqueCharmLevel = 10;      // Arpg.Rift.UniqueCharmLevel
        float riftUniqueCharmPct = 10.0f;   // Arpg.Rift.UniqueCharmChance

        void Load(Core::ConfigFile const& f)
        {
            auto pos = [](float v) { return std::max(0.0f, v); };
            charmsEnabled = f.GetBool("Arpg.Charms.Enabled", true);
            charmsGrantPouch = f.GetBool("Arpg.Charms.GrantPouch", true);
            charmDrop[0] = pos(f.GetFloat("Arpg.Charms.DropNormal", 0.3f));
            charmDrop[1] = pos(f.GetFloat("Arpg.Charms.DropElite", 1.5f));
            charmDrop[2] = pos(f.GetFloat("Arpg.Charms.DropChampion", 6.0f));
            charmDrop[3] = pos(f.GetFloat("Arpg.Charms.DropSuperunique", 20.0f));
            charmDrop[4] = pos(f.GetFloat("Arpg.Charms.DropBoss", 12.0f));
            charmWeight[0] = std::max(0, f.GetInt("Arpg.Charms.WeightSmall", 70));
            charmWeight[1] = std::max(0, f.GetInt("Arpg.Charms.WeightLarge", 25));
            charmWeight[2] = std::max(0, f.GetInt("Arpg.Charms.WeightGrand", 5));
            charmUniqueSuperuniquePct = pos(f.GetFloat("Arpg.Charms.UniqueSuperunique", 2.0f));
            charmScanMs = uint32_t(std::max(200, f.GetInt("Arpg.Charms.ScanMs", 1000)));

            shrineEnabled = f.GetBool("Arpg.Shrine.Enabled", true);
            shrineKillChancePct = pos(f.GetFloat("Arpg.Shrine.KillChance", 1.0f));
            shrineCooldownS = uint32_t(std::max(0, f.GetInt("Arpg.Shrine.Cooldown", 240)));
            shrineLifetimeS = uint32_t(std::max(5, f.GetInt("Arpg.Shrine.Lifetime", 180)));
            shrineInDungeons = f.GetBool("Arpg.Shrine.InDungeons", true);
            shrineGroupRange = std::max(0.0f, f.GetFloat("Arpg.Shrine.GroupRange", 40.0f));

            goblinEnabled = f.GetBool("Arpg.Goblin.Enabled", true);
            goblinKillChancePct = pos(f.GetFloat("Arpg.Goblin.KillChance", 0.4f));
            goblinCooldownS = uint32_t(std::max(0, f.GetInt("Arpg.Goblin.Cooldown", 600)));
            goblinEscapeS = uint32_t(std::max(1, f.GetInt("Arpg.Goblin.EscapeSeconds", 30)));
            goblinLifetimeS = uint32_t(std::max(5, f.GetInt("Arpg.Goblin.Lifetime", 180)));
            goblinHealthMult = std::max(0.1f, f.GetFloat("Arpg.Goblin.HealthMult", 8.0f));
            goblinGoldPerLevel = uint32_t(std::max(0, f.GetInt("Arpg.Goblin.GoldPerLevel", 3000)));
            goblinItems = std::clamp(f.GetInt("Arpg.Goblin.Items", 4), 0, 12);
            goblinBluePct = pos(f.GetFloat("Arpg.Goblin.BlueChance", 50.0f));
            goblinEpicPct = pos(f.GetFloat("Arpg.Goblin.EpicChance", 10.0f));
            goblinEssences = std::clamp(f.GetInt("Arpg.Goblin.Essences", 6), 0, 200);
            goblinCatalystPct = pos(f.GetFloat("Arpg.Goblin.CatalystChance", 25.0f));
            goblinCharmPct = pos(f.GetFloat("Arpg.Goblin.CharmChance", 35.0f));
            goblinUniqueCharmPct = pos(f.GetFloat("Arpg.Goblin.UniqueCharmChance", 5.0f));

            riftEnabled = f.GetBool("Arpg.Rift.Enabled", true);
            riftHealthGrowth = std::max(1.0f, f.GetFloat("Arpg.Rift.HealthGrowth", 1.10f));
            riftDamageGrowth = std::max(1.0f, f.GetFloat("Arpg.Rift.DamageGrowth", 1.07f));
            riftAffixLevel = std::max(1, f.GetInt("Arpg.Rift.AffixLevel", 2));
            riftDefaultParMin = uint32_t(std::max(1, f.GetInt("Arpg.Rift.DefaultParMinutes", 25)));
            riftDeathPenaltyS = uint32_t(std::max(0, f.GetInt("Arpg.Rift.DeathPenalty", 10)));
            riftRewardItems = std::clamp(f.GetInt("Arpg.Rift.RewardItems", 2), 0, 20);
            riftRewardItemsMax = std::clamp(f.GetInt("Arpg.Rift.RewardItemsMax", 8), 1, 20);
            riftEpicBasePct = pos(f.GetFloat("Arpg.Rift.EpicChance", 10.0f));
            riftEpicStepPct = pos(f.GetFloat("Arpg.Rift.EpicChancePerLevel", 3.0f));
            riftEpicCapPct = pos(f.GetFloat("Arpg.Rift.EpicChanceCap", 60.0f));
            riftGoldPerLevel = uint32_t(std::max(0, f.GetInt("Arpg.Rift.GoldPerLevel", 50)));
            riftParagonXpPerLevel = uint32_t(std::max(0, f.GetInt("Arpg.Rift.ParagonXpPerLevel", 40000)));
            riftCharmPct = pos(f.GetFloat("Arpg.Rift.CharmChance", 20.0f));
            riftCharmStepPct = pos(f.GetFloat("Arpg.Rift.CharmChancePerLevel", 2.0f));
            riftUniqueCharmLevel = std::max(1, f.GetInt("Arpg.Rift.UniqueCharmLevel", 10));
            riftUniqueCharmPct = pos(f.GetFloat("Arpg.Rift.UniqueCharmChance", 10.0f));
        }

        static bool KnownKey(std::string const& key)
        {
            for (char const* p : { "Arpg.Charms.", "Arpg.Shrine.", "Arpg.Goblin.", "Arpg.Rift." })
                if (key.rfind(p, 0) == 0)
                    return true;
            return false;
        }
    };
}

#endif
