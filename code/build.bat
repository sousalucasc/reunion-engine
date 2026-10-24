@echo off

rem Usage: build.bat          debug build (/Od, symbols)
rem        build.bat release  optimized build (/O2 /fp:fast)

set "BUILD_RESULT=1"

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
set BUILD_RESULT=1
if /i "%~1"=="release" (
    cl -FC -W3 -O2 -fp:fast ..\code\main.cpp user32.lib gdi32.lib /Fe:..\build\mc.exe
) else (
    cl -FC -Zi -Od -W3 ..\code\main.cpp user32.lib gdi32.lib /Fe:..\build\mc.exe
)
if not errorlevel 1 set "BUILD_RESULT=0"
popd
exit /b %BUILD_RESULT%
