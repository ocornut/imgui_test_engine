// dear imgui
// (tests: viewports)

#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_tests.h"
#include "imgui_test_engine/imgui_te_engine.h"      // IM_REGISTER_TEST()
#include "imgui_test_engine/imgui_te_context.h"

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
// Ideas/Specs for future tests
// It is important we take the habit to write those down.
// - Even if we don't implement the test right away: they allow us to remember edge cases and interesting things to test.
// - Even if they will be hard to actually implement/automate, they still allow us to manually check things.
//-------------------------------------------------------------------------
// TODO: Tests: Viewport: host A, viewport B over A, make new Modal appears where geometry overlap A -> SHOULD APPEAR OVER B
// TODO: Tests: Viewport: host A, viewport B over A, window C with TopMost, move over host A (may be merged) then drag over B -> SHOULD APPEAR OVER B AKA NOT MERGE IN A, OR, UNMERGE FROM A
//              Actually same test is meaningful even without TopMost flag, since clicking C supposedly should bring it to front, C merged in A and dragging around B should appear over B
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// Tests: Viewports
//-------------------------------------------------------------------------

void RegisterTests_Viewports(ImGuiTestEngine* e)
{
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiTest* t = NULL;

    // Require -viewport to register those tests
#if 1 // Set to 0 to verify that a given tests actually fails without viewport enabled
    if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0)
        return;
    if ((ImGui::GetIO().BackendFlags & ImGuiBackendFlags_PlatformHasViewports) == 0)
        return;
    if ((ImGui::GetIO().BackendFlags & ImGuiBackendFlags_RendererHasViewports) == 0)
        return;
#endif

    // ## Basic viewport test
    t = IM_REGISTER_TEST(e, "viewport", "viewport_basic_1");
    //t->Flags |= ImGuiTestFlags_RequireViewports;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
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
        ImGuiTestGenericVars& vars = ctx->GenericVars;
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

    // ## Test value of ParentViewportID (include bug #4756)
    t = IM_REGISTER_TEST(e, "viewport", "viewport_parent_id");
    struct DefaultParentIdVars { ImGuiWindowClass WindowClass; bool SetWindowClass = false; };
    t->SetVarsDataType<DefaultParentIdVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DefaultParentIdVars& vars = ctx->GetVars<DefaultParentIdVars>();
        if (vars.SetWindowClass)
            ImGui::SetNextWindowClass(&vars.WindowClass);
        ImGui::SetNextWindowSize(ImVec2(50, 50));
        ImGui::Begin("Test Window with Class", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        DefaultParentIdVars& vars = ctx->GetVars<DefaultParentIdVars>();
        ctx->SetRef("Test Window with Class");
        ImGuiWindow* window = ctx->GetWindowByRef("");

        // Reset class at it currently persist if not set
        vars.WindowClass = ImGuiWindowClass();
        vars.SetWindowClass = true;
        ctx->Yield();

        // Default value.
        ctx->WindowMove("", ImGui::GetMainViewport()->Pos + ImVec2(100, 100));
        IM_CHECK_EQ(window->ViewportOwned, false);
        IM_CHECK_EQ(window->WindowClass.ParentViewportId, (ImGuiID)-1);  // Default value

        // Unset. Result depends on ConfigViewportsNoDefaultParent value.
        vars.WindowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge;
        vars.SetWindowClass = true;

        g.IO.ConfigViewportsNoDefaultParent = true;
        ctx->Yield();
        IM_CHECK_EQ(window->Viewport->ParentViewportId, (ImGuiID)0);

        g.IO.ConfigViewportsNoDefaultParent = false;
        ctx->Yield();
        const ImGuiID IMGUI_VIEWPORT_DEFAULT_ID = ImGui::GetMainViewport()->ID;
        IM_CHECK_EQ(window->Viewport->ParentViewportId, IMGUI_VIEWPORT_DEFAULT_ID);

        // Explicitly set parent viewport id. 0 may or may not be a special value. Currently it isn't.
        vars.WindowClass.ParentViewportId = 0;
        ctx->Yield();
        IM_CHECK_EQ(window->Viewport->ParentViewportId, (ImGuiID)0);

        // This is definitely a non-special value.
        vars.WindowClass.ParentViewportId = 0x12345678;
        ctx->Yield();
        IM_CHECK_EQ(window->Viewport->ParentViewportId, (ImGuiID)0x12345678);
    };

#else
    IM_UNUSED(e);
#endif
}
