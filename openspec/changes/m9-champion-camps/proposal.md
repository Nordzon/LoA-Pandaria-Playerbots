# m9-champion-camps: one Champion per camp, never quest NPCs; Superuniques in every zone

## Why (owner 2026-11-28, playing a Pandaren on the Wandering Isle)
- "can we make it so each spawn location with mobs gets a champion? ... a rather large disparity in champion spawns -
  sometimes quest npcs will be champions and some places will not have a champion at all."
- "I have not noticed any superuniques yet" -> "add rare monsters - several per zone please."
- Load question: "will it load the server too much? If not - proceed."

## Scope
- Camps: the open world is cut into squares (`Arpg.Champion.CampSize` 100 yd). A square with at least `CampMin` (4) eligible
  monsters holds one Champion (a random eligible one); `CampRespawn` (600 s) after it is gone the square gets a new one. The
  check runs inside the existing scaling tick's first sight of a monster (no new timers; a small map of squares). The old
  random roll stays as `Arpg.Champion.Chance`, now 0 by default.
- Never Champions: NPCs with a menu/quest flag, C++-scripted story NPCs, monsters friendly to the player (sparring partners),
  critters, rares, bosses, Superunique rares, entries in `arpg_champion_exclude` (Training Target, trainees, Jaomin Ro...).
- Superuniques: zones with fewer than 5 rares are topped up with "Unique" versions of their toughest ordinary hostile
  monsters (generator `gen_superuniques.py`, ids 5000+; 145 stand-ins in 50 zones incl. 5 on the Wandering Isle);
  `Arpg.Superunique.PerZone` 1 -> 3 alive per zone. The "native rare steps aside" rule now only applies to real rares.
- Tests: `.arpg champion camp <char> [reset]`, `.arpg champion eligible <char> <entry>`, GM test 63, unit tests (camps).

## Load
One square lookup per newly seen monster, plus one 100-yd search when a square needs a Champion; nothing runs where no
real player is.
