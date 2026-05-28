@echo off
REM DOSCurl Build Script for Windows
REM Builds the project using Open Watcom C/C++

echo DOSCurl Build Script
echo ====================
echo.

REM Check if WATCOM environment variable is set
if not defined WATCOM (
    echo Error: WATCOM environment variable is not set
    echo Please set WATCOM to your Open Watcom installation directory
    echo Example: set WATCOM=C:\WATCOM
    exit /b 1
)

REM Set up Open Watcom environment
set PATH=%WATCOM%\binnt;%PATH%
set INCLUDE=%WATCOM%\h;%WATCOM%\h\nt;%WATCOM%\h\wattcp
set LIB=%WATCOM%\lib286

echo Using Open Watcom from: %WATCOM%
echo.

REM Change to project directory
cd /d %~dp0

REM Run wmake
echo Building doscurl.exe...
wmake

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed!
    exit /b 1
)

echo.
echo Build successful!
echo Executable: build\doscurl.exe
echo.