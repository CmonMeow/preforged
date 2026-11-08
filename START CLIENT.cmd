@echo off
cd /d "%~dp0"
bin\client\client_launcher.exe
if errorlevel 1 pause
