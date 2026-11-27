// Typed blank bases (change m6-blank-bases).
#include "TestHarness.h"
#include "Blanks.h"

using namespace Arpg::Core;

TEST(blank_bracket_highest_not_above_level)              // Scenario: blank mapping follows the loot level
{
    std::vector<int> b = { 10, 20, 30, 90 };
    CHECK_EQ(PickBlankBracket(b, 20), 20);
    CHECK_EQ(PickBlankBracket(b, 29), 20);
    CHECK_EQ(PickBlankBracket(b, 85), 30);
    CHECK_EQ(PickBlankBracket(b, 90), 90);
    CHECK_EQ(PickBlankBracket(b, 4), 10);                  // below the lowest: the lowest
    CHECK_EQ(PickBlankBracket({}, 50), 0);
}

TEST(blank_main_stat_variant)                             // Scenario: ... and the spec (Agility variant)
{
    CHECK_EQ(PickBlankMainStat({ 3, 5 }, 3), 3u);
    CHECK_EQ(PickBlankMainStat({ 3, 5 }, 4), 3u);          // no Strength leather: the first variant
    CHECK_EQ(PickBlankMainStat({}, 4), 0u);
}

TEST(blank_armor_type_by_class_and_level)
{
    CHECK_EQ(ArmorSubclassForClass(1, 39), 3);            // warrior: mail before 40
    CHECK_EQ(ArmorSubclassForClass(1, 40), 4);
    CHECK_EQ(ArmorSubclassForClass(6, 55), 4);            // death knight
    CHECK_EQ(ArmorSubclassForClass(3, 20), 2);            // hunter: leather before 40
    CHECK_EQ(ArmorSubclassForClass(7, 90), 3);
    CHECK_EQ(ArmorSubclassForClass(10, 20), 2);           // monk
    CHECK_EQ(ArmorSubclassForClass(8, 90), 1);            // mage
}

TEST(blank_rolls_max_affixes_for_quality)                 // Scenario: a blank drops -> 2 / 3 affixes
{
    CHECK_EQ(BlankAffixCount(2, 0), 2);
    CHECK_EQ(BlankAffixCount(3, 0), 3);
    CHECK_EQ(BlankAffixCount(4, 0), 5);
    CHECK_EQ(BlankAffixCount(4, 4), 4);                   // a Unique: 4 + its power line
}
