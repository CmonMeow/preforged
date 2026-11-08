@echo off
cd /d "%~dp0"
set "logger_pid=auto"
set /p "logger_pid=Guild Wars PID (Enter for auto): "
packet_control.exe "%logger_pid%" start
if errorlevel 1 goto failed
echo Recording runs inside Guild Wars. This window may be closed.
echo Logs are packets-*.jsonl in this folder.
goto done
:failed
echo Logger failed to start. Read the error above.
:done
pause
