param([string]$Port = "COM3", [int]$Baud = 115200)

$sp = New-Object System.IO.Ports.SerialPort $Port, $Baud
$sp.ReadTimeout = 3000
$sp.DtrEnable = $true
$sp.RtsEnable = $false
$sp.Open()
Write-Host "=== Serial Monitor $Port $Baud ==="
Write-Host "Ctrl+C to exit"

try {
    while ($true) {
        try {
            $line = $sp.ReadLine()
            Write-Host $line
        } catch [System.TimeoutException] {
            # just retry
        }
    }
} finally {
    $sp.Close()
}
