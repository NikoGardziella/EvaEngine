@echo off
setlocal

REM Repo root = folder where this .bat is located
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

pushd "%ROOT%"

REM ---- vcpkg DLL copy ----
set "SOURCE_PATH=%ROOT%\EvaEngine\vendor\vcpkg\installed\x64-windows\bin"
set "TARGET_PATH=%ROOT%\Editor\bin\Debug-windows-x86_64\Editor"

echo Copying DLLs from:
echo   %SOURCE_PATH%
echo To:
echo   %TARGET_PATH%
echo.

if not exist "%SOURCE_PATH%" (
    echo ERROR: Source path missing:
    echo   %SOURCE_PATH%
    popd
    pause
    exit /b 1
)

if not exist "%TARGET_PATH%" (
    echo Creating target directory...
    mkdir "%TARGET_PATH%" 2>nul
)

for %%F in (libcurl.dll zlib1.dll) do (
    if exist "%SOURCE_PATH%\%%F" (
        copy /y "%SOURCE_PATH%\%%F" "%TARGET_PATH%\%%F" >nul
        echo Copied %%F
    ) else (
        echo WARNING: Missing %SOURCE_PATH%\%%F
    )
)

echo.
echo Done copying.
echo.

REM ---- premake ----
set "PREMAKE=%ROOT%\EvaEngine\vendor\premake\premake5.exe"
if not exist "%PREMAKE%" (
    echo ERROR: premake5.exe not found:
    echo   %PREMAKE%
    popd
    pause
    exit /b 1
)

if not exist "%ROOT%\premake5.lua" (
    echo ERROR: premake5.lua not found in repo root:
    echo   %ROOT%
    popd
    pause
    exit /b 1
)

echo Deleting generated .sln/.vcxproj in repo root...
del /f /q "%ROOT%\*.sln" >nul 2>nul
del /f /q "%ROOT%\*.vcxproj" >nul 2>nul
del /f /q "%ROOT%\*.vcxproj.filters" >nul 2>nul

echo Running premake in:
echo   %ROOT%
"%PREMAKE%" vs2022

popd
pause
endlocal
