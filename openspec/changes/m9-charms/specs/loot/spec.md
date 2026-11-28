## ADDED Requirements

### Requirement: Every character has a Charm Pouch
With `Arpg.Charms.Enabled` = 1, a real player's character that has no Charm Pouch (item 720000) anywhere (bags, bag slots,
bank) SHALL get one at login: in a free bag slot if there is one, else in the bags, else by mail. Only charms SHALL fit in it.

#### Scenario: Pouch granted once
- WHEN a character without a pouch logs in (or `.arpg charm pouch <char>` runs)
- THEN it has exactly one item 720000, and a second check grants nothing (`granted=0`)

### Requirement: Charms roll random lines
A Small, Large or Grand Charm reaching a real player's bags SHALL roll 1, 2 or 3 lines from `arpg_charm_affix` (no line
twice); flat-stat lines SHALL be a share of a ring's stat budget B at the loot level, effect lines a value range in
hundredths of a percent. Unique charms SHALL get their fixed lines from `arpg_charm_unique`.

#### Scenario: A Grand Charm has three lines
- WHEN `.arpg charm give <char> grand` runs
- THEN the reply lists `lines=3` with three different line names

### Requirement: Only charms in the pouch grant their lines
A charm's lines SHALL apply while, and only while, the charm is inside an equipped Charm Pouch.

#### Scenario: Moving a charm in and out
- WHEN a Small Charm with a stamina line is moved into the equipped pouch
- THEN `.arpg charm info` shows `active=1` and the line's stamina; moved back to the backpack, `active=0`

### Requirement: Charms drop from kills
A kill by a real player SHALL add a charm to the corpse with `Arpg.Charms.Drop<Kind>` % by monster kind (normal, elite,
Champion, Superunique, boss), size by `Arpg.Charms.Weight<Size>`.

#### Scenario: Forced drop
- WHEN `Arpg.Charms.DropNormal` is 100 and a test character kills a normal monster
- THEN the corpse loot holds one of 720001-720003
