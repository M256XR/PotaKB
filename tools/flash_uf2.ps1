# PotaKB UF2 Flash Script (bootloader drag-and-drop)
# Use this if serial DFU does not work.
#
# Steps:
#   1. Double-click the reset button on XIAO -> XIAO_BOOT drive appears
#   2. double-click 3_flash_uf2.bat

$ErrorActionPreference = 'Stop'

$toolsDir = $PSScriptRoot
$buildDir = "$toolsDir\.build"

$uf2 = Get-ChildItem -Path $buildDir -Filter "*.uf2" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $uf2) {
    Write-Error "No .uf2 file found. Run build.ps1 first."
}
Write-Host "UF2: $($uf2.FullName)"

Write-Host "Waiting for XIAO_BOOT drive..."
Write-Host "(Double-click XIAO reset button if not yet done)"

$bootDrive = $null
for ($i = 0; $i -lt 30; $i++) {
    $bootDrive = Get-Volume | Where-Object {
        $_.FileSystemLabel -match "XIAO" -or $_.FileSystemLabel -eq "BOOT"
    } | Select-Object -First 1

    if ($bootDrive) { break }
    Write-Host "  Waiting... ($($i+1)/30s)" -NoNewline
    Write-Host "`r" -NoNewline
    Start-Sleep 1
}

if (-not $bootDrive) {
    Write-Error "BOOT drive not found. Double-click the reset button and try again."
}

$dest = "$($bootDrive.DriveLetter):\"
Write-Host "  Found: $dest ($($bootDrive.FileSystemLabel))"
Write-Host "Copying .uf2..."
Copy-Item -Path $uf2.FullName -Destination $dest -Force

Write-Host ""
Write-Host "=== Flash complete! Device is rebooting. ==="
