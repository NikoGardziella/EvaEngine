@echo off
setlocal enabledelayedexpansion

REM Base directory (folder where this .bat resides)
set "BASE_DIR=%~dp0"
if "%BASE_DIR:~-1%"=="\" set "BASE_DIR=%BASE_DIR:~0,-1%"

pushd "%BASE_DIR%"

REM ---- vcpkg DLL copy (optional) ----
set "SOURCE_PATH=%BASE_DIR%\EvaEngine\vendor\vcpkg\installed\x64-windows\bin"
set "TARGET_PATH=%BASE_DIR%\Editor\bin\Debug-windows-x86_64\Editor"

echo Copying DLLs from:
echo   "%SOURCE_PATH%"
echo To:
echo   "%TARGET_PATH%"
echo.

if not exist "%TARGET_PATH%" (
    echo Creating target directory...
    mkdir "%TARGET_PATH%" 2>nul
)

if not exist "%SOURCE_PATH%" (
    echo ERROR: Source path does not exist:
    echo   "%SOURCE_PATH%"
    popd
    pause
    exit /b 1
)

for %%F in (libcurl.dll zlib1.dll) do (
    if exist "%SOURCE_PATH%\%%F" (
        copy /y "%SOURCE_PATH%\%%F" "%TARGET_PATH%\%%F" >nul
        echo Copied %%F
    ) else (
        echo WARNING: Missing "%SOURCE_PATH%\%%F"
    )
)

echo.
echo Done copying.
echo.

REM ---- premake exe ----
set "PREMAKE=%BASE_DIR%\EvaEngine\vendor\premake\premake5.exe"
if not exist "%PREMAKE%" (
    echo ERROR: premake5.exe not found at:
    echo   "%PREMAKE%"
    popd
    pause
    exit /b 1
)

REM ---- premake root (where premake5.lua is) ----
set "PREMAKE_ROOT="
if exist "%BASE_DIR%\premake5.lua" set "PREMAKE_ROOT=%BASE_DIR%"
if not defined PREMAKE_ROOT if exist "%BASE_DIR%\EvaEngine\premake5.lua" set "PREMAKE_ROOT=%BASE_DIR%\EvaEngine"

if not defined PREMAKE_ROOT (
    echo ERROR: premake5.lua not found in:
    echo   "%BASE_DIR%"
    echo   "%BASE_DIR%\EvaEngine"
    popd
    pause
    exit /b 1
)

echo Premake root: "%PREMAKE_ROOT%"

REM Delete solution + project files only under premake root
echo Deleting .sln and .vcxproj files under premake root...
for /r "%PREMAKE_ROOT%" %%G in (*.sln *.vcxproj) do (
    del /f /q "%%G" >nul 2>nul
)

REM Run premake once
pushd "%PREMAKE_ROOT%"
echo Using premake: "%PREMAKE%"
"%PREMAKE%" vs2022
popd

popd
pause
endlocal
