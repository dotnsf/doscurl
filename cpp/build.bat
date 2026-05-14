@echo off
echo DOSCurl Build Script (mTCP version)
echo ====================================
echo.

if "%WATCOM%"=="" (
    echo Error: WATCOM environment variable not set
    echo Please run WATCOM's setvars.bat first
    exit /b 1
)

echo Using Open Watcom from: %WATCOM%
echo.

echo Building doscurl.exe...
wmake

if errorlevel 1 (
    echo.
    echo Build failed!
    exit /b 1
)

echo.
echo Build successful!
echo Executable: doscurl.exe

