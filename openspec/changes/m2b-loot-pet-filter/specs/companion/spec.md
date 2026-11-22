# companion — delta

## ADDED Requirements

### Requirement: Native service roles
The summoned companion SHALL carry the NPC flags of every role enabled in `Arpg.Companion.*` (vendor, repair, bank,
mailbox, reforge, transmogrification, void storage), so the player can open the core's own window for that service on it.

#### Scenario: Selling and repairing in the field
- **WHEN** the companion is summoned with `Arpg.Companion.Vendor` = 1 and `Arpg.Companion.Repair` = 1 and the player right-clicks it
- **THEN** the vendor window opens with a repair button, and selling and repairing work as at a town vendor

#### Scenario: Appearance changer
- **WHEN** `Arpg.Companion.Transmog` = 1 and the player chooses transmogrification on the companion
- **THEN** the native transmogrification window opens and applied appearances persist like a town transmogrifier's

#### Scenario: Role switched off
- **WHEN** `Arpg.Companion.Bank` is set to 0 and `.arpg reload` runs
- **THEN** the companion no longer offers the bank, without re-summoning

### Requirement: No services in combat
With `Arpg.Companion.InCombat` = 0, the companion SHALL offer no service while the player is in combat.

#### Scenario: Mid-fight
- **WHEN** the player enters combat
- **THEN** the companion's service flags are cleared, and restored when combat ends

### Requirement: Companion appearance
The player SHALL be able to choose the companion's model from `arpg_companion_model`, and the choice SHALL persist across logins.

#### Scenario: Model change persists
- **WHEN** the player picks another model from the companion's gossip menu and relogs
- **THEN** the companion is summoned with the chosen model
