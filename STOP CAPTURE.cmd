@echo off
setlocal
cd /d "%~dp0"
set "capture_pid=%~1"
if not defined capture_pid set /p "capture_pid=Official Guild Wars PID: "
if not defined capture_pid exit /b 1
echo(%capture_pid%| findstr /r /x "[0-9][0-9]*" >nul || exit /b 1
set "capture_failed=0"
"bin\PacketLogger\packet_control.exe" "%capture_pid%" stop
if errorlevel 1 set "capture_failed=1"
"bin\GuildWarsLogger\capture_control.exe" "%capture_pid%" stop
if errorlevel 1 set "capture_failed=1"
echo Stop requests completed. Read any errors above. Guild Wars was not closed.
pause
exit /b %capture_failed%
