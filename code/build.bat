@echo off

where cl >nul 2>nul && goto build

for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvarsall.bat" x64

:build
if not exist ..\build mkdir ..\build
pushd ..\build
cl -Zi ..\code\win32_handmade.cpp user32.lib
popd