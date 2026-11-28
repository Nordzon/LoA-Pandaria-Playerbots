// .arpg charm / shrine / goblin / rift (GM, SOAP: explicit character name) and the player's .rift (changes m9-*).
#include "Commands/M9Commands.h"

#include "ArpgConfig.h"
#include "Charms/Charms.h"
#include "Dungeons/Rifts.h"
#include "Loot/GiveItem.h"
#include "Monsters/TreasureGoblin.h"
#include "World/Shrines.h"

#include "Bag.h"
#include "Creature.h"
#include "GameObject.h"
#include "Item.h"
#include "LootMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>

namespace
{
    using Trinity::ChatCommands::Console;

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

    bool Fail(ChatHandler* handler, std::string const& msg)
    {
        handler->PSendSysMessage("error: %s", msg.c_str());
        handler->SetSentErrorMessage(true);
        return false;
    }

    Player* Online(ChatHandler* handler, std::vector<std::string> const& t, char const* usage)
    {
        if (t.empty() || t[0].size() > 12)
        {
            Fail(handler, usage);
            return nullptr;
        }
        std::string n = t[0];
        for (size_t i = 0; i < n.size(); ++i)
            n[i] = char(i == 0 ? std::toupper(static_cast<unsigned char>(n[i])) : std::tolower(static_cast<unsigned char>(n[i])));
        Player* p = ObjectAccessor::FindPlayerByName(n);
        if (!p || !p->IsInWorld())
        {
            Fail(handler, "character not online");
            return nullptr;
        }
        return p;
    }

    uint32 Num(std::string const& s) { return uint32(std::strtoul(s.c_str(), nullptr, 10)); }

    uint32 g_lastCharm = 0;   // tests: the last charm given (low guid)

    // ---- charms ----
    // .arpg charm give <char> <small|large|grand|heart|coin|sigil|entry>
    bool HandleCharmGive(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg charm give <char> <small|large|grand|heart|coin|sigil|entry>");
        if (!p)
            return false;
        if (t.size() != 2)
            return Fail(handler, "usage: .arpg charm give <char> <small|large|grand|heart|coin|sigil|entry>");
        std::string const& w = t[1];
        uint32 entry = w == "small" ? Arpg::Charms::SmallCharm : w == "large" ? Arpg::Charms::LargeCharm :
            w == "grand" ? Arpg::Charms::GrandCharm : w == "heart" ? Arpg::Charms::HeartOfThePandaren :
            w == "coin" ? Arpg::Charms::GoblinsLuckyCoin : w == "sigil" ? Arpg::Charms::RiftwalkersSigil :
            w == "random" ? Arpg::Charms::RandomCharmEntry() : Num(w);
        if (!Arpg::Charms::IsCharm(entry))
            return Fail(handler, "not a charm: " + w);
        bool mailed = false;
        Item* it = Arpg::Loot::GiveItem(p, entry, 1, 0, "Charm", mailed);
        if (!it)
            return Fail(handler, "could not create (unique charm already owned?)");
        g_lastCharm = it->GetGUID().GetCounter();
        handler->PSendSysMessage("ok %s charm item=%u guid=%u mailed=%d %s", p->GetName().c_str(), entry, g_lastCharm, mailed ? 1 : 0,
            Arpg::Charms::DescribeItem(g_lastCharm).c_str());
        return true;
    }

    // .arpg charm info <char> [guid|last]
    bool HandleCharmInfo(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg charm info <char> [guid|last]");
        if (!p)
            return false;
        Arpg::Charms::Rescan(p);
        if (t.size() == 2)
        {
            uint32 g = t[1] == "last" ? g_lastCharm : Num(t[1]);
            handler->PSendSysMessage("ok charm guid=%u %s", g, Arpg::Charms::DescribeItem(g).c_str());
            return true;
        }
        handler->PSendSysMessage("ok %s", Arpg::Charms::Describe(p).c_str());
        return true;
    }

    // .arpg charm pouch <char> [equip|remove]   grant check; equip = into a free bag slot; remove = destroy the pouch (tests)
    bool HandleCharmPouch(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg charm pouch <char> [equip|remove]");
        if (!p)
            return false;
        if (t.size() == 2 && t[1] == "equip")
        {
            std::string why;
            if (!Arpg::Charms::EquipPouch(p, why))
                return Fail(handler, why);
            handler->PSendSysMessage("ok pouch %s", why.c_str());
            return true;
        }
        if (t.size() == 2 && t[1] == "remove")
        {
            uint32 n = 0;
            for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)   // bag slots + backpack
                if (Item* it = p->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    if (it->GetEntry() == Arpg::Charms::PouchEntry)
                    {
                        p->DestroyItem(INVENTORY_SLOT_BAG_0, slot, true);
                        ++n;
                    }
            for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
                if (Bag* bag = p->GetBagByPos(bagSlot))
                    for (uint32 i = 0; i < bag->GetBagSize(); ++i)
                        if (Item* it = bag->GetItemByPos(uint8(i)))
                            if (it->GetEntry() == Arpg::Charms::PouchEntry)
                            {
                                p->DestroyItem(bagSlot, uint8(i), true);
                                ++n;
                            }
            handler->PSendSysMessage("ok pouch removed=%u", n);
            return true;
        }
        std::string r = Arpg::Charms::EnsurePouch(p);
        handler->PSendSysMessage("ok pouch %s count=%u", r.c_str(), p->HasItemCount(Arpg::Charms::PouchEntry, 1, true) ? 1u : 0u);
        return true;
    }

    // .arpg charm move <char> <guid|last> <in|out>
    bool HandleCharmMove(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg charm move <char> <guid|last> <in|out>");
        if (!p)
            return false;
        if (t.size() != 3 || (t[2] != "in" && t[2] != "out"))
            return Fail(handler, "usage: .arpg charm move <char> <guid|last> <in|out>");
        uint32 g = t[1] == "last" ? g_lastCharm : Num(t[1]);
        std::string why;
        if (!Arpg::Charms::MoveCharm(p, g, t[2] == "in", why))
            return Fail(handler, why);
        handler->PSendSysMessage("ok moved %s | %s", why.c_str(), Arpg::Charms::Describe(p).c_str());
        return true;
    }

    // ---- shrines ----
    // .arpg shrine spawn <char> [id]
    bool HandleShrineSpawn(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg shrine spawn <char> [id]");
        if (!p)
            return false;
        std::string why;
        if (!Arpg::World::SpawnShrineNear(p, t.size() > 1 ? Num(t[1]) : 0, why))
            return Fail(handler, why);
        handler->PSendSysMessage("ok %s %s", p->GetName().c_str(), why.c_str());
        return true;
    }

    // .arpg shrine use <char>   the nearest shrine within 40 yd (same code as clicking it)
    bool HandleShrineUse(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg shrine use <char>");
        if (!p)
            return false;
        GameObject* go = Arpg::World::NearestShrine(p, 40.0f);
        if (!go)
            return Fail(handler, "no shrine within 40 yd");
        std::string out;
        if (!Arpg::World::UseShrine(p, go, out))
            return Fail(handler, out);
        handler->PSendSysMessage("ok %s %s", p->GetName().c_str(), out.c_str());
        return true;
    }

    // .arpg shrine info <char>
    bool HandleShrineInfo(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg shrine info <char>");
        if (!p)
            return false;
        handler->PSendSysMessage("ok %s", Arpg::World::Describe(p).c_str());
        return true;
    }

    // .arpg shrine clear <char>
    bool HandleShrineClear(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg shrine clear <char>");
        if (!p)
            return false;
        handler->PSendSysMessage("ok shrine buffs removed=%d", Arpg::World::ClearBuffs(p));
        return true;
    }

    // ---- goblins ----
    bool HandleGoblinSpawn(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg goblin spawn <char>");
        if (!p)
            return false;
        std::string why;
        if (!Arpg::Monsters::SpawnGoblin(p, why))
            return Fail(handler, why);
        handler->PSendSysMessage("ok %s %s", p->GetName().c_str(), why.c_str());
        return true;
    }

    // .arpg goblin hit <char>   the character deals 5% of the goblin's health (starts its escape timer)
    bool HandleGoblinHit(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg goblin hit <char>");
        if (!p)
            return false;
        Creature* c = Arpg::Monsters::LastGoblinOf(p);
        if (!c || !c->IsAlive())
            return Fail(handler, "no live goblin");
        p->DealDamage(c, std::max<uint32>(1, c->GetMaxHealth() / 20));
        handler->PSendSysMessage("ok %s", Arpg::Monsters::DescribeGoblin(p).c_str());
        return true;
    }

    bool HandleGoblinKill(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg goblin kill <char>");
        if (!p)
            return false;
        Creature* c = Arpg::Monsters::LastGoblinOf(p);
        if (!c || !c->IsAlive())
            return Fail(handler, "no live goblin");
        c->SetLootRecipient(p);
        p->Kill(c);
        uint32 items = 0;
        for (LootItem const& li : c->loot.items)
            if (!li.is_looted)
                ++items;
        handler->PSendSysMessage("ok %s killed goblin loot_items=%u gold=%u", p->GetName().c_str(), items, c->loot.gold);
        return true;
    }

    bool HandleGoblinLoot(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg goblin loot <char>");
        if (!p)
            return false;
        Creature* c = Arpg::Monsters::LastGoblinOf(p);
        if (!c || c->IsAlive())
            return Fail(handler, "no dead goblin");
        p->SetLootGUID(c->GetGUID());
        uint32 n = 0;
        for (uint8 i = 0; i < uint8(c->loot.items.size()); ++i)
            if (!c->loot.items[i].is_looted && !c->loot.items[i].currency)
            {
                p->StoreLootItem(i, &c->loot, c);
                ++n;
            }
        uint32 gold = c->loot.gold;
        if (gold)
        {
            p->ModifyMoney(gold);
            c->loot.gold = 0;
        }
        p->SetLootGUID(ObjectGuid::Empty);
        handler->PSendSysMessage("ok %s looted=%u gold=%u", p->GetName().c_str(), n, gold);
        return true;
    }

    bool HandleGoblinInfo(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg goblin info <char>");
        if (!p)
            return false;
        handler->PSendSysMessage("ok %s", Arpg::Monsters::DescribeGoblin(p).c_str());
        return true;
    }

    // ---- rifts ----
    bool HandleRiftOpen(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg rift open <char> <level>");
        if (!p)
            return false;
        if (t.size() != 2)
            return Fail(handler, "usage: .arpg rift open <char> <level>");
        std::string out;
        if (!Arpg::Dungeons::OpenRift(p, int(Num(t[1])), out))
            return Fail(handler, out);
        handler->PSendSysMessage("ok %s %s", p->GetName().c_str(), out.c_str());
        return true;
    }

    bool HandleRiftClose(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg rift close <char>");
        if (!p)
            return false;
        handler->PSendSysMessage("ok %s", Arpg::Dungeons::CloseRift(p).c_str());
        return true;
    }

    bool HandleRiftInfo(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg rift info <char>");
        if (!p)
            return false;
        handler->PSendSysMessage("ok %s", Arpg::Dungeons::DescribeRift(p).c_str());
        return true;
    }

    // .arpg rift finish <char> <elapsedSeconds>   as if the final boss died now after that many seconds
    bool HandleRiftFinish(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg rift finish <char> <seconds>");
        if (!p)
            return false;
        if (t.size() != 2)
            return Fail(handler, "usage: .arpg rift finish <char> <seconds>");
        std::string out;
        if (!Arpg::Dungeons::FinishRift(p, int(Num(t[1])), out))
            return Fail(handler, out);
        handler->PSendSysMessage("ok %s", out.c_str());
        return true;
    }

    bool HandleRiftKey(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg rift key <char> <level>");
        if (!p)
            return false;
        if (t.size() != 2)
            return Fail(handler, "usage: .arpg rift key <char> <level>");
        Arpg::Dungeons::SetKeyLevel(p, int(Num(t[1])));
        handler->PSendSysMessage("ok %s", Arpg::Dungeons::DescribeRift(p).c_str());
        return true;
    }

    bool HandleRiftDeath(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> t = Tokens(args);
        Player* p = Online(handler, t, "usage: .arpg rift death <char>");
        if (!p)
            return false;
        std::string out;
        if (!Arpg::Dungeons::AddRiftDeath(p, out))
            return Fail(handler, out);
        handler->PSendSysMessage("ok %s", out.c_str());
        return true;
    }

    // ---- player: .rift info | open <n> | close | data ----
    bool HandleMyRiftInfo(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!p)
            return false;
        handler->PSendSysMessage("%s (levels you can open: %s)", Arpg::Dungeons::DescribeRift(p).c_str(), Arpg::Dungeons::OfferedLevels(p).c_str());
        return true;
    }

    bool HandleMyRiftOpen(ChatHandler* handler, char const* args)
    {
        Player* p = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!p)
            return false;
        std::vector<std::string> t = Tokens(args);
        int level = t.empty() ? Arpg::Dungeons::KeyLevel(p) : int(Num(t[0]));
        std::string out;
        if (!Arpg::Dungeons::OpenRift(p, level, out))
            return Fail(handler, out);
        return true;
    }

    bool HandleMyRiftClose(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!p)
            return false;
        handler->PSendSysMessage("%s", Arpg::Dungeons::CloseRift(p).c_str());
        return true;
    }

    bool HandleMyRiftData(ChatHandler* handler, char const* /*args*/)
    {
        Player* p = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!p)
            return false;
        handler->PSendSysMessage("%s", Arpg::Dungeons::DataLine(p).c_str());
        return true;
    }

    class arpg_m9_commandscript : public CommandScript
    {
    public:
        arpg_m9_commandscript() : CommandScript("arpg_m9_commandscript") { }

        std::vector<ChatCommand> GetCommands() const override
        {
            static std::vector<ChatCommand> riftTable =
            {
                { "info",  &HandleMyRiftInfo,  rbac::RBAC_PERM_COMMAND_HELP, Console::No },
                { "open",  &HandleMyRiftOpen,  rbac::RBAC_PERM_COMMAND_HELP, Console::No },
                { "close", &HandleMyRiftClose, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
                { "data",  &HandleMyRiftData,  rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            };
            static std::vector<ChatCommand> commandTable =
            {
                { "rift", riftTable, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
            };
            return commandTable;
        }
    };
}

namespace Arpg::Commands
{
    std::vector<ChatCommand> const& CharmTable()
    {
        static std::vector<ChatCommand> t =
        {
            { "give",  &HandleCharmGive,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "info",  &HandleCharmInfo,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "pouch", &HandleCharmPouch, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "move",  &HandleCharmMove,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        return t;
    }

    std::vector<ChatCommand> const& ShrineTable()
    {
        static std::vector<ChatCommand> t =
        {
            { "spawn", &HandleShrineSpawn, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "use",   &HandleShrineUse,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "info",  &HandleShrineInfo,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "clear", &HandleShrineClear, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        return t;
    }

    std::vector<ChatCommand> const& GoblinTable()
    {
        static std::vector<ChatCommand> t =
        {
            { "spawn", &HandleGoblinSpawn, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "hit",   &HandleGoblinHit,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "kill",  &HandleGoblinKill,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "loot",  &HandleGoblinLoot,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "info",  &HandleGoblinInfo,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        return t;
    }

    std::vector<ChatCommand> const& RiftTable()
    {
        static std::vector<ChatCommand> t =
        {
            { "open",   &HandleRiftOpen,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "close",  &HandleRiftClose,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "info",   &HandleRiftInfo,   rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "finish", &HandleRiftFinish, rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "key",    &HandleRiftKey,    rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
            { "death",  &HandleRiftDeath,  rbac::RBAC_PERM_COMMAND_DEBUG, Console::Yes },
        };
        return t;
    }
}

void AddSC_arpg_m9_commandscript()
{
    new arpg_m9_commandscript();
}
