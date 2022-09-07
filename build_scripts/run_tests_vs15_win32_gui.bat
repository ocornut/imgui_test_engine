@echo off

set LOCAL_DIR=%~dp0
set FILE_OPENER_PATH=%LOCAL_DIR%..\imgui_test_suite\tools\win32_open_with_sublime.cmd tests

set TOOLSET=v140
set PLATFORM=Win32
set CONFIGURATION=Debug
set TEST_RUN_OPTIONS= -gui -v -fileopener %FILE_OPENER_PATH% tests

call run_tests_vs.bat
