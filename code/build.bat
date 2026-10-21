@echo off

where cl >nul 2>nul && goto build

for %%E in (Community Professional Enterprise BuildTools) do (
    if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
        call "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat" x64
        goto build
    )
)

echo Visual Studio 2022 C++ tools not found.
exit /b 1

:build
if not exist ..\build mkdir ..\build
pushd ..\build
cl -FC -Zi ..\code\win32_handmade.cpp user32.lib
popd