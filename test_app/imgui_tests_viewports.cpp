// dear imgui
// (tests: viewports)

#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_tests.h"
#include "test_engine/imgui_te_engine.h"      // IM_REGISTER_TEST()
#include "test_engine/imgui_te_context.h"

// Warnings
#ifdef _MSC_VER
#pragma warning (disable: 4100) // unreferenced formal parameter
#pragma warning (disable: 4127) // conditional expression is constant
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

// Operators
static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }    // for IM_CHECK_EQ()
static inline bool operator!=(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x != rhs.x || lhs.y != rhs.y; }    // for IM_CHECK_NE()

//-------------------------------------------------------------------------
// Tests: Viewports
//-------------------------------------------------------------------------

void RegisterTests_Viewports(ImGuiTestEngine* e)
{
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiTest* t = NULL;

    // Require -viewport to register those tests
    if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0)
        return;

    // ## Basic viewport test
    t = IM_REGISTER_TEST(e, "viewport", "viewport_basic_1");
    //t->Flags |= ImGuiTestFlags_RequireViewports;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        if (vars.Step == 0)
        {
            ImGui::SetNextWindowSize(ImVec2(200, 200));
            ImGui::SetNextWindowPos(main_viewport->WorkPos + ImVec2(20, 20));
        }

        if (vars.Step == 2)
        {
            ImGuiWindow* window = ImGui::FindWindowByName("Test Window");
            if (ctx->UiContext->MovingWindow == window)
                vars.Count++;
            if (vars.Count >= 1)
                return;
        }

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("hello!");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ImGuiWindow* window = ctx->GetWindowByRef("");
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        auto& vars = ctx->GenericVars;
        vars.Step = 0;
        ctx->Yield();
        IM_CHECK(window->Viewport == main_viewport);

        vars.Step = 1;
        ctx->WindowMove("", main_viewport->Pos - ImVec2(20.0f, 20.0f));
        IM_CHECK(window->Viewport != main_viewport);
        IM_CHECK(window->ViewportOwned);
        IM_CHECK(window->Viewport->Window == window);

        ctx->WindowMove("", main_viewport->WorkPos + ImVec2(20.0f, 20.0f));
        IM_CHECK(window->Viewport == main_viewport);
        IM_CHECK(window->Viewport->Window == NULL);
        IM_CHECK_EQ(window->Pos, main_viewport->WorkPos + ImVec2(20.0f, 20.0f));

        // Test viewport disappearing while moving
        ctx->WindowMove("", main_viewport->Pos - ImVec2(20.0f, 20.0f));
        vars.Step = 2;
        vars.Count = 0;
        ctx->WindowMove("", main_viewport->WorkPos + ImVec2(100.0f, 100.0f));
        IM_CHECK_NE(window->Pos, main_viewport->WorkPos + ImVec2(100.0f, 100.0f));
    };

#else
    IM_UNUSED(e);
#endif
}
