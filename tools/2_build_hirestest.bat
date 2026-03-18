@echo off
chcp 65001 > nul
powershell.exe -NoExit -ExecutionPolicy Bypass -File "%~dp0build.ps1" -Sketch test_hiresscroll
pause
