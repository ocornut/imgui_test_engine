@REM Minimal build file for building app_minimal with MSVC (cl.exe) x64 builds
@REM NOTE: Requires Visual Studio or Microsoft Build Tools Installed and may require that you update the path to vcvarsall.bat below
@REM Usage:
@REM    build_win64.bat -- Builds app_minimal in Debug Mode, outputs EXE and OBJ files etc. to app_minimal\build directory

@echo off

REM Check for vcvarsall.bat existence
set VcVarsLocation=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat
IF NOT EXIST "%VcVarsLocation%" (
    echo Unable to located vcvarsall.bat at %VcVarsLocation%
    echo Please update VcVarsLocation variable in build_win64.bat to point to your vcvarsall.bat file
    exit /b 2
)

setlocal
call "%VcVarsLocation%" x64

REM NOTE: Assumes we are in the build_scripts directory
IF NOT EXIST build mkdir build

set CommonCompilerFlags=-Fobuild\
set Defines=-DWIN32=1 -DIMGUI_USER_CONFIG=\"app_minimal/app_minimal_imconfig.h\" -DUNICODE
set LinkerLibs=d3d11.lib d3dcompiler.lib dxgi.lib shell32.lib
set CommonLinkerFlags= -incremental %LinkerLibs%

set Includes=-I "%WindowsSdkDir%Include\um" -I "%WindowsSdkDir%Include\shared"
set Includes=%Includes% -I.. -I..\..\imgui -I..\..\imgui\backends -I..\shared

set Sources=app_minimal_main.cpp app_minimal_tests.cpp ..\imgui_test_engine\imgui*.cpp
set Sources=%Sources% ..\..\imgui\imgui*.cpp ..\shared\imgui_app.cpp

set OutputFile=-Febuild\app_minimal.exe
set MapFile=-Fmbuild\app_minimal.map

cl %OutputFile% %CommonCompilerFlags% %Defines% %Includes% %MapFile% %Sources% /link %CommonLinkerFlags%
endlocal

