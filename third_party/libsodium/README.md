# libsodium dependency snapshot

Headers and the Windows x64 Release static library were copied read-only from the user's oi checkout on 2026-09-25. The supplied headers report 1.0.22; this does not independently establish the provenance of the prebuilt library. `snapshot.json` records the copied input hashes. Only the include tree and the x64 Release library are needed; no original project files or private material are used.

The build now uses this framework-local snapshot and includes its headers/library in the source build identity. The original dependency directory is unchanged. Upstream ISC license: https://github.com/jedisct1/libsodium/blob/master/LICENSE (included as LICENSE).
