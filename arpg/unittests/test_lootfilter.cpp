// Loot filter (change m2b task 2.1, spec loot-filter): match types, rule order, protected items, text form.
#include "TestHarness.h"
#include "LootFilter.h"

using namespace Arpg::Core;

static FilterRule Rule(std::string const& text)
{
    FilterRule r;
    CHECK(RuleFromText(text, r));
    return r;
}

TEST(filter_greys_sold_and_default_pickup)
{
    std::vector<FilterRule> rules = { Rule("quality<=0 -> sell") };
    ItemFacts grey; grey.quality = 0;
    ItemFacts green; green.quality = 2;
    int m = 0;
    CHECK(EvaluateFilter(rules, grey, m) == FilterAction::AutoSell);
    CHECK_EQ(m, 0);
    CHECK(EvaluateFilter(rules, green, m) == FilterAction::Pickup);   // no rule matches
    CHECK_EQ(m, -1);
}

TEST(filter_unusable_left_and_rule_order)
{
    std::vector<FilterRule> rules = { Rule("tier>=5 -> highlight"), Rule("usable=0 -> leave") };
    ItemFacts cloth; cloth.quality = 2; cloth.usable = false; cloth.bestTier = 2;
    ItemFacts clothT5 = cloth; clothT5.bestTier = 5;
    int m = 0;
    CHECK(EvaluateFilter(rules, cloth, m) == FilterAction::Leave);
    CHECK(EvaluateFilter(rules, clothT5, m) == FilterAction::Highlight);   // first rule wins
    CHECK_EQ(m, 0);
}

TEST(filter_protected_items_always_picked_up)
{
    std::vector<FilterRule> rules = { Rule("quality>=0 -> sell") };   // a bad rule: sell everything
    ItemFacts unique; unique.quality = 4; unique.unique = true;
    ItemFacts quest; quest.quest = true;
    int m = 0;
    CHECK(EvaluateFilter(rules, unique, m) == FilterAction::Pickup);
    CHECK_EQ(m, -1);
    CHECK(EvaluateFilter(rules, quest, m) == FilterAction::Pickup);
}

TEST(filter_match_types)
{
    ItemFacts f; f.itemId = 82437; f.quality = 4; f.itemClass = 4; f.itemSubClass = 1; f.affixCount = 3; f.bestTier = 4;
    f.itemLevel = 450; f.equippedItemLevel = 463;
    CHECK(RuleMatches(Rule("class=4 sub=1 -> leave"), f));
    CHECK(!RuleMatches(Rule("class=2 -> leave"), f));
    CHECK(RuleMatches(Rule("affixes>=3 affixes<=3 -> leave"), f));
    CHECK(!RuleMatches(Rule("affixes>=4 -> leave"), f));
    CHECK(RuleMatches(Rule("vs=0 -> sell"), f));            // lower than equipped
    CHECK(!RuleMatches(Rule("vs=1 -> pickup"), f));
    CHECK(RuleMatches(Rule("ids=1,82437 -> pickup"), f));
    CHECK(!RuleMatches(Rule("ids=1,2 -> pickup"), f));
    ItemFacts noGear = f; noGear.equippedItemLevel = -1;
    CHECK(!RuleMatches(Rule("vs=0 -> sell"), noGear));      // nothing equipped: no comparison
}

TEST(filter_text_round_trip_and_bad_input)
{
    FilterRule r = Rule("quality<=1 usable=0 class=4 tier>=3 ids=5,6 -> highlight");
    FilterRule back;
    CHECK(RuleFromText(RuleToText(r), back));
    CHECK_EQ(back.maxQuality, 1);
    CHECK_EQ(back.usable, 0);
    CHECK_EQ(back.minBestTier, 3);
    CHECK_EQ(int(back.itemIds.size()), 2);
    CHECK(back.action == FilterAction::Highlight);
    FilterRule bad;
    CHECK(!RuleFromText("quality<=1", bad));                // no action
    CHECK(!RuleFromText("colour=red -> sell", bad));        // unknown key
    CHECK(!RuleFromText("quality<=1 -> burn", bad));        // unknown action
}
