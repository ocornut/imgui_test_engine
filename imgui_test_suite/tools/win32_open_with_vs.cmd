@REM Pass to imgui_test_suite.exe with '-fileopener win32_open_with_vs.cmd' or use own script.
@REM Unfortunately this:
@REM - use the first/any instance of VS IDE, rather than last used.
@REM - doesn't seem to support opening at a given line.
@REM We are printing file/line in the Debug Output console to facilitate double-clicking into the line.

"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe" /edit "%1"
@REM "%DevEnvDir%\devenv.exe" /edit "%1"

