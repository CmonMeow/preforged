Guild Wars capture package

This folder contains the combined world/NPC/item/quest collector and incoming
packet logger. Start them with the project-root START CAPTURE.cmd and stop them
with STOP CAPTURE.cmd. Logs are written beside these binaries in this folder.

This build accepts valid 32-bit Guild Wars images and discovers the relevant
client structures through code fingerprints and runtime layout checks instead
of a fixed PE timestamp allowlist. The patched November 2026 executable has
one matching packet-handler fingerprint on disk. Live attachment to this build
has not been verified; inspect the diagnostic log if initialization refuses.

Do not overwrite the DLLs while a logger is loaded in a game process. The
controller refuses to operate a collector loaded from another folder. The
logger does not launch, close, or restart Guild Wars.

GWCA runtime provenance and its upstream license are included. The standalone
GWToolboxdll.dll is a complete Toolbox plugin, not a drop-in GWCA runtime, and
is not loaded by this package.
