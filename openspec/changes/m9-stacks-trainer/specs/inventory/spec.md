## ADDED Requirements
### Requirement: Big stacks
Every item whose stock stack size is 2 to `Arpg.Stacks.Max` - 1 SHALL stack to `Arpg.Stacks.Max` (default 999); items that
do not stack SHALL stay unstackable; 0 SHALL restore the stock sizes.

#### Scenario: Cloth stacks to 999
- WHEN a character receives 500 Linen Cloth (stock stack 20) into empty bags
- THEN they form one stack of 500

#### Scenario: Switched off
- WHEN `Arpg.Stacks.Max` is set to 0 and `.arpg stacks apply` runs
- THEN Linen Cloth stacks to 20 again
