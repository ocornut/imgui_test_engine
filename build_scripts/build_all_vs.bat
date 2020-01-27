@echo off

setlocal enabledelayedexpansion

if "%TOOLSETS%"=="" (
    set TOOLSETS=
    set TOOLSETS=!TOOLSETS! v100
    set TOOLSETS=!TOOLSETS! v140
    set TOOLSETS=!TOOLSETS! v141
    set TOOLSETS=!TOOLSETS! v140_clang_c2
    set TOOLSETS=!TOOLSETS! LLVM-vs2014
)

if "%PROJECTS%"=="" (
    set PROJECTS=
    set PROJECTS=!PROJECTS! example_glfw_opengl2
    set PROJECTS=!PROJECTS! example_glfw_opengl3
    set PROJECTS=!PROJECTS! example_win32_directx9
    set PROJECTS=!PROJECTS! example_win32_directx10
    set PROJECTS=!PROJECTS! example_win32_directx11
)

if "%PLATFORMS%"=="" (
    set PLATFORMS=
    set PLATFORMS=!PLATFORMS! Win32
    set PLATFORMS=!PLATFORMS! x64
)

if "%CONFIGURATIONS%"=="" (
    set CONFIGURATIONS=
    set CONFIGURATIONS=!CONFIGURATIONS! Debug
    set CONFIGURATIONS=!CONFIGURATIONS! Release
)

set LOCAL_DIR=%~dp0
set SOLUTION_PATH=%LOCAL_DIR%..\..\imgui\examples\imgui_examples.sln

set OPTIONS=
set OPTIONS=%OPTIONS% /nologo
set OPTIONS=%OPTIONS% /verbosity:minimal
set OPTIONS=%OPTIONS% /p:DefineConstants="IMGUI_DISABLE_OBSOLETE_FUNCTIONS"

call utils\hMSBuild.bat %OPTIONS% /t:Clean %SOLUTION_PATH%

echo ################# Toolsets: %TOOLSETS%
echo ################# Projects: %PROJECTS%
echo ################# Platforms: %PLATFORMS%
echo ################# Configurations: %CONFIGURATIONS%

for %%T in (!TOOLSETS!) do (
for %%P in (!PROJECTS!) do (
for %%A in (!PLATFORMS!) do (
for %%C in (!CONFIGURATIONS!) do (

    echo ################# Building project %%P with toolset %%T for achitecture %%A and configuration %%C #################
    call utils\hMSBuild.bat %OPTIONS% /p:PlatformToolset=%%T /t:%%P /p:Platform=%%A /p:Configuration=%%C %SOLUTION_PATH%
    
))))
