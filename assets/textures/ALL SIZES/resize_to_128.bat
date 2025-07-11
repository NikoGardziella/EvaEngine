@echo off
setlocal enabledelayedexpansion

REM Create output directory
set OUTPUT_FOLDER=resized_128x128
if not exist "%OUTPUT_FOLDER%" (
    mkdir "%OUTPUT_FOLDER%"
)

REM Loop through all PNG files in the current directory
for %%f in (*.png) do (
    echo Resizing: %%f
    magick "%%f" -resize 128x128^! "%OUTPUT_FOLDER%\%%~nxf"
)

echo Done! Resized images saved in %OUTPUT_FOLDER%
pause
