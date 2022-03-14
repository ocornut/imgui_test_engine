@echo off

if "%TOOLSET%"==""          set TOOLSET=v140
if "%PLATFORM%"==""         set PLATFORM=Win32
if "%CONFIGURATION%"==""    set CONFIGURATION=Release
if "%TEST_RUN_OPTIONS%"=="" set TEST_RUN_OPTIONS=-nogui -nopause

set BUILD_OPTIONS=/nologo /verbosity:minimal
set LOCAL_DIR=%~dp0
set IMGUI_TESTS_APP_DIR=%LOCAL_DIR%..\imgui_tests

call utils\hMSBuild.bat %BUILD_OPTIONS% /p:PlatformToolset=%TOOLSET% /t:imgui_tests /p:Platform=%PLATFORM% /p:Configuration=%CONFIGURATION% %IMGUI_TESTS_APP_DIR%\..\imgui_test_engine.sln
echo imgui_tests.exe %TEST_RUN_OPTIONS%
%IMGUI_TESTS_APP_DIR%\%CONFIGURATION%\imgui_tests.exe %TEST_RUN_OPTIONS%

pause
