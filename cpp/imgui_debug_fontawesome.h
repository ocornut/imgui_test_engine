/*
 *
 * Index of this file:
 *
 * - DebugMergeFontAwesome4()
 *
 */

//-----------------------------------------------------------------------------------
// Header Mess
//-----------------------------------------------------------------------------------

#define _CRT_SECURE_NO_WARNINGS
#include "imgui_internal.h"

#ifdef _MSC_VER
#pragma warning (disable: 4996) // warning C4996: 'sprintf': This function or variable may be unsafe.
#endif

//-----------------------------------------------------------------------------------
// Implementations
//-----------------------------------------------------------------------------------

#include "IconsFontAwesome4_c.h"
#include "imgui_fonts_fontawesome4.h"

// define IMGUI_FONTS_FONTAWESOME4_IMPLEMENTATION before including imgui_debug.h in one cpp file
// ImGui::Text(ICON_FA_ARROWS_H " New Horizontal divider");
// ImGui::Text(ICON_FA_ARROWS_V " New Vertical divider");
ImFont* DebugMergeFontAwesome4(float size)
{
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig cfg;
    cfg.MergeMode = true;
    cfg.GlyphMinAdvanceX = size;
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

    const void* font_data = NULL;
    unsigned int font_data_size = 0;
    GetFontAwesomeCompressedFontDataTTF(&font_data, &font_data_size);
    return io.Fonts->AddFontFromMemoryCompressedTTF(font_data, font_data_size, size, &cfg, icon_ranges);
}
