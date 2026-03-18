# PotaKB Flash Script (Serial DFU)
# Usage: double-click 3_flash.bat
#        or: .\flash.ps1 -Port COM3
#            .\flash.ps1 -BuildFirst

param(
    [string]$Sketch = "PotaKB_fw",   # PotaKB_fw / test_hiresscroll / etc.
    [string]$Port = "",
    [switch]$BuildFirst
)

$ErrorActionPreference = 'Stop'

$toolsDir   = $PSScriptRoot
$cliExe     = "$toolsDir\arduino-cli.exe"
$configFile = "$toolsDir\arduino-cli.yaml"
$sketchDir  = "$toolsDir\..\program\$Sketch"
$buildDir   = "$toolsDir\.build\$Sketch"
$fqbn       = "Seeeduino:nrf52:xiaonRF52840"

if (-not (Test-Path $cliExe)) {
    Write-Error "arduino-cli.exe not found. Run setup.ps1 first."
}

if ($BuildFirst) {
    Write-Host "Building first..."
    & "$toolsDir\build.ps1"
}

if (-not (Test-Path $buildDir)) {
    Write-Error "No build output found. Run build.ps1 first."
}

# Auto-detect COM port via Windows WMI
if ($Port -eq "") {
    Write-Host "Detecting COM port..."

    $allPorts = Get-WmiObject Win32_SerialPort | Select-Object DeviceID, Description

    # Prefer nRF52840 / XIAO / Adafruit in description
    $found = $allPorts | Where-Object {
        $_.Description -match "nRF52840|XIAO|Adafruit|PotaKB"
    } | Select-Object -First 1

    # Fall back to any USB Serial Device (English or Japanese)
    if (-not $found) {
        $found = $allPorts | Select-Object -First 1
    }

    if ($found) {
        $Port = $found.DeviceID
        Write-Host "  Found: $Port ($($found.Description))"
    } else {
        Write-Host ""
        Write-Host "Available COM ports:"
        if ($allPorts) {
            $allPorts | ForEach-Object { Write-Host "  $($_.DeviceID) - $($_.Description)" }
        } else {
            Write-Host "  (none)"
        }
        Write-Error "Device not found. Specify port manually: .\flash.ps1 -Port COM3"
    }
}

Write-Host ""
Write-Host "Flashing..."
Write-Host "  Port : $Port"
Write-Host "  FQBN : $fqbn"

& $cliExe upload `
    --config-file $configFile `
    --fqbn        $fqbn `
    --port        $Port `
    --input-dir   $buildDir `
    $sketchDir

if ($LASTEXITCODE -ne 0) {
    Write-Error "Flash failed (exit code: $LASTEXITCODE)"
}

Write-Host ""
Write-Host "=== Flash complete! ==="
