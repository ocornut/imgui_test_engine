// dear imgui
// (tests)

#define _CRT_SECURE_NO_WARNINGS
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_tests.h"
#include "imgui_te_core.h"
#include "imgui_te_context.h"
#include "imgui_te_util.h"

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

// Helper Operators
static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
static inline bool FloatEqual(float f1, float f2, float epsilon = FLT_EPSILON) { float d = f2 - f1; return fabsf(d) <= FLT_EPSILON; }

#define REGISTER_TEST(_CATEGORY, _NAME)    ImGuiTestEngine_RegisterTest(e, _CATEGORY, _NAME, __FILE__, __LINE__);

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
            //ctx->Log("%f %f, %f %f\n", pos.x, pos.y, size.x, size.y);
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
            //ctx->Log("%f %f, %f %f\n", pos.x, pos.y, size.x, size.y);
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
        ctx->SetRef("Test Window");
        ctx->WindowSetCollapsed("", false);
        ctx->LogVerbose("Size %f %f, SizeFull %f %f\n", window->Size.x, window->Size.y, window->SizeFull.x, window->SizeFull.y);
        IM_CHECK_EQ(window->Size, window->SizeFull);
        ImVec2 size_full_when_uncollapsed = window->SizeFull;
        ctx->WindowSetCollapsed("", true);
        ctx->LogVerbose("Size %f %f, SizeFull %f %f\n", window->Size.x, window->Size.y, window->SizeFull.x, window->SizeFull.y);
        ImVec2 size_collapsed = window->Size;
        IM_CHECK_GT(size_full_when_uncollapsed.y, size_collapsed.y);
        IM_CHECK_EQ(size_full_when_uncollapsed, window->SizeFull);
        ctx->WindowSetCollapsed("", false);
        ctx->LogVerbose("Size %f %f, SizeFull %f %f\n", window->Size.x, window->Size.y, window->SizeFull.x, window->SizeFull.y);
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
        ctx->FocusWindow("/CCCC");
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
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Popups & Modal windows");
        ctx->ItemOpen("Popups");
        ctx->ItemClick("Popups/Toggle..");

        ImGuiWindow* popup_1 = g.NavWindow;
        ctx->SetRef(popup_1->Name);
        ctx->ItemClick("Stacked Popup");
        IM_CHECK(popup_1->WasActive);

        ImGuiWindow* popup_2 = g.NavWindow;
        ctx->MouseMove("Bream", ImGuiTestOpFlags_NoFocusWindow | ImGuiTestOpFlags_NoCheckHoveredId);
        ctx->MouseClick(1); // Close with right-click
        IM_CHECK(popup_1->WasActive);
        IM_CHECK(!popup_2->WasActive);
        IM_CHECK(g.NavWindow == popup_1);
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
            IM_CHECK(ctx->UiContext->CurrentWindow->SkipItems == false);
        ImGui::EndChild();
        ImGui::SetCursorPos(ImVec2(300, 300));
        ImGui::BeginChild("Child 2", ImVec2(100, 100));
        if (ctx->FrameCount == 2)
            IM_CHECK(ctx->UiContext->CurrentWindow->SkipItems == true);
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
        ctx->LogVerbose("scroll_y = %f\n", scroll_y);
        ctx->LogVerbose("scroll_max_y = %f\n", scroll_max_y);
        ctx->LogVerbose("window->SizeContents.y = %f\n", window->ContentSize.y);

        IM_CHECK_NO_RET(scroll_y > 0.0f);
        IM_CHECK_NO_RET(scroll_y == scroll_max_y);

        float expected_size_contents_y = 100 * ImGui::GetTextLineHeightWithSpacing() - style.ItemSpacing.y; // Newer definition of SizeContents as per 1.71
        IM_CHECK(FloatEqual(window->ContentSize.y, expected_size_contents_y));

        float expected_scroll_max_y = expected_size_contents_y + window->WindowPadding.y * 2.0f - window->InnerRect.GetHeight();
        IM_CHECK(FloatEqual(scroll_max_y, expected_scroll_max_y));
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
                ctx->LogVerbose("Test '%s' variant %d\n", item_type_name, n);

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

                char label[32];
                ImFormatString(label, IM_ARRAYSIZE(label), (label_line_count == 1) ? "%s%d" : "%s%d\nHello", item_type_name, n);

                float expected_padding = 0.0f;
                switch (item_type)
                {
                case ItemType_SmallButton:
                    expected_padding = window->DC.CurrLineTextBaseOffset;
                    ImGui::SmallButton(label);
                    break;
                case ItemType_Button:
                    expected_padding = style.FramePadding.y * 2.0f;
                    ImGui::Button(label);
                    break;
                case ItemType_Text:
                    expected_padding = window->DC.CurrLineTextBaseOffset;
                    ImGui::Text(label);
                    break;
                case ItemType_BulletText:
                    expected_padding = (n <= 2) ? 0.0f : style.FramePadding.y * 1.0f;
                    ImGui::BulletText(label);
                    break;
                case ItemType_TreeNode:
                    expected_padding = (n <= 2) ? 0.0f : style.FramePadding.y * 2.0f;
                    ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_NoTreePushOnOpen);
                    break;
                case ItemType_Selectable:
                    // FIXME-TESTS: We may want to aim the specificies of Selectable() and not clear ItemSpacing
                    //expected_padding = style.ItemSpacing.y * 0.5f;
                    expected_padding = window->DC.CurrLineTextBaseOffset;
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                    ImGui::Selectable(label);
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
    struct ButtonPressTestVars { int ButtonPressCount[4] = { 0 }; };
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
        if (ImGui::ButtonEx("Button3", ImVec2(0, 0), ImGuiButtonFlags_Repeat))
            vars.ButtonPressCount[3]++;
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ButtonPressTestVars& vars = ctx->GetUserData<ButtonPressTestVars>();

        ctx->SetRef("Test Window");
        ctx->ItemClick("Button0");
        IM_CHECK_EQ(vars.ButtonPressCount[0], 1);
        ctx->ItemDoubleClick("Button1");
        IM_CHECK_EQ(vars.ButtonPressCount[1], 1);
        ctx->ItemDoubleClick("Button2");
        IM_CHECK_EQ(vars.ButtonPressCount[2], 2);

        ctx->ItemClick("Button3");
        IM_CHECK_EQ(vars.ButtonPressCount[3], 1);
        ctx->MouseDown(0);
        IM_CHECK_EQ(vars.ButtonPressCount[3], 1);
        ctx->SleepDebugNoSkip(ctx->UiContext->IO.KeyRepeatDelay + ctx->UiContext->IO.KeyRepeatRate * 3); // FIXME-TESTS: Can we elapse context time without elapsing wall clock time?
        IM_CHECK_EQ(vars.ButtonPressCount[3], 1 + 3 * 2); // FIXME: MouseRepeatRate is double KeyRepeatRate, that's not documented
        ctx->MouseUp(0);
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
    struct ButtonStateTestVars { ButtonStateMachineTestStep NextStep; ImGuiTestGenericStatus Status; };
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

        ctx->SetRef("Test Window");

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
        IM_CHECK(ctx->GenericVars.Bool1 == false);
        ctx->ItemClick("Window1/Checkbox");
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
        ctx->SetRef("Test Window");

        IM_CHECK_EQ(vars.Int1, 0);
        ctx->ItemInput("Drag");
        IM_CHECK_EQ(ctx->UiContext->ActiveId, ctx->GetID("Drag"));
        ctx->KeyCharsAppendEnter("123");
        IM_CHECK_EQ(vars.Int1, 123);

        ctx->ItemInput("Color##Y");
        IM_CHECK_EQ(ctx->UiContext->ActiveId, ctx->GetID("Color##Y"));
        ctx->KeyCharsAppend("123");
        IM_CHECK(FloatEqual(vars.Vec4.y, 123.0f / 255.0f));
        ctx->KeyPressMap(ImGuiKey_Tab);
        ctx->KeyCharsAppendEnter("200");
        IM_CHECK(FloatEqual(vars.Vec4.x,   0.0f / 255.0f));
        IM_CHECK(FloatEqual(vars.Vec4.y, 123.0f / 255.0f));
        IM_CHECK(FloatEqual(vars.Vec4.z, 200.0f / 255.0f));
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

        ctx->SetRef("Test Window");

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
        //ImDebugShowInputTextState();
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

        ctx->SetRef("Test Window");
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
        //ctx->KeyPressMap(ImGuiKey_V, ImGuiKeyModFlags_Ctrl, 10);

        // Select all, copy, paste 3 times
        // FIXME-TESTS: [Omar] This is occasionally failing under Windows.. which I tracked to Win32 ::OpenClipbard() returning false occasionally :(
        // I really don't know what we are supposed to do, if there are local apps creating clipboard contention and what is the proper solution.
        // When it happened and I first studied the problem, I noticed that after a reboot it would not happen again..
        // Perhaps the win32 clipboard behavior is up to applications behaving decently, 
        // - if our imgui bindings/apps are the misbehaving cause: what's the issue and how do we fix it?
        // - if our imgui bindings/apps are not the cause: can we workaround it? How would other apps deal with it, retry calling OpenClipboard multiple times??
        // Somehow it would make some sense if the testing engine used its own internal clipboard and then this would be a non-issue.
        // But also the purpose of testing is to also detect those larger issues...
        ctx->KeyPressMap(ImGuiKey_A, ImGuiKeyModFlags_Ctrl);    // Select all
        ctx->KeyPressMap(ImGuiKey_C, ImGuiKeyModFlags_Ctrl);    // Copy
        ctx->KeyPressMap(ImGuiKey_End, ImGuiKeyModFlags_Ctrl);  // Go to end, clear selection
        ctx->SleepShort();
        for (int n = 0; n < 3; n++)
        {
            ctx->KeyPressMap(ImGuiKey_V, ImGuiKeyModFlags_Ctrl);// Paste append three times
            ctx->SleepShort();
        }
        int len = (int)strlen(vars.StrLarge.Data);
        IM_CHECK_EQ(len, 350 * 4);
        IM_CHECK_EQ(undo_state.undo_point, 3);
        IM_CHECK_EQ(undo_state.undo_char_point, 0);

        // Undo x2
        IM_CHECK(undo_state.redo_point == STB_TEXTEDIT_UNDOSTATECOUNT);
        ctx->KeyPressMap(ImGuiKey_Z, ImGuiKeyModFlags_Ctrl);
        ctx->KeyPressMap(ImGuiKey_Z, ImGuiKeyModFlags_Ctrl);
        len = (int)strlen(vars.StrLarge.Data);
        IM_CHECK_EQ(len, 350 * 2);
        IM_CHECK_EQ(undo_state.undo_point, 1);
        IM_CHECK_EQ(undo_state.redo_point, STB_TEXTEDIT_UNDOSTATECOUNT - 2);
        IM_CHECK_EQ(undo_state.redo_char_point, STB_TEXTEDIT_UNDOCHARCOUNT - 350 * 2);

        // Undo x1 should call stb_textedit_discard_redo()
        ctx->KeyPressMap(ImGuiKey_Z, ImGuiKeyModFlags_Ctrl);
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
        ImFormatString(vars.Str2, IM_ARRAYSIZE(vars.Str2), "%s", ctx->UiContext->LogBuffer.c_str());
        ImGui::LogFinish();
        ImGui::Text("Captured: \"%s\"", vars.Str2);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        char* buf_user = vars.Str1;
        char* buf_visible = vars.Str2;
        ctx->SetRef("Test Window");

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
        ctx->SetRef("Test Window");
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
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field");
        ctx->UiContext->IO.AddInputCharacter((ImWchar)'\t');
        ctx->KeyPressMap(ImGuiKey_Tab);
        IM_CHECK_STR_EQ(vars.Str1, "\t");
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
        ctx->SetRef("Test Window");
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
        ctx->SetRef("Test Window");
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
        ctx->SetRef("Test Window");
        int n;
        n = 0; ImGuiID field_0 = ImHash(&n, sizeof(n), ctx->GetID("Field"));
        n = 1; ImGuiID field_1 = ImHash(&n, sizeof(n), ctx->GetID("Field"));
        //n = 2; ImGuiID field_2 = ImHash(&n, sizeof(n), ctx->GetID("Field"));

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
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field");
        ctx->KeyCharsAppend("1");
        IM_CHECK(status.Ret == 1 && status.Edited == 1 && status.Activated == 1 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0);
        ctx->Yield();
        ctx->Yield();
        IM_CHECK(status.Edited == 1);
    };

    // ## Test ColorEdit basic Drag and Drop
    t = REGISTER_TEST("widgets", "widgets_coloredit_drag");
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

        ctx->SetRef("Test Window");

        IM_CHECK_NE(memcmp(&vars.Vec4Array[0], &vars.Vec4Array[1], sizeof(ImVec4)), 0);
        ctx->ItemDragAndDrop("ColorEdit1/##ColorButton", "ColorEdit2/##X"); // FIXME-TESTS: Inner items
        IM_CHECK_EQ(memcmp(&vars.Vec4Array[0], &vars.Vec4Array[1], sizeof(ImVec4)), 0);
    };

    // ## Test that disabled Selectable has an ID but doesn't interfere with navigation
    t = REGISTER_TEST("widgets", "widgets_selectable_disabled");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ctx->SetRef("Test Window");
        ImGui::Selectable("Selectable A");
        if (ctx->FrameCount == 0)
            IM_CHECK_EQ(ImGui::GetItemID(), ctx->GetID("Selectable A"));
        ImGui::Selectable("Selectable B", false, ImGuiSelectableFlags_Disabled);
        if (ctx->FrameCount == 0)
            IM_CHECK_EQ(ImGui::GetItemID(), ctx->GetID("Selectable B")); // Make sure B has an ID
        ImGui::Selectable("Selectable C");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
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
            {
                char name[32];
                ImFormatString(name, sizeof(name), "Tab %d", i);
                if (ImGui::BeginTabItem(name))
                    ImGui::EndTabItem();
            }
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
                {
                    char buf[64];
                    ImFormatString(buf, IM_ARRAYSIZE(buf), "Inner TabBar %d", i);
                    if (ImGui::BeginTabBar(buf))
                    {
                        if (ImGui::BeginTabItem("Inner TabItem"))
                            ImGui::EndTabItem();
                        ImGui::EndTabBar();
                    }
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
}

//-------------------------------------------------------------------------
// Tests: Nav
//-------------------------------------------------------------------------

void RegisterTests_Nav(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test opening a new window from a checkbox setting the focus to the new window.
    // In 9ba2028 (2019/01/04) we fixed a bug where holding ImGuiNavInputs_Activate too long on a button would hold the focus on the wrong window.
    t = REGISTER_TEST("nav", "nav_001");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetInputMode(ImGuiInputSource_Nav);
        ctx->SetRef("Hello, world!");
        ctx->ItemUncheck("Demo Window");
        ctx->ItemCheck("Demo Window");

        ImGuiContext& g = *ctx->UiContext;
        IM_CHECK(g.NavWindow && g.NavWindow->ID == ctx->GetID("/Dear ImGui Demo"));
    };

    // ## Test that CTRL+Tab steal focus (#2380)
    t = REGISTER_TEST("nav", "nav_ctrl_tab_takes_focus_away");
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
        ctx->SetRef("Test window 1");
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

        ctx->SetRef("Test Window");
        ctx->ItemClick("Open Popup");

        while (popup_id == 0 && !ctx->IsError())
            ctx->Yield();

        ctx->SetRef(popup_id);
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
        ctx->SetRef("Test window");
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
        {
            for (int i = 0; i < 10; i++)
            {
                char name[32];
                ImFormatString(name, IM_ARRAYSIZE(name), "Button %d", i);
                ImGui::Button(name);
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        IM_CHECK(ctx->UiContext->IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard);
        ImGuiWindow* window = ImGui::FindWindowByName("Test Window");
        ctx->SetRef("Test window");
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
}

//-------------------------------------------------------------------------
// Tests: Columns
//-------------------------------------------------------------------------

#ifdef IMGUI_HAS_TABLE
static void HelperTableSubmitCells(int count_w, int count_h)
{
    for (int line = 0; line < count_h; line++)
        for (int column = 0; column < count_w; column++)
        {
            char label[32];
            ImFormatString(label, IM_ARRAYSIZE(label), "%d,%d", line, column);
            ImGui::TableNextCell();
            ImGui::Button(label, ImVec2(-FLT_MIN, 0.0f));
        }
}
#endif

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

#ifdef IMGUI_HAS_TABLE

    t = REGISTER_TEST("table", "table_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::BeginTable("##table0", 4);
        ImGui::TableAddColumn("One", 0, ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableAddColumn("Two");
        ImGui::TableAddColumn("Three");
        ImGui::TableAddColumn("Four");
        HelperTableSubmitCells(4, 5);
        ImGuiTable* table = ctx->UiContext->CurrentTable;
        IM_CHECK_EQ(table->Columns[0].WidthRequested, 100.0f);
        ImGui::EndTable();
        ImGui::End();
    };

    // ## Table: measure draw calls count
    t = REGISTER_TEST("table", "table_2_draw_calls");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImGui::Text("Text before");
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            ImGui::BeginTable("##table1", 4, ImGuiTableFlags_NoClipX | ImGuiTableFlags_Borders, ImVec2(400, 0));
            HelperTableSubmitCells(4, 5);
            ImGui::EndTable();
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            ImGui::BeginTable("##table2", 4, ImGuiTableFlags_Borders, ImVec2(400, 0));
            HelperTableSubmitCells(4, 5);
            ImGui::EndTable();
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }

        ImGui::End();
    };
#endif
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
        ctx->LogVerbose("ConfigDockingAlwaysTabBar = false\n");
        ctx->UiContext->IO.ConfigDockingAlwaysTabBar = false;
        ctx->YieldFrames(4);
        ctx->LogVerbose("ConfigDockingAlwaysTabBar = true\n");
        ctx->UiContext->IO.ConfigDockingAlwaysTabBar = true;
        ctx->YieldFrames(4);
        ctx->LogVerbose("ConfigDockingAlwaysTabBar = false\n");
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
            ctx->Log("## TEST CASE %d: with ConfigDockingAlwaysTabBar = %d\n", test_case_n, io.ConfigDockingAlwaysTabBar);

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

    // FIXME-TESTS
    t = REGISTER_TEST("demo", "demo_misc_001");
    t->GuiFunc = NULL;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
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
    t = REGISTER_TEST("demo", "demo_cov_auto_open");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpenAll("");

        ImGuiWindow* window = ctx->GetWindowByRef("");
        ctx->ScrollVerifyScrollMax(window);
    };

    // ## Coverage: closes everything in demo window
    t = REGISTER_TEST("demo", "demo_cov_auto_close");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
    };

    t = REGISTER_TEST("demo", "demo_cov_001");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
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

    // ## Open misc elements which are beneficial to coverage and not covered with ItemOpenAll
    t = REGISTER_TEST("demo", "demo_cov_002");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Layout");
        ctx->ItemOpen("Scrolling");
        ctx->ItemCheck("Scrolling/Show Horizontal contents size demo window");   // FIXME-TESTS: ItemXXX functions could do the recursion (e.g. Open parent)
        ctx->ItemUncheck("Scrolling/Show Horizontal contents size demo window");

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Tools/About Dear ImGui");
        ctx->SetRef("About Dear ImGui");
        ctx->ItemCheck("Config\\/Build Information");
        ctx->SetRef("Dear ImGui Demo");

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Tools/Style Editor");
        ctx->SetRef("Style Editor");
        ctx->ItemClick("##tabs/Sizes");
        ctx->ItemClick("##tabs/Colors");
        ctx->ItemClick("##tabs/Fonts");
        ctx->ItemClick("##tabs/Rendering");

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Custom rendering");
        ctx->SetRef("Example: Custom rendering");
        ctx->ItemClick("##TabBar/Primitives");
        ctx->ItemClick("##TabBar/Canvas");
        ctx->ItemClick("##TabBar/BG\\/FG draw lists");

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuUncheckAll("Tools");
    };

    t = REGISTER_TEST("demo", "demo_cov_apps");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
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
        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuAction(ImGuiTestAction_Check, "Tools/Style Editor");

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
}

//-------------------------------------------------------------------------
// Tests: Perf
//-------------------------------------------------------------------------

void RegisterTests_Perf(ImGuiTestEngine* e)
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

        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpenAll("");
        ctx->MenuCheckAll("Examples");
        ctx->MenuCheckAll("Tools");

        IM_CHECK_GT(ctx->UiContext->IO.DisplaySize.x, 820);
        IM_CHECK_GT(ctx->UiContext->IO.DisplaySize.y, 820);

        // FIXME-TESTS: Backup full layout
        ImVec2 pos = ctx->GetMainViewportPos() + ImVec2(20, 20);
        for (ImGuiWindow* window : ctx->UiContext->Windows)
        {
            window->Pos = pos;
            window->SizeFull = ImVec2(800, 800);
        }
        ctx->PerfCapture();

        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuUncheckAll("Tools");
    };

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
        default:
            IM_ASSERT(0);
        }
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
            ctx->Log("%d primitives over %d channels\n", loop_count, split_count);
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
            ctx->Log("Vertices: %d\n", draw_list->VtxBuffer.Size);

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

void RegisterTests_Capture(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    t = REGISTER_TEST("capture", "capture_demo_documents");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
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

        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuCheck("Examples/Simple overlay");
        ctx->SetRef("Example: Simple overlay");
        ImGuiWindow* window_overlay = ctx->GetWindowByRef("");
        IM_CHECK(window_overlay != NULL);

        // FIXME-TESTS: Find last newly opened window?

        float fh = ImGui::GetFontSize();
        float pad = fh;

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Custom rendering");
        ctx->SetRef("Example: Custom rendering");
        ctx->WindowResize("", ImVec2(fh * 30, fh * 30));
        ctx->WindowMove("", window_overlay->Rect().GetBL() + ImVec2(0.0f, pad));
        ImGuiWindow* window_custom_rendering = ctx->GetWindowByRef("");
        IM_CHECK(window_custom_rendering != NULL);

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Simple layout");
        ctx->SetRef("Example: Simple layout");
        ctx->WindowResize("", ImVec2(fh * 50, fh * 15));
        ctx->WindowMove("", ImVec2(pad, io.DisplaySize.y - pad), ImVec2(0.0f, 1.0f));

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Documents");
        ctx->SetRef("Example: Documents");
        ctx->WindowResize("", ImVec2(fh * 20, fh * 27));
        ctx->WindowMove("", ImVec2(window_custom_rendering->Pos.x + window_custom_rendering->Size.x + pad, pad));

        ctx->LogVerbose("Setup Console window...\n");
        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Console");
        ctx->SetRef("Example: Console");
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

        ctx->LogVerbose("Setup Demo window...\n");
        ctx->SetRef("Dear ImGui Demo");
        ctx->WindowResize("", ImVec2(fh * 35, io.DisplaySize.y - pad * 2.0f));
        ctx->WindowMove("", ImVec2(io.DisplaySize.x - pad, pad), ImVec2(1.0f, 0.0f));
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Color\\/Picker Widgets");
        ctx->ItemOpen("Layout");
        ctx->ItemOpen("Groups");
        ctx->ScrollToY("Layout", 0.8f);

        // FIXME-TESTS: Snap!
        ctx->Sleep(2.0f);

        // Close everything
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuUncheckAll("Tools");
    };
#endif
}

void RegisterTests(ImGuiTestEngine* e)
{
    RegisterTests_Window(e);
    RegisterTests_Layout(e);
    RegisterTests_Widgets(e);
    RegisterTests_Nav(e);
    RegisterTests_Columns(e);
    RegisterTests_Docking(e);
    RegisterTests_Misc(e);
    //RegisterTests_Perf(e);
    RegisterTests_Capture(e);
}
