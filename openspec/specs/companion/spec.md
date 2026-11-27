# companion Specification

## Purpose
TBD - created by archiving change m2b-mount-shop. Update Purpose after archive.

## Requirements

### Requirement: Reputation mounts for gold
The companion's dialogue SHALL sell every reputation mount in `arpg_mount_shop` that the character doesn't know, for
`Arpg.MountShop.Price` gold (or the row's own price), teaching the mount without any reputation requirement.

#### Scenario: Buy a mount
- WHEN a character with 6000 gold buys the Swift Violet Ram
- THEN 5000 gold is taken, the mount spell is learned, and the Ram is no longer listed

#### Scenario: Not enough gold
- WHEN a character with 100 gold tries to buy it
- THEN the purchase is refused and nothing changes
