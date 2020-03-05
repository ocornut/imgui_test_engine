@echo off

setlocal enabledelayedexpansion

if [%MINGW_PATH%]==[] (    
    call config\default_build_config.bat
    if exist config\build_config.bat ( call config\build_config.bat )
    set MINGW_PATH=!CONFIG_MINGW_PATH!
)

set PATH=%PATH%;%MINGW_PATH%

set LOCAL_DIR=%~dp0
pushd %LOCAL_DIR%..\..\imgui\examples\example_null\

set CFLAGS=-Wall -pedantic -std=c++03 -Wno-variadic-macros -Wextra

@echo on
g++ %CFLAGS% -g main.cpp ../../*.cpp -I../..
@echo off

if not "%1"=="--nopause" pause

del a.exe
popd

endlocal
