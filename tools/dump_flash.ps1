$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$openocdRoot = Get-ChildItem "$env:LOCALAPPDATA\stlink-tool\cache" -Recurse -Filter openocd.exe | Select-Object -First 1 | ForEach-Object { Split-Path (Split-Path $_.FullName -Parent) -Parent }
if (-not $openocdRoot) { throw 'OpenOCD not found. Run tools\stlink-tool-windows-x64.exe once.' }
$openocd = Join-Path $openocdRoot 'bin\openocd.exe'
$scripts = Join-Path $openocdRoot 'scripts'
$backupDir = Join-Path $root 'backup'
New-Item -ItemType Directory -Force $backupDir | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$image = Join-Path $backupDir "original-$stamp.bin"
& $openocd -s $scripts -f interface/stlink.cfg -f target/stm32f1x.cfg -c "transport select swd; adapter speed 950; init; reset halt; dump_image {$image} 0x08000000 0x10000; verify_image {$image} 0x08000000 bin; reset run; shutdown"
if ($LASTEXITCODE -ne 0) { throw 'Flash read/verification failed; nothing was erased.' }
$hash = Get-FileHash -Algorithm SHA256 $image
$hash | Format-List
$hash.Hash | Set-Content "$image.sha256"
Write-Host "Backup: $image"
