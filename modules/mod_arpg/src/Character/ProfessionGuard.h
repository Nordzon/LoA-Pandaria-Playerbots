// Profession guardrails (change m9-stacks-trainer, owner 2026-11-28: "learned apprentice herbalism and jewelcrafting and my
// other professions were wiped"). With 11 primary professions the client still believes in 2 (it has two profession
// slots), so its "abandon" can hit a profession the player never meant. The server now refuses the client's abandon
// (CMSG_UNLEARN_SKILL) for primary professions unless Arpg.Professions.AllowClientAbandon is 1; abandoning is the explicit
// player command ".professions abandon <name>". Every primary profession learned / removed is written to Server.log
// ("[ARPG] profession ...") and to the test events.
#ifndef ARPG_CHARACTER_PROFESSIONGUARD_H
#define ARPG_CHARACTER_PROFESSIONGUARD_H

#include "Define.h"

#include <string>
#include <utility>
#include <vector>

class Player;

namespace Arpg::Character
{
    void InstallProfessionGuard();
    // Skill id of a primary profession by name ("herbalism", "jewelcrafting", "jc" ...); 0 if unknown.
    uint32 ProfessionByName(std::string name);
    // Removes a primary profession on purpose. out = reply.
    bool AbandonProfession(Player* player, uint32 skill, std::string& out);
    // Primary professions the character can still learn (or knows as a bare skill without its spell): (name, skill).
    std::vector<std::pair<std::string, uint32>> LearnableProfessions(Player* player);
    // Teaches the Apprentice rank of a primary profession (companion "Learn a new profession": the client's trainer
    // refuses a third profession because it has only two profession slots). Costs Arpg.Companion.ProfessionCost.
    bool LearnProfession(Player* player, uint32 skill, std::string& out);
    // "professions count=N <name>=value/max ..." (primary professions only).
    std::string DescribeProfessions(Player* player);
}

#endif
