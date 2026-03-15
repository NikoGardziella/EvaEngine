@echo off
setlocal

REM 1. Set ROOT immediately
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
pushd "%ROOT%"

REM 2. Check if vcpkg submodule is initialized (look for the bootstrap file)
if not exist "%ROOT%\EvaEngine\vendor\vcpkg\bootstrap-vcpkg.bat" (
    echo [EvaEngine] vcpkg submodule missing. Initializing...
    git submodule update --init --recursive
)

REM 3. Bootstrap vcpkg.exe if it doesn't exist
if not exist "%ROOT%\EvaEngine\vendor\vcpkg\vcpkg.exe" (
    echo [EvaEngine] Bootstrapping vcpkg...
    call "%ROOT%\EvaEngine\vendor\vcpkg\bootstrap-vcpkg.bat"
)

REM 4. Install Dependencies via Manifest (vcpkg.json)
echo [EvaEngine] Installing dependencies...
"%ROOT%\EvaEngine\vendor\vcpkg\vcpkg.exe" install --triplet x64-windows --x-install-root="%ROOT%\EvaEngine\vendor\vcpkg_installed"

REM 5. Run Premake
set "PREMAKE=%ROOT%\EvaEngine\vendor\premake\premake5.exe"
if not exist "%PREMAKE%" (
    echo ERROR: premake5.exe not found at %PREMAKE%
    pause
    exit /b 1
)

echo [EvaEngine] Cleaning old project files...
del /f /q "%ROOT%\*.sln" >nul 2>nul

echo [EvaEngine] Generating Visual Studio 2022 solution...
"%PREMAKE%" vs2022

popd
echo [EvaEngine] Done!
pause
endlocal