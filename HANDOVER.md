# Handover → Claude Code (2026-11-21)

Planning happened in a claude.ai chat that Claude Code can't read. Everything needed to continue is in this repo.
**Start here, then follow the session protocol in `CLAUDE.md`.**

## What this project is
A single-player Mists of Pandaria 5.4.8 server (fork of LoA-Pandaria-Playerbots) with a Diablo II-style layer:
affix loot, upward-only creature scaling, Champions/Superuniques in every zone, Paragon after 90, party-size
dungeon scaling, a loot pet + loot filter, all professions on one character. Bots are built but **off until the last milestone (m7)**.

## Where things are
| File | Role |
| --- | --- |
| `CLAUDE.md` | Rules, read order, session protocol, commands. Read every session. |
| `docs/PROGRESS.md` | **The one tracking file.** "Now" = where we are; Log = what happened. Update every session. |
| `docs/SPEC.md` | **What we build** — canonical spec with all decisions and numbers. |
| `openspec/changes/<id>/` | Per-change proposal + tasks (checklists) + spec deltas. The active change's `tasks.md` is the to-do list. |
| `docs/CODEMAP.md` | What's where in the code; every upstream file we touched. |
| `docs/TESTING.md` | Test layers, `.gmtest` format, GM command cheat sheet. |
| `arpg/scripts/` | `setup-tools.ps1`, `gm.ps1` (SOAP), `run-gmtests.ps1`, `update-upstream.ps1` (+ `.example` local configs) |

## Current state
- Active change: **m0-server-setup** — see `openspec/changes/m0-server-setup/tasks.md` (ticked items are done).
- Machine: Windows, repo at `D:\WoWMoPProject\LoA-Pandaria-Playerbots`. Installed: Git 2.47, CMake 4.3.2, MSVC Build Tools 2022,
  MySQL 8.0.45 (Server only), Node 22, OpenSpec CLI 1.10, Python 3.14 (Python install manager, under `WindowsApps` — it's real), DBeaver.
- In progress: OpenSSL 3.0 full → `C:\Program Files\OpenSSL-Win64-3.0` (OpenSSL 4.0.0 also present for other work — don't use it);
  Boost 1.84.0 → `C:\local\boost_1_84_0`; `.vscode/settings.json` sets `BOOST_ROOT` (configureEnvironment) and `OPENSSL_ROOT_DIR`
  (configureSettings) with forward slashes.
- Boost facts from `dep/boost/CMakeLists.txt`: Windows needs ≥ 1.78, `Boost_USE_STATIC_LIBS ON`, reads env `BOOST_ROOT`.
- SSH: Git configured to use Windows OpenSSH; github.com host key accepted.
- Not yet verified: whether `origin` is the owner's fork (task 2.1).

## Milestones (bots last)
0 server setup (bots off) → 1 module skeleton + world scaling (+1b all professions) → 2 affix loot + MPQ spike (+2b loot pet & filter)
→ 3 Champions & Superuniques → 4 Paragon + ArpgUI → 5 dungeon scaling → 6 client patch → 7 bots.

## Owner preferences (from the planning chat)
- Wants all work tracked in markdown so any new session can resume; keep PROGRESS.md current.
- Some C++ experience; Claude writes most code; explain build/debug steps plainly.
- Concise answers; Windows/PowerShell commands; no Docker/WSL.
- Prefers keeping this project isolated from work tooling (no global PATH changes; project-local settings where possible).

## First prompt to use in Claude Code
> Read HANDOVER.md, CLAUDE.md, docs/PROGRESS.md and openspec/changes/m0-server-setup/tasks.md.
> Continue m0-server-setup from the first unticked task. Update PROGRESS.md as you go.
