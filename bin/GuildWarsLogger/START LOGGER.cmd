@echo off
cd /d "%~dp0"
set "capture_pid=auto"
set /p "capture_pid=Guild Wars PID (Enter for auto): "
capture_control.exe "%capture_pid%" start
if errorlevel 1 goto failed
echo.
echo Logger initialization completed. The logger runs INSIDE Guild Wars.
echo You can close this command window; recording continues in the game.
echo.
echo To confirm recording: enter an outpost, then run STATUS.cmd and check
echo that a world-*.jsonl file in this folder is growing.
echo Use STOP LOGGER.cmd to stop recording. Closing this window does not stop it.
goto done
:failed
echo.
echo START FAILED. Read the error above; do not assume recording has started.
:done
echo.
pause
