# m8-friends tasks

## 1. Safety first
- [x] 1.1 `arpg/scripts/backup-db.ps1` (auth + characters, keep 30) + call in rebuild-and-test before stopping (2026-11-27)
- [x] 1.2 rebuild-and-test refuses to stop while a real player is online (`-Force` overrides)

## 2. Sign-up page
- [x] 2.1 `arpg/signup/signup.py` + `signup.local.json.example`; `.gitignore` the local file - done 2026-11-27
- [x] 2.2 `start-signup.ps1`; start-stack starts it when configured
- [x] 2.3 `test-signup.ps1` (headless login with the new account; refusals; cleanup) - ALL OK (12 checks incl. headless login; no restart)

## 3. Network + client kit
- [x] 3.1 docs/FRIENDS.md: Tailscale steps (owner + mates), firewall rule, what to send mates - docs/FRIENDS.md
- [x] 3.2 `write-confs.ps1 -BindIP`, `set-realm-address.ps1` (realmlist address/localAddress) - BindIP verified in a temp conf; realm script tested with -Reset
- [x] 3.3 `make-client-kit.ps1` (patch MPQ, ArpgUI, realmlist, README) -> zip - kit built + installed into a fake client (realmlist, MPQ, 8 addon files)
- [ ] 3.4 Owner: install Tailscale, firewall rule; a mate signs up and logs in

## 4. Multiplayer rules (owner 2026-11-27: "as recommended"; Paragon tier "highest for now, easy reconfig")
- [x] 4.1 World scaling: highest level among the real players near a monster; Vale world tier by `Arpg.Paragon.WorldTierMode`
  (highest|average|lowest, `Core::GroupParagonLevel` + unit test); Paragon XP uses the monster's applied tier
- [x] 4.2 Personal loot (`Loot/PersonalLoot`, `Arpg.Party.PersonalLoot|PersonalLootRange`): killer keeps the corpse (round robin
  owner; groups with 2+ real players switch to round robin), every other real player nearby gets their own loot-table roll +
  extras into the bags (mail when full); companion already one per player (queues its owner's kills)
- [x] 4.3 Champion floor, Superunique share (blue + Unique ladder on their own counter), boss pity, potions, catalysts per player
- [x] 4.4 Bots: `$FriendsBotCount` in network.local.ps1 (300) applies while friends mode is set; explicit -BotCount wins
- [x] 4.5 `55-friends.gmtest` with Arpgtest + Arpgtesttwo (account ARPGTEST2, held by rebuild-and-test) - needs the restart - ALL OK 19:20 (Arpgtesttwo parked at `park-stormwind` between tests)
- [ ] 4.6 Owner: play with a mate; tune

## 5. Docs
- [ ] 5.1 SPEC 4.11, CODEMAP, PROGRESS
