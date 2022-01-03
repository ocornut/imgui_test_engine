@echo off

setlocal enabledelayedexpansion

if [%MINGW_PATH%]==[] (    
    call config\default_build_config.bat
    if exist config\build_config.bat ( call config\build_config.bat )
    set MINGW_PATH=!CONFIG_MINGW_PATH!
)

set PATH=%PATH%;%MINGW_PATH%

set LOCAL_DIR=%~dp0
pushd %LOCAL_DIR%..\..\imgui\examples\example_win32_directx9\

set CFLAGS=-Wall -pedantic -std=c++11 -Wextra
set LIBS=-ld3d9 -lgdi32 -ldwmapi

@echo on
g++ %CFLAGS% -g main.cpp ../../backends/imgui_impl_dx9.cpp ../../backends/imgui_impl_win32.cpp ../../*.cpp -I../.. -I../../backends %LIBS% 
@echo off

if not "%1"=="--nopause" pause

del a.exe
popd

endlocal
