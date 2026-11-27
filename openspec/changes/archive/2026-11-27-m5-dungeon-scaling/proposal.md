# m5-dungeon-scaling

## Why
The owner wants every dungeon and raid soloable or doable with a few bots, at any level, by any role (SPEC 4.6, BALANCE §5,
§6, §10): dungeons are the catch-up path when the open world's loot ceiling says "move on", and raids are the endgame
ladder that Paragon power climbs (with the Ascended affix tiers T6-T8 as the reward).

## What changes
- **Party-size table** instead of AutoBalance's curve (which gives a solo player 12% creature stats): `arpg_party_scale`
  (instance kind 5-man / raid, players 1..N -> health %, damage %). 5-man: 30/40, 45/55, 65/70, 80/85, 100/100;
  raids (10-man normal = 100%): 10/25, 16/32, 22/40, 35/55, 100/100 for 1/2/3/5/10. Built on the core's AutoBalance
  (already ported): one small `// ARPG:` hook where AutoBalance sets a creature's health/damage multipliers.
- **Raid tiers** (`arpg_raid_tier` by map id, tiers 1-7, Classic -> Siege of Orgrimmar): extra `1 + 0.15 × tier` on health
  and damage; recommended Paragon per tier (BALANCE §6).
- **Solo roles** (alone in a 5-man): non-tank spec -> creature damage × `Arpg.Dungeon.SoloNonTankDamage` (0.75); tank spec
  -> player damage dealt × `SoloTankDamageDealt` (1.25); healer spec -> player damage dealt × `SoloHealerDamageDealt` (1.6).
- **Dungeon level selector**: the player picks the level a dungeon runs at (dungeon min level .. own level), remembered per
  dungeon; creatures and loot (budget, affix tiers) follow it. `.arpg dungeon level <char> <map> <n>` (companion gossip later).
- **Ascended affix tiers in raids**: T6/T7/T8 need raid tier 3/5/7 **and** Paragon 25/60/110 (`Core::MaxAffixTier`, BALANCE §12).
- Test tools: `.arpg dungeon probe <char>` (map, players counted, role, multipliers of the nearest creature).

## Out of scope
LFG any-level queueing with bots (m7), companion gossip UI (m2b), boss mechanic changes.

## Milestone
5 (after m4 Paragon).
