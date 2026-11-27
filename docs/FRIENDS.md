# Playing with friends (m8-friends)

Your mates connect to your PC over **Tailscale**, a free private network between your PCs. Nothing is opened to the internet
and no router port forwarding is needed. They make their own account on a small **sign-up page** and install a **client kit**
(our patch + addon) into their own MoP 5.4.8 client.

## Once, on your PC (about 15 minutes)
1. **Tailscale:** install from https://tailscale.com/download, sign in. Note your address: `tailscale ip -4` (100.x.y.z).
   In the admin console (https://login.tailscale.com/admin/machines) use **Share** on your PC and send each mate the link:
   they then see only this PC, nothing else of yours.
2. **Firewall** (PowerShell as administrator; only Tailscale addresses get in):
   ```powershell
   New-NetFirewallRule -DisplayName 'Pandaria ARPG (Tailscale)' -Direction Inbound -Protocol TCP `
     -LocalPort 3724,8085,8090 -RemoteAddress 100.64.0.0/10 -Action Allow
   ```
   3724 = login, 8085 = game world, 8090 = sign-up page. SOAP (7878) listens on 127.0.0.1 only.
   **MySQL (found 2026-11-27):** it listens on all interfaces and an inbound rule "Port 3307" allows any address (probably from the
   MySQL installer). Before mates join, limit it to this PC (as administrator):
   ```powershell
   Set-NetFirewallRule -DisplayName 'Port 3307' -RemoteAddress 127.0.0.1    # or Disable-NetFirewallRule -DisplayName 'Port 3307'
   ```
   (Only the servers on this PC use MySQL. Also check "Port 33070", the MySQL X protocol: same treatment.)
3. **Network settings:** copy `arpg\scripts\network.local.ps1.example` to `network.local.ps1` and put your 100.x address in
   `$RealmAddress` (keep `$BindIP = '0.0.0.0'`; the firewall rule above limits who can reach it).
4. **Sign-up page:** copy `arpg\signup\signup.local.json.example` to `signup.local.json`: `"bind": "0.0.0.0"`, an
   `invite_code` you tell your mates, `realm_address` = your 100.x address (shown on the page).
5. **Apply (one server restart, done with Claude so nobody is mid-game):** `write-confs.ps1` (it now reads `$BindIP`), restart
   the servers, `set-realm-address.ps1`, `start-signup.ps1`. From then on `start-stack.ps1` starts the sign-up page too.
6. **Kit:** `powershell -ExecutionPolicy Bypass -File arpg\scripts\make-client-kit.ps1` -> a zip in
   `D:\WoWMoPProject\ServerData\client-kit\`. Add `-IncludeExe` if you're happy to share `Wow-64-all.exe` (the unlocked
   exe that loads our patch); otherwise mates need to get it themselves.

Send each mate: the Tailscale share link, the zip, the sign-up address `http://<your 100.x address>:8090/` and the invite code.

## What a mate does
1. Install Tailscale, open your share link, sign in.
2. Unzip the kit, right-click `install-kit.ps1` -> Run with PowerShell, point it at their MoP 5.4.8 client folder.
3. Open the sign-up page, pick a name and password.
4. Start `Wow-64-all.exe`, log in.

## Good to know
- Your own client keeps connecting to 127.0.0.1; nothing changes for you.
- Accounts from the page are normal players (no GM). Reset a mate's password: `new-account.ps1 -Name X -Password Y`.
  The page never changes an existing account. `signup.log` (ServerData\logs) lists sign-ups, never passwords.
- Turn sign-ups off: `start-signup.ps1 -Stop` (or lower `max_accounts`).
- Kit updates: after a client patch or addon change, run `make-client-kit.ps1` again and send the new zip.
- Back to solo: `set-realm-address.ps1 -Reset`, delete `network.local.ps1`, rerun write-confs + restart.
- Accounts and characters are backed up before every rebuild (`ServerData\backups`, last 30); restore steps are in the header of
  `arpg\scripts\backup-db.ps1`.
