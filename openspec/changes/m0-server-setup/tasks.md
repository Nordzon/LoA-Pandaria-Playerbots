# Tasks — m0-server-setup

Bots stay **off** for the whole of m0–m6 (playerbots module built but disabled). They are enabled in m7.

## 1. Tools
- [x] 1.1 Git 2.47, CMake 4.3.2, VS Code, MSVC Build Tools 2022, MySQL 8.0.45, Node 22, OpenSpec CLI 1.10 — OK (2026-11-21)
- [x] 1.2 Boost requirement found: `dep/boost/CMakeLists.txt` → Windows needs ≥ 1.78, static libs, `BOOST_ROOT`. Chosen first: 1.84.0; **changed to 1.87.0** (upstream Windows CI uses 1.87, README says ≥ 1.85)
- [x] 1.3 Boost 1.87.0 installed at `C:\local\boost_1_87_0` (installer MD5 verified vs SourceForge); `BOOST_ROOT` in `.vscode/settings.json` — 46 static libs; smoke build OK (2026-11-21)
- [x] 1.4 OpenSSL 3.0.x full installed at `C:\Program Files\OpenSSL-Win64-3.0` (DLLs in its own bin, not System32); OpenSSL 4.0.0 left untouched for other work — 3.0.22 OK; smoke build linked + ran (2026-11-21). System32 has OpenSSL **3.5.2** DLLs from another app, loaded before PATH → 3.4 is mandatory
- [x] 1.5 Python check passes — 3.14.10 final (install manager updated itself from 3.14.0rc1)
- [x] 1.6 VS Code extensions: cpptools, cmake-tools, sumneko.lua (Claude Code already installed)
- [x] 1.7 `setup-tools.ps1 -WowPath <client>` — "All set." (2026-11-21). Client `D:\WoWMoPProject\MOP-5.4.8.18414-enUS-Repack` (Wow-SF*.exe 18414). Tools moved to `arpg\tools\bin\` (Defender scan clean). Required (HeidiSQL/SQLTools optional: DBeaver is used; MPQ Editor/WDBX can wait until m2/m6)

## 2. Repo and build
- [x] 2.0 Repo cloned to `D:\WoWMoPProject\LoA-Pandaria-Playerbots` (no spaces); kit copied into its root
- [x] 2.1 `git remote -v`: `origin` = **your fork**, `upstream` = https://github.com/1LineAtaTime/LoA-Pandaria-Playerbots.git. — origin = Nordzon fork; upstream added (2026-11-21)
- [x] 2.1a `.gitignore.arpg` lines appended to `.gitignore`; kit committed ("Add ARPG kit", 2026-11-21) as its own commit ("Add ARPG kit")
- [ ] 2.1b Stable = `playerbot` (beta = `playerbot-beta`); local files created (mysql.local.cnf: port 3307, creds after 3.2); dry run needs the 2.1a commit. Pick upstream's stable branch; create `arpg\scripts\upstream.local.ps1` and `arpg\scripts\mysql.local.cnf` from the `.example` files; `update-upstream.ps1 -DryRun` runs cleanly
- [x] 2.2 `.vscode/settings.json`: `cmake.configureEnvironment.BOOST_ROOT`, `cmake.configureSettings.OPENSSL_ROOT_DIR` (forward slashes in JSON) — folder was misnamed `,vscode`, fixed; also sets LIB_EAY_*/SSL_EAY_* (repo FindOpenSSL misses `lib/VC/x64/MD`), TOOLS=ON, ELUNA=0
- [ ] 2.3 Configure with CMake Tools (kit: Visual Studio Build Tools 2022 – amd64), build RelWithDebInfo. Record any configure/build fixes in PROGRESS log
- [x] 2.4 `.vscode/tasks.json` (build, unit tests, tool check) and `launch.json` (attach `cppvsdbg` to worldserver/authserver) — 2026-11-21; note `.vscode/` is gitignored upstream, so these stay local

## 3. Data and databases
- [ ] 3.1 Run extractors against the client (DBC, maps, vmaps, mmaps — mmaps overnight; creatures need them too, not only bots)
- [ ] 3.2 MySQL: create DB user; import release dumps (auth, characters, world, playerbots — playerbots DB still created so the build/config is complete)
- [ ] 3.3 Configure authserver/worldserver conf (DB strings incl. port, DataDir, SOAP on 127.0.0.1:7878); playerbots conf with **bots disabled**; realmlist 127.0.0.1
- [ ] 3.4 Server programs find OpenSSL 3 / libmysql DLLs (copy next to the .exe files if "libcrypto-3-x64.dll not found")

## 4. First run
- [ ] 4.1 Start authserver + worldserver; create GM account (level 3); connection-patch client; log in
- [ ] 4.2 Create `arpg\scripts\soap.local.ps1`; `gm.ps1 ".server info"` works
- [ ] 4.3 Write `restart-world.ps1` (stops/starts worldserver, waits for SOAP; checks the DLLs from 3.4)
- [ ] 4.4 `run-gmtests.ps1 -Filter smoke` — first block passes (the `.arpg version` block passes after m1)
- [ ] 4.5 First full `update-upstream.ps1` run end to end (backup, tag, merge, build, tests)

## 5. Docs
- [ ] 5.1 Verify CODEMAP **?** paths against this fork
- [ ] 5.2 Update PROGRESS (Now + Log)
