@echo off
setlocal
cd /d "%~dp0"
if not exist "client_launcher.exe" (
  echo Client launcher missing. Build the project first.
  pause
  exit /b 1
)
echo Start START SERVER.cmd first and keep its console open.
echo Private login: test@example.test / test
client_launcher.exe
if errorlevel 1 pause
