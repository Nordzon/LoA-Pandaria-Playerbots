// Companion professions (change m10-companion-professions, owner 2026-11-28: "professions trainers in the class pet ...
// each profession ... all of the recipes as well ... the profession-specific recipes that you usually get from rep grind and
// dailies sold for gold"). Per profession:
//  - a trainer window with EVERY rank and recipe any trainer of that profession teaches (npc_trainer rows by reqskill, built
//    here: npc_trainer ignores creature entries >= 200000 like the companion; served through ArpgHooks::TrainerSpells);
//  - recipe vendor pages: reputation / token / daily recipes as gold copies (arpg_recipe_shop, sql/tools/gen_recipe_shop.py).
#ifndef ARPG_CHARACTER_PROFESSIONSHOP_H
#define ARPG_CHARACTER_PROFESSIONSHOP_H

#include "Define.h"

#include <string>
#include <utility>
#include <vector>

class Creature;
class Player;

namespace Arpg::Character
{
    std::string LoadProfessionShop();
    void InstallProfessionShop();

    // Professions the shop serves (11 primaries, Cooking, First Aid, Fishing), menu order; known or not.
    std::vector<uint32> ShopSkills(Player* player);
    std::string SkillName(uint32 skill);
    // Opens the profession's trainer window on the companion. out = reply on failure.
    bool OpenProfessionTrainer(Player* player, Creature* companion, uint32 skill, std::string& out);
    // Recipe pages of a profession: (page, number of recipes).
    std::vector<std::pair<uint32, uint32>> RecipePages(uint32 skill);
    // Opens a recipe vendor page on the companion.
    bool OpenRecipePage(Player* player, Creature* companion, uint32 skill, uint32 page, std::string& out);
    // Tests: "profshop skill=S trainer_spells=N recipes=R pages=P".
    std::string DescribeProfessionShop(uint32 skill);
    // Tests: the trainer list the hook serves for this player now ("none" or "skill=S spells=N").
    std::string DescribeServedTrainer(Player* player);
    // Tests: select a profession trainer for the player without a window (what the menu does before SendTrainerList).
    void SelectTrainer(Player* player, uint32 skill);
}

#endif
