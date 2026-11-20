@echo off
if exist "%~dp0GooPadCompanion.exe" (
    start "" "%~dp0GooPadCompanion.exe" %*
) else (
    py -3 "%~dp0GooPadCompanion.py" --gui %*
)
