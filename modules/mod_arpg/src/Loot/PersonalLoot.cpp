#include "Loot/PersonalLoot.h"
#include "ArpgConfig.h"
#include "Loot/AffixRoller.h"
#include "Monsters/Champions.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Group.h"
#include "Item.h"
#include "LootMgr.h"
#include "Mail.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include <map>
#include <mutex>
#include <string>
#include <utility>

namespace
{
    std::mutex g_lock;
    // (member guid, corpse raw guid) -> extra items added by the other kill scripts for that member
    std::map<std::pair<uint32, uint64>, std::vector<uint32>> g_extras;

    // Stores one item for `p` (bags, else returned for the mail) and applies its planned affixes like a pickup would.
    Item* MakeItem(Player* p, LootItem const& li, uint32 sourceEntry, bool& stored)
    {
        uint32 count = std::max<uint32>(1, li.count);
        Item* item = nullptr;
        ItemPosCountVec dest;
        stored = p->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, li.itemid, count) == EQUIP_ERR_OK;
        if (stored)
            item = p->StoreNewItem(dest, li.itemid, true, li.randomPropertyId);
        else if ((item = Item::CreateItem(li.itemid, count, p)) && li.randomPropertyId)
            item->SetItemRandomProperties(li.randomPropertyId);
        if (!item)
            return nullptr;
        Arpg::Loot::ApplyPickupRoll(p, item, ItemPickupSourceType::CreatureLoot, sourceEntry);
        if (stored)
            p->SendNewItem(item, count, false, false, true);   // "You receive loot: [item]" with its final name
        return item;
    }

    void Deliver(Player* m, Creature* killed, std::vector<uint32> const& extras)
    {
        // The member's own roll of the creature's loot table (our LootFilled hook makes it level-appropriate for them).
        // A creature in "not normal" loot mode would add rolls for every looter: then only the plain table, no source.
        ::Loot loot;
        Object* source = killed->HasNormalLootMode() ? static_cast<Object*>(killed) : nullptr;
        if (uint32 lootId = killed->GetCreatureTemplate()->lootid)
            loot.FillLoot(source, lootId, LootTemplates_Creature, m, true, true, killed->GetMap()->GetDifficulty());
        for (uint32 id : extras)
            Arpg::Monsters::AddLootItem(loot, id);
        Arpg::Loot::PlanLoot(m, loot, ItemPickupSourceType::CreatureLoot, killed->GetEntry(), Arpg::Monsters::WasChampionKill(killed));

        uint32 stored = 0;
        std::vector<Item*> mail;
        std::string ids;
        for (LootItem const& li : loot.items)
        {
            if (li.currency || li.needs_quest || li.freeforall || li.is_blocked || !li.itemid)
                continue;   // quest / free-for-all items are already in the corpse for everyone
            bool inBags = false;
            if (Item* it = MakeItem(m, li, killed->GetEntry(), inBags))
            {
                if (inBags)
                    ++stored;
                else
                    mail.push_back(it);
                ids += (ids.empty() ? "" : ",") + std::to_string(li.itemid);
            }
        }
        for (size_t i = 0; i < mail.size(); i += MAX_MAIL_ITEMS)
        {
            MailDraft draft("Personal loot", "Your bags were full, so your share of the loot was sent here.");
            for (size_t j = i; j < mail.size() && j < i + MAX_MAIL_ITEMS; ++j)
                draft.AddItem(mail[j]);
            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            draft.SendMailTo(trans, MailReceiver(m), MailSender(MAIL_NORMAL, 0), MAIL_CHECK_MASK_COPIED);
            CharacterDatabase.CommitTransaction(trans);
        }
        Arpg::TestEvents::Record(m->GetGUID().GetCounter(), "personal loot from=" + std::to_string(killed->GetEntry()) +
            " items=" + std::to_string(stored + mail.size()) + " mailed=" + std::to_string(mail.size()) + " ids=" + (ids.empty() ? "-" : ids));
    }

    int RealMembers(Group* g)
    {
        int n = 0;
        for (GroupReference* ref = g->GetFirstMember(); ref; ref = ref->next())
            if (Player* m = ref->GetSource())
                if (m->IsInWorld() && Arpg::Scaling::IsRealPlayer(m))
                    ++n;
        return n;
    }

    // Several real players: round robin, so nobody rolls on (or takes) another's corpse loot - each has their own.
    void EnforceRoundRobin(Group* g)
    {
        if (!g || !sArpgConfig.Get().personalLoot || RealMembers(g) < 2 || g->GetLootMethod() == ROUND_ROBIN)
            return;
        g->SetLootMethod(ROUND_ROBIN);
        g->SendUpdate();
    }

    class ArpgPersonalLootGroupScript : public GroupScript
    {
    public:
        ArpgPersonalLootGroupScript() : GroupScript("arpg_personal_loot_group") { }

        void OnAddMember(Group* group, ObjectGuid /*guid*/) override
        {
            EnforceRoundRobin(group);
        }
    };

    // Last kill script: every recipient's own loot (after Champions/Superuniques/pity scripts added their extras).
    class ArpgPersonalLootScript : public PlayerScript
    {
    public:
        ArpgPersonalLootScript() : PlayerScript("arpg_personal_loot") { }

        void OnCreatureKill(Player* killer, Creature* killed) override
        {
            if (!killer || !killed)
                return;
            uint64 corpse = killed->GetGUID().GetRawValue();
            std::vector<Player*> recipients = Arpg::Loot::PersonalRecipients(killer, killed);
            if (!recipients.empty())
            {
                // The corpse is the killer's alone (round robin owner); the others get theirs below.
                EnforceRoundRobin(killer->GetGroup());
                killed->loot.roundRobinPlayer = killer->GetGUID();
            }
            for (Player* m : recipients)
            {
                std::vector<uint32> extras;
                {
                    std::lock_guard<std::mutex> l(g_lock);
                    auto it = g_extras.find({ m->GetGUID().GetCounter(), corpse });
                    if (it != g_extras.end())
                    {
                        extras = std::move(it->second);
                        g_extras.erase(it);
                    }
                }
                Deliver(m, killed, extras);
            }
            // Extras for players that turned out not to qualify (left the group mid-kill): drop them.
            std::lock_guard<std::mutex> l(g_lock);
            for (auto it = g_extras.begin(); it != g_extras.end();)
                it = it->first.second == corpse ? g_extras.erase(it) : std::next(it);
        }
    };
}

namespace Arpg::Loot
{
    std::vector<Player*> PersonalRecipients(Player* killer, Creature* killed)
    {
        std::vector<Player*> out;
        Arpg::Settings const& s = sArpgConfig.Get();
        if (!s.personalLoot || !s.lootEnabled || !killer || !killed || !Arpg::Scaling::IsRealPlayer(killer))
            return out;
        Group* g = killer->GetGroup();
        if (!g)
            return out;
        for (GroupReference* ref = g->GetFirstMember(); ref; ref = ref->next())
        {
            Player* m = ref->GetSource();
            if (!m || m == killer || !m->IsInWorld() || !Arpg::Scaling::IsRealPlayer(m) || m->GetMap() != killed->GetMap())
                continue;
            if (!m->IsWithinDist(killed, s.personalLootRange))
                continue;
            out.push_back(m);
        }
        return out;
    }

    void AddPersonalItem(Player* member, Creature* killed, uint32 itemId)
    {
        if (!member || !killed || !itemId)
            return;
        std::lock_guard<std::mutex> l(g_lock);
        g_extras[{ member->GetGUID().GetCounter(), killed->GetGUID().GetRawValue() }].push_back(itemId);
    }

    void InstallPersonalLoot()
    {
        new ArpgPersonalLootScript();
        new ArpgPersonalLootGroupScript();
    }
}
