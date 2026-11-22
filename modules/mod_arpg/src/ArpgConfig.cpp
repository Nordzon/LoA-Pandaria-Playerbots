#include "ArpgConfig.h"

#include "Log.h"
#include "World.h"

#include <algorithm>
#include <set>

namespace
{
    // Keys .arpg config set accepts (the ones Apply reads).
    std::set<std::string> const KnownKeys =
    {
        "Arpg.Scale.Enabled", "Arpg.Scale.LevelOffset", "Arpg.Scale.Radius", "Arpg.Scale.HealthMod",
        "Arpg.Scale.DamageMod", "Arpg.Scale.TickMs", "Arpg.Test.RecordEvents", "Arpg.Loot.EpicIlvlCurve",
        "Arpg.Loot.Enabled", "Arpg.Loot.AffixBotItems", "Arpg.Loot.SmartLootPct", "Arpg.Display.Enabled",
        "Arpg.Display.EnchantBase", "Arpg.Display.EnchantStride", "Arpg.Display.EffectBase", "Arpg.Display.SuffixBase", "Arpg.Display.UniqueBase", "Arpg.Test.ForceProcs", "Arpg.Scale.ZoneCapAbove", "Arpg.Scale.UncappedZones",
        "Arpg.Loot.ZoneCeilingMargin", "Arpg.Loot.SubstituteGap", "Arpg.Loot.SellCopperPerAffixLevel", "Arpg.Loot.GodRollPityEvery",
        "Arpg.Craft.MaxSkill", "Arpg.Craft.UniqueChance", "Arpg.Craft.UniquePityStep", "Arpg.Potion.DropMult",
        "Arpg.Paragon.Enabled", "Arpg.Paragon.XpBase", "Arpg.Paragon.XpGrowth", "Arpg.Paragon.QuestXpPct",
        "Arpg.Superunique.Enabled", "Arpg.Superunique.PerZone", "Arpg.Superunique.RespawnDelay", "Arpg.Superunique.UniqueChance",
        "Arpg.Superunique.UniqueBadLuckStep", "Arpg.Superunique.Scale", "Arpg.Champion.EntryOffset", "Arpg.Superunique.EntryOffset",
        "Arpg.Superunique.PatrolRadius", "Arpg.Superunique.Announce",
        "Arpg.Champion.Enabled", "Arpg.Champion.Chance", "Arpg.Champion.Scale", "Arpg.Champion.VisualAura", "Arpg.Champion.LootBlueChance",
        "Arpg.Champion.ChillSpell", "Arpg.Champion.SparkChance", "Arpg.Champion.NukeSpell", "Arpg.Champion.NukeEveryMs",
        "Arpg.Champion.ExplosionRadius", "Arpg.Champion.ExplosionDelayMs",
    };
    char const* const DefaultEpicIlvlCurve = "1:6,59:64,60:66,70:115,80:213,85:359,90:463";
}

namespace Arpg
{
    Config& Config::Instance()
    {
        static Config instance;
        return instance;
    }

    void Config::Apply(Core::ConfigFile const& file)
    {
        Settings s;
        s.scale.enabled = file.GetBool("Arpg.Scale.Enabled", true);
        s.scale.levelOffset = file.GetInt("Arpg.Scale.LevelOffset", 0);
        s.scale.healthMod = file.GetFloat("Arpg.Scale.HealthMod", 1.0f);
        s.scale.damageMod = file.GetFloat("Arpg.Scale.DamageMod", 1.0f);
        s.scale.maxLevel = int(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
        s.scaleRadius = file.GetFloat("Arpg.Scale.Radius", 100.0f);
        s.scaleTickMs = uint32_t(std::max(200, file.GetInt("Arpg.Scale.TickMs", 1000)));
        s.recordEvents = file.GetBool("Arpg.Test.RecordEvents", false);
        s.forceProcs = file.GetBool("Arpg.Test.ForceProcs", false);
        s.scaleZoneCapAbove = std::max(0, file.GetInt("Arpg.Scale.ZoneCapAbove", 15));
        s.lootZoneCeilingMargin = std::max(0, file.GetInt("Arpg.Loot.ZoneCeilingMargin", 15));
        s.lootSubstituteGap = std::max(0, file.GetInt("Arpg.Loot.SubstituteGap", 5));
        s.sellCopperPerAffixLevel = uint32_t(std::max(0, file.GetInt("Arpg.Loot.SellCopperPerAffixLevel", 200)));
        s.godRollPityEvery = std::max(1, file.GetInt("Arpg.Loot.GodRollPityEvery", 3));
        s.craftMaxSkill = std::max(1, file.GetInt("Arpg.Craft.MaxSkill", 600));
        s.potionDropMult = std::max(0.0f, file.GetFloat("Arpg.Potion.DropMult", 1.0f));
        s.paragonEnabled = file.GetBool("Arpg.Paragon.Enabled", true);
        s.paragonXpBase = std::max(1.0, double(file.GetFloat("Arpg.Paragon.XpBase", 600000.0f)));
        s.paragonXpGrowth = std::max(1.0, double(file.GetFloat("Arpg.Paragon.XpGrowth", 1.04f)));
        s.paragonQuestXpPct = std::max(0.0f, file.GetFloat("Arpg.Paragon.QuestXpPct", 100.0f));
        s.superuniqueEnabled = file.GetBool("Arpg.Superunique.Enabled", true);
        s.superuniquePerZone = std::max(0, file.GetInt("Arpg.Superunique.PerZone", 1));
        s.superuniqueRespawnDelaySec = uint32_t(std::max(0, file.GetInt("Arpg.Superunique.RespawnDelay", 300)));
        s.superuniqueUniqueChancePct = std::max(0.0f, file.GetFloat("Arpg.Superunique.UniqueChance", 10.0f));
        s.superuniqueUniqueStepPct = std::max(0.0f, file.GetFloat("Arpg.Superunique.UniqueBadLuckStep", 5.0f));
        s.superuniqueScale = std::max(0.0f, file.GetFloat("Arpg.Superunique.Scale", 1.4f));
        s.superuniquePatrolRadius = std::max(0.0f, file.GetFloat("Arpg.Superunique.PatrolRadius", 15.0f));
        s.superuniqueAnnounce = file.GetBool("Arpg.Superunique.Announce", true);
        s.championEntryOffset = uint32_t(std::max(0, file.GetInt("Arpg.Champion.EntryOffset", 5000000)));
        s.superuniqueEntryOffset = uint32_t(std::max(0, file.GetInt("Arpg.Superunique.EntryOffset", 6000000)));
        s.championEnabled = file.GetBool("Arpg.Champion.Enabled", true);
        s.championChancePct = std::max(0.0f, file.GetFloat("Arpg.Champion.Chance", 2.0f));
        s.championScale = std::max(0.0f, file.GetFloat("Arpg.Champion.Scale", 1.25f));
        s.championVisualAura = uint32(std::max(0, file.GetInt("Arpg.Champion.VisualAura", 0)));
        s.championLootBlueChancePct = std::max(0.0f, file.GetFloat("Arpg.Champion.LootBlueChance", 25.0f));
        s.championChillSpell = uint32_t(std::max(0, file.GetInt("Arpg.Champion.ChillSpell", 6136)));
        s.championSparkChancePct = std::max(0.0f, file.GetFloat("Arpg.Champion.SparkChance", 25.0f));
        s.championNukeSpell = uint32_t(std::max(0, file.GetInt("Arpg.Champion.NukeSpell", 133)));
        s.championNukeEveryMs = uint32_t(std::max(1000, file.GetInt("Arpg.Champion.NukeEveryMs", 8000)));
        s.championExplosionRadius = std::max(1.0f, file.GetFloat("Arpg.Champion.ExplosionRadius", 8.0f));
        s.championExplosionDelayMs = uint32_t(std::max(0, file.GetInt("Arpg.Champion.ExplosionDelayMs", 1500)));
        s.craftUniqueChancePct = std::max(0.0f, file.GetFloat("Arpg.Craft.UniqueChance", 2.0f));
        s.craftUniquePityStepPct = std::max(0.0f, file.GetFloat("Arpg.Craft.UniquePityStep", 1.0f));
        s.uncappedZones.clear();
        {
            std::string list = file.GetString("Arpg.Scale.UncappedZones", "5840"), cur;
            for (char ch : list + ",")
            {
                if (ch >= '0' && ch <= '9')
                    cur += ch;
                else if (!cur.empty())
                {
                    s.uncappedZones.insert(uint32_t(std::stoul(cur)));
                    cur.clear();
                }
            }
        }
        s.lootEnabled = file.GetBool("Arpg.Loot.Enabled", true);
        s.affixBotItems = file.GetBool("Arpg.Loot.AffixBotItems", false);
        s.smartLootPct = std::clamp(file.GetInt("Arpg.Loot.SmartLootPct", 70), 0, 100);
        s.displayEnabled = file.GetBool("Arpg.Display.Enabled", true);
        s.displayBase = uint32_t(std::max(0, file.GetInt("Arpg.Display.EnchantBase", 60000)));
        s.displayStride = uint32_t(std::max(2, file.GetInt("Arpg.Display.EnchantStride", 2000)));
        s.displayEffectBase = uint32_t(std::max(0, file.GetInt("Arpg.Display.EffectBase", 100000)));
        s.displaySuffixBase = uint32_t(std::clamp(file.GetInt("Arpg.Display.SuffixBase", 30000), 0, 32000));
        s.displayUniqueBase = uint32_t(std::max(0, file.GetInt("Arpg.Display.UniqueBase", 200000)));
        if (!s.epicIlvlCurve.Parse(file.GetString("Arpg.Loot.EpicIlvlCurve", DefaultEpicIlvlCurve)))
        {
            TC_LOG_ERROR("server.loading", "mod_arpg: Arpg.Loot.EpicIlvlCurve is malformed; using the default");
            s.epicIlvlCurve.Parse(DefaultEpicIlvlCurve);
        }

        std::lock_guard<std::mutex> l(_lock);
        _settings = s;
    }

    bool Config::Load(std::string& message)
    {
        Core::ConfigFile file;
        std::string error;
        if (!file.LoadFile(_path, error))
        {
            message = "arpg.conf not loaded (" + error + "); using defaults";
            TC_LOG_ERROR("server.loading", "mod_arpg: %s", message.c_str());
            Apply(_overrides); // defaults + overrides
            return false;
        }
        int keys = int(file.Values().size());
        for (auto const& kv : _overrides.Values())
            file.Set(kv.first, kv.second);
        Apply(file);
        message = "arpg.conf loaded (" + std::to_string(keys) + " keys, " + std::to_string(file.BadLines()) + " bad lines, "
            + std::to_string(_overrides.Values().size()) + " overrides)";
        TC_LOG_INFO("server.loading", "mod_arpg %s: %s", ARPG_VERSION, message.c_str());
        return true;
    }

    bool Config::SetOverride(std::string const& key, std::string const& value, std::string& message)
    {
        if (!KnownKeys.count(key))
        {
            message = "unknown key " + key;
            return false;
        }
        _overrides.Set(key, value);
        std::string loadMsg;
        Load(loadMsg);
        message = key + " = " + value;
        return true;
    }
}
