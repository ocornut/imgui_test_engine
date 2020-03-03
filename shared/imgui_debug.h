/*
//-----------------------------------------------------------------------------------
// Index of this file
//-----------------------------------------------------------------------------------

- DebugSlowDown()               [Win32 only]
- DebugShowInputTextState()

//-----------------------------------------------------------------------------------
// Typical setup
//-----------------------------------------------------------------------------------

#include "../imgui_dev/shared/imgui_debug.h"

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

// Add a slider to slow down the application. Useful to track the kind of graphical glitches that only happens for a frame or two.
#ifdef _WIN32
void DebugSlowDown()
{
    static bool slow_down = false;
    static int slow_down_ms = 100;
    if (slow_down)
        ::Sleep(slow_down_ms);
    ImGui::Checkbox("Slow down", &slow_down);
    ImGui::SameLine();
    ImGui::PushItemWidth(70);
    ImGui::SliderInt("##ms", &slow_down_ms, 0, 400, "%.0f ms");
    ImGui::PopItemWidth();
}
#endif // #ifdef _WIN32

//-----------------------------------------------------------------------------------

// Visualize internals of stb_textedit.h
// https://github.com/nothings/stb/issues/321
void DebugInputTextState()
{
    ImGuiContext& g = *GImGui;
    //static MemoryEditor mem_edit;
    using namespace ImGui;

    ImGui::Begin("Debug stb_textedit.h");

    ImGuiInputTextState& imstate = g.InputTextState;
    if (g.ActiveId != 0 && imstate.ID == g.ActiveId)
        ImGui::Text("Active");
    else
        ImGui::Text("Inactive");

    ImStb::StbUndoState& undostate = imstate.Stb.undostate;

    ImGui::Text("undo_point: %d\nredo_point:%d\nundo_char_point: %d\nredo_char_point:%d",
        undostate.undo_point,
        undostate.redo_point,
        undostate.undo_char_point,
        undostate.redo_char_point);

    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    for (int n = 0; n < STB_TEXTEDIT_UNDOSTATECOUNT; n++)
    {
        char type = ' ';
        if (n < undostate.undo_point)
            type = 'u';
        else if (n >= undostate.redo_point)
            type = 'r';

        ImVec4 col = (type == ' ') ? style.Colors[ImGuiCol_TextDisabled] : style.Colors[ImGuiCol_Text];
        ImGui::TextColored(col, "%c [%02d] where %03d, insert %03d, delete %03d, char_storage %03d",
            type, n, undostate.undo_rec[n].where, undostate.undo_rec[n].insert_length, undostate.undo_rec[n].delete_length, undostate.undo_rec[n].char_storage);
        //if (ImGui::IsItemClicked() && undostate.undo_rec[n].char_storage != -1)
        //    mem_edit.GotoAddrAndHighlight(undostate.undo_rec[n].char_storage, undostate.undo_rec[n].char_storage + undostate.undo_rec[n].insert_length);
    }
    ImGui::PopStyleVar();

    ImGui::End();

    ImGui::Begin("Debug stb_textedit.h char_storage");
    ImVec2 p = ImGui::GetCursorPos();
    for (int n = 0; n < STB_TEXTEDIT_UNDOCHARCOUNT; n++)
    {
        int c = undostate.undo_char[n];
        if (c > 256)
            continue;
        if ((n % 32) == 0)
        {
            ImGui::SetCursorPos(ImVec2(p.x + (n % 32) * 11, p.y + (n / 32) * 13));
            ImGui::Text("%03d:", n);
        }
        ImGui::SetCursorPos(ImVec2(p.x + 40 + (n % 32) * 11, p.y + (n / 32) * 13));
        ImGui::Text("%c", c);
    }
    ImGui::End();

    //mem_edit.Rows = 32;
    //mem_edit.ReadOnly = true;
    //mem_edit.DrawWindow("Debug stb_textedit.h char_storage", (unsigned char*)undostate.undo_char, STB_TEXTEDIT_UNDOCHARCOUNT, 0);
}

//-----------------------------------------------------------------------------------
