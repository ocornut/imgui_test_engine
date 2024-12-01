// dear imgui
// Demo Tests demonstrating the Dear ImGui Test Engine

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_test_engine/imgui_te_context.h"
#include "imgui_test_engine/imgui_capture_tool.h"

void ShowGameWindowUnderTest()
{
    IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Hello World For Card Game.  Missing Dear ImGui Context");
    IMGUI_CHECKVERSION();

    ImGuiWindowFlags window_flags = 0;
    bool* p_open = NULL; 

    if (!ImGui::Begin("Minimal Card Game", p_open, window_flags))
    {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }

    ImGui::Text("Hello World (%s) (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);
    ImGui::Spacing();
    ImGui::End();
}