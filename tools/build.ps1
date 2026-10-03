param([int]$LedCount = 8, [string]$BuildDir = 'build')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$gcc = Get-ChildItem 'C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi' -Recurse -Filter arm-none-eabi-gcc.exe | Select-Object -First 1
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$ninja = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Recurse -Filter ninja.exe | Select-Object -First 1
if (-not $gcc -or -not (Test-Path $cmake) -or -not $ninja) { throw 'Install GNU Arm Toolchain, CMake, and Ninja first.' }
$env:Path = $gcc.DirectoryName + ';' + $env:Path
& $cmake -S $root -B (Join-Path $root $BuildDir) -G Ninja "-DCMAKE_MAKE_PROGRAM=$($ninja.FullName)" "-DCMAKE_TOOLCHAIN_FILE=$(Join-Path $root 'cmake\arm-gcc.cmake')" "-DWS2812_LED_COUNT=$LedCount"
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
& $cmake --build (Join-Path $root $BuildDir)
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
