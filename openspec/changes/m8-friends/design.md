# m8-friends design

## Sign-up page (`arpg/signup/signup.py`)
- Python 3 stdlib `http.server` (ThreadingHTTPServer); no packages. MySQL through `mysql.exe` + `arpg/scripts/mysql.local.cnf`
  (as every script here). Settings: `arpg/signup/signup.local.json` (git-ignored: bind, port, invite code, cap, realm address
  shown to the player); without it: 127.0.0.1:8090, no invite code, cap 20.
- Rules: name `^[A-Za-z0-9]{3,16}$`, password `^[A-Za-z0-9]{4,16}$` (the same rules as new-account.ps1; MoP's client sends
  upper-case SRP, so case doesn't matter), both password fields equal, reserved prefixes refused (RNDBOT, ARPG), invite code
  compared in constant time, cap counts accounts with `invited_by = 'signup'`, 5 attempts per address per 10 minutes.
- Storage = new-account.ps1: `sha_pass_hash = UPPER(SHA1(UPPER(name):UPPER(pass)))`, `v`/`s` empty (authserver derives them at
  the first login), expansion 4, `realmcharacters` rows, no `account_access` row (player). Plain `INSERT`: an existing name is
  refused, never reset. Inputs are restricted to letters/digits before they reach SQL.
- Log: `D:\WoWMoPProject\ServerData\logs\signup.log` (time, address, name, result - never the password).
- Start: `arpg/scripts/start-signup.ps1` (detached, idempotent); `start-stack.ps1` starts it too when signup.local.json exists.
- Test: `arpg/scripts/test-signup.ps1` - page on a test port; register; DB row; headless login with the new account works and
  a wrong password fails; duplicate, mismatch, bad name, bad invite refused; test account deleted. No server restart.

## Network
Tailscale (WireGuard) tailnet: the owner's PC gets a stable 100.x address and a MagicDNS name. authserver/worldserver BindIP
`0.0.0.0` (or that address), `realmlist.address` = the Tailscale address, `localAddress` 127.0.0.1 + mask so the owner's own
client still connects locally. Windows Firewall: allow inbound 3724/8085 (and 8090 for sign-up) on the Tailscale interface only.
