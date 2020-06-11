// dear imgui
// (tests: main)

#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_tests.h"
#include "shared/imgui_capture_tool.h"
#include "shared/imgui_utils.h"
#include "test_engine/imgui_te_core.h"      // IM_REGISTER_TEST()
#include "test_engine/imgui_te_context.h"
#include "libs/Str/Str.h"

// Warnings
#ifdef _MSC_VER
#pragma warning (disable: 4100) // unreferenced formal parameter
#pragma warning (disable: 4127) // conditional expression is constant
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

// Helpers
static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs)     { return lhs.x == rhs.x && lhs.y == rhs.y; }    // for IM_CHECK_EQ()

//-------------------------------------------------------------------------
// Tests: Window
//-------------------------------------------------------------------------

void RegisterTests_Window(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test size of an empty window
    t = IM_REGISTER_TEST(e, "window", "empty");
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
    t = IM_REGISTER_TEST(e, "window", "window_size_collapsed_1");
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

    t = IM_REGISTER_TEST(e, "window", "window_size_contents");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        {
            ImGui::Begin("Test Contents Size 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::ColorButton("test", ImVec4(1, 0.4f, 0, 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(150, 150));
            ImGuiWindow* window = ctx->UiContext->CurrentWindow;
            if (ctx->FrameCount > 0)
                IM_CHECK_EQ(window->ContentSize, ImVec2(150.0f, 150.0f));
            ImGui::End();
        }
        {
            ImGui::SetNextWindowContentSize(ImVec2(150, 150));
            ImGui::Begin("Test Contents Size 2", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
            ImGuiWindow* window = ctx->UiContext->CurrentWindow;
            if (ctx->FrameCount >= 0)
                IM_CHECK_EQ(window->ContentSize, ImVec2(150.0f, 150.0f));
            ImGui::End();
        }
        {
            ImGui::SetNextWindowContentSize(ImVec2(150, 150));
            ImGui::SetNextWindowSize(ImVec2(150, 150) + style.WindowPadding * 2.0f + ImVec2(0.0f, ImGui::GetFrameHeight()));
            ImGui::Begin("Test Contents Size 3", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_None);
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
            ImGui::Begin("Test Contents Size 4", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_None);
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
    t = IM_REGISTER_TEST(e, "window", "window_size_unrounded");
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
    t = IM_REGISTER_TEST(e, "window", "window_auto_resize_basic");
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
    t = IM_REGISTER_TEST(e, "window", "window_auto_resize_uncollapse");
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
    t = IM_REGISTER_TEST(e, "window", "window_append");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Line 1");
        ImGui::End();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
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
    t = IM_REGISTER_TEST(e, "window", "window_focus_1");
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
    t = IM_REGISTER_TEST(e, "window", "window_focus_popup");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->WindowRef("Dear ImGui Demo");
        ctx->ItemOpen("Popups & Modal windows");
        ctx->ItemOpen("Popups");
        ctx->ItemClick("Popups/Toggle..");

        ImGuiWindow* popup_1 = g.NavWindow;
        ctx->WindowRef(popup_1);
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
    t = IM_REGISTER_TEST(e, "window", "window_child_layout_size");
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
    t = IM_REGISTER_TEST(e, "window", "window_child_clip");
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
    t = IM_REGISTER_TEST(e, "window", "window_scroll_001");
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
    t = IM_REGISTER_TEST(e, "window", "window_scroll_002");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Scrolling 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Dummy(ImVec2(200, 200));
        ImGuiWindow* window1 = ctx->UiContext->CurrentWindow;
        IM_CHECK_NO_RET(window1->ScrollMax.x == 0.0f); // FIXME-TESTS: If another window in another test used same name, ScrollMax won't be zero on first frame
        IM_CHECK_NO_RET(window1->ScrollMax.y == 0.0f);
        ImGui::End();

        ImGui::Begin("Test Scrolling 2", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
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
    t = IM_REGISTER_TEST(e, "window", "window_scroll_003");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Scrolling 3", NULL, ImGuiWindowFlags_NoSavedSettings);
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
    t = IM_REGISTER_TEST(e, "window", "window_scroll_while_resizing");
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
    t = IM_REGISTER_TEST(e, "window", "window_move");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("Movable Window", NULL, ImGuiWindowFlags_NoSavedSettings);
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
    t = IM_REGISTER_TEST(e, "window", "window_close_current_popup");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("Popups", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
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
    t = IM_REGISTER_TEST(e, "layout", "layout_baseline_and_cursormax");
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
    t = IM_REGISTER_TEST(e, "layout", "layout_height_label");
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
// Tests: Nav
//-------------------------------------------------------------------------

void RegisterTests_Nav(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test opening a new window from a checkbox setting the focus to the new window.
    // In 9ba2028 (2019/01/04) we fixed a bug where holding ImGuiNavInputs_Activate too long on a button would hold the focus on the wrong window.
    t = IM_REGISTER_TEST(e, "nav", "nav_basic");
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
    t = IM_REGISTER_TEST(e, "nav", "nav_ctrl_tab_takes_activeid_away");
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
    t = IM_REGISTER_TEST(e, "nav", "nav_esc_popup");
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

    // ## Test that Alt toggle layer, test that AltGr doesn't.
    t = IM_REGISTER_TEST(e, "nav", "nav_menu_alt_key");
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
    t = IM_REGISTER_TEST(e, "nav", "nav_home_end_keys");
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
    t = IM_REGISTER_TEST(e, "nav", "nav_menu_wraparound");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuClick("Menu");
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt); // FIXME
        IM_CHECK(ctx->UiContext->NavId == ctx->GetID("/##Menu_00/New"));
        ctx->NavKeyPress(ImGuiNavInput_KeyUp_);
        IM_CHECK(ctx->UiContext->NavId == ctx->GetID("/##Menu_00/Quit"));
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
        IM_CHECK(ctx->UiContext->NavId == ctx->GetID("/##Menu_00/New"));
    };

    // ## Test CTRL+TAB window focusing
    t = IM_REGISTER_TEST(e, "nav", "nav_ctrl_tab_focusing");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextUnformatted("Not empty space");
        ImGui::End();

        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
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
    t = IM_REGISTER_TEST(e, "nav", "nav_ctrl_tab_nav_id_restore");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Button("Button 1");
        ImGui::End();

        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
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
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_restore");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Button("Button 1");
        ImGui::End();

        if (!ctx->GenericVars.Bool1)
            return;
        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
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
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_restore_menu");
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

    // ## Test navigation in popups that are appended across multiple calls to BeginPopup()/EndPopup(). (#3223)
    t = IM_REGISTER_TEST(e, "nav", "nav_appended_popup");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Menu"))
            {
                ImGui::MenuItem("a");
                ImGui::MenuItem("b");
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Menu"))
            {
                ImGui::MenuItem("c");
                ImGui::MenuItem("d");
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("##MainMenuBar");

        // Open menu, focus first "a" item.
        ctx->MenuClick("Menu");
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt); // FIXME
        ctx->WindowRef(ctx->UiContext->NavWindow);

        // Navigate to "c" item.
        IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("a"));
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
        IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("c"));
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
        IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("a"));
    };
}

//-------------------------------------------------------------------------
// Tests: Draw, ImDrawList
//-------------------------------------------------------------------------

struct HelpersTextureId
{
    // Fake texture ID
    ImTextureID DummyTex0 = (ImTextureID)100;
    ImTextureID DummyTex1 = (ImTextureID)200;

    // Replace fake texture IDs with a known good ID in order to prevent graphics API crashing application.
    void RemoveDummyTexFromDrawList(ImDrawList* draw_list, ImTextureID replacement_tex_id)
    {
        for (ImDrawCmd& cmd : draw_list->CmdBuffer)
            if (cmd.TextureId == DummyTex0 || cmd.TextureId == DummyTex1)
                cmd.TextureId = replacement_tex_id;
    }
};

static bool CanTestVtxOffset(ImGuiTestContext* ctx)
{
    if ((ctx->UiContext->IO.BackendFlags & ImGuiBackendFlags_RendererHasVtxOffset) == 0)
    {
        ctx->LogInfo("Skipping: back-end does not support RendererHasVtxOffset!");
        return false;
    }
    if (sizeof(ImDrawIdx) != 2)
    {
        ctx->LogInfo("sizeof(ImDrawIdx) != 2");
        return false;
    }
    return true;
};

void RegisterTests_DrawList(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test AddCallback()
    t = IM_REGISTER_TEST(e, "drawlist", "drawlist_callbacks");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, 2);
        IM_CHECK_EQ(draw_list->CmdBuffer.back().ElemCount, 0u);
        ImGui::Button("Hello");

        ImDrawCallback cb = [](const ImDrawList* parent_list, const ImDrawCmd* cmd)
        {
            ImGuiTestContext* ctx = (ImGuiTestContext*)cmd->UserCallbackData;
            ctx->GenericVars.Int1++;
        };
        draw_list->AddCallback(cb, (void*)ctx);
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, 4);

        draw_list->AddCallback(cb, (void*)ctx);
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, 5);

        // Test callbacks in columns
        ImGui::Columns(3);
        ImGui::Columns(1);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->GenericVars.Int1 = 0;
        ctx->Yield();
        IM_CHECK_EQ(ctx->GenericVars.Int1, 2);
    };

    // ## Test whether splitting/merging draw lists properly retains a texture id.
    t = IM_REGISTER_TEST(e, "drawlist", "drawlist_splitter_texture_id");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImTextureID prev_texture_id = draw_list->_TextureIdStack.back();
        const int start_cmdbuffer_size = draw_list->CmdBuffer.Size;
        IM_CHECK_EQ(draw_list->CmdBuffer.back().ElemCount, 0u);

        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(100 + 10 + 100, 100));

        HelpersTextureId dummy_tex;

        draw_list->ChannelsSplit(2);
        draw_list->ChannelsSetCurrent(0);
        draw_list->AddImage(dummy_tex.DummyTex0, p, p + ImVec2(100, 100));
        draw_list->ChannelsSetCurrent(1);
        draw_list->AddImage(dummy_tex.DummyTex1, p + ImVec2(110, 0), p + ImVec2(210, 100));
        draw_list->ChannelsMerge();

        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.Size, start_cmdbuffer_size + 2);
        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.back().ElemCount, 0u);
        IM_CHECK_EQ_NO_RET(prev_texture_id, draw_list->CmdBuffer.back().TextureId);

        dummy_tex.RemoveDummyTexFromDrawList(draw_list, prev_texture_id);

        ImGui::End();
    };

    t = IM_REGISTER_TEST(e, "drawlist", "drawlist_merge_cmd");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 200)); // Make sure our columns aren't clipped
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        IM_CHECK_EQ(draw_list->CmdBuffer.back().ElemCount, 0u);

        const int start_cmdbuffer_size = draw_list->CmdBuffer.Size;
        //ImGui::Text("Hello");
        draw_list->AddDrawCmd(); // Empty command
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, start_cmdbuffer_size + 1);
        IM_CHECK_EQ(draw_list->CmdBuffer.back().ElemCount, 0u);

        ImGui::Columns(2);
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, 1); // In channel 1
        ImGui::Text("One");
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, 1); // In channel 1

        ImGui::NextColumn();
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, 1); // In channel 2
        ImGui::Text("Two");
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, 1); // In channel 2

        ImGui::PushColumnsBackground(); // In channel 0 -> will trigger merge
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, start_cmdbuffer_size);
        ImGui::PopColumnsBackground();

        IM_CHECK_EQ(draw_list->CmdBuffer.Size, 1); // In channel 2

        ImGui::Columns(1);
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, start_cmdbuffer_size + 2); // Channel 1 and 2 each other one Cmd

        ImGui::End();
    };

    // ## Test VtxOffset
    t = IM_REGISTER_TEST(e, "drawlist", "drawlist_vtxoffset_basic");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        if (CanTestVtxOffset(ctx) == false)
            return;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        IM_CHECK_EQ(draw_list->CmdBuffer.back().ElemCount, 0u);
        IM_CHECK_EQ(draw_list->CmdBuffer.back().VtxOffset, 0u);

        const int start_vtxbuffer_size = draw_list->VtxBuffer.Size;
        const int start_cmdbuffer_size = draw_list->CmdBuffer.Size;
        ctx->LogDebug("VtxBuffer.Size = %d, CmdBuffer.Size = %d", start_vtxbuffer_size, start_cmdbuffer_size);

        // fill up vertex buffer with rectangles
        const int rect_count = (65536 - start_vtxbuffer_size - 1) / 4;
        const int expected_threshold = rect_count * 4 + start_vtxbuffer_size;
        ctx->LogDebug("rect_count = %d", rect_count);
        ctx->LogDebug("expected_threshold = %d", expected_threshold);

        const ImVec2 p_min = ImGui::GetCursorScreenPos();
        const ImVec2 p_max = p_min + ImVec2(50, 50);
        ImGui::Dummy(p_max - p_min);
        for (int n = 0; n < rect_count; n++)
            draw_list->AddRectFilled(p_min, p_max, IM_COL32(255,0,0,255));

        // we are just before reaching 64k vertex count, so far everything
        // should land single command buffer
        IM_CHECK_EQ_NO_RET(draw_list->VtxBuffer.Size, expected_threshold);
        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.Size, start_cmdbuffer_size);
        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.back().VtxOffset, 0u);
        IM_CHECK_EQ_NO_RET(draw_list->_CmdHeader.VtxOffset, 0u);

        // Test #3232
#if 1
        draw_list->PrimReserve(6, 4);
        draw_list->PrimUnreserve(6, 4);
        ImVec4 clip_rect = draw_list->_ClipRectStack.back();
        draw_list->PushClipRect(ImVec2(clip_rect.x, clip_rect.y), ImVec2(clip_rect.z, clip_rect.w)); // Use same cliprect so pop will easily
        draw_list->PopClipRect();
        IM_CHECK_EQ(draw_list->_CmdHeader.VtxOffset, draw_list->CmdBuffer.back().VtxOffset);
#endif

        // Next rect should pass 64k threshold and emit new command
        draw_list->AddRectFilled(p_min, p_max, IM_COL32(0,255,0,255));
        IM_CHECK_GE_NO_RET(draw_list->VtxBuffer.Size, 65536);
        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.Size, start_cmdbuffer_size + 1);
        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.back().VtxOffset, (unsigned int)expected_threshold);
        IM_CHECK_EQ_NO_RET(draw_list->_CmdHeader.VtxOffset, (unsigned int)expected_threshold);

        ImGui::End();
    };

    // ## Test VtxOffset with Splitter, which should generate identical draw call pattern as test without one
    t = IM_REGISTER_TEST(e, "drawlist", "drawlist_vtxoffset_splitter");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        if (CanTestVtxOffset(ctx) == false)
            return;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        IM_CHECK_EQ(draw_list->CmdBuffer.back().ElemCount, 0u);
        IM_CHECK_EQ(draw_list->CmdBuffer.back().VtxOffset, 0u);

        const int start_vtxbuffer_size = draw_list->VtxBuffer.Size;
        const int start_cmdbuffer_size = draw_list->CmdBuffer.Size;
        ctx->LogDebug("VtxBuffer.Size = %d, CmdBuffer.Size = %d", start_vtxbuffer_size, start_cmdbuffer_size);

        // Fill up vertex buffer with rectangles
        const int rect_count = (65536 - start_vtxbuffer_size - 1) / 4;
        const int expected_threshold = rect_count * 4 + start_vtxbuffer_size;
        ctx->LogDebug("rect_count = %d", rect_count);
        ctx->LogDebug("expected_threshold = %d", expected_threshold);

        const ImVec2 p_min = ImGui::GetCursorScreenPos();
        const ImVec2 p_max = p_min + ImVec2(50, 50);
        const ImU32 color = IM_COL32(255, 255, 255, 255);
        ImGui::Dummy(p_max - p_min);
        draw_list->ChannelsSplit(rect_count);
        for (int n = 0; n < rect_count; n++)
        {
            draw_list->ChannelsSetCurrent(n);
            draw_list->AddRectFilled(p_min, p_max, color);
        }
        draw_list->ChannelsMerge();

        // We are just before reaching 64k vertex count, so far everything should land in single draw command
        IM_CHECK_EQ_NO_RET(draw_list->VtxBuffer.Size, expected_threshold);
        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.Size, start_cmdbuffer_size);
        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.back().VtxOffset, 0u);

        // Next rect should pass 64k threshold and emit new command
        draw_list->AddRectFilled(p_min, p_max, color);
        IM_CHECK_GE_NO_RET(draw_list->VtxBuffer.Size, 65536);
        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.Size, start_cmdbuffer_size + 1);
        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.back().VtxOffset, (unsigned int)expected_threshold);

        ImGui::End();
    };

    // ## Test starting Splitter when VtxOffset != 0
    t = IM_REGISTER_TEST(e, "drawlist", "drawlist_vtxoffset_splitter_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        if (CanTestVtxOffset(ctx) == false)
            return;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
      
        // fill up vertex buffer with rectangles
        const int start_vtxbuffer_size = draw_list->VtxBuffer.Size;
        const int rect_count = (65536 - start_vtxbuffer_size - 1) / 4;
        const int expected_threshold = rect_count * 4 + start_vtxbuffer_size;

        const ImVec2 p_min = ImGui::GetCursorScreenPos();
        const ImVec2 p_max = p_min + ImVec2(50, 50);
        ImGui::Dummy(p_max - p_min);
        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.back().VtxOffset, 0u);
        for (int n = 0; n < rect_count + 1; n++)
            draw_list->AddRectFilled(p_min, p_max, IM_COL32(255, 0, 0, 255));
        unsigned int vtx_offset = draw_list->CmdBuffer.back().VtxOffset;
        IM_CHECK_GT_NO_RET(draw_list->CmdBuffer.back().VtxOffset, 0u);

        ImGui::Columns(3);
        ImGui::Text("AAA");
        ImGui::NextColumn();
        ImGui::Text("BBB");
        ImGui::NextColumn();
        ImGui::Text("CCC");
        ImGui::NextColumn();

        ImGui::Text("AAA 2");
        ImGui::NextColumn();
        ImGui::Text("BBB 2");
        ImGui::NextColumn();
        ImGui::Text("CCC 2");
        ImGui::NextColumn();
        ImGui::Columns(1);

        draw_list->ChannelsSplit(3);
        for (int n = 1; n < 3; n++)
        {
            draw_list->ChannelsSetCurrent(n);
            IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.back().VtxOffset, vtx_offset);
            draw_list->AddRectFilled(p_min, p_max, IM_COL32(0, 255, 0, 255));
        }
        draw_list->ChannelsMerge();

        ImGui::End();
    };

    // ## Test VtxOffset with Splitter with worst case scenario
    // Draw calls are interleaved, one with VtxOffset == 0, next with VtxOffset != 0
    t = IM_REGISTER_TEST(e, "drawlist", "drawlist_vtxoffset_splitter_draw_call_explosion");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        if (CanTestVtxOffset(ctx) == false)
            return;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        IM_CHECK_EQ(draw_list->CmdBuffer.back().ElemCount, 0u);
        IM_CHECK_EQ(draw_list->CmdBuffer.back().VtxOffset, 0u);

        const int start_vtxbuffer_size = draw_list->VtxBuffer.Size;
        const int start_cmdbuffer_size = draw_list->CmdBuffer.Size;
        ctx->LogDebug("VtxBuffer.Size = %d, CmdBuffer.Size = %d", start_vtxbuffer_size, start_cmdbuffer_size);

        // Fill up vertex buffer with rectangles
        const int rect_count = (65536 - start_vtxbuffer_size - 1) / 4;
        ctx->LogDebug("rect_count = %d", rect_count);

        // Expected number of draw calls after interleaving channels with VtxOffset == 0 and != 0
        const int expected_draw_command_count = start_cmdbuffer_size + rect_count * 2 - 1; // minus one, because last channel became active one

        const ImVec2 p_min = ImGui::GetCursorScreenPos();
        const ImVec2 p_max = p_min + ImVec2(50, 50);
        const ImU32 color = IM_COL32(255, 255, 255, 255);
        ImGui::Dummy(p_max - p_min);

        // Make split and draw rect to every even channel
        draw_list->ChannelsSplit(rect_count * 2);

        for (int n = 0; n < rect_count; n++)
        {
            draw_list->ChannelsSetCurrent(n * 2);
            draw_list->AddRectFilled(p_min, p_max, color);
            if (n == 0 || n == rect_count - 1) // Reduce check/log spam
            {
                IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.back().VtxOffset, 0u);
                IM_CHECK_EQ_NO_RET(draw_list->_CmdHeader.VtxOffset, 0u);
            }
        }

        // From this point all new rects will pass 64k vertex count, and draw calls will have VtxOffset != 0
        // Draw rect to every odd channel
        for (int n = 0; n < rect_count; n++)
        {
            draw_list->ChannelsSetCurrent(n * 2 + 1);
            if (n == 0 || n == rect_count - 1) // Reduce check/log spam
                IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.Size, 1);
            if (n == 0)
                IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.back().VtxOffset, 0u);
            draw_list->AddRectFilled(p_min, p_max, color);
            if (n == 0 || n == rect_count - 1) // Reduce check/log spam
            {
                IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.Size, 1); // Confirm that VtxOffset change didn't grow CmdBuffer (at n==0, empty command gets recycled)
                IM_CHECK_GT_NO_RET(draw_list->CmdBuffer.back().VtxOffset, 0u);
                IM_CHECK_GT_NO_RET(draw_list->_CmdHeader.VtxOffset, 0u);
            }
        }

        draw_list->ChannelsSetCurrent(0);
        IM_CHECK_GT_NO_RET(draw_list->CmdBuffer.back().VtxOffset, 0u);

        draw_list->ChannelsMerge();
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, expected_draw_command_count);

        ImGui::End();
    };
}

//-------------------------------------------------------------------------
// Tests: Misc
//-------------------------------------------------------------------------

void RegisterTests_Misc(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test watchdog
#if 0
    t = IM_REGISTER_TEST(e, "misc", "misc_watchdog");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        while (true)
            ctx->Yield();
    };
#endif

    // ## Test window data garbage collection
    t = IM_REGISTER_TEST(e, "misc", "misc_gc");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // Pretend window is no longer active once we start testing.
        if (ctx->FrameCount < 2)
        {
            for (int i = 0; i < 5; i++)
            {
                Str16f name("GC Test %d", i);
                ImGui::Begin(name.c_str(), NULL, ImGuiWindowFlags_NoSavedSettings);
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
    t = IM_REGISTER_TEST(e, "misc", "misc_hash_001");
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
    t = IM_REGISTER_TEST(e, "misc", "misc_vector_001");
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
    t = IM_REGISTER_TEST(e, "misc", "misc_bitarray");
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
    t = IM_REGISTER_TEST(e, "misc", "misc_pool_001");
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
    t = IM_REGISTER_TEST(e, "misc", "misc_format_parse");
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
    t = IM_REGISTER_TEST(e, "misc", "misc_atlas_build_glyph_overlap");
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

    t = IM_REGISTER_TEST(e, "misc", "misc_atlas_ranges_builder");
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


    t = IM_REGISTER_TEST(e, "misc", "misc_repeat_typematic");
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
    t = IM_REGISTER_TEST(e, "misc", "misc_input_scalar_overflow");
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
    t = IM_REGISTER_TEST(e, "misc", "misc_clipboard");
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
    t = IM_REGISTER_TEST(e, "misc", "misc_utf8");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto check_utf8 = [](const char* utf8, const ImWchar* unicode)
        {
            const int utf8_len = (int)strlen(utf8);
            const int max_chars = utf8_len * 4 + 1;

            ImWchar* converted = (ImWchar*)IM_ALLOC((size_t)max_chars * sizeof(ImWchar));
            char* reconverted = (char*)IM_ALLOC((size_t)max_chars * sizeof(char));

            // Convert UTF-8 text to unicode codepoints and check against expected value.
            int result_bytes = ImTextStrFromUtf8(converted, max_chars, utf8, NULL);
            bool success = ImStrlenW((ImWchar*)unicode) == result_bytes && memcmp(converted, unicode, (size_t)result_bytes * sizeof(ImWchar)) == 0;

            // Convert resulting unicode codepoints back to UTF-8 and check them against initial UTF-8 input value.
            if (success)
            {
                result_bytes = ImTextStrToUtf8(reconverted, max_chars, converted, NULL);
                success &= (utf8_len == result_bytes) && (memcmp(utf8, reconverted, (size_t)result_bytes * sizeof(char)) == 0);
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
#ifdef IMGUI_USE_WCHAR32
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

#ifdef IMGUI_USE_WCHAR32
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
#ifdef IMGUI_USE_WCHAR32
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
    };

#ifdef IMGUI_USE_WCHAR32
    t = IM_REGISTER_TEST(e, "misc", "misc_utf16_surrogate");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // See: http://www.russellcottrell.com/greek/utilities/SurrogatePairCalculator.htm
        // 0x20628 = 0xD841 + 0xDE28
        ImGuiIO& io = ctx->UiContext->IO;
        io.ClearInputCharacters();
        IM_CHECK_EQ(io.InputQueueSurrogate, 0);
        io.AddInputCharacterUTF16(0xD841);
        IM_CHECK_EQ(io.InputQueueCharacters.Size, 0);
        IM_CHECK_EQ(io.InputQueueSurrogate, 0xD841);
        io.AddInputCharacterUTF16(0xDE28);
        IM_CHECK_EQ(io.InputQueueCharacters.Size, 1);
        IM_CHECK_EQ(io.InputQueueCharacters[0], (ImWchar)0x20628);
    };
#endif

    // ## Test ImGuiTextFilter
    t = IM_REGISTER_TEST(e, "misc", "misc_text_filter");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        static ImGuiTextFilter filter;
        ImGui::Begin("Text filter", NULL, ImGuiWindowFlags_NoSavedSettings);
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
    t = IM_REGISTER_TEST(e, "misc", "misc_bezier_closest_point");
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
    t = IM_REGISTER_TEST(e, "demo", "demo_misc_001");
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
    t = IM_REGISTER_TEST(e, "demo", "demo_cov_auto_open");
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
    t = IM_REGISTER_TEST(e, "demo", "demo_cov_auto_close");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
    };

    t = IM_REGISTER_TEST(e, "demo", "demo_cov_001");
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
    t = IM_REGISTER_TEST(e, "demo", "demo_cov_002");
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
        ctx->WindowRef("Dear ImGui Style Editor");
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

    t = IM_REGISTER_TEST(e, "demo", "demo_cov_apps");
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
    t = IM_REGISTER_TEST(e, "demo", "demo_cov_styles");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuAction(ImGuiTestAction_Check, "Tools/Style Editor");

        ImGuiTestRef ref_window = "Dear ImGui Style Editor";
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
// Tests: Capture
//-------------------------------------------------------------------------

void RegisterTests_Capture(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    t = IM_REGISTER_TEST(e, "capture", "capture_demo_documents");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Dear ImGui Demo");
        ctx->MenuAction(ImGuiTestAction_Check, "Examples/Documents");

        ctx->WindowRef("Example: Documents");
        ctx->WindowResize(ctx->RefID, ImVec2(600, 300));    // Ensure no items are clipped, because then they cant be found by item search
        ctx->ItemCheck("**/Tomato");
        ctx->ItemCheck("**/A Rather Long Title");
        ctx->ItemClick("##tabs/Eggplant");
        ctx->WindowRef(ctx->GetID("##tabs/Eggplant"));
        ctx->MouseMove("**/Modify");
        ctx->Sleep(1.0f);
    };

#if 1
    // TODO: Better position of windows.
    // TODO: Draw in custom rendering canvas
    // TODO: Select color picker mode
    // TODO: Flags document as "Modified"
    // TODO: InputText selection
    t = IM_REGISTER_TEST(e, "capture", "capture_readme_misc");
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
    t = IM_REGISTER_TEST(e, "capture", "capture_readme_styles");
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
                ImGui::Begin("Debug##Dark", &open, ImGuiWindowFlags_NoSavedSettings);
            }
            else
            {
                ImGui::StyleColorsLight(&style);
                ImGui::Begin("Debug##Light", &open, ImGuiWindowFlags_NoSavedSettings);
            }
            float float_value = 0.6f;
            ImGui::Text("Hello, world 123");
            ImGui::Button("Save");
            ImGui::SetNextItemWidth(194);
            ImGui::InputText("string", ctx->GenericVars.Str1, IM_ARRAYSIZE(ctx->GenericVars.Str1));
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
            ctx->WindowRef(window);
            ctx->ItemClick("string");
            ctx->KeyCharsReplace("quick brown fox");
            //ctx->KeyPressMap(ImGuiKey_End);
            ctx->MouseMove("float");
            ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(30, -10));

            ImGuiCaptureArgs args;
            ctx->CaptureInitArgs(&args);
            ctx->CaptureAddWindow(&args, window->Name);
            ctx->CaptureScreenshot(&args);
        }
    };
    t = IM_REGISTER_TEST(e, "capture", "capture_readme_gif");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Appearing);
        ImGui::Begin("CaptureGif", NULL, ImGuiWindowFlags_NoSavedSettings);
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
    };

#ifdef IMGUI_HAS_TABLE
    // ## Capture all tables demo
    t = IM_REGISTER_TEST(e, "capture", "capture_table_demo");
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
    extern void RegisterTests_Docking(ImGuiTestEngine * e);     // imgui_tests_docking.cpp
    extern void RegisterTests_Perf(ImGuiTestEngine * e);        // imgui_tests_perf.cpp
    extern void RegisterTests_Columns(ImGuiTestEngine * e);     // imgui_tests_table.cpp
    extern void RegisterTests_Table(ImGuiTestEngine * e);       // imgui_tests_table.cpp
    extern void RegisterTests_Widgets(ImGuiTestEngine * e);     // imgui_tests_widgets.cpp

    // Tests
    RegisterTests_Window(e);
    RegisterTests_Layout(e);
    RegisterTests_Widgets(e);
    RegisterTests_Nav(e);
    RegisterTests_Columns(e);
    RegisterTests_Table(e);
    RegisterTests_Docking(e);
    RegisterTests_DrawList(e);
    RegisterTests_Misc(e);

    // Captures
    RegisterTests_Capture(e);

    // Performance Benchmarks
    RegisterTests_Perf(e);
}
