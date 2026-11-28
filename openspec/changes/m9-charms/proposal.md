# m9-charms: charms and the Charm Pouch (owner 2026-11-28)

## Why
Diablo II charms are the one item type from the original spec (SPEC 4.4) we never built. Owner: "ideally grant players a charm
bag and limit the effects to items in that bag only." Charms add a second gearing layer (stats, magic find, gold find, XP,
move speed) without touching equipment slots, and a bag-space choice: the pouch has a fixed number of slots.

## What
- **Charm Pouch** (item 720000): a soul-bag-type container (`BagFamily` 4) every character gets once (login, free bag slot or
  bags, mailed when full). Only charms fit in it. Unique, soulbound, 8 slots (item_template; SQL is the knob).
- **Charms** (720001 Small / 720002 Large / 720003 Grand, green/blue/purple): 1 / 2 / 3 random lines from `arpg_charm_affix`
  (flat stats as a share of a ring's B at the loot level, or effect lines: magic find, gold find, XP, move speed, armour,
  regen). Lines roll when the charm reaches a real player's bags (loot pickup, test give) and are stored per item
  (`item_instance_arpg_charm`).
- **Unique charms** (720010-720012, orange, max one each per character): fixed line sets (`arpg_charm_unique`); dropped by
  Superuniques, Treasure Goblins and rifts.
- **Only the pouch counts:** a charm grants its lines only while it sits in an equipped Charm Pouch; moving it out removes them.
- **Drops:** chance per kill by monster kind (normal, elite, Champion, Superunique, boss), size weights 70/25/5; tunables
  `Arpg.Charms.*`.

## Out of scope
- Charm-only client tooltip work beyond the affix display rows we already ship (owner checks how the lines look).
- Multi-slot charm sizes (WoW bags have 1-slot items; size = number of lines).
