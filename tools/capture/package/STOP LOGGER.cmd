@echo off
cd /d "%~dp0"
set "capture_pid=auto"
set /p "capture_pid=Guild Wars PID (Enter for auto): "
capture_control.exe "%capture_pid%" stop
pause
