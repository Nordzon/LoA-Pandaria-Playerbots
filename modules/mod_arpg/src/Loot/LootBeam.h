// Loot beam (SPEC 4.7 / m2b 2.4, owner pick 2026-11-26: spell 120720 "Light Beam: Feralas Blue"): a corpse that still holds
// an item of Arpg.Loot.BeamMinQuality or better after loot planning gets a light beam - an invisible marker creature at the
// corpse carrying Arpg.Loot.BeamAura (a dead creature can't hold the aura itself). The marker goes when the corpse is
// looted, despawns, or after 5 minutes.
#ifndef ARPG_LOOT_LOOTBEAM_H
#define ARPG_LOOT_LOOTBEAM_H

class Creature;
class Player;

namespace Arpg::Loot
{
    void InstallLootBeam();
    // Called after the corpse's loot is planned (kill script): beam it if it holds something worth picking up.
    // Returns true if a beam was put up.
    bool MaybeBeamCorpse(Player* owner, Creature* corpse);
    // How many beams this player's kills currently show (tests).
    int ActiveBeams(Player* owner);
}

#endif
