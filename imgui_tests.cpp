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

#define REGISTER_TEST(_CATEGORY, _NAME)    ctx->RegisterTest(_CATEGORY, _NAME, __FILE__, __LINE__);

void RegisterTests_Window(ImGuiTestContext* ctx)
{
    ImGuiTest* t = NULL;

    t = REGISTER_TEST("window", "empty");
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

    t = REGISTER_TEST("scrolling", "scrolling_001");
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

    t = REGISTER_TEST("checkbox", "checkbox_001");
    t->RootFunc = run_func;
    t->GuiFunc = gui_func;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto data = (DataGeneric*)ctx->UserData;
        IM_CHECK(data->Bool1 == false);
        ctx->ItemClick("Window1/Checkbox");
        IM_CHECK(data->Bool1 == true);
    };

    t = REGISTER_TEST("demo", "demo_misc_001");
    t->GuiFunc = NULL;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Basic");
        ctx->ItemClick("Basic/Button");
        ctx->ItemClick("Basic/radio a");
        ctx->ItemClick("Basic/radio b");
        ctx->ItemClick("Basic/radio c");
        ctx->ItemClick("Basic/combo");
        ctx->ItemClick("Basic/combo");
        ctx->ItemClick("Basic/color 2/##ColorButton");
        //ctx->ItemClick("##Combo/BBBB");     // id chain
        ctx->SleepShort();
        ctx->PopupClose();

        //ctx->ItemClick("Layout");  // FIXME: close popup
        ctx->ItemOpen("Layout");
        ctx->ItemOpen("Horizontal Scrolling");
        ctx->ItemHold("Horizontal Scrolling/>>", 1.0f);
        ctx->SleepShort();
    };

    t = REGISTER_TEST("demo", "demo_cov_auto_open");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->ItemOpenAllRecurse("", 999);
        //ctx->ItemOpenAllRecurse("", 1, 1);
    };

    t = REGISTER_TEST("demo", "demo_cov_auto_close");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->ItemCloseAllRecurse("", 999);
    };

    t = REGISTER_TEST("demo", "demo_cov_001");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->ItemOpen("Help");
        ctx->ItemOpen("Configuration");
        ctx->ItemOpen("Window options");
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Layout");
        ctx->ItemOpen("Popups & Modal windows");
        ctx->ItemOpen("Columns");
        ctx->ItemOpen("Filtering");
        ctx->ItemOpen("Inputs, Navigation & Focus");
    };

    t = REGISTER_TEST("demo", "demo_cov_002");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->MenuClick("Menu/Open Recent/More..");

        //ctx->ItemClick("$REF/Main menu bar");
        //ctx->ItemClick("$NAV/Main menu bar");
        //ctx->ItemClick("$TOP/Main menu bar");

        for (int n = 0; n < 2; n++)
        {
            ImGuiTestAction action = (n == 0) ? ImGuiTestAction_Check : ImGuiTestAction_Uncheck;
            ctx->MenuAction(action, "Examples/Main menu bar");
            ctx->MenuAction(action, "Examples/Console");
            ctx->MenuAction(action, "Examples/Log");
            ctx->MenuAction(action, "Examples/Simple layout");
            ctx->MenuAction(action, "Examples/Property editor");
            ctx->MenuAction(action, "Examples/Long text display");
            ctx->MenuAction(action, "Examples/Auto-resizing window");
            ctx->MenuAction(action, "Examples/Constrained-resizing window");
            ctx->MenuAction(action, "Examples/Simple overlay");
            ctx->MenuAction(action, "Examples/Manipulating window titles");
            ctx->MenuAction(action, "Examples/Custom rendering");

            ctx->MenuAction(action, "Help/Metrics");
            ctx->MenuAction(action, "Help/Style Editor");
            ctx->MenuAction(action, "Help/About Dear ImGui");
        }
    };

    t = REGISTER_TEST("demo", "console");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->MenuCheck("Examples/Console");

        ctx->SetRef("Example: Console");
        ctx->ItemClick("Clear");
        ctx->ItemClick("Add Dummy Text");
        ctx->ItemClick("Add Dummy Error");
        ctx->WindowClose();

        //IM_CHECK(false);
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

// Notes
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

    // ItemLocate(it) --> OK
    // - core engine task (hook ItemAdd)

    // ItemClick(id) --> OK
    // - MouseMove(id)
    //   - ItemLocate(id)
    //   - Scroll
    //   - Move cursor
    //   - validate hovering
    // - MouseClick()

    // ItemOpen(id) / TreeNodeOpen(id) --> OK
    // - ItemLocate(id)
    // - if (window->GetStateStorage(id) == 0)
    //    - ItemClick()

    // CheckboxCheck(id) --> OK
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

    // MenuActivate("path") --> OK
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

    // MenuActivate("ImGui Demo/Examples/Console"
    // WindowSelect("Example: Console")
    // InputTextSubmit("Input", "HELP")
    // InputTextSubmit("Input", "HISTORY")
    // CaptureWindow(0, "filename.png") --> capture to PNG (padding set in some config structure)

    // MenuActivate("ImGui Demo/Examples/Console"
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
    // MenuActivate("ImGui Demo/Menu")
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
    // MenuActivate("ImGui Demo/Menu/Open Recent")
    // window = GetTopLevelWindow()
    // ASSERT(window.Size.y == style.WindowPadding.y * 2 + GetFrameHeight() * 4))

    // #1500 resize small
    // MenuActivate("ImGui Demo", "Menu", "Console")
    // window = GetTopLevelWindow()
    // ASSERT(window->Name == "Examples: Console)
    // WindowResize(window, ImVec2(10,10))  // FIXME: disable error
    // ASSERT(window->Size == ImMax(style.WindowMinSize, ImVec2(10,10))
}

#endif
