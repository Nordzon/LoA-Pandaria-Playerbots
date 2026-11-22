# Pandaria ARPG — working rules for Claude

Single-player MoP 5.4.8 server (LoA-Pandaria-Playerbots fork) with a Diablo-style layer:
affix loot, upward-only creature scaling, Champions/Superuniques, Paragon, party-size dungeon scaling.
Native Windows build (MSVC Build Tools 2022 + CMake + VS Code). Bots: on for play (300, `start-stack.ps1`), off for ARPG feature tests (`write-confs.ps1 -Bots off`); bot rules + 1000 bots in m7.
Spec: `docs/SPEC.md` (canonical). New here? Read `HANDOVER.md` once.

## Session protocol (so any session can pick up where the last one stopped)
**Start:** read, in this order (stable → volatile, so sessions reuse cached context):
1. This file.
2. `docs/CODEMAP.md` — where things are, which upstream files we touched. Read before searching the tree.
3. `docs/PROGRESS.md` — the **Now** section: active change, where we stopped, next step, blockers.
4. The active change: `openspec/changes/<id>/proposal.md`, `design.md`, `tasks.md`.
5. `docs/SPEC.md` — only the section for the feature you're working on.
Don't grep the whole core when CODEMAP answers it. If CODEMAP is wrong or missing something, fix it.

**During:** tick `- [x]` in the change's `tasks.md` as each task is done. If you learn a fact about the machine
(paths, versions, ports) put it in PROGRESS "Environment facts". If a decision changes, update `docs/SPEC.md` and say so in the Log.

**End (also before any long-running step or when the user stops):** update PROGRESS **Now** (where we stopped,
exact next step, blockers) and add one Log line. A stale PROGRESS is the worst failure here — the next session starts from it.

## Autonomous work and the scheduler (owner rule, 2026-11-22: applies to every session)
The owner wants work to keep going without him. Every session SHALL:
1. **Long-running steps go through a resumable script, never a foreground command:** builds, extraction, server
   start/stop, test runs. Use `arpg\scripts\overnight.ps1` (stages with `.done` markers in `D:\WoWMoPProject\ServerData\pipeline\`,
   `status.json`, `report.md`) or a script in the same style. Launch it detached (`Start-Process pwsh … -WindowStyle Minimized`)
   so it survives the session and Claude running out of credits.
2. **Schedule check-ins at the start of any autonomous stretch** with CronCreate (jobs are session-only: re-create them
   every session; they die when Claude exits). Default: every 30 min at :13/:43 while work runs, plus one wrap-up before the
   owner is back. A check-in reads status/logs, fixes a failed stage (delete its `.done`, relaunch) and logs one line in PROGRESS;
   when the run is finished it updates PROGRESS + tasks and deletes its own job. Keep check-ins cheap.
3. **Don't block on the owner.** Keep moving on work that doesn't need a running build (spec, SQL, `Core/` logic + unit tests),
   and put open questions in PROGRESS "Blockers" instead of waiting.
4. **Never run two builds in `build\` at once** (file locks). Wait for a running build; `--target arpg_unittests` is the one
   exception (separate project, no shared outputs).
5. The PC sleeps after 10 min idle: scripts that must run unattended call `SetThreadExecutionState` (see `overnight.ps1`).
   No power-setting changes.

## Workflow (OpenSpec-style)
- No feature code without a change folder: `openspec/changes/<id>/` with `proposal.md` (why, scope),
  `tasks.md` (checklist), optional `design.md`, and spec deltas in `specs/<capability>/spec.md`
  (`## ADDED|MODIFIED|REMOVED Requirements`, each `### Requirement:` uses SHALL and has ≥1 `#### Scenario:` with WHEN/THEN).
- Implement task by task; tick `- [x]` as each is done.
- When a change is complete and tested: merge its deltas into `openspec/specs/<capability>/spec.md`,
  move the folder to `openspec/changes/archive/YYYY-MM-DD-<id>/`, update PROGRESS.
- Every scenario gets a test (see `docs/TESTING.md`): unit test, `.gmtest`, or a written manual check.

## Commits (owner 2026-11-22: "a nice commit every so often")
- Work happens on branch **`arpg`** (never commit to `playerbot`; upstream merges follow "Staying in sync" below).
- **Commit after every green full run** (`rebuild-and-test.ps1` ends `ALL OK`) that finished a task or a meaningful step, and at
  the end of a session if the tree is green. Never commit a red tree; if a run fails, fix first.
- One commit per finished step, message: `<change-id>: <what, in plain words>` + a short body (what/why, tests: "N GM files pass,
  M unit tests"). Logical splits are welcome (hooks / module / tests / docs) when a step touched many areas.
- Never commit secrets (`*.local.*`, passwords) or binaries (client patch MPQs, DLLs, DBCs); check `git status` first.
- **Push after each commit** to the owner's fork: `git push origin arpg` (owner 2026-11-22: "I don't want to lose this").
  Never push to `upstream` (1LineAtaTime) and never force-push.

## Code rules
- Our code lives in `modules/mod_arpg/`. Touch upstream `src/` only to add a missing hook; keep it a tiny,
  isolated patch, mark it `// ARPG:` and list it in CODEMAP "Upstream touches".
- Numbers go in `arpg.conf` or SQL tables (`arpg_*`), never hard-coded. `.arpg reload` must pick them up.
- Pure logic (rolls, curves, multipliers) goes in `modules/mod_arpg/src/Core/` with no game headers, so it is unit-testable.
- Every debug command must work over SOAP: take an explicit player name or GUID, never rely on a selected target.
- SQL changes: idempotent files in `modules/mod_arpg/sql/{world,characters}/`, named `YYYY_MM_DD_NN_<what>.sql`.

## Commands
- Build: `cmake --build build --config RelWithDebInfo -- /m:2 /p:CL_MPCount=6 /nodeReuse:false` (or F7 in VS Code).
  Never `--parallel`/uncapped: 24 projects x 24 compilers runs out of memory on the game PCH (C3859/C1076).
  Configure once with CMake Tools (settings in `.vscode/settings.json`).
- Restart world server: `arpg\scripts\restart-world.ps1` (to be written in m0).
- Send a GM command: `arpg\scripts\gm.ps1 ".server info"`
- Run GM tests: `arpg\scripts\run-gmtests.ps1 [-Filter name]`
- Unit tests: `ctest --test-dir build -C RelWithDebInfo -R arpg`
- Tool check: `arpg\scripts\setup-tools.ps1`
- Upstream sync: `arpg\scripts\update-upstream.ps1 [-DryRun] [-CherryPick <hash>]`

## Staying in sync with upstream
We are a fork of LoA-Pandaria-Playerbots. Remote `upstream` = their repo; we track their **stable** branch
(set in `arpg/scripts/upstream.local.ps1`), never beta for day-to-day play.
- Sync: `arpg\scripts\update-upstream.ps1` (`-DryRun` first to see incoming commits and risk flags).
  It backs up the databases, tags `pre-sync-<timestamp>`, merges with `--no-ff`, builds, and runs unit and GM tests.
- One fix from beta: `update-upstream.ps1 -CherryPick <hash>`.
- Always merge, never rebase our `main`.
- On conflicts: keep upstream's version of their code, then re-apply our `// ARPG:` hook on top. Update the
  CODEMAP "Upstream touches" row if the hook moved.
- If tests fail after a sync: find which upstream commit caused it (`git log pre-sync-<ts>..HEAD -- <path>`),
  fix on our side (preferably inside `mod_arpg`), or roll back with the commands the script prints.
- After a sync: add the script's PROGRESS log line.

## After each session
Update `docs/CODEMAP.md` (new files, hooks used, upstream touches) and `docs/PROGRESS.md` (Now / Log).
Keep both short: CODEMAP is a map, not documentation; PROGRESS log entries are one line each.

## Working with the owner
Windows + PowerShell commands only (no WSL/Docker). Some C++ experience: explain build/debug steps plainly.
Keep this project isolated from his work tooling: no global PATH or system changes without asking; prefer project-local
settings (`.vscode/settings.json`, CMake cache, `*.local.*` files).
