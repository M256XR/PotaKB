# PotaKB Build Environment Setup
# Run this script once on a new PC.
# Usage: double-click 1_setup.bat

$ErrorActionPreference = 'Stop'

$toolsDir   = $PSScriptRoot
$cliExe     = "$toolsDir\arduino-cli.exe"
$configFile = "$toolsDir\arduino-cli.yaml"
$dataDir    = "$toolsDir\.arduino-data"
$userDir    = "$toolsDir\.arduino-user"

# -------------------------------------------
# 1. Download arduino-cli
# -------------------------------------------
if (-not (Test-Path $cliExe)) {
    Write-Host "[1/4] Downloading arduino-cli..."
    $url = "https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip"
    $zip = "$toolsDir\_tmp.zip"
    Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing
    Expand-Archive -Path $zip -DestinationPath $toolsDir -Force
    Remove-Item $zip
    Write-Host "  -> arduino-cli.exe ready"
} else {
    Write-Host "[1/4] arduino-cli.exe already exists. Skipping."
}

# -------------------------------------------
# 2. Write config file (keep everything local)
# -------------------------------------------
Write-Host "[2/4] Writing config file..."

$dataDirFwd = $dataDir -replace '\\', '/'
$userDirFwd = $userDir -replace '\\', '/'

$yaml = "board_manager:`n  additional_urls:`n  - https://adafruit.github.io/arduino-board-index/package_adafruit_index.json`n  - https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json`ndirectories:`n  data: $dataDirFwd`n  user: $userDirFwd`nlogging:`n  format: text`n  level: warn`n"
[System.IO.File]::WriteAllText($configFile, $yaml, [System.Text.Encoding]::ASCII)
Write-Host "  -> $configFile"

# -------------------------------------------
# 3. Install cores
# -------------------------------------------
Write-Host "[3/4] Installing board cores (this may take a few minutes)..."
& $cliExe --config-file $configFile core update-index
& $cliExe --config-file $configFile core install adafruit:nrf52
& $cliExe --config-file $configFile core install Seeeduino:nrf52

# -------------------------------------------
# 4. Install libraries
# -------------------------------------------
Write-Host "[4/4] Installing libraries..."
& $cliExe --config-file $configFile lib install "Adafruit MCP23017 Arduino Library"

Write-Host ""
Write-Host "=== Setup complete! ==="
Write-Host "Next: double-click 2_build.bat"
Write-Host "Then: double-click 3_flash.bat"
