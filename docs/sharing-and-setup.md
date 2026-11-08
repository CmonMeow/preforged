# Sharing and local setup

Preforged is an unfinished community private-server development project. It is not affiliated with or endorsed by ArenaNet. The repository contains server code, synthetic test fixtures, and limited third-party development dependencies. It does not include a playable Guild Wars client or `Gw.dat`.

## Requirements

- Windows 10 or 11, x64.
- Visual Studio 2022 or later with the Desktop development with C++ workload and Windows SDK.
- CMake 3.24 or later.
- A legally obtained, compatible Guild Wars 2022 client and its `Gw.dat` archive for local client testing. Each player supplies these files; keep them local and out of Git.

## Build and run

From the repository root:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Run `START SERVER.cmd` and keep its console open. Run `START CLIENT.cmd` in a second window. Place the compatible client and the player-supplied `Gw.dat` under `bin/client` locally. `.gitignore` excludes `Gw.dat` regardless of which project folder it is placed in. The local test account is `test@example.test` with password `test`. The server stores account and game state under `data/` and writes logs under `data/logs/`.

The capture scripts are optional developer tools. `START CAPTURE.cmd` targets an explicitly supplied official-client PID and starts both capture DLLs. It is not needed to run the server or private client. Read each logger's README and provenance before use. Never use a private test-client PID for official capture.

## Repository contents and privacy

Git intentionally excludes `/bin/`, `/build/`, and `/data/`. These folders can contain executables, the multi-gigabyte client archive, DLLs, account databases, character data, private captures, logs, and generated reports. Do not commit them. Keep credentials, account imports, crash dumps, and raw logs out of issues and pull requests unless redacted and explicitly reviewed.

`third_party/` contains dependency snapshots and provenance. Review each dependency's license and upstream terms before redistributing binaries or source. Guild Wars names, client files, assets, and trademarks remain their respective owners' property. Do not bundle proprietary client files, a DAT archive, modified game executables, or account data in releases.

Contributions should include focused changes, tests where appropriate, and evidence for any behavior described as verified. Client-visible behavior must be distinguished from native simulation tests. The full server and custom remote client transport are not complete; avoid describing the project as production-ready.
