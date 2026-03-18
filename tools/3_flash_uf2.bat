@echo off
chcp 65001 > nul
powershell.exe -NoExit -ExecutionPolicy Bypass -File "%~dp0flash_uf2.ps1"
pause
