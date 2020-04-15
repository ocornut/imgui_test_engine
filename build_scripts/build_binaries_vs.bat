@echo off

setlocal enabledelayedexpansion

set LOCAL_DIR=%~dp0
set IMGUI_DIR=%LOCAL_DIR%..\..\imgui\

set TOOLSETS=v140

set PROJECTS=
set PROJECTS=%PROJECTS% example_glfw_opengl2
set PROJECTS=%PROJECTS% example_glfw_opengl3
REM set PROJECTS=%PROJECTS% example_glfw_vulkan
set PROJECTS=%PROJECTS% example_win32_directx9
set PROJECTS=%PROJECTS% example_win32_directx11

set PLATFORMS=Win32

set CONFIGURATIONS=Release

call build_all_vs.bat

set EXECUTABLES=
for %%P in (%PROJECTS%) do (
for %%C in (%CONFIGURATIONS%) do (
    set EXECUTABLES=!EXECUTABLES! %IMGUI_DIR%examples\%%P\%%C\%%P.exe
))

call build_binary_to_compressed_c.bat
if "%ERRORLEVEL%"=="0" ( set EXECUTABLES=!EXECUTABLES! tools\binary_to_compressed_c.exe )

for /f "tokens=1-3 delims=/- " %%i in ("%date%") do (
     set year=%%i
     set month=%%j
     set day=%%k
)

set ARCHIVE_NAME=imgui-demo-binaries-%year%%month%%day%.zip
if exist %ARCHIVE_NAME% del %ARCHIVE_NAME%
7z a %ARCHIVE_NAME% !EXECUTABLES!
7z l %ARCHIVE_NAME%

pause
