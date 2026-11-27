// Open-world creature level scaling near real players (change m1-world-scaling, SPEC 4.2).
#ifndef ARPG_WORLD_SCALING_H
#define ARPG_WORLD_SCALING_H

#include "Define.h"

#include <string>

class Creature;
class Player;

namespace Arpg::Scaling
{
    // Sets the Creature::SelectLevel hook (ArpgHooks::CreatureLevel) and registers the player script.
    void Install();

    // A player controlled by a person (not a playerbot).
    bool IsRealPlayer(Player* player);

    // Nearest creature to the player that is a scaling candidate by type (alive, not a pet/summon/critter), or null.
    Creature* FindNearestCandidate(Player* player, float radius);
    // Tests: the nearest creature that is scaled right now (skips fights, which keep their state); uncappedOnly = only in
    // zones with a world tier (Arpg.Scale.UncappedZones - the Vale test spot stands at the Vale's edge).
    Creature* FindNearestScaled(Player* player, float radius, bool uncappedOnly);

    // One line for .arpg scale probe: entry, levels, "scaled=<level>|none", health and multipliers.
    std::string Describe(Creature* creature);
    // The world tier multiplier applied to this creature (1 = none / not scaled).
    float TierOf(Creature* creature);

    // Number of creatures currently scaled (all maps).
    size_t ScaledCount();

    // m3 Champions: fixes a creature's level and adds health/damage multipliers on top of the scaling mods; the scaling tick
    // and sweep leave it alone until Forget (death). A respawn after Forget starts clean.
    void Pin(Creature* creature, uint8 level, float healthMult, float damageMult);
    bool IsPinned(Creature* creature);
    void Forget(uint64 rawGuid);
    // Called once per creature life the first time the scaling tick sees it near a real player (open world, attackable).
    // Return true if the handler pinned it (the tick then skips normal scaling).
    using NewCandidateFn = bool (*)(Creature* creature, Player* player);
    void SetNewCandidateHandler(NewCandidateFn fn);
}

#endif
