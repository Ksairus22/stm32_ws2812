$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
foreach ($count in 1, 8, 60) {
  $dir = "build-verify-$count"
  & (Join-Path $PSScriptRoot 'build.ps1') -LedCount $count -BuildDir $dir
  $elf = Join-Path $root "$dir\stm32_ws2812.elf"
  $bin = Join-Path $root "$dir\stm32_ws2812.bin"
  if ((Get-Item $bin).Length -gt 65536) { throw "LED $count image exceeds flash." }
  $sizeTool = Get-ChildItem 'C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi' -Recurse -Filter arm-none-eabi-size.exe | Select-Object -First 1
  & $sizeTool.FullName $elf
}
Write-Host 'Build verification passed for 1, 8, and 60 LEDs.'
