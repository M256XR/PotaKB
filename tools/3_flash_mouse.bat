@echo off
chcp 65001 > nul
powershell.exe -NoExit -ExecutionPolicy Bypass -File "%~dp0flash.ps1" -Sketch PotaKB_mouse
pause
