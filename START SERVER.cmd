@echo off
cd /d "%~dp0"
set "FW_GW_PROBE_EMPTY_MANIFEST=1"
set "FW_GW_PROBE_PLAYER_VIEW=1"
set "FW_GW_PROBE_MAP_EXPLORATION=1"
bin\server_host.exe
if errorlevel 1 pause
