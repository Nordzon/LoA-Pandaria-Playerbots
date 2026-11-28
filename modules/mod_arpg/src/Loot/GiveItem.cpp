#include "Loot/GiveItem.h"

#include "Loot/AffixRoller.h"

#include "DatabaseEnv.h"
#include "Item.h"
#include "Mail.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace Arpg::Loot
{
    Item* GiveItem(Player* player, uint32 itemId, uint32 count, uint32 sourceEntry, std::string const& mailSubject,
                   bool& mailed, bool roll)
    {
        mailed = false;
        if (!player || !itemId || !sObjectMgr->GetItemTemplate(itemId))
            return nullptr;
        count = std::max<uint32>(1, count);
        ItemPosCountVec dest;
        Item* item = nullptr;
        InventoryResult const r = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count);
        if (r == EQUIP_ERR_OK)
            item = player->StoreNewItem(dest, itemId, true);
        else if (r == EQUIP_ERR_ITEM_MAX_COUNT || r == EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS)
            return nullptr;   // unique (one per character): never by mail either
        else
        {
            item = Item::CreateItem(itemId, count, player);
            mailed = item != nullptr;
        }
        if (!item)
            return nullptr;
        if (roll)
            ApplyPickupRoll(player, item, ItemPickupSourceType::CreatureLoot, sourceEntry);
        if (mailed)
        {
            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            MailDraft(mailSubject, "Your bags were full, so it was sent here.").AddItem(item)
                .SendMailTo(trans, MailReceiver(player), MailSender(MAIL_NORMAL, 0), MAIL_CHECK_MASK_COPIED);
            CharacterDatabase.CommitTransaction(trans);
        }
        else
            player->SendNewItem(item, count, true, false, false);
        return item;
    }
}
