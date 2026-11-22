# Tasks — m0-server-setup

Bots: off during build/smoke; turned **on (300)** for play once mmaps exist (owner decision 2026-11-22). Bot rules and tuning are m7.

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
- [x] 2.1b Stable = `playerbot` (beta = `playerbot-beta`); local files created (mysql.local.cnf: port 3307, creds after 3.2); dry run clean (2026-11-21). Pick upstream's stable branch; create `arpg\scripts\upstream.local.ps1` and `arpg\scripts\mysql.local.cnf` from the `.example` files; `update-upstream.ps1 -DryRun` runs cleanly
- [x] 2.2 `.vscode/settings.json`: `cmake.configureEnvironment.BOOST_ROOT`, `cmake.configureSettings.OPENSSL_ROOT_DIR` (forward slashes in JSON) — folder was misnamed `,vscode`, fixed; also sets LIB_EAY_*/SSL_EAY_* (repo FindOpenSSL misses `lib/VC/x64/MD`), TOOLS=ON, ELUNA=0
- [x] 2.3 Configure with CMake Tools (kit: Visual Studio Build Tools 2022 – amd64), build RelWithDebInfo. Record any configure/build fixes in PROGRESS log — OK 2026-11-22 (fixes: OpenSSL lib paths; capped parallelism)
- [x] 2.4 `.vscode/tasks.json` (build, unit tests, tool check) and `launch.json` (attach `cppvsdbg` to worldserver/authserver) — 2026-11-21; note `.vscode/` is gitignored upstream, so these stay local

## 3. Data and databases
- [x] 3.1 Run extractors against the client (DBC, maps, vmaps, mmaps — mmaps overnight; creatures need them too, not only bots)
- [x] 3.2 MySQL: create DB user; import release dumps (auth, characters, world, playerbots — playerbots DB still created so the build/config is complete) — user `arpg`, ci-playerbot dumps @ 6a4011fb; world 238 tables/57518 creature_template; realmlist already 127.0.0.1:8085 (2026-11-21)
- [x] 3.3 Configure authserver/worldserver conf (DB strings incl. port, DataDir, SOAP on 127.0.0.1:7878); playerbots conf with **bots disabled**; realmlist 127.0.0.1
- [x] 3.4 Server programs find OpenSSL 3 / libmysql DLLs (copy next to the .exe files if "libcrypto-3-x64.dll not found")

## 4. First run
- [x] 4.1 Start authserver + worldserver; create GM account (level 3); connection-patch client; log in — servers + ARPGADMIN + headless login done (2026-11-22); real client login pending owner — done 2026-11-22 with `Wow-64.exe` (Stormforge exes force Battle.net login)
- [x] 4.2 Create `arpg\scripts\soap.local.ps1`; `gm.ps1 ".server info"` works (ARPGADMIN; smoke 2026-11-22)
- [x] 4.3 Write `restart-world.ps1` (stops/starts worldserver, waits for SOAP; checks the DLLs from 3.4) — written 2026-11-21; first real use in overnight smoke stage
- [x] 4.4 `run-gmtests.ps1 -Filter smoke` — first block passes (the `.arpg version` block passes after m1)
- [ ] 4.5 First full `update-upstream.ps1` run end to end (backup, tag, merge, build, tests)

## 5. Docs
- [ ] 5.1 Verify CODEMAP **?** paths against this fork
- [x] 5.2 Update PROGRESS (Now + Log)
