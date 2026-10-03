param([Parameter(Mandatory=$true)][string]$Image)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$openocdRoot = Get-ChildItem "$env:LOCALAPPDATA\stlink-tool\cache" -Recurse -Filter openocd.exe | Select-Object -First 1 | ForEach-Object { Split-Path (Split-Path $_.FullName -Parent) -Parent }
if (-not $openocdRoot) { throw 'OpenOCD not found. Run tools\stlink-tool-windows-x64.exe once.' }
$openocd = Join-Path $openocdRoot 'bin\openocd.exe'
$scripts = Join-Path $openocdRoot 'scripts'
$resolved = Resolve-Path -LiteralPath $Image
$length = (Get-Item $resolved).Length
if ($length -notin 65536, 131072) { throw 'Expected a 64 KiB or 128 KiB STM32 flash image.' }
& $openocd -s $scripts -f interface/stlink.cfg -f target/stm32f1x.cfg -c "transport select swd; adapter speed 950; init; reset halt; flash write_image erase {$($resolved.Path)} 0x08000000 bin; verify_image {$($resolved.Path)} 0x08000000 bin; reset run; shutdown"
if ($LASTEXITCODE -ne 0) { throw 'Restore or verification failed.' }
