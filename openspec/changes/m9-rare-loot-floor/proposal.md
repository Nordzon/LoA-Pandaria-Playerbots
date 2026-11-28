# m9-rare-loot-floor: rare-exclusive loot drops at least 30%

## Why (owner 2026-11-28)
"Can we move all of the unique-exclusive drop rates to at least 30%" -> chosen: "Rare's own exclusive loot" (mounts, pets,
toys, its special gear), for the dopamine of cool mounts on top of the ARPG loot.

## Scope
SQL `2026_11_28_04_arpg_rare_loot_floor.sql`: for rares in Superunique pools (their "Unique" copy shares the loot table),
every directly listed, ungrouped, non-quest item that at most 3 creatures drop goes to max(stock, 30%). Stock chances are kept
in `arpg_rare_loot_floor` (undo query in the file). 164 rows (e.g. Reins of the Thundering Onyx Cloud Serpent 1% -> 30%,
Pandaria rare toys 10-20% -> 30%). Live via `.reload creature_loot_template`, no restart.
