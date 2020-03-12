// dear imgui editor demo
// (tests)

#include "imgui.h"
#include "test_engine/imgui_te_context.h"

void RegisterTests_Window(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test whether capture tool window opens and closes.
    t = IM_REGISTER_TEST(e, "window", "open_capture_tool");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImGuiWindow* window = ctx->GetWindowByRef("Dear ImGui Capture Tool");
        ctx->WindowRef("##MainMenuBar");

        // If capture tool was not opened yet window would be null.
        bool is_active = window ? window->WasActive : false;
        ctx->MenuClick("View");
        ctx->MenuClick("View/Capture Tool");

        if (window == NULL)
            window = ctx->GetWindowByRef("Dear ImGui Capture Tool");
        IM_CHECK(window->WasActive != is_active);

        ctx->MenuClick("View");
        ctx->MenuClick("View/Capture Tool");
        IM_CHECK(window->WasActive == is_active);
    };
}
