# m8-friends

## Why
The owner wants his mates to play on his server with him (2026-11-27): they should "just smash in a username and pass and get
an account", connect from their own PCs, and play the same ARPG layer. Today everything listens on 127.0.0.1 only, accounts
are made by script/GM command, and much of the ARPG layer assumes one real player.

## What changes
1. **Backups (done first, owner rule):** `backup-db.ps1` dumps auth + characters before every rebuild/restart (rebuild-and-test),
   keeps 30; rebuild-and-test refuses to stop the server while a real player is online (`-Force` to override).
2. **Self-service sign-up page:** `arpg/signup/signup.py` (Python stdlib, no installs): a form (name, password twice, optional
   invite code) that creates a normal player account in `auth` (same storage as `new-account.ps1`, never GM, never overwrites an
   existing account). Limits: invite code, account cap, per-address rate limit. Reachable only where it is bound (127.0.0.1 by
   default; the Tailscale address for friends).
3. **Private network (Tailscale):** docs/FRIENDS.md: install Tailscale on the owner's PC and each mate's PC, one shared tailnet;
   servers bind to the Tailscale address (`write-confs.ps1 -BindIP`), realm address set to it (`set-realm-address.ps1`). No port
   forwarding, nothing exposed to the internet. Installing Tailscale and a firewall rule = system changes -> owner does them.
4. **Client kit:** `make-client-kit.ps1` -> a zip with our parts only (patch MPQ, ArpgUI addon, realmlist, a README / install
   script); mates bring their own 5.4.8 client and start `Wow-64-all.exe`.
5. **Multiplayer rules (needs owner decisions, later tasks):** world scaling with several real players (highest level near the
   creature), personal loot, one companion per player, Champion/Superunique spawns and pity per player, bot count with friends
   online, Paragon world tier with mixed Paragon levels.

## Out of scope
Public internet hosting, web account management (password reset by the owner with `new-account.ps1`), Battle.net login.

## Milestone
8 (owner 2026-11-27).
