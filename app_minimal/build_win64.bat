
@REM Minimal build script for building app_mimimal with MSVC (cl.exe), x64 build
@REM You need to run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.

@REM NOTE: Requires Visual Studio or Microsoft Build Tools Installed and may require that you update the path to vcvarsall.bat below
@REM Usage:
@REM    build_win64.bat -- Builds app_minimal in Debug Mode, outputs EXE and OBJ files etc. to app_minimal\build directory

@echo off

setlocal

REM Attempt to run vcvarsall.bat automaticaly.
REM FIXME: This is probably not the greatest way to do this. Could have a list of filenames and iterate within them.
IF "%WindowsSdkDir%" == "" (
    IF exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    )
)
IF "%WindowsSdkDir%" == "" (
    IF exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    )
)
IF "%WindowsSdkDir%" == "" (
    echo vcvarsall.bat was not called!
    exit /b 2
)

@set OUT_DIR=Debug
@set OUT_EXE=app_minimal.exe

IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%

set CommonCompilerFlags=-Fo%OUT_DIR%\
set Defines=-DWIN32=1 -DIMGUI_USER_CONFIG=\"app_minimal/app_minimal_imconfig.h\" -DUNICODE
set LinkerLibs=d3d11.lib d3dcompiler.lib dxgi.lib shell32.lib
set CommonLinkerFlags= -incremental %LinkerLibs%

set INCLUDES=-I "%WindowsSdkDir%Include\um" -I "%WindowsSdkDir%Include\shared"
set INCLUDES=%INCLUDES% -I.. -I..\..\imgui -I..\..\imgui\backends -I..\shared

set SOURCES=*.cpp ..\imgui_test_engine\imgui*.cpp ..\..\imgui\imgui*.cpp ..\shared\imgui_app.cpp

set OutputFile=-Fe%OUT_DIR%\%OUT_EXE%
set MapFile=-Fm%OUT_DIR%\app_minimal.map

cl %OutputFile% %CommonCompilerFlags% %Defines% %INCLUDES% %MapFile% %SOURCES% /link %CommonLinkerFlags%

endlocal

