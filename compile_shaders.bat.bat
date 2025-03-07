@echo off
setlocal enabledelayedexpansion

:: Set Vulkan SDK path
set VULKAN_SDK=C:\VulkanSDK\1.4.304.1

:: Shader directory
set SHADER_DIR=C:\EvaEngine\Editor\assets\shaders

:: Compiler
set GLSLANG_VALIDATOR="%VULKAN_SDK%\Bin\glslangValidator.exe"

:: Check if glslangValidator exists
if not exist %GLSLANG_VALIDATOR% (
    echo Vulkan glslangValidator not found! Make sure Vulkan SDK is installed.
    exit /b 1
)

:: Compile all GLSL shaders in the directory
for %%f in (%SHADER_DIR%\*.glsl) do (
    set FILE=%%f
    set OUT_FILE=%%~dpnf.spv

    echo Compiling: !FILE! -> !OUT_FILE!
    
    :: Suppress output except for errors
    %GLSLANG_VALIDATOR% -V !FILE! -o !OUT_FILE! >nul 2>&1

    :: Check if the shader compilation was successful
    if errorlevel 1 (
        echo Error compiling shader: !FILE!
    ) else (
        echo Successfully compiled: !FILE!
    )
)

echo Done!
pause
