// dear imgui
// (tests)

#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_tests.h"
#include "shared/imgui_capture_tool.h"
#include "test_engine/imgui_te_core.h"
#include "test_engine/imgui_te_context.h"
#include "test_engine/imgui_te_util.h"
#include "libs/Str/Str.h"

//-------------------------------------------------------------------------
// NOTES (also see TODO in imgui_te_core.cpp)
//-------------------------------------------------------------------------
// - Tests can't reliably once ImGuiCond_Once or ImGuiCond_FirstUseEver
// - GuiFunc can't run code that yields. There is an assert for that.
//-------------------------------------------------------------------------

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4100) // unreferenced formal parameter
#pragma warning (disable: 4127) // conditional expression is constant
#endif

// Helpers
#define REGISTER_TEST(_CATEGORY, _NAME)                                 IM_REGISTER_TEST(e, _CATEGORY, _NAME)
static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs)     { return lhs.x == rhs.x && lhs.y == rhs.y; }    // for IM_CHECK_EQ()

//-------------------------------------------------------------------------
// Tests: Window
//-------------------------------------------------------------------------

void RegisterTests_Window(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test size of an empty window
    t = REGISTER_TEST("window", "empty");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        //ImGui::GetStyle().WindowMinSize = ImVec2(10, 10);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImGuiWindow* window = ImGui::FindWindowByName("Test Window");
        IM_CHECK_EQ(window->Size.x, ImMax(style.WindowMinSize.x, style.WindowPadding.x * 2.0f));
        IM_CHECK_EQ(window->Size.y, ImGui::GetFontSize() + style.FramePadding.y * 2.0f + style.WindowPadding.y * 2.0f);
        IM_CHECK_EQ(window->ContentSize, ImVec2(0, 0));
        IM_CHECK_EQ(window->Scroll, ImVec2(0, 0));
    };

    // ## Test that a window starting collapsed performs width/contents size measurement on its first few frames.
    t = REGISTER_TEST("window", "window_size_collapsed_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_Appearing);
        ImGui::Begin("Issue 2336", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("This is some text");
        ImGui::Text("This is some more text");
        ImGui::Text("This is some more text again");

        float w = ImGui::GetWindowWidth();
        if (ctx->FrameCount == 0) // We are past the warm-up frames already
        {
            float expected_w = ImGui::CalcTextSize("This is some more text again").x + ImGui::GetStyle().WindowPadding.x * 2.0f;
            IM_CHECK_LT(ImFabs(w - expected_w), 1.0f);
        }
        ImGui::End();
    };

    t = REGISTER_TEST("window", "window_size_contents");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        {
            ImGui::Begin("Test Contents Size 1", NULL, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::ColorButton("test", ImVec4(1, 0.4f, 0, 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(150, 150));
            ImGuiWindow* window = ctx->UiContext->CurrentWindow;
            if (ctx->FrameCount > 0)
                IM_CHECK_EQ(window->ContentSize, ImVec2(150.0f, 150.0f));
            ImGui::End();
        }
        {
            ImGui::SetNextWindowContentSize(ImVec2(150, 150));
            ImGui::Begin("Test Contents Size 2", NULL, ImGuiWindowFlags_AlwaysAutoResize);
            ImGuiWindow* window = ctx->UiContext->CurrentWindow;
            if (ctx->FrameCount >= 0)
                IM_CHECK_EQ(window->ContentSize, ImVec2(150.0f, 150.0f));
            ImGui::End();
        }
        {
            ImGui::SetNextWindowContentSize(ImVec2(150, 150));
            ImGui::SetNextWindowSize(ImVec2(150, 150) + style.WindowPadding * 2.0f + ImVec2(0.0f, ImGui::GetFrameHeight()));
            ImGui::Begin("Test Contents Size 3", NULL, ImGuiWindowFlags_None);
            ImGuiWindow* window = ctx->UiContext->CurrentWindow;
            if (ctx->FrameCount >= 0)
            {
                IM_CHECK(window->ScrollbarY == false);
                IM_CHECK_EQ(window->ScrollMax.y, 0.0f);
            }
            ImGui::End();
        }
        {
            ImGui::SetNextWindowContentSize(ImVec2(150, 150 + 1));
            ImGui::SetNextWindowSize(ImVec2(150, 150) + style.WindowPadding * 2.0f + ImVec2(0.0f, ImGui::GetFrameHeight()));
            ImGui::Begin("Test Contents Size 4", NULL, ImGuiWindowFlags_None);
            ImGuiWindow* window = ctx->UiContext->CurrentWindow;
            if (ctx->FrameCount >= 0)
            {
                IM_CHECK(window->ScrollbarY == true);
                IM_CHECK_EQ(window->ScrollMax.y, 1.0f);
            }
            ImGui::End();
        }

        if (ctx->FrameCount == 2)
            ctx->Finish();
    };

    // ## Test that non-integer size/position passed to window gets rounded down and not cause any drift.
    t = REGISTER_TEST("window", "window_size_unrounded");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // #2067
        {
            ImGui::SetNextWindowPos(ImVec2(901.0f, 103.0f), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(348.48f, 400.0f), ImGuiCond_Once);
            ImGui::Begin("Issue 2067", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();
            //ctx->LogDebug("%f %f, %f %f", pos.x, pos.y, size.x, size.y);
            IM_CHECK_NO_RET(pos.x == 901.0f && pos.y == 103.0f);
            IM_CHECK_NO_RET(size.x == 348.0f && size.y == 400.0f);
            ImGui::End();
        }
        // Test that non-rounded size constraint are not altering pos/size (#2530)
        {
            ImGui::SetNextWindowPos(ImVec2(901.0f, 103.0f), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(348.48f, 400.0f), ImGuiCond_Once);
            ImGui::SetNextWindowSizeConstraints(ImVec2(475.200012f, 0.0f), ImVec2(475.200012f, 100.4f));
            ImGui::Begin("Issue 2530", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();
            //ctx->LogDebug("%f %f, %f %f", pos.x, pos.y, size.x, size.y);
            IM_CHECK_EQ(pos, ImVec2(901.0f, 103.0f));
            IM_CHECK_EQ(size, ImVec2(475.0f, 100.0f));
            ImGui::End();
        }
        if (ctx->FrameCount == 2)
            ctx->Finish();
    };

    // ## Test basic window auto resize
    t = REGISTER_TEST("window", "window_auto_resize_basic");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // FIXME-TESTS: Ideally we'd like a variant with/without the if (Begin) here
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Hello World");
        ImGui::BeginChild("Child", ImVec2(0, 200));
        ImGui::EndChild();
        ImVec2 sz = ImGui::GetWindowSize();
        ImGui::End();
        if (ctx->FrameCount >= 0 && ctx->FrameCount <= 2)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            IM_CHECK_EQ((int)sz.x, (int)(ImGui::CalcTextSize("Hello World").x + style.WindowPadding.x * 2.0f));
            IM_CHECK_EQ((int)sz.y, (int)(ImGui::GetFrameHeight() + ImGui::CalcTextSize("Hello World").y + style.ItemSpacing.y + 200.0f + style.WindowPadding.y * 2.0f));
        }
    };

    // ## Test that uncollapsing an auto-resizing window does not go through a frame where the window is smaller than expected
    t = REGISTER_TEST("window", "window_auto_resize_uncollapse");
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
        ctx->OpFlags |= ImGuiTestOpFlags_NoAutoUncollapse;
        ctx->WindowRef("Test Window");
        ctx->WindowCollapse(window, false);
        ctx->LogDebug("Size %f %f, SizeFull %f %f", window->Size.x, window->Size.y, window->SizeFull.x, window->SizeFull.y);
        IM_CHECK_EQ(window->Size, window->SizeFull);
        ImVec2 size_full_when_uncollapsed = window->SizeFull;
        ctx->WindowCollapse(window, true);
        ctx->LogDebug("Size %f %f, SizeFull %f %f", window->Size.x, window->Size.y, window->SizeFull.x, window->SizeFull.y);
        ImVec2 size_collapsed = window->Size;
        IM_CHECK_GT(size_full_when_uncollapsed.y, size_collapsed.y);
        IM_CHECK_EQ(size_full_when_uncollapsed, window->SizeFull);
        ctx->WindowCollapse(window, false);
        ctx->LogDebug("Size %f %f, SizeFull %f %f", window->Size.x, window->Size.y, window->SizeFull.x, window->SizeFull.y);
        IM_CHECK(window->Size.y == size_full_when_uncollapsed.y && "Window should have restored to full size.");
        ctx->Yield();
        IM_CHECK_EQ(window->Size.y, size_full_when_uncollapsed.y);
    };

    // ## Test appending multiple times to a child window (bug #2282)
    t = REGISTER_TEST("window", "window_append");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Line 1");
        ImGui::End();
        ImGui::Begin("Test Window");
        ImGui::Text("Line 2");
        ImGui::BeginChild("Blah", ImVec2(0, 50), true);
        ImGui::Text("Line 3");
        ImGui::EndChild();
        ImVec2 pos1 = ImGui::GetCursorScreenPos();
        ImGui::BeginChild("Blah");
        ImGui::Text("Line 4");
        ImGui::EndChild();
        ImVec2 pos2 = ImGui::GetCursorScreenPos();
        IM_CHECK_EQ(pos1, pos2); // Append calls to BeginChild() shouldn't affect CursorPos in parent window
        ImGui::Text("Line 5");
        ImVec2 pos3 = ImGui::GetCursorScreenPos();
        ImGui::BeginChild("Blah");
        ImGui::Text("Line 6");
        ImGui::EndChild();
        ImVec2 pos4 = ImGui::GetCursorScreenPos();
        IM_CHECK_EQ(pos3, pos4); // Append calls to BeginChild() shouldn't affect CursorPos in parent window
        ImGui::End();
    };

    // ## Test basic focus behavior
    // FIXME-TESTS: This in particular when combined with Docking should be tested with and without ConfigDockingTabBarOnSingleWindows
    t = REGISTER_TEST("window", "window_focus_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("AAAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
        ImGui::Begin("BBBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
        if ((ctx->FrameCount >= 20 && ctx->FrameCount < 40) || (ctx->FrameCount >= 50))
        {
            ImGui::Begin("CCCC", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::End();
            ImGui::Begin("DDDD", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::End();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        IM_CHECK_EQ(g.NavWindow->ID, ctx->GetID("/BBBB"));
        ctx->YieldUntil(19);
        IM_CHECK_EQ(g.NavWindow->ID, ctx->GetID("/BBBB"));
        ctx->YieldUntil(20);
        IM_CHECK_EQ(g.NavWindow->ID, ctx->GetID("/DDDD"));
        ctx->YieldUntil(30);
        ctx->WindowFocus("/CCCC");
        IM_CHECK_EQ(g.NavWindow->ID, ctx->GetID("/CCCC"));
        ctx->YieldUntil(39);
        ctx->YieldUntil(40);
        IM_CHECK_EQ(g.NavWindow->ID, ctx->GetID("/CCCC"));

        // When docked, it should NOT takes 1 extra frame to lose focus (fixed 2019/03/28)
        ctx->YieldUntil(41);
        IM_CHECK_EQ(g.NavWindow->ID, ctx->GetID("/BBBB"));

        ctx->YieldUntil(49);
        IM_CHECK_EQ(g.NavWindow->ID, ctx->GetID("/BBBB"));
        ctx->YieldUntil(50);
        IM_CHECK_EQ(g.NavWindow->ID, ctx->GetID("/DDDD"));
    };

    // ## Test popup focus and right-click to close popups up to a given level
    t = REGISTER_TEST("window", "window_focus_popup");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->WindowRef("Dear ImGui Demo");
        ctx->ItemOpen("Popups & Modal windows");
        ctx->ItemOpen("Popups");
        ctx->ItemClick("Popups/Toggle..");

        ImGuiWindow* popup_1 = g.NavWindow;
        ctx->WindowRef(popup_1->Name);
        ctx->ItemClick("Stacked Popup");
        IM_CHECK(popup_1->WasActive);

        ImGuiWindow* popup_2 = g.NavWindow;
        ctx->MouseMove("Bream", ImGuiTestOpFlags_NoFocusWindow | ImGuiTestOpFlags_NoCheckHoveredId);
        ctx->MouseClick(1); // Close with right-click
        IM_CHECK(popup_1->WasActive);
        IM_CHECK(!popup_2->WasActive);
        IM_CHECK(g.NavWindow == popup_1);
    };

    // ## Test that child window correctly affect contents size based on how their size was specified.
    t = REGISTER_TEST("window", "window_child_layout_size");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Hi");
        ImGui::BeginChild("Child 1", ImVec2(100, 100), true);
        ImGui::EndChild();
        if (ctx->FrameCount == 2)
            IM_CHECK_EQ(ctx->UiContext->CurrentWindow->ContentSize, ImVec2(100, 100 + ImGui::GetTextLineHeightWithSpacing()));
        if (ctx->FrameCount == 2)
            ctx->Finish();
        ImGui::End();
    };

    // ## Test that child window outside the visible section of their parent are clipped
    t = REGISTER_TEST("window", "window_child_clip");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::BeginChild("Child 1", ImVec2(100, 100));
        if (ctx->FrameCount == 2)
            IM_CHECK_EQ(ctx->UiContext->CurrentWindow->SkipItems, false);
        ImGui::EndChild();
        ImGui::SetCursorPos(ImVec2(300, 300));
        ImGui::BeginChild("Child 2", ImVec2(100, 100));
        if (ctx->FrameCount == 2)
            IM_CHECK_EQ(ctx->UiContext->CurrentWindow->SkipItems, true);
        ImGui::EndChild();
        ImGui::End();
        if (ctx->FrameCount == 2)
            ctx->Finish();
    };

    // ## Test that basic SetScrollHereY call scrolls all the way (#1804)
    // ## Test expected value of ScrollMaxY
    t = REGISTER_TEST("window", "window_scroll_001");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(0.0f, 500));
        ImGui::Begin("Test Scrolling", NULL, ImGuiWindowFlags_NoSavedSettings);
        for (int n = 0; n < 100; n++)
            ImGui::Text("Hello %d\n", n);
        ImGui::SetScrollHereY(0.0f);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window = ImGui::FindWindowByName("Test Scrolling");
        ImGuiStyle& style = ImGui::GetStyle();

        IM_CHECK(window->ContentSize.y > 0.0f);
        float scroll_y = window->Scroll.y;
        float scroll_max_y = window->ScrollMax.y;
        ctx->LogDebug("scroll_y = %f", scroll_y);
        ctx->LogDebug("scroll_max_y = %f", scroll_max_y);
        ctx->LogDebug("window->SizeContents.y = %f", window->ContentSize.y);

        IM_CHECK_NO_RET(scroll_y > 0.0f);
        IM_CHECK_NO_RET(scroll_y == scroll_max_y);

        float expected_size_contents_y = 100 * ImGui::GetTextLineHeightWithSpacing() - style.ItemSpacing.y; // Newer definition of SizeContents as per 1.71
        IM_CHECK(ImFloatEq(window->ContentSize.y, expected_size_contents_y));

        float expected_scroll_max_y = expected_size_contents_y + window->WindowPadding.y * 2.0f - window->InnerRect.GetHeight();
        IM_CHECK(ImFloatEq(scroll_max_y, expected_scroll_max_y));
    };

    // ## Test that ScrollMax values are correctly zero (we had/have bugs where they are seldomly == BorderSize)
    t = REGISTER_TEST("window", "window_scroll_002");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Scrolling 1", NULL, ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Dummy(ImVec2(200, 200));
        ImGuiWindow* window1 = ctx->UiContext->CurrentWindow;
        IM_CHECK_NO_RET(window1->ScrollMax.x == 0.0f); // FIXME-TESTS: If another window in another test used same name, ScrollMax won't be zero on first frame
        IM_CHECK_NO_RET(window1->ScrollMax.y == 0.0f);
        ImGui::End();

        ImGui::Begin("Test Scrolling 2", NULL, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Dummy(ImVec2(200, 200));
        ImGuiWindow* window2 = ctx->UiContext->CurrentWindow;
        IM_CHECK_NO_RET(window2->ScrollMax.x == 0.0f);
        IM_CHECK_NO_RET(window2->ScrollMax.y == 0.0f);
        ImGui::End();
        if (ctx->FrameCount == 2)
            ctx->Finish();
    };

    // ## Test that SetScrollY/GetScrollY values are matching. You'd think this would be obvious! Think again!
    // FIXME-TESTS: With/without menu bars, could we easily allow for test variations that affects both GuiFunc and TestFunc
    t = REGISTER_TEST("window", "window_scroll_003");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Scrolling 3");
        for (int n = 0; n < 100; n++)
            ImGui::Text("Line %d", n);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->Yield();
        ImGuiWindow* window = ImGui::FindWindowByName("Test Scrolling 3");
        ImGui::SetScrollY(window, 100.0f);
        ctx->Yield();
        float sy = window->Scroll.y;
        IM_CHECK_EQ(sy, 100.0f);
    };

    // ## Test that an auto-fit window doesn't have scrollbar while resizing (FIXME-TESTS: Also test non-zero ScrollMax when implemented)
    t = REGISTER_TEST("window", "window_scroll_while_resizing");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Scrolling", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Below is a child window");
        ImGui::BeginChild("blah", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGuiWindow* child_window = ctx->UiContext->CurrentWindow;
        ImGui::EndChild();
        IM_CHECK(child_window->ScrollbarY == false);
        if (ctx->FrameCount >= ctx->FirstFrameCount)
        {
            ImGuiWindow* window = ctx->UiContext->CurrentWindow;
            IM_CHECK(window->ScrollbarY == false);
            //IM_CHECK(window->ScrollMax.y == 0.0f);    // FIXME-TESTS: 1.71 I would like to make this change but unsure of side effects yet
            if (ctx->FrameCount == 2)
                ctx->Finish();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowResize("Test Scrolling", ImVec2(400, 400));
        ctx->WindowResize("Test Scrolling", ImVec2(100, 100));
    };

    // ## Test window moving
    t = REGISTER_TEST("window", "window_move");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("Movable Window");
        ImGui::TextUnformatted("Lorem ipsum dolor sit amet");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window = ctx->GetWindowByRef("Movable Window");
        ctx->WindowMove("Movable Window", ImVec2(0, 0));
        IM_CHECK(window->Pos == ImVec2(0, 0));
        ctx->WindowMove("Movable Window", ImVec2(100, 0));
        IM_CHECK(window->Pos == ImVec2(100, 0));
        ctx->WindowMove("Movable Window", ImVec2(50, 100));
        IM_CHECK(window->Pos == ImVec2(50, 100));
    };

    // ## Test closing current popup
    // FIXME-TESTS: Test left-click/right-click forms of closing popups
    t = REGISTER_TEST("window", "window_close_current_popup");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("Popups", NULL, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Menu"))
            {
                if (ImGui::BeginMenu("Submenu"))
                {
                    if (ImGui::MenuItem("Close"))
                        ImGui::CloseCurrentPopup();
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Popups");
        IM_CHECK(ctx->UiContext->OpenPopupStack.Size == 0);
        ctx->MenuClick("Menu");
        IM_CHECK(ctx->UiContext->OpenPopupStack.Size == 1);
        ctx->MenuClick("Menu/Submenu");
        IM_CHECK(ctx->UiContext->OpenPopupStack.Size == 2);
        ctx->MenuClick("Menu/Submenu/Close");
        IM_CHECK(ctx->UiContext->OpenPopupStack.Size == 0);
    };
}


//-------------------------------------------------------------------------
// Tests: Layout
//-------------------------------------------------------------------------

void RegisterTests_Layout(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test matching LastItemRect.Max/CursorMaxPos
    // ## Test text baseline
    t = REGISTER_TEST("layout", "layout_baseline_and_cursormax");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        float y = 0.0f;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGuiContext& g = *ctx->UiContext;
        ImGuiStyle& style = ImGui::GetStyle();
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImDrawList* draw_list = window->DrawList;

        enum ItemType
        {
            ItemType_SmallButton,
            ItemType_Button,
            ItemType_Text,
            ItemType_BulletText,
            ItemType_TreeNode,
            ItemType_Selectable,
            ItemType_ImageButton,
            ItemType_COUNT,
            ItemType_START = 0
        };
        const char* item_type_names[] =
        {
            "SmallButton",
            "Button",
            "Text",
            "BulletText",
            "TreeNode",
            "Selectable",
            "ImageButton"
        };

        for (int item_type = ItemType_START; item_type < ItemType_COUNT; item_type++)
        {
            const char* item_type_name = item_type_names[item_type];
            for (int n = 0; n < 5; n++)
            {
                ctx->LogDebug("Test '%s' variant %d", item_type_name, n);

                // Emit button with varying baseline
                y = window->DC.CursorPos.y;
                if (n > 0)
                {
                    if (n >= 1 && n <= 2)
                        ImGui::SmallButton("Button");
                    else if (n >= 3 && n <= 4)
                        ImGui::Button("Button");
                    ImGui::SameLine();
                }

                const int label_line_count = (n == 0 || n == 1 || n == 3) ? 1 : 2;

                Str30f label(label_line_count == 1 ? "%s%d" : "%s%d\nHello", item_type_name, n);

                float expected_padding = 0.0f;
                switch (item_type)
                {
                case ItemType_SmallButton:
                    expected_padding = window->DC.CurrLineTextBaseOffset;
                    ImGui::SmallButton(label.c_str());
                    break;
                case ItemType_Button:
                    expected_padding = style.FramePadding.y * 2.0f;
                    ImGui::Button(label.c_str());
                    break;
                case ItemType_Text:
                    expected_padding = window->DC.CurrLineTextBaseOffset;
                    ImGui::Text("%s", label.c_str());
                    break;
                case ItemType_BulletText:
                    expected_padding = (n <= 2) ? 0.0f : style.FramePadding.y * 1.0f;
                    ImGui::BulletText("%s", label.c_str());
                    break;
                case ItemType_TreeNode:
                    expected_padding = (n <= 2) ? 0.0f : style.FramePadding.y * 2.0f;
                    ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_NoTreePushOnOpen);
                    break;
                case ItemType_Selectable:
                    // FIXME-TESTS: We may want to aim the specificies of Selectable() and not clear ItemSpacing
                    //expected_padding = style.ItemSpacing.y * 0.5f;
                    expected_padding = window->DC.CurrLineTextBaseOffset;
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                    ImGui::Selectable(label.c_str());
                    ImGui::PopStyleVar();
                    ImGui::Spacing();
                    break;
                case ItemType_ImageButton:
                    expected_padding = style.FramePadding.y * 2.0f;
                    ImGui::ImageButton(ImGui::GetIO().Fonts->TexID, ImVec2(100, ImGui::GetTextLineHeight() * label_line_count), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
                    break;
                }

                ImGui::DebugDrawItemRect(IM_COL32(255, 0, 0, 200));
                draw_list->AddLine(ImVec2(window->Pos.x, window->DC.CursorMaxPos.y), ImVec2(window->Pos.x + window->Size.x, window->DC.CursorMaxPos.y), IM_COL32(255, 255, 0, 100));
                if (label_line_count > 1)
                    IM_CHECK_EQ_NO_RET(window->DC.CursorMaxPos.y, window->DC.LastItemRect.Max.y);

                const float current_height = window->DC.LastItemRect.Max.y - y;
                const float expected_height = g.FontSize * label_line_count + expected_padding;
                IM_CHECK_EQ_NO_RET(current_height, expected_height);
            }

            ImGui::Spacing();
        }

        // FIXME-TESTS: Selectable()

        ImGui::End();
    };

    // ## Test size of items with explicit thin size and larger label
#if 0
    t = REGISTER_TEST("layout", "layout_height_label");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImGuiContext& g = *ctx->UiContext;

        ImGui::Button("Button", ImVec2(0, 8));
        IM_CHECK_EQ_NO_RET(window->DC.LastItemRect.GetHeight(), 8.0f);

        ImGui::Separator();

        float values[3] = { 1.0f, 2.0f, 3.0f };
        ImGui::PlotHistogram("##Histogram", values, 3, 0, NULL, 0.0f, 4.0f, ImVec2(0, 8));
        IM_CHECK_EQ_NO_RET(window->DC.LastItemRect.GetHeight(), 8.0f);
        ImGui::Separator();

        ImGui::PlotHistogram("Histogram\nTwoLines", values, 3, 0, NULL, 0.0f, 4.0f, ImVec2(0, 8));
        IM_CHECK_EQ_NO_RET(window->DC.LastItemRect.GetHeight(), g.FontSize * 2.0f);
        ImGui::Separator();

        static char buf[128] = "";
        ImGui::InputTextMultiline("##InputText", buf, IM_ARRAYSIZE(buf), ImVec2(0, 8));
        IM_CHECK_EQ_NO_RET(window->DC.LastItemRect.GetHeight(), 8.0f);
        ImGui::Separator();

        ImGui::InputTextMultiline("InputText\nTwoLines", buf, IM_ARRAYSIZE(buf), ImVec2(0, 8));
        IM_CHECK_EQ_NO_RET(window->DC.LastItemRect.GetHeight(), g.FontSize * 2.0f);
        ImGui::Separator();

        ImGui::Text("Text");
        IM_CHECK_EQ_NO_RET(window->DC.LastItemRect.GetHeight(), g.FontSize);

        ImGui::End();
    };
#endif

}

//-------------------------------------------------------------------------
// Tests: Widgets
//-------------------------------------------------------------------------

void RegisterTests_Widgets(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test basic button presses
    t = REGISTER_TEST("widgets", "widgets_button_press");
    struct ButtonPressTestVars { int ButtonPressCount[6] = { 0 }; };
    t->SetUserDataType<ButtonPressTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ButtonPressTestVars& vars = ctx->GetUserData<ButtonPressTestVars>();

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Button0"))
            vars.ButtonPressCount[0]++;
        if (ImGui::ButtonEx("Button1", ImVec2(0, 0), ImGuiButtonFlags_PressedOnDoubleClick))
            vars.ButtonPressCount[1]++;
        if (ImGui::ButtonEx("Button2", ImVec2(0, 0), ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick))
            vars.ButtonPressCount[2]++;
        if (ImGui::ButtonEx("Button3", ImVec2(0, 0), ImGuiButtonFlags_PressedOnClickReleaseAnywhere))
            vars.ButtonPressCount[3]++;
        if (ImGui::ButtonEx("Button4", ImVec2(0, 0), ImGuiButtonFlags_Repeat))
            vars.ButtonPressCount[4]++;
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ButtonPressTestVars& vars = ctx->GetUserData<ButtonPressTestVars>();

        ctx->WindowRef("Test Window");
        ctx->ItemClick("Button0");
        IM_CHECK_EQ(vars.ButtonPressCount[0], 1);
        ctx->ItemDoubleClick("Button1");
        IM_CHECK_EQ(vars.ButtonPressCount[1], 1);
        ctx->ItemDoubleClick("Button2");
        IM_CHECK_EQ(vars.ButtonPressCount[2], 2);

        // Test ImGuiButtonFlags_PressedOnClickRelease vs ImGuiButtonFlags_PressedOnClickReleaseAnywhere
        vars.ButtonPressCount[2] = 0;
        ctx->MouseMove("Button2");
        ctx->MouseDown(0);
        ctx->MouseMove("Button0", ImGuiTestOpFlags_NoCheckHoveredId);
        ctx->MouseUp(0);
        IM_CHECK_EQ(vars.ButtonPressCount[2], 0);
        ctx->MouseMove("Button3");
        ctx->MouseDown(0);
        ctx->MouseMove("Button0", ImGuiTestOpFlags_NoCheckHoveredId);
        ctx->MouseUp(0);
        IM_CHECK_EQ(vars.ButtonPressCount[3], 1);

        // Test ImGuiButtonFlags_Repeat
        ctx->ItemClick("Button4");
        IM_CHECK_EQ(vars.ButtonPressCount[4], 1);
        ctx->MouseDown(0);
        IM_CHECK_EQ(vars.ButtonPressCount[4], 1);
        const float step = ImMin(ctx->UiContext->IO.KeyRepeatDelay, ctx->UiContext->IO.KeyRepeatRate) * 0.50f;
        ctx->SleepNoSkip(ctx->UiContext->IO.KeyRepeatDelay, step);
        ctx->SleepNoSkip(ctx->UiContext->IO.KeyRepeatRate, step);
        ctx->SleepNoSkip(ctx->UiContext->IO.KeyRepeatRate, step);
        ctx->SleepNoSkip(ctx->UiContext->IO.KeyRepeatRate, step);
        IM_CHECK_EQ(vars.ButtonPressCount[4], 1 + 1 + 3 * 2); // FIXME: MouseRepeatRate is double KeyRepeatRate, that's not documented / or that's a bug
        ctx->MouseUp(0);
    };

    // ## Test basic button presses
    t = REGISTER_TEST("widgets", "widgets_button_mouse_buttons");
    t->SetUserDataType<ButtonPressTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ButtonPressTestVars& vars = ctx->GetUserData<ButtonPressTestVars>();

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::ButtonEx("ButtonL", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonLeft))
            vars.ButtonPressCount[0]++;
        if (ImGui::ButtonEx("ButtonR", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonRight))
            vars.ButtonPressCount[1]++;
        if (ImGui::ButtonEx("ButtonM", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonMiddle))
            vars.ButtonPressCount[2]++;
        if (ImGui::ButtonEx("ButtonLR", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight))
            vars.ButtonPressCount[3]++;

        if (ImGui::ButtonEx("ButtonL-release", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_PressedOnRelease))
            vars.ButtonPressCount[4]++;
        if (ImGui::ButtonEx("ButtonR-release", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_PressedOnRelease))
        {
            ctx->LogDebug("Pressed!");
            vars.ButtonPressCount[5]++;
        }
        for (int n = 0; n < IM_ARRAYSIZE(vars.ButtonPressCount); n++)
            ImGui::Text("%d: %d", n, vars.ButtonPressCount[n]);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ButtonPressTestVars& vars = ctx->GetUserData<ButtonPressTestVars>();

        ctx->WindowRef("Test Window");
        ctx->ItemClick("ButtonL", 0);
        IM_CHECK_EQ(vars.ButtonPressCount[0], 1);
        ctx->ItemClick("ButtonR", 1);
        IM_CHECK_EQ(vars.ButtonPressCount[1], 1);
        ctx->ItemClick("ButtonM", 2);
        IM_CHECK_EQ(vars.ButtonPressCount[2], 1);
        ctx->ItemClick("ButtonLR", 0);
        ctx->ItemClick("ButtonLR", 1);
        IM_CHECK_EQ(vars.ButtonPressCount[3], 2);

        vars.ButtonPressCount[3] = 0;
        ctx->MouseMove("ButtonLR");
        ctx->MouseDown(0);
        ctx->MouseDown(1);
        ctx->MouseUp(0);
        ctx->MouseUp(1);
        IM_CHECK_EQ(vars.ButtonPressCount[3], 1);

        vars.ButtonPressCount[3] = 0;
        ctx->MouseMove("ButtonLR");
        ctx->MouseDown(0);
        ctx->MouseMove("ButtonR", ImGuiTestOpFlags_NoCheckHoveredId);
        ctx->MouseDown(1);
        ctx->MouseUp(0);
        ctx->MouseMove("ButtonLR");
        ctx->MouseUp(1);
        IM_CHECK_EQ(vars.ButtonPressCount[3], 0);
    };

    // ## Test ButtonBehavior frame by frame behaviors (see comments at the top of the ButtonBehavior() function)
    enum ButtonStateMachineTestStep
    {
        ButtonStateMachineTestStep_None,
        ButtonStateMachineTestStep_Init,
        ButtonStateMachineTestStep_MovedOver,
        ButtonStateMachineTestStep_MouseDown,
        ButtonStateMachineTestStep_MovedAway,
        ButtonStateMachineTestStep_MovedOverAgain,
        ButtonStateMachineTestStep_MouseUp,
        ButtonStateMachineTestStep_Done
    };
    t = REGISTER_TEST("widgets", "widgets_button_status");
    struct ButtonStateTestVars
    {
        ButtonStateMachineTestStep NextStep;
        ImGuiTestGenericStatus Status;
        ButtonStateTestVars() { NextStep = ButtonStateMachineTestStep_None; }
    };
    t->SetUserDataType<ButtonStateTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ButtonStateTestVars& vars = ctx->GetUserData<ButtonStateTestVars>();
        ImGuiTestGenericStatus& status = vars.Status;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        const bool pressed = ImGui::Button("Test");
        status.QuerySet();
        switch (vars.NextStep)
        {
        case ButtonStateMachineTestStep_Init:
            IM_CHECK(0 == pressed);
            IM_CHECK(0 == status.Hovered);
            IM_CHECK(0 == status.Active);
            IM_CHECK(0 == status.Activated);
            IM_CHECK(0 == status.Deactivated);
            break;
        case ButtonStateMachineTestStep_MovedOver:
            IM_CHECK(0 == pressed);
            IM_CHECK(1 == status.Hovered);
            IM_CHECK(0 == status.Active);
            IM_CHECK(0 == status.Activated);
            IM_CHECK(0 == status.Deactivated);
            break;
        case ButtonStateMachineTestStep_MouseDown:
            IM_CHECK(0 == pressed);
            IM_CHECK(1 == status.Hovered);
            IM_CHECK(1 == status.Active);
            IM_CHECK(1 == status.Activated);
            IM_CHECK(0 == status.Deactivated);
            break;
        case ButtonStateMachineTestStep_MovedAway:
            IM_CHECK(0 == pressed);
            IM_CHECK(0 == status.Hovered);
            IM_CHECK(1 == status.Active);
            IM_CHECK(0 == status.Activated);
            IM_CHECK(0 == status.Deactivated);
            break;
        case ButtonStateMachineTestStep_MovedOverAgain:
            IM_CHECK(0 == pressed);
            IM_CHECK(1 == status.Hovered);
            IM_CHECK(1 == status.Active);
            IM_CHECK(0 == status.Activated);
            IM_CHECK(0 == status.Deactivated);
            break;
        case ButtonStateMachineTestStep_MouseUp:
            IM_CHECK(1 == pressed);
            IM_CHECK(1 == status.Hovered);
            IM_CHECK(0 == status.Active);
            IM_CHECK(0 == status.Activated);
            IM_CHECK(1 == status.Deactivated);
            break;
        case ButtonStateMachineTestStep_Done:
            IM_CHECK(0 == pressed);
            IM_CHECK(0 == status.Hovered);
            IM_CHECK(0 == status.Active);
            IM_CHECK(0 == status.Activated);
            IM_CHECK(0 == status.Deactivated);
            break;
        default:
            break;
        }
        vars.NextStep = ButtonStateMachineTestStep_None;

        // The "Dummy" button allows to move the mouse away from the "Test" button
        ImGui::Button("Dummy");

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ButtonStateTestVars& vars = ctx->GetUserData<ButtonStateTestVars>();
        vars.NextStep = ButtonStateMachineTestStep_None;

        ctx->WindowRef("Test Window");

        // Move mouse away from "Test" button
        ctx->MouseMove("Dummy");
        vars.NextStep = ButtonStateMachineTestStep_Init;
        ctx->Yield();

        ctx->MouseMove("Test");
        vars.NextStep = ButtonStateMachineTestStep_MovedOver;
        ctx->Yield();

        vars.NextStep = ButtonStateMachineTestStep_MouseDown;
        ctx->MouseDown();

        ctx->MouseMove("Dummy", ImGuiTestOpFlags_NoCheckHoveredId);
        vars.NextStep = ButtonStateMachineTestStep_MovedAway;
        ctx->Yield();

        ctx->MouseMove("Test");
        vars.NextStep = ButtonStateMachineTestStep_MovedOverAgain;
        ctx->Yield();

        vars.NextStep = ButtonStateMachineTestStep_MouseUp;
        ctx->MouseUp();

        ctx->MouseMove("Dummy");
        vars.NextStep = ButtonStateMachineTestStep_Done;
        ctx->Yield();
    };

    // ## Test checkbox click
    t = REGISTER_TEST("widgets", "widgets_checkbox_001");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window1");
        ImGui::Checkbox("Checkbox", &ctx->GenericVars.Bool1);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // We use WindowRef() to ensure the window is uncollapsed.
        IM_CHECK(ctx->GenericVars.Bool1 == false);
        ctx->WindowRef("Window1");
        ctx->ItemClick("Checkbox");
        IM_CHECK(ctx->GenericVars.Bool1 == true);
    };

    // FIXME-TESTS: WIP
    t = REGISTER_TEST("widgets", "widgets_datatype_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        char buf[3] = { 42, 100, 42 };
        ImGui::DragScalar("Drag", ImGuiDataType_S8, &buf[1], 0.5f, NULL, NULL);
        IM_ASSERT(buf[0] == 42 && buf[2] == 42);
        ImGui::End();
    };

    // ## Test DragInt() as InputText
    // ## Test ColorEdit4() as InputText (#2557)
    t = REGISTER_TEST("widgets", "widgets_as_input");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::DragInt("Drag", &vars.Int1);
        ImGui::ColorEdit4("Color", &vars.Vec4.x);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->WindowRef("Test Window");

        IM_CHECK_EQ(vars.Int1, 0);
        ctx->ItemInput("Drag");
        IM_CHECK_EQ(ctx->UiContext->ActiveId, ctx->GetID("Drag"));
        ctx->KeyCharsAppendEnter("123");
        IM_CHECK_EQ(vars.Int1, 123);

        ctx->ItemInput("Color##Y");
        IM_CHECK_EQ(ctx->UiContext->ActiveId, ctx->GetID("Color##Y"));
        ctx->KeyCharsAppend("123");
        IM_CHECK(ImFloatEq(vars.Vec4.y, 123.0f / 255.0f));
        ctx->KeyPressMap(ImGuiKey_Tab);
        ctx->KeyCharsAppendEnter("200");
        IM_CHECK(ImFloatEq(vars.Vec4.x,   0.0f / 255.0f));
        IM_CHECK(ImFloatEq(vars.Vec4.y, 123.0f / 255.0f));
        IM_CHECK(ImFloatEq(vars.Vec4.z, 200.0f / 255.0f));
    };

    // ## Test InputText widget
    t = REGISTER_TEST("widgets", "widgets_inputtext_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        char* buf = ctx->GenericVars.Str1;

        ctx->WindowRef("Test Window");

        // Insert
        strcpy(buf, "Hello");
        ctx->ItemClick("InputText");
        ctx->KeyCharsAppendEnter("World123");
        IM_CHECK_STR_EQ(buf, "HelloWorld123");

        // Delete
        ctx->ItemClick("InputText");
        ctx->KeyPressMap(ImGuiKey_End);
        ctx->KeyPressMap(ImGuiKey_Backspace, ImGuiKeyModFlags_None, 3);
        ctx->KeyPressMap(ImGuiKey_Enter);
        IM_CHECK_STR_EQ(buf, "HelloWorld");

        // Insert, Cancel
        ctx->ItemClick("InputText");
        ctx->KeyPressMap(ImGuiKey_End);
        ctx->KeyChars("XXXXX");
        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK_STR_EQ(buf, "HelloWorld");

        // Delete, Cancel
        ctx->ItemClick("InputText");
        ctx->KeyPressMap(ImGuiKey_End);
        ctx->KeyPressMap(ImGuiKey_Backspace, ImGuiKeyModFlags_None, 5);
        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK_STR_EQ(buf, "HelloWorld");
    };

    // ## Test InputText undo/redo ops, in particular related to issue we had with stb_textedit undo/redo buffers
    t = REGISTER_TEST("widgets", "widgets_inputtext_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        if (vars.StrLarge.empty())
            vars.StrLarge.resize(10000, 0);
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 50, 0.0f));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("strlen() = %d", (int)strlen(vars.StrLarge.Data));
        ImGui::InputText("Dummy", vars.Str1, IM_ARRAYSIZE(vars.Str1), ImGuiInputTextFlags_None);
        ImGui::InputTextMultiline("InputText", vars.StrLarge.Data, vars.StrLarge.Size, ImVec2(-1, ImGui::GetFontSize() * 20), ImGuiInputTextFlags_None);
        ImGui::End();
        //DebugInputTextState();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // https://github.com/nothings/stb/issues/321
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        // Start with a 350 characters buffer.
        // For this test we don't inject the characters via pasting or key-by-key in order to precisely control the undo/redo state.
        char* buf = vars.StrLarge.Data;
        IM_CHECK_EQ((int)strlen(buf), 0);
        for (int n = 0; n < 10; n++)
            strcat(buf, "xxxxxxx abcdefghijklmnopqrstuvwxyz\n");
        IM_CHECK_EQ((int)strlen(buf), 350);

        ctx->WindowRef("Test Window");
        ctx->ItemClick("Dummy"); // This is to ensure stb_textedit_clear_state() gets called (clear the undo buffer, etc.)
        ctx->ItemClick("InputText");

        ImGuiInputTextState& input_text_state = GImGui->InputTextState;
        ImStb::StbUndoState& undo_state = input_text_state.Stb.undostate;
        IM_CHECK_EQ(input_text_state.ID, GImGui->ActiveId);
        IM_CHECK_EQ(undo_state.undo_point, 0);
        IM_CHECK_EQ(undo_state.undo_char_point, 0);
        IM_CHECK_EQ(undo_state.redo_point, STB_TEXTEDIT_UNDOSTATECOUNT);
        IM_CHECK_EQ(undo_state.redo_char_point, STB_TEXTEDIT_UNDOCHARCOUNT);
        IM_CHECK_EQ(STB_TEXTEDIT_UNDOCHARCOUNT, 999); // Test designed for this value

        // Insert 350 characters via 10 paste operations
        // We use paste operations instead of key-by-key insertion so we know our undo buffer will contains 10 undo points.
        //const char line_buf[26+8+1+1] = "xxxxxxx abcdefghijklmnopqrstuvwxyz\n"; // 8+26+1 = 35
        //ImGui::SetClipboardText(line_buf);
        //IM_CHECK(strlen(line_buf) == 35);
        //ctx->KeyPressMap(ImGuiKey_V, ImGuiKeyModFlags_Shortcut, 10);

        // Select all, copy, paste 3 times
        ctx->KeyPressMap(ImGuiKey_A, ImGuiKeyModFlags_Shortcut);    // Select all
        ctx->KeyPressMap(ImGuiKey_C, ImGuiKeyModFlags_Shortcut);    // Copy
        ctx->KeyPressMap(ImGuiKey_End, ImGuiKeyModFlags_Shortcut);  // Go to end, clear selection
        ctx->SleepShort();
        for (int n = 0; n < 3; n++)
        {
            ctx->KeyPressMap(ImGuiKey_V, ImGuiKeyModFlags_Shortcut);// Paste append three times
            ctx->SleepShort();
        }
        int len = (int)strlen(vars.StrLarge.Data);
        IM_CHECK_EQ(len, 350 * 4);
        IM_CHECK_EQ(undo_state.undo_point, 3);
        IM_CHECK_EQ(undo_state.undo_char_point, 0);

        // Undo x2
        IM_CHECK(undo_state.redo_point == STB_TEXTEDIT_UNDOSTATECOUNT);
        ctx->KeyPressMap(ImGuiKey_Z, ImGuiKeyModFlags_Shortcut);
        ctx->KeyPressMap(ImGuiKey_Z, ImGuiKeyModFlags_Shortcut);
        len = (int)strlen(vars.StrLarge.Data);
        IM_CHECK_EQ(len, 350 * 2);
        IM_CHECK_EQ(undo_state.undo_point, 1);
        IM_CHECK_EQ(undo_state.redo_point, STB_TEXTEDIT_UNDOSTATECOUNT - 2);
        IM_CHECK_EQ(undo_state.redo_char_point, STB_TEXTEDIT_UNDOCHARCOUNT - 350 * 2);

        // Undo x1 should call stb_textedit_discard_redo()
        ctx->KeyPressMap(ImGuiKey_Z, ImGuiKeyModFlags_Shortcut);
        len = (int)strlen(vars.StrLarge.Data);
        IM_CHECK_EQ(len, 350 * 1);
    };

    // ## Test InputText vs user ownership of data
    t = REGISTER_TEST("widgets", "widgets_inputtext_3_text_ownership");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::LogToBuffer();
        ImGui::InputText("##InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1)); // Remove label to simplify the capture/comparison
        ImStrncpy(vars.Str2, ctx->UiContext->LogBuffer.c_str(), IM_ARRAYSIZE(vars.Str2));
        ImGui::LogFinish();
        ImGui::Text("Captured: \"%s\"", vars.Str2);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        char* buf_user = vars.Str1;
        char* buf_visible = vars.Str2;
        ctx->WindowRef("Test Window");

        IM_CHECK_STR_EQ(buf_visible, "");
        strcpy(buf_user, "Hello");
        ctx->Yield();
        IM_CHECK_STR_EQ(buf_visible, "Hello");
        ctx->ItemClick("##InputText");
        ctx->KeyCharsAppend("1");
        ctx->Yield();
        IM_CHECK_STR_EQ(buf_user, "Hello1");
        IM_CHECK_STR_EQ(buf_visible, "Hello1");

        // Because the item is active, it owns the source data, so:
        strcpy(buf_user, "Overwritten");
        ctx->Yield();
        IM_CHECK_STR_EQ(buf_user, "Hello1");
        IM_CHECK_STR_EQ(buf_visible, "Hello1");

        // Lose focus, at this point the InputTextState->ID should be holding on the last active state,
        // so we verify that InputText() is picking up external changes.
        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK_EQ(ctx->UiContext->ActiveId, (unsigned)0);
        strcpy(buf_user, "Hello2");
        ctx->Yield();
        IM_CHECK_STR_EQ(buf_user, "Hello2");
        IM_CHECK_STR_EQ(buf_visible, "Hello2");
    };

    // ## Test that InputText doesn't go havoc when activated via another item
    t = REGISTER_TEST("widgets", "widgets_inputtext_4_id_conflict");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 50, 0.0f));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ctx->FrameCount < 50)
            ImGui::Button("Hello");
        else
            ImGui::InputText("Hello", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Test Window");
        ctx->ItemHoldForFrames("Hello", 100);
    };

    // ## Test that InputText doesn't append two tab characters if the backend supplies both tab key and character
    t = REGISTER_TEST("widgets", "widgets_inputtext_5_tab_double_insertion");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1), ImGuiInputTextFlags_AllowTabInput);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->WindowRef("Test Window");
        ctx->ItemClick("Field");
        ctx->UiContext->IO.AddInputCharacter((ImWchar)'\t');
        ctx->KeyPressMap(ImGuiKey_Tab);
        IM_CHECK_STR_EQ(vars.Str1, "\t");
    };

    // ## Test input clearing action (ESC key) being undoable (#3008).
    t = REGISTER_TEST("widgets", "widgets_inputtext_6_esc_undo");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        ImGui::End();

    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        // FIXME-TESTS: Facilitate usage of variants
        const int test_count = ctx->HasDock ? 2 : 1;
        for (int test_n = 0; test_n < test_count; test_n++)
        {
            ctx->LogDebug("TEST CASE %d", test_n);
            const char* initial_value = (test_n == 0) ? "" : "initial";
            strcpy(vars.Str1, initial_value);
            ctx->WindowRef("Test Window");
            ctx->ItemInput("Field");
            ctx->KeyCharsReplace("text");
            IM_CHECK_STR_EQ(vars.Str1, "text");
            ctx->KeyPressMap(ImGuiKey_Escape);                      // Reset input to initial value.
            IM_CHECK_STR_EQ(vars.Str1, initial_value);
            ctx->ItemInput("Field");
            ctx->KeyPressMap(ImGuiKey_Z, ImGuiKeyModFlags_Shortcut);    // Undo
            IM_CHECK_STR_EQ(vars.Str1, "text");
            ctx->KeyPressMap(ImGuiKey_Enter);                       // Unfocus otherwise test_n==1 strcpy will fail
        }
    };

    // ## Test resize callback (#3009, #2006, #1443, #1008)
    t = REGISTER_TEST("widgets", "widgets_inputtext_7_resizecallback");
    struct StrVars { Str str; };
    t->SetUserDataType<StrVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        StrVars& vars = ctx->GetUserData<StrVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::InputText("Field1", &vars.str, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            IM_CHECK_EQ(vars.str.capacity(), 4 + 5 + 1);
            IM_CHECK_STR_EQ(vars.str.c_str(), "abcdhello");
        }
        Str str_local_unsaved = "abcd";
        if (ImGui::InputText("Field2", &str_local_unsaved, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            IM_CHECK_EQ(str_local_unsaved.capacity(), 4 + 5 + 1);
            IM_CHECK_STR_EQ(str_local_unsaved.c_str(), "abcdhello");
        }
        ImGui::End();

    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        StrVars& vars = ctx->GetUserData<StrVars>();
        vars.str.set("abcd");
        IM_CHECK_EQ(vars.str.capacity(), 4+1);
        ctx->WindowRef("Test Window");
        ctx->ItemInput("Field1");
        ctx->KeyCharsAppendEnter("hello");
        ctx->ItemInput("Field2");
        ctx->KeyCharsAppendEnter("hello");
    };

    // ## Test for Nav interference
    t = REGISTER_TEST("widgets", "widgets_inputtext_nav");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImVec2 sz(50, 0);
        ImGui::Button("UL", sz); ImGui::SameLine();
        ImGui::Button("U",  sz); ImGui::SameLine();
        ImGui::Button("UR", sz);
        ImGui::Button("L",  sz); ImGui::SameLine();
        ImGui::SetNextItemWidth(sz.x);
        ImGui::InputText("##Field", vars.Str1, IM_ARRAYSIZE(vars.Str1), ImGuiInputTextFlags_AllowTabInput);
        ImGui::SameLine();
        ImGui::Button("R", sz);
        ImGui::Button("DL", sz); ImGui::SameLine();
        ImGui::Button("D", sz); ImGui::SameLine();
        ImGui::Button("DR", sz);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Test Window");
        ctx->ItemClick("##Field");
        ctx->KeyPressMap(ImGuiKey_LeftArrow);
        IM_CHECK_EQ(ctx->UiContext->NavId, ctx->GetID("##Field"));
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK_EQ(ctx->UiContext->NavId, ctx->GetID("##Field"));
        ctx->KeyPressMap(ImGuiKey_UpArrow);
        IM_CHECK_EQ(ctx->UiContext->NavId, ctx->GetID("U"));
        ctx->KeyPressMap(ImGuiKey_DownArrow);
        ctx->KeyPressMap(ImGuiKey_DownArrow);
        IM_CHECK_EQ(ctx->UiContext->NavId, ctx->GetID("D"));
    };

    // ## Test ColorEdit4() and IsItemDeactivatedXXX() functions
    // ## Test that IsItemActivated() doesn't trigger when clicking the color button to open picker
    t = REGISTER_TEST("widgets", "widgets_status_coloredit");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        bool ret = ImGui::ColorEdit4("Field", &vars.Vec4.x, ImGuiColorEditFlags_None);
        vars.Status.QueryInc(ret);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Accumulate return values over several frames/action into each bool
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiTestGenericStatus& status = vars.Status;

        // Testing activation flag being set
        ctx->WindowRef("Test Window");
        ctx->ItemClick("Field/##ColorButton");
        IM_CHECK(status.Ret == 0 && status.Activated == 1 && status.Deactivated == 1 && status.DeactivatedAfterEdit == 0 && status.Edited == 0);
        status.Clear();

        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK(status.Ret == 0 && status.Activated == 0 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0 && status.Edited == 0);
        status.Clear();
    };

    // ## Test InputText() and IsItemDeactivatedXXX() functions (mentioned in #2215)
    t = REGISTER_TEST("widgets", "widgets_status_inputtext");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        bool ret = ImGui::InputText("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        vars.Status.QueryInc(ret);
        ImGui::InputText("Dummy Sibling", vars.Str2, IM_ARRAYSIZE(vars.Str2));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Accumulate return values over several frames/action into each bool
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiTestGenericStatus& status = vars.Status;

        // Testing activation flag being set
        ctx->WindowRef("Test Window");
        ctx->ItemClick("Field");
        IM_CHECK(status.Ret == 0 && status.Activated == 1 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0 && status.Edited == 0);
        status.Clear();

        // Testing deactivated flag being set when canceling with Escape
        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK(status.Ret == 0 && status.Activated == 0 && status.Deactivated == 1 && status.DeactivatedAfterEdit == 0 && status.Edited == 0);
        status.Clear();

        // Testing validation with Return after editing
        ctx->ItemClick("Field");
        IM_CHECK(!status.Ret && status.Activated && !status.Deactivated && !status.DeactivatedAfterEdit && status.Edited == 0);
        status.Clear();
        ctx->KeyCharsAppend("Hello");
        IM_CHECK(status.Ret && !status.Activated && !status.Deactivated && !status.DeactivatedAfterEdit && status.Edited >= 1);
        status.Clear();
        ctx->KeyPressMap(ImGuiKey_Enter);
        IM_CHECK(!status.Ret && !status.Activated && status.Deactivated && status.DeactivatedAfterEdit && status.Edited == 0);
        status.Clear();

        // Testing validation with Tab after editing
        ctx->ItemClick("Field");
        ctx->KeyCharsAppend(" World");
        IM_CHECK(status.Ret && status.Activated && !status.Deactivated && !status.DeactivatedAfterEdit && status.Edited >= 1);
        status.Clear();
        ctx->KeyPressMap(ImGuiKey_Tab);
        IM_CHECK(!status.Ret && !status.Activated && status.Deactivated && status.DeactivatedAfterEdit && status.Edited == 0);
        status.Clear();
    };

    // ## Test the IsItemDeactivatedXXX() functions (e.g. #2550, #1875)
    t = REGISTER_TEST("widgets", "widgets_status_multicomponent");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        bool ret = ImGui::InputFloat4("Field", &vars.FloatArray[0]);
        vars.Status.QueryInc(ret);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Accumulate return values over several frames/action into each bool
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiTestGenericStatus& status = vars.Status;

        // FIXME-TESTS: Better helper to build ids out of various type of data
        ctx->WindowRef("Test Window");
        int n;
        n = 0; ImGuiID field_0 = ImHashData(&n, sizeof(n), ctx->GetID("Field"));
        n = 1; ImGuiID field_1 = ImHashData(&n, sizeof(n), ctx->GetID("Field"));
        //n = 2; ImGuiID field_2 = ImHashData(&n, sizeof(n), ctx->GetID("Field"));

        // Testing activation/deactivation flags
        ctx->ItemClick(field_0);
        IM_CHECK(status.Ret == 0 && status.Activated == 1 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0);
        status.Clear();
        ctx->KeyPressMap(ImGuiKey_Enter);
        IM_CHECK(status.Ret == 0 && status.Activated == 0 && status.Deactivated == 1 && status.DeactivatedAfterEdit == 0);
        status.Clear();

        // Testing validation with Return after editing
        ctx->ItemClick(field_0);
        status.Clear();
        ctx->KeyCharsAppend("123");
        IM_CHECK(status.Ret >= 1 && status.Activated == 0 && status.Deactivated == 0);
        status.Clear();
        ctx->KeyPressMap(ImGuiKey_Enter);
        IM_CHECK(status.Ret == 0 && status.Activated == 0 && status.Deactivated == 1);
        status.Clear();

        // Testing validation with Tab after editing
        ctx->ItemClick(field_0);
        ctx->KeyCharsAppend("456");
        status.Clear();
        ctx->KeyPressMap(ImGuiKey_Tab);
        IM_CHECK(status.Ret == 0 && status.Activated == 1 && status.Deactivated == 1 && status.DeactivatedAfterEdit == 1);

        // Testing Edited flag on all components
        ctx->ItemClick(field_1); // FIXME-TESTS: Should not be necessary!
        ctx->ItemClick(field_0);
        ctx->KeyCharsAppend("111");
        IM_CHECK(status.Edited >= 1);
        ctx->KeyPressMap(ImGuiKey_Tab);
        status.Clear();
        ctx->KeyCharsAppend("222");
        IM_CHECK(status.Edited >= 1);
        ctx->KeyPressMap(ImGuiKey_Tab);
        status.Clear();
        ctx->KeyCharsAppend("333");
        IM_CHECK(status.Edited >= 1);
    };

    // ## Test the IsItemEdited() function when input vs output format are not matching
    t = REGISTER_TEST("widgets", "widgets_status_inputfloat_format_mismatch");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        bool ret = ImGui::InputFloat("Field", &vars.Float1);
        vars.Status.QueryInc(ret);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiTestGenericStatus& status = vars.Status;

        // Input "1" which will be formatted as "1.000", make sure we don't report IsItemEdited() multiple times!
        ctx->WindowRef("Test Window");
        ctx->ItemClick("Field");
        ctx->KeyCharsAppend("1");
        IM_CHECK(status.Ret == 1 && status.Edited == 1 && status.Activated == 1 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0);
        ctx->Yield();
        ctx->Yield();
        IM_CHECK(status.Edited == 1);
    };

    // ## Test that disabled Selectable has an ID but doesn't interfere with navigation
    t = REGISTER_TEST("widgets", "widgets_selectable_disabled");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Selectable("Selectable A");
        if (ctx->FrameCount == 0)
            IM_CHECK_EQ(ImGui::GetItemID(), ImGui::GetID("Selectable A"));
        ImGui::Selectable("Selectable B", false, ImGuiSelectableFlags_Disabled);
        if (ctx->FrameCount == 0)
            IM_CHECK_EQ(ImGui::GetItemID(), ImGui::GetID("Selectable B")); // Make sure B has an ID
        ImGui::Selectable("Selectable C");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Test Window");
        ctx->ItemClick("Selectable A");
        IM_CHECK_EQ(ctx->UiContext->NavId, ctx->GetID("Selectable A"));
        ctx->KeyPressMap(ImGuiKey_DownArrow);
        IM_CHECK_EQ(ctx->UiContext->NavId, ctx->GetID("Selectable C")); // Make sure we have skipped B
    };

    // ## Test that tight tab bar does not create extra drawcalls
    t = REGISTER_TEST("widgets", "widgets_tabbar_drawcalls");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTabBar("Tab Drawcalls"))
        {
            for (int i = 0; i < 20; i++)
                if (ImGui::BeginTabItem(Str30f("Tab %d", i).c_str()))
                    ImGui::EndTabItem();
            ImGui::EndTabBar();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window = ImGui::FindWindowByName("Test Window");
        ctx->WindowResize("Test Window", ImVec2(300, 300));
        int draw_calls = window->DrawList->CmdBuffer.Size;
        ctx->WindowResize("Test Window", ImVec2(1, 1));
        IM_CHECK(draw_calls == window->DrawList->CmdBuffer.Size);
    };

    // ## Test recursing Tab Bars (Bug #2371)
    t = REGISTER_TEST("widgets", "widgets_tabbar_recurse");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::BeginTabBar("TabBar 0"))
        {
            if (ImGui::BeginTabItem("TabItem"))
            {
                // If we have many tab bars here, it will invalidate pointers from pooled tab bars
                for (int i = 0; i < 128; i++)
                    if (ImGui::BeginTabBar(Str30f("Inner TabBar %d", i).c_str()))
                    {
                        if (ImGui::BeginTabItem("Inner TabItem"))
                            ImGui::EndTabItem();
                        ImGui::EndTabBar();
                    }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    };

#ifdef IMGUI_HAS_DOCK
    // ## Test Dockspace within a TabItem
    t = REGISTER_TEST("widgets", "widgets_tabbar_dockspace");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTabBar("TabBar"))
        {
            if (ImGui::BeginTabItem("TabItem"))
            {
                ImGui::DockSpace(ImGui::GetID("Hello"), ImVec2(0, 0));
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    };
#endif

    // ## Test SetSelected on first frame of a TabItem
    t = REGISTER_TEST("widgets", "widgets_tabbar_tabitem_setselected");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTabBar("tab_bar"))
        {
            if (ImGui::BeginTabItem("TabItem 0"))
            {
                ImGui::TextUnformatted("First tab content");
                ImGui::EndTabItem();
            }

            if (ctx->FrameCount >= 0)
            {
                bool tab_item_visible = ImGui::BeginTabItem("TabItem 1", NULL, ctx->FrameCount == 0 ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None);
                if (tab_item_visible)
                {
                    ImGui::TextUnformatted("Second tab content");
                    ImGui::EndTabItem();
                }
                if (ctx->FrameCount > 0)
                    IM_CHECK(tab_item_visible);
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) { ctx->Yield(); };

    // ## Test various TreeNode flags
    t = REGISTER_TEST("widgets", "widgets_treenode_behaviors");
    struct TreeNodeTestVars { bool Reset = true, IsOpen = false, IsMultiSelect = false; int ToggleCount = 0; ImGuiTreeNodeFlags Flags = 0; };
    t->SetUserDataType<TreeNodeTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Always);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        TreeNodeTestVars& vars = ctx->GetUserData<TreeNodeTestVars>();
        if (vars.Reset)
        {
            ImGui::GetStateStorage()->SetInt(ImGui::GetID("AAA"), 0);
            vars.ToggleCount = 0;
        }
        vars.Reset = false;
        ImGui::Text("Flags: 0x%08X, MultiSelect: %d", vars.Flags, vars.IsMultiSelect);

#ifdef IMGUI_HAS_MULTI_SELECT
        if (vars.IsMultiSelect)
        {
            ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_None, NULL, false); // Dummy, won't interact properly
            ImGui::SetNextItemSelectionData(NULL);
        }
#endif

        vars.IsOpen = ImGui::TreeNodeEx("AAA", vars.Flags);
        if (ImGui::IsItemToggledOpen())
            vars.ToggleCount++;
        if (vars.IsOpen)
            ImGui::TreePop();

#ifdef IMGUI_HAS_MULTI_SELECT
        if (vars.IsMultiSelect)
            ImGui::EndMultiSelect();
#endif

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TreeNodeTestVars& vars = ctx->GetUserData<TreeNodeTestVars>();
        ctx->WindowRef("Test Window");

#ifdef IMGUI_HAS_MULTI_SELECT
        int loop_count = 2;
#else
        int loop_count = 1;
#endif

        for (int loop_n = 0; loop_n < loop_count; loop_n++)
        {
            vars.IsMultiSelect = (loop_n == 1);

            if (!vars.IsMultiSelect) // _OpenOnArrow is implicit/automatic with MultiSelect
            {
                ctx->LogInfo("## ImGuiTreeNodeFlags_None, IsMultiSelect=%d", vars.IsMultiSelect);
                vars.Reset = true;
                vars.Flags = ImGuiTreeNodeFlags_None;
                ctx->Yield();
                IM_CHECK(vars.IsOpen == false && vars.ToggleCount == 0);

                // Click on arrow
                ctx->MouseMove("AAA", ImGuiTestOpFlags_MoveToEdgeL);
                ctx->MouseDown(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                ctx->MouseUp(0); // Toggle on Up with _OpenOnArrow (may change!)
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 4);

                // Click on main section 
                vars.ToggleCount = 0;
                ctx->MouseMove("AAA");
                ctx->MouseClick(0);
                IM_CHECK_EQ_NO_RET(vars.IsOpen, true);
                ctx->MouseClick(0);
                IM_CHECK_EQ_NO_RET(vars.IsOpen, false);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ_NO_RET(vars.IsOpen, false);
                IM_CHECK_EQ_NO_RET(vars.ToggleCount, 4);
            }

            if (!vars.IsMultiSelect) // _OpenOnArrow is implicit/automatic with MultiSelect
            {
                ctx->LogInfo("## ImGuiTreeNodeFlags_OpenOnDoubleClick, IsMultiSelect=%d", vars.IsMultiSelect);
                vars.Reset = true;
                vars.Flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
                ctx->Yield();
                IM_CHECK(vars.IsOpen == false && vars.ToggleCount == 0);

                // Click on arrow
                ctx->MouseMove("AAA", ImGuiTestOpFlags_MoveToEdgeL);
                ctx->MouseDown(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                ctx->MouseUp(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 0);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 2);

                // Click on main section
                vars.ToggleCount = 0;
                ctx->MouseMove("AAA");
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 0);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 2);
            }

            {
                ctx->LogInfo("## ImGuiTreeNodeFlags_OpenOnArrow, IsMultiSelect=%d", vars.IsMultiSelect);
                vars.Reset = true;
                vars.Flags = ImGuiTreeNodeFlags_OpenOnArrow;
                ctx->Yield();
                IM_CHECK(vars.IsOpen == false && vars.ToggleCount == 0);

                // Click on arrow
                ctx->MouseMove("AAA", ImGuiTestOpFlags_MoveToEdgeL);
                ctx->MouseDown(0);
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseUp(0);
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 2);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 4);

                // Click on main section
                vars.ToggleCount = 0;
                ctx->MouseMove("AAA");
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 0);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 0);
            }

            {
                ctx->LogInfo("## ImGuiTreeNodeFlags_OpenOnArrow|ImGuiTreeNodeFlags_OpenOnDoubleClick, IsMultiSelect=%d", vars.IsMultiSelect);
                vars.Reset = true;
                vars.Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
                ctx->Yield();
                IM_CHECK(vars.IsOpen == false && vars.ToggleCount == 0);

                // Click on arrow
                ctx->MouseMove("AAA", ImGuiTestOpFlags_MoveToEdgeL);
                ctx->MouseDown(0);
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseUp(0);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 2);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 4);

                // Click on main section
                vars.ToggleCount = 0;
                ctx->MouseMove("AAA");
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 0);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 2);
            }
        }
    };

    // ## Test ImGuiTreeNodeFlags_SpanAvailWidth and ImGuiTreeNodeFlags_SpanFullWidth flags
    t = REGISTER_TEST("widgets", "widgets_treenode_span_width");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Always);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        ImGui::SetNextItemOpen(true);
        if (ImGui::TreeNodeEx("Parent"))
        {
            // Interaction rect does not span entire width of work area.
            IM_CHECK(window->DC.LastItemRect.Max.x < window->WorkRect.Max.x);
            // But it starts at very beginning of WorkRect for first tree level.
            IM_CHECK(window->DC.LastItemRect.Min.x == window->WorkRect.Min.x);
            ImGui::SetNextItemOpen(true);
            if (ImGui::TreeNodeEx("Regular"))
            {
                // Interaction rect does not span entire width of work area.
                IM_CHECK(window->DC.LastItemRect.Max.x < window->WorkRect.Max.x);
                IM_CHECK(window->DC.LastItemRect.Min.x > window->WorkRect.Min.x);
                ImGui::TreePop();
            }
            ImGui::SetNextItemOpen(true);
            if (ImGui::TreeNodeEx("SpanAvailWidth", ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                // Interaction rect matches visible frame rect
                IM_CHECK((window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HasDisplayRect) != 0);
                IM_CHECK(window->DC.LastItemDisplayRect.Min == window->DC.LastItemRect.Min);
                IM_CHECK(window->DC.LastItemDisplayRect.Max == window->DC.LastItemRect.Max);
                // Interaction rect extends to the end of the available area.
                IM_CHECK(window->DC.LastItemRect.Max.x == window->WorkRect.Max.x);
                ImGui::TreePop();
            }
            ImGui::SetNextItemOpen(true);
            if (ImGui::TreeNodeEx("SpanFullWidth", ImGuiTreeNodeFlags_SpanFullWidth))
            {
                // Interaction rect matches visible frame rect
                IM_CHECK((window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HasDisplayRect) != 0);
                IM_CHECK(window->DC.LastItemDisplayRect.Min == window->DC.LastItemRect.Min);
                IM_CHECK(window->DC.LastItemDisplayRect.Max == window->DC.LastItemRect.Max);
                // Interaction rect extends to the end of the available area.
                IM_CHECK(window->DC.LastItemRect.Max.x == window->WorkRect.Max.x);
                // ImGuiTreeNodeFlags_SpanFullWidth also extends interaction rect to the left.
                IM_CHECK(window->DC.LastItemRect.Min.x == window->WorkRect.Min.x);
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        ImGui::End();
    };

    // ## Test PlotLines() with a single value (#2387).
    t = REGISTER_TEST("widgets", "widgets_plot_lines_unexpected_input");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        float values[1] = {0.f};
        ImGui::PlotLines("PlotLines 1", NULL, 0);
        ImGui::PlotLines("PlotLines 2", values, 0);
        ImGui::PlotLines("PlotLines 3", values, 1);
        // FIXME-TESTS: If test did not crash - it passed. A better way to check this would be useful.
    };

    // ## Test ColorEdit basic Drag and Drop
    t = REGISTER_TEST("widgets", "widgets_drag_coloredit");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(300, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::ColorEdit4("ColorEdit1", &vars.Vec4Array[0].x, ImGuiColorEditFlags_None);
        ImGui::ColorEdit4("ColorEdit2", &vars.Vec4Array[1].x, ImGuiColorEditFlags_None);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        vars.Vec4Array[0] = ImVec4(1, 0, 0, 1);
        vars.Vec4Array[1] = ImVec4(0, 1, 0, 1);

        ctx->WindowRef("Test Window");

        IM_CHECK_NE(memcmp(&vars.Vec4Array[0], &vars.Vec4Array[1], sizeof(ImVec4)), 0);
        ctx->ItemDragAndDrop("ColorEdit1/##ColorButton", "ColorEdit2/##X"); // FIXME-TESTS: Inner items
        IM_CHECK_EQ(memcmp(&vars.Vec4Array[0], &vars.Vec4Array[1], sizeof(ImVec4)), 0);
    };

    // ## Test BeginDragDropSource() with NULL id.
    t = REGISTER_TEST("widgets", "widgets_drag_source_null_id");
    struct WidgetDragSourceNullIDData
    {
        ImVec2 Source;
        ImVec2 Destination;
        bool Dropped = false;
    };
    t->SetUserDataType<WidgetDragSourceNullIDData>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        WidgetDragSourceNullIDData& user_data = *(WidgetDragSourceNullIDData*)ctx->UserData;

        ImGui::Begin("Null ID Test");
        ImGui::TextUnformatted("Null ID");
        user_data.Source = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()).GetCenter();

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            int magic = 0xF00;
            ImGui::SetDragDropPayload("MAGIC", &magic, sizeof(int));
            ImGui::EndDragDropSource();
        }
        ImGui::TextUnformatted("Drop Here");
        user_data.Destination = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()).GetCenter();

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MAGIC"))
            {
                user_data.Dropped = true;
                IM_CHECK_EQ(payload->DataSize, (int)sizeof(int));
                IM_CHECK_EQ(*(int*)payload->Data, 0xF00);
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        WidgetDragSourceNullIDData& user_data = *(WidgetDragSourceNullIDData*)ctx->UserData;

        // ImGui::TextUnformatted() does not have an ID therefore we can not use ctx->ItemDragAndDrop() as that refers
        // to items by their ID.
        ctx->MouseMoveToPos(user_data.Source);
        ctx->SleepShort();
        ctx->MouseDown(0);

        ctx->MouseMoveToPos(user_data.Destination);
        ctx->SleepShort();
        ctx->MouseUp(0);

        IM_CHECK(user_data.Dropped);
    };

    // ## Test overlapping drag and drop targets. The drag and drop system always prioritize the smaller target.
    t = REGISTER_TEST("widgets", "widgets_drag_overlapping_targets");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Button("Drag");
        if (ImGui::BeginDragDropSource())
        {
            int value = 0xF00D;
            ImGui::SetDragDropPayload("_TEST_VALUE", &value, sizeof(int));
            ImGui::EndDragDropSource();
        }

        auto render_button = [](ImGuiTestContext* ctx, const char* name, const ImVec2& pos, const ImVec2& size)
        {
            ImGui::SetCursorScreenPos(pos);
            ImGui::Button(name, size);
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_TEST_VALUE"))
                {
                    IM_UNUSED(payload);
                    ctx->GenericVars.Id = ImGui::GetItemID();
                }
                ImGui::EndDragDropTarget();
            }
        };

        // Render small button over big one
        ImVec2 pos = ImGui::GetCursorScreenPos();
        render_button(ctx, "Big1", pos, ImVec2(100, 100));
        render_button(ctx, "Small1", pos + ImVec2(25, 25), ImVec2(50, 50));

        // Render small button over small one
        render_button(ctx, "Small2", pos + ImVec2(0, 110) + ImVec2(25, 25), ImVec2(50, 50));
        render_button(ctx, "Big2", pos + ImVec2(0, 110), ImVec2(100, 100));

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Test Window");

        ctx->GenericVars.Id = 0;
        ctx->ItemDragAndDrop("Drag", "Small1");
        IM_CHECK(ctx->GenericVars.Id == ctx->GetID("Small1"));

        ctx->GenericVars.Id = 0;
        ctx->ItemDragAndDrop("Drag", "Small2");
        IM_CHECK(ctx->GenericVars.Id == ctx->GetID("Small2"));
    };

    // ## Test long text rendering by TextUnformatted().
    t = REGISTER_TEST("widgets", "widgets_text_unformatted_long");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuClick("Examples/Long text display");
        ctx->WindowRef("Example: Long text display");
        ctx->ItemClick("Add 1000 lines");
        ctx->SleepShort();

        Str64f title("/Example: Long text display\\/Log_%08X", ctx->GetID("Log"));
        ImGuiWindow* log_panel = ctx->GetWindowByRef(title.c_str());
        IM_CHECK(log_panel != NULL);
        ImGui::SetScrollY(log_panel, log_panel->ScrollMax.y);
        ctx->SleepShort();
        ctx->ItemClick("Clear");
        // FIXME-TESTS: A bit of extra testing that will be possible once tomato problem is solved.
        // ctx->ComboClick("Test type/Single call to TextUnformatted()");
        // ctx->ComboClick("Test type/Multiple calls to Text(), clipped");
        // ctx->ComboClick("Test type/Multiple calls to Text(), not clipped (slow)");
        ctx->WindowClose("");
    };

    // ## Test menu appending.
    t = REGISTER_TEST("widgets", "widgets_menu_append");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Append Menus", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);
        ImGui::BeginMenuBar();

        // Menu that we will append to.
        if (ImGui::BeginMenu("First Menu"))
        {
            ImGui::MenuItem("1 First");
            if (ImGui::BeginMenu("Second Menu"))
            {
                ImGui::MenuItem("2 First");
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        // Append to first menu.
        if (ImGui::BeginMenu("First Menu"))
        {
            if (ImGui::MenuItem("1 Second"))
                ctx->GenericVars.Bool1 = true;
            if (ImGui::BeginMenu("Second Menu"))
            {
                ImGui::MenuItem("2 Second");
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Append Menus");
        ctx->MenuClick("First Menu");
        ctx->MenuClick("First Menu/1 First");
        IM_CHECK_EQ(ctx->GenericVars.Bool1, false);
        ctx->MenuClick("First Menu/1 Second");
        IM_CHECK_EQ(ctx->GenericVars.Bool1, true);
        ctx->MenuClick("First Menu/Second Menu/2 First");
        ctx->MenuClick("First Menu/Second Menu/2 Second");
    };

    // ## Test main menubar appending.
    t = REGISTER_TEST("widgets", "widgets_main_menubar_append");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // Menu that we will append to.
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("First Menu"))
                ImGui::EndMenu();
            ImGui::EndMenuBar();
        }

        // Append to first menu.
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Second Menu"))
            {
                if (ImGui::MenuItem("Second"))
                    ctx->GenericVars.Bool1 = true;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("##MainMenuBar"
        );
        ctx->MenuClick("Second Menu/Second");
        IM_CHECK_EQ(ctx->GenericVars.Bool1, true);
    };

#ifdef IMGUI_HAS_MULTI_SELECT
    // ## Test MultiSelect API
    struct ExampleSelection
    {
        ImGuiStorage                        Storage;
        int                                 SelectionSize;  // Number of selected items (== number of 1 in the Storage, maintained by this class)
        int                                 RangeRef;       // Reference/pivot item (generally last clicked item)

        ExampleSelection()                  { RangeRef = 0; Clear(); }
        void Clear()                        { Storage.Clear(); SelectionSize = 0; }
        bool GetSelected(int n) const       { return Storage.GetInt((ImGuiID)n, 0) != 0; }
        void SetSelected(int n, bool v)     { int* p_int = Storage.GetIntRef((ImGuiID)n, 0); if (*p_int == (int)v) return; if (v) SelectionSize++; else SelectionSize--; *p_int = (bool)v; }
        int  GetSelectionSize() const       { return SelectionSize; }

        // When using SelectAll() / SetRange() we assume that our objects ID are indices.
        // In this demo we always store selection using indices and never in another manner (e.g. object ID or pointers).
        // If your selection system is storing selection using object ID and you want to support Shift+Click range-selection, 
        // you will need a way to iterate from one object to another given the ID you use.
        // You are likely to need some kind of data structure to convert 'view index' <> 'object ID'.
        // FIXME-MULTISELECT: Would be worth providing a demo of doing this.
        // FIXME-MULTISELECT: SetRange() is currently very inefficient since it doesn't take advantage of the fact that ImGuiStorage stores sorted key.
        void SetRange(int n1, int n2, bool v)   { if (n2 < n1) { int tmp = n2; n2 = n1; n1 = tmp; } for (int n = n1; n <= n2; n++) SetSelected(n, v); }
        void SelectAll(int count)               { Storage.Data.resize(count); for (int idx = 0; idx < count; idx++) Storage.Data[idx] = ImGuiStorage::ImGuiStoragePair((ImGuiID)idx, 1); SelectionSize = count; } // This could be using SetRange(), but it this way is faster.
    };
    struct MultiSelectTestVars 
    {
        ExampleSelection    Selection;
    };
    auto multiselect_guifunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetUserData<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection;

        const int ITEMS_COUNT = 100;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("(Size = %d items)", selection.SelectionSize);
        ImGui::Text("(RangeRef = %04d)", selection.RangeRef);
        ImGui::Separator();

        ImGuiMultiSelectData* multi_select_data = ImGui::BeginMultiSelect(0, (void*)(intptr_t)selection.RangeRef, selection.GetSelected(selection.RangeRef));
        if (multi_select_data->RequestClear)     { selection.Clear(); }
        if (multi_select_data->RequestSelectAll) { selection.SelectAll(ITEMS_COUNT); }

        if (ctx->Test->ArgVariant == 1)
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));

        ImGuiListClipper clipper(ITEMS_COUNT);
        while (clipper.Step())
        {
            if (clipper.DisplayStart > selection.RangeRef)
                multi_select_data->RangeSrcPassedBy = true;
            for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
            {
                Str64f label("Object %04d", n);
                bool item_is_selected = selection.GetSelected(n);

                ImGui::SetNextItemSelectionData((void*)(intptr_t)n);
                if (ctx->Test->ArgVariant == 0)
                {
                    ImGui::Selectable(label.c_str(), item_is_selected);
                    bool toggled = ImGui::IsItemToggledSelection();
                    if (toggled)
                        selection.SetSelected(n, !item_is_selected);
                }
                else if (ctx->Test->ArgVariant == 1)
                {
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
                    flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
                    if (item_is_selected)
                        flags |= ImGuiTreeNodeFlags_Selected;
                    if (ImGui::TreeNodeEx(label.c_str(), flags))
                        ImGui::TreePop();
                    if (ImGui::IsItemToggledSelection())
                        selection.SetSelected(n, !item_is_selected);
                }
            }
        }

        if (ctx->Test->ArgVariant == 1)
            ImGui::PopStyleVar();

        // Apply multi-select requests
        multi_select_data = ImGui::EndMultiSelect();
        selection.RangeRef = (int)(intptr_t)multi_select_data->RangeSrc;
        if (multi_select_data->RequestClear)     { selection.Clear(); }
        if (multi_select_data->RequestSelectAll) { selection.SelectAll(ITEMS_COUNT); }
        if (multi_select_data->RequestSetRange)  { selection.SetRange((int)(intptr_t)multi_select_data->RangeSrc, (int)(intptr_t)multi_select_data->RangeDst, multi_select_data->RangeValue ? 1 : 0); }
        ImGui::End();
    };
    auto multiselect_testfunc = [](ImGuiTestContext* ctx)
    {
        // We are using lots of MouseMove+MouseDown+MouseUp (instead of ItemClick) because we need to test precise MouseUp vs MouseDown reactions.
        ImGuiContext& g = *ctx->UiContext;
        MultiSelectTestVars& vars = ctx->GetUserData<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection;

        ctx->WindowRef("Test Window");

        selection.Clear();
        ctx->Yield();
        IM_CHECK_EQ(selection.SelectionSize, 0);

        // Single click
        ctx->ItemClick("Object 0000");
        IM_CHECK_EQ(selection.SelectionSize, 1);
        IM_CHECK_EQ(selection.GetSelected(0), true);

        // Verify that click on another item alter selection on MouseDown
        ctx->MouseMove("Object 0001");
        ctx->MouseDown(0);
        IM_CHECK_EQ(selection.SelectionSize, 1);
        IM_CHECK_EQ(selection.GetSelected(1), true);
        ctx->MouseUp(0);

        // CTRL-A
        ctx->KeyPressMap(ImGuiKey_A, ImGuiKeyModFlags_Ctrl);
        IM_CHECK_EQ(selection.SelectionSize, 100);

        // Verify that click on selected item clear other items from selection on MouseUp
        ctx->MouseMove("Object 0001");
        ctx->MouseDown(0);
        IM_CHECK_EQ(selection.SelectionSize, 100);
        ctx->MouseUp(0);
        IM_CHECK_EQ(selection.SelectionSize, 1);
        IM_CHECK_EQ(selection.GetSelected(1), true);

        // Test SHIFT+Click
        ctx->ItemClick("Object 0001");
        ctx->KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
        ctx->MouseMove("Object 0006");
        ctx->MouseDown(0);
        IM_CHECK_EQ(selection.SelectionSize, 6);
        ctx->MouseUp(0);
        ctx->KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);

        // Test that CTRL+A preserve RangeSrc (which was 0001)
        ctx->KeyPressMap(ImGuiKey_A, ImGuiKeyModFlags_Ctrl);
        IM_CHECK_EQ(selection.SelectionSize, 100);
        ctx->KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
        ctx->ItemClick("Object 0008");
        ctx->KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
        IM_CHECK_EQ(selection.SelectionSize, 8);

        // Test reverse clipped SHIFT+Click
        // FIXME-TESTS: Locate query could disable clipper?
        // FIXME-TESTS: We would need to disable clipper because it conveniently rely on cliprect which is affected by actual viewport, so ScrollToBottom() is not enough...
        //ctx->ScrollToBottom();
        ctx->ItemClick("Object 0030");
        ctx->ScrollToTop();
        ctx->KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
        ctx->ItemClick("Object 0002");
        ctx->KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
        IM_CHECK_EQ(selection.SelectionSize, 29);

        // Test ESC to clear selection
        // FIXME-TESTS
#if 0
        ctx->KeyPressMap(ImGuiKey_Escape);
        ctx->Yield();
        IM_CHECK_EQ(selection.SelectionSize, 0);
#endif

        // Test SHIFT+Arrow
        ctx->ItemClick("Object 0002");
        IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0002"));
        IM_CHECK_EQ(selection.SelectionSize, 1);
        ctx->KeyPressMap(ImGuiKey_DownArrow, ImGuiKeyModFlags_Shift);
        ctx->KeyPressMap(ImGuiKey_DownArrow, ImGuiKeyModFlags_Shift);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0004"));
        IM_CHECK_EQ(selection.SelectionSize, 3);

        // Test CTRL+Arrow
        ctx->KeyPressMap(ImGuiKey_DownArrow, ImGuiKeyModFlags_Ctrl);
        ctx->KeyPressMap(ImGuiKey_DownArrow, ImGuiKeyModFlags_Ctrl);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0006"));
        IM_CHECK_EQ(selection.SelectionSize, 3);

        // Test SHIFT+Arrow after a gap
        ctx->KeyPressMap(ImGuiKey_DownArrow, ImGuiKeyModFlags_Shift);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0007"));
        IM_CHECK_EQ(selection.SelectionSize, 6);

        // Test SHIFT+Arrow reducing selection
        ctx->KeyPressMap(ImGuiKey_UpArrow, ImGuiKeyModFlags_Shift);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0006"));
        IM_CHECK_EQ(selection.SelectionSize, 5);

        // Test CTRL+Shift+Arrow moving or appending without reducing selection
        ctx->KeyPressMap(ImGuiKey_UpArrow, ImGuiKeyModFlags_Ctrl | ImGuiKeyModFlags_Shift, 4);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0002"));
        IM_CHECK_EQ(selection.SelectionSize, 5);

        // Test SHIFT+Arrow replacing selection
        ctx->KeyPressMap(ImGuiKey_UpArrow, ImGuiKeyModFlags_Shift);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0001"));
        IM_CHECK_EQ(selection.SelectionSize, 2);

        // Test Arrow replacing selection
        ctx->KeyPressMap(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0002"));
        IM_CHECK_EQ(selection.SelectionSize, 1);
        IM_CHECK_EQ(selection.GetSelected(2), true);

        // Test Home/End
        ctx->KeyPressMap(ImGuiKey_Home);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0000"));
        IM_CHECK_EQ(selection.SelectionSize, 1);
        IM_CHECK_EQ(selection.GetSelected(0), true);
        ctx->KeyPressMap(ImGuiKey_End);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0099"));
        IM_CHECK_EQ(selection.SelectionSize, 1);
        IM_CHECK_EQ(selection.GetSelected(99), true); // Would break if clipped by viewport
        ctx->KeyPressMap(ImGuiKey_Home, ImGuiKeyModFlags_Ctrl);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0000"));
        IM_CHECK_EQ(selection.SelectionSize, 1);
        IM_CHECK_EQ(selection.GetSelected(99), true);
        ctx->KeyPressMap(ImGuiKey_Home);
        IM_CHECK_EQ(selection.SelectionSize, 1);
        IM_CHECK_EQ(selection.GetSelected(99), true); // FIXME: A Home/End/PageUp/PageDown leading to same target doesn't trigger JustMovedTo, may be reasonable.
        ctx->KeyPressMap(ImGuiKey_Space);
        IM_CHECK_EQ(selection.SelectionSize, 1);
        IM_CHECK_EQ(selection.GetSelected(0), true);
        ctx->KeyPressMap(ImGuiKey_End, ImGuiKeyModFlags_Shift);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0099"));
        IM_CHECK_EQ(selection.SelectionSize, 100);
    };

    t = REGISTER_TEST("widgets", "widgets_multiselect_1_selectables");
    t->SetUserDataType<MultiSelectTestVars>();
    t->GuiFunc = multiselect_guifunc;
    t->TestFunc = multiselect_testfunc;
    t->ArgVariant = 0;

    t = REGISTER_TEST("widgets", "widgets_multiselect_2_treenode");
    t->SetUserDataType<MultiSelectTestVars>();
    t->GuiFunc = multiselect_guifunc;
    t->TestFunc = multiselect_testfunc;
    t->ArgVariant = 1;
#endif
}

//-------------------------------------------------------------------------
// Tests: Nav
//-------------------------------------------------------------------------

void RegisterTests_Nav(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test opening a new window from a checkbox setting the focus to the new window.
    // In 9ba2028 (2019/01/04) we fixed a bug where holding ImGuiNavInputs_Activate too long on a button would hold the focus on the wrong window.
    t = REGISTER_TEST("nav", "nav_basic");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetInputMode(ImGuiInputSource_Nav);
        ctx->WindowRef("Hello, world!");
        ctx->ItemUncheck("Demo Window");
        ctx->ItemCheck("Demo Window");

        ImGuiContext& g = *ctx->UiContext;
        IM_CHECK(g.NavWindow && g.NavWindow->ID == ctx->GetID("/Dear ImGui Demo"));
    };

    // ## Test that CTRL+Tab steal active id (#2380)
    t = REGISTER_TEST("nav", "nav_ctrl_tab_takes_activeid_away");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("This is test window 1");
        ImGui::InputText("InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        ImGui::End();
        ImGui::Begin("Test window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("This is test window 2");
        ImGui::InputText("InputText", vars.Str2, IM_ARRAYSIZE(vars.Str2));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // FIXME-TESTS: Fails if window is resized too small
        ctx->SetInputMode(ImGuiInputSource_Nav);
        ctx->WindowRef("Test window 1");
        ctx->ItemInput("InputText");
        ctx->KeyCharsAppend("123");
        IM_CHECK_EQ(ctx->UiContext->ActiveId, ctx->GetID("InputText"));
        ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Ctrl);
        IM_CHECK_EQ(ctx->UiContext->ActiveId, (ImGuiID)0);
        ctx->Sleep(1.0f);
    };

    // ## Test that ESC deactivate InputText without closing current Popup (#2321, #787)
    t = REGISTER_TEST("nav", "nav_esc_popup");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        bool& b_popup_open = vars.Bool1;
        bool& b_field_active = vars.Bool2;
        ImGuiID& popup_id = vars.Id;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Open Popup"))
            ImGui::OpenPopup("Popup");

        b_popup_open = ImGui::BeginPopup("Popup");
        if (b_popup_open)
        {
            popup_id = ImGui::GetCurrentWindow()->ID;
            ImGui::InputText("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            b_field_active = ImGui::IsItemActive();
            ImGui::EndPopup();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        bool& b_popup_open = vars.Bool1;
        bool& b_field_active = vars.Bool2;

        // FIXME-TESTS: Come up with a better mechanism to get popup ID
        ImGuiID& popup_id = vars.Id;
        popup_id = 0;

        ctx->WindowRef("Test Window");
        ctx->ItemClick("Open Popup");

        while (popup_id == 0 && !ctx->IsError())
            ctx->Yield();

        ctx->WindowRef(popup_id);
        ctx->ItemClick("Field");
        IM_CHECK(b_popup_open);
        IM_CHECK(b_field_active);

        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK(b_popup_open);
        IM_CHECK(!b_field_active);
    };

    // ## Test AltGr doesn't trigger menu layer
    t = REGISTER_TEST("nav", "nav_altgr_no_menu");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Menu"))
            {
                ImGui::Text("Blah");
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // FIXME-TESTS: Fails if window is resized too small
        IM_CHECK(ctx->UiContext->IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard);
        //ctx->SetInputMode(ImGuiInputSource_Nav);
        ctx->WindowRef("Test window");
        IM_CHECK(ctx->UiContext->NavLayer == ImGuiNavLayer_Main);
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);
        IM_CHECK(ctx->UiContext->NavLayer == ImGuiNavLayer_Menu);
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);
        IM_CHECK(ctx->UiContext->NavLayer == ImGuiNavLayer_Main);
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt | ImGuiKeyModFlags_Ctrl);
        IM_CHECK(ctx->UiContext->NavLayer == ImGuiNavLayer_Main);
    };

    // ## Test navigation home and end keys
    t = REGISTER_TEST("nav", "nav_home_end_keys");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(100, 150));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        for (int i = 0; i < 10; i++)
            ImGui::Button(Str30f("Button %d", i).c_str());
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        IM_CHECK(ctx->UiContext->IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard);
        ImGuiWindow* window = ImGui::FindWindowByName("Test Window");
        ctx->WindowRef("Test window");
        ctx->SetInputMode(ImGuiInputSource_Nav);

        // FIXME-TESTS: This should not be required but nav init request is not applied until we start navigating, this is a workaround
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);

        IM_CHECK(ctx->UiContext->NavId == window->GetID("Button 0"));
        IM_CHECK(window->Scroll.y == 0);
        // Navigate to the middle of window
        for (int i = 0; i < 5; i++)
            ctx->KeyPressMap(ImGuiKey_DownArrow);
        IM_CHECK(ctx->UiContext->NavId == window->GetID("Button 5"));
        IM_CHECK(window->Scroll.y > 0 && window->Scroll.y < window->ScrollMax.y);
        // From the middle to the end
        ctx->KeyPressMap(ImGuiKey_End);
        IM_CHECK(ctx->UiContext->NavId == window->GetID("Button 9"));
        IM_CHECK(window->Scroll.y == window->ScrollMax.y);
        // From the end to the start
        ctx->KeyPressMap(ImGuiKey_Home);
        IM_CHECK(ctx->UiContext->NavId == window->GetID("Button 0"));
        IM_CHECK(window->Scroll.y == 0);
    };

    // ## Test vertical wrap-around in menus/popups
    t = REGISTER_TEST("nav", "nav_popup_wraparound");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuClick("Menu");
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);
        IM_CHECK(ctx->UiContext->NavId == ctx->GetID("/##Menu_00/New"));
        ctx->NavKeyPress(ImGuiNavInput_KeyUp_);
        IM_CHECK(ctx->UiContext->NavId == ctx->GetID("/##Menu_00/Quit"));
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
        IM_CHECK(ctx->UiContext->NavId == ctx->GetID("/##Menu_00/New"));
    };

    // ## Test CTRL+TAB window focusing
    t = REGISTER_TEST("nav", "nav_ctrl_tab_focusing");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextUnformatted("Not empty space");
        ImGui::End();

        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Button("Button Out");
        ImGui::BeginChild("Child", ImVec2(50, 50), true);
        ImGui::Button("Button In");
        ImGui::EndChild();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiWindow* window2 = ctx->GetWindowByRef("Window 2");

        // FIXME-TESTS: Facilitate usage of variants
        const int test_count = ctx->HasDock ? 2 : 1;
        for (int test_n = 0; test_n < test_count; test_n++)
        {
            ctx->LogDebug("TEST CASE %d", test_n);
#ifdef IMGUI_HAS_DOCK
            ctx->DockMultiClear("Window 1", "Window 2", NULL);
            if (test_n == 1)
                ctx->DockWindowInto("Window 1", "Window 2");
#endif

            // Set up window focus order.
            ctx->WindowFocus("Window 1");
            ctx->WindowFocus("Window 2");

            ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Ctrl);
            IM_CHECK(g.NavWindow == ctx->GetWindowByRef("Window 1"));
            ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Ctrl);
            IM_CHECK(g.NavWindow == ctx->GetWindowByRef("Window 2"));

            // Set up window focus order, focus child window.
            ctx->WindowFocus("Window 1");
            ctx->WindowFocus("Window 2"); // FIXME: Needed for case when docked
            ctx->ItemClick(Str30f("Window 2\\/Child_%08X/Button In", window2->GetID("Child")).c_str());

            ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Ctrl);
            IM_CHECK(g.NavWindow == ctx->GetWindowByRef("Window 1"));
        }
    };

    // ## Test NavID restoration during CTRL+TAB focusing
    t = REGISTER_TEST("nav", "nav_ctrl_tab_nav_id_restore");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Button("Button 1");
        ImGui::End();

        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::BeginChild("Child", ImVec2(50, 50), true);
        ImGui::Button("Button 2");
        ImGui::EndChild();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;

        // FIXME-TESTS: Facilitate usage of variants
        const int test_count = ctx->HasDock ? 2 : 1;
        for (int test_n = 0; test_n < test_count; test_n++)
        {
            ctx->LogDebug("TEST CASE %d", test_n);
#ifdef IMGUI_HAS_DOCK
            ctx->DockMultiClear("Window 1", "Window 2", NULL);
            if (test_n == 1)
                ctx->DockWindowInto("Window 2", "Window 1");
#endif

            ImGuiWindow* window2 = ctx->GetWindowByRef("Window 2");

            Str30  win1_button_ref("Window 1/Button 1");
            Str30f win2_button_ref("Window 2\\/Child_%08X/Button 2", window2->GetID("Child"));

            // Focus Window 1, navigate to the button
            ctx->WindowFocus("Window 1");
            ctx->NavMoveTo(win1_button_ref.c_str());

            // Focus Window 2, ensure nav id was changed, navigate to the button
            ctx->WindowFocus("Window 2");
            IM_CHECK_NE(ctx->GetID(win1_button_ref.c_str()), g.NavId);
            ctx->NavMoveTo(win2_button_ref.c_str());

            // Ctrl+Tab back to previous window, check if nav id was restored
            ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Ctrl);
            IM_CHECK_EQ(ctx->GetID(win1_button_ref.c_str()), g.NavId);

            // Ctrl+Tab back to previous window, check if nav id was restored
            ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Ctrl);
            IM_CHECK_EQ(ctx->GetID(win2_button_ref.c_str()), g.NavId);
        }
    };

    // ## Test NavID restoration when focusing another window or STOPPING to submit another world
    t = REGISTER_TEST("nav", "nav_focus_restore");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Button("Button 1");
        ImGui::End();

        if (!ctx->GenericVars.Bool1)
            return;
        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Button("Button 2");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;

        // FIXME-TESTS: Facilitate usage of variants
        const int test_count = ctx->HasDock ? 2 : 1;
        for (int test_n = 0; test_n < test_count; test_n++)
        {
            ctx->LogDebug("TEST CASE %d", test_n);
            ctx->GenericVars.Bool1 = true;
            ctx->YieldFrames(2);
#ifdef IMGUI_HAS_DOCK
            ctx->DockMultiClear("Window 1", "Window 2", NULL);
            if (test_n == 1)
                ctx->DockWindowInto("Window 2", "Window 1");
#endif
            ctx->WindowFocus("Window 1");
            ctx->NavMoveTo("Window 1/Button 1");

            ctx->WindowFocus("Window 2");
            ctx->NavMoveTo("Window 2/Button 2");

            ctx->WindowFocus("Window 1");
            IM_CHECK_EQ(g.NavId, ctx->GetID("Window 1/Button 1"));

            ctx->WindowFocus("Window 2");
            IM_CHECK_EQ(g.NavId, ctx->GetID("Window 2/Button 2"));

            ctx->GenericVars.Bool1 = false;
            ctx->YieldFrames(2);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Window 1/Button 1"));
        }
    };

    // ## Test NavID restoration after activating menu item.
    t = REGISTER_TEST("nav", "nav_focus_restore_menus");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;

        // FIXME-TESTS: Facilitate usage of variants
        const int test_count = ctx->HasDock ? 2 : 1;
        for (int test_n = 0; test_n < test_count; test_n++)
        {
            ctx->LogDebug("TEST CASE %d", test_n);
#ifdef IMGUI_HAS_DOCK
            ctx->WindowRef(ImGuiTestRef());
            ctx->DockMultiClear("Dear ImGui Demo", "Hello, world!", NULL);
            if (test_n == 0)
                ctx->DockWindowInto("Dear ImGui Demo", "Hello, world!");
#endif
            ctx->WindowRef("Dear ImGui Demo");
            // Focus item.
            ctx->NavMoveTo("Configuration");
            // Focus menu.
            ctx->NavKeyPress(ImGuiNavInput_Menu);
            // Open menu, focus first item in the menu.
            ctx->NavActivate();
            // Activate first item in the menu.
            ctx->NavActivate();
            // Verify NavId was restored to initial value.
            IM_CHECK_EQ(g.NavId, ctx->GetID("Configuration"));
        }
    };
}

//-------------------------------------------------------------------------
// Tests: Columns
//-------------------------------------------------------------------------

void RegisterTests_Columns(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test number of draw calls used by columns
    t = REGISTER_TEST("columns", "columns_draw_calls");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Hello");
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Test: Single column don't consume draw call.
        int cmd_count = draw_list->CmdBuffer.Size;
        ImGui::BeginColumns("columns1", 1);
        ImGui::Text("AAAA"); ImGui::NextColumn();
        ImGui::Text("BBBB"); ImGui::NextColumn();
        ImGui::EndColumns();
        ImGui::Text("Hello");
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, cmd_count + 0);

        // Test: Multi-column consume 1 draw call per column + 1 due to conservative overlap expectation (FIXME)
        ImGui::BeginColumns("columns3", 3);
        ImGui::Text("AAAA"); ImGui::NextColumn();
        ImGui::Text("BBBB"); ImGui::NextColumn();
        ImGui::Text("CCCC"); ImGui::NextColumn();
        ImGui::EndColumns();
        ImGui::Text("Hello");
        IM_CHECK(draw_list->CmdBuffer.Size == cmd_count || draw_list->CmdBuffer.Size == cmd_count + 3 + 1);

        // Test: Unused column don't consume a draw call
        cmd_count = draw_list->CmdBuffer.Size;
        ImGui::BeginColumns("columns3", 3);
        ImGui::Text("AAAA"); ImGui::NextColumn();
        ImGui::Text("BBBB"); ImGui::NextColumn(); // Leave one column empty
        ImGui::EndColumns();
        ImGui::Text("Hello");
        IM_CHECK(draw_list->CmdBuffer.Size == cmd_count || draw_list->CmdBuffer.Size == cmd_count + 2 + 1);

        // Test: Separators in columns don't consume a draw call
        cmd_count = draw_list->CmdBuffer.Size;
        ImGui::BeginColumns("columns3", 3);
        ImGui::Text("AAAA"); ImGui::NextColumn();
        ImGui::Text("BBBB"); ImGui::NextColumn();
        ImGui::Text("CCCC"); ImGui::NextColumn();
        ImGui::Separator();
        ImGui::Text("1111"); ImGui::NextColumn();
        ImGui::Text("2222"); ImGui::NextColumn();
        ImGui::Text("3333"); ImGui::NextColumn();
        ImGui::EndColumns();
        ImGui::Text("Hello");
        IM_CHECK(draw_list->CmdBuffer.Size == cmd_count || draw_list->CmdBuffer.Size == cmd_count + 3 + 1);

        // Test: Selectables in columns don't consume a draw call
        cmd_count = draw_list->CmdBuffer.Size;
        ImGui::BeginColumns("columns3", 3);
        ImGui::Selectable("AAAA", true, ImGuiSelectableFlags_SpanAllColumns); ImGui::NextColumn();
        ImGui::Text("BBBB"); ImGui::NextColumn();
        ImGui::Text("CCCC"); ImGui::NextColumn();
        ImGui::Selectable("1111", true, ImGuiSelectableFlags_SpanAllColumns); ImGui::NextColumn();
        ImGui::Text("2222"); ImGui::NextColumn();
        ImGui::Text("3333"); ImGui::NextColumn();
        ImGui::EndColumns();
        ImGui::Text("Hello");
        IM_CHECK(draw_list->CmdBuffer.Size == cmd_count || draw_list->CmdBuffer.Size == cmd_count + 3 + 1);

        ImGui::End();
    };

    // ## Test behavior of some Column functions without Columns/BeginColumns.
    t = REGISTER_TEST("columns", "columns_functions_without_columns");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        IM_CHECK_EQ(ImGui::GetColumnsCount(), 1);
        IM_CHECK_EQ(ImGui::GetColumnOffset(), 0.0f);
        IM_CHECK_EQ(ImGui::GetColumnWidth(), ImGui::GetContentRegionAvail().x);
        IM_CHECK_EQ(ImGui::GetColumnIndex(), 0);
        ImGui::End();
    };
}

#ifdef IMGUI_HAS_TABLE
static void HelperTableSubmitCells(int count_w, int count_h)
{
    for (int line = 0; line < count_h; line++)
    {
        ImGui::TableNextRow();
        for (int column = 0; column < count_w; column++)
        {
            if (!ImGui::TableSetColumnIndex(column))
                continue;
            Str16f label("%d,%d", line, column);
            //ImGui::TextUnformatted(label.c_str());
            ImGui::Button(label.c_str(), ImVec2(-FLT_MIN, 0.0f));
        }
    }
}

// columns_desc = "WWW", "FFW", "FAA" etc.
static void HelperTableWithResizingPolicies(const char* table_id, ImGuiTableFlags table_flags, const char* columns_desc)
{
    table_flags |= ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Borders;

    int columns_count = (int)strlen(columns_desc);
    IM_ASSERT(columns_count < 26); // Because we are using alphabetical letters for names
    if (!ImGui::BeginTable(table_id, columns_count, table_flags))
        return;
    for (int column = 0; column < columns_count; column++)
    {
        const char policy = columns_desc[column];
        ImGuiTableColumnFlags column_flags = ImGuiTableColumnFlags_None;
        if      (policy >= 'a' && policy <= 'z') { column_flags |= ImGuiTableColumnFlags_DefaultHide; }
        if      (policy == 'f' || policy == 'F') { column_flags |= ImGuiTableColumnFlags_WidthFixed; }
        else if (policy == 'w' || policy == 'W') { column_flags |= ImGuiTableColumnFlags_WidthStretch; }
        else if (policy == 'a' || policy == 'A') { column_flags |= ImGuiTableColumnFlags_WidthAlwaysAutoResize; }
        else IM_ASSERT(0);
        ImGui::TableSetupColumn(Str16f("%c%d", policy, column + 1).c_str(), column_flags);
    }
    ImFont* font = FindFontByName("Roboto-Medium.ttf, 16px");
    if (!font)
        IM_CHECK_NO_RET(font != NULL);
    ImGui::PushFont(font);
    ImGui::TableAutoHeaders();
    ImGui::PopFont();
    for (int row = 0; row < 2; row++)
    {
        ImGui::TableNextRow();
        for (int column = 0; column < columns_count; column++)
        {
            ImGui::TableSetColumnIndex(column);
            const char* column_desc = "Unknown";
            const char policy = columns_desc[column];
            if (policy == 'F') { column_desc = "Fixed"; }
            if (policy == 'W') { column_desc = "Stretch"; }
            if (policy == 'A') { column_desc = "Auto"; }
            ImGui::Text("%s %d,%d", column_desc, row, column);
        }
    }
    ImGui::EndTable();
}
#endif // #ifdef IMGUI_HAS_TABLE

void RegisterTests_Table(ImGuiTestEngine* e)
{
#ifdef IMGUI_HAS_TABLE
    ImGuiTest* t = NULL;

    t = REGISTER_TEST("table", "table_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTable("##table0", 4))
        {
            ImGui::TableSetupColumn("One", ImGuiTableColumnFlags_WidthFixed, 100.0f, 0);
            ImGui::TableSetupColumn("Two");
            ImGui::TableSetupColumn("Three");
            ImGui::TableSetupColumn("Four");
            HelperTableSubmitCells(4, 5);
            ImGuiTable* table = ctx->UiContext->CurrentTable;
            IM_CHECK_EQ(table->Columns[0].WidthRequested, 100.0f);
            ImGui::EndTable();
        }
        ImGui::End();
    };

    // ## Table: measure draw calls count
    // FIXME-TESTS: Resize window width to e.g. ideal size first, then resize down
    // Important: HelperTableSubmitCells uses Button() with -1 width which will CPU clip text, so we don't have interference from the contents here.
    t = REGISTER_TEST("table", "table_2_draw_calls");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImGui::Text("Text before");
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("##table1", 4, ImGuiTableFlags_NoClipX | ImGuiTableFlags_Borders, ImVec2(400, 0)))
            {
                HelperTableSubmitCells(4, 5);
                ImGui::EndTable();
            }
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("##table2", 4, ImGuiTableFlags_Borders, ImVec2(400, 0)))
            {
                HelperTableSubmitCells(4, 4);
                ImGui::EndTable();
            }
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("##table3", 4, ImGuiTableFlags_Borders, ImVec2(400, 0)))
            {
                ImGui::TableSetupColumn("One");
                ImGui::TableSetupColumn("TwoTwo");
                ImGui::TableSetupColumn("ThreeThreeThree");
                ImGui::TableSetupColumn("FourFourFourFour");
                ImGui::TableAutoHeaders();
                HelperTableSubmitCells(4, 4);
                ImGui::EndTable();
            }
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("##table4", 3, ImGuiTableFlags_Borders))
            {
                ImGui::TableSetupColumn("One");
                ImGui::TableSetupColumn("TwoTwo");
                ImGui::TableSetupColumn("ThreeThreeThree");
                ImGui::TableAutoHeaders();
                HelperTableSubmitCells(3, 4);
                ImGui::EndTable();
            }
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        ImGui::End();
    };

    // ## Table: measure equal width
    t = REGISTER_TEST("table", "table_3_width");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        struct TestCase
        {
            int             ColumnCount;
            ImGuiTableFlags Flags;
        };

        TestCase test_cases[] =
        {
            { 2, ImGuiTableFlags_None },
            { 3, ImGuiTableFlags_None },
            { 9, ImGuiTableFlags_None },
            { 2, ImGuiTableFlags_BordersOuter },
            { 3, ImGuiTableFlags_BordersOuter },
            { 9, ImGuiTableFlags_BordersOuter },
            { 2, ImGuiTableFlags_BordersV },
            { 3, ImGuiTableFlags_BordersV },
            { 9, ImGuiTableFlags_BordersV },
            { 2, ImGuiTableFlags_Borders },
            { 3, ImGuiTableFlags_Borders },
            { 9, ImGuiTableFlags_Borders },
        };

        ImGui::Text("(width variance should be <= 1.0f)");
        for (int test_case_n = 0; test_case_n < IM_ARRAYSIZE(test_cases); test_case_n++)
        {
            const TestCase& tc = test_cases[test_case_n];
            ImGui::PushID(test_case_n);

            ImGui::Spacing();
            ImGui::Spacing();
            if (ImGui::BeginTable("##table", tc.ColumnCount, tc.Flags, ImVec2(0, 0)))
            {
                ImGui::TableNextRow();

                float min_w = FLT_MAX;
                float max_w = -FLT_MAX;
                for (int n = 0; n < tc.ColumnCount; n++)
                {
                    ImGui::TableSetColumnIndex(n);
                    float w = ImGui::GetContentRegionAvail().x;
                    min_w = ImMin(w, min_w);
                    max_w = ImMax(w, max_w);
                    ImGui::Text("Width %.2f", w);
                }
                float w_variance = max_w - min_w;
                IM_CHECK_LE_NO_RET(w_variance, 1.0f);
                ImGui::EndTable();

                if (w_variance > 1.0f)
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
                ImGui::Text("#%02d: Variance %.2f (min %.2f max %.2f)", test_case_n, w_variance, min_w, max_w);
                if (w_variance > 1.0f)
                    ImGui::PopStyleColor();
            }
            ImGui::PopID();
        }

        ImGui::End();
    };

    // ## Test behavior of some Table functions without BeginTable
    t = REGISTER_TEST("table", "table_4_functions_without_table");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        IM_CHECK_EQ(ImGui::TableGetColumnIndex(), 0);
        IM_CHECK_EQ(ImGui::TableSetColumnIndex(42), false);
        IM_CHECK_EQ(ImGui::TableGetColumnIsVisible(0), false);
        IM_CHECK_EQ(ImGui::TableGetColumnIsSorted(0), false);
        ImGui::End();
    };

    // ## Resizing test-bed (not an actual automated test)
    t = REGISTER_TEST("table", "table_5_resizing_behaviors");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowPos(ctx->GetMainViewportPos() + ImVec2(20, 5), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(400.0f, 0.0f), ImGuiCond_Once);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGui::BulletText("OK: Resize from F1| or F2|");    // ok: alter ->WidthRequested of Fixed column. Subsequent columns will be offset.
        ImGui::BulletText("OK: Resize from F3|");           // ok: alter ->WidthRequested of Fixed column. If active, ScrollX extent can be altered.
        HelperTableWithResizingPolicies("table1", 0, "FFF");
        ImGui::Spacing();

        ImGui::BulletText("OK: Resize from F1| or F2|");    // ok: alter ->WidthRequested of Fixed column. If active, ScrollX extent can be altered, but it doesn't make much sense as the Weighted column will always be minimal size.
        ImGui::BulletText("OK: Resize from W3| (off)");     // ok: no-op (disabled by Rule A)
        HelperTableWithResizingPolicies("table2", 0, "FFW");
        ImGui::Spacing();

        ImGui::BulletText("KO: Resize from W1| or W2|");    // FIXME: not implemented
        ImGui::BulletText("OK: Resize from W3| (off)");     // ok: no-op (disabled by Rule A)
        HelperTableWithResizingPolicies("table3", 0, "WWWw");
        ImGui::Spacing();

        // Need F2w + F3w to be stable to avoid moving W1
        // lock F2L
        // move F2R
        // move F3L
        // lock F3R
        ImGui::BulletText("OK: Resize from W1| (fwd)");     // ok: forward to resizing |F2. F3 will not move.
        ImGui::BulletText("KO: Resize from F2| or F3|");    // FIXME should resize F2, F3 and not have effect on W1 (Weighted columns are _before_ the Fixed column).
        ImGui::BulletText("OK: Resize from F4| (off)");     // ok: no-op (disabled by Rule A)
        HelperTableWithResizingPolicies("table4", 0, "WFFF");
        ImGui::Spacing();

        ImGui::BulletText("OK: Resize from W1| (fwd)");     // ok: forward to resizing |F2
        ImGui::BulletText("OK: Resize from F2| (off)");     // ok: no-op (disabled by Rule A)
        HelperTableWithResizingPolicies("table5", 0, "WF");
        ImGui::Spacing();

        ImGui::BulletText("KO: Resize from W1|");           // FIXME
        ImGui::BulletText("KO: Resize from W2|");           // FIXME
        HelperTableWithResizingPolicies("table6", 0, "WWF");
        ImGui::Spacing();

        ImGui::BulletText("KO: Resize from W1|");           // FIXME
        ImGui::BulletText("KO: Resize from F2|");           // FIXME
        HelperTableWithResizingPolicies("table7", 0, "WFW");
        ImGui::Spacing();

        ImGui::BulletText("OK: Resize from W2| (fwd)");     // ok: forward
        HelperTableWithResizingPolicies("table8", 0, "FWF");
        ImGui::Spacing();

        ImGui::BulletText("OK: Resize from ");
        HelperTableWithResizingPolicies("table9", 0, "WWFWW");
        ImGui::Spacing();

        ImGui::End();
    };

    // ## Test Visible flag
    t = REGISTER_TEST("table", "table_6_clip");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_ScrollX | ImGuiTableFlags_Borders, ImVec2(200, 200)))
        {
            ImGui::TableSetupColumn("One", 0, 80);
            ImGui::TableSetupColumn("Two", 0, 80);
            ImGui::TableSetupColumn("Three", 0, 80);
            ImGui::TableSetupColumn("Four", 0, 80);
            for (int row = 0; row < 2; row++)
            {
                ImGui::TableNextRow();
                bool visible_0 = ImGui::TableSetColumnIndex(0);
                ImGui::Text(visible_0 ? "1" : "0");
                bool visible_1 = ImGui::TableSetColumnIndex(1);
                ImGui::Text(visible_1 ? "1" : "0");
                bool visible_2 = ImGui::TableSetColumnIndex(2);
                ImGui::Text(visible_2 ? "1" : "0");
                bool visible_3 = ImGui::TableSetColumnIndex(3);
                ImGui::Text(visible_3 ? "1" : "0");
                if (ctx->FrameCount > 1)
                {
                    IM_CHECK_EQ(visible_0, true);
                    IM_CHECK_EQ(visible_1, true);
                    IM_CHECK_EQ(visible_2, true); // Half Visible
                    IM_CHECK_EQ(visible_3, false);
                    ctx->Finish();
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();
    };

    // ## Test that BeginTable/EndTable with no contents doesn't fail
    t = REGISTER_TEST("table", "table_7_empty");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::BeginTable("Table", 3);
        ImGui::EndTable();
        ImGui::End();
    };

    // ## Test default sort order assignment
    t = REGISTER_TEST("table", "table_8_default_sort_order");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::BeginTable("Table", 4, ImGuiTableFlags_MultiSortable);
        ImGui::TableSetupColumn("0", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("1", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("1", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortAscending);
        ImGui::TableSetupColumn("2", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending);
        const ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
        ImGui::TableAutoHeaders();
        IM_CHECK_EQ(sort_specs->SpecsCount, 3);
        IM_CHECK_EQ(sort_specs->Specs[0].ColumnIndex, 1);
        IM_CHECK_EQ(sort_specs->Specs[1].ColumnIndex, 2);
        IM_CHECK_EQ(sort_specs->Specs[2].ColumnIndex, 3);
        IM_CHECK_EQ(sort_specs->Specs[0].SortDirection, ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(sort_specs->Specs[1].SortDirection, ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(sort_specs->Specs[2].SortDirection, ImGuiSortDirection_Descending);
        //ImGuiTable* table = ctx->UiContext->CurrentTable;
        //IM_CHECK_EQ(table->SortSpecsCount, 3);
        ImGui::EndTable();
        ImGui::End();
    };

    // ## Test using the maximum of 64 columns (#3058)
    t = REGISTER_TEST("table", "table_9_max_columns");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        //ImDrawList* cmd = ImGui::GetWindowDrawList();
        ImGui::BeginTable("Table", 64);
        for (int n = 0; n < 64; n++)
            ImGui::TableSetupColumn("Header");
        ImGui::TableAutoHeaders();
        for (int i = 0; i < 10; i++)
        {
            ImGui::TableNextRow();
            for (int n = 0; n < 64; n++)
            {
                ImGui::Text("Data");
                ImGui::TableNextCell();
            }
        }
        ImGui::EndTable();
        ImGui::End();
    };

#endif // #ifdef IMGUI_HAS_TABLE
};

//-------------------------------------------------------------------------
// Tests: Docking
//-------------------------------------------------------------------------

void RegisterTests_Docking(ImGuiTestEngine* e)
{
#ifdef IMGUI_HAS_DOCK
    ImGuiTest* t = NULL;

    // FIXME-TESTS
    // - Dock window A+B into Dockspace
    // - Drag/extract dock node (will create new node)
    // - Check that new dock pos/size are right

    t = REGISTER_TEST("docking", "docking_basic_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

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
        ImGui::Text("This is CCCC (longer)");
        ImGui::End();

        if (ctx->FrameCount == 1)
        {
            IM_CHECK(ctx->DockIdIsUndockedOrStandalone(ids[0]));
            IM_CHECK(ctx->DockIdIsUndockedOrStandalone(ids[1]));
            IM_CHECK(ctx->DockIdIsUndockedOrStandalone(ids[2]));
        }
        if (ctx->FrameCount == 11)
        {
            IM_CHECK(vars.DockId != (ImGuiID)0);
            IM_CHECK(ids[0] == vars.DockId);
            IM_CHECK(ids[0] == ids[1] && ids[0] == ids[2]);
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->DockMultiClear("AAAA", "BBBB", "CCCC", NULL);
        ctx->YieldUntil(10);
        vars.DockId = ctx->DockMultiSetupBasic(0, "AAAA", "BBBB", "CCCC", NULL);
        ctx->YieldUntil(20);
    };

    // ## Test SetNextWindowDockID() api
    t = REGISTER_TEST("docking", "docking_basic_set_next_api");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        if (vars.Id == 0)
            vars.Id = ImGui::DockContextGenNodeID(ctx->UiContext);

        ImGui::SetNextWindowDockID(vars.Id, ImGuiCond_Always);
        ImGui::Begin("AAAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        IM_CHECK_EQ(vars.Id, ImGui::GetWindowDockID());
        ImGui::Text("This is AAAA");
        ImGui::End();

        ImGui::SetNextWindowDockID(vars.Id, ImGuiCond_Always);
        ImGui::Begin("BBBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        IM_CHECK_EQ(vars.Id, ImGui::GetWindowDockID());
        ImGui::Text("This is BBBB");
        ImGui::End();

        if (ctx->FrameCount == 3)
            ctx->Finish();
    };

    // ## Test that initial size of new dock node is based on visible/focused window
    // FIXME-DOCK FIXME-WIP: This is not actually reliable yet (see stashes around August 2019)
#if 0
    t = REGISTER_TEST("docking", "docking_basic_initial_node_size");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        if (vars.Id == 0)
            vars.Id = ImGui::DockContextGenNodeID(ctx->UiContext);

        ImGuiWindow* window_a = ImGui::FindWindowByName("AAAA");
        ImGuiWindow* window_b = ImGui::FindWindowByName("BBBB");

        ImGui::SetNextWindowDockID(vars.Id, ImGuiCond_Appearing);
        ImGui::Begin("AAAA", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        IM_CHECK_EQ(vars.Id, ImGui::GetWindowDockID());
        ImGui::Text("This is AAAA");
        ImGui::End();

        ImGui::SetNextWindowDockID(vars.Id, ImGuiCond_Appearing);
        ImGui::Begin("BBBB", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        IM_CHECK_EQ(vars.Id, ImGui::GetWindowDockID());
        ImGui::Text("This is BBBB... longer!");
        ImGui::End();

        if (ctx->FrameCount == 3)
        {
            // If the node is (200,200) here: this is the size setup by DockMultiSetupBasic() in a previous test,
            // and we have a side-effect/leak from a previous host window.
            ImGuiDockNode* node = window_b->DockNode;
            IM_CHECK(window_a->DockNode == window_b->DockNode);
            IM_CHECK(node->VisibleWindow == window_b);              // BBBB is visible
            IM_CHECK_GE(node->Size.x, window_b->ContentSize.x);     // Therefore we expect node to be fitting BBBB
            IM_CHECK_GE(node->Size.y, window_b->ContentSize.y);
            ctx->Finish();
        }
    };
#endif

    // ## Test that docking into a parent node forwarding docking into the central node or last focused node
    t = REGISTER_TEST("docking", "docking_into_parent_node");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ImGuiID dockspace_id = ImHashStr("Dockspace");

        if (ctx->IsFirstFrame())
        {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderDockWindow("AAAA", 0);
            ImGui::DockBuilderDockWindow("BBBB", 0);
        }
        if (ctx->FrameCount == 2)
        {
            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.5f, &vars.IdArray[0], &vars.IdArray[1]);
            IM_CHECK(vars.IdArray[0] != 0 && vars.IdArray[1]);
            ImGui::DockBuilderDockWindow("BBBB", dockspace_id);
            ImGuiWindow* window = ImGui::FindWindowByName("BBBB");
            IM_CHECK(window->DockId != dockspace_id);
            IM_CHECK(window->DockId == vars.IdArray[1]);
        }

        ImGui::Begin("AAAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::DockSpace(dockspace_id);
        ImGui::End();

        ImGui::Begin("BBBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();

        if (ctx->FrameCount == 10)
            ctx->Finish();
    };

    // ## Test that ConfigDockingTabBarOnSingleWindows transitions doesn't break window size.
    t = REGISTER_TEST("docking", "docking_auto_nodes_size");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        const ImVec2 expected_sz = ImVec2(200, 200);
        if (ctx->FrameCount == 0)
        {
            ImGui::SetNextWindowDockID(0);
            ImGui::SetNextWindowSize(expected_sz);
        }
        ImGui::Begin("AAAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("This is AAAA");
        const ImVec2 actual_sz = ImGui::GetWindowSize();
        if (ctx->FrameCount >= 0)
            IM_CHECK_EQ(actual_sz, expected_sz);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        bool backup_cfg = ctx->UiContext->IO.ConfigDockingAlwaysTabBar;
        ctx->LogDebug("ConfigDockingAlwaysTabBar = false");
        ctx->UiContext->IO.ConfigDockingAlwaysTabBar = false;
        ctx->YieldFrames(4);
        ctx->LogDebug("ConfigDockingAlwaysTabBar = true");
        ctx->UiContext->IO.ConfigDockingAlwaysTabBar = true;
        ctx->YieldFrames(4);
        ctx->LogDebug("ConfigDockingAlwaysTabBar = false");
        ctx->UiContext->IO.ConfigDockingAlwaysTabBar = false;
        ctx->YieldFrames(4);
        ctx->UiContext->IO.ConfigDockingAlwaysTabBar = backup_cfg;
    };

    // ## Test that undocking a whole _node_ doesn't lose/reset size
    t = REGISTER_TEST("docking", "docking_undock_from_dockspace_size");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiID dockspace_id = ctx->GetID("/Test Window/Dockspace");
        if (ctx->IsFirstFrame())
        {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderDockWindow("AAAA", dockspace_id);
            ImGui::DockBuilderDockWindow("BBBB", dockspace_id);
        }

        ImGui::SetNextWindowSize(ImVec2(500, 500));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::DockSpace(dockspace_id);
        ImGui::End();

        ImGui::Begin("AAAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("This is AAAA");
        ImGui::End();

        ImGui::Begin("BBBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("This is BBBB");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiID dockspace_id = ctx->GetID("/Test Window/Dockspace");

        ImGuiWindow* window_a = ImGui::FindWindowByName("AAAA");
        ImGuiWindow* window_b = ImGui::FindWindowByName("BBBB");
        ImVec2 window_a_size_old = window_a->Size;
        ImVec2 window_b_size_old = window_b->Size;

        ctx->UndockNode(dockspace_id);

        IM_CHECK(window_a->DockNode != NULL);
        IM_CHECK(window_b->DockNode != NULL);
        IM_CHECK(window_a->DockNode == window_b->DockNode);
        ImVec2 window_a_size_new = window_a->Size;
        ImVec2 window_b_size_new = window_b->Size;
        IM_CHECK_EQ(window_a->DockNode->Size, window_b_size_new);
        IM_CHECK_EQ(window_a_size_old, window_a_size_new);
        IM_CHECK_EQ(window_b_size_old, window_b_size_new);
    };

    // ## Test merging windows by dragging them.
    t = REGISTER_TEST("docking", "docking_drag_merge");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("AAAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("This is AAAA");
        ImGui::End();

        ImGui::Begin("BBBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("This is BBBB");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiIO& io = ImGui::GetIO();
        const bool backup_cfg_docking_always_tab_bar = io.ConfigDockingAlwaysTabBar; // FIXME-TESTS: Abstract that as a helper (e.g test case iterator)
        for (int test_case_n = 0; test_case_n < 2; test_case_n++)
        {
            // FIXME-TESTS: Tests doesn't work if already docked
            // FIXME-TESTS: DockSetMulti takes window_name not ref
            io.ConfigDockingAlwaysTabBar = (test_case_n == 1);
            ctx->LogDebug("## TEST CASE %d: with ConfigDockingAlwaysTabBar = %d", test_case_n, io.ConfigDockingAlwaysTabBar);

            ImGuiWindow* window_aaaa = ctx->GetWindowByRef("AAAA");
            ImGuiWindow* window_bbbb = ctx->GetWindowByRef("BBBB");

            // Init state
            ctx->DockMultiClear("AAAA", "BBBB", NULL);
            if (ctx->UiContext->IO.ConfigDockingAlwaysTabBar)
                IM_CHECK(window_aaaa->DockId != 0 && window_bbbb->DockId != 0 && window_aaaa->DockId != window_bbbb->DockId);
            else
                IM_CHECK(window_aaaa->DockId == 0 && window_bbbb->DockId == 0);
            ctx->WindowResize("/AAAA", ImVec2(200, 200));
            ctx->WindowMove("/AAAA", ImVec2(100, 100));
            ctx->WindowResize("/BBBB", ImVec2(200, 200));
            ctx->WindowMove("/BBBB", ImVec2(200, 200));

            // Dock Once
            ctx->DockWindowInto("AAAA", "BBBB");
            IM_CHECK(window_aaaa->DockNode != NULL);
            IM_CHECK(window_aaaa->DockNode == window_bbbb->DockNode);
            IM_CHECK_EQ(window_aaaa->DockNode->Pos, ImVec2(200, 200));
            IM_CHECK_EQ(window_aaaa->Pos, ImVec2(200, 200));
            IM_CHECK_EQ(window_bbbb->Pos, ImVec2(200, 200));
            ImGuiID dock_id = window_bbbb->DockId;
            ctx->Sleep(0.5f);

            {
                // Undock AAAA, BBBB should still refer/dock to node.
                ctx->DockMultiClear("AAAA", NULL);
                IM_CHECK(ctx->DockIdIsUndockedOrStandalone(window_aaaa->DockId));
                IM_CHECK(window_bbbb->DockId == dock_id);

                // Intentionally move both floating windows away
                ctx->WindowMove("/AAAA", ImVec2(100, 100));
                ctx->WindowResize("/AAAA", ImVec2(100, 100));
                ctx->WindowMove("/BBBB", ImVec2(300, 300));
                ctx->WindowResize("/BBBB", ImVec2(200, 200)); // Should already the case

                // Dock again (BBBB still refers to dock id, making this different from the first docking)
                ctx->DockWindowInto("/AAAA", "/BBBB", ImGuiDir_None);
                IM_CHECK_EQ(window_aaaa->DockId, dock_id);
                IM_CHECK_EQ(window_bbbb->DockId, dock_id);
                IM_CHECK_EQ(window_aaaa->Pos, ImVec2(300, 300));
                IM_CHECK_EQ(window_bbbb->Pos, ImVec2(300, 300));
                IM_CHECK_EQ(window_aaaa->Size, ImVec2(200, 200));
                IM_CHECK_EQ(window_bbbb->Size, ImVec2(200, 200));
                IM_CHECK_EQ(window_aaaa->DockNode->Pos, ImVec2(300, 300));
                IM_CHECK_EQ(window_aaaa->DockNode->Size, ImVec2(200, 200));
            }

            {
                // Undock AAAA, BBBB should still refer/dock to node.
                ctx->DockMultiClear("AAAA", NULL);
                IM_CHECK(ctx->DockIdIsUndockedOrStandalone(window_aaaa->DockId));
                IM_CHECK_EQ(window_bbbb->DockId, dock_id);

                // Intentionally move both floating windows away
                ctx->WindowMove("/AAAA", ImVec2(100, 100));
                ctx->WindowMove("/BBBB", ImVec2(200, 200));

                // Dock on the side (BBBB still refers to dock id, making this different from the first docking)
                ctx->DockWindowInto("/AAAA", "/BBBB", ImGuiDir_Left);
                IM_CHECK_EQ(window_aaaa->DockNode->ParentNode->ID, dock_id);
                IM_CHECK_EQ(window_bbbb->DockNode->ParentNode->ID, dock_id);
                IM_CHECK_EQ(window_aaaa->DockNode->ParentNode->Pos, ImVec2(200, 200));
                IM_CHECK_EQ(window_aaaa->Pos, ImVec2(200, 200));
                IM_CHECK_GT(window_bbbb->Pos.x, window_aaaa->Pos.x);
                IM_CHECK_EQ(window_bbbb->Pos.y, 200);
            }
        }
        io.ConfigDockingAlwaysTabBar = backup_cfg_docking_always_tab_bar;
    };

    // ## Test setting focus on a docked window, and setting focus on a specific item inside. (#2453)
    // ## In particular, while the selected tab is locked down early (causing a frame delay in the tab selection),
    // ## the window that has requested focus should allow items to be submitted (SkipItems==false) during its hidden frame,
    // ## mimicking the behavior of any newly appearing window.
    t = REGISTER_TEST("docking", "docking_focus_1");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        if (ctx->FrameCount == 0)
            vars.DockId = ctx->DockMultiSetupBasic(0, "AAAA", "BBBB", "CCCC", NULL);

        if (ctx->FrameCount == 10)  ImGui::SetNextWindowFocus();
        ImGui::Begin("AAAA");
        if (ctx->FrameCount == 10)  IM_CHECK(ImGui::IsWindowFocused());
        if (ctx->FrameCount == 10)  ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Input", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        if (ctx->FrameCount == 10)  IM_CHECK(!ImGui::IsItemActive());
        if (ctx->FrameCount == 11)  IM_CHECK(ImGui::IsItemActive());
        ImGui::End();

        if (ctx->FrameCount == 50)  ImGui::SetNextWindowFocus();
        ImGui::Begin("BBBB");
        if (ctx->FrameCount == 50)  IM_CHECK(ImGui::IsWindowFocused());
        if (ctx->FrameCount == 50)  ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Input", vars.Str2, IM_ARRAYSIZE(vars.Str2));
        if (ctx->FrameCount == 50)  IM_CHECK(!ImGui::IsItemActive());
        if (ctx->FrameCount == 51)  IM_CHECK(ImGui::IsItemActive());
        ImGui::End();

        if (ctx->FrameCount == 60)
            ctx->Finish();
    };
#endif
}

//-------------------------------------------------------------------------
// Tests: Misc
//-------------------------------------------------------------------------

void RegisterTests_Misc(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test watchdog
#if 0
    t = REGISTER_TEST("misc", "misc_watchdog");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        while (true)
            ctx->Yield();
    };
#endif

    // ## Test window data garbage collection
    t = REGISTER_TEST("misc", "misc_gc");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // Pretend window is no longer active once we start testing.
        if (ctx->FrameCount < 2)
        {
            for (int i = 0; i < 5; i++)
            {
                Str16f name("GC Test %d", i);
                ImGui::Begin(name.c_str());
                ImGui::TextUnformatted(name.c_str());
                ImGui::End();
            }
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->LogDebug("Check normal state");
        for (int i = 0; i < 3; i++)
        {
            ImGuiWindow* window = ctx->GetWindowByRef(Str16f("GC Test %d", i).c_str());
            IM_CHECK(!window->MemoryCompacted);
            IM_CHECK(!window->DrawList->CmdBuffer.empty());
        }

        float backup_timer = 0.0f;
        ImSwap(ctx->UiContext->IO.ConfigWindowsMemoryCompactTimer, backup_timer);

        ctx->YieldFrames(3); // Give time to perform GC
        ctx->LogDebug("Check GC-ed state");
        for (int i = 0; i < 3; i++)
        {
            ImGuiWindow* window = ctx->GetWindowByRef(Str16f("GC Test %d", i).c_str());
            IM_CHECK(window->MemoryCompacted);
            IM_CHECK(window->IDStack.empty());
            IM_CHECK(window->DrawList->CmdBuffer.empty());
        }
        ImSwap(ctx->UiContext->IO.ConfigWindowsMemoryCompactTimer, backup_timer);
    };

    // ## Test hash functions and ##/### operators
    t = REGISTER_TEST("misc", "misc_hash_001");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Test hash function for the property we need
        IM_CHECK_EQ(ImHashStr("helloworld"), ImHashStr("world", 0, ImHashStr("hello", 0)));  // String concatenation
        IM_CHECK_EQ(ImHashStr("hello###world"), ImHashStr("###world"));                      // ### operator reset back to the seed
        IM_CHECK_EQ(ImHashStr("hello###world", 0, 1234), ImHashStr("###world", 0, 1234));    // ### operator reset back to the seed
        IM_CHECK_EQ(ImHashStr("helloxxx", 5), ImHashStr("hello"));                           // String size is honored
        IM_CHECK_EQ(ImHashStr("", 0, 0), (ImU32)0);                                          // Empty string doesn't alter hash
        IM_CHECK_EQ(ImHashStr("", 0, 1234), (ImU32)1234);                                    // Empty string doesn't alter hash
        IM_CHECK_EQ(ImHashStr("hello", 5), ImHashData("hello", 5));                          // FIXME: Do we need to guarantee this?

        const int data[2] = { 42, 50 };
        IM_CHECK_EQ(ImHashData(&data[0], sizeof(int) * 2), ImHashData(&data[1], sizeof(int), ImHashData(&data[0], sizeof(int))));
        IM_CHECK_EQ(ImHashData("", 0, 1234), (ImU32)1234);                                   // Empty data doesn't alter hash

        // Verify that Test Engine high-level hash wrapper works
        IM_CHECK_EQ(ImHashDecoratedPath("Hello/world"), ImHashStr("Helloworld"));            // Slashes are ignored
        IM_CHECK_EQ(ImHashDecoratedPath("Hello\\/world"), ImHashStr("Hello/world"));         // Slashes can be inhibited
        IM_CHECK_EQ(ImHashDecoratedPath("/Hello", 42), ImHashDecoratedPath("Hello"));        // Leading / clears seed
    };

    // ## Test ImVector functions
    t = REGISTER_TEST("misc", "misc_vector_001");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImVector<int> v;
        IM_CHECK(v.Data == NULL);
        v.push_back(0);
        v.push_back(1);
        IM_CHECK(v.Data != NULL && v.Size == 2);
        v.push_back(2);
        bool r = v.find_erase(1);
        IM_CHECK(r == true);
        IM_CHECK(v.Data != NULL && v.Size == 2);
        r = v.find_erase(1);
        IM_CHECK(r == false);
        IM_CHECK(v.contains(0));
        IM_CHECK(v.contains(2));
        v.resize(0);
        IM_CHECK(v.Data != NULL && v.Capacity >= 3);
        v.clear();
        IM_CHECK(v.Data == NULL && v.Capacity == 0);
    };

    // ## Test ImVector functions
#ifdef IMGUI_HAS_TABLE
    t = REGISTER_TEST("misc", "misc_bitarray");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImBitArray<128> v128;
        IM_CHECK_EQ((int)sizeof(v128), 16);
        v128.ClearBits();
        v128.SetBitRange(1, 1);
        IM_CHECK(v128.Storage[0] == 0x00000002 && v128.Storage[1] == 0x00000000 && v128.Storage[2] == 0x00000000);
        v128.ClearBits();
        v128.SetBitRange(1, 31);
        IM_CHECK(v128.Storage[0] == 0xFFFFFFFE && v128.Storage[1] == 0x00000000 && v128.Storage[2] == 0x00000000);
        v128.ClearBits();
        v128.SetBitRange(1, 32);
        IM_CHECK(v128.Storage[0] == 0xFFFFFFFE && v128.Storage[1] == 0x00000001 && v128.Storage[2] == 0x00000000);
        v128.ClearBits();
        v128.SetBitRange(0, 64);
        IM_CHECK(v128.Storage[0] == 0xFFFFFFFF && v128.Storage[1] == 0xFFFFFFFF && v128.Storage[2] == 0x00000001);

        ImBitArray<129> v129;
        IM_CHECK_EQ((int)sizeof(v129), 20);
        v129.SetBit(128);
        IM_CHECK(v129.TestBit(128) == true);
    };
#endif

    // ## Test ImPool functions
    t = REGISTER_TEST("misc", "misc_pool_001");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImPool<ImGuiTabBar> pool;
        pool.GetOrAddByKey(0x11);
        pool.GetOrAddByKey(0x22); // May invalidate first point
        ImGuiTabBar* t1 = pool.GetByKey(0x11);
        ImGuiTabBar* t2 = pool.GetByKey(0x22);
        IM_CHECK(t1 != NULL && t2 != NULL);
        IM_CHECK(t1 + 1 == t2);
        IM_CHECK(pool.GetIndex(t1) == 0);
        IM_CHECK(pool.GetIndex(t2) == 1);
        IM_CHECK(pool.Contains(t1) && pool.Contains(t2));
        IM_CHECK(pool.Contains(t2 + 1) == false);
        IM_CHECK(pool.GetByIndex(pool.GetIndex(t1)) == t1);
        IM_CHECK(pool.GetByIndex(pool.GetIndex(t2)) == t2);
        ImGuiTabBar* t3 = pool.GetOrAddByKey(0x33);
        IM_CHECK(pool.GetIndex(t3) == 2);
        IM_CHECK(pool.GetSize() == 3);
        pool.Remove(0x22, pool.GetByKey(0x22));
        IM_CHECK(pool.GetByKey(0x22) == NULL);
        IM_CHECK(pool.GetSize() == 3);
        ImGuiTabBar* t4 = pool.GetOrAddByKey(0x40);
        IM_CHECK(pool.GetIndex(t4) == 1);
        IM_CHECK(pool.GetSize() == 3);
        pool.Clear();
        IM_CHECK(pool.GetSize() == 0);
    };

    // ## Test behavior of ImParseFormatTrimDecorations
    t = REGISTER_TEST("misc", "misc_format_parse");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // fmt = "blah blah"  -> return fmt
        // fmt = "%.3f"       -> return fmt
        // fmt = "hello %.3f" -> return fmt + 6
        // fmt = "%.3f hello" -> return buf, "%.3f"
        //const char* ImGui::ParseFormatTrimDecorations(const char* fmt, char* buf, int buf_size)
        const char* fmt = NULL;
        const char* out = NULL;
        char buf[32] = { 0 };
        size_t buf_size = sizeof(buf);

        fmt = "blah blah";
        out = ImParseFormatTrimDecorations(fmt, buf, buf_size);
        IM_CHECK(out == fmt);

        fmt = "%.3f";
        out = ImParseFormatTrimDecorations(fmt, buf, buf_size);
        IM_CHECK(out == fmt);

        fmt = "hello %.3f";
        out = ImParseFormatTrimDecorations(fmt, buf, buf_size);
        IM_CHECK(out == fmt + 6);
        IM_CHECK(strcmp(out, "%.3f") == 0);

        fmt = "%%hi%.3f";
        out = ImParseFormatTrimDecorations(fmt, buf, buf_size);
        IM_CHECK(out == fmt + 4);
        IM_CHECK(strcmp(out, "%.3f") == 0);

        fmt = "hello %.3f ms";
        out = ImParseFormatTrimDecorations(fmt, buf, buf_size);
        IM_CHECK(out == buf);
        IM_CHECK(strcmp(out, "%.3f") == 0);

        fmt = "hello %f blah";
        out = ImParseFormatTrimDecorations(fmt, buf, buf_size);
        IM_CHECK(out == buf);
        IM_CHECK(strcmp(out, "%f") == 0);
    };

    // ## Test ImFontAtlas building with overlapping glyph ranges (#2353, #2233)
    t = REGISTER_TEST("misc", "misc_atlas_build_glyph_overlap");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImFontAtlas atlas;
        ImFontConfig font_config;
        static const ImWchar default_ranges[] =
        {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x0080, 0x00FF, // Latin_Supplement
            0,
        };
        font_config.GlyphRanges = default_ranges;
        atlas.AddFontDefault(&font_config);
        atlas.Build();
    };

    t = REGISTER_TEST("misc", "misc_atlas_ranges_builder");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImFontGlyphRangesBuilder builder;
        builder.AddChar(31);
        builder.AddChar(0x10000-1);
        ImVector<ImWchar> out_ranges;
        builder.BuildRanges(&out_ranges);
        builder.Clear();
        IM_CHECK_EQ(out_ranges.Size, 5);
    };

    // ## Test whether splitting/merging draw lists properly retains a texture id.
    t = REGISTER_TEST("misc", "misc_drawlist_splitter_texture_id");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImTextureID prev_texture_id = draw_list->_TextureIdStack.back();
        const int draw_count = draw_list->CmdBuffer.Size;
        IM_CHECK(draw_list->CmdBuffer.back().ElemCount == 0);

        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(100+10+100, 100));

        draw_list->ChannelsSplit(2);
        draw_list->ChannelsSetCurrent(0);
        // Image wont be clipped when added directly into the draw list.
        draw_list->AddImage((ImTextureID)100, p, p + ImVec2(100, 100));
        draw_list->ChannelsSetCurrent(1);
        draw_list->AddImage((ImTextureID)200, p + ImVec2(110, 0), p + ImVec2(210, 100));
        draw_list->ChannelsMerge();

        IM_CHECK_NO_RET(draw_list->CmdBuffer.Size == draw_count + 2);
        IM_CHECK_NO_RET(draw_list->CmdBuffer.back().ElemCount == 0);
        IM_CHECK_NO_RET(prev_texture_id == draw_list->CmdBuffer.back().TextureId);

        // Replace fake texture IDs with a known good ID in order to prevent graphics API crashing application.
        for (ImDrawCmd& cmd : draw_list->CmdBuffer)
            if (cmd.TextureId == (ImTextureID)100 || cmd.TextureId == (ImTextureID)200)
                cmd.TextureId = prev_texture_id;

        ImGui::End();
    };

    t = REGISTER_TEST("misc", "misc_repeat_typematic");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->LogDebug("Regular repeat delay/rate");
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.00f, 0.00f, 1.0f, 0.2f), 1); // Trigger @ 0.0f, 1.0f, 1.2f, 1.4f, etc.
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.00f, 0.99f, 1.0f, 0.2f), 0); // "
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.99f, 1.00f, 1.0f, 0.2f), 1); // "
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.99f, 1.01f, 1.0f, 0.2f), 1); // "
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.99f, 1.41f, 1.0f, 0.2f), 3); // "
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(1.01f, 1.41f, 1.0f, 0.2f), 2); // "

        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.99f, 1.01f, 1.1f, 0.2f), 0); // Trigger @ 0.0f, 1.1f, 1.3f, 1.5f, etc.

        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.99f, 1.01f, 0.1f, 1.0f), 0); // Trigger @ 0.0f, 0.1f, 1.1f, 2.1f, etc.
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.99f, 1.11f, 0.1f, 1.0f), 1); // "

        ctx->LogDebug("No repeat delay");
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.00f, 0.00f, 0.0f, 0.2f), 1); // Trigger @ 0.0f, 0.2f, 0.4f, 0.6f, etc.
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.19f, 0.20f, 0.0f, 0.2f), 1); // Trigger @ 0.0f, 0.2f, 0.4f, 0.6f, etc.
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.20f, 0.20f, 0.0f, 0.2f), 0); // Trigger @ 0.0f, 0.2f, 0.4f, 0.6f, etc.
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.19f, 1.01f, 0.0f, 0.2f), 5); // Trigger @ 0.0f, 0.2f, 0.4f, 0.6f, etc.

        ctx->LogDebug("No repeat rate");
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.00f, 0.00f, 1.0f, 0.0f), 1); // Trigger @ 0.0f, 1.0f
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.99f, 1.01f, 1.0f, 0.0f), 1); // "
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(1.01f, 2.00f, 1.0f, 0.0f), 0); // "

        ctx->LogDebug("No repeat delay/rate");
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.00f, 0.00f, 0.0f, 0.0f), 1); // Trigger @ 0.0f
        IM_CHECK_EQ_NO_RET(ImGui::CalcTypematicRepeatAmount(0.01f, 1.01f, 0.0f, 0.0f), 0); // "
    };

    // ## Test ImGui::InputScalar() handling overflow for different data types
    t = REGISTER_TEST("misc", "misc_input_scalar_overflow");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        {
            ImS8 one = 1;
            ImS8 value = 2;
            ImGui::DataTypeApplyOp(ImGuiDataType_S8, '+', &value, &value, &one);
            IM_CHECK(value == 3);
            value = SCHAR_MAX;
            ImGui::DataTypeApplyOp(ImGuiDataType_S8, '+', &value, &value, &one);
            IM_CHECK(value == SCHAR_MAX);
            value = SCHAR_MIN;
            ImGui::DataTypeApplyOp(ImGuiDataType_S8, '-', &value, &value, &one);
            IM_CHECK(value == SCHAR_MIN);
        }
        {
            ImU8 one = 1;
            ImU8 value = 2;
            ImGui::DataTypeApplyOp(ImGuiDataType_U8, '+', &value, &value, &one);
            IM_CHECK(value == 3);
            value = UCHAR_MAX;
            ImGui::DataTypeApplyOp(ImGuiDataType_U8, '+', &value, &value, &one);
            IM_CHECK(value == UCHAR_MAX);
            value = 0;
            ImGui::DataTypeApplyOp(ImGuiDataType_U8, '-', &value, &value, &one);
            IM_CHECK(value == 0);
        }
        {
            ImS16 one = 1;
            ImS16 value = 2;
            ImGui::DataTypeApplyOp(ImGuiDataType_S16, '+', &value, &value, &one);
            IM_CHECK(value == 3);
            value = SHRT_MAX;
            ImGui::DataTypeApplyOp(ImGuiDataType_S16, '+', &value, &value, &one);
            IM_CHECK(value == SHRT_MAX);
            value = SHRT_MIN;
            ImGui::DataTypeApplyOp(ImGuiDataType_S16, '-', &value, &value, &one);
            IM_CHECK(value == SHRT_MIN);
        }
        {
            ImU16 one = 1;
            ImU16 value = 2;
            ImGui::DataTypeApplyOp(ImGuiDataType_U16, '+', &value, &value, &one);
            IM_CHECK(value == 3);
            value = USHRT_MAX;
            ImGui::DataTypeApplyOp(ImGuiDataType_U16, '+', &value, &value, &one);
            IM_CHECK(value == USHRT_MAX);
            value = 0;
            ImGui::DataTypeApplyOp(ImGuiDataType_U16, '-', &value, &value, &one);
            IM_CHECK(value == 0);
        }
        {
            ImS32 one = 1;
            ImS32 value = 2;
            ImGui::DataTypeApplyOp(ImGuiDataType_S32, '+', &value, &value, &one);
            IM_CHECK(value == 3);
            value = INT_MAX;
            ImGui::DataTypeApplyOp(ImGuiDataType_S32, '+', &value, &value, &one);
            IM_CHECK(value == INT_MAX);
            value = INT_MIN;
            ImGui::DataTypeApplyOp(ImGuiDataType_S32, '-', &value, &value, &one);
            IM_CHECK(value == INT_MIN);
        }
        {
            ImU32 one = 1;
            ImU32 value = 2;
            ImGui::DataTypeApplyOp(ImGuiDataType_U32, '+', &value, &value, &one);
            IM_CHECK(value == 3);
            value = UINT_MAX;
            ImGui::DataTypeApplyOp(ImGuiDataType_U32, '+', &value, &value, &one);
            IM_CHECK(value == UINT_MAX);
            value = 0;
            ImGui::DataTypeApplyOp(ImGuiDataType_U32, '-', &value, &value, &one);
            IM_CHECK(value == 0);
        }
        {
            ImS64 one = 1;
            ImS64 value = 2;
            ImGui::DataTypeApplyOp(ImGuiDataType_S64, '+', &value, &value, &one);
            IM_CHECK(value == 3);
            value = LLONG_MAX;
            ImGui::DataTypeApplyOp(ImGuiDataType_S64, '+', &value, &value, &one);
            IM_CHECK(value == LLONG_MAX);
            value = LLONG_MIN;
            ImGui::DataTypeApplyOp(ImGuiDataType_S64, '-', &value, &value, &one);
            IM_CHECK(value == LLONG_MIN);
        }
        {
            ImU64 one = 1;
            ImU64 value = 2;
            ImGui::DataTypeApplyOp(ImGuiDataType_U64, '+', &value, &value, &one);
            IM_CHECK(value == 3);
            value = ULLONG_MAX;
            ImGui::DataTypeApplyOp(ImGuiDataType_U64, '+', &value, &value, &one);
            IM_CHECK(value == ULLONG_MAX);
            value = 0;
            ImGui::DataTypeApplyOp(ImGuiDataType_U64, '-', &value, &value, &one);
            IM_CHECK(value == 0);
        }
    };

    // ## Test basic clipboard, test that clipboard is empty on start
    t = REGISTER_TEST("misc", "misc_clipboard");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // By specs, the testing system should provide an empty clipboard (we don't want user clipboard leaking into tests!)
        const char* clipboard_text = ImGui::GetClipboardText();
        IM_CHECK_STR_EQ(clipboard_text, "");

        // Regular clipboard test
        const char* message = "Clippy is alive.";
        ImGui::SetClipboardText(message);
        clipboard_text = ImGui::GetClipboardText();
        IM_CHECK_STR_EQ(message, clipboard_text);
    };

    // ## Test UTF-8 encoding and decoding.
    // Note that this is ONLY testing encoding/decoding, we are not attempting to display those characters not trying to be i18n compliant
    t = REGISTER_TEST("misc", "misc_utf8");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
#define IM_TEST_USING_ImWchar32 1
#define IM_IS_WCHAR32 IM_TOKENCONCAT(IM_TEST_USING_, ImWchar)

        auto check_utf8 = [](const char* utf8, const ImWchar* unicode)
        {
            const int utf8_len = (int)strlen(utf8);
            const int max_chars = utf8_len * 4 + 1;

            ImWchar* converted = (ImWchar*)IM_ALLOC(max_chars * sizeof(ImWchar));
            char* reconverted = (char*)IM_ALLOC(max_chars * sizeof(char));

            // Convert UTF-8 text to unicode codepoints and check against expected value.
            int result_bytes = ImTextStrFromUtf8(converted, max_chars, utf8, NULL);
            bool success = ImStrlenW((ImWchar*)unicode) == result_bytes && memcmp(converted, unicode, result_bytes * sizeof(ImWchar)) == 0;

            // Convert resulting unicode codepoints back to UTF-8 and check them against initial UTF-8 input value.
            if (success)
            {
                result_bytes = ImTextStrToUtf8(reconverted, max_chars, converted, NULL);
                success &= (utf8_len == result_bytes) && (memcmp(utf8, reconverted, result_bytes * sizeof(char)) == 0);
            }

            IM_FREE(converted);
            IM_FREE(reconverted);
            return success;
        };
        auto get_first_codepoint = [](const char* str)
        {
            unsigned code_point = 0;
            ImTextCharFromUtf8(&code_point, str, str + strlen(str));
            return code_point;
        };
#if IM_IS_WCHAR32
        #define IM_CHECK_UTF8(_TEXT)   (check_utf8(u8##_TEXT, (ImWchar*)U##_TEXT))
        // Test whether 32bit codepoints are correctly decoded.
        IM_CHECK_NO_RET(get_first_codepoint((const char*)u8"\U0001f60d") == 0x0001f60d);
#else
        #define IM_CHECK_UTF8(_TEXT)   (check_utf8(u8##_TEXT, (ImWchar*)u##_TEXT))
        // Test whether 32bit codepoints are correctly discarded.
        IM_CHECK_NO_RET(get_first_codepoint((const char*)u8"\U0001f60d") == IM_UNICODE_CODEPOINT_INVALID);
#endif

        // Test data taken from https://bitbucket.org/knight666/utf8rewind/src/default/testdata/big-list-of-naughty-strings-master/blns.txt

        // Special Characters
        // Strings which contain common special ASCII characters (may need to be escaped)
        IM_CHECK_NO_RET(IM_CHECK_UTF8(",./;'[]\\-="));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("<>?:\"{}|_+"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("!@#$%^&*()`~"));

        // Unicode Symbols
        // Strings which contain common unicode symbols (e.g. smart quotes)
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u03a9\u2248\u00e7\u221a\u222b\u02dc\u00b5\u2264\u2265\u00f7"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u00e5\u00df\u2202\u0192\u00a9\u02d9\u2206\u02da\u00ac\u2026\u00e6"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u0153\u2211\u00b4\u00ae\u2020\u00a5\u00a8\u02c6\u00f8\u03c0\u201c\u2018"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u00a1\u2122\u00a3\u00a2\u221e\u00a7\u00b6\u2022\u00aa\u00ba\u2013\u2260"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u00b8\u02db\u00c7\u25ca\u0131\u02dc\u00c2\u00af\u02d8\u00bf"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u00c5\u00cd\u00ce\u00cf\u02dd\u00d3\u00d4\uf8ff\u00d2\u00da\u00c6\u2603"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u0152\u201e\u00b4\u2030\u02c7\u00c1\u00a8\u02c6\u00d8\u220f\u201d\u2019"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("`\u2044\u20ac\u2039\u203a\ufb01\ufb02\u2021\u00b0\u00b7\u201a\u2014\u00b1"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u215b\u215c\u215d\u215e"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u0401\u0402\u0403\u0404\u0405\u0406\u0407\u0408\u0409\u040a\u040b\u040c\u040d\u040e\u040f\u0410\u0411\u0412\u0413\u0414\u0415\u0416\u0417\u0418\u0419\u041a\u041b\u041c\u041d\u041e\u041f\u0420\u0421\u0422\u0423\u0424\u0425\u0426\u0427\u0428\u0429\u042a\u042b\u042c\u042d\u042e\u042f\u0430\u0431\u0432\u0433\u0434\u0435\u0436\u0437\u0438\u0439\u043a\u043b\u043c\u043d\u043e\u043f\u0440\u0441\u0442\u0443\u0444\u0445\u0446\u0447\u0448\u0449\u044a\u044b\u044c\u044d\u044e\u044f"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u0660\u0661\u0662\u0663\u0664\u0665\u0666\u0667\u0668\u0669"));

        // Unicode Subscript/Superscript
        // Strings which contain unicode subscripts/superscripts; can cause rendering issues
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u2070\u2074\u2075"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u2080\u2081\u2082"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u2070\u2074\u2075\u2080\u2081\u2082"));

        // Quotation Marks
        // Strings which contain misplaced quotation marks; can cause encoding errors
        IM_CHECK_NO_RET(IM_CHECK_UTF8("'"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\""));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("''"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\"\""));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("'\"'"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\"''''\"'\""));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\"'\"'\"''''\""));

        // Two-Byte Characters
        // Strings which contain two-byte characters: can cause rendering issues or character-length issues
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u7530\u4e2d\u3055\u3093\u306b\u3042\u3052\u3066\u4e0b\u3055\u3044"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u30d1\u30fc\u30c6\u30a3\u30fc\u3078\u884c\u304b\u306a\u3044\u304b"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u548c\u88fd\u6f22\u8a9e"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u90e8\u843d\u683c"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\uc0ac\ud68c\uacfc\ud559\uc6d0 \uc5b4\ud559\uc5f0\uad6c\uc18c"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\ucc26\ucc28\ub97c \ud0c0\uace0 \uc628 \ud3b2\uc2dc\ub9e8\uacfc \uc45b\ub2e4\ub9ac \ub620\ubc29\uac01\ud558"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u793e\u6703\u79d1\u5b78\u9662\u8a9e\u5b78\u7814\u7a76\u6240"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\uc6b8\ub780\ubc14\ud1a0\ub974"));
        // IM_CHECK_NO_RET(IM_CHECK_UTF8_CP32("\U0002070e\U00020731\U00020779\U00020c53\U00020c78\U00020c96\U00020ccf"));

        // Japanese Emoticons
        // Strings which consists of Japanese-style emoticons which are popular on the web
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u30fd\u0f3c\u0e88\u0644\u035c\u0e88\u0f3d\uff89 \u30fd\u0f3c\u0e88\u0644\u035c\u0e88\u0f3d\uff89"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("(\uff61\u25d5 \u2200 \u25d5\uff61)"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\uff40\uff68(\u00b4\u2200\uff40\u2229"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("__\uff9b(,_,*)"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u30fb(\uffe3\u2200\uffe3)\u30fb:*:"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\uff9f\uff65\u273f\u30fe\u2572(\uff61\u25d5\u203f\u25d5\uff61)\u2571\u273f\uff65\uff9f"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8(",\u3002\u30fb:*:\u30fb\u309c\u2019( \u263b \u03c9 \u263b )\u3002\u30fb:*:\u30fb\u309c\u2019"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("(\u256f\u00b0\u25a1\u00b0\uff09\u256f\ufe35 \u253b\u2501\u253b)"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("(\uff89\u0ca5\u76ca\u0ca5\uff09\uff89\ufeff \u253b\u2501\u253b"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("( \u0361\u00b0 \u035c\u0296 \u0361\u00b0)"));

#if IM_IS_WCHAR32
        // Emoji
        // Strings which contain Emoji; should be the same behavior as two-byte characters, but not always
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\U0001f60d"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\U0001f469\U0001f3fd"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\U0001f47e \U0001f647 \U0001f481 \U0001f645 \U0001f646 \U0001f64b \U0001f64e \U0001f64d"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\U0001f435 \U0001f648 \U0001f649 \U0001f64a"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u2764\ufe0f \U0001f494 \U0001f48c \U0001f495 \U0001f49e \U0001f493 \U0001f497 \U0001f496 \U0001f498 \U0001f49d \U0001f49f \U0001f49c \U0001f49b \U0001f49a \U0001f499"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u270b\U0001f3ff \U0001f4aa\U0001f3ff \U0001f450\U0001f3ff \U0001f64c\U0001f3ff \U0001f44f\U0001f3ff \U0001f64f\U0001f3ff"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\U0001f6be \U0001f192 \U0001f193 \U0001f195 \U0001f196 \U0001f197 \U0001f199 \U0001f3e7"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("0\ufe0f\u20e3 1\ufe0f\u20e3 2\ufe0f\u20e3 3\ufe0f\u20e3 4\ufe0f\u20e3 5\ufe0f\u20e3 6\ufe0f\u20e3 7\ufe0f\u20e3 8\ufe0f\u20e3 9\ufe0f\u20e3 \U0001f51f"));
#endif
        // Unicode Numbers
        // Strings which contain unicode numbers; if the code is localized, it should see the input as numeric
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\uff11\uff12\uff13"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u0661\u0662\u0663"));

        // Right-To-Left Strings
        // Strings which contain text that should be rendered RTL if possible (e.g. Arabic, Hebrew)
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u062b\u0645 \u0646\u0641\u0633 \u0633\u0642\u0637\u062a \u0648\u0628\u0627\u0644\u062a\u062d\u062f\u064a\u062f\u060c, \u062c\u0632\u064a\u0631\u062a\u064a \u0628\u0627\u0633\u062a\u062e\u062f\u0627\u0645 \u0623\u0646 \u062f\u0646\u0648. \u0625\u0630 \u0647\u0646\u0627\u061f \u0627\u0644\u0633\u062a\u0627\u0631 \u0648\u062a\u0646\u0635\u064a\u0628 \u0643\u0627\u0646. \u0623\u0647\u0651\u0644 \u0627\u064a\u0637\u0627\u0644\u064a\u0627\u060c \u0628\u0631\u064a\u0637\u0627\u0646\u064a\u0627-\u0641\u0631\u0646\u0633\u0627 \u0642\u062f \u0623\u062e\u0630. \u0633\u0644\u064a\u0645\u0627\u0646\u060c \u0625\u062a\u0641\u0627\u0642\u064a\u0629 \u0628\u064a\u0646 \u0645\u0627, \u064a\u0630\u0643\u0631 \u0627\u0644\u062d\u062f\u0648\u062f \u0623\u064a \u0628\u0639\u062f, \u0645\u0639\u0627\u0645\u0644\u0629 \u0628\u0648\u0644\u0646\u062f\u0627\u060c \u0627\u0644\u0625\u0637\u0644\u0627\u0642 \u0639\u0644 \u0625\u064a\u0648."));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u05d1\u05b0\u05bc\u05e8\u05b5\u05d0\u05e9\u05b4\u05c1\u05d9\u05ea, \u05d1\u05b8\u05bc\u05e8\u05b8\u05d0 \u05d0\u05b1\u05dc\u05b9\u05d4\u05b4\u05d9\u05dd, \u05d0\u05b5\u05ea \u05d4\u05b7\u05e9\u05b8\u05bc\u05c1\u05de\u05b7\u05d9\u05b4\u05dd, \u05d5\u05b0\u05d0\u05b5\u05ea \u05d4\u05b8\u05d0\u05b8\u05e8\u05b6\u05e5"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u05d4\u05b8\u05d9\u05b0\u05ea\u05b8\u05d4test\u0627\u0644\u0635\u0641\u062d\u0627\u062a \u0627\u0644\u062a\u0651\u062d\u0648\u0644"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\ufdfd"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\ufdfa"));

        // Unicode Spaces
        // Strings which contain unicode space characters with special properties (c.f. https://www.cs.tut.fi/~jkorpela/chars/spaces.html)
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u200b"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u180e"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\ufeff"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u2423"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u2422"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u2421"));

        // Trick Unicode
        // Strings which contain unicode with unusual properties (e.g. Right-to-left override) (c.f. http://www.unicode.org/charts/PDF/U2000.pdf)
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u202a\u202atest\u202a"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u202btest\u202b"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("test"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("test\u2060test\u202b"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u2066test\u2067"));

        // Zalgo Text
        // Strings which contain "corrupted" text. The corruption will not appear in non-HTML text, however. (via http://www.eeemo.net)
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u1e70\u033a\u033a\u0315o\u035e \u0337i\u0332\u032c\u0347\u032a\u0359n\u031d\u0317\u0355v\u031f\u031c\u0318\u0326\u035fo\u0336\u0319\u0330\u0320k\u00e8\u035a\u032e\u033a\u032a\u0339\u0331\u0324 \u0316t\u031d\u0355\u0333\u0323\u033b\u032a\u035eh\u033c\u0353\u0332\u0326\u0333\u0318\u0332e\u0347\u0323\u0330\u0326\u032c\u034e \u0322\u033c\u033b\u0331\u0318h\u035a\u034e\u0359\u031c\u0323\u0332\u0345i\u0326\u0332\u0323\u0330\u0324v\u033b\u034de\u033a\u032d\u0333\u032a\u0330-m\u0322i\u0345n\u0316\u033a\u031e\u0332\u032f\u0330d\u0335\u033c\u031f\u0359\u0329\u033c\u0318\u0333 \u031e\u0325\u0331\u0333\u032dr\u031b\u0317\u0318e\u0359p\u0360r\u033c\u031e\u033b\u032d\u0317e\u033a\u0320\u0323\u035fs\u0318\u0347\u0333\u034d\u031d\u0349e\u0349\u0325\u032f\u031e\u0332\u035a\u032c\u035c\u01f9\u032c\u034e\u034e\u031f\u0316\u0347\u0324t\u034d\u032c\u0324\u0353\u033c\u032d\u0358\u0345i\u032a\u0331n\u0360g\u0334\u0349 \u034f\u0349\u0345c\u032c\u031fh\u0361a\u032b\u033b\u032f\u0358o\u032b\u031f\u0316\u034d\u0319\u031d\u0349s\u0317\u0326\u0332.\u0328\u0339\u0348\u0323"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u0321\u0353\u031e\u0345I\u0317\u0318\u0326\u035dn\u0347\u0347\u0359v\u032e\u032bok\u0332\u032b\u0319\u0348i\u0316\u0359\u032d\u0339\u0320\u031en\u0321\u033b\u032e\u0323\u033ag\u0332\u0348\u0359\u032d\u0359\u032c\u034e \u0330t\u0354\u0326h\u031e\u0332e\u0322\u0324 \u034d\u032c\u0332\u0356f\u0334\u0318\u0355\u0323\u00e8\u0356\u1eb9\u0325\u0329l\u0356\u0354\u035ai\u0353\u035a\u0326\u0360n\u0356\u034d\u0317\u0353\u0333\u032eg\u034d \u0328o\u035a\u032a\u0361f\u0318\u0323\u032c \u0316\u0318\u0356\u031f\u0359\u032ec\u0489\u0354\u032b\u0356\u0353\u0347\u0356\u0345h\u0335\u0324\u0323\u035a\u0354\u00e1\u0317\u033c\u0355\u0345o\u033c\u0323\u0325s\u0331\u0348\u033a\u0316\u0326\u033b\u0362.\u031b\u0316\u031e\u0320\u032b\u0330"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u0317\u033a\u0356\u0339\u032f\u0353\u1e6e\u0324\u034d\u0325\u0347\u0348h\u0332\u0301e\u034f\u0353\u033c\u0317\u0319\u033c\u0323\u0354 \u0347\u031c\u0331\u0320\u0353\u034d\u0345N\u0355\u0360e\u0317\u0331z\u0318\u031d\u031c\u033a\u0359p\u0324\u033a\u0339\u034d\u032f\u035ae\u0320\u033b\u0320\u035cr\u0328\u0324\u034d\u033a\u0316\u0354\u0316\u0316d\u0320\u031f\u032d\u032c\u031d\u035fi\u0326\u0356\u0329\u0353\u0354\u0324a\u0320\u0317\u032c\u0349\u0319n\u035a\u035c \u033b\u031e\u0330\u035a\u0345h\u0335\u0349i\u0333\u031ev\u0322\u0347\u1e19\u034e\u035f-\u0489\u032d\u0329\u033c\u0354m\u0324\u032d\u032bi\u0355\u0347\u031d\u0326n\u0317\u0359\u1e0d\u031f \u032f\u0332\u0355\u035e\u01eb\u031f\u032f\u0330\u0332\u0359\u033b\u031df \u032a\u0330\u0330\u0317\u0316\u032d\u0318\u0358c\u0326\u034d\u0332\u031e\u034d\u0329\u0319\u1e25\u035aa\u032e\u034e\u031f\u0319\u035c\u01a1\u0329\u0339\u034es\u0324.\u031d\u031d \u0489Z\u0321\u0316\u031c\u0356\u0330\u0323\u0349\u031ca\u0356\u0330\u0359\u032c\u0361l\u0332\u032b\u0333\u034d\u0329g\u0321\u031f\u033c\u0331\u035a\u031e\u032c\u0345o\u0317\u035c.\u031f"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u0326H\u032c\u0324\u0317\u0324\u035de\u035c \u031c\u0325\u031d\u033b\u034d\u031f\u0301w\u0315h\u0316\u032f\u0353o\u031d\u0359\u0316\u034e\u0331\u032e \u0489\u033a\u0319\u031e\u031f\u0348W\u0337\u033c\u032da\u033a\u032a\u034d\u012f\u0348\u0355\u032d\u0359\u032f\u031ct\u0336\u033c\u032es\u0318\u0359\u0356\u0315 \u0320\u032b\u0320B\u033b\u034d\u0359\u0349\u0333\u0345e\u0335h\u0335\u032c\u0347\u032b\u0359i\u0339\u0353\u0333\u0333\u032e\u034e\u032b\u0315n\u035fd\u0334\u032a\u031c\u0316 \u0330\u0349\u0329\u0347\u0359\u0332\u035e\u0345T\u0356\u033c\u0353\u032a\u0362h\u034f\u0353\u032e\u033be\u032c\u031d\u031f\u0345 \u0324\u0339\u031dW\u0359\u031e\u031d\u0354\u0347\u035d\u0345a\u034f\u0353\u0354\u0339\u033c\u0323l\u0334\u0354\u0330\u0324\u031f\u0354\u1e3d\u032b.\u0355"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("Z\u032e\u031e\u0320\u0359\u0354\u0345\u1e00\u0317\u031e\u0348\u033b\u0317\u1e36\u0359\u034e\u032f\u0339\u031e\u0353G\u033bO\u032d\u0317\u032e"));

        // Unicode Upsidedown
        // Strings which contain unicode with an "upsidedown" effect (via http://www.upsidedowntext.com)
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u02d9\u0250nb\u1d09l\u0250 \u0250u\u0183\u0250\u026f \u01dd\u0279olop \u0287\u01dd \u01dd\u0279oq\u0250l \u0287n \u0287unp\u1d09p\u1d09\u0254u\u1d09 \u0279od\u026f\u01dd\u0287 po\u026fsn\u1d09\u01dd op p\u01dds '\u0287\u1d09l\u01dd \u0183u\u1d09\u0254s\u1d09d\u1d09p\u0250 \u0279n\u0287\u01dd\u0287\u0254\u01ddsuo\u0254 '\u0287\u01dd\u026f\u0250 \u0287\u1d09s \u0279olop \u026fnsd\u1d09 \u026f\u01dd\u0279o\u02e5"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("00\u02d9\u0196$-"));

        // Unicode font
        // Strings which contain bold/italic/etc. versions of normal characters
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\uff34\uff48\uff45 \uff51\uff55\uff49\uff43\uff4b \uff42\uff52\uff4f\uff57\uff4e \uff46\uff4f\uff58 \uff4a\uff55\uff4d\uff50\uff53 \uff4f\uff56\uff45\uff52 \uff54\uff48\uff45 \uff4c\uff41\uff5a\uff59 \uff44\uff4f\uff47"));
#if IM_IS_WCHAR32
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\U0001d413\U0001d421\U0001d41e \U0001d42a\U0001d42e\U0001d422\U0001d41c\U0001d424 \U0001d41b\U0001d42b\U0001d428\U0001d430\U0001d427 \U0001d41f\U0001d428\U0001d431 \U0001d423\U0001d42e\U0001d426\U0001d429\U0001d42c \U0001d428\U0001d42f\U0001d41e\U0001d42b \U0001d42d\U0001d421\U0001d41e \U0001d425\U0001d41a\U0001d433\U0001d432 \U0001d41d\U0001d428\U0001d420"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\U0001d57f\U0001d58d\U0001d58a \U0001d596\U0001d59a\U0001d58e\U0001d588\U0001d590 \U0001d587\U0001d597\U0001d594\U0001d59c\U0001d593 \U0001d58b\U0001d594\U0001d59d \U0001d58f\U0001d59a\U0001d592\U0001d595\U0001d598 \U0001d594\U0001d59b\U0001d58a\U0001d597 \U0001d599\U0001d58d\U0001d58a \U0001d591\U0001d586\U0001d59f\U0001d59e \U0001d589\U0001d594\U0001d58c"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\U0001d47b\U0001d489\U0001d486 \U0001d492\U0001d496\U0001d48a\U0001d484\U0001d48c \U0001d483\U0001d493\U0001d490\U0001d498\U0001d48f \U0001d487\U0001d490\U0001d499 \U0001d48b\U0001d496\U0001d48e\U0001d491\U0001d494 \U0001d490\U0001d497\U0001d486\U0001d493 \U0001d495\U0001d489\U0001d486 \U0001d48d\U0001d482\U0001d49b\U0001d49a \U0001d485\U0001d490\U0001d488"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\U0001d4e3\U0001d4f1\U0001d4ee \U0001d4fa\U0001d4fe\U0001d4f2\U0001d4ec\U0001d4f4 \U0001d4eb\U0001d4fb\U0001d4f8\U0001d500\U0001d4f7 \U0001d4ef\U0001d4f8\U0001d501 \U0001d4f3\U0001d4fe\U0001d4f6\U0001d4f9\U0001d4fc \U0001d4f8\U0001d4ff\U0001d4ee\U0001d4fb \U0001d4fd\U0001d4f1\U0001d4ee \U0001d4f5\U0001d4ea\U0001d503\U0001d502 \U0001d4ed\U0001d4f8\U0001d4f0"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\U0001d54b\U0001d559\U0001d556 \U0001d562\U0001d566\U0001d55a\U0001d554\U0001d55c \U0001d553\U0001d563\U0001d560\U0001d568\U0001d55f \U0001d557\U0001d560\U0001d569 \U0001d55b\U0001d566\U0001d55e\U0001d561\U0001d564 \U0001d560\U0001d567\U0001d556\U0001d563 \U0001d565\U0001d559\U0001d556 \U0001d55d\U0001d552\U0001d56b\U0001d56a \U0001d555\U0001d560\U0001d558"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\U0001d683\U0001d691\U0001d68e \U0001d69a\U0001d69e\U0001d692\U0001d68c\U0001d694 \U0001d68b\U0001d69b\U0001d698\U0001d6a0\U0001d697 \U0001d68f\U0001d698\U0001d6a1 \U0001d693\U0001d69e\U0001d696\U0001d699\U0001d69c \U0001d698\U0001d69f\U0001d68e\U0001d69b \U0001d69d\U0001d691\U0001d68e \U0001d695\U0001d68a\U0001d6a3\U0001d6a2 \U0001d68d\U0001d698\U0001d690"));
#endif
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u24af\u24a3\u24a0 \u24ac\u24b0\u24a4\u249e\u24a6 \u249d\u24ad\u24aa\u24b2\u24a9 \u24a1\u24aa\u24b3 \u24a5\u24b0\u24a8\u24ab\u24ae \u24aa\u24b1\u24a0\u24ad \u24af\u24a3\u24a0 \u24a7\u249c\u24b5\u24b4 \u249f\u24aa\u24a2"));

        // iOS Vulnerability
        // Strings which crashed iMessage in iOS versions 8.3 and earlier
        IM_CHECK_NO_RET(IM_CHECK_UTF8("Power\u0644\u064f\u0644\u064f\u0635\u0651\u0628\u064f\u0644\u064f\u0644\u0635\u0651\u0628\u064f\u0631\u0631\u064b \u0963 \u0963h \u0963 \u0963\u5197"));

#undef IM_TEST_USING_ImWchar32
#undef IM_IS_WCHAR32
    };

    // ## Test ImGuiTextFilter
    t = REGISTER_TEST("misc", "misc_text_filter");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        static ImGuiTextFilter filter;
        ImGui::Begin("Text filter");
        filter.Draw("Filter", ImGui::GetFontSize() * 16);   // Test input filter drawing
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Test ImGuiTextFilter::Draw()
        ctx->WindowRef("Text filter");
        ctx->ItemInput("Filter");
        ctx->KeyCharsAppend("Big,Cat,, ,  ,Bird"); // Trigger filter rebuild

        // Test functionality
        ImGuiTextFilter filter;
        ImStrncpy(filter.InputBuf, "-bar", IM_ARRAYSIZE(filter.InputBuf));
        filter.Build();

        IM_CHECK(filter.PassFilter("bartender") == false);
        IM_CHECK(filter.PassFilter("cartender") == true);

        ImStrncpy(filter.InputBuf, "bar ", IM_ARRAYSIZE(filter.InputBuf));
        filter.Build();
        IM_CHECK(filter.PassFilter("bartender") == true);
        IM_CHECK(filter.PassFilter("cartender") == false);

        ImStrncpy(filter.InputBuf, "bar", IM_ARRAYSIZE(filter.InputBuf));
        filter.Build();
        IM_CHECK(filter.PassFilter("bartender") == true);
        IM_CHECK(filter.PassFilter("cartender") == false);
    };

        // ## Visual ImBezierClosestPoint test.
    t = REGISTER_TEST("misc", "misc_bezier_closest_point");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // FIXME-TESTS: Store normalized?
        static ImVec2 points[4] = { ImVec2(30, 75), ImVec2(185, 355), ImVec2(400, 60), ImVec2(590, 370) };
        static int num_segments = 0;
        const ImGuiStyle& style = ctx->UiContext->Style;

        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Appearing);
        ImGui::Begin("Bezier", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::DragInt("Segments", &num_segments, 0.05f, 0, 20);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 mouse_pos = ImGui::GetMousePos();
        const ImVec2 wp = ImGui::GetWindowPos();

        // Draw modifiable control points
        for (ImVec2& pt : points)
        {
            const float half_circle = 2.0f;
            const float full_circle = half_circle * 2.0f;
            ImRect r(wp + pt - ImVec2(half_circle, half_circle), wp + pt + ImVec2(half_circle, half_circle));
            ImGuiID id = ImGui::GetID((void*)(&pt - points));

            ImGui::ItemAdd(r, id);
            bool is_hovered = ImGui::IsItemHovered();
            bool is_active = ImGui::IsItemActive();
            if (is_hovered || is_active)
                draw_list->AddCircleFilled(r.GetCenter(), full_circle, IM_COL32(0,255,0,255));
            else
                draw_list->AddCircle(r.GetCenter(), full_circle, IM_COL32(0,255,0,255));

            if (is_active)
            {
                if (ImGui::IsMouseDown(0))
                    pt = mouse_pos - wp;
                else
                    ImGui::ClearActiveID();
            }
            else if (ImGui::IsMouseDown(0) && is_hovered)
                ImGui::SetActiveID(id, ImGui::GetCurrentWindow());
        }
        draw_list->AddLine(wp + points[0], wp + points[1], IM_COL32(0,255,0,100));
        draw_list->AddLine(wp + points[2], wp + points[3], IM_COL32(0,255,0,100));

        // Draw curve itself
        draw_list->AddBezierCurve(wp + points[0], wp + points[1], wp + points[2], wp + points[3], IM_COL32_WHITE, 2.0f, num_segments);

        // Draw point closest to the mouse cursor
        ImVec2 point;
        if (num_segments == 0)
            point = ImBezierClosestPointCasteljau(wp + points[0], wp + points[1], wp + points[2], wp + points[3], mouse_pos, style.CurveTessellationTol);
        else
            point = ImBezierClosestPoint(wp + points[0], wp + points[1], wp + points[2], wp + points[3], mouse_pos, num_segments);
        draw_list->AddCircleFilled(point, 4.0f, IM_COL32(255,0,0,255));

        ImGui::End();
    };

    // FIXME-TESTS
    t = REGISTER_TEST("demo", "demo_misc_001");
    t->GuiFunc = NULL;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
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
        ctx->ItemOpen("Scrolling");
        ctx->ItemHold("Scrolling/>>", 1.0f);
        ctx->SleepShort();
    };

    // ## Coverage: open everything in demo window
    // ## Extra: test for inconsistent ScrollMax.y across whole demo window
    // ## Extra: run Log/Capture api on whole demo window
    t = REGISTER_TEST("demo", "demo_cov_auto_open");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->ItemOpenAll("");

        // Additional tests we bundled here because we are benefiting from the "opened all" state
        ImGuiWindow* window = ctx->GetWindowByRef("");
        ctx->ScrollVerifyScrollMax(window);

        // Test the Log/Capture api
        const char* clipboard = ImGui::GetClipboardText();
        IM_CHECK(strlen(clipboard) == 0);
        ctx->ItemClick("Capture\\/Logging/LogButtons/Log To Clipboard");
        clipboard = ImGui::GetClipboardText();
        const int clipboard_len = (int)strlen(clipboard);
        IM_CHECK_GT(clipboard_len, 15000); // This is going to vary (as of 2019-11-18 on Master this 22766)
    };

    // ## Coverage: closes everything in demo window
    t = REGISTER_TEST("demo", "demo_cov_auto_close");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
    };

    t = REGISTER_TEST("demo", "demo_cov_001");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->ItemOpen("Help");
        ctx->ItemOpen("Configuration");
        ctx->ItemOpen("Window options");
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Layout");
        ctx->ItemOpen("Popups & Modal windows");
#if IMGUI_HAS_TABLE
        ctx->ItemOpen("Tables & Columns");
#else
        ctx->ItemOpen("Columns");
#endif
        ctx->ItemOpen("Filtering");
        ctx->ItemOpen("Inputs, Navigation & Focus");
    };

    // ## Open misc elements which are beneficial to coverage and not covered with ItemOpenAll
    t = REGISTER_TEST("demo", "demo_cov_002");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->ItemOpen("Layout");
        ctx->ItemOpen("Scrolling");
        ctx->ItemCheck("Scrolling/Show Horizontal contents size demo window");   // FIXME-TESTS: ItemXXX functions could do the recursion (e.g. Open parent)
        ctx->ItemUncheck("Scrolling/Show Horizontal contents size demo window");

        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuCheck("Tools/About Dear ImGui");
        ctx->WindowRef("About Dear ImGui");
        ctx->ItemCheck("Config\\/Build Information");
        ctx->WindowRef("Dear ImGui Demo");

        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuCheck("Tools/Style Editor");
        ctx->WindowRef("Style Editor");
        ctx->ItemClick("##tabs/Sizes");
        ctx->ItemClick("##tabs/Colors");
        ctx->ItemClick("##tabs/Fonts");
        ctx->ItemClick("##tabs/Rendering");

        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Custom rendering");
        ctx->WindowRef("Example: Custom rendering");
        ctx->ItemClick("##TabBar/Primitives");
        ctx->ItemClick("##TabBar/Canvas");
        ctx->ItemClick("##TabBar/BG\\/FG draw lists");

        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuUncheckAll("Tools");
    };

    t = REGISTER_TEST("demo", "demo_cov_apps");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuClick("Menu/Open Recent/More..");
        ctx->MenuCheckAll("Examples");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuCheckAll("Tools");
        ctx->MenuUncheckAll("Tools");
    };

    // ## Coverage: select all styles via the Style Editor
    t = REGISTER_TEST("demo", "demo_cov_styles");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuAction(ImGuiTestAction_Check, "Tools/Style Editor");

        ImGuiTestRef ref_window = "Style Editor";
        ctx->WindowRef(ref_window);
        ctx->ItemClick("Colors##Selector");
        ctx->Yield();
        ImGuiTestRef ref_popup = ctx->GetFocusWindowRef();

        ImGuiStyle style_backup = ImGui::GetStyle();
        ImGuiTestItemList items;
        ctx->GatherItems(&items, ref_popup);
        for (int n = 0; n < items.GetSize(); n++)
        {
            ctx->WindowRef(ref_window);
            ctx->ItemClick("Colors##Selector");
            ctx->WindowRef(ref_popup);
            ctx->ItemClick(items[n]->ID);
        }
        ImGui::GetStyle() = style_backup;
    };
}

//-------------------------------------------------------------------------
// Tests: Performance Tests
//-------------------------------------------------------------------------
// FIXME-TESTS: Maybe group and expose in a different spot of the UI?
// We currently don't call RegisterTests_Perf() by default because those are more costly.
//-------------------------------------------------------------------------

void RegisterTests_Perf(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    auto PerfCaptureFunc = [](ImGuiTestContext* ctx)
    {
        //ctx->KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
        ctx->PerfCapture();
        //ctx->KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
    };

    // ## Measure the cost all demo contents
    t = REGISTER_TEST("perf", "perf_demo_all");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->PerfCalcRef();

        ctx->WindowRef("Dear ImGui Demo");
        ctx->ItemOpenAll("");
        ctx->MenuCheckAll("Examples");
        ctx->MenuCheckAll("Tools");

        IM_CHECK_SILENT(ctx->UiContext->IO.DisplaySize.x > 820);
        IM_CHECK_SILENT(ctx->UiContext->IO.DisplaySize.y > 820);

        // FIXME-TESTS: Backup full layout
        ImVec2 pos = ctx->GetMainViewportPos() + ImVec2(20, 20);
        for (ImGuiWindow* window : ctx->UiContext->Windows)
        {
            window->Pos = pos;
            window->SizeFull = ImVec2(800, 800);
        }
        ctx->PerfCapture();

        ctx->WindowRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuUncheckAll("Tools");
    };

    // ## Measure the drawing cost of various ImDrawList primitives
    enum
    {
        DrawPrimFunc_RectStroke,
        DrawPrimFunc_RectStrokeThick,
        DrawPrimFunc_RectFilled,
        DrawPrimFunc_RectRoundedStroke,
        DrawPrimFunc_RectRoundedStrokeThick,
        DrawPrimFunc_RectRoundedFilled,
        DrawPrimFunc_CircleStroke,
        DrawPrimFunc_CircleStrokeThick,
        DrawPrimFunc_CircleFilled,
        DrawPrimFunc_TriangleStroke,
        DrawPrimFunc_TriangleStrokeThick,
        DrawPrimFunc_LongStroke,
        DrawPrimFunc_LongStrokeThick,
        DrawPrimFunc_LongJaggedStroke,
        DrawPrimFunc_LongJaggedStrokeThick,
		DrawPrimFunc_Line,
		DrawPrimFunc_LineAA,
#ifdef IMGUI_HAS_TEXLINES
        DrawPrimFunc_LineAANoTex,
#endif
        DrawPrimFunc_LineThick,
		DrawPrimFunc_LineThickAA,
#ifdef IMGUI_HAS_TEXLINES
        DrawPrimFunc_LineThickAANoTex
#endif
    };

    auto DrawPrimFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 200 * ctx->PerfStressAmount;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        int segments = 0;
        ImGui::Button("##CircleFilled", ImVec2(120, 120));
        ImVec2 bounds_min = ImGui::GetItemRectMin();
        ImVec2 bounds_size = ImGui::GetItemRectSize();
        ImVec2 center = bounds_min + bounds_size * 0.5f;
        float r = (float)(int)(ImMin(bounds_size.x, bounds_size.y) * 0.8f * 0.5f);
        float rounding = 8.0f;
        ImU32 col = IM_COL32(255, 255, 0, 255);
		ImDrawListFlags old_flags = draw_list->Flags; // Save old flags as some of these tests manipulate them
        switch (ctx->Test->ArgVariant)
        {
        case DrawPrimFunc_RectStroke:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r,r), center + ImVec2(r,r), col, 0.0f, ~0, 1.0f);
            break;
        case DrawPrimFunc_RectStrokeThick:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r, r), center + ImVec2(r, r), col, 0.0f, ~0, 4.0f);
            break;
        case DrawPrimFunc_RectFilled:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRectFilled(center - ImVec2(r, r), center + ImVec2(r, r), col, 0.0f);
            break;
        case DrawPrimFunc_RectRoundedStroke:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r, r), center + ImVec2(r, r), col, rounding, ~0, 1.0f);
            break;
        case DrawPrimFunc_RectRoundedStrokeThick:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r, r), center + ImVec2(r, r), col, rounding, ~0, 4.0f);
            break;
        case DrawPrimFunc_RectRoundedFilled:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRectFilled(center - ImVec2(r, r), center + ImVec2(r, r), col, rounding);
            break;
        case DrawPrimFunc_CircleStroke:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddCircle(center, r, col, segments, 1.0f);
            break;
        case DrawPrimFunc_CircleStrokeThick:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddCircle(center, r, col, segments, 4.0f);
            break;
        case DrawPrimFunc_CircleFilled:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddCircleFilled(center, r, col, segments);
            break;
        case DrawPrimFunc_TriangleStroke:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddNgon(center, r, col, 3, 1.0f);
            break;
        case DrawPrimFunc_TriangleStrokeThick:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddNgon(center, r, col, 3, 4.0f);
            break;
        case DrawPrimFunc_LongStroke:
            draw_list->AddNgon(center, r, col, 10 * loop_count, 1.0f);
            break;
        case DrawPrimFunc_LongStrokeThick:
            draw_list->AddNgon(center, r, col, 10 * loop_count, 4.0f);
            break;
        case DrawPrimFunc_LongJaggedStroke:
            for (float n = 0; n < 10 * loop_count; n += 2.51327412287f)
                draw_list->PathLineTo(center + ImVec2(r * sinf(n), r * cosf(n)));
            draw_list->PathStroke(col, false, 1.0);
            break;
        case DrawPrimFunc_LongJaggedStrokeThick:
            for (float n = 0; n < 10 * loop_count; n += 2.51327412287f)
                draw_list->PathLineTo(center + ImVec2(r * sinf(n), r * cosf(n)));
            draw_list->PathStroke(col, false, 4.0);
            break;
		case DrawPrimFunc_Line:
			draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;
			for (int n = 0; n < loop_count; n++)
				draw_list->AddLine(center - ImVec2(r, r), center + ImVec2(r, r), col, 1.0f);
			break;
		case DrawPrimFunc_LineAA:
            draw_list->Flags |= ImDrawListFlags_AntiAliasedLines;
			for (int n = 0; n < loop_count; n++)
				draw_list->AddLine(center - ImVec2(r, r), center + ImVec2(r, r), col, 1.0f);
			break;
#ifdef IMGUI_HAS_TEXLINES
		case DrawPrimFunc_LineAANoTex:
			draw_list->Flags |= ImDrawListFlags_AntiAliasedLines;
			draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLinesUseTexData;
			for (int n = 0; n < loop_count; n++)
				draw_list->AddLine(center - ImVec2(r, r), center + ImVec2(r, r), col, 1.0f);
			break;
#endif
		case DrawPrimFunc_LineThick:
			draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;
			for (int n = 0; n < loop_count; n++)
				draw_list->AddLine(center - ImVec2(r, r), center + ImVec2(r, r), col, 4.0f);
			break;
		case DrawPrimFunc_LineThickAA:
			draw_list->Flags |= ImDrawListFlags_AntiAliasedLines;
			for (int n = 0; n < loop_count; n++)
				draw_list->AddLine(center - ImVec2(r, r), center + ImVec2(r, r), col, 4.0f);
			break;
#ifdef IMGUI_HAS_TEXLINES
        case DrawPrimFunc_LineThickAANoTex:
			draw_list->Flags |= ImDrawListFlags_AntiAliasedLines;
			draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLinesUseTexData;
			for (int n = 0; n < loop_count; n++)
				draw_list->AddLine(center - ImVec2(r, r), center + ImVec2(r, r), col, 4.0f);
			break;
#endif
        default:
            IM_ASSERT(0);
        }
		draw_list->Flags = old_flags; // Restre flags
        ImGui::End();
    };

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_stroke");
    t->ArgVariant = DrawPrimFunc_RectStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_stroke_thick");
    t->ArgVariant = DrawPrimFunc_RectStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_filled");
    t->ArgVariant = DrawPrimFunc_RectFilled;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_rounded_stroke");
    t->ArgVariant = DrawPrimFunc_RectRoundedStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_rounded_stroke_thick");
    t->ArgVariant = DrawPrimFunc_RectRoundedStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_rounded_filled");
    t->ArgVariant = DrawPrimFunc_RectRoundedFilled;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_circle_stroke");
    t->ArgVariant = DrawPrimFunc_CircleStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_circle_stroke_thick");
    t->ArgVariant = DrawPrimFunc_CircleStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_circle_filled");
    t->ArgVariant = DrawPrimFunc_CircleFilled;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_triangle_stroke");
    t->ArgVariant = DrawPrimFunc_TriangleStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_triangle_stroke_thick");
    t->ArgVariant = DrawPrimFunc_TriangleStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_long_stroke");
    t->ArgVariant = DrawPrimFunc_LongStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_long_stroke_thick");
    t->ArgVariant = DrawPrimFunc_LongStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_long_jagged_stroke");
    t->ArgVariant = DrawPrimFunc_LongJaggedStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_long_jagged_stroke_thick");
    t->ArgVariant = DrawPrimFunc_LongJaggedStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_line");
	t->ArgVariant = DrawPrimFunc_Line;
	t->GuiFunc = DrawPrimFunc;
	t->TestFunc = PerfCaptureFunc;

	t = REGISTER_TEST("perf", "perf_draw_prim_line_antialiased");
	t->ArgVariant = DrawPrimFunc_LineAA;
	t->GuiFunc = DrawPrimFunc;
	t->TestFunc = PerfCaptureFunc;

#ifdef IMGUI_HAS_TEXLINES
	t = REGISTER_TEST("perf", "perf_draw_prim_line_antialiased_no_tex");
	t->ArgVariant = DrawPrimFunc_LineAANoTex;
	t->GuiFunc = DrawPrimFunc;
	t->TestFunc = PerfCaptureFunc;
#endif

	t = REGISTER_TEST("perf", "perf_draw_prim_line_thick");
	t->ArgVariant = DrawPrimFunc_LineThick;
	t->GuiFunc = DrawPrimFunc;
	t->TestFunc = PerfCaptureFunc;

	t = REGISTER_TEST("perf", "perf_draw_prim_line_thick_antialiased");
	t->ArgVariant = DrawPrimFunc_LineThickAA;
	t->GuiFunc = DrawPrimFunc;
	t->TestFunc = PerfCaptureFunc;

#ifdef IMGUI_HAS_TEXLINES
	t = REGISTER_TEST("perf", "perf_draw_prim_line_thick_antialiased_no_tex");
	t->ArgVariant = DrawPrimFunc_LineThickAANoTex;
	t->GuiFunc = DrawPrimFunc;
	t->TestFunc = PerfCaptureFunc;
#endif

    // ## Measure the cost of ImDrawListSplitter split/merge functions
    auto DrawSplittedFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        int split_count = ctx->Test->ArgVariant;
        int loop_count = 200 * ctx->PerfStressAmount;

        ImGui::Button("##CircleFilled", ImVec2(200, 200));
        ImVec2 bounds_min = ImGui::GetItemRectMin();
        ImVec2 bounds_size = ImGui::GetItemRectSize();
        ImVec2 center = bounds_min + bounds_size * 0.5f;
        float r = (float)(int)(ImMin(bounds_size.x, bounds_size.y) * 0.8f * 0.5f);

        if (ctx->FrameCount == 0)
            ctx->LogDebug("%d primitives over %d channels", loop_count, split_count);
        if (split_count != 1)
        {
            IM_CHECK_SILENT((loop_count % split_count) == 0);
            loop_count /= split_count;
            draw_list->ChannelsSplit(split_count);
            for (int n = 0; n < loop_count; n++)
                for (int ch = 0; ch < split_count; ch++)
                {
                    draw_list->ChannelsSetCurrent(ch);
                    draw_list->AddCircleFilled(center, r, IM_COL32(255, 255, 0, 255), 12);
                }
            draw_list->ChannelsMerge();
        }
        else
        {
            for (int n = 0; n < loop_count; n++)
                draw_list->AddCircleFilled(center, r, IM_COL32(255, 255, 0, 255), 12);
        }
        if (ctx->FrameCount == 0)
            ctx->LogDebug("Vertices: %d", draw_list->VtxBuffer.Size);

        ImGui::End();
    };

    t = REGISTER_TEST("perf", "perf_draw_split_1");
    t->ArgVariant = 1;
    t->GuiFunc = DrawSplittedFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_split_0");
    t->ArgVariant = 10;
    t->GuiFunc = DrawSplittedFunc;
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of simple Button() calls
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

    // ## Measure the cost of simple Checkbox() calls
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

    // ## Measure the cost of dumb column-like setup using SameLine()
    t = REGISTER_TEST("perf", "perf_stress_rows_1a");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 50 * 2 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::TextUnformatted("Cell 1");
            ImGui::SameLine(100);
            ImGui::TextUnformatted("Cell 2");
            ImGui::SameLine(200);
            ImGui::TextUnformatted("Cell 3");
        }
        ImGui::Columns(1);
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_rows_1b");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 50 * 2 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::Text("Cell 1");
            ImGui::SameLine(100);
            ImGui::Text("Cell 2");
            ImGui::SameLine(200);
            ImGui::Text("Cell 3");
        }
        ImGui::Columns(1);
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of NextColumn(): one column set, many rows
    t = REGISTER_TEST("perf", "perf_stress_columns_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Columns(3, "Columns", true);
        int loop_count = 50 * 2 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::Text("Cell 1,%d", n);
            ImGui::NextColumn();
            ImGui::TextUnformatted("Cell 2");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Cell 3");
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of Columns(): many columns sets, few rows
    t = REGISTER_TEST("perf", "perf_stress_columns_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 50 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            ImGui::Columns(3, "Columns", true);
            for (int row = 0; row < 2; row++)
            {
                ImGui::Text("Cell 1,%d", n);
                ImGui::NextColumn();
                ImGui::TextUnformatted("Cell 2");
                ImGui::NextColumn();
                ImGui::TextUnformatted("Cell 3");
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

#ifdef IMGUI_HAS_TABLE
    // ## Measure the cost of TableNextCell(), TableNextRow(): one table, many rows
    t = REGISTER_TEST("perf", "perf_stress_table_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 50 * 2 * ctx->PerfStressAmount;
        if (ImGui::BeginTable("Table", 3, ImGuiTableFlags_BordersV))
        {
            for (int n = 0; n < loop_count; n++)
            {
                ImGui::TableNextCell();
                ImGui::Text("Cell 1,%d", n);
                ImGui::TableNextCell();
                ImGui::TextUnformatted("Cell 2");
                ImGui::TableNextCell();
                ImGui::TextUnformatted("Cell 3");
            }
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of BeginTable(): many tables with few rows
    t = REGISTER_TEST("perf", "perf_stress_table_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 50 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            if (ImGui::BeginTable("Table", 3, ImGuiTableFlags_BordersV))
            {
                for (int row = 0; row < 2; row++)
                {
                    ImGui::TableNextCell();
                    ImGui::Text("Cell 1,%d", n);
                    ImGui::TableNextCell();
                    ImGui::TextUnformatted("Cell 2");
                    ImGui::TableNextCell();
                    ImGui::TextUnformatted("Cell 3");
                }
                ImGui::EndTable();
            }
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;
#endif // IMGUI_HAS_TABLE

    // ## Measure the cost of simple ColorEdit4() calls (multi-component, group based widgets are quite heavy)
    t = REGISTER_TEST("perf", "perf_stress_coloredit4");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 500 * ctx->PerfStressAmount;
        ImVec4 col(1.0f, 0.0f, 0.0f, 1.0f);
        for (int n = 0; n < loop_count / 2; n++)
        {
            ImGui::PushID(n);
            ImGui::ColorEdit4("Color", &col.x);
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of simple InputText() calls
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

    // ## Measure the cost of simple InputTextMultiline() calls
    // (this is creating a child window for every non-clipped widget, so doesn't scale very well)
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

    // ## Measure the cost of our ImHashXXX functions
    t = REGISTER_TEST("perf", "perf_stress_hash");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Hashing..");
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

    // ## Measure the cost of simple Listbox() calls
    // (this is creating a child window for every non-clipped widget, so doesn't scale very well)
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

    // ## Measure the cost of simple SliderFloat() calls
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

    // ## Measure the cost of simple SliderFloat2() calls
    // This at a glance by compared to SliderFloat() test shows us the overhead of group-based multi-component widgets
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

    // ## Measure the cost of TextUnformatted() calls with relatively short text
    t = REGISTER_TEST("perf", "perf_stress_text_unformatted_1");
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

    // ## Measure the cost of TextUnformatted() calls with long text
    t = REGISTER_TEST("perf", "perf_stress_text_unformatted_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        static ImGuiTextBuffer buf;
        if (buf.empty())
        {
            for (int i = 0; i < 1000; i++)
                buf.appendf("%i The quick brown fox jumps over the lazy dog\n", i);
        }
        int loop_count = 100 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
            ImGui::TextUnformatted(buf.begin(), buf.end());
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost/overhead of creating too many windows
    t = REGISTER_TEST("perf", "perf_stress_window");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImVec2 pos = ctx->GetMainViewportPos() + ImVec2(20, 20);
        int loop_count = 200 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::SetNextWindowPos(pos);
            ImGui::Begin(Str16f("Window_%05d", n + 1).c_str(), NULL, ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextUnformatted("Opening many windows!");
            ImGui::End();
        }
    };
    t->TestFunc = PerfCaptureFunc;

	// ## Circle segment count comparisons
	t = REGISTER_TEST("perf", "perf_circle_segment_counts");
	t->GuiFunc = [](ImGuiTestContext* ctx)
	{
		const int num_cols = 3; // Number of columns to draw
		const int num_rows = 3; // Number of rows to draw
		const float max_radius = 400.0f; // Maximum allowed radius

		static ImVec2 content_size(32.0f, 32.0f); // Size of window content on last frame

        // ImGui::SetNextWindowSize(ImVec2((num_cols + 0.5f) * item_spacing.x, (num_rows * item_spacing.y) + 128.0f));
		if (ImGui::Begin("perf_circle_segment_counts", NULL, ImGuiWindowFlags_HorizontalScrollbar))
		{
			// Control area
			static float line_width = 1.0f;
			static float radius = 32.0f;
            static int segment_count_manual = 32;
            static bool no_aa = false;
            static bool overdraw = false;
            static ImVec4 color_bg = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
            static ImVec4 color_fg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImGui::SliderFloat("Line width", &line_width, 1.0f, 10.0f);
            ImGui::SliderFloat("Radius", &radius, 1.0f, max_radius);
			ImGui::SliderInt("Segments", &segment_count_manual, 1, 512);
			ImGui::DragFloat("Circle segment Max Error", &ImGui::GetStyle().CircleSegmentMaxError, 0.01f, 0.1f, 10.0f, "%.2f", 1.0f);
			ImGui::Checkbox("No anti-aliasing", &no_aa);
			ImGui::SameLine();
			ImGui::Checkbox("Overdraw", &overdraw);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Draws each primitive repeatedly so that stray low-alpha pixels are easier to spot");
			ImGui::SameLine();
			const char* fill_modes[] = { "Stroke", "Fill", "Stroke+Fill", "Fill+Stroke" };
			static int fill_mode = 0;
			ImGui::SetNextItemWidth(128.0f);
			ImGui::Combo("Fill mode", &fill_mode, fill_modes, IM_ARRAYSIZE(fill_modes));

            ImGui::ColorEdit4("Color BG", &color_bg.x);
            ImGui::ColorEdit4("Color FG", &color_fg.x);

			// Display area
			ImGui::SetNextWindowContentSize(content_size);
			ImGui::BeginChild("Display", ImGui::GetContentRegionAvail(), false, ImGuiWindowFlags_HorizontalScrollbar);
			ImDrawList* draw_list = ImGui::GetWindowDrawList();

			// Set up the grid layout
			const float spacing = ImMax(96.0f, (radius * 2.0f) + line_width + 8.0f);
			ImVec2 item_spacing(spacing, spacing); // Spacing between rows/columns
			ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

			// Draw the first <n> radius/segment size pairs in a quasi-logarithmic down the side
			for (int pair_rad = 1, step = 1; pair_rad <= 512; pair_rad += step)
			{
                int segment_count = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(pair_rad, draw_list->_Data->CircleSegmentMaxError);
				ImGui::TextColored((pair_rad == (int)radius) ? color_bg : color_fg, "Rad %d = %d segs", pair_rad, segment_count);
				if ((pair_rad >= 16) && ((pair_rad & (pair_rad - 1)) == 0))
					step *= 2;
			}

			// Calculate the worst-case width for the size pairs
			float max_pair_width = ImGui::CalcTextSize("Rad 0000 = 0000 segs").x;

			const ImVec2 text_standoff(max_pair_width + 64.0f, 16.0f); // How much space to leave for the size list and labels
			ImVec2 base_pos(cursor_pos.x + (item_spacing.x * 0.5f) + text_standoff.x, cursor_pos.y + text_standoff.y);

			// Update content size for next frame
			content_size = ImVec2(cursor_pos.x + (item_spacing.x * num_cols) + text_standoff.x, ImMax(cursor_pos.y + (item_spacing.y * num_rows) + text_standoff.y, ImGui::GetCursorPosY()));

			// Save old flags
			ImDrawListFlags backup_draw_list_flags = draw_list->Flags;
			if (no_aa)
				draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;

			// Get the suggested segment count for this radius
			const int segment_count_suggested = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, draw_list->_Data->CircleSegmentMaxError);
            const int segment_count_by_column[3] = { segment_count_suggested, segment_count_manual, 512 };
            const char* desc_by_column[3] = { "auto", "manual", "max" };

			// Draw row/column labels
			for (int i = 0; i < num_cols; i++)
			{
                Str64f name("%s\n%d segs", desc_by_column[i], segment_count_by_column[i]);
                draw_list->AddText(
                    ImVec2(cursor_pos.x + max_pair_width + 8.0f, cursor_pos.y + text_standoff.y + ((i + 0.5f) * item_spacing.y)),
                    ImGui::GetColorU32(color_fg), name.c_str());
                draw_list->AddText(
                    ImVec2(cursor_pos.x + text_standoff.x + ((i + 0.2f) * item_spacing.x), cursor_pos.y),
                    ImGui::GetColorU32(color_bg), name.c_str());
			}

			// Draw circles
			for (int y = 0; y < num_rows; y++)
			{
				for (int x = 0; x < num_cols; x++)
				{
					ImVec2 center = ImVec2(base_pos.x + (item_spacing.x * x), base_pos.y + (item_spacing.y * (y + 0.5f)));
					for (int pass = 0; pass < 2; pass++)
					{
                        const int type_index = (pass == 0) ? x : y;
                        const ImU32 color = ImColor((pass == 0) ? color_bg : color_fg);
                        const int num_segment_count = segment_count_by_column[type_index];
                        const int num_shapes_to_draw = overdraw ? 20 : 1;

                        // We fill either if fill mode was selected, or in stroke+fill mode for the first pass (so we can see what varying segment count fill+stroke looks like)
                        const bool fill = (fill_mode == 1) || (fill_mode == 2 && pass == 1) || (fill_mode == 3 && pass == 0);
						for (int i = 0; i < num_shapes_to_draw; i++)
						{
							if (fill)
								draw_list->AddCircleFilled(center, radius, color, num_segment_count);
							else
								draw_list->AddCircle(center, radius, color, num_segment_count, line_width);
						}
					}
				}
			}

			// Restore draw list flags
			draw_list->Flags = backup_draw_list_flags;
			ImGui::EndChild();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;
    
    // ## Draw various AA/non-AA lines (not really a perf test, more a correctness one)
	t = REGISTER_TEST("perf", "perf_misc_lines");
	t->GuiFunc = [](ImGuiTestContext* ctx)
	{
		const int num_cols = 16; // Number of columns (line rotations) to draw
		const int num_rows = 3; // Number of rows (line variants) to draw
		const float line_len = 64.0f; // Length of line to draw
		const ImVec2 line_spacing(80.0f, 96.0f); // Spacing between lines

		ImGui::SetNextWindowSize(ImVec2((num_cols + 0.5f) * line_spacing.x, (num_rows * 2 * line_spacing.y) + 128.0f), ImGuiCond_Once);
		if (ImGui::Begin("perf_misc_lines"))
		{
			ImDrawList* draw_list = ImGui::GetWindowDrawList();

			static float base_rot = 0.0f;
			ImGui::SliderFloat("Base rotation", &base_rot, 0.0f, 360.0f);
			static float line_width = 1.0f;
			ImGui::SliderFloat("Line width", &line_width, 1.0f, 10.0f);

            ImGui::Text("Press SHIFT to toggle textured/non-textured rows");
            bool tex_toggle = ImGui::GetIO().KeyShift;

            // Rotating lines
			ImVec2 cursor_pos = ImGui::GetCursorPos();
            ImVec2 cursor_screen_pos = ImGui::GetCursorScreenPos();
            ImVec2 base_pos(cursor_screen_pos.x + (line_spacing.x * 0.5f), cursor_screen_pos.y);

#ifndef IMGUI_HAS_TEXLINES
            const ImDrawListFlags ImDrawListFlags_AntiAliasedLinesUseTexData = 0;
#endif

			for (int i = 0; i < num_rows; i++)
			{
                int row_idx = i;

                if (tex_toggle)
                {
                    if (i == 1)
                        row_idx = 2;
                    else if (i == 2)
                        row_idx = 1;
                }

				const char* name = "";
				switch (row_idx)
				{
				case 0: name = "No AA";  draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines; break;
				case 1: name = "AA no texturing"; draw_list->Flags |= ImDrawListFlags_AntiAliasedLines; draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLinesUseTexData; break;
				case 2: name = "AA w/ texturing"; draw_list->Flags |= ImDrawListFlags_AntiAliasedLines; draw_list->Flags |= ImDrawListFlags_AntiAliasedLinesUseTexData; break;
				}

				int initial_vtx_count = draw_list->VtxBuffer.Size;
				int initial_idx_count = draw_list->IdxBuffer.Size;

				for (int j = 0; j < num_cols; j++)
				{
					float r = (base_rot * IM_PI / 180.0f) + ((j * IM_PI * 0.5f) / (num_cols - 1));

					ImVec2 center = ImVec2(base_pos.x + (line_spacing.x * j), base_pos.y + (line_spacing.y * (i + 0.5f)));
					ImVec2 start = ImVec2(center.x + (ImSin(r) * line_len * 0.5f), center.y + (ImCos(r) * line_len * 0.5f));
					ImVec2 end = ImVec2(center.x - (ImSin(r) * line_len * 0.5f), center.y - (ImCos(r) * line_len * 0.5f));

					draw_list->AddLine(start, end, IM_COL32(255, 255, 255, 255), line_width);
				}

				ImGui::SetCursorPosY(cursor_pos.y + (i * line_spacing.y));
				ImGui::Text("%s - %d vertices, %d indices", name, draw_list->VtxBuffer.Size - initial_vtx_count, draw_list->IdxBuffer.Size - initial_idx_count);
			}

			ImGui::SetCursorPosY(cursor_pos.y + (num_rows * line_spacing.y));

            // Squares
            cursor_pos = ImGui::GetCursorPos();
            cursor_screen_pos = ImGui::GetCursorScreenPos();
            base_pos = ImVec2(cursor_screen_pos.x + (line_spacing.x * 0.5f), cursor_screen_pos.y);

            for (int i = 0; i < num_rows; i++)
            {
                int row_idx = i;

                if (tex_toggle)
                {
                    if (i == 1)
                        row_idx = 2;
                    else if (i == 2)
                        row_idx = 1;
                }

                const char* name = "";
                switch (row_idx)
                {
                case 0: name = "No AA";  draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines; break;
                case 1: name = "AA no texturing"; draw_list->Flags |= ImDrawListFlags_AntiAliasedLines; draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLinesUseTexData; break;
                case 2: name = "AA w/ texturing"; draw_list->Flags |= ImDrawListFlags_AntiAliasedLines; draw_list->Flags |= ImDrawListFlags_AntiAliasedLinesUseTexData; break;
                }

                int initial_vtx_count = draw_list->VtxBuffer.Size;
                int initial_idx_count = draw_list->IdxBuffer.Size;

                for (int j = 0; j < num_cols; j++)
                {
                    float cell_line_width = line_width + ((j * 4.0f) / (num_cols - 1));

                    ImVec2 center = ImVec2(base_pos.x + (line_spacing.x * j), base_pos.y + (line_spacing.y * (i + 0.5f)));
                    ImVec2 top_left = ImVec2(center.x - (line_len * 0.5f), center.y - (line_len * 0.5f));
                    ImVec2 bottom_right = ImVec2(center.x + (line_len * 0.5f), center.y + (line_len * 0.5f));

                    draw_list->AddRect(top_left, bottom_right, IM_COL32(255, 255, 255, 255), 0.0f, ImDrawCornerFlags_All, cell_line_width);
                    
                    ImGui::SetCursorPos(ImVec2(cursor_pos.x + ((j + 0.5f) * line_spacing.x) - 16.0f, cursor_pos.y + ((i + 0.5f) * line_spacing.y) - (ImGui::GetTextLineHeight() * 0.5f)));
                    ImGui::Text("%.2f", cell_line_width);
                }

                ImGui::SetCursorPosY(cursor_pos.y + (i * line_spacing.y));
                ImGui::Text("%s - %d vertices, %d indices", name, draw_list->VtxBuffer.Size - initial_vtx_count, draw_list->IdxBuffer.Size - initial_idx_count);
            }
		}
		ImGui::End();
	};
	t->TestFunc = PerfCaptureFunc;

    // ## Measure performance of drawlist text rendering
    {
        enum PerfTestTextFlags : int
        {
            PerfTestTextFlags_TextShort             = 1 << 0,
            PerfTestTextFlags_TextLong              = 1 << 1,
            PerfTestTextFlags_TextWayTooLong        = 1 << 2,
            PerfTestTextFlags_NoWrapWidth           = 1 << 3,
            PerfTestTextFlags_WithWrapWidth         = 1 << 4,
            PerfTestTextFlags_NoCpuFineClipRect     = 1 << 5,
            PerfTestTextFlags_WithCpuFineClipRect   = 1 << 6,
        };
        auto measure_text_rendering_perf = [](ImGuiTestContext* ctx)
        {
            ImGui::SetNextWindowSize(ImVec2(300, 120), ImGuiCond_Always);
            ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings);

            ImGuiWindow* window = ImGui::GetCurrentWindow();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            int test_variant = ctx->Test->ArgVariant;
            ImVector<char>& str = ctx->GenericVars.StrLarge;
            float wrap_width = 0.0f;
            int& line_count = ctx->GenericVars.Int1;
            int& line_length = ctx->GenericVars.Int2;
            ImVec4* cpu_fine_clip_rect = NULL;
            ImVec2& text_size = ctx->GenericVars.Vec2;
            ImVec2 window_padding = ImGui::GetCursorScreenPos() - window->Pos;

            if (test_variant & PerfTestTextFlags_WithWrapWidth)
                wrap_width = 250.0f;

            if (test_variant & PerfTestTextFlags_WithCpuFineClipRect)
                cpu_fine_clip_rect = &ctx->GenericVars.Vec4;

            // Set up test string.
            if (ctx->GenericVars.StrLarge.empty())
            {
                if (test_variant & PerfTestTextFlags_TextLong)
                {
                    line_count = 6;
                    line_length = 2000;
                }
                else if (test_variant & PerfTestTextFlags_TextWayTooLong)
                {
                    line_count = 1;
                    line_length = 10000;
                }
                else
                {
                    line_count = 400;
                    line_length = 30;
                }

                // Support variable stress.
                line_count *= ctx->PerfStressAmount;

                // Create a test string.
                str.resize(line_length * line_count);
                for (int i = 0; i < str.Size; i++)
                    str.Data[i] = '0' + (char)(((2166136261 ^ i) * 16777619) & 0x7F) % ('z' - '0');
                for (int i = 14; i < str.Size; i += 15)
                    str.Data[i] = ' ';      // Spaces for word wrap.
                str.back() = 0;             // Null-terminate

                // Measure text size and cache result.
                text_size = ImGui::CalcTextSize(str.begin(), str.begin() + line_length, false, wrap_width);

                // Set up a cpu fine clip rect which should be about half of rect rendered text would occupy.
                if (test_variant & PerfTestTextFlags_WithCpuFineClipRect)
                {
                    cpu_fine_clip_rect->x = window->Pos.x + window_padding.x;
                    cpu_fine_clip_rect->y = window->Pos.y + window_padding.y;
                    cpu_fine_clip_rect->z = window->Pos.x + window_padding.x + text_size.x * 0.5f;
                    cpu_fine_clip_rect->w = window->Pos.y + window_padding.y + text_size.y * line_count * 0.5f;
                }
            }

            // Push artificially large clip rect to prevent coarse clipping of text on CPU side.
            ImVec2 text_pos = window->Pos + window_padding;
            draw_list->PushClipRect(text_pos, text_pos + ImVec2(text_size.x * 1.0f, text_size.y * line_count));
            for (int i = 0, end = line_count; i < end; i++)
                draw_list->AddText(NULL, 0.0f, text_pos + ImVec2(0.0f, text_size.y * (float)i),
                    IM_COL32_WHITE, str.begin() + (i * line_length), str.begin() + ((i + 1) * line_length), wrap_width, cpu_fine_clip_rect);
            draw_list->PopClipRect();

            ImGui::End();
        };
        const char* base_name = "perf_draw_text";
        const char* text_suffixes[] = { "_short", "_long", "_too_long" };
        const char* wrap_suffixes[] = { "", "_wrapped" };
        const char* clip_suffixes[] = { "", "_clipped" };
        for (int i = 0; i < IM_ARRAYSIZE(text_suffixes); i++)
        {
            for (int j = 0; j < IM_ARRAYSIZE(wrap_suffixes); j++)
            {
                for (int k = 0; k < IM_ARRAYSIZE(clip_suffixes); k++)
                {
                    Str64f test_name("%s%s%s%s", base_name, text_suffixes[i], wrap_suffixes[j], clip_suffixes[k]);
                    t = REGISTER_TEST("perf", "");
                    t->SetOwnedName(test_name.c_str());
                    t->ArgVariant = (PerfTestTextFlags_TextShort << i) | (PerfTestTextFlags_NoWrapWidth << j) | (PerfTestTextFlags_NoCpuFineClipRect << k);
                    t->GuiFunc = measure_text_rendering_perf;
                    t->TestFunc = PerfCaptureFunc;
                }
            }
        }
    }
}

//-------------------------------------------------------------------------
// Tests: Capture
//-------------------------------------------------------------------------

void RegisterTests_Capture(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    t = REGISTER_TEST("capture", "capture_demo_documents");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuAction(ImGuiTestAction_Check, "Examples/Documents");

        ctx->WindowRef("Example: Documents");
        // FIXME-TESTS: Locate within stack that uses windows/<pointer>/name
        //ctx->ItemCheck("Tomato"); // FIXME: WILL FAIL, NEED TO LOCATE BY NAME (STACK WILDCARD?)
        //ctx->ItemCheck("A Rather Long Title"); // FIXME: WILL FAIL
        //ctx->ItemClick("##tabs/Eggplant");
        //ctx->MouseMove("##tabs/Eggplant/Modify");
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

        ctx->WindowRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuCheck("Examples/Simple overlay");
        ctx->WindowRef("Example: Simple overlay");
        ImGuiWindow* window_overlay = ctx->GetWindowByRef("");
        IM_CHECK(window_overlay != NULL);

        // FIXME-TESTS: Find last newly opened window?

        float fh = ImGui::GetFontSize();
        float pad = fh;

        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Custom rendering");
        ctx->WindowRef("Example: Custom rendering");
        ctx->WindowResize("", ImVec2(fh * 30, fh * 30));
        ctx->WindowMove("", window_overlay->Rect().GetBL() + ImVec2(0.0f, pad));
        ImGuiWindow* window_custom_rendering = ctx->GetWindowByRef("");
        IM_CHECK(window_custom_rendering != NULL);

        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Simple layout");
        ctx->WindowRef("Example: Simple layout");
        ctx->WindowResize("", ImVec2(fh * 50, fh * 15));
        ctx->WindowMove("", ImVec2(pad, io.DisplaySize.y - pad), ImVec2(0.0f, 1.0f));

        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Documents");
        ctx->WindowRef("Example: Documents");
        ctx->WindowResize("", ImVec2(fh * 20, fh * 27));
        ctx->WindowMove("", ImVec2(window_custom_rendering->Pos.x + window_custom_rendering->Size.x + pad, pad));

        ctx->LogDebug("Setup Console window...");
        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Console");
        ctx->WindowRef("Example: Console");
        ctx->WindowResize("", ImVec2(fh * 40, fh * (34-7)));
        ctx->WindowMove("", window_custom_rendering->Pos + window_custom_rendering->Size * ImVec2(0.30f, 0.60f));
        ctx->ItemClick("Clear");
        ctx->ItemClick("Add Dummy Text");
        ctx->ItemClick("Add Dummy Error");
        ctx->ItemClick("Input");
        ctx->KeyChars("H");
        ctx->KeyPressMap(ImGuiKey_Tab);
        ctx->KeyCharsAppendEnter("ELP");
        ctx->KeyCharsAppendEnter("hello, imgui world!");

        ctx->LogDebug("Setup Demo window...");
        ctx->WindowRef("Dear ImGui Demo");
        ctx->WindowResize("", ImVec2(fh * 35, io.DisplaySize.y - pad * 2.0f));
        ctx->WindowMove("", ImVec2(io.DisplaySize.x - pad, pad), ImVec2(1.0f, 0.0f));
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Color\\/Picker Widgets");
        ctx->ItemOpen("Layout");
        ctx->ItemOpen("Groups");
        ctx->ScrollToItemY("Layout", 0.8f);

        ctx->LogDebug("Capture screenshot...");
        ctx->WindowRef("");

        ImGuiCaptureArgs args;
        ctx->CaptureInitArgs(&args);
        args.InPadding = pad;
        ctx->CaptureAddWindow(&args, "Dear ImGui Demo");
        ctx->CaptureAddWindow(&args, "Example: Simple overlay");
        ctx->CaptureAddWindow(&args, "Example: Custom rendering");
        ctx->CaptureAddWindow(&args, "Example: Simple layout");
        ctx->CaptureAddWindow(&args, "Example: Documents");
        ctx->CaptureAddWindow(&args, "Example: Console");
        ctx->CaptureScreenshot(&args);

        // Close everything
        ctx->WindowRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuUncheckAll("Tools");
    };
#endif

    // ## Capture a screenshot displaying different supported styles.
    t = REGISTER_TEST("capture", "capture_readme_styles");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        const ImGuiStyle backup_style = style;
        const bool backup_cursor_blink = io.ConfigInputTextCursorBlink;

        // Setup style
        // FIXME-TESTS: Ideally we'd want to be able to manipulate fonts
        ImFont* font = FindFontByName("Roboto-Medium.ttf, 16px");
        IM_CHECK_SILENT(font != NULL);
        ImGui::PushFont(font);
        style.FrameRounding = style.ChildRounding = 0;
        style.GrabRounding = 0;
        style.FrameBorderSize = style.ChildBorderSize = 1;
        io.ConfigInputTextCursorBlink = false;

        // Show two windows
        for (int n = 0; n < 2; n++)
        {
            bool open = true;
            ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Appearing);
            if (n == 0)
            {
                ImGui::StyleColorsDark(&style);
                ImGui::Begin("Debug##Dark", &open);
            }
            else
            {
                ImGui::StyleColorsLight(&style);
                ImGui::Begin("Debug##Light", &open);
            }
            char string_buffer[] = "";
            float float_value = 0.6f;
            ImGui::Text("Hello, world 123");
            ImGui::Button("Save");
            ImGui::SetNextItemWidth(194);
            ImGui::InputText("string", string_buffer, IM_ARRAYSIZE(string_buffer));
            ImGui::SetNextItemWidth(194);
            ImGui::SliderFloat("float", &float_value, 0.0f, 1.0f);
            ImGui::End();
        }
        ImGui::PopFont();

        // Restore style
        style = backup_style;
        io.ConfigInputTextCursorBlink = backup_cursor_blink;
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Capture both windows in separate captures
        ImGuiContext& g = *ctx->UiContext;
        for (int n = 0; n < 2; n++)
        {
            ImGuiWindow* window = (n == 0) ? ctx->GetWindowByRef("/Debug##Dark") : ctx->GetWindowByRef("/Debug##Light");
            ctx->WindowRef(window->Name);
            ctx->ItemClick("string");
            ctx->KeyChars("quick brown fox");
            //ctx->KeyPressMap(ImGuiKey_End);
            ctx->MouseMove("float");
            ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(30, -10));

            ImGuiCaptureArgs args;
            ctx->CaptureInitArgs(&args);
            ctx->CaptureAddWindow(&args, window->Name);
            ctx->CaptureScreenshot(&args);
        }
    };
    t = REGISTER_TEST("capture", "capture_readme_gif");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Appearing);
        ImGui::Begin("CaptureGif");
        static char string_buffer[64] = {};
        static float float_value = 0.6f;
        ImGui::Text("Hello, world 123");
        ImGui::Button("Save");
        ImGui::SetNextItemWidth(194);
        ImGui::InputText("string", string_buffer, IM_ARRAYSIZE(string_buffer));
        ImGui::SetNextItemWidth(194);
        ImGui::SliderFloat("float", &float_value, 0.0f, 4.0f);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("CaptureGif");
        ImGuiWindow* window = ctx->GetWindowByRef("/CaptureGif");
        ImGuiCaptureArgs args;
        ctx->CaptureInitArgs(&args);
        ctx->CaptureAddWindow(&args, window->Name);
        ctx->BeginCaptureGif(&args);
        ctx->ItemInput("string");
        ctx->KeyCharsReplace("Dear ImGui: Now with gif animations \\o/");
        ctx->SleepShort();
        ctx->ItemInput("float");
        ctx->KeyCharsReplaceEnter("3.14");
        ctx->SleepShort();
        ctx->ItemClick("Save");
        ctx->SleepShort();
        ctx->EndCaptureGif(&args);
        ctx->Yield();
    };

#ifdef IMGUI_HAS_TABLE
    // ## Capture all tables demo
    t = REGISTER_TEST("capture", "capture_table_demo");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->ItemOpen("Tables & Columns");
        ctx->ItemClick("Tables/Open all");
        
        ImGuiCaptureArgs args;
        ctx->CaptureInitArgs(&args, ImGuiCaptureFlags_StitchFullContents);
        ctx->CaptureAddWindow(&args, "");
        ctx->CaptureScreenshot(&args);
    };
#endif
}

void RegisterTests(ImGuiTestEngine* e)
{
    // Tests
    RegisterTests_Window(e);
    RegisterTests_Layout(e);
    RegisterTests_Widgets(e);
    RegisterTests_Nav(e);
    RegisterTests_Columns(e);
    RegisterTests_Table(e);
    RegisterTests_Docking(e);
    RegisterTests_Misc(e);

    // Captures
    RegisterTests_Capture(e);

    // Performance Benchmarks
    RegisterTests_Perf(e);
}
