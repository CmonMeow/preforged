@echo off
cd /d "%~dp0"
set /p "OBSERVER_CAPTURE_PID=Official Guild Wars PID: "
observer_capture.exe "%OBSERVER_CAPTURE_PID%" 3600
pause
