## ADDED Requirements
### Requirement: Class trainer on the companion
The companion menu SHALL offer "Class trainer" (when `Arpg.Companion.Trainer` is 1) with: reset specialization and reset
talents (from level 10, through the game's confirm box), Dual Specialization for `Arpg.Companion.DualSpecCost` when the
character has one specialization and the level for it, Proving Grounds at level 90, and "Learn a new profession".

#### Scenario: Dual Specialization
- WHEN a level 30+ character with one specialization and 10 g picks "Learn Dual Specialization"
- THEN 10 g are taken and the character has two specializations (manual check)

### Requirement: Learn any profession from the companion
"Learn a new profession" SHALL list every primary profession the character lacks (or knows only as a skill without its
spell) and teach its Apprentice rank for `Arpg.Companion.ProfessionCost` from `Arpg.Companion.ProfessionLevel`.

#### Scenario: Fourth profession
- WHEN a character with three professions picks "Apprentice Tailoring"
- THEN it knows Tailoring as well and keeps the other three (manual check)
