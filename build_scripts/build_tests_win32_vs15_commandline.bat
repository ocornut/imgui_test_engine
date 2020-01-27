@echo off

set TOOLSET=v140
set PLATFORM=Win32
set CONFIGURATION=Release
set TEST_RUN_OPTIONS=-nogui -nopause -v2

call build_and_run_tests.bat