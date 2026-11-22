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

    // Stats inside a tab (Diablo 3 style). Core: 0 main stat, 1 stamina. Offense: 0 crit, 1 haste, 2 mastery.
    // Defense: 0 armour, 1 dodge, 2 parry. Utility: 0 move speed, 1 magic find, 2 gold find.
    bool ParseTabStat(std::string const& tab, std::string const& stat, int& tabOut, int& statOut);
    // Spends n points of the tab on the stat (if the rotation gave the tab that many unspent points) and re-applies the
    // bonuses. `out` reports the measured change of that stat.
    bool Spend(Player* player, int tab, int stat, int n, std::string& out);
    // Free respec (owner): every point back, bonuses removed.
    void Respec(Player* player);
    // "applied main_stat_pct=… stamina_pct=… crit=… …"
    std::string DescribeApplied(Player* player);
}

#endif
