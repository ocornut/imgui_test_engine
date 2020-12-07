// dear imgui editor demo
// (tests)

#include "imgui.h"
#include "test_engine/imgui_te_context.h"
#include "shared/imgui_capture_tool.h"

void RegisterTests(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test whether capture tool window opens and closes.
    t = IM_REGISTER_TEST(e, "window", "open_capture_tool");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("##MainMenuBar");
        ctx->MenuCheck("View/Capture Tool");
    };

    // ## Capture entire Dear ImGui Demo window.
    t = IM_REGISTER_TEST(e, "capture", "capture_imgui_demo");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("##MainMenuBar");
        ctx->MenuCheck("View/Demo: Dear ImGui Demo");
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpenAll("");
        ImGuiCaptureArgs args;
        ctx->CaptureInitArgs(&args, ImGuiCaptureFlags_StitchFullContents | ImGuiCaptureFlags_HideMouseCursor | ImGuiCaptureFlags_HideCaptureToolWindow);
        args.InPadding = 16.0f;
        ctx->CaptureAddWindow(&args, "Dear ImGui Demo");
        ctx->CaptureScreenshot(&args);
    };

    // ## Capture entire ImPlot Demo window.
    t = IM_REGISTER_TEST(e, "capture", "capture_implot_demo");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("##MainMenuBar");
        ctx->MenuCheck("View/Demo: ImPlot");
        ctx->SetRef("ImPlot Demo");
        ctx->ItemOpenAll("");
        ImGuiCaptureArgs args;
        ctx->CaptureInitArgs(&args, ImGuiCaptureFlags_StitchFullContents | ImGuiCaptureFlags_HideMouseCursor | ImGuiCaptureFlags_HideCaptureToolWindow);
        args.InPadding = 16.0f;
        ctx->CaptureAddWindow(&args, "ImPlot Demo");
        ctx->CaptureScreenshot(&args);
    };
}
