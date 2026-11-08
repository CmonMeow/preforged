Independent inventory capture tool (native Win32).

This package is built and offline-tested; live attachment is not yet verified.
It is separate from the position/spawn logger. Do not load both GWCA collectors
in one client session; coexistence is not verified and the controller refuses
an existing foreign GWCA instance.

From this folder use a specific official client process ID:
inventory_control.exe PID inspect
inventory_control.exe PID start
inventory_control.exe PID status
inventory_control.exe PID stop

The controller verifies the official installation path and pinned executable
hash before loading any DLL. Unsupported builds are rejected. The current
supported locations are Desktop/Guild Wars/Gw.exe and Desktop/gwlauncher/Gw.exe.
No client is selected automatically. Starting does not log in or travel.

The command may exit after readiness; capture continues in the client for up to
30 seconds. Status4 means inventory snapshot ready, with names possibly pending;
status3 means capture finished. Logs are written beside these DLLs. Unloaded
bank panes are not represented as empty. Account databases are never modified.
The DLL remains pinned until the game exits; Stop does not unload it.

Source/build instructions: ../../tools/inventory/README.md.
