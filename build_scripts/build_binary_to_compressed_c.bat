@echo off

setlocal enabledelayedexpansion

set LOCAL_DIR=%~dp0
set IMGUI_DIR=%LOCAL_DIR%..\..\imgui\

rem #### Trying to find CL.exe

if exist "%programfiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set VS_VARS_BATCH="%programfiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
    goto found_vs_env
)
for %%V in (VS140COMNTOOLS VS130COMNTOOLS VS120COMNTOOLS VS110COMNTOOLS VS100COMNTOOLS) do (
    if not "!%%V!"=="" (
        set VS_VARS_BATCH="!%%V!vsvars32.bat"
        goto found_vs_env
    )
)

echo #### Error: Couldn't find CL.exe  1>&2
exit /B 1

:found_vs_env
call %VS_VARS_BATCH%
if not exist tools\ mkdir tools
cl /Os %IMGUI_DIR%misc\fonts\binary_to_compressed_c.cpp /Fotools/ /Fetools/
