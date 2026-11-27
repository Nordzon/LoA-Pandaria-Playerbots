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

## 4. Multiplayer rules (owner decisions first, then one task each)
- [ ] 4.1 World scaling with several real players near a creature
- [ ] 4.2 Loot: personal loot / plans per player, loot pet per player
- [ ] 4.3 Champions/Superuniques per player, pity per player (already per character)
- [ ] 4.4 Bot count and server load with friends online
- [ ] 4.5 GM tests with two real test characters

## 5. Docs
- [ ] 5.1 SPEC 4.11, CODEMAP, PROGRESS
