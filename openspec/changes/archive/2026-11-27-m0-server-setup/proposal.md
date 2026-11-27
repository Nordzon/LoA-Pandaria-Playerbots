# m0-server-setup

## Why
Nothing else can be built or tested until the unmodified core runs locally with bots and the test harness can talk to it.

## What changes
- Fork LoA-Pandaria-Playerbots; build it natively on Windows (MSVC, CMake, RelWithDebInfo).
- Databases loaded from the release dump; client data extracted (maps, vmaps, mmaps, DBC).
- GM account; SOAP enabled on 127.0.0.1 for scripts.
- Scripts: `restart-world.ps1`; `gm.ps1` and `run-gmtests.ps1` verified against the live server.
- VS Code: CMake Tools kit, build task, `cppvsdbg` attach-to-worldserver launch config.
- CODEMAP: verify every **?** path.

## Machine state (2026-11-21)
Repo at `D:\WoWMoPProject\LoA-Pandaria-Playerbots`. Installed: Git 2.47, CMake 4.3.2, MSVC Build Tools 2022,
MySQL 8.0.45 (Server only), Node 22, OpenSpec CLI 1.10, Python 3.14 (install manager), DBeaver.
In progress: OpenSSL 3.0 full → `C:\Program Files\OpenSSL-Win64-3.0`; Boost 1.87.0 → `C:\local\boost_1_87_0` (changed from 1.84 on 2026-11-21 to match upstream CI).

## Out of scope
Any gameplay change. No behaviour spec deltas (infrastructure only).

## Done when
You log in on 127.0.0.1 with bots disabled, and `run-gmtests.ps1 -Filter smoke` passes its first block.
Bots are enabled only in m7.
