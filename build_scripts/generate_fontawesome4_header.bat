@echo off

setlocal enabledelayedexpansion

call build_binary_to_compressed_c.bat
if not "%ERRORLEVEL%"=="0" (
    echo Failed to generate tool
    exit /B 1
)

(set LF=^
%=Do not remove this line=%
)

set OUPTUT_FILE_PATH=..\cpp\imgui_fonts_fontawesome4.h

echo #pragma once!LF!^
!LF!^
// File generated from build/generate_font_awesome4_header.bat!LF!^
// You can find fontawesome-webfont.ttf at https://github.com/FortAwesome/Font-Awesome/blob/fa-4/fonts/fontawesome-webfont.ttf!LF!^
// List of icons' strings: https://github.com/juliettef/IconFontCppHeaders!LF!^
// Generated with tools\binary_to_compressed_c.exe ..\fonts\fontawesome-webfont.ttf font_awesome_4_webfont!LF!^
!LF!^
void GetFontAwesomeCompressedFontDataTTF(const void** data, unsigned int* size);!LF!^
!LF!^
#ifdef IMGUI_FONTS_FONTAWESOME4_IMPLEMENTATION!LF! > %OUPTUT_FILE_PATH%

tools\binary_to_compressed_c.exe ..\fonts\fontawesome-webfont.ttf font_awesome_4_webfont >> %OUPTUT_FILE_PATH%

echo !LF!^
void GetFontAwesomeCompressedFontDataTTF(const void** data, unsigned int* size)!LF!^
{!LF!^
    *data = font_awesome_4_webfont_compressed_data;!LF!^
    *size = font_awesome_4_webfont_compressed_size;!LF!^
}!LF!^
#endif // IMGUI_FONTS_FONTAWESOME4_IMPLEMENTATION >> %OUPTUT_FILE_PATH%