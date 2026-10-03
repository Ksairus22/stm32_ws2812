@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0dump_flash.ps1" %*
exit /b %ERRORLEVEL%
