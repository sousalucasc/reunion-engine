@echo off

where cl >nul 2>nul && goto build

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86
    goto build
)

echo Visual Studio 2013 C++ tools not found.
exit /b 1

:build
if not exist ..\build mkdir ..\build
pushd ..\build
cl -Zi ..\code\win32_handmade.cpp user32.lib
popd