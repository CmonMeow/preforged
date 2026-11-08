@echo off
setlocal
title Guild Wars Skills Logger - Read Only
cd /d "%~dp0"
echo Guild Wars Skills Logger
echo Keep this window open. Ctrl+C stops only the logger.
echo Supply the PID of your official Guild Wars client.
echo Private project clients and unverified builds are rejected.
echo Verified static skill constants are also captured once per client build.
echo.
set "SKILLS_OFFICIAL_PID=%~1"
if not defined SKILLS_OFFICIAL_PID set /p "SKILLS_OFFICIAL_PID=Official Gw.exe PID: "
if not defined SKILLS_OFFICIAL_PID (
  echo No PID supplied. Nothing was attached.
  pause
  exit /b 1
)
"%~dp0skills_logger.exe" "%SKILLS_OFFICIAL_PID%" --dump-constants
set "SKILLS_EXIT_CODE=%ERRORLEVEL%"
echo.
echo Logger exited with code %SKILLS_EXIT_CODE%. Guild Wars was not changed.
pause
exit /b %SKILLS_EXIT_CODE%
