# m10-companion-professions: profession trainers and a recipe shop on the companion

## Why (owner 2026-11-28)
"I want the professions trainers in the class pet please. Professions submenu > each profession as part of that. All of the
recipes as well please. Then each profession sub-menu will also have the profession-specific recipes that you usually get
from rep grind and dailies sold for gold. Imagine some numbers that are not easy to get, but bearable."

## Scope
- Companion "Professions" -> each profession the character knows (11 primaries, Cooking, First Aid, Fishing) ->
  "<Profession> trainer (all ranks and recipes)": a trainer window with every spell any trainer of that profession teaches
  (npc_trainer rows by reqskill, lowest price), built in `Character/ProfessionShop` and served through a new hook
  `ArpgHooks::TrainerSpells` (SendTrainerList + HandleTrainerBuySpellOpcode): npc_trainer ignores entries >= 200000.
- "Reputation recipes for gold (page n/m)": recipes sold by reputation or token vendors or rewarded by daily/weekly quests
  (996: JC 396, LW 223, BS 122, Tailoring 78, Enchanting 75, Cooking 60, Alchemy 32, ...), copies at 1,100,000 + id without
  reputation/faction/race limits, sold by hidden vendors 1,000,100+ (150 per page). Prices (`arpg_recipe_price`, gold by
  required skill): <=300: 25, <=375: 75, <=450: 150, <=525: 300, <=600: 600; epic x2.
- Settings `Arpg.Companion.ProfessionTrainer`, `Arpg.Companion.RecipeShop`; the companion gets the trainer NPC flag.
