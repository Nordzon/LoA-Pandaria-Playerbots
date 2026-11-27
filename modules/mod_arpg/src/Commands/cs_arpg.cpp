// .arpg commands. Every command takes an explicit character name (works over SOAP, which has no selected target).
#include "ArpgConfig.h"
#include "ArpgHooks.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"
#include "Loot/Budget.h"
#include "Loot/NativeEnchants.h"
#include "Loot/AffixRoller.h"
#include "Loot/AffixStats.h"
#include "Loot/AffixEffects.h"
#include "Loot/ZoneLevels.h"
#include "Loot/Substitution.h"
#include "Loot/BossPity.h"
#include "Loot/Crafting.h"
#include "Loot/LootFilter.h"
#include "Loot/LootPet.h"
#include "Loot/Uniques.h"
#include "Monsters/Champions.h"
#include "Monsters/Superuniques.h"
#include "Loot/Potions.h"
#include "Paragon/Paragon.h"
#include "Dungeons/InstanceScaling.h"
#include "Loot/ZoneLevels.h"

#include "Bag.h"
#include "Item.h"

#include "ObjectMgr.h"
#include "TemporarySummon.h"
#include "Character/AnyWeapon.h"
#include "Loot/Salvage.h"
#include "Loot/MountShop.h"
#include "Bots/BotItemScore.h"
#include "Loot/AuctionRolls.h"
#include "Loot/LootBeam.h"
#include "Loot/Blanks.h"
#include "Group.h"
#include "GroupMgr.h"
#include "StatsWeightCalculator.h"
#include <map>

#include "CellImpl.h"
#include "GameObject.h"
#include "LootMgr.h"
#include "Map.h"
#include <algorithm>
#include "Chat.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Log.h"
#include "World.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "Opcodes.h"

#include <cctype>
#include <cstdlib>
#include <array>
#include <cmath>
#include <list>
#include <sstream>
#include <vector>

namespace
{
    std::vector<std::string> Tokens(char const* args)
    {
        std::vector<std::string> out;
        if (!args)
            return out;
        std::istringstream in(args);
        std::string t;
        while (in >> t)
            out.push_back(t);
        return out;
    }

    bool IsNameLike(std::string const& s)
    {
        if (s.empty() || s.size() > 12)
            return false;
        for (char ch : s)
            if (!std::isalpha(static_cast<unsigned char>(ch)))
                return false;
        return true;
    }

    bool IsSpotLike(std::string const& s)
    {
        if (s.empty() || s.size() > 48)
            return false;
        for (char ch : s)
            if (!(std::islower(static_cast<unsigned char>(ch)) || std::isdigit(static_cast<unsigned char>(ch)) || ch == '-'))
                return false;
        return true;
    }

    std::string NormalizeName(std::string s)
    {
        for (size_t i = 0; i < s.size(); ++i)
            s[i] = char(i == 0 ? std::toupper(static_cast<unsigned char>(s[i])) : std::tolower(static_cast<unsigned char>(s[i])));
        return s;
    }

    // ---- stat probe for .arpg test equip/unequip (m2 4.1d) ----
    // Order: str agi int spi sta crit haste hit exp mastery dodge parry
    constexpr size_t kProbeStats = 12;
    using StatVec = std::array<int64, kProbeStats>;
    char const* const kProbeNames[kProbeStats] = { "str", "agi", "int", "spi", "sta", "crit", "haste", "hit", "exp", "mastery", "dodge", "parry" };

    int ProbeIndex(uint32 itemMod)
    {
        switch (itemMod)
        {
            case ITEM_MOD_STRENGTH: return 0; case ITEM_MOD_AGILITY: return 1; case ITEM_MOD_INTELLECT: return 2;
            case ITEM_MOD_SPIRIT: return 3; case ITEM_MOD_STAMINA: return 4;
            case ITEM_MOD_CRIT_RATING: case ITEM_MOD_CRIT_MELEE_RATING: return 5;
            case ITEM_MOD_HASTE_RATING: case ITEM_MOD_HASTE_MELEE_RATING: return 6;
            case ITEM_MOD_HIT_RATING: case ITEM_MOD_HIT_MELEE_RATING: return 7;
            case ITEM_MOD_EXPERTISE_RATING: return 8; case ITEM_MOD_MASTERY_RATING: return 9;
            case ITEM_MOD_DODGE_RATING: return 10; case ITEM_MOD_PARRY_RATING: return 11;
            default: return -1;
        }
    }

    // What the character has now: flat main stats (item stats are BASE_VALUE, enchants TOTAL_VALUE), rating fields.
    StatVec Snapshot(Player* p)
    {
        StatVec v{};
        UnitMods const mods[5] = { UNIT_MOD_STAT_STRENGTH, UNIT_MOD_STAT_AGILITY, UNIT_MOD_STAT_INTELLECT, UNIT_MOD_STAT_SPIRIT, UNIT_MOD_STAT_STAMINA };
        for (int i = 0; i < 5; ++i)
            v[i] = int64(std::lround(p->GetModifierValue(mods[i], BASE_VALUE) + p->GetModifierValue(mods[i], TOTAL_VALUE)));
        CombatRating const crs[7] = { CR_CRIT_MELEE, CR_HASTE_MELEE, CR_HIT_MELEE, CR_EXPERTISE, CR_MASTERY, CR_DODGE, CR_PARRY };
        for (int i = 0; i < 7; ++i)
            v[5 + i] = int64(p->GetUInt32Value(PLAYER_FIELD_COMBAT_RATINGS + crs[i]));
        return v;
    }

    // What the item should add: its own template stats + all its affix stat lines (native ones included).
    StatVec ItemTotals(Item const* item)
    {
        StatVec v{};
        if (!item)
            return v;
        ItemTemplate const* proto = item->GetTemplate();
        for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        {
            int idx = ProbeIndex(proto->ItemStat[i].ItemStatType);
            if (idx >= 0)
                v[idx] += item->GetStatValue(i);
        }
        for (Arpg::Loot::StatLine const& sl : Arpg::Loot::ItemStatLines(item->GetGUID().GetCounter(), true))
        {
            int idx = ProbeIndex(sl.stat);
            if (idx >= 0)
                v[idx] += sl.value;
        }
        return v;
    }

    // " | str d=+12 e=+12 ... | match=yes": measured change vs expected change, for every stat that moved or should have.
    std::string CompareStats(StatVec const& before, StatVec const& after, StatVec const& expected)
    {
        std::string out;
        bool match = true;
        for (size_t i = 0; i < kProbeStats; ++i)
        {
            int64 d = after[i] - before[i];
            if (!d && !expected[i])
                continue;
            if (d - expected[i] > 1 || expected[i] - d > 1)
                match = false;
            out += " | " + std::string(kProbeNames[i]) + " d=" + (d >= 0 ? "+" : "") + std::to_string(d) +
                " e=" + (expected[i] >= 0 ? "+" : "") + std::to_string(expected[i]);
        }
        return out + (match ? " | match=yes" : " | match=NO");
    }

    bool Fail(ChatHandler* handler, std::string const& msg)
    {
        handler->PSendSysMessage("error: %s", msg.c_str());
        handler->SetSentErrorMessage(true);
        return false;
    }
}

class arpg_commandscript : public CommandScript
{
public:
    arpg_commandscript() : CommandScript("arpg_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        using Trinity::ChatCommands::Console;
        static std::vector<ChatCommand> configTable =
        {
            { "set",   &HandleConfigSet,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        static std::vector<ChatCommand> testTable =
        {
            { "setup",    &HandleTestSetup,    rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "learnall", &HandleTestLearnAll, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "professions", &HandleTestProfessions, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "weapons",  &HandleTestWeapons,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "visual",   &HandleTestVisual,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "entries",  &HandleTestEntries,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "beams",    &HandleTestBeams,    rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "lootpreview", &HandleTestLootPreview, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "count",    &HandleTestCount,    rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "salvage",  &HandleTestSalvage,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "mount",    &HandleTestMount,    rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "botscore", &HandleTestBotScore, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "planscore", &HandleTestPlanScore, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "ah",       &HandleTestAh,       rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "money",    &HandleTestMoney,    rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "sell",     &HandleTestSell,     rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "relog",    &HandleTestRelog,    rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "bot",      &HandleTestBot,      rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "party",    &HandleTestParty,    rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "events",   &HandleTestEvents,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "pack",     &HandleTestPack,     rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "give",     &HandleTestGive,     rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "clearbags", &HandleTestClearBags, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "fillbags", &HandleTestFillBags, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "debuff",   &HandleTestDebuff,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "equip",    &HandleTestEquip,    rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "unequip",  &HandleTestUnequip,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "probeitem", &HandleTestProbeItem, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "strike",   &HandleTestStrike,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "struck",   &HandleTestStruck,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "effect",   &HandleTestEffect,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "xp",       &HandleTestXp,       rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "suffixprobe", &HandleTestSuffixProbe, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "unique",   &HandleTestUnique,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "craft",    &HandleTestCraft,    rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "potion",   &HandleTestPotion,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        static std::vector<ChatCommand> scaleTable =
        {
            { "probe", &HandleScaleProbe, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "combat", &HandleScaleCombat, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        static std::vector<ChatCommand> lootTable =
        {
            { "budget",   &HandleLootBudget,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "enchants", &HandleLootEnchants, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "last",     &HandleLootLast,     rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "applied",  &HandleLootApplied,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "effects",  &HandleLootEffects,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "substitute", &HandleLootSubstitute, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "isboss",   &HandleLootIsBoss,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        static std::vector<ChatCommand> pityTable =
        {
            { "info", &HandlePityInfo, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "set",  &HandlePitySet,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        static std::vector<ChatCommand> championTable =
        {
            { "make", &HandleChampionMake, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "info", &HandleChampionInfo, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "clear",  &HandleChampionClear,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "kill", &HandleChampionKill, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "loot", &HandleChampionLoot, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "strike", &HandleChampionStrike, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        static std::vector<ChatCommand> superuniqueTable =
        {
            { "spawn", &HandleSuperuniqueSpawn, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "next",  &HandleSuperuniqueNext,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "info",  &HandleSuperuniqueInfo,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "kill",  &HandleSuperuniqueKill,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "loot",  &HandleSuperuniqueLoot,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "fight", &HandleSuperuniqueFight, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        static std::vector<ChatCommand> paragonTable =
        {
            { "info", &HandleParagonInfo, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "set",  &HandleParagonSet,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "xp",   &HandleParagonXp,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "spend",  &HandleParagonSpend,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "respec", &HandleParagonRespec, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "data",   &HandleParagonData,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        // .paragon - the player's own Paragon (ArpgUI addon talks through these); every account has the help permission.
        static std::vector<ChatCommand> playerParagonTable =
        {
            { "data",   &HandleMyParagonData,   rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "info",   &HandleMyParagonInfo,   rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "spend",  &HandleMyParagonSpend,  rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "respec", &HandleMyParagonRespec, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };
        static std::vector<ChatCommand> dungeonTable =
        {
            { "info",  &HandleDungeonInfo,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "level", &HandleDungeonLevel, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        static std::vector<ChatCommand> filterTable =
        {
            { "list",   &HandleFilterList,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "add",    &HandleFilterAdd,    rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "remove", &HandleFilterRemove, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "move",   &HandleFilterMove,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "reset",  &HandleFilterReset,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "data",   &HandleFilterData,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        static std::vector<ChatCommand> lootpetTable =
        {
            { "on",   &HandleLootPetOn,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "off",  &HandleLootPetOff,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "info", &HandleLootPetInfo, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "model", &HandleLootPetModel, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        // .lootpet / .lootfilter - the player's own (ArpgUI talks through these); every account has the help permission.
        static std::vector<ChatCommand> playerLootPetTable =
        {
            { "on",   &HandleMyLootPetOn,   rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "off",  &HandleMyLootPetOff,  rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "data", &HandleMyLootPetData, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };
        static std::vector<ChatCommand> playerFilterTable =
        {
            { "data",   &HandleMyFilterData,   rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "add",    &HandleMyFilterAdd,    rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "remove", &HandleMyFilterRemove, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "move",   &HandleMyFilterMove,   rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "reset",  &HandleMyFilterReset,  rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };
        static std::vector<ChatCommand> blankTable =
        {
            { "for",  &HandleBlankFor,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "buy",  &HandleBlankBuy,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        static std::vector<ChatCommand> arpgTable =
        {
            { "blank",   blankTable,      rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "lootpet", lootpetTable,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "filter",  filterTable,    rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "dungeon", dungeonTable,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "paragon", paragonTable,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "superunique", superuniqueTable, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "champion", championTable,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "loot",    lootTable,       rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "pity",    pityTable,       rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "version", &HandleVersion,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "progress", &HandleProgress, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "reload",  &HandleReload,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "config",  configTable,     rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "test",    testTable,       rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "scale",   scaleTable,      rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        // .lootinfo data - affix lines of the open loot window's items (ArpgUI shows them on the loot tooltip)
        static std::vector<ChatCommand> playerLootInfoTable =
        {
            { "data", &HandleMyLootInfoData, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };
        static std::vector<ChatCommand> playerProfessionsTable =
        {
            { "data", &HandleMyProfessionsData, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "arpg",    arpgTable,       rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "paragon", playerParagonTable, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "lootpet", playerLootPetTable, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "lootfilter", playerFilterTable, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "professions", playerProfessionsTable, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            { "lootinfo", playerLootInfoTable, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
        };
        return commandTable;
    }

    // .arpg progress <text>   (owner 2026-11-26) prints "[ARPG] <text>" in the worldserver console and Server.log: the test
    // runner reports each GM test file there, so the owner can follow a run in the server window.
    static bool HandleProgress(ChatHandler* handler, char const* args)
    {
        std::string text = args ? args : "";
        TC_LOG_INFO("server.worldserver", "[ARPG] %s", text.c_str());
        handler->SendSysMessage("ok");
        return true;
    }

    // .arpg version
    static bool HandleVersion(ChatHandler* handler, char const* /*args*/)
    {
        handler->PSendSysMessage("mod_arpg %s (%s) scaled creatures=%u config=%s", ARPG_VERSION, ARPG_MILESTONE,
            uint32(Arpg::Scaling::ScaledCount()), sArpgConfig.Path().c_str());
        return true;
    }

    // .arpg reload
    static bool HandleReload(ChatHandler* handler, char const* /*args*/)
    {
        std::string msg;
        bool ok = sArpgConfig.Load(msg);
        std::string loot = Arpg::Loot::LoadAffixData();
        std::string fx = Arpg::Loot::LoadEffectData();
        std::string zones = Arpg::Loot::LoadZoneLevels();
        std::string subst = Arpg::Loot::LoadSubstitutionIndex();
        std::string uniq = Arpg::Loot::LoadUniques();
        std::string champ = Arpg::Monsters::LoadChampionData();
        std::string su = Arpg::Monsters::LoadSuperuniqueData();
        std::string pot = Arpg::Loot::LoadPotionData();
        std::string dun = Arpg::Dungeons::LoadInstanceData();
        std::string comp = Arpg::Loot::LoadCompanionModels();
        std::string mounts = Arpg::Loot::LoadMountShop();
        std::string blanks = Arpg::Loot::LoadBlanks();
        handler->PSendSysMessage("%s %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s; %s", ok ? "ok" : "error:", msg.c_str(), loot.c_str(), fx.c_str(), zones.c_str(),
            subst.c_str(), uniq.c_str(), champ.c_str(), su.c_str(), pot.c_str(), dun.c_str(), comp.c_str(), mounts.c_str(), blanks.c_str());
        return ok;
    }

    // .arpg config set <key> <value>
    static bool HandleConfigSet(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2)
            return Fail(handler, "usage: .arpg config set <key> <value>");
        std::string msg;
        if (!sArpgConfig.SetOverride(t[0], t[1], msg))
            return Fail(handler, msg);
        handler->PSendSysMessage("ok %s", msg.c_str());
        return true;
    }

    // .arpg test setup <char> <level> <spot>   (spot = arpg_test_spot.name)
    static bool HandleTestSetup(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 3)
            return Fail(handler, "usage: .arpg test setup <char> <level> <spot>");
        if (!IsNameLike(t[0]))
            return Fail(handler, "bad character name");
        if (!IsSpotLike(t[2]))
            return Fail(handler, "bad spot name (a-z, 0-9, -)");
        std::string name = NormalizeName(t[0]);
        int level = std::atoi(t[1].c_str());
        int maxLevel = int(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
        if (level < 1 || level > maxLevel)
            return Fail(handler, "level must be 1-" + std::to_string(maxLevel));

        QueryResult spot = WorldDatabase.PQuery("SELECT map, x, y, z, o FROM arpg_test_spot WHERE name = '%s'", t[2].c_str());
        if (!spot)
            return Fail(handler, "unknown spot " + t[2] + " (see table arpg_test_spot)");
        Field* f = spot->Fetch();
        uint32 map = f[0].GetUInt32();
        float x = f[1].GetFloat(), y = f[2].GetFloat(), z = f[3].GetFloat(), o = f[4].GetFloat();

        // Connected lookup: a character in a far teleport is connected but not "in world" - never treat it as offline.
        if (Player* p = ObjectAccessor::FindConnectedPlayerByName(name))
        {
            if (!p->IsInWorld())
                return Fail(handler, "character in transfer (teleporting), retry");
            // A test character always starts alive (an earlier test may have got it killed: a ghost sees no creatures).
            if (!p->IsAlive())
            {
                p->ResurrectPlayer(1.0f);
                p->SpawnCorpseBones();
            }
            if (int(p->GetLevel()) != level)
                p->GiveLevel(uint8(level));
            p->SetFullHealth();
            p->TeleportTo(map, x, y, z, o);
            handler->PSendSysMessage("ok %s level=%d spot=%s (online)", name.c_str(), level, t[2].c_str());
            return true;
        }

        QueryResult exists = CharacterDatabase.PQuery("SELECT guid FROM characters WHERE name = '%s'", name.c_str());
        if (!exists)
            return Fail(handler, "character " + name + " not found (create it, e.g. with the headless client)");
        CharacterDatabase.DirectPExecute("UPDATE characters SET level = %d, xp = 0, map = %u, position_x = %f, position_y = %f, "
            "position_z = %f, orientation = %f, zone = 0 WHERE name = '%s'", level, map, x, y, z, o, name.c_str());
        handler->PSendSysMessage("ok %s level=%d spot=%s (offline: saved, applies at next login)", name.c_str(), level, t[2].c_str());
        return true;
    }

    // .arpg test learnall <char>   (m1b) learns all 11 primary professions (every ability of each skill line, like the
    // core's ".learn all crafts", but by name over SOAP and including the gathering professions it skips)
    static bool HandleTestLearnAll(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test learnall <char>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");

        // Alchemy, Blacksmithing, Enchanting, Engineering, Herbalism, Inscription, Jewelcrafting, Leatherworking,
        // Mining, Skinning, Tailoring
        static uint32 const primarySkills[] = { 171, 164, 333, 202, 182, 773, 755, 165, 186, 393, 197 };
        uint32 classMask = p->GetClassMask();
        for (uint32 skillId : primarySkills)
        {
            for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
            {
                SkillLineAbilityEntry const* sla = sSkillLineAbilityStore.LookupEntry(j);
                if (!sla || sla->SkillLine != skillId || sla->SupercededBySpell || sla->RaceMask != 0)
                    continue;
                if (sla->ClassMask && (sla->ClassMask & classMask) == 0)
                    continue;
                SpellInfo const* info = sSpellMgr->GetSpellInfo(sla->Spell);
                if (!info || !SpellMgr::IsSpellValid(info, p, false))
                    continue;
                p->LearnSpell(sla->Spell, false);
            }
        }
        uint32 have = 0;
        for (uint32 skillId : primarySkills)
            if (p->HasSkill(skillId))
                ++have;
        handler->PSendSysMessage("ok %s professions=%u of 11", p->GetName().c_str(), have);
        return true;
    }

    // Professions panel data (m1b 2.1): "ARPGT|skill:value:max:spell;..." for every primary profession the character has, in
    // the order below. spell = a known spell of that skill line that opens its window (trade skill effect), 0 if none
    // (Herbalism, Skinning). The client can't list more than two primaries itself, so ArpgUI reads this line.
    static std::string ProfessionsDataLine(Player* p)
    {
        static uint32 const primarySkills[] = { 171, 164, 333, 202, 182, 773, 755, 165, 186, 393, 197 };
        std::string line = "ARPGT|";
        bool first = true;
        for (uint32 skillId : primarySkills)
        {
            if (!p->HasSkill(skillId))
                continue;
            uint32 openSpell = 0;
            for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows() && !openSpell; ++j)
            {
                SkillLineAbilityEntry const* sla = sSkillLineAbilityStore.LookupEntry(j);
                if (!sla || sla->SkillLine != skillId || !p->HasSpell(sla->Spell))
                    continue;
                SpellInfo const* info = sSpellMgr->GetSpellInfo(sla->Spell);
                if (info && info->HasEffect(SPELL_EFFECT_TRADE_SKILL))
                    openSpell = sla->Spell;
            }
            line += (first ? "" : ";") + std::to_string(skillId) + ":" + std::to_string(p->GetSkillValue(skillId)) + ":"
                + std::to_string(p->GetMaxSkillValue(skillId)) + ":" + std::to_string(openSpell);
            first = false;
        }
        return line;
    }

    // .arpg test ah <itemId>   (m7 living auction house) what the hook does to one new AuctionHouseBot listing of that item,
    // with the share forced: an ownerless item rolled at its required level, buyout before/after. The item is thrown away.
    static bool HandleTestAh(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1)
            return Fail(handler, "usage: .arpg test ah <itemId>");
        uint32 itemId = uint32(std::strtoul(t[0].c_str(), nullptr, 10));
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            return Fail(handler, "unknown item " + t[0]);
        Item* item = Item::CreateItem(itemId, 1);
        if (!item)
            return Fail(handler, "could not create item");
        uint32 base = std::max<uint32>(1, proto->SellPrice * 4);   // a stand-in for the seller's own price
        uint32 buyout = base, bid = base * 3 / 4;
        std::string why;
        int n = Arpg::Loot::RollAuctionListing(item, buyout, bid, true, why);
        uint32 guid = item->GetGUID().GetCounter();
        CharacterDatabase.DirectPExecute("DELETE FROM item_instance_arpg_affix WHERE item_guid = %u", guid);
        delete item;
        handler->PSendSysMessage("ok ah item=%u level=%u affixes=%d base=%u buyout=%u bid=%u (%s)", itemId,
            uint32(proto->RequiredLevel), n, base, buyout, bid, why.c_str());
        return true;
    }

    // .arpg test planscore <char> <bot>|any <itemId>   (m7 3b.3) plans the item as corpse loot for the character (a loot list of
    // one, the same planner a kill uses), then reports the planned affix stats a party bot sees for it before anyone loots.
    static bool HandleTestPlanScore(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 3 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test planscore <char> <bot>|any <itemId>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        Player* bot = nullptr;
        if (t[1] == "any")
        {
            if (Group* g = p->GetGroup())
                for (GroupReference* ref = g->GetFirstMember(); ref && !bot; ref = ref->next())
                    if (Player* m = ref->GetSource())
                        if (m != p && !Arpg::Scaling::IsRealPlayer(m))
                            bot = m;
        }
        else
            bot = ObjectAccessor::FindPlayerByName(NormalizeName(t[1]));
        if (!bot)
            return Fail(handler, "no party bot (group one first: .arpg test botscore <char> any <item>)");
        uint32 itemId = uint32(std::strtoul(t[2].c_str(), nullptr, 10));
        ::Loot loot;
        LootItem li;
        li.itemid = itemId;
        li.count = 1;
        loot.items.push_back(li);
        loot.unlootedCount = 1;
        Arpg::Loot::PlanLoot(p, loot, ItemPickupSourceType::CreatureLoot, 0, false);
        int32 rp = loot.items[0].randomPropertyId;
        std::string why;
        int sum = Arpg::Bots::AffixScoreInput(bot, itemId, rp, why);
        handler->PSendSysMessage("ok planscore bot=%s item=%u rp=%d planned_sum=%d (%s)", bot->GetName().c_str(), itemId, rp, sum,
            why.c_str());
        return true;
    }

    // .arpg test botscore <char> <bot> <itemId> [ungroup]   (m7) puts the bot in the character's group, gives the bot the item
    // rolled with affixes at the character's level, and reports the bot's own item score (playerbots StatsWeightCalculator)
    // with and without mod_arpg's affix hook. "ungroup" takes the bot out of the group again.
    static bool HandleTestBotScore(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        char const* usage = "usage: .arpg test botscore <char> <bot>|any <itemId>|ungroup";
        if (t.size() != 3 || !IsNameLike(t[0]) || !IsNameLike(t[1]))
            return Fail(handler, usage);
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        Player* bot = nullptr;
        if (t[1] == "any")   // the first online random bot that isn't grouped with someone else
        {
            // Bot sessions live with playerbots, not in the world's session list: look through every online player. The bot
            // must wear a green-epic item without stock random stats (low-level bots in white gear have nothing to roll).
            auto wearsGreen = [](Player* b) -> bool
            {
                for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                    if (Item* it = b->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    {
                        ItemTemplate const* t = it->GetTemplate();   // stock random-stat items ("of the Bear") never roll
                        if (t->Quality >= ITEM_QUALITY_UNCOMMON && t->Quality <= ITEM_QUALITY_EPIC && !t->RandomProperty && !t->RandomSuffix)
                            return true;
                    }
                return false;
            };
            for (auto const& [guid, b] : ObjectAccessor::GetPlayers())
                if (b && b != p && !Arpg::Scaling::IsRealPlayer(b) && b->IsInWorld() && b->IsAlive() &&
                    (!b->GetGroup() || (p && b->GetGroup() == p->GetGroup())) && wearsGreen(b))
                {
                    bot = b;
                    break;
                }
        }
        else
            bot = ObjectAccessor::FindPlayerByName(NormalizeName(t[1]));
        if (p && p->IsInWorld() && t[2] == "ungroup")
            bot = p;   // not used
        if (!p || !p->IsInWorld() || !bot || !bot->IsInWorld())
            return Fail(handler, "character or bot not online");
        if (t[2] == "ungroup")   // disbands the character's group (the test bot leaves it)
        {
            if (Group* g = p->GetGroup())
                g->Disband();
            handler->SendSysMessage("ok ungrouped");
            return true;
        }
        if (bot->GetGroup() != p->GetGroup() || !p->GetGroup())
        {
            if (bot->GetGroup())
                bot->GetGroup()->RemoveMember(bot->GetGUID());
            Group* g = p->GetGroup();
            if (!g)
            {
                g = new Group();
                g->Create(p);
                sGroupMgr->AddGroup(g);
            }
            g->AddMember(bot);
        }
        uint32 itemId = uint32(std::strtoul(t[2].c_str(), nullptr, 10));
        ItemPosCountVec dest;
        if (bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, 1) != EQUIP_ERR_OK)
            return Fail(handler, "bot's bags full");
        Item* item = bot->StoreNewItem(dest, itemId, true);
        if (!item)
            return Fail(handler, "could not create item");
        std::string why;
        int n = Arpg::Loot::RollForItem(p, item, ItemPickupSourceType::CreatureLoot, why);
        int32 rp = item->GetItemRandomPropertyId();
        StatsWeightCalculator calc(bot);
        float with = calc.CalculateItem(itemId, rp);
        auto hook = StatsWeightCalculator::ExtraItemStats;
        StatsWeightCalculator::ExtraItemStats = nullptr;
        StatsWeightCalculator plain(bot);
        float without = plain.CalculateItem(itemId, rp);
        StatsWeightCalculator::ExtraItemStats = hook;
        std::string input;
        int sum = Arpg::Bots::AffixScoreInput(bot, itemId, rp, input);
        handler->PSendSysMessage("ok botscore bot=%s item=%u affixes=%d affix_sum=%d (%s) with=%.1f without=%.1f gain=%.1f equipped_affixed=%d",
            bot->GetName().c_str(), itemId, n, sum, input.c_str(), with, without, with - without, Arpg::Bots::AffixedEquipped(bot));
        return true;
    }

    // .arpg test mount <char> list|buy <spellId>   the companion's mount shop over SOAP
    static bool HandleTestMount(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        char const* usage = "usage: .arpg test mount <char> list|buy <spellId>|forget <spellId>";
        if (t.size() < 2 || !IsNameLike(t[0]))
            return Fail(handler, usage);
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        if (t[1] == "list" && t.size() == 2)
        {
            std::vector<Arpg::Loot::ShopMount> m = Arpg::Loot::MountsForSale(p);
            std::string first = m.empty() ? "" : (" first=" + std::to_string(m[0].spellId) + " (" + m[0].name + ") price=" +
                std::to_string(m[0].priceGold));
            handler->PSendSysMessage("ok mountshop for_sale=%u%s", uint32(m.size()), first.c_str());
            return true;
        }
        if (t[1] == "buy" && t.size() == 3)
        {
            std::string out;
            if (!Arpg::Loot::BuyMount(p, uint32(std::strtoul(t[2].c_str(), nullptr, 10)), out))
                return Fail(handler, out);
            handler->PSendSysMessage("ok %s", out.c_str());
            return true;
        }
        if (t[1] == "vendor" && t.size() == 2)   // the vendor window's list (Mount Merchant, entry 1000002)
        {
            VendorItemData const* v = sObjectMgr->GetNpcVendorItemList(1000002);
            handler->PSendSysMessage("ok mount vendor items=%u", v ? uint32(v->GetItemCount()) : 0u);
            return true;
        }
        if (t[1] == "forget" && t.size() == 3)
        {
            p->RemoveSpell(uint32(std::strtoul(t[2].c_str(), nullptr, 10)));
            handler->SendSysMessage("ok forgot");
            return true;
        }
        return Fail(handler, usage);
    }

    // .arpg test money <char> <gold>   sets the character's money (tests)
    static bool HandleTestMoney(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test money <char> <gold>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint64 copper = uint64(std::strtoull(t[1].c_str(), nullptr, 10)) * 10000;
        p->SetMoney(copper);
        handler->PSendSysMessage("ok money gold=%u", uint32(copper / 10000));
        return true;
    }

    // .arpg test sell <char> <vendorEntry>   (m2 5.4) sells the last item rolled for the character through the real
    // CMSG_SELL_ITEM handler (a vendor of that entry is summoned next to it for 30 s): money delta vs the affix sell formula.
    static bool HandleTestSell(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test sell <char> <vendorEntry>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 low = Arpg::Loot::LastItemGuid(p);
        Item* item = low ? p->GetItemByGuid(ObjectGuid(HighGuid::Item, low)) : nullptr;
        if (!item)
            return Fail(handler, "no rolled item in the bags (use .arpg test give first)");
        uint32 entry = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        CreatureTemplate const* ct = sObjectMgr->GetCreatureTemplate(entry);
        if (!ct || !(ct->npcflag & UNIT_NPC_FLAG_VENDOR))
            return Fail(handler, "creature " + t[1] + " is not a vendor");
        Position pos = p->GetNearPosition(2.0f, 0.0f);
        TempSummon* vendor = p->SummonCreature(entry, pos, TEMPSUMMON_TIMED_DESPAWN, Milliseconds(30000));
        if (!vendor)
            return Fail(handler, "vendor not summoned");
        uint64 expected = Arpg::Loot::AffixSellPrice(item, uint64(item->GetTemplate()->SellPrice) * item->GetCount());
        int affixes = int(Arpg::Loot::ItemStatLines(low, true).size() + Arpg::Loot::ItemEffectLines(low).size());
        uint64 before = p->GetMoney();

        // The client's packet: uint32 count (0 = the whole stack), then the two guids as masked bits + byte sequence.
        ObjectGuid itemGuid = item->GetGUID(), vendorGuid = vendor->GetGUID();
        WorldPacket pkt(CMSG_SELL_ITEM, 4 + 2 + 16);
        pkt << uint32(0);
        pkt.WriteBit(itemGuid[4]); pkt.WriteBit(itemGuid[3]); pkt.WriteBit(itemGuid[7]); pkt.WriteBit(vendorGuid[6]);
        pkt.WriteBit(vendorGuid[5]); pkt.WriteBit(vendorGuid[1]); pkt.WriteBit(itemGuid[5]); pkt.WriteBit(itemGuid[2]);
        pkt.WriteBit(itemGuid[1]); pkt.WriteBit(vendorGuid[2]); pkt.WriteBit(itemGuid[6]); pkt.WriteBit(vendorGuid[4]);
        pkt.WriteBit(vendorGuid[0]); pkt.WriteBit(vendorGuid[7]); pkt.WriteBit(vendorGuid[3]); pkt.WriteBit(itemGuid[0]);
        pkt.FlushBits();
        pkt.WriteByteSeq(vendorGuid[6]); pkt.WriteByteSeq(vendorGuid[3]); pkt.WriteByteSeq(vendorGuid[1]); pkt.WriteByteSeq(itemGuid[1]);
        pkt.WriteByteSeq(vendorGuid[2]); pkt.WriteByteSeq(itemGuid[7]); pkt.WriteByteSeq(itemGuid[5]); pkt.WriteByteSeq(vendorGuid[7]);
        pkt.WriteByteSeq(itemGuid[2]); pkt.WriteByteSeq(vendorGuid[0]); pkt.WriteByteSeq(vendorGuid[5]); pkt.WriteByteSeq(itemGuid[3]);
        pkt.WriteByteSeq(itemGuid[6]); pkt.WriteByteSeq(vendorGuid[4]); pkt.WriteByteSeq(itemGuid[4]); pkt.WriteByteSeq(itemGuid[0]);
        p->GetSession()->HandleSellItemOpcode(pkt);

        bool gone = p->GetItemByGuid(itemGuid) == nullptr;
        vendor->DespawnOrUnsummon();
        handler->PSendSysMessage("ok sold=%u item=%u affixes=%d delta=%llu expected=%llu", gone ? 1u : 0u, low, affixes,
            (unsigned long long)(p->GetMoney() - before), (unsigned long long)expected);
        return true;
    }

    // .arpg test relog <char>   (m2 5.2) what a relog does to affixed gear, without logging out: all item mods off, the
    // affix cache re-read from the database (as at server start), all item mods on again (the login path). Stats must match.
    static bool HandleTestRelog(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test relog <char>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        int affixed = 0;
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            if (Item* it = p->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                if (!Arpg::Loot::ItemStatLines(it->GetGUID().GetCounter(), true).empty())
                    ++affixed;
        StatVec before = Snapshot(p);
        p->_RemoveAllItemMods();
        StatVec off = Snapshot(p);
        Arpg::Loot::LoadStatCache();
        p->_ApplyAllItemMods();
        StatVec after = Snapshot(p);
        int64 removed = 0;
        std::string diff;
        for (size_t i = 0; i < kProbeStats; ++i)
        {
            removed += before[i] - off[i];
            if (before[i] != after[i])
                diff += std::string(" ") + kProbeNames[i] + "=" + std::to_string(before[i]) + "->" + std::to_string(after[i]);
        }
        handler->PSendSysMessage("ok relog affixed_equipped=%d removed=%lld match=%s%s", affixed, (long long)removed,
            diff.empty() ? "yes" : "no", diff.c_str());
        return true;
    }

    // .arpg test bot <char> join [minLevel] | bring | item <itemId> | leave   (m7 2.x) a real random bot for tests: join groups
    // an online random bot with the character, bring teleports it next to the character (same map / instance), item gives
    // the bot an item through the roller (bots' own items get no affixes), leave disbands the group.
    static Player* TestBotOf(Player* p)
    {
        if (Group* g = p->GetGroup())
            for (GroupReference* ref = g->GetFirstMember(); ref; ref = ref->next())
                if (Player* m = ref->GetSource())
                    if (m != p && !Arpg::Scaling::IsRealPlayer(m))
                        return m;
        return nullptr;
    }

    static bool HandleTestBot(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        char const* usage = "usage: .arpg test bot <char> join [minLevel]|bring|item <itemId>|leave";
        if (t.size() < 2 || !IsNameLike(t[0]))
            return Fail(handler, usage);
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        if (t[1] == "join")
        {
            if (Player* b = TestBotOf(p))
            {
                handler->PSendSysMessage("ok bot=%s level=%u real=%d (already grouped)", b->GetName().c_str(), b->GetLevel(),
                    Arpg::Scaling::IsRealPlayer(b) ? 1 : 0);
                return true;
            }
            uint32 minLevel = t.size() > 2 ? uint32(std::strtoul(t[2].c_str(), nullptr, 10)) : 1;
            Player* bot = nullptr;
            for (auto const& [guid, b] : ObjectAccessor::GetPlayers())
                if (b && b != p && !Arpg::Scaling::IsRealPlayer(b) && b->IsInWorld() && b->IsAlive() && !b->GetGroup() &&
                    b->GetLevel() >= minLevel && !b->IsInCombat() && !b->GetMap()->Instanceable())
                {
                    bot = b;
                    break;
                }
            if (!bot)
                return Fail(handler, "no free online random bot");
            Group* g = p->GetGroup();
            if (!g)
            {
                g = new Group();
                g->Create(p);
                sGroupMgr->AddGroup(g);
            }
            g->AddMember(bot);
            handler->PSendSysMessage("ok bot=%s level=%u real=%d", bot->GetName().c_str(), bot->GetLevel(),
                Arpg::Scaling::IsRealPlayer(bot) ? 1 : 0);
            return true;
        }
        if (t[1] == "leave")
        {
            if (Group* g = p->GetGroup())
                g->Disband();
            handler->SendSysMessage("ok bot left");
            return true;
        }
        Player* bot = TestBotOf(p);
        if (!bot || !bot->IsInWorld())
            return Fail(handler, "no bot in the character's group (join first)");
        if (t[1] == "bring")
        {
            Position pos = p->GetNearPosition(2.0f, float(M_PI));
            bot->TeleportTo(p->GetMapId(), pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), p->GetOrientation());
            handler->PSendSysMessage("ok bot=%s brought map=%u", bot->GetName().c_str(), p->GetMapId());
            return true;
        }
        if (t[1] == "item" && t.size() == 3)
        {
            uint32 itemId = uint32(std::strtoul(t[2].c_str(), nullptr, 10));
            ItemPosCountVec dest;
            if (!sObjectMgr->GetItemTemplate(itemId))
                return Fail(handler, "unknown item " + t[2]);
            if (bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, 1) != EQUIP_ERR_OK)
                return Fail(handler, "bot's bags full");
            Item* item = bot->StoreNewItem(dest, itemId, true);
            if (!item)
                return Fail(handler, "could not create item");
            std::string why;
            int n = Arpg::Loot::RollForItem(bot, item, ItemPickupSourceType::CreatureLoot, why);
            handler->PSendSysMessage("ok bot=%s item=%u affixes=%d (%s)", bot->GetName().c_str(), itemId, n, why.c_str());
            bot->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
            return true;
        }
        return Fail(handler, usage);
    }

    // .arpg test party <char> <other>|status|leave   (m8) groups two characters (both real test characters, the first leads),
    // shows the group (members, real players, loot method: 1 = round robin), or disbands it.
    static bool HandleTestParty(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2 || !IsNameLike(t[0]) || !IsNameLike(t[1]))
            return Fail(handler, "usage: .arpg test party <char> <other>|status|leave");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        if (t[1] == "leave")
        {
            if (Group* g = p->GetGroup())
                g->Disband();
            handler->SendSysMessage("ok party left");
            return true;
        }
        if (t[1] != "status")
        {
            Player* o = ObjectAccessor::FindPlayerByName(NormalizeName(t[1]));
            if (!o || !o->IsInWorld() || o == p)
                return Fail(handler, "other character not online");
            if (o->GetGroup() && o->GetGroup() != p->GetGroup())
                o->GetGroup()->RemoveMember(o->GetGUID());
            Group* g = p->GetGroup();
            if (!g)
            {
                g = new Group();
                g->Create(p);
                sGroupMgr->AddGroup(g);
            }
            if (o->GetGroup() != g)
                g->AddMember(o);
        }
        Group* g = p->GetGroup();
        if (!g)
        {
            handler->SendSysMessage("ok party none");
            return true;
        }
        int members = 0, real = 0;
        for (GroupReference* ref = g->GetFirstMember(); ref; ref = ref->next())
            if (Player* m = ref->GetSource())
            {
                ++members;
                real += Arpg::Scaling::IsRealPlayer(m) ? 1 : 0;
            }
        handler->PSendSysMessage("ok party members=%d real=%d loot_method=%u", members, real, uint32(g->GetLootMethod()));
        return true;
    }

    // .arpg test salvage <char> preview|do|buy <band>   the companion's Salvage / Buy catalysts, by name over SOAP
    static bool HandleTestSalvage(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        char const* usage = "usage: .arpg test salvage <char> preview|do|buy <band>";
        if (t.size() < 2 || !IsNameLike(t[0]))
            return Fail(handler, usage);
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        std::string out;
        if (t[1] == "preview" && t.size() == 2)
        {
            Arpg::Loot::SalvagePreview sp = Arpg::Loot::PreviewSalvage(p);
            out = "salvage preview items=" + std::to_string(sp.items) + " essences=" + std::to_string(sp.essences);
        }
        else if (t[1] == "do" && t.size() == 2)
            Arpg::Loot::DoSalvage(p, out);
        else if (t[1] == "buy" && t.size() == 3)
        {
            if (!Arpg::Loot::BuyCatalyst(p, std::atoi(t[2].c_str()), out))
                return Fail(handler, out);
        }
        else
            return Fail(handler, usage);
        handler->PSendSysMessage("ok %s", out.c_str());
        return true;
    }

    // .arpg test count <char> <itemId>   how many of that item the character carries (bags + bank)
    static bool HandleTestCount(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test count <char> <itemId>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 itemId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        handler->PSendSysMessage("ok item=%u count=%u", itemId, p->GetItemCount(itemId, true));
        return true;
    }

    // Sorted ";"-parts without the Unique marker, so a preview and an item compare regardless of line order.
    static std::string CanonPreview(std::string const& s)
    {
        std::vector<std::string> parts;
        std::stringstream ss(s);
        std::string part;
        while (std::getline(ss, part, ';'))
            if (!part.empty() && part[0] != 'u')
                parts.push_back(part);
        std::sort(parts.begin(), parts.end());
        std::string out;
        for (std::string const& p : parts)
            out += (out.empty() ? "" : ";") + p;
        return out;
    }

    // .arpg test lootpreview <char>   the character's last dead Champion: previews each planned item (what the loot window
    // shows), loots it, and compares with the item it got. "ok previewed=N match=N" when every preview was exact.
    static bool HandleTestLootPreview(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test lootpreview <char>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        Creature* c = Arpg::Monsters::LastChampionOf(p);
        if (!c || c->IsAlive())
            return Fail(handler, "no dead champion");
        uint32 previewed = 0, match = 0;
        p->SetLootGUID(c->GetGUID());
        for (uint8 i = 0; i < uint8(c->loot.items.size()); ++i)
        {
            LootItem const& li = c->loot.items[i];
            if (li.is_looted || li.currency || li.randomPropertyId >= 0)
                continue;
            std::string preview = Arpg::Loot::PlanPreview(p, li.itemid, li.randomPropertyId);
            if (preview.empty())
                continue;
            ++previewed;
            p->StoreLootItem(i, &c->loot, c);
            std::string got = Arpg::Loot::LastItemPreview(p);
            if (CanonPreview(preview) == CanonPreview(got))
                ++match;
            else
                handler->PSendSysMessage("mismatch item=%u preview=%s got=%s", li.itemid, preview.c_str(), got.c_str());
        }
        p->SetLootGUID(ObjectGuid::Empty);
        handler->PSendSysMessage("ok previewed=%u match=%u", previewed, match);
        return true;
    }

    // .lootinfo data   one "ARPGV|<item>|<suffix id>|<lines>" per planned item of the loot window the player has open
    static bool HandleMyLootInfoData(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        ObjectGuid g = p->GetLootGUID();
        ::Loot* loot = nullptr;
        if (g.IsCreatureOrVehicle())
        {
            if (Creature* c = p->GetMap()->GetCreature(g))
                loot = &c->loot;
        }
        else if (g.IsGameObject())
        {
            if (GameObject* go = p->GetMap()->GetGameObject(g))
                loot = &go->loot;
        }
        if (!loot)
            return true;
        for (LootItem const& li : loot->items)
        {
            if (li.is_looted || li.currency || li.randomPropertyId >= 0)
                continue;
            std::string preview = Arpg::Loot::PlanPreview(p, li.itemid, li.randomPropertyId);
            if (!preview.empty())
                handler->PSendSysMessage("ARPGV|%u|%d|%s", li.itemid, li.randomPropertyId, preview.c_str());
        }
        return true;
    }

    // .arpg blank for <char> <itemId>   the blank that character would get for this item (its loot level and spec)
    static bool HandleBlankFor(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg blank for <char> <itemId>");
        if (!p || t.size() != 2)
            return p ? Fail(handler, "usage: .arpg blank for <char> <itemId>") : false;
        uint32 item = uint32(atoi(t[1].c_str()));
        uint32 blank = Arpg::Loot::BlankForItem(p, item);
        ItemTemplate const* bp = blank ? sObjectMgr->GetItemTemplate(blank) : nullptr;
        if (!bp)
            return Fail(handler, "no blank for that item");
        handler->PSendSysMessage("ok item=%u blank=%u name=%s q=%u req=%u stat1=%u:%d stat2=%u:%d blank_check=%d", item, blank,
            bp->Name1.c_str(), bp->Quality, bp->RequiredLevel, bp->ItemStat[0].ItemStatType, bp->ItemStat[0].ItemStatValue,
            bp->ItemStat[1].ItemStatType, bp->ItemStat[1].ItemStatValue, Arpg::Loot::IsBlank(blank) ? 1 : 0);
        return true;
    }

    // .arpg blank buy <char> <slot index> <quality 2-4>   the companion's "Blank bases" purchase (slot index as in the menu)
    static bool HandleBlankBuy(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg blank buy <char> <slot> <quality>");
        if (!p || t.size() != 3)
            return p ? Fail(handler, "usage: .arpg blank buy <char> <slot> <quality>") : false;
        std::string out;
        bool ok = Arpg::Loot::BuyBlank(p, atoi(t[1].c_str()), uint32(atoi(t[2].c_str())), out);
        handler->PSendSysMessage("%s %s", ok ? "ok" : "fail", out.c_str());
        return true;
    }

    // .arpg test beams <char>   loot beams currently up over that character's kills
    static bool HandleTestBeams(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test beams <char>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        handler->PSendSysMessage("ok beams=%d", Arpg::Loot::ActiveBeams(p));
        return true;
    }

    // .arpg test entries   the highest creature template entry the server loaded: a MoP creature GUID carries only 20 bits of
    // the entry, so anything above 1,048,575 reaches the client cut and shows as nothing (2026-11-26 invisible companion).
    static bool HandleTestEntries(ChatHandler* handler, char const* /*args*/)
    {
        uint32 maxEntry = 0, over = 0;
        for (auto const& [entry, tmpl] : *sObjectMgr->GetCreatureTemplates())
        {
            maxEntry = std::max(maxEntry, entry);
            if (entry > 0xFFFFF)
                ++over;
        }
        handler->PSendSysMessage("ok creature max_entry=%u over_20bit=%u", maxEntry, over);
        return true;
    }

    // .arpg test visual <char> <spellId>|clear   (owner picks the loot beam / Champion glow) puts that spell's visual on the
    // creature nearest to the character for 60 s: as an aura if the spell is one, else cast on itself (triggered).
    // "clear" removes the last one shown.
    static bool HandleTestVisual(ChatHandler* handler, char const* args)
    {
        static std::map<uint64, std::pair<ObjectGuid, uint32>> shown;   // player guid -> (creature, spell)
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test visual <char> <spellId>|clear");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint64 key = p->GetGUID().GetRawValue();
        auto last = shown.find(key);
        if (last != shown.end())
        {
            if (Creature* old = ObjectAccessor::GetCreature(*p, last->second.first))
                old->RemoveAurasDueToSpell(last->second.second);
            shown.erase(last);
        }
        if (t[1] == "clear")
        {
            handler->SendSysMessage("ok visual cleared");
            return true;
        }
        uint32 spellId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        SpellInfo const* si = sSpellMgr->GetSpellInfo(spellId);
        if (!si)
            return Fail(handler, "unknown spell " + t[1]);
        Creature* c = nullptr;
        float best = 30.0f;
        std::list<Unit*> units;
        Trinity::AnyUnitInObjectRangeCheck check(p, best);
        Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(p, units, check);
        Cell::VisitAllObjects(p, searcher, best);
        for (Unit* u : units)
            if (Creature* cr = u->ToCreature())
                if (cr->IsAlive() && !cr->IsPet() && p->GetDistance(cr) < best)
                {
                    best = p->GetDistance(cr);
                    c = cr;
                }
        if (!c)
            return Fail(handler, "no creature within 30 yards");
        char const* how = "aura";
        if (Aura* a = c->AddAura(spellId, c))
        {
            a->SetMaxDuration(60000);
            a->SetDuration(60000);
        }
        else
        {
            c->CastSpell(c, spellId, true);
            how = "cast";
        }
        shown[key] = { c->GetGUID(), spellId };
        handler->PSendSysMessage("ok visual spell=%u on=%s (%u) as=%s", spellId, c->GetName().c_str(), c->GetEntry(), how);
        return true;
    }

    // .arpg test weapons <char> [itemId]   (m6-any-weapon) grants what's missing, then reports the proficiencies known and,
    // with an item id, whether the character could equip it (equip error code; 0 = yes)
    static bool HandleTestWeapons(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.empty() || t.size() > 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test weapons <char> [itemId]");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        int added = Arpg::Character::ApplyAnyWeapon(p);
        std::string line = "ok " + Arpg::Character::DescribeAnyWeapon(p) + " added=" + std::to_string(added);
        if (t.size() == 2)
        {
            uint32 itemId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
            if (!proto)
                return Fail(handler, "unknown item " + t[1]);
            line += " item=" + std::to_string(itemId) + " canuse=" + std::to_string(uint32(p->CanUseItem(proto)));
        }
        handler->SendSysMessage(line.c_str());
        return true;
    }

    // .arpg test professions <char>   the professions panel line of an online character
    static bool HandleTestProfessions(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test professions <char>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        handler->SendSysMessage(ProfessionsDataLine(p).c_str());
        return true;
    }

    // .professions data   (player) the panel line for ArpgUI
    static bool HandleMyProfessionsData(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        handler->SendSysMessage(ProfessionsDataLine(p).c_str());
        return true;
    }

    // .arpg loot budget <playerLevel> <itemId>   B for that item's slot at that level (no character needed)
    static bool HandleLootBudget(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2)
            return Fail(handler, "usage: .arpg loot budget <playerLevel> <itemId>");
        int level = std::atoi(t[0].c_str());
        uint32 itemId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        if (level < 1 || level > 255)
            return Fail(handler, "bad level");
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            return Fail(handler, "unknown item " + t[1]);
        Arpg::Loot::BudgetInfo b = Arpg::Loot::BudgetB(proto, uint8(level));
        handler->PSendSysMessage("item=%u invtype=%u level=%d ilvl=%u slot=%d B=%u", itemId, uint32(proto->InventoryType), level,
            b.itemLevel, b.slotOffset, b.budget);
        return true;
    }

    // .arpg loot enchants   native "+N stat" enchantments available per affix stat (count, min-max)
    static bool HandleLootEnchants(ChatHandler* handler, char const* /*args*/)
    {
        handler->PSendSysMessage("%s", Arpg::Loot::DescribeNativeEnchants().c_str());
        return true;
    }

    // .arpg loot last <char>   the last item rolled for the character, read back from item_instance_arpg_affix
    static bool HandleLootLast(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg loot last <char>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p)
            return Fail(handler, "character not online");
        handler->PSendSysMessage("%s", Arpg::Loot::DescribeLastItem(p).c_str());
        return true;
    }

    // .arpg test give <char> <itemId>   creates the item in the character's bags (real store path) and rolls affixes as
    // "Command" source - so tests can make items of a chosen quality without killing anything.
    static bool HandleTestGive(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        char const* usage = "usage: .arpg test give <char> <itemId> [boss <creatureEntry>] [raid <tier>]";
        if (t.size() < 2 || t.size() % 2 != 0 || !IsNameLike(t[0]))
            return Fail(handler, usage);
        uint32 bossEntry = 0;
        int raidTier = 0;
        for (size_t i = 2; i + 1 < t.size(); i += 2)
        {
            if (t[i] == "boss")
                bossEntry = uint32(std::strtoul(t[i + 1].c_str(), nullptr, 10));
            else if (t[i] == "raid")
                raidTier = atoi(t[i + 1].c_str());
            else
                return Fail(handler, usage);
        }
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        if (raidTier > 0)
            Arpg::Loot::SetTestRaidTier(p, raidTier);
        uint32 itemId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        if (!sObjectMgr->GetItemTemplate(itemId))
            return Fail(handler, "unknown item " + t[1]);
        ItemPosCountVec dest;
        if (p->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, 1) != EQUIP_ERR_OK)
            return Fail(handler, "bags full");
        Item* item = p->StoreNewItem(dest, itemId, true);
        if (!item)
            return Fail(handler, "could not create item");
        p->SendNewItem(item, 1, true, false);
        std::string why;
        int n = bossEntry ? Arpg::Loot::RollForItem(p, item, ItemPickupSourceType::CreatureLoot, why, bossEntry)
                          : Arpg::Loot::RollForItem(p, item, ItemPickupSourceType::Command, why);
        handler->PSendSysMessage("ok %s item=%u guid=%u quality=%u affixes=%d%s", p->GetName().c_str(), itemId,
            item->GetGUID().GetCounter(), uint32(item->GetTemplate()->Quality), n, n ? "" : (" (" + why + ")").c_str());
        return true;
    }

    // .arpg test equip <char> <itemId>   creates the item (affixes roll as for loot), equips it in its slot (the old item
    // goes to the bags) and reports the measured stat change against the expected one (new item - old item).
    static bool HandleTestEquip(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test equip <char> <itemId>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 itemId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        if (!sObjectMgr->GetItemTemplate(itemId))
            return Fail(handler, "unknown item " + t[1]);
        ItemPosCountVec bagDest;
        if (p->CanStoreNewItem(NULL_BAG, NULL_SLOT, bagDest, itemId, 1) != EQUIP_ERR_OK)
            return Fail(handler, "bags full");
        Item* item = p->StoreNewItem(bagDest, itemId, true);
        if (!item)
            return Fail(handler, "could not create item");
        std::string why;
        int n = Arpg::Loot::RollForItem(p, item, ItemPickupSourceType::Command, why);
        uint16 equipDest = 0;
        InventoryResult res = p->CanEquipItem(NULL_SLOT, equipDest, item, true);
        if (res != EQUIP_ERR_OK)
            return Fail(handler, "cannot equip (code " + std::to_string(uint32(res)) + ")");
        Item const* old = p->GetItemByPos(equipDest);
        StatVec expected = ItemTotals(item);
        StatVec oldTotals = ItemTotals(old);
        for (size_t i = 0; i < kProbeStats; ++i)
            expected[i] -= oldTotals[i];
        StatVec before = Snapshot(p);
        p->SwapItem(item->GetPos(), equipDest);
        StatVec after = Snapshot(p);
        handler->PSendSysMessage("ok equipped item=%u guid=%u slot=%u affixes=%d%s", itemId, item->GetGUID().GetCounter(),
            uint32(equipDest & 255), n, CompareStats(before, after, expected).c_str());
        return true;
    }

    // .arpg test unequip <char> <slot>   moves the equipped item (slot 0-18, e.g. 4 = chest) to the bags; reports the
    // measured stat change against minus the item's totals.
    static bool HandleTestUnequip(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test unequip <char> <slot 0-18>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 slot = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        if (slot >= EQUIPMENT_SLOT_END)
            return Fail(handler, "slot must be 0-18");
        Item* item = p->GetItemByPos(INVENTORY_SLOT_BAG_0, uint8(slot));
        if (!item)
            return Fail(handler, "slot empty");
        ItemPosCountVec dest;
        if (p->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false) != EQUIP_ERR_OK)
            return Fail(handler, "bags full");
        StatVec expected = ItemTotals(item);
        for (int64& e : expected)
            e = -e;
        StatVec before = Snapshot(p);
        p->RemoveItem(INVENTORY_SLOT_BAG_0, uint8(slot), true);
        p->StoreItem(dest, item, true);
        StatVec after = Snapshot(p);
        handler->PSendSysMessage("ok unequipped guid=%u slot=%u%s", item->GetGUID().GetCounter(), slot, CompareStats(before, after, expected).c_str());
        return true;
    }

    // .arpg loot applied <char>   affix stats mod_arpg currently applies to the character
    static bool HandleLootApplied(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg loot applied <char>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        handler->PSendSysMessage("%s", Arpg::Loot::DescribeApplied(p).c_str());
        return true;
    }

    // .arpg test probeitem <char> <itemId> [enchantId]   client-patch spike (m2 4.6): a new item WITHOUT affixes whose first
    // random-property display slot (9) holds enchantId (default 60000 = the probe row of arpg/client-patch/build-probe-patch.py).
    // The tooltip shows the enchant's text only if the client loaded our patch MPQ.
    static bool HandleTestProbeItem(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() < 2 || t.size() > 3 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test probeitem <char> <itemId> [enchantId]");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 itemId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        uint32 enchantId = t.size() == 3 ? uint32(std::strtoul(t[2].c_str(), nullptr, 10)) : 60000u;
        if (!sObjectMgr->GetItemTemplate(itemId))
            return Fail(handler, "unknown item " + t[1]);
        ItemPosCountVec dest;
        if (p->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, 1) != EQUIP_ERR_OK)
            return Fail(handler, "bags full");
        Item* item = p->StoreNewItem(dest, itemId, true);
        if (!item)
            return Fail(handler, "could not create item");
        item->SetEnchantment(PROP_ENCHANTMENT_SLOT_1, enchantId, 0, 0);   // slot 9: the client hides slot 8 (reforge)
        item->SetState(ITEM_CHANGED, p);
        p->SendNewItem(item, 1, true, false);
        handler->PSendSysMessage("ok %s item=%u guid=%u enchant=%u (server knows it: %s)", p->GetName().c_str(), itemId,
            item->GetGUID().GetCounter(), enchantId, sSpellItemEnchantmentStore.LookupEntry(enchantId) ? "yes" : "no");
        return true;
    }

    // Nearest living hostile creature within 30 yards of the character (test commands below).
    static Creature* NearestHostile(Player* p)
    {
        std::list<Unit*> units;
        Trinity::AnyUnitInObjectRangeCheck check(p, 30.0f);
        Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(p, units, check);
        Cell::VisitAllObjects(p, searcher, 30.0f);
        Creature* best = nullptr;
        for (Unit* u : units)
            if (Creature* c = u->ToCreature())
                if (c->IsAlive() && p->IsValidAttackTarget(c) && (!best || p->GetExactDist(c) < p->GetExactDist(best)))
                    best = c;
        return best;
    }

    // .arpg test strike <char> [n]   the character swings n times (real melee swings: affix on-hit hooks run) at the
    // nearest hostile creature. Reports the creature's health before/after.
    static bool HandleTestStrike(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.empty() || t.size() > 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test strike <char> [n]");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        int n = t.size() == 2 ? std::max(1, std::min(20, atoi(t[1].c_str()))) : 1;
        Creature* c = NearestHostile(p);
        if (!c)
            return Fail(handler, "no hostile creature within 30 yards");
        uint32 before = c->GetHealth();
        for (int i = 0; i < n && c->IsAlive(); ++i)
            p->AttackerStateUpdate(c, true);
        handler->PSendSysMessage("ok strike entry=%u hp %u -> %u alive=%s", c->GetEntry(), before, c->GetHealth(), c->IsAlive() ? "yes" : "no");
        return true;
    }

    // .arpg test struck <char> [n]   the nearest hostile creature swings n times at the character (victim-side affixes).
    static bool HandleTestStruck(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.empty() || t.size() > 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test struck <char> [n]");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        int n = t.size() == 2 ? std::max(1, std::min(20, atoi(t[1].c_str()))) : 1;
        Creature* c = NearestHostile(p);
        if (!c)
            return Fail(handler, "no hostile creature within 30 yards");
        uint32 before = p->GetHealth();
        // Stops below 30% health: a test must never get its character killed.
        for (int i = 0; i < n && p->IsAlive() && c->IsAlive() && p->GetHealthPct() > 30.0f; ++i)
            c->AttackerStateUpdate(p, true);
        handler->PSendSysMessage("ok struck entry=%u char hp %u -> %u creature alive=%s", c->GetEntry(), before, p->GetHealth(), c->IsAlive() ? "yes" : "no");
        return true;
    }

    // .arpg test effect <char> <effect> <value>   adds a test effect line (negative value removes it). Names = arpg_affix.effect.
    static bool HandleTestEffect(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 3 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test effect <char> <effect> <value>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        std::string msg;
        if (!Arpg::Loot::AddTestEffect(p, t[1], atoi(t[2].c_str()), msg))
            return Fail(handler, msg);
        handler->PSendSysMessage("ok %s", msg.c_str());
        return true;
    }

    // .arpg test xp <char> <amount>   gives XP through the core's GiveXP (same hooks as kills and quests: XP affixes).
    static bool HandleTestXp(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test xp <char> <amount>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 amount = uint32(std::max(1, atoi(t[1].c_str())));
        uint32 before = p->GetUInt32Value(PLAYER_FIELD_XP);
        p->GiveXP(amount, nullptr);
        handler->PSendSysMessage("ok xp given=%u xp %u -> %u level=%u", amount, before, p->GetUInt32Value(PLAYER_FIELD_XP), uint32(p->GetLevel()));
        return true;
    }

    // .arpg loot substitute <char> <itemId>   what a drop of itemId becomes for the character where it stands (task 4.8)
    static bool HandleLootSubstitute(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg loot substitute <char> <itemId>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 itemId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        ItemTemplate const* from = sObjectMgr->GetItemTemplate(itemId);
        if (!from)
            return Fail(handler, "unknown item " + t[1]);
        std::string why;
        uint32 to = Arpg::Loot::SubstituteFor(p, itemId, why);
        ItemTemplate const* toProto = to ? sObjectMgr->GetItemTemplate(to) : nullptr;
        if (!toProto)
            handler->PSendSysMessage("kept item=%u req=%u zone=%u area=%u (%s)", itemId, uint32(from->RequiredLevel), p->GetZoneId(), p->GetAreaId(), why.c_str());
        else
            handler->PSendSysMessage("substitute item=%u req=%u -> item=%u req=%u quality=%u zone=%u area=%u (%s)", itemId, uint32(from->RequiredLevel),
                to, uint32(toProto->RequiredLevel), uint32(toProto->Quality), p->GetZoneId(), p->GetAreaId(), why.c_str());
        return true;
    }

    // .arpg pity info <char> | .arpg pity set <char> godroll <n>   (task 1.4)
    static bool HandlePityInfo(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg pity info <char>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        handler->PSendSysMessage("%s", Arpg::Loot::DescribePity(p).c_str());
        return true;
    }

    static bool HandlePitySet(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        bool godroll = t.size() == 3 && t[1] == "godroll", unique = t.size() == 4 && t[1] == "unique";
        bool craft = t.size() == 4 && t[1] == "craft";
        if ((!godroll && !unique && !craft) || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg pity set <char> godroll <n> | unique <bossEntry>|superunique <misses> | craft <skillLine> <misses>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        if (godroll)
            Arpg::Loot::SetGodRollKills(p, atoi(t[2].c_str()));
        else if (craft)
        {
            uint32 line = uint32(std::strtoul(t[2].c_str(), nullptr, 10));
            Arpg::Loot::SetCraftUniqueMisses(p, line, atoi(t[3].c_str()));
            handler->PSendSysMessage("ok craft_misses skill_line=%u = %d", line, Arpg::Loot::CraftUniqueMisses(p, line));
            return true;
        }
        else
        {
            // "superunique" = the shared Superunique counter
            uint32 boss = t[2] == "superunique" ? Arpg::Loot::SharedSuperuniquePityKey : uint32(std::strtoul(t[2].c_str(), nullptr, 10));
            Arpg::Loot::SetUniqueMisses(p, boss, atoi(t[3].c_str()));
            handler->PSendSysMessage("ok unique_misses boss=%u = %d", boss, Arpg::Loot::UniqueMisses(p, boss));
            return true;
        }
        handler->PSendSysMessage("ok %s", Arpg::Loot::DescribePity(p).c_str());
        return true;
    }

    // .arpg loot isboss <creatureEntry>
    static bool HandleLootIsBoss(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1)
            return Fail(handler, "usage: .arpg loot isboss <creatureEntry>");
        uint32 entry = uint32(std::strtoul(t[0].c_str(), nullptr, 10));
        handler->PSendSysMessage("entry=%u boss=%s", entry, Arpg::Loot::IsBossEntry(entry) ? "yes" : "no");
        return true;
    }

    // .arpg test unique <char> <uniqueId> <tier>   creates the Unique on an epic base of its slot near the character's loot
    // level: up to 4 rolled affixes + the power (tier, rolled numbers) as the 5th line, named after the Unique.
    static bool HandleTestUnique(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if ((t.size() != 3 && !(t.size() == 4 && t[3] == "equip")) || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test unique <char> <uniqueId> <tier 1-8> [equip]");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 uniqueId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        int tier = atoi(t[2].c_str());
        uint32 inv = 0, armor = 0;
        if (!Arpg::Loot::UniqueSlot(uniqueId, inv, armor))
            return Fail(handler, "unknown unique " + t[1]);
        uint32 base = Arpg::Loot::FindBaseItem(inv, armor, ITEM_QUALITY_EPIC, Arpg::Loot::LootLevelFor(p));
        if (!base)
            base = Arpg::Loot::FindBaseItem(inv, armor, ITEM_QUALITY_RARE, Arpg::Loot::LootLevelFor(p));
        if (!base)
            return Fail(handler, "no base item for that slot near your level");
        ItemPosCountVec dest;
        if (p->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, base, 1) != EQUIP_ERR_OK)
            return Fail(handler, "bags full");
        Item* item = p->StoreNewItem(dest, base, true);
        if (!item)
            return Fail(handler, "could not create item");
        std::string why, msg;
        int n = Arpg::Loot::RollForItem(p, item, ItemPickupSourceType::Command, why, 0, 4);
        if (!Arpg::Loot::MakeUnique(p, item, uniqueId, tier, msg))
            return Fail(handler, msg);
        p->SendNewItem(item, 1, true, false);
        std::string equipError;
        if (t.size() == 4)
        {
            // Test tool: armour can't be changed in combat (a previous test's monsters may still be on the character).
            if (p->IsInCombat())
                p->CombatStop(true);
            uint16 equipDest = 0;
            InventoryResult res = p->CanEquipItem(NULL_SLOT, equipDest, item, true);
            if (res == EQUIP_ERR_OK)
                p->SwapItem(item->GetPos(), equipDest);
            else
                equipError = " equip_error=" + std::to_string(uint32(res));
        }
        handler->PSendSysMessage("ok %s base=%u guid=%u affixes=%d%s%s %s", p->GetName().c_str(), base, item->GetGUID().GetCounter(), n,
            item->IsEquipped() ? " equipped" : "", equipError.c_str(), msg.c_str());
        return true;
    }

    // .arpg loot effects <char>   effect totals (capped) the character has from equipped items (+ test effects)
    static bool HandleLootEffects(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg loot effects <char>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        handler->PSendSysMessage("%s", Arpg::Loot::DescribeEffects(p).c_str());
        return true;
    }

    // .arpg test suffixprobe <char> <itemId> <suffixId>   display probe (5th line + names): a new item without affixes whose
    // display slots 8-12 hold "+1".."+5 Strength" (our client rows) and whose random-suffix id is -suffixId (client patch row).
    static bool HandleTestSuffixProbe(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 3 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test suffixprobe <char> <itemId> <suffixId>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 itemId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        int32 suffixId = atoi(t[2].c_str());
        if (!sObjectMgr->GetItemTemplate(itemId))
            return Fail(handler, "unknown item " + t[1]);
        ItemPosCountVec dest;
        if (p->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, 1) != EQUIP_ERR_OK)
            return Fail(handler, "bags full");
        Item* item = p->StoreNewItem(dest, itemId, true);
        if (!item)
            return Fail(handler, "could not create item");
        Arpg::Settings s = sArpgConfig.Get();
        for (uint32 i = 0; i < 5; ++i)   // "+1".."+5 Strength" rows
            item->SetEnchantment(EnchantmentSlot(PROP_ENCHANTMENT_SLOT_0 + i), s.displayBase + 1 + i, 0, 0);
        item->SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID, -suffixId);   // field only: the server never applies it
        item->SetState(ITEM_CHANGED, p);
        p->SendNewItem(item, 1, true, false);
        handler->PSendSysMessage("ok suffixprobe item=%u guid=%u suffix=%d slots 8-12 = +1..+5 Strength", itemId,
            item->GetGUID().GetCounter(), suffixId);
        return true;
    }

    // .arpg test clearbags <char>   destroys everything in the backpack and bags (not equipped gear, not the bags).
    // Test characters only: GM tests give items and would otherwise fill the bags. Affix rows of destroyed items are
    // removed at the next start (CleanOrphanAffixes).
    // .arpg test craft <char> <spellId> [skill <n>] [catalyst]   creates a recipe's item for the character the way a real
    // craft does (Spell source = the recipe): the recipe colour comes from the skill (real, or the override), SPEC 4.8.
    static bool HandleTestCraft(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() < 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test craft <char> <spellId> [skill <n>] [catalyst]");
        int skill = -1;
        bool catalyst = false;
        for (size_t i = 2; i < t.size(); ++i)
        {
            if (t[i] == "catalyst")
                catalyst = true;
            else if (t[i] == "skill" && i + 1 < t.size())
                skill = atoi(t[++i].c_str());
            else
                return Fail(handler, "usage: .arpg test craft <char> <spellId> [skill <n>] [catalyst]");
        }
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 spellId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId);
        if (!spell)
            return Fail(handler, "unknown spell " + t[1]);
        uint32 itemId = 0;
        for (uint8 e = 0; e < MAX_SPELL_EFFECTS; ++e)
            if (spell->Effects[e].Effect == SPELL_EFFECT_CREATE_ITEM && spell->Effects[e].ItemType)
            {
                itemId = spell->Effects[e].ItemType;
                break;
            }
        if (!itemId || !sObjectMgr->GetItemTemplate(itemId))
            return Fail(handler, "spell " + t[1] + " creates no item");
        Arpg::Loot::SetCraftOverride(p, skill, catalyst);
        Arpg::Loot::CraftInfo info = Arpg::Loot::CraftFor(p, spellId);     // for the report (consumes the override)
        if (!info.isCraft)
            return Fail(handler, "spell " + t[1] + " is not a tradeskill recipe");
        std::string report = Arpg::Loot::DescribeCraft(p, info);
        Arpg::Loot::SetCraftOverride(p, skill, catalyst);                  // again, for the roll itself
        if (ArpgHooks::CraftedItem)                                       // as Spell::DoCreateItem: crafted gear -> its blank
            itemId = ArpgHooks::CraftedItem(p, spellId, itemId);
        ItemPosCountVec dest;
        if (p->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, 1) != EQUIP_ERR_OK)
            return Fail(handler, "bags full");
        Item* item = p->StoreNewItem(dest, itemId, true);
        if (!item)
            return Fail(handler, "could not create item");
        p->SendNewItem(item, 1, true, true);
        std::string why;
        int n = Arpg::Loot::RollForItem(p, item, ItemPickupSourceType::Spell, why, spellId);
        handler->PSendSysMessage("ok %s spell=%u item=%u guid=%u quality=%u affixes=%d %s%s", p->GetName().c_str(), spellId, itemId,
            item->GetGUID().GetCounter(), uint32(item->GetTemplate()->Quality), n, report.c_str(), n ? "" : (" (" + why + ")").c_str());
        return true;
    }

    // .arpg champion clear <char> <spawnId>   turns that spawn (near the character) back into its plain creature if it rolled
    // Champion (tests keep their probe creatures plain)
    static bool HandleChampionClear(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg champion clear <char> <spawnId>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 spawnId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        float radius = sArpgConfig.Get().scaleRadius;
        Creature* c = nullptr;
        std::list<Unit*> units;
        Trinity::AnyUnitInObjectRangeCheck check(p, radius);
        Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(p, units, check);
        Cell::VisitAllObjects(p, searcher, radius);
        for (Unit* u : units)
            if (Creature* cr = u->ToCreature())
                if (cr->GetDBTableGUIDLow() == spawnId)
                    c = cr;
        if (!c)
            return Fail(handler, "spawn " + t[1] + " not near " + p->GetName());
        std::string out;
        Arpg::Monsters::UnmakeChampion(c, out);
        handler->PSendSysMessage("ok %s", out.c_str());
        return true;
    }

    // .arpg champion make <char> <creatureEntry>   spawns the creature next to the character and makes it a Champion (band by
    // the character's level, minions, affixes) - the same code the first-sight roll uses.
    static bool HandleChampionMake(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if ((t.size() != 2 && !(t.size() == 4 && t[2] == "affix")) || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg champion make <char> <creatureEntry> [affix <id,id,...>]");
        std::vector<uint32> forced;
        if (t.size() == 4)
        {
            std::string cur;
            for (char ch : t[3] + ",")
                if (ch >= '0' && ch <= '9')
                    cur += ch;
                else if (!cur.empty())
                {
                    forced.push_back(uint32(std::strtoul(cur.c_str(), nullptr, 10)));
                    cur.clear();
                }
        }
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 entry = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        if (!sObjectMgr->GetCreatureTemplate(entry))
            return Fail(handler, "unknown creature entry " + t[1]);
        Position pos = p->GetNearPosition(4.0f, 0.0f);   // inside the Fire Enchanted blast radius
        Creature* c = p->SummonCreature(entry, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
            TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, Milliseconds(300000));
        if (!c)
            return Fail(handler, "could not spawn");
        std::string out;
        if (!Arpg::Monsters::MakeChampion(c, p, out, forced))
            return Fail(handler, out);
        handler->PSendSysMessage("ok %s %s", p->GetName().c_str(), out.c_str());
        return true;
    }

    // .arpg champion strike <char> [n]   the last Champion attacks the character n times (real melee path, in combat);
    // stops below 30% health so a test never kills its character.
    static bool HandleChampionStrike(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.empty() || t.size() > 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg champion strike <char> [n]");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        Creature* c = Arpg::Monsters::LastChampionOf(p);
        if (!c || !c->IsAlive())
            return Fail(handler, "no live champion");
        int n = t.size() == 2 ? std::max(1, std::min(20, atoi(t[1].c_str()))) : 1;
        p->SetHealth(p->GetMaxHealth());
        if (c->IsAIEnabled)
            c->AI()->AttackStart(p);
        uint32 before = p->GetHealth();
        for (int i = 0; i < n && p->IsAlive() && c->IsAlive() && p->GetHealthPct() > 30.0f; ++i)
            c->AttackerStateUpdate(p, true);
        handler->PSendSysMessage("ok struck by champion entry=%u char hp %u -> %u in_combat=%d", Arpg::Monsters::BaseEntry(c->GetEntry()), before, p->GetHealth(),
            c->IsInCombat() ? 1 : 0);
        return true;
    }

    // .arpg champion info <char>   the last Champion made for the character
    static bool HandleChampionInfo(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg champion info <char>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        Creature* c = Arpg::Monsters::LastChampionOf(p);
        if (!c)
            return Fail(handler, "no champion");
        handler->PSendSysMessage("%s alive=%d pinned=%d", Arpg::Monsters::DescribeChampion(c).c_str(), c->IsAlive() ? 1 : 0,
            Arpg::Scaling::IsPinned(c) ? 1 : 0);
        return true;
    }

    // .arpg champion kill <char>   the character kills its last Champion (real kill path: loot, orb, kill scripts)
    static bool HandleChampionKill(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if ((t.size() != 1 && !(t.size() == 2 && t[1] == "all")) || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg champion kill <char> [all]");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        Creature* c = Arpg::Monsters::LastChampionOf(p);
        if (!c || !c->IsAlive())
            return Fail(handler, "no live champion");
        if (t.size() == 2)   // "all": its minions die too (tests that need the fight to be over)
            for (uint64 g : Arpg::Monsters::ChampionMinions(c))
                if (Creature* m = ObjectAccessor::GetCreature(*p, ObjectGuid(g)))
                    if (m->IsAlive())
                    {
                        m->SetLootRecipient(p);
                        p->Kill(m);
                    }
        // Start from half health/power so the orb shows.
        p->SetHealth(std::max<uint32>(1, p->GetMaxHealth() / 2));
        p->SetPower(p->GetPowerType(), p->GetMaxPower(p->GetPowerType()) / 2);
        c->SetLootRecipient(p);
        p->Kill(c);
        uint32 items = 0;
        for (LootItem const& li : c->loot.items)
            if (!li.is_looted)
                ++items;
        handler->PSendSysMessage("ok %s killed champion entry=%u health_pct=%u loot_items=%u", p->GetName().c_str(), Arpg::Monsters::BaseEntry(c->GetEntry()),
            uint32(p->GetHealthPct()), items);
        return true;
    }

    // .arpg champion loot <char>   the character loots every item from its last (dead) Champion into its bags
    static bool HandleChampionLoot(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg champion loot <char>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        Creature* c = Arpg::Monsters::LastChampionOf(p);
        if (!c || c->IsAlive())
            return Fail(handler, "no dead champion");
        p->SetLootGUID(c->GetGUID());
        uint32 n = 0;
        for (uint8 i = 0; i < uint8(c->loot.items.size()); ++i)
            if (!c->loot.items[i].is_looted && !c->loot.items[i].currency)
            {
                p->StoreLootItem(i, &c->loot, c);
                ++n;
            }
        p->SetLootGUID(ObjectGuid::Empty);
        handler->PSendSysMessage("ok %s looted=%u", p->GetName().c_str(), n);
        return true;
    }

    static Player* OnlineChar(ChatHandler* handler, std::vector<std::string> const& t, char const* usage)
    {
        if (t.empty() || !IsNameLike(t[0]))
        {
            Fail(handler, usage);
            return nullptr;
        }
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
        {
            Fail(handler, "character not online");
            return nullptr;
        }
        return p;
    }

    // .arpg superunique spawn <char> [id]   spawns a Superunique of the character's zone next to it (replaces the live one)
    static bool HandleSuperuniqueSpawn(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg superunique spawn <char> [id]");
        if (!p)
            return false;
        std::string out;
        if (!Arpg::Monsters::SpawnSuperuniqueNear(p, t.size() > 1 ? uint32(std::strtoul(t[1].c_str(), nullptr, 10)) : 0, out))
            return Fail(handler, out);
        handler->PSendSysMessage("ok %s %s", p->GetName().c_str(), out.c_str());
        return true;
    }

    // .arpg superunique next <char>   the zone's next spawn happens on the next tick (skips the respawn delay)
    static bool HandleSuperuniqueNext(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg superunique next <char>");
        if (!p)
            return false;
        Arpg::Monsters::SkipSuperuniqueDelay(p);
        handler->PSendSysMessage("ok %s next spawn now", p->GetName().c_str());
        return true;
    }

    // .arpg superunique info <char>   the character's zone: pool, live one, respawn timer
    static bool HandleSuperuniqueInfo(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg superunique info <char>");
        if (!p)
            return false;
        handler->PSendSysMessage("%s", Arpg::Monsters::DescribeSuperuniques(p).c_str());
        return true;
    }

    // .arpg superunique kill <char>   the character kills its zone's live Superunique (real kill path)
    static bool HandleSuperuniqueKill(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg superunique kill <char>");
        if (!p)
            return false;
        Creature* c = Arpg::Monsters::ZoneSuperunique(p);
        if (!c || !c->IsAlive())
            return Fail(handler, "no live superunique in reach");
        p->SetHealth(std::max<uint32>(1, p->GetMaxHealth() / 2));
        c->SetLootRecipient(p);
        p->Kill(c);
        uint32 items = 0;
        for (LootItem const& li : c->loot.items)
            if (!li.is_looted)
                ++items;
        handler->PSendSysMessage("ok %s killed superunique entry=%u health_pct=%u loot_items=%u", p->GetName().c_str(), Arpg::Monsters::BaseEntry(c->GetEntry()),
            uint32(p->GetHealthPct()), items);
        return true;
    }

    // .arpg superunique loot <char>   loots every item of the last Superunique the character killed
    static bool HandleSuperuniqueLoot(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg superunique loot <char>");
        if (!p)
            return false;
        Creature* c = Arpg::Monsters::LastKilledSuperunique(p);
        if (!c || c->IsAlive())
            return Fail(handler, "no dead superunique");
        p->SetLootGUID(c->GetGUID());
        uint32 n = 0;
        for (uint8 i = 0; i < uint8(c->loot.items.size()); ++i)
            if (!c->loot.items[i].is_looted && !c->loot.items[i].currency)
            {
                p->StoreLootItem(i, &c->loot, c);
                ++n;
            }
        p->SetLootGUID(ObjectGuid::Empty);
        handler->PSendSysMessage("ok %s looted=%u", p->GetName().c_str(), n);
        return true;
    }

    // .arpg superunique fight <char>   the zone's Superunique attacks the character (abilities run on its tick)
    static bool HandleSuperuniqueFight(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg superunique fight <char>");
        if (!p)
            return false;
        Creature* c = Arpg::Monsters::ZoneSuperunique(p);
        if (!c || !c->IsAlive())
            return Fail(handler, "no live superunique in reach");
        p->SetHealth(p->GetMaxHealth());
        // Test tool: the character holds aggro for sure (Goldshire guards would otherwise take it - and the kit would rightly
        // go at them, the aggro holder).
        c->GetThreatManager().AddThreat(p, 100000.0f, nullptr, true, true);
        c->GetThreatManager().FixateTarget(p);
        if (c->IsAIEnabled)
            c->AI()->AttackStart(p);
        handler->PSendSysMessage("ok %s fighting entry=%u in_combat=%d", p->GetName().c_str(), Arpg::Monsters::BaseEntry(c->GetEntry()), c->IsInCombat() ? 1 : 0);
        return true;
    }

    // .arpg test potion <char> <itemId>   drinks one (put in the bags if missing) from half health/power through the real cast
    // path: heal and resource by the % rules, the shared cooldown
    static bool HandleTestPotion(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test potion <char> <itemId>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        handler->PSendSysMessage("%s", Arpg::Loot::TestDrink(p, uint32(std::strtoul(t[1].c_str(), nullptr, 10))).c_str());
        return true;
    }

    // .arpg paragon info <char>
    static bool HandleParagonInfo(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg paragon info <char>");
        if (!p)
            return false;
        handler->PSendSysMessage("%s", Arpg::Paragon::Describe(p).c_str());
        return true;
    }

    // .arpg paragon set <char> <level> [xp]
    static bool HandleParagonSet(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() < 2)
            return Fail(handler, "usage: .arpg paragon set <char> <level> [xp]");
        Player* p = OnlineChar(handler, t, "usage: .arpg paragon set <char> <level> [xp]");
        if (!p)
            return false;
        Arpg::Paragon::SetParagon(p, atoi(t[1].c_str()), t.size() > 2 ? std::strtoull(t[2].c_str(), nullptr, 10) : 0);
        handler->PSendSysMessage("ok %s", Arpg::Paragon::Describe(p).c_str());
        return true;
    }

    static Player* Me(ChatHandler* handler)
    {
        WorldSession* s = handler->GetSession();
        return s ? s->GetPlayer() : nullptr;
    }

    // ArpgUI data lines (hidden by the addon): "ARPGF|count|rule|rule|..." and "ARPGL|on|model".
    static std::string FilterDataLine(Player* p)
    {
        std::vector<std::string> rules = Arpg::Loot::FilterRules(p);
        std::string out = "ARPGF|" + std::to_string(rules.size());
        for (std::string const& r : rules)
            out += "|" + r;
        return out;
    }

    static std::string LootPetDataLine(Player* p)
    {
        return std::string("ARPGL|") + (Arpg::Loot::LootPetOn(p) ? "1" : "0");
    }

    static bool HandleFilterData(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg filter data <char>");
        if (!p)
            return false;
        handler->PSendSysMessage("%s", FilterDataLine(p).c_str());
        return true;
    }

    static bool HandleMyLootPetOn(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        Arpg::Loot::SetLootPet(p, true);
        handler->SendSysMessage(LootPetDataLine(p).c_str());
        return true;
    }

    static bool HandleMyLootPetOff(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        Arpg::Loot::SetLootPet(p, false);
        handler->SendSysMessage(LootPetDataLine(p).c_str());
        return true;
    }

    static bool HandleMyLootPetData(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        handler->SendSysMessage(LootPetDataLine(p).c_str());
        return true;
    }

    static bool HandleMyFilterData(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        handler->SendSysMessage(FilterDataLine(p).c_str());
        return true;
    }

    // .lootfilter add <rule>
    static bool HandleMyFilterAdd(ChatHandler* handler, char const* args)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        std::string err;
        if (!Arpg::Loot::AddFilterRule(p, args ? std::string(args) : std::string(), err))
            handler->PSendSysMessage("%s", err.c_str());
        handler->SendSysMessage(FilterDataLine(p).c_str());
        return true;
    }

    static bool HandleMyFilterRemove(ChatHandler* handler, char const* args)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        std::string err;
        if (!Arpg::Loot::RemoveFilterRule(p, args ? atoi(args) : 0, err))
            handler->PSendSysMessage("%s", err.c_str());
        handler->SendSysMessage(FilterDataLine(p).c_str());
        return true;
    }

    // .lootfilter move <from> <to>
    static bool HandleMyFilterMove(ChatHandler* handler, char const* args)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        std::vector<std::string> t = Tokens(args);
        std::string err;
        if (t.size() != 2 || !Arpg::Loot::MoveFilterRule(p, atoi(t[0].c_str()), atoi(t[1].c_str()), err))
            handler->PSendSysMessage("%s", err.empty() ? "usage: .lootfilter move <from> <to>" : err.c_str());
        handler->SendSysMessage(FilterDataLine(p).c_str());
        return true;
    }

    static bool HandleMyFilterReset(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        Arpg::Loot::ResetFilter(p);
        handler->SendSysMessage(FilterDataLine(p).c_str());
        return true;
    }

    static bool HandleMyParagonData(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        handler->SendSysMessage(Arpg::Paragon::DataLine(p).c_str());
        return true;
    }

    static bool HandleMyParagonInfo(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        handler->PSendSysMessage("%s", Arpg::Paragon::Describe(p).c_str());
        handler->PSendSysMessage("%s", Arpg::Paragon::DescribeApplied(p).c_str());
        return true;
    }

    // .paragon spend <core|offense|defense|utility> <stat> [n]
    static bool HandleMyParagonSpend(ChatHandler* handler, char const* args)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        std::vector<std::string> t = Tokens(args);
        int tab = 0, stat = 0;
        if (t.size() < 2 || !Arpg::Paragon::ParseTabStat(t[0], t[1], tab, stat))
            return Fail(handler, "usage: .paragon spend <core|offense|defense|utility> <stat> [n]");
        std::string out;
        if (!Arpg::Paragon::Spend(p, tab, stat, t.size() > 2 ? std::max(1, atoi(t[2].c_str())) : 1, out))
            handler->PSendSysMessage("%s", out.c_str());
        Arpg::Paragon::SendData(p);
        return true;
    }

    static bool HandleMyParagonRespec(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = Me(handler);
        if (!p)
            return false;
        Arpg::Paragon::Respec(p);
        handler->SendSysMessage("Paragon points refunded.");
        Arpg::Paragon::SendData(p);
        return true;
    }

    // .arpg paragon spend <char> <tab> <stat> [n]   tab: core|offense|defense|utility; stat: strength|agility|intellect|stamina, crit|haste|mastery,
    // armor|dodge|parry, speed|magicfind|goldfind (Diablo 3 rotation: only points the tab has earned)
    static bool HandleParagonSpend(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        char const* usage = "usage: .arpg paragon spend <char> <core|offense|defense|utility> <stat> [n]";
        if (t.size() < 3 || t.size() > 4)
            return Fail(handler, usage);
        Player* p = OnlineChar(handler, t, usage);
        if (!p)
            return false;
        int tab = 0, stat = 0;
        if (!Arpg::Paragon::ParseTabStat(t[1], t[2], tab, stat))
            return Fail(handler, usage);
        std::string out;
        if (!Arpg::Paragon::Spend(p, tab, stat, t.size() == 4 ? atoi(t[3].c_str()) : 1, out))
            return Fail(handler, out);
        handler->PSendSysMessage("ok %s | %s", out.c_str(), Arpg::Paragon::DescribeApplied(p).c_str());
        return true;
    }

    // .arpg paragon data <char>   the addon's data line (tests)
    static bool HandleParagonData(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg paragon data <char>");
        if (!p)
            return false;
        handler->PSendSysMessage("%s", Arpg::Paragon::DataLine(p).c_str());
        return true;
    }

    // .arpg paragon respec <char>   free (owner): every point back
    static bool HandleParagonRespec(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg paragon respec <char>");
        if (!p)
            return false;
        Arpg::Paragon::Respec(p);
        handler->PSendSysMessage("ok respec | %s | %s", Arpg::Paragon::Describe(p).c_str(), Arpg::Paragon::DescribeApplied(p).c_str());
        return true;
    }

    // .arpg paragon xp <char> <amount>   gives XP through the core's GiveXP (at the max level it becomes Paragon XP)
    static bool HandleParagonXp(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2)
            return Fail(handler, "usage: .arpg paragon xp <char> <amount>");
        Player* p = OnlineChar(handler, t, "usage: .arpg paragon xp <char> <amount>");
        if (!p)
            return false;
        p->GiveXP(uint32(std::max(1, atoi(t[1].c_str()))), nullptr);
        handler->PSendSysMessage("ok %s", Arpg::Paragon::Describe(p).c_str());
        return true;
    }

    // .arpg dungeon info <char>   instance, players counted, role, solo, chosen level, raid tier
    static bool HandleDungeonInfo(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg dungeon info <char>");
        if (!p)
            return false;
        handler->PSendSysMessage("%s", Arpg::Dungeons::Describe(p).c_str());
        return true;
    }

    // .arpg dungeon level <char> <mapId> <level|0>   the level that dungeon runs at for the character (0 = own level)
    static bool HandleDungeonLevel(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 3)
            return Fail(handler, "usage: .arpg dungeon level <char> <mapId> <level|0>");
        Player* p = OnlineChar(handler, t, "usage: .arpg dungeon level <char> <mapId> <level|0>");
        if (!p)
            return false;
        uint32 map = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        int level = std::max(0, std::min(atoi(t[2].c_str()), int(p->GetLevel())));
        Arpg::Dungeons::SetChosenLevel(p, map, level);
        handler->PSendSysMessage("ok dungeon level map=%u level=%d", map, Arpg::Dungeons::ChosenLevel(p, map));
        return true;
    }

    static void SendFilterList(ChatHandler* handler, Player* p)
    {
        std::vector<std::string> rules = Arpg::Loot::FilterRules(p);
        std::string out = "filter " + std::to_string(rules.size()) + " rules";
        for (size_t i = 0; i < rules.size(); ++i)
            out += " | " + std::to_string(i + 1) + ": " + rules[i];
        handler->PSendSysMessage("%s", out.c_str());
    }

    // .arpg filter list <char>
    static bool HandleFilterList(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg filter list <char>");
        if (!p)
            return false;
        SendFilterList(handler, p);
        return true;
    }

    // .arpg filter add <char> <rule...>   e.g. ".arpg filter add Bob quality<=0 -> sell"
    static bool HandleFilterAdd(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg filter add <char> <rule>");
        if (!p || t.size() < 2)
            return p ? Fail(handler, "usage: .arpg filter add <char> <rule>") : false;
        std::string text;
        for (size_t i = 1; i < t.size(); ++i)
            text += (i > 1 ? " " : "") + t[i];
        std::string err;
        if (!Arpg::Loot::AddFilterRule(p, text, err))
            return Fail(handler, err);
        SendFilterList(handler, p);
        return true;
    }

    // .arpg filter remove <char> <position>
    static bool HandleFilterRemove(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg filter remove <char> <position>");
        if (!p || t.size() != 2)
            return p ? Fail(handler, "usage: .arpg filter remove <char> <position>") : false;
        std::string err;
        if (!Arpg::Loot::RemoveFilterRule(p, atoi(t[1].c_str()), err))
            return Fail(handler, err);
        SendFilterList(handler, p);
        return true;
    }

    // .arpg filter move <char> <from> <to>
    static bool HandleFilterMove(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg filter move <char> <from> <to>");
        if (!p || t.size() != 3)
            return p ? Fail(handler, "usage: .arpg filter move <char> <from> <to>") : false;
        std::string err;
        if (!Arpg::Loot::MoveFilterRule(p, atoi(t[1].c_str()), atoi(t[2].c_str()), err))
            return Fail(handler, err);
        SendFilterList(handler, p);
        return true;
    }

    // .arpg filter reset <char>   back to the default (sell greys)
    static bool HandleFilterReset(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg filter reset <char>");
        if (!p)
            return false;
        Arpg::Loot::ResetFilter(p);
        SendFilterList(handler, p);
        return true;
    }

    // .arpg lootpet on|off|info <char>   the loot companion (persists per character)
    static bool HandleLootPetOn(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg lootpet on <char>");
        if (!p)
            return false;
        Arpg::Loot::SetLootPet(p, true);
        handler->PSendSysMessage("ok %s", Arpg::Loot::DescribeLootPet(p).c_str());
        return true;
    }

    static bool HandleLootPetOff(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg lootpet off <char>");
        if (!p)
            return false;
        Arpg::Loot::SetLootPet(p, false);
        handler->PSendSysMessage("ok %s", Arpg::Loot::DescribeLootPet(p).c_str());
        return true;
    }

    // .arpg lootpet model <char> <id>   the companion's look (arpg_companion_model), saved per character
    static bool HandleLootPetModel(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2)
            return Fail(handler, "usage: .arpg lootpet model <char> <id>");
        Player* p = OnlineChar(handler, t, "usage: .arpg lootpet model <char> <id>");
        if (!p)
            return false;
        std::string out;
        if (!Arpg::Loot::SetCompanionModel(p, uint32(atoi(t[1].c_str())), out))
            return Fail(handler, out);
        handler->PSendSysMessage("ok %s", out.c_str());
        return true;
    }

    static bool HandleLootPetInfo(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = OnlineChar(handler, t, "usage: .arpg lootpet info <char>");
        if (!p)
            return false;
        handler->PSendSysMessage("%s", Arpg::Loot::DescribeLootPet(p).c_str());
        return true;
    }

    // .arpg test debuff <char> <spellId>   the nearest hostile creature puts the aura on the character; reports its base and
    // final duration (CC / snare reduction affixes), then removes it
    static bool HandleTestDebuff(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2)
            return Fail(handler, "usage: .arpg test debuff <char> <spellId>");
        Player* p = OnlineChar(handler, t, "usage: .arpg test debuff <char> <spellId>");
        if (!p)
            return false;
        uint32 spellId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        SpellInfo const* si = sSpellMgr->GetSpellInfo(spellId);
        if (!si)
            return Fail(handler, "unknown spell " + t[1]);
        Creature* c = NearestHostile(p);
        if (!c)
            return Fail(handler, "no hostile creature within 30 yards");
        Aura* a = c->AddAura(spellId, p);
        if (!a)
            return Fail(handler, "aura not applied");
        int32 base = si->GetMaxDuration();
        int32 dur = a->GetMaxDuration();
        p->RemoveAurasDueToSpell(spellId);
        handler->PSendSysMessage("ok aura spell=%u base=%d duration=%d", spellId, base, dur);
        return true;
    }

    // .arpg test fillbags <char> <itemId>   fills every free bag slot with that (non-stacking) item (overflow tests)
    static bool HandleTestFillBags(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2)
            return Fail(handler, "usage: .arpg test fillbags <char> <itemId>");
        Player* p = OnlineChar(handler, t, "usage: .arpg test fillbags <char> <itemId>");
        if (!p)
            return false;
        uint32 itemId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        if (!sObjectMgr->GetItemTemplate(itemId))
            return Fail(handler, "unknown item " + t[1]);
        uint32 n = 0;
        for (; n < 200; ++n)
        {
            ItemPosCountVec dest;
            if (p->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, 1) != EQUIP_ERR_OK)
                break;
            if (!p->StoreNewItem(dest, itemId, true))
                break;
        }
        handler->PSendSysMessage("ok %s filled=%u", p->GetName().c_str(), n);
        return true;
    }

    static bool HandleTestClearBags(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 1 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test clearbags <char>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 n = 0;
        // Worn gear too: tests equip what they need, and leftovers (Uniques, unique-equipped rings) leak into later tests.
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            if (p->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                p->DestroyItem(INVENTORY_SLOT_BAG_0, slot, true);
                ++n;
            }
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
            if (p->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                p->DestroyItem(INVENTORY_SLOT_BAG_0, slot, true);
                ++n;
            }
        for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
            if (Bag* bag = p->GetBagByPos(bagSlot))
                for (uint8 slot = 0; slot < bag->GetBagSize(); ++slot)
                    if (bag->GetItemByPos(slot))
                    {
                        p->DestroyItem(bagSlot, slot, true);
                        ++n;
                    }
        handler->PSendSysMessage("ok %s destroyed=%u", p->GetName().c_str(), n);
        return true;
    }

    // .arpg test events <char> [clear]   (event recorder; needs Arpg.Test.RecordEvents = 1)
    static bool HandleTestEvents(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.empty() || t.size() > 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg test events <char> [clear]");
        QueryResult r = CharacterDatabase.PQuery("SELECT guid FROM characters WHERE name = '%s'", NormalizeName(t[0]).c_str());
        if (!r)
            return Fail(handler, "character not found");
        uint64 key = r->Fetch()[0].GetUInt64();
        if (t.size() == 2 && t[1] == "clear")
        {
            Arpg::TestEvents::Clear(key);
            handler->PSendSysMessage("ok events cleared");
            return true;
        }
        std::vector<std::string> lines = Arpg::TestEvents::Get(key, 30);
        handler->PSendSysMessage("events=%u recording=%s", uint32(lines.size()), Arpg::TestEvents::Enabled() ? "on" : "off");
        for (std::string const& l : lines)
            handler->PSendSysMessage("%s", l.c_str());
        return true;
    }

    // .arpg test pack <char> <entry> <n> [kill]   spawns n creatures around the character (max 40); "kill" kills them
    // credited to the character, so the real loot code runs for that character.
    static bool HandleTestPack(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        bool const withLoot = t.size() == 5 && t[3] == "kill" && t[4] == "loot";
        if (t.size() < 3 || t.size() > 5 || !IsNameLike(t[0]) || (t.size() == 4 && t[3] != "kill" && t[3] != "aggro") ||
            (t.size() == 5 && !withLoot))
            return Fail(handler, "usage: .arpg test pack <char> <entry> <n> [kill [loot]|aggro]");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 entry = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        int n = std::atoi(t[2].c_str());
        if (!sObjectMgr->GetCreatureTemplate(entry))
            return Fail(handler, "unknown creature entry " + t[1]);
        if (n < 1 || n > 40)
            return Fail(handler, "n must be 1-40");
        bool kill = t.size() >= 4 && t[3] == "kill";
        bool aggro = t.size() == 4 && t[3] == "aggro";   // the pack fights the character (neutral mobs too)

        int spawned = 0, killed = 0;
        for (int i = 0; i < n; ++i)
        {
            float angle = float(i) * (2.0f * float(M_PI) / float(n));
            Position pos = p->GetNearPosition(6.0f + float(i % 3) * 2.0f, angle);
            Creature* c = p->SummonCreature(entry, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(),
                TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, Milliseconds(300000));
            if (!c)
                continue;
            ++spawned;
            if (aggro && c->IsAIEnabled)
                c->AI()->AttackStart(p);
            if (kill)
            {
                c->SetLootRecipient(p);
                if (withLoot)   // "kill loot": counts as the player's damage, so the core fills the corpse's loot
                    c->LowerPlayerDamageReq(c->GetMaxHealth());
                p->Kill(c);
                ++killed;
            }
        }
        Arpg::TestEvents::Record(p->GetGUID().GetCounter(), "pack entry=" + std::to_string(entry) + " spawned=" +
            std::to_string(spawned) + " killed=" + std::to_string(killed));
        handler->PSendSysMessage("ok %s pack entry=%u spawned=%d killed=%d", p->GetName().c_str(), entry, spawned, killed);
        return true;
    }

    // .arpg scale probe <char> nearest|<spawnId>
    // .arpg scale combat <char> <spawnId> on|off   (m1 3.4) the bot lock without bots: "on" makes that creature fight a
    // summoned stand-in (a Young Wolf on a player faction, defensive, with a million health) that is not a real player, as when a bot pulls it;
    // "off" removes the stand-in so the creature leaves combat. A creature in such a fight must keep its level.
    static bool HandleScaleCombat(ChatHandler* handler, char const* args)
    {
        static std::map<uint32, ObjectGuid> standIns;   // spawnId -> stand-in
        std::vector<std::string> t = Tokens(args);
        static std::map<uint32, uint32> savedHealth;    // spawnId -> max health before "bot"
        if (t.size() != 3 || !IsNameLike(t[0]) || (t[2] != "on" && t[2] != "off" && t[2] != "status" && t[2] != "bot"))
            return Fail(handler, "usage: .arpg scale combat <char> <spawnId> on|bot|off|status");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        uint32 spawnId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
        float radius = sArpgConfig.Get().scaleRadius;
        Creature* c = nullptr;
        std::list<Unit*> units;
        Trinity::AnyUnitInObjectRangeCheck check(p, radius);
        Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(p, units, check);
        Cell::VisitAllObjects(p, searcher, radius);
        for (Unit* u : units)
            if (Creature* cr = u->ToCreature())
                if (cr->GetDBTableGUIDLow() == spawnId)
                    c = cr;
        if (!c || !c->IsAlive())
            return Fail(handler, "spawn " + t[1] + " not found alive near " + p->GetName());

        if (t[2] == "status")
        {
            Unit* v = c->GetVictim();
            Creature* d = nullptr;
            auto it = standIns.find(spawnId);
            if (it != standIns.end())
                d = ObjectAccessor::GetCreature(*p, it->second);
            Player* tb = TestBotOf(p);
            if (tb && c->GetThreatManager().IsThreatenedBy(tb))   // "bot" mode: the grouped random bot is on its threat list
            {
                handler->PSendSysMessage("ok combat status spawn=%u incombat=%u victim_player=%s real=%d level=%u alive=%u hp=%u", spawnId,
                    c->IsInCombat() ? 1u : 0u, tb->GetName().c_str(), Arpg::Scaling::IsRealPlayer(tb) ? 1 : 0, c->GetLevel(),
                    c->IsAlive() ? 1u : 0u, c->GetHealth());
                return true;
            }
            if (v && v->ToPlayer())
            {
                handler->PSendSysMessage("ok combat status spawn=%u incombat=%u victim_player=%s real=%d level=%u alive=%u hp=%u", spawnId,
                    c->IsInCombat() ? 1u : 0u, v->GetName().c_str(), Arpg::Scaling::IsRealPlayer(v->ToPlayer()) ? 1 : 0, c->GetLevel(),
                    c->IsAlive() ? 1u : 0u, c->GetHealth());
                return true;
            }
            handler->PSendSysMessage("ok combat status spawn=%u incombat=%u victim=%u evading=%u standin=%u alive=%u hp=%u faction=%u "
                "hostile=%u valid=%u standin_combat=%u", spawnId, c->IsInCombat() ? 1u : 0u,
                v && v->ToCreature() ? v->ToCreature()->GetEntry() : 0u, c->IsInEvadeMode() ? 1u : 0u, d ? 1u : 0u,
                d && d->IsAlive() ? 1u : 0u, d ? d->GetHealth() : 0u, d ? d->GetFaction() : 0u, d && c->IsHostileTo(d) ? 1u : 0u,
                d && c->IsValidAttackTarget(d) ? 1u : 0u, d && d->IsInCombat() ? 1u : 0u);
            return true;
        }
        if (t[2] == "off")
        {
            auto it = standIns.find(spawnId);
            if (it != standIns.end())
            {
                if (Creature* d = ObjectAccessor::GetCreature(*p, it->second))
                    d->DespawnOrUnsummon();
                standIns.erase(it);
            }
            c->RestoreFaction();
            c->CombatStop(true);
            if (auto h = savedHealth.find(spawnId); h != savedHealth.end())
            {
                c->SetMaxHealth(h->second);
                c->SetFullHealth();
                savedHealth.erase(h);
            }
            if (Player* b = TestBotOf(p))
            {
                b->CombatStop(true);
                b->UpdateMaxHealth();
                b->SetFullHealth();
                // Away from the mob: left next to it, the bot attacks it again and kills it at its normal health.
                b->TeleportTo(b->m_homebindMapId, b->m_homebindX, b->m_homebindY, b->m_homebindZ, 0.0f);
            }
            handler->PSendSysMessage("ok combat off spawn=%u", spawnId);
            return true;
        }

        if (t[2] == "bot")   // m7 2.1: the character's grouped random bot fights it (both made unkillable for the test)
        {
            Player* b = TestBotOf(p);
            if (!b || !b->IsInWorld() || b->GetMapId() != c->GetMapId())
                return Fail(handler, "no grouped bot on this map (.arpg test bot <char> join, then bring)");
            Position pos = c->GetNearPosition(2.0f, 0.0f);
            b->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), c->GetOrientation());
            b->SetMaxHealth(1000000);
            b->SetFullHealth();
            savedHealth[spawnId] = c->GetMaxHealth();
            c->SetMaxHealth(200000000);   // a level-90 bot must not kill it during the test
            c->SetFullHealth();
            c->SetFaction(14);
            c->GetThreatManager().AddThreat(b, 1000.0f, nullptr, true, true);
            if (c->AI())
                c->AI()->AttackStart(b);
            handler->PSendSysMessage("ok combat bot spawn=%u bot=%s incombat=%u", spawnId, b->GetName().c_str(), c->IsInCombat() ? 1u : 0u);
            return true;
        }

        uint32 const youngWolf = 299;   // a plain beast: a valid target (a critter is not, the fight dropped at once)
        // Summoned by the map, not by the creature: a creature's own summon counts as its side.
        Creature* d = c->GetMap()->SummonCreature(youngWolf, c->GetNearPosition(3.0f, 0.0f), nullptr, Milliseconds(120000));
        if (!d)
            return Fail(handler, "stand-in not summoned");
        d->SetFaction(1);                      // a player faction (Human): world monsters treat it as an enemy, like a bot
        d->SetReactState(REACT_DEFENSIVE);     // hits back only at what hits it
        d->SetMaxHealth(1000000);
        d->SetFullHealth();
        standIns[spawnId] = d->GetGUID();
        // Neutral world mobs (faction 7) don't fight NPCs of any side; for the test the creature turns Monster (14, hostile to
        // players and so to the stand-in) until "off".
        c->SetFaction(14);
        // Threat first: AttackStart alone leaves the threat list empty until a hit lands, and UpdateVictim then evades at once.
        c->GetThreatManager().AddThreat(d, 1000.0f, nullptr, true, true);
        if (c->AI())
            c->AI()->AttackStart(d);
        handler->PSendSysMessage("ok combat on spawn=%u incombat=%u", spawnId, c->IsInCombat() ? 1u : 0u);
        return true;
    }

    static bool HandleScaleProbe(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        if (t.size() != 2 || !IsNameLike(t[0]))
            return Fail(handler, "usage: .arpg scale probe <char> nearest|scaled|tiered|<spawnId>");
        Player* p = ObjectAccessor::FindPlayerByName(NormalizeName(t[0]));
        if (!p || !p->IsInWorld())
            return Fail(handler, "character not online");
        float radius = sArpgConfig.Get().scaleRadius;

        Creature* c = nullptr;
        if (t[1] == "nearest")
            c = Arpg::Scaling::FindNearestCandidate(p, radius);
        else if (t[1] == "scaled" || t[1] == "tiered")
            c = Arpg::Scaling::FindNearestScaled(p, radius, t[1] == "tiered");
        else
        {
            uint32 spawnId = uint32(std::strtoul(t[1].c_str(), nullptr, 10));
            std::list<Unit*> units;
            Trinity::AnyUnitInObjectRangeCheck check(p, radius);
            Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(p, units, check);
            Cell::VisitAllObjects(p, searcher, radius);
            for (Unit* u : units)
                if (Creature* cr = u->ToCreature())
                    if (cr->GetDBTableGUIDLow() == spawnId)
                        c = cr;
        }
        if (!c)
        {
            // Nothing scalable nearby is a valid answer ("nothing is scaled here"), not an error.
            handler->PSendSysMessage("no candidate creature within %d yd of %s scaled=none", int(radius), p->GetName().c_str());
            return true;
        }
        handler->PSendSysMessage("%s", Arpg::Scaling::Describe(c).c_str());
        return true;
    }
};

void AddSC_arpg_commandscript()
{
    new arpg_commandscript();
}
