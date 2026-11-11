@echo off
cd /d "%~dp0"
echo Start the logger first and leave chat closed.
echo This tries every map ID from 13 through 882 in order, with 3-5 second delays.
echo Focus Guild Wars during the five-second countdown.
where py >nul 2>nul
if errorlevel 1 goto python
py -3 auto_tp.py
goto done
:python
python auto_tp.py
:done
pause
