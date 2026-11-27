# m6-blank-bases: typed blank bases

## Why
Owner 2026-11-22 (BALANCE §8c) decided on typed blank bases: an item whose power comes from our affixes, keeping only its armour
type's main stat + Stamina. On 2026-11-27 the owner picked their use: "all of these" = (a) a drop that always rolls the most
affixes for its quality, (b) the crafting wheel's target items, (c) sold by the companion for essences.

## What changes
- **Items:** generated copies of real droppable gear (ids 710000+, `gen_blank_bases.py`), one per slot x item class/subclass x
  quality x 10-level bracket x main-stat variant (plate Str/Int, leather and mail Agi/Int, cloth Int, jewellery and cloaks
  Str/Agi/Int, weapons by their usual users). Same look, armour/damage and sockets; only main stat + Stamina at the
  original's values; no item set, no on-use/equip spells; description "Blank base: always rolls the most affixes for its
  quality." Table `arpg_blank` maps them; the client gets them through Item/Item-sparse hotfixes (as the custom items).
- **Rule:** a blank that drops or is bought rolls the most affixes for its quality (green 2, blue 3, epic 5; a Unique 4).
  A crafted blank rolls with the crafting wheel's rules (recipe colour, skill, catalyst).
- **(a) Drops:** `Arpg.Blank.DropChance` (10%) per affix-eligible green-epic weapon or armour piece in creature/chest loot:
  it becomes the blank of its slot/type/quality at the loot level, main stat of the looter's spec.
- **(b) Crafting:** `Arpg.Blank.Crafted` (1): crafted gear comes out as its blank (`ArpgHooks::CraftedItem`, a 2-line
  `// ARPG:` hook in `Spell::DoCreateItem`).
- **(c) Companion:** "Blank bases" in the companion dialogue: pick a slot, then a quality; costs
  `Arpg.Blank.EssenceCost.Green|Blue|Epic` (5 / 15 / 45) Salvaged Essence; armour type = the best the class wears at its
  level, main stat = the spec's, level = the loot level.

## Out of scope
Replacing every affixed drop by its blank (BALANCE §8c option b in full) - a later switch (`DropChance` 100) once played.

## Milestone
6 (owner decision 2026-11-27).
