# Progress

The single source of truth for where work stands. Every session starts by reading **Now** and ends by
updating **Now** and adding a **Log** line. If this file and memory disagree, this file wins.

## Now
- **Active change:** `m0-server-setup` (see `openspec/changes/m0-server-setup/tasks.md` for the checklist)
- **Where we stopped (2026-11-21):** section 1 (tools) done — setup-tools "All set.". Remotes, `.vscode/settings.json`,
  local config files, gitignore done. First configure + build (2.3) started into `build/`. Kit committed.
- **Next step:** check the 2.3 build result (log: `build/arpg-first-build.log`)
  → `update-upstream.ps1 -DryRun` (2.1b) → section 3 (extractors, DBs).
- **Blockers:** none.
- **Bots:** disabled until m7.

## Roadmap (one change folder each; propose the backlog ones from the spec Doc when their turn comes)
| # | Change | Status |
| --- | --- | --- |
| 0 | m0-server-setup (bots off) | in progress |
| 1 | m1-world-scaling (module skeleton, `.arpg` commands, test harness, scaling) | proposed |
| 1 | m1b-all-professions | proposed |
| 2 | m2-affix-loot (affix pool v1 is in the spec Doc; + MPQ-loading spike) | backlog |
| 2 | m2b-loot-pet-filter | proposed |
| 3 | m3-champions-superuniques (a different Superunique respawns 5 min after a kill) | backlog |
| 4 | m4-paragon-addon | backlog |
| 5 | m5-dungeon-scaling (party size 1–5, leader's level, difficulty knob) | backlog |
| 6 | m6-client-patch (native affix lines + names, Uniques, charms, race/class combos) | backlog |
| 7 | m7-bots (enable bots, verify bot rules, Dungeon Finder hook, optional crowding) | proposed |
| – | multiplayer with friends (client updater, Tailscale, group rules) | idea |
| – | loot pet gathers herbs/ore | idea |

## Upstream sync
- Branch tracked: `playerbot` (beta: `playerbot-beta`, cherry-pick only) · Last sync: _(none yet)_ · Cadence: every 2–4 weeks, or when a fix we need lands

## Environment facts (keep current)
| Item | Value |
| --- | --- |
| Repo | `D:\WoWMoPProject\LoA-Pandaria-Playerbots` |
| MySQL | 8.0.45, Server only, service `MySQL80`, port **3307**, StartType Manual |
| OpenSSL for the build | `C:/Program Files/OpenSSL-Win64-3.0` (3.0.22 full; libs in `lib/VC/x64/MD`); 4.0.0 in `OpenSSL-Win64` for other work — don't use. System32 has OpenSSL 3.5.2 DLLs (other app) → copy 3.0 DLLs next to server exes |
| Boost | 1.87.0 at `C:/local/boost_1_87_0` (= upstream CI; static libs; `BOOST_ROOT` only in `.vscode/settings.json`). 1.84 at `C:/local/boost_1_84_0` unused |
| Python | 3.14.10 via Python install manager (`py`) |
| DB GUI | DBeaver |
| WoW client | `D:\WoWMoPProject\MOP-5.4.8.18414-enUS-Repack` (repack; exes `Wow-SF.exe`/`Wow-SF-64.exe`), 5.4.8 build 18414 enUS |
| Toolchain | MSVC 14.44 (VS Build Tools 17.14), Windows SDK 10.0.22621 + 10.0.26100 |
| Machine | 24 logical CPUs, 32 GB RAM, fixed 26 GB pagefile. Build capped: `cmake --build build --config RelWithDebInfo -- /m:2 /p:CL_MPCount=6 /nodeReuse:false` (`--parallel` → out of memory, C3859/C1076) |
| Git remotes | origin = `git@github.com:Nordzon/LoA-Pandaria-Playerbots.git`, upstream = 1LineAtaTime (https) |
| SSH | Git uses Windows OpenSSH (`core.sshCommand`); github.com host key accepted |

## Log (newest first, one line each)
- 2026-11-21 — First configure OK (OpenSSL 3.0.22, Boost 1.87, MySQL, Playerbots). First build `--parallel` failed: out of memory (C3859 PCH); rerun capped at /m:2 × /MP 6.
- 2026-11-21 — Tools verified (sigs/hashes, Defender scan, Boost+OpenSSL+MySQL smoke build). Decision: Boost 1.84 → 1.87.0 (upstream CI); SPEC updated. Fixed `,vscode`→`.vscode`, OpenSSL lib paths, setup-tools.ps1 (OpenSSL/Boost/client detection), non-ASCII in update-upstream.ps1 (PS 5.1 parse error); tools → `arpg/tools/bin`; upstream remote added.
- 2026-11-21 — Handover to Claude Code. Bots moved to the end (m7). Boost 1.84 chosen. setup-tools.ps1 Python detection fixed.
- 2026-11-21 — Repo cloned, kit copied in. Tools check: Git, CMake, MSVC, MySQL 8.0.45, Node, OpenSpec OK.
- 2026-11-18 — Affix pool v1 drafted in the spec Doc; loot pet + filter, all professions, 5-min Superunique respawn added.
- 2026-11-18 — Added update-upstream.ps1 and the upstream sync rules.
- 2026-11-15 — Spec agreed; kit created (setup script, OpenSpec layout, CODEMAP, TESTING, SOAP test runner).
