@echo off
setlocal enabledelayedexpansion

call config\default_build_config.bat
if exist config\build_config.bat ( call config\build_config.bat )

set MINGW_PATH=%CONFIG_MINGW_64_PATH%

call build_mingw.bat