@echo off

set TOOLSET=v140
set PLATFORM=Win32
set CONFIGURATION=Debug

set LOCAL_DIR=%~dp0
set OPENER_PATH=%LOCAL_DIR%..\tests\tools\win32_open_with_sublime.cmd tests
set TEST_RUN_OPTIONS= -gui -v -fileopener %OPENER_PATH% tests

call build_and_run_tests.bat