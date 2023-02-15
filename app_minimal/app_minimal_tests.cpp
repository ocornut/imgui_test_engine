// dear imgui
// Demo Tests demonstrating the Dear ImGui Test Engine

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_test_engine/imgui_te_context.h"
#include "imgui_test_engine/imgui_capture_tool.h"

void RegisterAppMinimalTests(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    //-----------------------------------------------------------------
    // ## Demo Test: Hello Automation World
    //-----------------------------------------------------------------

    t = IM_REGISTER_TEST(e, "demo_tests", "test1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        IM_UNUSED(ctx);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Hello, automation world");
        ImGui::Button("Click Me");
        if (ImGui::TreeNode("Node"))
        {
            static bool b = false;
            ImGui::Checkbox("Checkbox", &b);
            ImGui::TreePop();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ctx->ItemClick("Click Me");
        ctx->ItemOpen("Node"); // Optional as ItemCheck("Node/Checkbox") can do it automatically
        ctx->ItemCheck("Node/Checkbox");
        ctx->ItemUncheck("Node/Checkbox");
    };

    //-----------------------------------------------------------------
    // ## Demo Test: Use variables to communicate data between GuiFunc and TestFunc
    //-----------------------------------------------------------------

    t = IM_REGISTER_TEST(e, "demo_tests", "test2");
    struct TestVars2 { int MyInt = 42; };
    t->SetVarsDataType<TestVars2>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TestVars2& vars = ctx->GetVars<TestVars2>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::SliderInt("Slider", &vars.MyInt, 0, 1000);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TestVars2& vars = ctx->GetVars<TestVars2>();
        ctx->SetRef("Test Window");

        IM_CHECK_EQ(vars.MyInt, 42);
        ctx->ItemInputValue("Slider", 123);
        IM_CHECK_EQ(vars.MyInt, 123);
    };

    //-----------------------------------------------------------------
    // ## Open Metrics window
    //-----------------------------------------------------------------

    t = IM_REGISTER_TEST(e, "demo_tests", "open_metrics");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Tools/Metrics\\/Debugger");
    };

    //-----------------------------------------------------------------
    // ## Capture entire Dear ImGui Demo window.
    //-----------------------------------------------------------------

    t = IM_REGISTER_TEST(e, "demo_tests", "capture_screenshot");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Widgets");       // Open collapsing header
        ctx->ItemOpenAll("Basic");      // Open tree node and all its descendant
        ctx->CaptureScreenshotWindow("Dear ImGui Demo", ImGuiCaptureFlags_StitchAll | ImGuiCaptureFlags_HideMouseCursor);
    };

    t = IM_REGISTER_TEST(e, "demo_tests", "capture_video");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MouseTeleportToPos(ctx->GetWindowByRef("")->Pos);

        ctx->CaptureAddWindow("Dear ImGui Demo"); // Optional: Capture single window
        ctx->CaptureBeginVideo();
        ctx->ItemOpen("Widgets");
        ctx->ItemInputValue("Basic/input text", "My first video!");
        ctx->CaptureEndVideo();
    };


}
