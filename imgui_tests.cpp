// dear imgui
// (tests)

#define _CRT_SECURE_NO_WARNINGS
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_tests.h"
#include "imgui_te_core.h"

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4100) // unreferenced formal parameter
#endif

// Helper Operators
static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
static inline bool operator!=(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x != rhs.x && lhs.y != rhs.y; }

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
    void*       Ptr1;
    void*       Ptr2;
    void*       PtrArray[10];
    void*       UserData;

    DataGeneric() { memset(this, 0, sizeof(*this)); }
};

#define REGISTER_TEST(_CATEGORY, _NAME)    ctx->RegisterTest(_CATEGORY, _NAME, __FILE__, __LINE__);

//-------------------------------------------------------------------------
// Tests: Window
//-------------------------------------------------------------------------

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
        ImGuiWindow* window = ImGui::FindWindowByName("Test Window");
        IM_CHECK(window->Size.x == ImMax(style.WindowMinSize.x, style.WindowPadding.x * 2.0f));
        IM_CHECK(window->Size.y == ImGui::GetFontSize() + style.FramePadding.y * 2.0f + style.WindowPadding.y * 2.0f);
        IM_CHECK(window->Size.y == window->SizeContents.y);
        IM_CHECK(window->Scroll.x == 0.0f && window->Scroll.y == 0.0f);
    };

    t = REGISTER_TEST("window", "window_auto_resize_collapse");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // FIXME-TESTS: Ideally we'd like a variant with/without the if (Begin) here
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Some text\nOver two lines\nOver three lines");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window = ImGui::FindWindowByName("Test Window");
        ctx->SetRef("Test Window");
        ctx->WindowSetCollapsed(false);
        ctx->LogVerbose("Size %f %f, SizeFull %f %f\n", window->Size.x, window->Size.y, window->SizeFull.x, window->SizeFull.y);
        IM_CHECK(window->Size == window->SizeFull);
        ImVec2 size_full_when_uncollapsed = window->SizeFull;
        ctx->WindowSetCollapsed(true);
        ctx->LogVerbose("Size %f %f, SizeFull %f %f\n", window->Size.x, window->Size.y, window->SizeFull.x, window->SizeFull.y);
        ImVec2 size_collapsed = window->Size;
        IM_CHECK(size_full_when_uncollapsed.y > size_collapsed.y);
        IM_CHECK(size_full_when_uncollapsed == window->SizeFull);
        ctx->WindowSetCollapsed(false);
        ctx->LogVerbose("Size %f %f, SizeFull %f %f\n", window->Size.x, window->Size.y, window->SizeFull.x, window->SizeFull.y);
        IM_CHECK(window->Size.y == size_full_when_uncollapsed.y && "Window should have restored to full size.");
        ctx->Yield();
        IM_CHECK(window->Size.y == size_full_when_uncollapsed.y);
    };

    // Bug #2282
    t = REGISTER_TEST("window", "window_append");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Line 1");
        ImGui::End();
        ImGui::Begin("Test Window");
        ImGui::Text("Line 2");
        ImGui::BeginChild("Blah", ImVec2(0,50), true);
        ImGui::Text("Line 3");
        ImGui::EndChild();
        ImVec2 pos1 = ImGui::GetCursorScreenPos();
        ImGui::BeginChild("Blah");
        ImGui::Text("Line 4");
        ImGui::EndChild();
        ImVec2 pos2 = ImGui::GetCursorScreenPos();
        IM_CHECK(pos1 == pos2); // Append calls to BeginChild() shouldn't affect CursorPos in parent window
        ImGui::Text("Line 5");
        ImVec2 pos3 = ImGui::GetCursorScreenPos();
        ImGui::BeginChild("Blah");
        ImGui::Text("Line 6");
        ImGui::EndChild();
        ImVec2 pos4 = ImGui::GetCursorScreenPos();
        IM_CHECK(pos3 == pos4); // Append calls to BeginChild() shouldn't affect CursorPos in parent window
        ImGui::End();
    };

    t = REGISTER_TEST("window", "window_focus_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("AAAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
        if ((ctx->FrameCount >= 20 && ctx->FrameCount < 40) || (ctx->FrameCount >= 50))
        {
            ImGui::Begin("BBBB", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::End();
            ImGui::Begin("CCCC", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::End();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/AAAA"));
        ctx->YieldUntil(19);
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/AAAA"));
        ctx->YieldUntil(20);
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/CCCC"));
        ctx->YieldUntil(30);
        ctx->FocusWindow("/BBBB");
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/BBBB"));
        ctx->YieldUntil(40);
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/BBBB"));

        ctx->YieldUntil(41);
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/AAAA"));

        ctx->YieldUntil(49);
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/AAAA"));
        ctx->YieldUntil(50);
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/CCCC"));
    };
}

//-------------------------------------------------------------------------
// Tests: Scrolling
//-------------------------------------------------------------------------

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
        ctx->LogVerbose("scroll_y = %f\n", scroll_y);
        ImGui::End();
    };
}

//-------------------------------------------------------------------------
// Tests: Widgets
//-------------------------------------------------------------------------

void RegisterTests_Widgets(ImGuiTestContext* ctx)
{
    ImGuiTest* t = NULL;

    t = REGISTER_TEST("widgets", "widgets_inputtext");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericState& gs = ctx->GenericState;
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("InputText", gs.Str256, IM_ARRAYSIZE(gs.Str256));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        char* buf = ctx->GenericState.Str256;

        ctx->SetRef("Test Window");

        // Insert
        strcpy(buf, "Hello");
        ctx->ItemClick("InputText");
        ctx->KeyCharsInputAppend("World123");
        IM_CHECK(strcmp(buf, "HelloWorld123") == 0);

        // Delete
        ctx->ItemClick("InputText");
        ctx->KeyPressMap(ImGuiKey_End);
        ctx->KeyPressMap(ImGuiKey_Backspace, 3);
        ctx->KeyPressMap(ImGuiKey_Enter);
        IM_CHECK(strcmp(buf, "HelloWorld") == 0);

        // Insert, Cancel
        ctx->ItemClick("InputText");
        ctx->KeyPressMap(ImGuiKey_End);
        ctx->KeyChars("XXXXX");
        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK(strcmp(buf, "HelloWorld") == 0);

        // Delete, Cancel
        ctx->ItemClick("InputText");
        ctx->KeyPressMap(ImGuiKey_End);
        ctx->KeyPressMap(ImGuiKey_Backspace, 5);
        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK(strcmp(buf, "HelloWorld") == 0);
    };

    t = REGISTER_TEST("widgets", "widgets_coloredit_drag");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericState& gs = ctx->GenericState;
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::ColorEdit4("ColorEdit1", &gs.Vec4Array[0].x, ImGuiColorEditFlags_None);
        ImGui::ColorEdit4("ColorEdit2", &gs.Vec4Array[1].x, ImGuiColorEditFlags_None);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericState& gs = ctx->GenericState;
        gs.Vec4Array[0] = ImVec4(1, 0, 0, 1);
        gs.Vec4Array[1] = ImVec4(0, 1, 0, 1);

        ctx->SetRef("Test Window");

        IM_CHECK(memcmp(&gs.Vec4Array[0], &gs.Vec4Array[1], sizeof(ImVec4)) != 0);
        ctx->ItemDragAndDrop("ColorEdit1/##ColorButton", "ColorEdit2/##X"); // FIXME-TESTS: Inner items
        IM_CHECK(memcmp(&gs.Vec4Array[0], &gs.Vec4Array[1], sizeof(ImVec4)) == 0);

        ctx->Sleep(1.0f);
    };
}

//-------------------------------------------------------------------------
// Tests: Nav
//-------------------------------------------------------------------------

//static void gui_func_demo(ImGuiTestContext*)
//{
//    ImGui::ShowDemoWindow();
//}

void RegisterTests_Nav(ImGuiTestContext* ctx)
{
    ImGuiTest* t = NULL;

    // Test opening a new window from a checkbox setting the focus to the new window.
    // In 9ba2028 (2019/01/04) we fixed a bug where holding ImGuiNavInputs_Activate too long on a button would hold the focus on the wrong window.
    t = REGISTER_TEST("nav", "nav_001");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetInputMode(ImGuiInputSource_Nav);
        ctx->SetRef("Hello, world!");
        ctx->ItemUncheck("Demo Window");
        ctx->ItemCheck("Demo Window");

        ImGuiContext& g = *ctx->UiContext;
        IM_CHECK(g.NavWindow && g.NavWindow->ID == ctx->GetID("/ImGui Demo"));
    };
}

//-------------------------------------------------------------------------
// Tests: Docking
//-------------------------------------------------------------------------

void RegisterTests_Docking(ImGuiTestContext* ctx)
{
#ifdef IMGUI_HAS_DOCK
    ImGuiTest* t = NULL;

    t = REGISTER_TEST("docking", "docking_drag");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Once);
        ImGui::Begin("AAAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("This is AAAA");
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(200, 200), ImGuiCond_Once);
        ImGui::Begin("BBBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("This is BBBB");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->ItemDragAndDrop("/AAAA/#COLLAPSE", "/BBBB/#COLLAPSE");
        ctx->SleepShort();
    };

    t = REGISTER_TEST("docking", "docking_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericState& gs = ctx->GenericState;
        if (ctx->FrameCount == 0)
        {
            ImGui::DockBuilderDockWindow("AAAA", 0);
            ImGui::DockBuilderDockWindow("BBBB", 0);
            ImGui::DockBuilderDockWindow("CCCC", 0);
        }
        if (ctx->FrameCount == 10)
        {
            ImGuiID dock_id = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_None);
            gs.Id = dock_id;
            ImGui::DockBuilderSetNodePos(dock_id, ImGui::GetMainViewport()->Pos + ImVec2(100, 100));
            ImGui::DockBuilderSetNodeSize(dock_id, ImVec2(200, 200));
            ImGui::DockBuilderDockWindow("AAAA", dock_id);
            ImGui::DockBuilderDockWindow("BBBB", dock_id);
            ImGui::DockBuilderDockWindow("CCCC", dock_id);
            ImGui::DockBuilderFinish(dock_id);
        }

        ImGuiID ids[3];
        ImGui::Begin("AAAA");
        ids[0] = ImGui::GetWindowDockID();
        ImGui::Text("This is AAAA");
        ImGui::End();

        ImGui::Begin("BBBB");
        ids[1] = ImGui::GetWindowDockID();
        ImGui::Text("This is BBBB");
        ImGui::End();

        ImGui::Begin("CCCC");
        ids[2] = ImGui::GetWindowDockID();
        ImGui::Text("This is CCCC");
        ImGui::End();

        if (ctx->FrameCount == 1)
        {
            IM_CHECK(ids[0] == 0);
            IM_CHECK(ids[0] == ids[1] && ids[0] == ids[2]);
        }
        if (ctx->FrameCount == 10)
        {
            IM_CHECK(gs.Id != 0);
            IM_CHECK(ids[0] == gs.Id);
            IM_CHECK(ids[0] == ids[1] && ids[0] == ids[2]);
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->YieldFrames(20);
    };
#endif
}

//-------------------------------------------------------------------------
// Tests: Misc
//-------------------------------------------------------------------------

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
        ctx->ItemOpenAll("");
    };

    t = REGISTER_TEST("demo", "demo_cov_auto_close");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->ItemCloseAll("");
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

    t = REGISTER_TEST("demo", "demo_cov_apps");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->MenuClick("Menu/Open Recent/More..");

#if 1
        ctx->MenuCheckAll("Examples");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuCheckAll("Help");
        ctx->MenuUncheckAll("Help");
#else
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
#endif
    };

    t = REGISTER_TEST("demo", "demo_cov_styles");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->MenuAction(ImGuiTestAction_Check, "Help/Style Editor");

        ImGuiTestRef ref_window = "Style Editor";
        ctx->SetRef(ref_window);
        ctx->ItemClick("Colors##Selector");
        ctx->Yield();
        ImGuiTestRef ref_popup = ctx->GetFocusWindowRef();

        ImGuiStyle style_backup = ImGui::GetStyle();
        ImGuiTestItemList items;
        ctx->GatherItems(&items, ref_popup);
        for (int n = 0; n < items.Size; n++)
        {
            ctx->SetRef(ref_window);
            ctx->ItemClick("Colors##Selector");
            ctx->SetRef(ref_popup);
            ctx->ItemClick(items[n]->ID);
        }
        ImGui::GetStyle() = style_backup;
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

//-------------------------------------------------------------------------
// Tests: Perf
//-------------------------------------------------------------------------

void RegisterTests_Perf(ImGuiTestContext* ctx)
{
    ImGuiTest* t = NULL;

    auto PerfCaptureFunc = [](ImGuiTestContext* ctx) 
    {
        ctx->PerfCapture();
    };

    t = REGISTER_TEST("perf", "perf_demo_all");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->PerfCalcRef();

        ctx->SetRef("ImGui Demo");
        ctx->ItemOpenAll("");
        ctx->MenuCheckAll("Examples");
        ctx->MenuCheckAll("Help");

        IM_CHECK(ctx->UiContext->IO.DisplaySize.x > 820);
        IM_CHECK(ctx->UiContext->IO.DisplaySize.y > 820);

        // FIXME-TESTS: Backup full layout
        ImVec2 pos = ctx->GetMainViewportPos() + ImVec2(20, 20);
        for (ImGuiWindow* window : ctx->UiContext->Windows)
        {
            window->Pos = pos;
            window->SizeFull = ImVec2(800, 800);
        }
        ctx->PerfCapture();

        ctx->SetRef("ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuUncheckAll("Help");
    };

    enum
    {
        DrawPrimFunc_RectOutline,
        DrawPrimFunc_RectFilled,
        DrawPrimFunc_RectRoundedOutline,
        DrawPrimFunc_RectRoundedFilled,
        DrawPrimFunc_CircleOutline,
        DrawPrimFunc_CircleFilled,
    };

    auto DrawPrimFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 200 * ctx->PerfStressAmount;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        int segments = 12;
        ImGui::Button("##CircleFilled", ImVec2(200, 200));
        ImVec2 bounds_min = ImGui::GetItemRectMin();
        ImVec2 bounds_size = ImGui::GetItemRectSize();
        ImVec2 center = bounds_min + bounds_size * 0.5f;
        float r = (float)(int)(ImMin(bounds_size.x, bounds_size.y) * 0.8f * 0.5f);
        float rounding = 8.0f;
        ImU32 col = IM_COL32(255, 255, 0, 255);
        switch (ctx->Test->ArgVariant)
        {
        case DrawPrimFunc_RectOutline:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r,r), center + ImVec2(r,r), col, 0.0f);
            break;
        case DrawPrimFunc_RectFilled:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRectFilled(center - ImVec2(r, r), center + ImVec2(r, r), col, 0.0f);
            break;
        case DrawPrimFunc_RectRoundedOutline:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r, r), center + ImVec2(r, r), col, rounding);
            break;
        case DrawPrimFunc_RectRoundedFilled:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRectFilled(center - ImVec2(r, r), center + ImVec2(r, r), col, rounding);
            break;
        case DrawPrimFunc_CircleOutline:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddCircle(center, r, col, segments);
            break;
        case DrawPrimFunc_CircleFilled:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddCircleFilled(center, r, col, segments);
            break;
        default:
            IM_ASSERT(0);
        }
        ImGui::End();
    };

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_outline");
    t->ArgVariant = DrawPrimFunc_RectOutline;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_filled");
    t->ArgVariant = DrawPrimFunc_RectFilled;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_rounded_outline");
    t->ArgVariant = DrawPrimFunc_RectRoundedOutline;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_rounded_filled");
    t->ArgVariant = DrawPrimFunc_RectRoundedFilled;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_circle_outline");
    t->ArgVariant = DrawPrimFunc_CircleOutline;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_circle_filled");
    t->ArgVariant = DrawPrimFunc_CircleFilled;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_button");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
            ImGui::Button("Hello, world");
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_checkbox");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        bool v1 = false, v2 = true;
        for (int n = 0; n < loop_count / 2; n++)
        {
            ImGui::PushID(n);
            ImGui::Checkbox("Hello, world", &v1);
            ImGui::Checkbox("Hello, world", &v2);
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_input_text");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        char buf[32] = "123";
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            ImGui::InputText("InputText", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_None);
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_input_text_multiline");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        char buf[32] = "123";
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            ImGui::InputTextMultiline("InputText", buf, IM_ARRAYSIZE(buf), ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 2), ImGuiInputTextFlags_None);
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_hash");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 5000 * ctx->PerfStressAmount;
        char buf[32] = { 0 };
        ImU32 seed = 0;
        for (int n = 0; n < loop_count; n++)
        {
            seed = ImHashData(buf, 32, seed);
            seed = ImHashStr("Hash me tender", 0, seed);
            seed = ImHashStr("Hash me true", 12, seed);
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_list_box");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            if (ImGui::ListBoxHeader("ListBox", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 2)))
            {
                ImGui::MenuItem("Hello");
                ImGui::MenuItem("World");
                ImGui::ListBoxFooter();
            }
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_slider");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        float f = 1.234f;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            ImGui::SliderFloat("SliderFloat", &f, 0.0f, 10.0f);
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_slider2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        ImVec2 v(0.0f, 0.0f);
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            ImGui::SliderFloat2("SliderFloat2", &v.x, 0.0f, 10.0f);
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_text_unformatted");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        const char* buf =
            "0123456789 The quick brown fox jumps over the lazy dog.\n"
            "0123456789   The quick brown fox jumps over the lazy dog.\n"
            "0123456789     The quick brown fox jumps over the lazy dog.\n";
        int loop_count = 1000 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
            ImGui::TextUnformatted(buf);
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_window");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImVec2 pos = ctx->GetMainViewportPos() + ImVec2(20, 20);
        int loop_count = 200 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            char window_name[32];
            ImFormatString(window_name, IM_ARRAYSIZE(window_name), "Window_%05d", n+1);
            ImGui::SetNextWindowPos(pos);
            ImGui::Begin(window_name, NULL, ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextUnformatted("Opening many windows!");
            ImGui::End();
        }
    };
    t->TestFunc = PerfCaptureFunc;
}

//-------------------------------------------------------------------------
// Tests: Capture
//-------------------------------------------------------------------------

void RegisterTests_Capture(ImGuiTestContext* ctx)
{
    ImGuiTest* t = NULL;

    t = REGISTER_TEST("capture", "capture_demo_documents");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->MenuAction(ImGuiTestAction_Check, "Examples/Documents");

        ctx->SetRef("Examples: Documents");
        // FIXME-TESTS: Locate within stack that uses windows/<pointer>/name
        ctx->ItemCheck("Tomato"); // FIXME: WILL FAIL, NEED TO LOCATE BY NAME (STACK WILDCARD?)
        ctx->ItemCheck("A Rather Long Title");
        ctx->ItemClick("##tabs/Eggplant");
        ctx->MouseMove("##tabs/Eggplant/Modify");
        ctx->Sleep(1.0f);
    };

#if 1

    // TODO: Better position of windows.
    // TODO: Draw in custom rendering canvas
    // TODO: Select color picker mode
    // TODO: Flags document as "Modified"
    // TODO: InputText selection
    t = REGISTER_TEST("capture", "capture_readme_misc");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiIO& io = ImGui::GetIO();
        //ImGuiStyle& style = ImGui::GetStyle();

        ctx->SetRef("ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuCheck("Examples/Simple overlay");
        ctx->SetRef("Example: Simple overlay");
        ImGuiWindow* window_overlay = ctx->GetRefWindow();
        IM_CHECK_ABORT(window_overlay != NULL);

        // FIXME-TESTS: Find last newly opened window?

        float fh = ImGui::GetFontSize();
        float pad = fh;

        ctx->SetRef("ImGui Demo");
        ctx->MenuCheck("Examples/Custom rendering");
        ctx->SetRef("Example: Custom rendering");
        ctx->WindowResize(ImVec2(fh * 30, fh * 30));
        ctx->WindowMove(window_overlay->Rect().GetBL() + ImVec2(0.0f, pad));
        ImGuiWindow* window_custom_rendering = ctx->GetRefWindow();
        IM_CHECK_ABORT(window_custom_rendering != NULL);

        ctx->SetRef("ImGui Demo");
        ctx->MenuCheck("Examples/Console");
        ctx->SetRef("Example: Console");
        ctx->WindowResize(ImVec2(fh * 42, fh * (34-20)));
        ctx->WindowMove(window_custom_rendering->Pos + window_custom_rendering->Size * ImVec2(0.35f, 0.55f));
        ctx->ItemClick("Clear");
        ctx->ItemClick("Add Dummy Text");
        ctx->ItemClick("Add Dummy Error");
        ctx->ItemClick("Input");

        ctx->KeyChars("H");
        ctx->KeyPressMap(ImGuiKey_Tab);
        ctx->KeyCharsInputAppend("ELP");
        ctx->KeyCharsInputAppend("hello, imgui world!");

        ctx->SetRef("ImGui Demo");
        ctx->MenuCheck("Examples/Simple layout");
        ctx->SetRef("Example: Simple layout");
        ctx->WindowResize(ImVec2(fh * 50, fh * 15));
        ctx->WindowMove(ImVec2(pad, io.DisplaySize.y - pad), ImVec2(0.0f, 1.0f));

        ctx->SetRef("ImGui Demo");
        ctx->WindowResize(ImVec2(fh * 35, io.DisplaySize.y - pad * 2.0f));
        ctx->WindowMove(ImVec2(io.DisplaySize.x - pad, pad), ImVec2(1.0f, 0.0f));
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Color\\/Picker Widgets");
        ctx->ItemOpen("Layout");
        ctx->ItemOpen("Groups");
        ctx->ScrollToY("Layout", 0.8f);
    };
#endif
}

void RegisterTests(ImGuiTestEngine* e)
{
    ImGuiTestContext ctx;
    ctx.Engine = e;
    ctx.UiContext = NULL;

    RegisterTests_Window(&ctx);
    RegisterTests_Scrolling(&ctx);
    RegisterTests_Widgets(&ctx);
    RegisterTests_Nav(&ctx);
    RegisterTests_Docking(&ctx);
    RegisterTests_Misc(&ctx);
    RegisterTests_Perf(&ctx);
    RegisterTests_Capture(&ctx);
}

// Notes/Ideas
#if 0

//ctx->ItemClick("$REF/Main menu bar");
//ctx->ItemClick("$NAV/Main menu bar");
//ctx->ItemClick("$TOP/Main menu bar");

void GuiFunc_001(DataGeneric* data)
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
    DataGeneric data;
    data.GenericInt[0] = 50;
    GuiFunc_001(&data);

    // MouseMove("Window1", "Drag Int")
    // MouseClickDrag(ImVec2(-100, 0))
    // IM_ASSERT(data.GenericInt[0] < 50)
}

void Test_Button_001(ImGuiTestEngine* te)
{
    DataGeneric data;
    GuiFunc_001(&data);

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
