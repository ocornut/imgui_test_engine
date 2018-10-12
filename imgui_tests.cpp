// dear imgui
// (tests)

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_tests.h"
#include "imgui_test_engine.h"

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4100) // unreferenced formal parameter
#endif

// Generic structure with varied data.
// This is useful for tests to quickly share data between the GUI functions and the Test function.
struct DataGeneric
{
    int         Int1, Int2;
    int         IntArray[10];
    float       Float1, Float2;
    float       FloatArray[10];
    bool        Bool1, Bool2;
    bool        BoolArray[10];
    char        Str256[256];
    void*       Ptr1, *Ptr2;
    void*       PtrArray[10];
    void*       UserData;

    DataGeneric() { memset(this, 0, sizeof(*this)); }
};

void RegisterTests_Window(ImGuiTestContext* ctx)
{
    ImGuiTest* t = NULL;

    t = ctx->RegisterTest("window", "empty");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::GetStyle().WindowMinSize = ImVec2(10, 10);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::Begin("Test Window");
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        IM_CHECK(window->Size.x == ImMax(style.WindowMinSize.x, style.WindowPadding.x * 2.0f));
        IM_CHECK(window->Size.y == ImGui::GetFontSize() + style.FramePadding.y * 2.0f + style.WindowPadding.y * 2.0f);
        IM_CHECK(window->Size.y == window->SizeContents.y);
        IM_CHECK(window->Scroll.x == 0.0f && window->Scroll.y == 0.0f);
        ImGui::End();
    };
}

void RegisterTests_Scrolling(ImGuiTestContext* ctx)
{
    ImGuiTest* t = NULL;

    t = ctx->RegisterTest("scrolling", "scrolling_001");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Scrolling", NULL, ImGuiWindowFlags_NoSavedSettings);
        for (int n = 0; n < 100; n++)
            ImGui::Text("Hello %d\n", n);
        ImGui::SetScrollHereY(0.0f);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Scrolling");
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        IM_ASSERT(window->SizeContents.y > 0.0f);
        float scroll_y = ImGui::GetScrollY();
        IM_CHECK(scroll_y > 0.0f);
        IM_CHECK(scroll_y == ImGui::GetScrollMaxY());       // #1804
        ctx->Log("scroll_y = %f\n", scroll_y);
        ImGui::End();
    };
}

//static void gui_func_demo(ImGuiTestContext*)
//{
//    ImGui::ShowDemoWindow();
//}

#if 1
void RegisterTests_Misc(ImGuiTestContext* ctx)
{
    ImGuiTest* t = NULL;
    auto run_func = [](ImGuiTestContext* ctx)
    {
        DataGeneric data;
        data.Bool1 = false;
        ctx->RunCurrentTest(&data);
    };
    auto gui_func = [](ImGuiTestContext* ctx)
    {
        auto data = (DataGeneric*)ctx->UserData;
        ImGui::Begin("Window1");
        ImGui::Checkbox("Checkbox", &data->Bool1);
        ImGui::End();
    };

    t = ctx->RegisterTest("checkbox", "checkbox_001");
    t->RootFunc = run_func;
    t->GuiFunc = gui_func;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto data = (DataGeneric*)ctx->UserData;
        IM_CHECK(data->Bool1 == false);
        ctx->ItemClick("Window1" "Checkbox");
        IM_CHECK(data->Bool1 == true);
    };

    t = ctx->RegisterTest("demo", "demo_001");
    t->GuiFunc = NULL;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->ItemOpen("ImGui Demo" "Widgets");
        ctx->SleepShort();
        ctx->ItemOpen("ImGui Demo" "Basic");
        ctx->SleepShort();
        ctx->ItemClick("ImGui Demo" "Basic" "Button");
        ctx->SleepShort();
        ctx->ItemClick("ImGui Demo" "Basic" "radio a");
        ctx->SleepShort();
        ctx->ItemClick("ImGui Demo" "Basic" "radio b");
        ctx->SleepShort();
        ctx->ItemClick("ImGui Demo" "Basic" "radio c");
        ctx->SleepShort();
        ctx->ItemClick("ImGui Demo" "Basic" "combo");
        ctx->SleepShort();
        ctx->ItemClick("ImGui Demo" "Basic" "combo");
        ctx->SleepShort();
        ctx->ItemClick("ImGui Demo" "Basic" "color 2" "##ColorButton");
        //ctx->ItemClick("##Combo" "BBBB");     // id chain
        ctx->SleepShort();
        ctx->PopupClose();

        //ctx->ItemClick("ImGui Demo" "Layout");  // FIXME: close popup
        ctx->ItemOpen("ImGui Demo" "Layout");
        ctx->ItemOpen("ImGui Demo" "Horizontal Scrolling");
        ctx->ItemHold("ImGui Demo" "Horizontal Scrolling" ">>", 1.0f);
        ctx->SleepShort();
    };

#if 0
    t = ImGuiTestEngine_AddTest(ctx, "misc", "demo_002_scroll");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->ItemOpen("ImGui Demo" "Widgets");
        ctx->SleepShort();
        ctx->ItemOpen("ImGui Demo" "Basic");
        ctx->SleepShort();
        ctx->ItemClick("ImGui Demo" "Basic" "color 2" "##ColorButton");
        ctx->SleepShort();
    };
#endif

    t = ctx->RegisterTest("demo", "demo_003_menu");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->ItemClick("ImGui Demo" "##menubar" "Menu");
        ctx->SleepShort();
        ctx->MouseMove("ImGui Demo" "##menubar" "Examples");
        ctx->SleepShort();
        ctx->ItemClick("##Menu_00" "Console");
        ctx->SleepShort();
        ctx->ItemClick("Example: Console" "Add Dummy Text");
        ctx->SleepShort();
        ctx->ItemClick("Example: Console" "Add Dummy Error");
        ctx->SleepShort();
        ctx->ItemClick("Example: Console" "#CLOSE");
    };
}
#endif

void RegisterTests(ImGuiTestEngine* e)
{
    ImGuiTestContext ctx;
    ctx.Engine = e;
    ctx.UiContext = NULL;

    RegisterTests_Window(&ctx);
    RegisterTests_Scrolling(&ctx);
    RegisterTests_Misc(&ctx);
}

#if 0
struct ImGuiTestData
{
    int         GenericInt[10];
    float       GenericFloat[10];
    bool        GenericBool[10];
    char        GenericCharBuf256[256];
    void*       UserData;
};

void GuiFunc_001(ImGuiTestData* data)
{
    ImGui::SetNextWindowPos(ImVec2(100, 100));
    ImGui::Begin("Window1");
    ImGui::Text("This is a test.");
    ImGui::Button("Button1");
    ImGui::DragInt("Drag Int", &data->GenericInt[0]);
    ImGui::End();
}

void Test_Drag_001(ImGuiTestEngine* te)
{
    ImGuiTestData data;
    data.GenericInt[0] = 50;
    GuiFunc_001(&data);

    // MouseMove("Window1", "Drag Int")
    // MouseClickDrag(ImVec2(-100, 0))
    // IM_ASSERT(data.GenericInt[0] < 50)
}

void Test_Button_001(ImGuiTestEngine* te)
{
    ImGuiTestData data;
    GuiFunc_001(&data);

    //ts->TestName = __FUNCTION__;

    //const char* id = "Window1" "Button1";
    //const char* id = "ImGui Demo" "Widgets";
    //TT_MouseMove(te, id);
    //te->MouseMove(id);

    // ItemLocate(it)
    // - core engine task (hook ItemAdd)

    // ItemClick(id)
    // - MouseMove(id)
    //   - ItemLocate(id)
    //   - Scroll
    //   - Move cursor
    //   - validate hovering
    // - MouseClick()

    // ItemOpen(id) / TreeNodeOpen(id)
    // - ItemLocate(id)
    // - if (window->GetStateStorage(id) == 0)
    //    - ItemClick()

    // CheckboxCheck(id)
    // - ItemLocate(id)
    // ?? - add ImGuiItemStatusFlags_NotClipped
    // ?? - code sets ImGuiItemStatusFlags_Selected // VALID IF NOT CLIPPED
    // ?? - actual flags polled on NEXT ItemAdd.. dodgy

    // InputTextFill(id, ...)
    // - ItemActivate/ItemClick(id)
    // - Send keys e.g. CTRL+A, Delete
    // - Send keys "my keywords

    // InputTextSubmit()
    // - InputTextFill
    // - Send key: Return

    // MenuActivate("path")
    // - WindowFocus
    // - ItemClick
    // - ItemClick
    // - etc.

    // WindowFocus("ImGui Demo")
    // - FocusWindow/BringToFront <- bypass z-sorting

    // CaptureItem
    // - ItemLocate
    // - FocusWindow
    // - CaptureRect

    // id/scope
    // - fully qualified
    // - implicit relative to window
    // - implicit relative to last interacted item?

    // GuiFunc = ShowDemoWindow()
    // WindowFocus("ImGui Demo")
    // ItemOpen("Widgets")
    // ItemOpen("Basic")
    // CaptureItem("Drag Int", "drag.png")
    // ScrollToItem("Drag Int", 0.5f)
    // CaptureWindow("ImGui Demo", "demo.png")

    // MenuActivate("ImGui Demo" "Examples" "Console"
    // WindowSelect("Example: Console")
    // InputTextSubmit("Input", "HELP")
    // InputTextSubmit("Input", "HISTORY")
    // CaptureWindow(0, "filename.png") --> capture to PNG (padding set in some config structure)

    // MenuActivate("ImGui Demo" "Examples" "Console"
    // window = WindowFocus("Example: Console")
    // child = GetChildByIndex(window, 0) / GetChildByName(window, "##scrolling")
    // TesterDrawOutput tester(child->DrawList)
    // count = tester.Count()
    // InputTextSubmit("Input", "HELP")
    // InputTextSubmit("Input", "HISTORY")
    // ASSERT(tester.Count() > count)

    // (glitch testing -> only two variations over a span of frames) // fixme: if style allows e.g. border e.g. active/hovered, our test fails
    // TesterDrawOutput tester;
    // tester.LogHashPosUV()     // background task log every frame
    // MenuActivate("ImGui Demo" "Menu")
    // WaitFrames(10)
    // ASSERT(tester.LogUniqueCount() == 2)

    // (error handling behavior)
    // -> by default, failures are auto reported + task/stack unwind
    // -> be able to override this

    // test IsItemHovered() for windows title bar
    // GuiFunc()
    // {
    //    Begin("Blah")
    //    data->GenericBool[0] = IsItemHovered()
    //    data->GenericBool[1] = IsWindowHovered()
    //    End()
    // }
    // TestFunc:
    //  WindowMove("Blah", ImVec2(100,100)
    //  MouseMove(ImVec2(10,10))
    //  ASSERT(g.HoveredWindow != window)
    //  ASSERT(data->GenericBool[1] == false)
    //  MouseMove(window->TitleBarRect().Center)
    //  ASSERT(g.HoveredWindow == window)
    //  ASSERT(data->GenericBool[0] == true)
    //  ASSERT(data->GenericBool[1] == true)

    // #1909 test window size
    // MenuActivate("ImGui Demo" "Menu" "Open Recent")
    // window = GetTopLevelWindow()
    // ASSERT(window.Size.y == style.WindowPadding.y * 2 + GetFrameHeight() * 4))

    // #1500 resize small
    // MenuActivate("ImGui Demo", "Menu", "Console")
    // window = GetTopLevelWindow()
    // ASSERT(window->Name == "Examples: Console)
    // WindowResize(window, ImVec2(10,10))  // fixme: disable error
    // ASSERT(window->Size == ImMax(style.WindowMinSize, ImVec2(10,10))
}

#endif
