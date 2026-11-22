// Superuniques (SPEC 4.3, BALANCE §14, change m3 task 3.4): every zone with a pool (arpg_superunique) keeps
// Arpg.Superunique.PerZone of them alive while a real player is in the zone. Power from arpg_monster_band kind 1 by the
// player's level (Scaling::Pin), abilities from the kit (arpg_superunique_ability, damage = % of max health). After a kill,
// Arpg.Superunique.RespawnDelay later a DIFFERENT one spawns at one of its own points (Core::NextSuperunique). Kill:
// health orb, a guaranteed blue, a Unique ladder (Arpg.Superunique.UniqueChance + UniqueBadLuckStep per kill without one).
#ifndef ARPG_MONSTERS_SUPERUNIQUES_H
#define ARPG_MONSTERS_SUPERUNIQUES_H

#include "Define.h"

#include <string>

class Creature;
class Player;

namespace Arpg::Monsters
{
    std::string LoadSuperuniqueData();
    void InstallSuperuniques();

    // Tests: spawns Superunique `id` (0 = the rotation's pick) for the player's zone next to the player, replacing the live one.
    bool SpawnSuperuniqueNear(Player* player, uint32 id, std::string& out);
    // Tests: the next spawn in the player's zone happens on the next tick (skips the delay).
    void SkipSuperuniqueDelay(Player* player);
    // The live Superunique of the player's zone (on the player's map), or null.
    Creature* ZoneSuperunique(Player* player);
    // The last Superunique killed by this player (its corpse), or null.
    Creature* LastKilledSuperunique(Player* player);
    // "superunique zone=12 pool=5 alive=1 [id=3 entry=471 level=.. hp=.. abilities=..] respawn_in=0 last_killed=0"
    std::string DescribeSuperuniques(Player* player);
    // A native spawn of a creature that is in a Superunique pool (the rare's own spawn, not ours): it is despawned on sight so
    // the zone only has the Superunique version.
    bool IsNativeSuperuniqueRare(Creature* c);
}

#endif
