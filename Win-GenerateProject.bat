del /f /s /q .\*.vcxproj .\*.sln
del /f /s /q .\**\*.vcxproj  <--- This deletes project files inside all subdirectories
.\Vendor\bin\premake5.exe vs2022
PAUSE