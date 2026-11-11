@echo off
setlocal
cd /d "%~dp0"
echo Starts BOTH the world/NPC/item/quest collector and incoming packet logger.
echo Use the PID of your OFFICIAL Guild Wars client, not the private client.
set "capture_pid=%~1"
if not defined capture_pid set /p "capture_pid=Official Guild Wars PID: "
if not defined capture_pid exit /b 1
echo(%capture_pid%| findstr /r /x "[0-9][0-9]*" >nul || exit /b 1
"bin\OfficialCapture\capture_control.exe" "%capture_pid%" start
if errorlevel 1 goto failed
"bin\OfficialCapture\packet_control.exe" "%capture_pid%" start
if errorlevel 1 goto partial
echo Both loggers started. You may close this window; recording continues.
echo World logs: bin\OfficialCapture\world-*.jsonl
echo Packet logs: bin\OfficialCapture\packets-*.jsonl
pause
exit /b 0
:partial
echo Packet logger failed. World logger may still be recording.
echo Use STOP CAPTURE.cmd with the same PID to stop both.
goto failed
:failed
echo Capture startup FAILED. Read the error above.
pause
exit /b 1
