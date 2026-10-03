param([string]$Image = 'build\stm32_ws2812.bin')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$openocdRoot = Get-ChildItem "$env:LOCALAPPDATA\stlink-tool\cache" -Recurse -Filter openocd.exe | Select-Object -First 1 | ForEach-Object { Split-Path (Split-Path $_.FullName -Parent) -Parent }
if (-not $openocdRoot) { throw 'OpenOCD not found. Run tools\stlink-tool-windows-x64.exe once.' }
$openocd = Join-Path $openocdRoot 'bin\openocd.exe'
$scripts = Join-Path $openocdRoot 'scripts'
$imagePath = if ([IO.Path]::IsPathRooted($Image)) { $Image } else { Join-Path $root $Image }
if (-not (Test-Path -LiteralPath $imagePath)) { throw "Firmware image not found: $imagePath" }
$imagePath = (Resolve-Path -LiteralPath $imagePath).Path
if (-not (Get-ChildItem (Join-Path $root 'backup') -Filter 'original-*.bin' -ErrorAction SilentlyContinue)) { throw 'No original flash backup; run dump_flash.cmd first.' }
& $openocd -s $scripts -f interface/stlink.cfg -f target/stm32f1x.cfg -c "transport select swd; adapter speed 950; init; reset halt; flash write_image erase {$imagePath} 0x08000000 bin; verify_image {$imagePath} 0x08000000 bin; reset run; shutdown"
if ($LASTEXITCODE -ne 0) { throw 'Programming or verification failed.' }
