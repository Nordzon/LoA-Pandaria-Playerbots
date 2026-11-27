// Rolls affixes onto a new item for a real player (m2 task 4.1c, design.md). Pool from arpg_affix / arpg_affix_tier.
#ifndef ARPG_LOOT_AFFIXROLLER_H
#define ARPG_LOOT_AFFIXROLLER_H

#include "Define.h"

#include <string>
#include <vector>
#include "Core/AffixRoll.h"

class Item;
class Player;
struct ItemTemplate;
struct Loot;
enum class ItemPickupSourceType : uint8;

namespace Arpg::Core { struct RolledAffix; }

namespace Arpg::Loot
{
    // One flat stat affix on an item: core stat id (ITEM_MOD_*), value, and whether a native enchant slot shows it
    // (native lines are applied by the core's own enchantment code, the others by AffixStats).
    struct StatLine
    {
        uint32 stat = 0;
        int32 value = 0;
        bool native = false;
    };

    // (Re)loads arpg_affix + arpg_affix_tier (startup, .arpg reload). Returns a one-line summary.
    std::string LoadAffixData();
    // Startup: removes affix rows of items that no longer exist.
    void CleanOrphanAffixes();
    // Party bot gear (m7): the bot's next roll passes the "bots don't roll" gate, with this tier offset (e.g. -1).
    void AllowNextBotRoll(Player* bot, int tierOffset);
    // The highest affix tier (T1-T5) that unlocks at this level (arpg_affix_tier).
    int MaxAffixTierAtLevel(int level);
    // Tests: the character's next roll counts as a drop in a raid of this tier (Ascended tiers).
    void SetTestRaidTier(Player* player, int tier);

    // Sets ArpgHooks::ItemPickup and ArpgHooks::ItemSellPrice.
    void InstallPickupHook();

    // Vendor price of an item with affixes: template price + affixes x required level x Arpg.Loot.SellCopperPerAffixLevel.
    uint64 AffixSellPrice(Item const* item, uint64 basePrice);

    // Rolls and saves affixes for this item if the rules allow it (real player, quality, source, no Blizzard random stats).
    // Returns the number of affixes rolled (0 = skipped). `why` explains a skip.
    // sourceId: creature entry for creature loot (boss loot rules), 0 otherwise.
    // maxCount: at most this many affixes (Uniques: 4 + the power line), 0 = the quality's normal count.
    int RollForItem(Player* player, Item* item, ItemPickupSourceType source, std::string& why, uint32 sourceId = 0, int maxCount = 0);

    // Roll at loot time (owner 2026-11-22): the affixes (and Unique) are decided when a corpse/chest is filled, the loot item
    // carries the final name (suffix id), and picking it up applies exactly that plan. Values roll at pickup (not visible before).
    struct RollPlan
    {
        uint32 itemId = 0;
        std::vector<Core::RolledAffix> rolled;
        uint32 makeUnique = 0;
        uint32 nameAffixId = 0;
        bool godRoll = false;
        bool championLoot = false;
        bool planned = false;
        ItemPickupSourceType source{};
        uint32 sourceId = 0;
        uint32 madeAt = 0;
    };
    bool PlanRoll(Player* player, ItemTemplate const* proto, ItemPickupSourceType source, uint32 sourceId, int maxCount,
                  bool championLoot, RollPlan& plan, std::string& why);
    int ApplyRoll(Player* player, Item* item, RollPlan const& plan, std::string& why);
    // Living auction house (m7): rolls affixes on an ownerless item for `level` (+ tierOffset tiers). `why` = summary or reason.
    int RollForAuction(Item* item, int level, int tierOffset, std::string& why);
    // Plans every rollable item of a loot (corpse, chest) and sets each item's suffix id so the loot window shows its name.
    void PlanLoot(Player* owner, ::Loot& loot, ItemPickupSourceType source, uint32 sourceId, bool championLoot);
    bool TakePlan(uint32 itemId, int32 rp, RollPlan& out);
    // The affix lines a planned loot item will get when `player` picks it up (exact values: the plan fixes its rolls), for the
    // loot window: "s:<display stat>:<value>;e:<effect>:<tenths, 0 = flag>;u:<unique id>:0". Empty = no plan. Nothing is taken.
    std::string PlanPreview(Player* player, uint32 itemId, int32 rp);
    // ITEM_MOD id of the player's spec main stat (class fallback below level 10).
    uint32 SpecMainStat(Player* player);
    // The same encoding (stats first, then effects; no "u:") for the last item rolled for `player` (tests).
    std::string LastItemPreview(Player* player);
    // The stat lines a planned loot item (still on a corpse) will roughly have at `level`: each flat-stat affix of the first
    // plan for (itemId, suffix id) at the middle of its tier range. Nothing is taken; empty = no plan (bots, Need rolls).
    std::vector<StatLine> PlannedStatLines(uint32 itemId, int32 rp, int level);
    // The final kill script that plans corpse loot; must be installed after BossPity, Champions and Superuniques.
    void InstallLootPlanner();

    // One effect affix on an item (leech, procs, move speed...): Core::Effect id and value in hundredths of a percent.
    struct EffectLine
    {
        int effect = 0;
        int32 value = 0;
    };

    // Startup (after CleanOrphanAffixes): loads every item's flat stat affixes into memory, so equipping never hits the DB.
    void LoadStatCache();
    // The item's flat stat affixes (empty = none). Thread-safe.
    std::vector<StatLine> ItemStatLines(uint32 itemGuid, bool includeNative);
    // The item's effect affixes (empty = none). Thread-safe.
    std::vector<EffectLine> ItemEffectLines(uint32 itemGuid);

    // ".arpg loot last <char>": the last item rolled for that character, read back from the database.
    std::string DescribeLastItem(Player* player);
    // Tests: low guid of the last item rolled for this character (0 = none since the server started).
    uint32 LastItemGuid(Player* player);
}

#endif
