@echo off
setlocal
set "FORMATTER=%APPDATA%\Python\Python314\Scripts\clang-format.exe"
if not exist "%FORMATTER%" (
  echo clang-format was not found at "%FORMATTER%".
  exit /b 1
)
"%FORMATTER%" -i -style=file "%~dp0..\src\*.c" "%~dp0..\include\*.h"
exit /b %ERRORLEVEL%
