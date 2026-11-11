@echo off
cd /d "%~dp0"
set "capture_pid=auto"
set /p "capture_pid=Guild Wars PID (Enter for auto): "
capture_control.exe "%capture_pid%" status
echo.
echo This is a one-time status check. You may close this window.
echo State 1 means initializing; state 4 means initialization completed.
echo Enter an outpost and check that the world-*.jsonl file grows.
echo.
echo Capture files in this folder, newest first:
dir /a-d /o-d /t:w "world-*.jsonl" 2>nul
echo.
echo Use STOP LOGGER.cmd to stop recording without closing Guild Wars.
pause
