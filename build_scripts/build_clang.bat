@echo off

setlocal enabledelayedexpansion

if [%CLANG_PATH%]==[] (    
    call config\default_build_config.bat
    if exist config\build_config.bat ( call config\build_config.bat )
    set CLANG_PATH=!CONFIG_CLANG_PATH!
)

set PATH=%PATH%;%CLANG_PATH%

set LOCAL_DIR=%~dp0
pushd %LOCAL_DIR%..\..\imgui\examples\example_null\

@REM -fsanitize=address
@REM -- -Wno-zero-as-null-pointer-constant  > still required for main.cpp
@REM -- -Wno-double-promotion               > still required for main.cpp

@REM Clang+Microsoft headers don't seem to work with -std=c++03
set CFLAGS=
set CFLAGS=%CFLAGS% -std=c++11
set CFLAGS=%CFLAGS% -Weverything -Wextra
set CFLAGS=%CFLAGS% -Wno-zero-as-null-pointer-constant
set CFLAGS=%CFLAGS% -Wno-double-promotion
set CFLAGS=%CFLAGS% -Wno-reserved-id-macro
set CFLAGS=%CFLAGS% -Wno-c++98-compat-pedantic
set CFLAGS=%CFLAGS% -Wno-variadic-macros
REM set CFLAGS=%CFLAGS% -Wno-old-style-cast
set CFLAGS=%CFLAGS% -Xclang -flto-visibility-public-std

set DEFINES=
set DEFINES=%DEFINES% -DIMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
set DEFINES=%DEFINES% -DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
set DEFINES=%DEFINES% -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS

@echo on
clang++ -m64 %DEFINES% %CFLAGS% -g main.cpp ../../*.cpp -I../..

clang++ -m32 %DEFINES% %CFLAGS% -g main.cpp ../../*.cpp -I../..
@echo off

@del a.ilk
if not "%1"=="--nopause" pause

@del a.exe
popd
