// dear imgui editor demo
// (tests)

#include "imgui.h"
#include "test_engine/imgui_te_context.h"

void RegisterTests(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test whether capture tool window opens and closes.
    t = IM_REGISTER_TEST(e, "window", "open_capture_tool");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("##MainMenuBar");
        ctx->MenuCheck("View/Capture Tool");
    };
}
