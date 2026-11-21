# loot-pet — delta

## ADDED Requirements

### Requirement: Pet collects loot from the player's kills
While the loot pet is active, each creature the player kills SHALL join the pet's queue (at most `Arpg.LootPet.MaxQueue`).
The pet SHALL move to each queued corpse within `Arpg.LootPet.Range`, and its loot SHALL be transferred to the player.

#### Scenario: Pack cleared
- **WHEN** the player kills 5 wolves and leaves combat
- **THEN** the pet visits each corpse, and their gold and items end up with the player

#### Scenario: Waits for combat to end
- **WHEN** `Arpg.LootPet.WaitForCombatEnd` = 1 and the player is still in combat
- **THEN** the pet follows the player and loots nothing until combat ends

### Requirement: Never stuck
If the pet cannot reach a corpse within `Arpg.LootPet.ReachTimeout`, the corpse SHALL be looted from where the pet is.

#### Scenario: Corpse on a ledge
- **WHEN** a corpse is unreachable by walking
- **THEN** it is looted after the timeout and the pet moves on

### Requirement: Transfer rules
Gold SHALL be added at once, quest items SHALL always be taken, items that don't fit the bags SHALL be mailed,
and only items the player has loot rights to SHALL be taken. Items going to a group roll SHALL stay in the roll.

#### Scenario: Full bags
- **WHEN** bags are full and a corpse holds a green item
- **THEN** the item arrives by mail and the player gets a chat notice

#### Scenario: Pet-looted items get affixes
- **WHEN** the pet loots a green item
- **THEN** the item has 1–2 affixes, as if looted by hand

### Requirement: Toggle and persistence
`.arpg lootpet on|off <char>` SHALL toggle the pet. The choice SHALL persist across logins, and the pet SHALL
return by itself after zoning, teleporting or dying.

#### Scenario: After a teleport
- **WHEN** the player teleports to another zone with the pet on
- **THEN** the pet appears next to the player within 2 seconds
