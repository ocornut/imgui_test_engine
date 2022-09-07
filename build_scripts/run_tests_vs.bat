@echo off

if "%TOOLSET%"==""          set TOOLSET=v140
if "%PLATFORM%"==""         set PLATFORM=Win32
if "%CONFIGURATION%"==""    set CONFIGURATION=Release
if "%TEST_RUN_OPTIONS%"=="" set TEST_RUN_OPTIONS=-nogui -nopause

set BUILD_OPTIONS=/nologo /verbosity:minimal
set LOCAL_DIR=%~dp0
set IMGUI_TEST_SUITE_DIR=%LOCAL_DIR%..\imgui_test_suite

call utils\hMSBuild.bat %BUILD_OPTIONS% /p:PlatformToolset=%TOOLSET% /t:imgui_test_suite /p:Platform=%PLATFORM% /p:Configuration=%CONFIGURATION% %IMGUI_TEST_SUITE_DIR%\..\imgui_test_engine.sln
echo imgui_test_suite.exe %TEST_RUN_OPTIONS%
%IMGUI_TEST_SUITE_DIR%\%CONFIGURATION%\imgui_test_suite.exe %TEST_RUN_OPTIONS%

pause
