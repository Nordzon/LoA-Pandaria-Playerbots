# Pandaria ARPG starter kit

Copy everything here into the root of your fork of LoA-Pandaria-Playerbots, then:

1. Append `.gitignore.arpg` to `.gitignore` and delete it.
2. In an elevated PowerShell: `powershell -ExecutionPolicy Bypass -File arpg\scripts\setup-tools.ps1 -Install -AddDefenderExclusion -WowPath "<your 5.4.8 client>"`
   Re-run without `-Install` until every row is OK (MANUAL rows are quick downloads).
3. Open the folder in VS Code and ask Claude: "Start change m0-server-setup."

| File | Purpose |
| --- | --- |
| `HANDOVER.md` | Start here: context from the planning chat, current state, first prompt for Claude Code |
| `CLAUDE.md` | Rules, session protocol and read order for Claude in every session |
| `docs/SPEC.md` | Canonical spec: every decision and starting number |
| `docs/CODEMAP.md` | What's where; what we touched upstream |
| `docs/PROGRESS.md` | Now / roadmap / one-line log |
| `docs/TESTING.md` | The four test layers and the GM command cheat sheet |
| `openspec/` | OpenSpec layout: `specs/` (current behaviour), `changes/` (proposals) |
| `arpg/scripts/setup-tools.ps1` | Check or install every tool |
| `arpg/scripts/gm.ps1` | Send one GM command over SOAP |
| `arpg/scripts/run-gmtests.ps1` | Run `arpg/tests/*.gmtest` |
| `arpg/scripts/update-upstream.ps1` | Safely take upstream fixes: backup, tag, merge, build, test, rollback info |

The OpenSpec CLI (`npm install -g @fission-ai/openspec@latest`, needs Node 20.19+) is optional. It adds
`openspec list`, `openspec validate` and Claude Code slash commands, but the folder works as plain markdown.
