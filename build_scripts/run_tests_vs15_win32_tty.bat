@echo off

set TOOLSET=v140
set PLATFORM=Win32
set CONFIGURATION=Release
set TEST_RUN_OPTIONS=-nogui -nopause -v2

call run_tests_vs.bat
