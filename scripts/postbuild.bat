@echo off

robocopy %1 %2 /MIR
if %errorlevel% == 1 (
    exit /b 0
)
exit /b %errorlevel%
