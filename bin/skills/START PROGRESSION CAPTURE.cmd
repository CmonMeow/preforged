@echo off
setlocal
title Guild Wars Progression Capture - Read Only
cd /d "%~dp0"
echo Captures your character's title records, encoded names and faction counters.
echo This reads memory only. Keep Guild Wars and this window open.
echo No login, injection, chat hook or server account changes are performed.
echo.
set "GW_PROGRESSION_PID=%~1"
if not defined GW_PROGRESSION_PID set /p "GW_PROGRESSION_PID=Official Gw.exe PID: "
if not defined GW_PROGRESSION_PID exit /b 1
"%~dp0skills_logger.exe" "%GW_PROGRESSION_PID%" --progression-only --once
set "GW_PROGRESSION_EXIT=%ERRORLEVEL%"
echo.
echo Capture exited with code %GW_PROGRESSION_EXIT%. Output is beside skills_logger.exe.
pause
exit /b %GW_PROGRESSION_EXIT%
