@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

REM =====================================================
REM  EvaEngine Project Generator (CMake version)
REM =====================================================
REM  Features:
REM   - Generates Visual Studio project files
REM   - Falls back from VS2022 to VS2019 automatically
REM   - Cleans old CMake cache and project files
REM   - Copies required DLLs (libcurl, zlib)
REM   - Supports build configuration argument: Debug / Release / Dist
REM =====================================================

REM Base directory
SET "BASE_DIR=%~dp0"
IF "%BASE_DIR:~-1%"=="\" SET "BASE_DIR=%BASE_DIR:~0,-1%"

REM Build type argument (default = Debug)
SET "BUILD_TYPE=Debug"
IF NOT "%~1"=="" SET "BUILD_TYPE=%~1"

REM DLL paths
SET "SOURCE_PATH=%BASE_DIR%\EvaEngine\vendor\vcpkg\x64-windows\bin"
SET "TARGET_PATH=%BASE_DIR%\Editor\bin\%BUILD_TYPE%-windows-x86_64\Editor"

echo ================================================
echo  EvaEngine Project Generator
echo ================================================
echo Base Directory : %BASE_DIR%
echo Source DLL Path: %SOURCE_PATH%
echo Target DLL Path: %TARGET_PATH%
echo Build Type     : %BUILD_TYPE%
echo ================================================
echo.

REM Step 1: Copy DLLs
echo 📦 Copying runtime DLLs...
IF NOT EXIST "%TARGET_PATH%" (
    mkdir "%TARGET_PATH%"
)
IF EXIST "%SOURCE_PATH%\libcurl.dll" copy /Y "%SOURCE_PATH%\libcurl.dll" "%TARGET_PATH%\libcurl.dll" >nul
IF EXIST "%SOURCE_PATH%\zlib1.dll" copy /Y "%SOURCE_PATH%\zlib1.dll" "%TARGET_PATH%\zlib1.dll" >nul
echo DLLs copied.
echo.

REM Step 2: Clean old project files
echo 🧹 Cleaning old project and CMake cache files...
for /R "%BASE_DIR%" %%F in (*.vcxproj *.sln *.filters *.user CMakeCache.txt) do del /F /Q "%%F" >nul 2>&1
for /D %%D in ("%BASE_DIR%\CMakeFiles") do rmdir /S /Q "%%D" >nul 2>&1
echo Clean complete.
echo.

REM Step 3: Attempt VS2022
echo ⚙️  Generating Visual Studio 2022 project...
cmake -S "%BASE_DIR%" -B "%BASE_DIR%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE%

IF %ERRORLEVEL% NEQ 0 (
    echo ❌ Visual Studio 2022 not found or generation failed.
    echo Cleaning cache before fallback...
    del /F /Q "%BASE_DIR%\CMakeCache.txt" >nul 2>&1
    rmdir /S /Q "%BASE_DIR%\CMakeFiles" >nul 2>&1

    echo Trying fallback: Visual Studio 2019...
    cmake -S "%BASE_DIR%" -B "%BASE_DIR%" -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE%

    IF %ERRORLEVEL% NEQ 0 (
        echo ❌ Both Visual Studio 2022 and 2019 generation failed!
        echo Please ensure Visual Studio with C++ development tools is installed.
        PAUSE
        EXIT /B 1
    ) ELSE (
        echo ✅ Successfully generated project with Visual Studio 2019.
    )
) ELSE (
    echo ✅ Successfully generated project with Visual Studio 2022.
)

REM Step 4: Rename solution to EvaWorkspace.sln if needed
FOR %%F IN ("%BASE_DIR%\*.sln") DO (
    IF /I NOT "%%~nxF"=="EvaWorkspace.sln" (
        ren "%%F" "EvaWorkspace.sln" >nul 2>&1
    )
)

IF EXIST "%BASE_DIR%\EvaWorkspace.sln" (
    echo ✅ Solution ready: %BASE_DIR%\EvaWorkspace.sln
) ELSE (
    echo ⚠️  Solution file not found.
)

echo.
echo ================================================
echo ✅ Project generation complete!
echo ================================================
echo.

PAUSE
ENDLOCAL
