## ADDED Requirements

### Requirement: Champion power follows the player's level band
Champions and Superuniques SHALL take their level offset, health and damage multipliers, monster-affix count and minion count
from the `arpg_monster_band` row for the player's level, and SHALL never be below the player's level.

#### Scenario: Fresh character meets a Champion
- WHEN a level 5 character is near a newly spawned Champion
- THEN the Champion is level 5 with 2.0x health, 1.15x damage, exactly 1 soft monster affix and 1 minion

#### Scenario: Endgame Champion
- WHEN a level 90 character is near a newly spawned Champion
- THEN the Champion is level 90 (capped) with 3.0x health, 1.5x damage and exactly 2 monster affixes

### Requirement: Monster affixes unlock by level and never one-shot
Monster affixes SHALL only appear from their unlock level (soft 1, Enchanted 15, Arcane Caster 25), with at most one Enchanted
affix per Champion, and affix damage SHALL be a share of the player's max health, always less than the player's max health.

#### Scenario: No explosion below 15
- WHEN a Champion spawns for a level 12 character
- THEN its affix is one of Extra Strong, Extra Fast or Stoneskin

#### Scenario: Death explosion is a share of health
- WHEN a Fire Enchanted Champion dies next to a level 40 character with 100,000 max health
- THEN the explosion deals 25,000 damage

### Requirement: Kills restore health and power
A Champion kill SHALL restore `orb_pct` (20%) of the killer's max health and max power, a Superunique kill 40%.

#### Scenario: Health orb
- WHEN a character at 30% health kills a Champion
- THEN the character is at 50% health right after the kill
