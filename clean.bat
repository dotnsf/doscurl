@echo off
REM DOSCurl Clean Script
REM Removes all build artifacts

echo DOSCurl Clean Script
echo ====================
echo.

REM Change to project directory
cd /d %~dp0

echo Cleaning build directory...

REM Remove object files
if exist build\*.obj del /q build\*.obj

REM Remove executable
if exist build\*.exe del /q build\*.exe

REM Remove error files
if exist build\*.err del /q build\*.err

REM Remove other build artifacts
if exist build\*.map del /q build\*.map
if exist build\*.sym del /q build\*.sym

echo.
echo Clean complete!
echo.

@REM Made with Bob
