// Small debug helpers

/*

//-----------------------------------------------------------------------------------
// Index of this file
//-----------------------------------------------------------------------------------

- DebugSlowDown()               [Win32 only, easy to port]


//-----------------------------------------------------------------------------------
// Typical setup
//-----------------------------------------------------------------------------------

#include "../imgui_test_engine/shared/imgui_debug.h"

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
