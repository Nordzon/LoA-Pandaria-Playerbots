# m1b-all-professions

## Why
Single player: there's nobody to trade with, so one character should be able to learn every profession.

## What changes
- Server: raise the primary-profession limit from 2 to 11 (TrinityCore-family config `MaxPrimaryTradeSkill`,
  to be confirmed in this core during m0; if it's hard-coded, a one-line `// ARPG:` patch).
- Client: the spellbook's professions tab only has two primary slots. ArpgUI adds a professions panel that
  lists every learned profession with its skill level and opens its window on click.

## Config / SQL added
`MaxPrimaryTradeSkill = 11` in worldserver.conf. No SQL.

## Out of scope
Bot professions. Profession-specific balance changes.

## Milestone
1
