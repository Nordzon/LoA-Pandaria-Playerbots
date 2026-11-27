// Typed blank bases (change m6-blank-bases, BALANCE 8c): which blank fits a character. Pure logic, no game headers.
#ifndef ARPG_CORE_BLANKS_H
#define ARPG_CORE_BLANKS_H

#include <cstdint>
#include <vector>

namespace Arpg::Core
{
    // Brackets are the top RequiredLevel of 10-level groups (10, 20, ... 90). The blank for `level` is the highest bracket
    // not above it (the character can wear it); below the lowest bracket, the lowest. 0 when there are none.
    int PickBlankBracket(std::vector<int> const& brackets, int level);

    // The variant with the wanted main stat (ITEM_MOD id) if there is one, else the first. 0 when the list is empty.
    uint32_t PickBlankMainStat(std::vector<uint32_t> const& available, uint32_t wanted);

    // The armour subclass a class wears at a level (1 cloth, 2 leather, 3 mail, 4 plate): warriors and paladins plate from
    // 40 (mail before), death knights always plate, hunters and shamans mail from 40 (leather before), rogues, druids and
    // monks leather, priests, mages and warlocks cloth. Class ids as in the game (1 warrior ... 11 druid).
    int ArmorSubclassForClass(int classId, int level);

    // The affix count a dropped or bought blank rolls: the most its quality allows (green 2, blue 3, epic 5), at most
    // `cap` when cap > 0 (a Unique keeps 4 + its power line).
    int BlankAffixCount(int quality, int cap);
}

#endif
