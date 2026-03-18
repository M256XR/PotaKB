# PotaKB Build Script
# Usage: double-click 2_build.bat

param(
    [string]$Sketch = "PotaKB_fw",   # PotaKB_fw / test_hiresscroll / etc.
    [switch]$Verbose
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

Write-Host "Building PotaKB_fw..."
Write-Host "  FQBN  : $fqbn"
Write-Host "  Sketch: $sketchDir"

$args = @(
    "compile",
    "--config-file", $configFile,
    "--fqbn",        $fqbn,
    "--build-path",  $buildDir,
    $sketchDir
)
if ($Verbose) { $args += "--verbose" }

& $cliExe @args

if ($LASTEXITCODE -ne 0) {
    Write-Error "Compile failed (exit code: $LASTEXITCODE)"
}

Write-Host ""
Write-Host "=== Build successful! ==="

$uf2 = Get-ChildItem -Path $buildDir -Filter "*.uf2" -ErrorAction SilentlyContinue | Select-Object -First 1
if ($uf2) {
    Write-Host "UF2: $($uf2.FullName)"
    Write-Host "     (You can also flash by double-clicking reset and copying this file to XIAO_BOOT drive)"
}
