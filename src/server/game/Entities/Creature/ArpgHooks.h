/*
 * ARPG: hook points for modules/mod_arpg (Pandaria ARPG fork). Listed in docs/CODEMAP.md "Upstream touches".
 * Each hook is a plain function pointer, null unless mod_arpg sets it, so with the module absent the core
 * behaves exactly as upstream.
 */
#ifndef TRINITY_ARPG_HOOKS_H
#define TRINITY_ARPG_HOOKS_H

#include "Define.h"

class Creature;
class Item;
class Player;
struct Loot;
class Map;
struct TrainerSpellData;
enum class ItemPickupSourceType : uint8;

namespace ArpgHooks
{
    // ScriptMgr::OnItemPickup, before the item's own ItemScript: a player received a new item (loot, quest, craft...).
    using ItemPickupFn = void (*)(Player* player, Item* item, ItemPickupSourceType sourceType, uint32 sourceId);
    TC_GAME_API extern ItemPickupFn ItemPickup;

    // Creature::SelectLevel, after the level roll (and AutoBalance). Return 0 to keep `level`, or the level to use
    // instead; health/damage/stats are then computed for that level by SelectLevel itself.
    using CreatureLevelFn = uint8 (*)(Creature* creature, uint8 level);
    TC_GAME_API extern CreatureLevelFn CreatureLevel;

    // Player::_ApplyItemBonuses (not the level-scale-only pass): an equipped item's stats are applied or removed
    // (equip, unequip, login, break/repair, item-level rescale). mod_arpg adds/removes the item's affix stats.
    using ItemBonusesFn = void (*)(Player* player, Item const* item, uint8 slot, bool apply);
    TC_GAME_API extern ItemBonusesFn ItemBonuses;

    // WorldSession::HandleLootMoneyOpcode, after AutoBalance and before the money is handed out: mod_arpg may raise the
    // looted gold (gold find affixes). `gold` is the loot's whole money (shared later if the group splits it).
    using LootMoneyFn = void (*)(Player* player, uint32& gold);
    TC_GAME_API extern LootMoneyFn LootMoney;

    // Loot::FillLoot, after the loot template (and world drops) filled `loot->items`, before group rights are set:
    // mod_arpg may swap item ids (level-appropriate loot with a zone ceiling).
    using LootFilledFn = void (*)(Loot* loot, Player* lootOwner);
    TC_GAME_API extern LootFilledFn LootFilled;

    // Vendor price of an item (WorldSession::HandleSellItemOpcode payout AND Player::AddItemToBuyBackSlot buyback price, so
    // the two always agree - no sell/buy-back gold loop). Returns the price to use; `basePrice` = template price * count.
    using ItemSellPriceFn = uint64 (*)(Item const* item, uint64 basePrice);
    TC_GAME_API extern ItemSellPriceFn ItemSellPrice;

    // AutoBalance (Maps/AutoBalanceLevelScaling.cpp ComputeCreatureMultipliers): a creature's player-count health / damage
    // multipliers before AutoBalance's floors. mod_arpg may replace them (party-size table x raid tier x solo role, m5).
    // playerCount = AutoBalance's adjusted count (bots included).
    using InstanceScaleFn = void (*)(Map* map, uint32 maxPlayers, uint32 playerCount, bool boss, float& health, float& damage);
    TC_GAME_API extern InstanceScaleFn InstanceScale;

    // AutoBalance RecountPlayers: the player level the instance scales its creatures to (the highest player level).
    // mod_arpg may lower it (dungeon level selector, m5).
    using InstanceLevelFn = void (*)(Map* map, uint8& level);
    TC_GAME_API extern InstanceLevelFn InstanceLevel;
    // AuctionHouseBotSeller::AddNewAuctions, after the seller created and priced a new listing: mod_arpg may roll affixes
    // on it and raise its prices (living auction house, m7).
    using AuctionBotItemFn = void (*)(Item* item, uint32& buyout, uint32& bid);
    TC_GAME_API extern AuctionBotItemFn AuctionBotItem;
    // Spell::DoCreateItem, before the item is made: mod_arpg may swap a crafted piece of gear for its typed blank base.
    using CraftedItemFn = uint32 (*)(Player* player, uint32 spellId, uint32 itemId);
    TC_GAME_API extern CraftedItemFn CraftedItem;
    // Player::SatisfyQuestClass, when the quest's class mask doesn't include the player's class: mod_arpg may still allow
    // it (new race/class pairs borrow their starting zone's Warrior quests, m6-start-quests). Return true to allow.
    using QuestClassAllowedFn = bool (*)(Player const* player, uint32 questId);
    TC_GAME_API extern QuestClassAllowedFn QuestClassAllowed;
    // Player::SetSkill: every skill add / change / removal (old value 0 = learned, new value 0 = removed). mod_arpg logs
    // profession changes (m9-stacks-trainer, owner 2026-11-28: professions went missing).
    using SkillChangedFn = void (*)(Player* player, uint32 skill, uint32 oldValue, uint32 newValue);
    TC_GAME_API extern SkillChangedFn SkillChanged;
    // CMSG_UNLEARN_SKILL (the client's "abandon profession"): return false to refuse it.
    using UnlearnSkillAllowedFn = bool (*)(Player* player, uint32 skill);
    TC_GAME_API extern UnlearnSkillAllowedFn UnlearnSkillAllowed;
    // SendTrainerList / HandleTrainerBuySpellOpcode: the trainer spells a creature offers this player (null = its own). mod_arpg
    // serves profession trainer lists from the companion (m10-companion-professions; npc_trainer ignores entries >= 200000).
    using TrainerSpellsFn = TrainerSpellData const* (*)(Player* player, Creature* trainer);
    TC_GAME_API extern TrainerSpellsFn TrainerSpells;
}

#endif
