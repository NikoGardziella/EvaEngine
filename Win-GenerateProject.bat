@echo off
SETLOCAL

REM Get the base directory (where this .bat file resides)
SET "BASE_DIR=%~dp0"

REM Trim trailing backslash if present
IF "%BASE_DIR:~-1%"=="\" SET "BASE_DIR=%BASE_DIR:~0,-1%"

REM Define paths relative to the BASE_DIR
SET "SOURCE_PATH=%BASE_DIR%\EvaEngine/vendor\vcpkg\x64-windows\bin"
SET "TARGET_PATH=%BASE_DIR%\Editor\bin\Debug-windows-x86_64\Editor"

REM Echo for debugging
echo Copying DLLs from:
echo   %SOURCE_PATH%
echo To:
echo   %TARGET_PATH%
echo.

REM Actually copy the DLLs
copy "%SOURCE_PATH%\libcurl.dll" "%TARGET_PATH%\libcurl.dll"
copy "%SOURCE_PATH%\zlib1.dll" "%TARGET_PATH%\zlib1.dll"

echo Done copying.

:: Continue with your build
del /f /s /q .\*.vcxproj .\*.sln
del /f /s /q .\**\*.vcxproj  <--- This deletes project files inside all subdirectories
.\Vendor\bin\premake5.exe vs2022
PAUSE
ENDLOCAL
