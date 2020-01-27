@rem This script aims to cover most of the use cases in a small amount of operations

@echo off

setlocal enabledelayedexpansion

echo ################# VS2010, Win32, Debug #################
set TOOLSETS=v100
set PROJECTS=example_glfw_opengl3 example_win32_directx11
set PLATFORMS=Win32
set CONFIGURATIONS=Debug
call build_all_vs.bat

echo ################# VS2017, x64, Release #################
set TOOLSETS=v141
set PROJECTS=example_glfw_opengl3 example_win32_directx11
set PLATFORMS=x64
set CONFIGURATIONS=Release
call build_all_vs.bat

echo ################# Clang #################
call build_clang.bat --nopause

echo ################# MinGW #################
call build_mingw.bat --nopause

pause