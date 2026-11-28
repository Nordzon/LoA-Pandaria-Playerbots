// GM / test commands of charms, shrines, Treasure Goblins and rifts (changes m9-*), hung under ".arpg" by cs_arpg.cpp;
// the player command ".rift" registers itself (AddSC_arpg_m9_commandscript).
#ifndef ARPG_COMMANDS_M9COMMANDS_H
#define ARPG_COMMANDS_M9COMMANDS_H

#include "Chat.h"

#include <vector>

namespace Arpg::Commands
{
    std::vector<ChatCommand> const& CharmTable();
    std::vector<ChatCommand> const& ShrineTable();
    std::vector<ChatCommand> const& GoblinTable();
    std::vector<ChatCommand> const& RiftTable();
}

void AddSC_arpg_m9_commandscript();

#endif
