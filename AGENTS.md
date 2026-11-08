Production code belongs in src; tests in tests; developer tools in tools; binaries in bin; generated output in build; persistent state in data. Sibling framework, oi, PathEngine and dz are read-only references. Native C++ runtime, one server executable, direct Guild Wars 2022 client connection. Do not add bridge processes.

Official capture sessions are separate from private-server testing. Never close,
restart, or target an official Guild Wars client during private-server build/test
cleanup. Verify both PID and full executable path before any process action;
never terminate all Gw.exe processes by name. Official paths currently include
Desktop/Guild Wars/Gw.exe and Desktop/gwlauncher/Gw.exe. Preserve ongoing capture
sessions for hours while working on the private server. Collector-specific
restart tests require the user's authorization for that test/session.

Coordinate ownership before agents edit shared runtime, account, or CMake files.
Serialize native Release builds with --parallel 1. Before starting a build,
obtain a stable save point from every agent editing included runtime/app sources,
registered tests, or CMake; keep those files frozen until the build finishes.
New detached files and read-only evidence analysis may continue during a build.
Do not publish a skill binding while required effect consumers are unfinished.
Record tested executable/source hashes and distinguish mechanics tests from
rendered client verification. Preserve historical checkpoint reports; keep newer
capture sessions separate from the fixed implementation checkpoint.
Candidate adoption must write fresh file timestamps or force a scoped rebuild.
Copy-Item can preserve a candidate header's old timestamp and leave an existing
test executable stale. Check that changed inputs were actually compiled; record
compiler-read inputs and executable hashes before accepting an AFTER result.
