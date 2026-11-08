@echo off
cd /d "%~dp0"
set "logger_pid=auto"
set /p "logger_pid=Guild Wars PID (Enter for auto): "
packet_control.exe "%logger_pid%" status
pause
