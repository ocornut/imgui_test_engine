@set PVS_DIR=C:\Program Files (x86)\PVS-Studio
@set WORK_DIR=%~dp0
@set IMGUI_DIR=%WORK_DIR%..\..\imgui
@set PROJ_DIR=%IMGUI_DIR%\examples\example_win32_directx11
@set PROJ_NAME=example_win32_directx11

@REM // warning: V1071 Consider inspecting the 'TreeNodeEx' function. The return value is not always used. Total calls: 10, discarded results: 1.
@set EXCLUDED_CODES=-d V1071

@echo ---- Running PVS Studio
@"%PVS_DIR%\PVS-Studio_Cmd.exe" -r -t "%PROJ_DIR%\%PROJ_NAME%.vcxproj"

@echo ---- Filtering
@mkdir "%WORK_DIR%\output"
@"%PVS_DIR%\PlogConverter.exe" -a GA:1,2;OP:1 %EXCLUDED_CODES% -t Html,FullHtml,Txt,Totals "%PROJ_DIR%\%PROJ_NAME%.plog" -o "%WORK_DIR%\output"
@del "%PROJ_DIR%\%PROJ_NAME%.plog"

@echo ---- Totals:
@type "%WORK_DIR%\output\%PROJ_NAME%.plog_totals.txt"

@REM "the start command needs blank quotes at the beginning, as it uses the first double quoted phrase as the "Window title""
@REM @start "" "%WORK_DIR%\output"
@start "" "%WORK_DIR%\output\fullhtml\index.html"

pause
