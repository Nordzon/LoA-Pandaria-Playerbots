// Paragon (SPEC 4.5, change m4-paragon): from the max level all XP (kills, quests) becomes Paragon XP; levels and XP are kept
// in character_paragon. Points/bonuses and the Vale world tier come in the next steps.
#ifndef ARPG_PARAGON_PARAGON_H
#define ARPG_PARAGON_PARAGON_H

#include "Define.h"

#include <string>

class Player;

namespace Arpg::Paragon
{
    void InstallParagon();

    int ParagonLevel(Player* player);
    // Adds Paragon XP (already multiplied); returns levels gained. Saves.
    int AddXp(Player* player, uint64 xp, std::string const& why);
    // Tests: sets level and XP into it.
    void SetParagon(Player* player, int level, uint64 xp);
    // "paragon level=… xp=…/… points core=… offense=… defense=… utility=…"
    std::string Describe(Player* player);
}

#endif
