@echo off
title TamaPoke 3.13.3 local installer
cd /d "%~dp0"
echo Starting installer at http://localhost:8001
echo Close Arduino Serial Monitor before flashing.
echo Leave Erase device UNCHECKED when updating.
echo.
start "" "http://localhost:8001/"
python -m http.server 8001
pause
