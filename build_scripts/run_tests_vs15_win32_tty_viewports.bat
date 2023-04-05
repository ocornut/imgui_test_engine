@echo off

set TOOLSET=v140
set PLATFORM=Win32
set CONFIGURATION=Release

@REM Run viewport tests first
set TEST_RUN_OPTIONS=-nogui -nopause -v2 -viewport-mock viewport_
call run_tests_vs.bat

@REM Run all tests
set TEST_RUN_OPTIONS=-nogui -nopause -v2 -viewport-mock
call run_tests_vs.bat
