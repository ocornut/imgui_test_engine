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
#include "test_engine/imgui_te_engine.h"      // IM_REGISTER_TEST()
#include "test_engine/imgui_te_context.h"
#include "libs/Str/Str.h"
#ifdef IMGUI_TEST_ENGINE_ENABLE_IMPLOT
#include "libs/implot/implot.h"
#endif

// Warnings
#ifdef _MSC_VER
#pragma warning (disable: 4100) // unreferenced formal parameter
#pragma warning (disable: 4127) // conditional expression is constant
#endif

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wstring-plus-int"                    // warning: adding 'int' to a string does not append to the string
#pragma clang diagnostic ignored "-Wunused-parameter"                   // warning: unused parameter 'XXX'
#elif defined(__GNUC__)
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
    t = IM_REGISTER_TEST(e, "window", "window_empty");
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
        ImVec2 viewport_pos = ctx->GetMainViewportPos();
        // #2067
        {
            ImGui::SetNextWindowPos(viewport_pos + ImVec2(401.0f, 103.0f), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(348.48f, 400.0f), ImGuiCond_Once);
            ImGui::Begin("Issue 2067", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImVec2 pos = ImGui::GetWindowPos() - viewport_pos;
            ImVec2 size = ImGui::GetWindowSize();
            //ctx->LogDebug("%f %f, %f %f", pos.x, pos.y, size.x, size.y);
            IM_CHECK_NO_RET(pos.x == 401.0f && pos.y == 103.0f);
            IM_CHECK_NO_RET(size.x == 348.0f && size.y == 400.0f);
            ImGui::End();
        }
        // Test that non-rounded size constraint are not altering pos/size (#2530)
        {
            ImGui::SetNextWindowPos(viewport_pos + ImVec2(401.0f, 103.0f), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(348.48f, 400.0f), ImGuiCond_Once);
            ImGui::SetNextWindowSizeConstraints(ImVec2(475.200012f, 0.0f), ImVec2(475.200012f, 100.4f));
            ImGui::Begin("Issue 2530", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();
            //ctx->LogDebug("%f %f, %f %f", pos.x, pos.y, size.x, size.y);
            IM_CHECK_EQ(pos, viewport_pos + ImVec2(401.0f, 103.0f));
            IM_CHECK_EQ(size, ImVec2(475.0f, 100.0f));
            ImGui::End();
        }
        if (ctx->FrameCount == 2)
            ctx->Finish();
    };

    // ## Test window size constraints.
    t = IM_REGISTER_TEST(e, "window", "window_size_constraints");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto constraints = [](ImGuiSizeCallbackData* data)
        {
            IM_ASSERT(data != NULL);
            ImGuiTestContext* ctx = (ImGuiTestContext*)data->UserData;
            if (ctx->IsFirstGuiFrame())
            {
                ImGuiWindow* window = ImGui::GetCurrentWindow();
                IM_CHECK_EQ(data->Pos, window->Pos);
                IM_CHECK_EQ(data->CurrentSize, window->SizeFull);
            }
            data->DesiredSize.x = ImMax(data->DesiredSize.x, data->DesiredSize.y);
            data->DesiredSize.y = ImMax(data->DesiredSize.x, data->DesiredSize.y);
        };
        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(500, 500), constraints, ctx);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("Lorem ipsum dolor sit amet");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window = ctx->GetWindowByRef("Test Window");
        for (int i = 0; i < 2; i++)
        {
            ctx->WindowResize("Test Window", ImVec2(200.0f + i * 100.0f, 100.0f));
            IM_CHECK_EQ(window->SizeFull.x, 200.0f + i * 100.0f);
            IM_CHECK_EQ(window->SizeFull.y, 200.0f + i * 100.0f);
        }
    };

    // ## Test basic window auto resize
    t = IM_REGISTER_TEST(e, "window", "window_size_auto_basic");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // FIXME-TESTS: Ideally we'd like a variant with/without the if (Begin) here
        // FIXME: BeginChild() auto-resizes horizontally, this width is calculated by using data from previous frame.
        //  If window was wider on previous frame child would permanently assume new width and that creates a feedback
        //  loop keeping new size.
        ImGui::SetNextWindowSize(ImVec2(1, 1), ImGuiCond_Appearing); // Fixes test failing due to side-effects caused by other tests using window with same name.
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
    t = IM_REGISTER_TEST(e, "window", "window_size_auto_uncollapse");
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
        ctx->SetRef("Test Window");
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
    t = IM_REGISTER_TEST(e, "window", "window_popup_focus");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Popups & Modal windows");
        ctx->ItemOpen("Popups");
        ctx->ItemClick("Popups/Toggle..");

        ImGuiWindow* popup_1 = g.NavWindow;
        ctx->SetRef(popup_1);
        ctx->ItemClick("Stacked Popup");
        IM_CHECK(popup_1->WasActive);

        ImGuiWindow* popup_2 = g.NavWindow;
        ctx->MouseMove("Bream", ImGuiTestOpFlags_NoFocusWindow | ImGuiTestOpFlags_NoCheckHoveredId);
        ctx->MouseClick(1); // Close with right-click
        IM_CHECK(popup_1->WasActive);
        IM_CHECK(!popup_2->WasActive);
        IM_CHECK(g.NavWindow == popup_1);
    };

    // ## Test an edge case of calling CloseCurrentPopup() after clicking it (#2880)
    // Previously we would mistakenly handle the click on EndFrame() while popup is already marked as closed, doing a double close
    t = IM_REGISTER_TEST(e, "window", "window_popup_focus2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Stacked Modal Popups");
        if (ImGui::Button("Open Modal Popup 1"))
            ImGui::OpenPopup("Popup1");
        if (ImGui::BeginPopupModal("Popup1", NULL, ImGuiWindowFlags_NoSavedSettings))
        {
            if (ImGui::Button("Open Modal Popup 2"))
                ImGui::OpenPopup("Popup2");
            ImGui::SetNextWindowSize(ImVec2(100, 100));
            if (ImGui::BeginPopupModal("Popup2", NULL, ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::Text("Click anywhere");
                if (ImGui::IsMouseClicked(0))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Stacked Modal Popups");
        ctx->ItemClick("Open Modal Popup 1");
        IM_CHECK_EQ(g.NavWindow->ID, ctx->GetID("/Popup1"));
        IM_CHECK_EQ(g.OpenPopupStack.Size, 1);
        ctx->SetRef("Popup1");
        ctx->ItemClick("Open Modal Popup 2");
        IM_CHECK_EQ(g.NavWindow->ID, ctx->GetID("/Popup2"));
        IM_CHECK_EQ(g.OpenPopupStack.Size, 2);
        ctx->MouseMoveToPos(g.NavWindow->Rect().GetCenter());
        ctx->MouseClick(0);
        IM_CHECK_EQ(g.OpenPopupStack.Size, 1);
        IM_CHECK_EQ(g.NavWindow->ID, ctx->GetID("/Popup1"));
    };

    // ## Test closing current popup
    t = IM_REGISTER_TEST(e, "window", "window_popup_close_current");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        //ImGui::SetNextWindowSize(ImVec2(0, 0)); // FIXME: break with io.ConfigDockingAlwaysTabBar
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
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Popups");
        IM_CHECK(g.OpenPopupStack.Size == 0);
        ctx->MenuClick("Menu");
        IM_CHECK(g.OpenPopupStack.Size == 1);
        ctx->MenuClick("Menu/Submenu");
        IM_CHECK(g.OpenPopupStack.Size == 2);
        ctx->MenuClick("Menu/Submenu/Close");
        IM_CHECK(g.OpenPopupStack.Size == 0);
    };

    // ## Test BeginPopupContextVoid()
    t = IM_REGISTER_TEST(e, "window", "window_popup_on_void");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ctx->GenericVars.Bool1 = false;
        if (ImGui::BeginPopupContextVoid("testpopup", ctx->GenericVars.Int1))
        {
            ctx->GenericVars.Bool1 = true;
            ImGui::Selectable("Close");
            ImGui::EndPopup();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->GenericVars.Int1 = 1;
        ctx->MouseClickOnVoid(1);
        IM_CHECK(ctx->GenericVars.Bool1 == true);
        ctx->MouseClickOnVoid(0);
        IM_CHECK(ctx->GenericVars.Bool1 == false);

        ctx->GenericVars.Int1 = 0;
        ctx->MouseClickOnVoid(0);
        IM_CHECK(ctx->GenericVars.Bool1 == true);
        ctx->MouseClickOnVoid(1);
        IM_CHECK(ctx->GenericVars.Bool1 == false);
    };

    // ## Test opening a popup after BeginPopup(), while window is out of focus. (#4308)
    t = IM_REGISTER_TEST(e, "window", "window_popup_open_after");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginPopup("Popup"))
        {
            ImGui::Text("...");
            ImGui::EndPopup();
        }
        ImGui::Button("Open");
        ImGui::OpenPopupOnItemClick("Popup", ImGuiPopupFlags_MouseButtonRight);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Window");
        ctx->MouseClickOnVoid();                                                        // Ensure no window is focused.
        ctx->ItemClick("Open", ImGuiMouseButton_Right, ImGuiTestOpFlags_NoFocusWindow); // Open popup without focusing window.
        IM_CHECK(g.OpenPopupStack.Size == 1);
        Str30f popup_name("##Popup_%08x", ctx->GetID("Popup"));
        IM_CHECK_STR_EQ(g.OpenPopupStack[0].Window->Name, popup_name.c_str());
    };

    // ## Test menus in a popup window (PR #3496).
#if IMGUI_BROKEN_TESTS
    t = IM_REGISTER_TEST(e, "window", "window_popup_menu");
    struct WindowPopupMenuTestVars { bool FirstOpen = false; bool SecondOpen = false; };
    t->SetUserDataType<WindowPopupMenuTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetUserData<WindowPopupMenuTestVars>();
        vars.FirstOpen = vars.SecondOpen = false;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ctx->IsFirstGuiFrame())
            ImGui::OpenPopup("Menu Popup");
        if (ImGui::BeginPopupModal("Menu Popup", NULL, ImGuiWindowFlags_MenuBar))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("First"))
                {
                    vars.FirstOpen = true;
                    ImGui::MenuItem("Lorem");
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Second"))
                {
                    vars.SecondOpen = true;
                    ImGui::MenuItem("Ipsum");
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            if (ImGui::IsKeyPressedMap(ImGuiKey_Escape))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window = ctx->GetWindowByRef("Menu Popup");
        auto& vars = ctx->GetUserData<WindowPopupMenuTestVars>();
        ctx->SetRef(window->Name);
        IM_CHECK_EQ(vars.FirstOpen, false);                                     // Nothing is open.
        IM_CHECK_EQ(vars.SecondOpen, false);
        ctx->ItemClick("##menubar/First");                                      // Click and open first menu.
        IM_CHECK_EQ(vars.FirstOpen, true);
        IM_CHECK_EQ(vars.SecondOpen, false);
        ctx->MouseMove("##menubar/Second", ImGuiTestOpFlags_NoFocusWindow);     // Hover and open second menu.
        IM_CHECK_EQ(vars.FirstOpen, false);
        IM_CHECK_EQ(vars.SecondOpen, true);
        ctx->MouseMove("##menubar/First", ImGuiTestOpFlags_NoFocusWindow);      // Hover and open first menu again.
        IM_CHECK_EQ(vars.FirstOpen, true);
        IM_CHECK_EQ(vars.SecondOpen, false);
        ctx->MouseMoveToPos(window->Pos + ImVec2(10.0f, 10.0f));                // Clicking window outside of menu closes it.
        ctx->MouseClick();
        ctx->ItemClick("##menubar/First");                                      // Click and reopen first menu.
        ctx->MouseClickOnVoid();                                                // Clicking outside of menu closes it.
        IM_CHECK_EQ(vars.FirstOpen, false);
        IM_CHECK_EQ(vars.SecondOpen, false);
    };
#endif

    // ## Test behavior of io.WantCaptureMouse and io.WantCaptureMouseUnlessPopupClose with popups. (#4480)
#if IMGUI_VERSION_NUM >= 18410
    t = IM_REGISTER_TEST(e, "window", "window_popup_want_capture");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Open Popup"))
            ImGui::OpenPopup("Popup");
        if (ImGui::Button("Open Modal"))
            ImGui::OpenPopup("Modal");
        if (ImGui::BeginPopup("Popup"))
        {
            ImGui::TextUnformatted("...");
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("Modal"))
        {
            ImGui::TextUnformatted("...");
            ImGui::EndPopup();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiIO& io = g.IO;
        ctx->SetRef("Test Window");
        ctx->ItemClick("Open Popup");
        IM_CHECK(g.NavWindow != NULL);
        ctx->MouseMoveToPos(g.NavWindow->Pos + ImVec2(10, 10));
        IM_CHECK(io.WantCaptureMouse == true);
        IM_CHECK(io.WantCaptureMouseUnlessPopupClose == true);
        ctx->MouseMoveToPos(ctx->GetVoidPos());
        IM_CHECK(io.WantCaptureMouse == true);
        IM_CHECK(io.WantCaptureMouseUnlessPopupClose == false);
        ctx->PopupCloseAll();

        ctx->ItemClick("Open Modal");
        IM_CHECK(g.NavWindow != NULL);
        ctx->MouseMoveToPos(g.NavWindow->Pos + ImVec2(10, 10));
        IM_CHECK(io.WantCaptureMouse == true);
        IM_CHECK(io.WantCaptureMouseUnlessPopupClose == true);
        ctx->MouseMoveToPos(ctx->GetVoidPos());
        IM_CHECK(g.HoveredWindow == NULL);
        IM_CHECK(io.WantCaptureMouse == true);
        IM_CHECK(io.WantCaptureMouseUnlessPopupClose == true);

        // Move to button behind, verify it is inhibited
        ctx->MouseMove("/Test Window/Open Modal", ImGuiTestOpFlags_NoFocusWindow | ImGuiTestOpFlags_NoCheckHoveredId);
        IM_CHECK(g.HoveredWindow == NULL);
        IM_CHECK(io.WantCaptureMouse == true);
        IM_CHECK(io.WantCaptureMouseUnlessPopupClose == true);

        ctx->PopupCloseAll();
    };
#endif

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
        {
            IM_CHECK_EQ(ctx->UiContext->CurrentWindow->ContentSize, ImVec2(100, 100 + ImGui::GetTextLineHeightWithSpacing()));
            ctx->Finish();
        }
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

    // ## Test that SetScrollHereXX functions make items fully visible
    // Regardless of WindowPadding and ItemSpacing values
#if IMGUI_VERSION_NUM >= 18202
    t = IM_REGISTER_TEST(e, "window", "window_scroll_tracking");
    struct ScrollTestVars { int Variant = 0; bool FullyVisible[30][30] = {}; int TrackX = 0; int TrackY = 0; };
    t->SetUserDataType<ScrollTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetUserData<ScrollTestVars>();

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::Button("Track 0,0")) { vars.TrackX = 0; vars.TrackY = 0; }
        if (ImGui::Button("Track 29,0")) { vars.TrackX = 29; vars.TrackY = 0; }
        if (ImGui::Button("Track 0,29")) { vars.TrackX = 0; vars.TrackY = 29; }
        if (ImGui::Button("Track 29,29")) { vars.TrackX = 29; vars.TrackY = 29; }

        if (vars.Variant & 1)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1, 1));
        if (vars.Variant & 2)
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 1));
        ImGui::BeginChild("Scrolling", ImVec2(500, 500), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);

        for (int y = 0; y < 30; y++)
        {
            for (int x = 0; x < 30; x++)
            {
                if (x > 0)
                    ImGui::SameLine();
                ImGui::Button(Str30f("%d Item %d,%d###%d,%d", vars.FullyVisible[y][x], y, x, y, x).c_str());
                if (vars.TrackX == x)
                    ImGui::SetScrollHereX(vars.TrackX / 29.0f);
                if (vars.TrackY == y)
                    ImGui::SetScrollHereY(vars.TrackY / 29.0f);

                ImRect clip_rect = ImGui::GetCurrentWindow()->ClipRect;
                ImRect item_rect = ImGui::GetCurrentContext()->LastItemData.Rect;
                vars.FullyVisible[y][x] = clip_rect.Contains(item_rect);
            }
        }
        vars.TrackX = vars.TrackY = -1;

        ImGui::EndChild();
        if (vars.Variant & 1)
            ImGui::PopStyleVar();
        if (vars.Variant & 2)
            ImGui::PopStyleVar();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetUserData<ScrollTestVars>();
        ctx->SetRef("Test Window");
        ImGuiWindow* window = ctx->GetWindowByRef("");

        for (int variant = 0; variant < 4; variant++)
        {
            vars.Variant = variant;
            ctx->Yield();

            ctx->ItemClick("Track 29,29");
            IM_CHECK_EQ(vars.FullyVisible[29][29], true);
            IM_CHECK_EQ(vars.FullyVisible[0][0], false);
            IM_CHECK_EQ(window->Scroll.x, window->ScrollMax.x);
            IM_CHECK_EQ(window->Scroll.y, window->ScrollMax.y);

            ctx->ItemClick("Track 0,29");
            IM_CHECK_EQ(vars.FullyVisible[29][0], true);
            IM_CHECK_EQ(window->Scroll.x, 0.0f);
            IM_CHECK_EQ(window->Scroll.y, window->ScrollMax.y);

            ctx->ItemClick("Track 29,0");
            IM_CHECK_EQ(vars.FullyVisible[0][29], true);
            IM_CHECK_EQ(window->Scroll.y, 0.0f);

            ctx->ItemClick("Track 0,0");
            IM_CHECK_EQ(vars.FullyVisible[0][0], true);
            IM_CHECK_EQ(vars.FullyVisible[29][29], false);
            IM_CHECK_EQ(window->Scroll.x, 0.0f);
            IM_CHECK_EQ(window->Scroll.y, 0.0f);
        }
    };
#endif

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
        if (ctx->FrameCount >= ctx->FirstTestFrameCount)
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

    // ## Test window scrolling using mouse wheel.
    t = IM_REGISTER_TEST(e, "window", "window_scroll_with_wheel");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(100, 100), ImGuiCond_Always);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::SetCursorPos(ImVec2(200, 200));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window = ctx->GetWindowByRef("Test Window");
        ImGui::SetScrollX(window, 0);
        ImGui::SetScrollY(window, 0);
        ctx->Yield();

        ctx->MouseMoveToPos(window->Rect().GetCenter());
        IM_CHECK_EQ(window->Scroll.x, 0.0f);
        IM_CHECK_EQ(window->Scroll.y, 0.0f);
        ctx->MouseWheel(-5.0f);                                     // Scroll down
        IM_CHECK_EQ(window->Scroll.x, 0.0f);
        IM_CHECK_GT(window->Scroll.y, 0.0f);
        ctx->MouseWheel(5.0f);                                      // Scroll up
        IM_CHECK_EQ(window->Scroll.x, 0.0f);
        IM_CHECK_EQ(window->Scroll.y, 0.0f);

        for (int n = 0; n < 2; n++)
        {
            if (n == 0)
            {
                ctx->MouseWheel(0.0f, -5.0f);                       // Scroll right (horizontal scroll)
            }
            else
            {
                ctx->KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
                ctx->MouseWheel(-5.0f);                             // Scroll right (shift + vertical scroll)
                ctx->KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
            }
            IM_CHECK_GT(window->Scroll.x, 0.0f);
            IM_CHECK_EQ(window->Scroll.y, 0.0f);
            if (n == 0)
            {
                ctx->MouseWheel(0.0f, 5.0f);                        // Scroll left (horizontal scroll)
            }
            else
            {
                ctx->KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
                ctx->MouseWheel(5.0f);                              // Scroll left (shift + vertical scroll)
                ctx->KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
            }
            IM_CHECK_EQ(window->Scroll.x, 0.0f);
            IM_CHECK_EQ(window->Scroll.y, 0.0f);
        }
    };

    // ## Test window scrolling using mouse wheel over a child window.
    t = IM_REGISTER_TEST(e, "window", "window_scroll_over_child");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200.0f, 200.0f), ImGuiCond_Always);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::SetCursorPosX(ImGui::GetCurrentWindow()->Size.y * 0.5f);
        ImGui::Button("Top");
        ImGui::Button("Left");
        ImGui::SameLine();
        ImGui::BeginChild("Child", ImVec2(100.0f, 100.0f), true, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::SetCursorPos(ImVec2(120.0f, 120.0f));
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(80.0f, 120.0f + 80.0f));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window = ctx->GetWindowByRef("Test Window");
        ImGuiWindow* child = ctx->GetWindowByRef(ctx->GetChildWindowID(window->Name, "Child"));
        ImGuiContext& g = *ctx->UiContext;
        ImGui::SetScrollX(window, 0);
        ImGui::SetScrollY(window, 0);
        ImGui::SetScrollX(child, 0);
        ImGui::SetScrollY(child, 0);
        ctx->Yield();

        ctx->SetRef("Test Window");
        IM_CHECK_EQ(window->Scroll.y, 0.0f);
        IM_CHECK_EQ(child->Scroll.y, 0.0f);

        // Test vertical scrolling. Scroll operation goes over child window but does not scroll it.
        ctx->MouseMove("Top");
        IM_CHECK_EQ(g.HoveredWindow, window);                   // Not hovering child initially
        ctx->MouseWheel(-3.0f);
        ctx->Yield();                                           // FIXME-TESTS: g.HoveredWindow needs extra frame to update.
        IM_CHECK_GT(window->Scroll.y, 0.0f);                    // Main window was scrolled
        IM_CHECK_EQ(child->Scroll.y, 0.0f);                     // Child window was not scrolled
        IM_CHECK_EQ(g.HoveredWindow, child);                    // Scroll operation happened over child

        // Pause for a while and perform scroll operation when cursor is hovering child. Now child is scrolled.
        float prev_scroll_y = window->Scroll.y;
        ctx->SleepNoSkip(2.0f, 1.0f / 2.0f);
        ctx->MouseWheel(-3.0f);
        IM_CHECK_EQ(window->Scroll.y, prev_scroll_y);
        IM_CHECK_GT(child->Scroll.y, 0.0f);
        IM_CHECK_EQ(g.HoveredWindow, child);                    // Still hovering child

        // Test horizontal scrolling. With horizontal mouse wheel input or vertical mouse wheel input + shift.
        ImGui::SetScrollY(window, 0);
        ImGui::SetScrollY(child, 0);

        for (int n = 0; n < 2; n++)
        {
            ImGui::SetScrollX(window, 0);
            ImGui::SetScrollX(child, 0);
            ctx->Yield();
            IM_CHECK_EQ(window->Scroll.x, 0.0f);
            IM_CHECK_EQ(child->Scroll.x, 0.0f);

            // Scroll operation goes over child window but does not scroll it.
            ctx->MouseMoveToPos(child->Pos);                    // FIXME-TESTS: Lack of mouse movement makes child window take scroll input as if mouse hovered it.
            ctx->MouseMove("Left");
            IM_CHECK_EQ(g.HoveredWindow, window);               // Not hovering child initially
            if (n == 0)
            {
                // Wheel (horizontal)
                ctx->MouseWheel(0, -3.0f);
                ctx->Yield();                                   // FIXME-TESTS: g.HoveredWindow needs extra frame to update.
            }
            else
            {
                // Wheel (vertical) + shift
                ctx->KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
                ctx->MouseWheel(-3.0f);
                ctx->KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
            }
            float prev_scroll_x = window->Scroll.x;
            IM_CHECK_GT(window->Scroll.x, 0.0f);                // Main window was scrolled
            IM_CHECK_EQ(child->Scroll.x, 0.0f);                 // Child window was not scrolled
            IM_CHECK_EQ(g.HoveredWindow, child);                // Scroll operation happened over child

            // Pause for a while and perform scroll operation when cursor is hovering child. Now child is scrolled.
            ctx->SleepNoSkip(2.0f, 1.0f / 2.0f);
            if (n == 0)
            {
                ctx->MouseWheel(0, -3.0f);
            }
            else
            {
                ctx->KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
                ctx->MouseWheel(-3.0f);
                ctx->KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
                ctx->Yield();
            }
            IM_CHECK_EQ(window->Scroll.x, prev_scroll_x);
            IM_CHECK_GT(child->Scroll.x, 0.0f);
            IM_CHECK_EQ(g.HoveredWindow, child);                // Still hovering child
        }
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
        ImVec2 viewport_pos = ctx->GetMainViewportPos();
        ImGuiWindow* window = ctx->GetWindowByRef("Movable Window");
        ctx->WindowMove("Movable Window", viewport_pos + ImVec2(0, 0));
#ifdef IMGUI_HAS_VIEWPORT
        ImGuiIO& io = ImGui::GetIO();
        const bool has_mouse_hovered_viewports = (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport) != 0;
        if (has_mouse_hovered_viewports)
            IM_CHECK(window->Viewport->ID == io.MouseHoveredViewport);
#endif
        IM_CHECK(window->Pos == viewport_pos + ImVec2(0, 0));
        ctx->WindowMove("Movable Window", viewport_pos + ImVec2(100, 0));
#ifdef IMGUI_HAS_VIEWPORT
        if (has_mouse_hovered_viewports)
            IM_CHECK(window->Viewport->ID == io.MouseHoveredViewport);
#endif
        IM_CHECK(window->Pos == viewport_pos + ImVec2(100, 0));
        ctx->WindowMove("Movable Window", viewport_pos + ImVec2(50, 100));
#ifdef IMGUI_HAS_VIEWPORT
        if (has_mouse_hovered_viewports)
            IM_CHECK(window->Viewport->ID == io.MouseHoveredViewport);
#endif
        IM_CHECK(window->Pos == viewport_pos + ImVec2(50, 100));
    };

    // ## Test explicit window positioning
    t = IM_REGISTER_TEST(e, "window", "window_pos_pivot");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::SetNextWindowPos(vars.Pos, ImGuiCond_Always, vars.Pivot);
        ImGui::Begin("Movable Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("Lorem ipsum dolor sit amet");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGuiWindow* window = ctx->GetWindowByRef("Movable Window");
        for (int n = 0; n < 4; n++)     // Test all pivot combinations.
            for (int c = 0; c < 2; c++) // Test collapsed and uncollapsed windows.
            {
                ctx->WindowCollapse(window, c != 0);
                vars.Pos = ctx->GetMainViewportPos() + window->Size; // Ensure window is tested within a visible viewport.
                vars.Pivot = ImVec2(1.0f * ((n & 1) != 0 ? 1 : 0), 1.0f * ((n & 2) != 0 ? 1 : 0));
                ctx->Yield();
                IM_CHECK_EQ(window->Pos, vars.Pos - (window->Size * vars.Pivot));
            }
    };

    // ## Test window resizing from edges and corners.
    t = IM_REGISTER_TEST(e, "window", "window_resizing");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
        ImGui::TextUnformatted("Lorem ipsum dolor sit amet");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        struct TestData
        {
            ImVec2 grab_pos;        // window->Pos + window->Size * grab_pos
            ImVec2 drag_dir;
            ImVec2 expect_pos;
            ImVec2 expect_size;
        };

        const TestData test_datas[] =
        {
            { ImVec2(0.5,   0), ImVec2(  0, -10), ImVec2(100,  90), ImVec2(200, 60) },    // From top edge go up
            { ImVec2(  1,   0), ImVec2( 10, -10), ImVec2(110,  90), ImVec2(200, 50) },    // From top-right corner, no resize, window is moved
            { ImVec2(  1, 0.5), ImVec2( 10,   0), ImVec2(100, 100), ImVec2(210, 50) },    // From right edge go right
            { ImVec2(  1,   1), ImVec2( 10,  10), ImVec2(100, 100), ImVec2(210, 60) },    // From bottom-right corner go right-down
            { ImVec2(0.5,   1), ImVec2(  0,  10), ImVec2(100, 100), ImVec2(200, 60) },    // From bottom edge go down
            { ImVec2(  0,   1), ImVec2(-10,  10), ImVec2( 90, 100), ImVec2(210, 60) },    // From bottom-left corner go left-down
            { ImVec2(  0, 0.5), ImVec2(-10,   0), ImVec2( 90, 100), ImVec2(210, 50) },    // From left edge go left
            { ImVec2(  0,   0), ImVec2(-10, -10), ImVec2( 90,  90), ImVec2(200, 50) },    // From left-top edge, no resize, window is moved
        };

        ImGuiWindow* window = ctx->GetWindowByRef("Test Window");
        for (auto& test_data : test_datas)
        {
            ImGui::SetWindowPos(window, ImVec2(100, 100));
            ImGui::SetWindowSize(window, ImVec2(200, 50));
            ctx->Yield();
            ctx->MouseMoveToPos(window->Pos + ((window->Size - ImVec2(1.0f, 1.0f)) * test_data.grab_pos));
            ctx->MouseDragWithDelta(test_data.drag_dir);
            IM_CHECK_EQ(window->Size, test_data.expect_size);
            IM_CHECK_EQ(window->Pos, test_data.expect_pos);
        }
    };

    // ## Test animated window title.
    // ## Use ImGuiWindowFlags_UnsavedDocument for dumb coverage purpose only.
    t = IM_REGISTER_TEST(e, "window", "window_title_animation");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin(Str30f("Frame %d###Test Window", ImGui::GetFrameCount()).c_str(), NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_UnsavedDocument);
        ImGui::TextUnformatted("Lorem ipsum dolor sit amet");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiWindow* window = ctx->GetWindowByRef("###Test Window");

        // Open window switcher (CTRL+TAB).
        ctx->KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Ctrl); // Hold CTRL down
        ctx->KeyPressMap(ImGuiKey_Tab, 0);
        ctx->SleepNoSkip(0.3f, 1.0f / 60.0f);
        for (int i = 0; i < 2; i++)
        {
            Str30f window_name("Frame %d###Test Window", g.FrameCount);
            IM_CHECK_STR_EQ_NO_RET(window->Name, window_name.c_str());    // Verify window->Name gets updated.
            ctx->Yield();
        }
        ctx->KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Ctrl);
    };

    // ## Test window appearing state.
    t = IM_REGISTER_TEST(e, "window", "window_appearing");
    struct WindowAppearingVars { enum TestState { Hidden, ShowWindow, ShowWindowSetSize, ShowWindowAutoSize, ShowTooltip, ShowPopup, ShowMax_ } State = Hidden; int ShowFrame = -1, AppearingFrame = -1, AppearingCount = 0; };
    t->SetUserDataType<WindowAppearingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        WindowAppearingVars& vars = ctx->GetUserData<WindowAppearingVars>();
        if (vars.State == WindowAppearingVars::Hidden)
            return;

        switch (vars.State)
        {
        case WindowAppearingVars::ShowWindow:
            ImGui::Begin("Appearing Window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
            break;
        case WindowAppearingVars::ShowWindowSetSize:
            ImGui::SetNextWindowSize(ImVec2(100, 100));
            ImGui::Begin("Appearing Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
            break;
        case WindowAppearingVars::ShowWindowAutoSize:
            ImGui::Begin("Appearing Window 3", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
            break;
        case WindowAppearingVars::ShowTooltip:
            ImGui::BeginTooltip();
            break;
        case WindowAppearingVars::ShowPopup:
            if (vars.ShowFrame == -1)
                ImGui::OpenPopup("##popup");
            ImGui::BeginPopup("##popup");
            break;
        default:
            IM_ASSERT(0);
            break;
        }

        ImGui::Text("Some text");
        if (vars.ShowFrame == -1)
            vars.ShowFrame = ctx->FrameCount;
        if (ImGui::IsWindowAppearing())
        {
            vars.AppearingCount++;
            vars.AppearingFrame = ctx->FrameCount;
        }

        switch (vars.State)
        {
        case WindowAppearingVars::ShowWindow:
        case WindowAppearingVars::ShowWindowSetSize:
        case WindowAppearingVars::ShowWindowAutoSize:
            ImGui::End();
            break;
        case WindowAppearingVars::ShowTooltip:
            ImGui::EndTooltip();
            break;
        case WindowAppearingVars::ShowPopup:
            ImGui::EndPopup();
            break;
        default:
            IM_ASSERT(0);
            break;
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        WindowAppearingVars& vars = ctx->GetUserData<WindowAppearingVars>();

        // FIXME-TESTS: Test using a viewport window.
        const char* variant_names[] = { NULL, "Window", "WindowSetSize", "WindowAutoSize", "Tooltip", "Popup" };
        for (int variant = WindowAppearingVars::Hidden + 1; variant < WindowAppearingVars::ShowMax_; variant++)
        {
            ctx->LogDebug("Test variant: %s", variant_names[variant]);
            vars.State = (WindowAppearingVars::TestState)variant;
            vars.ShowFrame = vars.AppearingFrame = -1;
            vars.AppearingCount = 0;
            ctx->Yield();  // Verify that IsWindowAppearing() only triggers on very first frame.
            IM_CHECK_EQ(vars.AppearingCount, 1);
            IM_CHECK_NE(vars.ShowFrame, -1);
            IM_CHECK_EQ(vars.ShowFrame, vars.AppearingFrame);
            ctx->Yield(5);
            IM_CHECK_EQ(vars.AppearingCount, 1);
            IM_CHECK_NE(vars.ShowFrame, -1);
            IM_CHECK_EQ(vars.ShowFrame, vars.AppearingFrame);
            vars.State = WindowAppearingVars::Hidden;
            ctx->Yield();
        }
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
                    IM_CHECK_EQ_NO_RET(window->DC.CursorMaxPos.y, g.LastItemData.Rect.Max.y);

                const float current_height = g.LastItemData.Rect.Max.y - y;
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

    t = IM_REGISTER_TEST(e, "layout", "layout_cursor_max");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        IM_CHECK_EQ(window->DC.CursorMaxPos.x, window->DC.CursorStartPos.x);
        IM_CHECK_EQ(window->DC.CursorMaxPos.y, window->DC.CursorStartPos.y);

        ImGui::SetCursorPos(ImVec2(200.0f, 100.0f));
        IM_CHECK_EQ(window->DC.CursorMaxPos.x, window->Pos.x + 200.0f);
        IM_CHECK_EQ(window->DC.CursorMaxPos.y, window->Pos.y + 100.0f);

        ImGui::NewLine();
        IM_CHECK_EQ(window->DC.CursorMaxPos.x, window->Pos.x + 200.0f);
        IM_CHECK_EQ(window->DC.CursorMaxPos.y, window->Pos.y + 100.0f + ImGui::GetTextLineHeight());

        ImGui::End();
    };
}

//-------------------------------------------------------------------------
// Tests: Draw, ImDrawList
//-------------------------------------------------------------------------

struct HelpersTextureId
{
    // Fake texture ID
    ImTextureID FakeTex0 = (ImTextureID)100;
    ImTextureID FakeTex1 = (ImTextureID)200;

    // Replace fake texture IDs with a known good ID in order to prevent graphics API crashing application.
    void RemoveFakeTexFromDrawList(ImDrawList* draw_list, ImTextureID replacement_tex_id)
    {
        for (ImDrawCmd& cmd : draw_list->CmdBuffer)
            if (cmd.TextureId == FakeTex0 || cmd.TextureId == FakeTex1)
                cmd.TextureId = replacement_tex_id;
    }
};

static bool CanTestVtxOffset(ImGuiTestContext* ctx)
{
    if ((ctx->UiContext->IO.BackendFlags & ImGuiBackendFlags_RendererHasVtxOffset) == 0)
    {
        ctx->LogInfo("Skipping: backend does not support RendererHasVtxOffset!");
        return false;
    }
    if (sizeof(ImDrawIdx) != 2)
    {
        ctx->LogInfo("sizeof(ImDrawIdx) != 2");
        return false;
    }
    return true;
}

void RegisterTests_DrawList(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test AddCallback()
    t = IM_REGISTER_TEST(e, "drawlist", "drawlist_callbacks");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowScroll(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(100.0f, 100.0f));
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

        HelpersTextureId fake_tex;

        draw_list->ChannelsSplit(2);
        draw_list->ChannelsSetCurrent(0);
        draw_list->AddImage(fake_tex.FakeTex0, p, p + ImVec2(100, 100));
        draw_list->ChannelsSetCurrent(1);
        draw_list->AddImage(fake_tex.FakeTex1, p + ImVec2(110, 0), p + ImVec2(210, 100));
        draw_list->ChannelsMerge();

        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.Size, start_cmdbuffer_size + 2);
        IM_CHECK_EQ_NO_RET(draw_list->CmdBuffer.back().ElemCount, 0u);
        IM_CHECK_NO_RET(prev_texture_id == draw_list->CmdBuffer.back().TextureId);

        fake_tex.RemoveFakeTexFromDrawList(draw_list, prev_texture_id);

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

        ImGui::PushColumnsBackground(); // In channel 0
        // 2020/06/13: we don't request of PushColumnsBackground() to merge the two same commands (one created by loose AddDrawCmd())!
        //IM_CHECK_EQ(draw_list->CmdBuffer.Size, start_cmdbuffer_size);
        ImGui::PopColumnsBackground();

        IM_CHECK_EQ(draw_list->CmdBuffer.Size, 1); // In channel 2

        ImGui::Text("Two Two"); // FIXME-TESTS: Make sure this is visible
        IM_CHECK_EQ(ImGui::IsItemVisible(), true);

        ImGui::Columns(1);
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, start_cmdbuffer_size + 1 + 2); // Channel 1 and 2 each other one Cmd

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

    // ## Test DebugRecoverFromErrors() FIXME-TESTS
    t = IM_REGISTER_TEST(e, "misc", "misc_recover");
    t->Flags |= ImGuiTestFlags_NoRecoverWarnings;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::BeginChild("Child");
        ImGui::PushFocusScope(ImGui::GetID("focusscope1"));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4());
        ImGui::PushID("hello");
        ImGui::BeginGroup();
        ImGui::SetNextItemOpen(true);
        ImGui::TreeNode("node");
        ImGui::BeginTabBar("tabbar");
        ImGui::BeginTable("table", 4);
        ctx->Finish();
    };

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

        ctx->UiContext->IO.ConfigMemoryCompactTimer = 0.0f;
        ctx->Yield(3); // Give time to perform GC
        ctx->LogDebug("Check GC-ed state");
        for (int i = 0; i < 3; i++)
        {
            ImGuiWindow* window = ctx->GetWindowByRef(Str16f("GC Test %d", i).c_str());
            IM_CHECK(window->MemoryCompacted);
            IM_CHECK(window->IDStack.empty());
            IM_CHECK(window->DrawList->CmdBuffer.empty());
        }

        // Perform an additional fuller GC
        ImGuiContext& g = *ctx->UiContext;
        g.GcCompactAll = true;
        ctx->Yield();
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
        IM_CHECK_EQ(ImHashDecoratedPath("/Hello", NULL, 42), ImHashDecoratedPath("Hello"));        // Leading / clears seed
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
        int max_size = v.max_size();
        IM_CHECK(max_size == INT_MAX / sizeof(int));
    };

    // ## Test ImVector functions
#ifdef IMGUI_HAS_TABLE
    t = IM_REGISTER_TEST(e, "misc", "misc_bitarray");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImBitArray<128> v128;
        IM_CHECK_EQ((int)sizeof(v128), 16);
        v128.ClearAllBits();
        v128.SetBitRange(1, 2);
        IM_CHECK(v128.Storage[0] == 0x00000002 && v128.Storage[1] == 0x00000000 && v128.Storage[2] == 0x00000000);
        v128.ClearAllBits();
        v128.SetBitRange(1, 32);
        IM_CHECK(v128.Storage[0] == 0xFFFFFFFE && v128.Storage[1] == 0x00000000 && v128.Storage[2] == 0x00000000);
        v128.ClearAllBits();
        v128.SetBitRange(1, 33);
        IM_CHECK(v128.Storage[0] == 0xFFFFFFFE && v128.Storage[1] == 0x00000001 && v128.Storage[2] == 0x00000000);
        v128.ClearAllBits();
        v128.SetBitRange(2, 33);
        IM_CHECK(v128.Storage[0] == 0xFFFFFFFC && v128.Storage[1] == 0x00000001 && v128.Storage[2] == 0x00000000);
        v128.ClearAllBits();
        v128.SetBitRange(0, 65);
        IM_CHECK(v128.Storage[0] == 0xFFFFFFFF && v128.Storage[1] == 0xFFFFFFFF && v128.Storage[2] == 0x00000001);

        ImBitArray<129> v129;
        v129.ClearAllBits();
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
        IM_CHECK(pool.GetMapSize() == 3);
        pool.Remove(0x22, pool.GetByKey(0x22));
        IM_CHECK(pool.GetByKey(0x22) == NULL);
        IM_CHECK(pool.GetMapSize() == 3);
        ImGuiTabBar* t4 = pool.GetOrAddByKey(0x40);
        IM_CHECK(pool.GetIndex(t4) == 1);
        IM_CHECK(pool.GetBufSize() == 3);
        IM_CHECK(pool.GetMapSize() == 4);
        pool.Clear();
        IM_CHECK(pool.GetMapSize() == 0);
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

    // ## Test ImGuiListClipper basic behavior
    // ## Test ImGuiListClipper with table frozen rows
    t = IM_REGISTER_TEST(e, "misc", "misc_clipper");
    struct ClipperTestVars { ImGuiWindow* WindowOut = NULL; float WindowHeightInItems = 10.0f; int ItemsIn = 100; int ItemsOut = 0; float OffsetY = 0.0f; ImBitVector ItemsOutMask; bool ClipperManualItemHeight = true; bool TableEnable = false; int TableFreezeRows = 0; };
    t->SetUserDataType<ClipperTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetUserData<ClipperTestVars>();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Simpler to use remove padding and decoration
        ImGui::SetNextWindowSize(ImVec2(300, ImGui::GetTextLineHeightWithSpacing() * vars.WindowHeightInItems));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);

        bool open = true;
#ifdef IMGUI_HAS_TABLE
        if (vars.TableEnable)
        {
            open = ImGui::BeginTable("table", 2, ImGuiTableFlags_ScrollY);
            if (open)
                ImGui::TableSetupScrollFreeze(0, vars.TableFreezeRows);
        }
#endif
        if (open)
        {
            vars.WindowOut = ImGui::GetCurrentWindow();
            vars.ItemsOut = 0;
            vars.ItemsOutMask.Create(vars.ItemsIn);

            float start_y = ImGui::GetCursorScreenPos().y;
            ImGuiListClipper clipper;
            if (vars.ClipperManualItemHeight)
                clipper.Begin(vars.ItemsIn, ImGui::GetTextLineHeightWithSpacing());
            else
                clipper.Begin(vars.ItemsIn);
            while (clipper.Step())
            {
                for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                {
#ifdef IMGUI_HAS_TABLE
                    if (vars.TableEnable)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                    }
#endif
                    ImGui::Text("Item %04d", n);
                    vars.ItemsOut++;
                    vars.ItemsOutMask.SetBit(n);
                }
            }
            vars.OffsetY = ImGui::GetCursorScreenPos().y - start_y;

#ifdef IMGUI_HAS_TABLE
            if (vars.TableEnable)
                ImGui::EndTable();
#endif
        }
        ImGui::End();
        ImGui::PopStyleVar();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetUserData<ClipperTestVars>();
        float item_height = ImGui::GetTextLineHeightWithSpacing(); // EXPECTED item height

#ifdef IMGUI_HAS_TABLE
        int step_count = 4;
#else
        int step_count = 1;
#endif
        for (int clipper_step = 0; clipper_step < 2; clipper_step++)
            for (int step = 0; step < step_count; step++)
            {
                vars.ClipperManualItemHeight = (clipper_step == 1);
                vars.TableEnable = (step > 0);
                vars.TableFreezeRows = (step == 2) ? 1 : (step == 3) ? 2 : 0;
                ctx->LogInfo("## Step %d, Table=%d, TableFreezeRows=%d, ClipperManualItemHeight=%d", step, vars.TableEnable, vars.TableFreezeRows, vars.ClipperManualItemHeight);

                ctx->Yield();
                ctx->Yield();
                ctx->SetRef(vars.WindowOut); // FIXME-TESTS: Can't use ->Name as the / would be ignored
                ctx->WindowFocus("");

                // Test only rendering items 0->9
                vars.WindowHeightInItems = 10.0f;
                vars.ItemsIn = 100;
                ctx->ScrollToTop();
                ctx->Yield();
                IM_CHECK_EQ(vars.OffsetY, vars.ItemsIn * item_height);
                IM_CHECK_EQ(vars.ItemsOut, 10);

                // Test only rendering items 0->10 (window slightly taller)
                vars.WindowHeightInItems = 10.5f;
                vars.ItemsIn = 100;
                ctx->Yield();
                IM_CHECK_EQ(vars.OffsetY, vars.ItemsIn * item_height);
                IM_CHECK_EQ(vars.ItemsOut, 11);

                // Test rendering 0 + trailing items (window scrolled)
                vars.WindowHeightInItems = 10.0f;
                vars.ItemsIn = 100;
                ctx->Yield();
                ctx->KeyPressMap(ImGuiKey_PageDown);
                ctx->Yield();
                IM_CHECK_EQ(vars.OffsetY, vars.ItemsIn * item_height);
                if (vars.ClipperManualItemHeight)
                    IM_CHECK_EQ(vars.ItemsOut, 10);
                else
                    IM_CHECK_EQ(vars.ItemsOut, 1 + 10);
                if (vars.ClipperManualItemHeight && vars.TableFreezeRows == 0)
                    IM_CHECK(vars.ItemsOutMask.TestBit(0) == false);
                else
                    IM_CHECK(vars.ItemsOutMask.TestBit(0) == true);
                ctx->ScrollToBottom();

                IM_CHECK_EQ(vars.OffsetY, vars.ItemsIn * item_height);
                if (vars.ClipperManualItemHeight)
                    IM_CHECK_EQ(vars.ItemsOut, 10);
                else
                    IM_CHECK_EQ(vars.ItemsOut, 1 + 10);
                if (vars.ClipperManualItemHeight && vars.TableFreezeRows == 0)
                    IM_CHECK(vars.ItemsOutMask.TestBit(0) == false);
                else
                    IM_CHECK(vars.ItemsOutMask.TestBit(0) == true);
                //IM_CHECK(vars.ItemsOutMask.TestBit(0 + vars.TableFreezeRows) == true);
                IM_CHECK(vars.ItemsOutMask.TestBit(1 + vars.TableFreezeRows) == false);

                //IM_CHECK(vars.ItemsOutMask.TestBit(89 + vars.TableFreezeRows) == false);
                IM_CHECK(vars.ItemsOutMask.TestBit(90 + vars.TableFreezeRows) == true);
                IM_CHECK(vars.ItemsOutMask.TestBit(99) == true);

                // Test some edges cases
                vars.ItemsIn = 1;
                ctx->Yield();
                ctx->Yield();
                IM_CHECK_EQ(vars.OffsetY, vars.ItemsIn * item_height);
                IM_CHECK_EQ(vars.ItemsOut, 1);
                IM_CHECK_EQ(vars.ItemsOutMask.TestBit(0), true);

                // Test some edges cases
                vars.ItemsIn = 0;
                ctx->Yield();
                IM_CHECK_EQ(vars.OffsetY, vars.ItemsIn* item_height);
                IM_CHECK_EQ(vars.ItemsOut, 0);
            }
    };

    // Test edge cases with single item or zero items clipper
    t = IM_REGISTER_TEST(e, "misc", "misc_clipper_single");
    t->SetUserDataType<ClipperTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetUserData<ClipperTestVars>();

        vars.WindowOut = ImGui::GetCurrentWindow();
        vars.ItemsOut = 0;
        vars.ItemsOutMask.Create(vars.ItemsIn);

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
        const int section[] = { 5, 1, 5, 0, 2 };
        for (int section_n = 0; section_n < IM_ARRAYSIZE(section); section_n++)
        {
            const int item_count = section[section_n];
            ctx->LogDebug("item_count = %d", item_count);
            ImGui::Text("Begin");

            const float start_y = ImGui::GetCursorScreenPos().y;
            ImGuiListClipper clipper;
            clipper.Begin(item_count);
            while (clipper.Step())
            {
                for (auto item_n = clipper.DisplayStart; item_n < clipper.DisplayEnd; item_n++)
                    ImGui::Button(Str30f("Section %d Button %d", section_n, item_n).c_str(), ImVec2(-FLT_MIN, 0.0f));
            }

            vars.OffsetY = ImGui::GetCursorScreenPos().y - start_y;
            IM_CHECK_EQ(vars.OffsetY, ImGui::GetFrameHeightWithSpacing() * item_count);

            ImGui::Text("End");
            ImGui::Separator();
        }
        ImGui::End();
    };

    // ## Test ImFontAtlas clearing of input data (#4455, #3487)
    t = IM_REGISTER_TEST(e, "misc", "misc_atlas_clear");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImFontAtlas atlas;
        atlas.Build();
        IM_ASSERT(atlas.IsBuilt());

        atlas.ClearTexData();
        IM_ASSERT(atlas.IsBuilt());

        atlas.ClearInputData();
        IM_ASSERT(atlas.Fonts.Size > 0);
#if IMGUI_VERSION_NUM > 18407
        IM_ASSERT(atlas.IsBuilt());
#endif

        atlas.ClearFonts();
        IM_ASSERT(atlas.Fonts.Size == 0);
        IM_ASSERT(!atlas.IsBuilt());

        atlas.Build();  // Build after clear
        atlas.Build();  // Build too many times
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
        IM_CHECK_EQ(out_ranges.Size, 2*2+1);
        builder.AddText("\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e"); // "Ni-hon-go"
        out_ranges.clear();
        builder.BuildRanges(&out_ranges);
        IM_CHECK_EQ(out_ranges.Size, 3*2+1);
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
            int result_bytes = ImTextStrFromUtf8(converted, max_chars, utf8, utf8 + utf8_len);
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

#define IM_FIRST_CODEPOINT_OP(str, expect, op)                          \
        do                                                              \
        {                                                               \
            unsigned int codepoint1 = 0;                                \
            unsigned int codepoint2 = 0;                                \
            const char* end = str + strlen(str);                        \
            int consumed1 = ImTextCharFromUtf8(&codepoint1, str, end);  \
            int consumed2 = ImTextCharFromUtf8(&codepoint2, str, NULL); \
            IM_CHECK_EQ_NO_RET(consumed1, consumed2);                   \
            IM_CHECK_LE_NO_RET(str + consumed1, end);                   \
            IM_CHECK_LE_NO_RET(str + consumed2, end);                   \
            IM_CHECK_OP(codepoint1, (unsigned int)expect, op, false);   \
        } while (0)

#ifdef IMGUI_USE_WCHAR32
#define IM_CHECK_UTF8(_TEXT)   (check_utf8(u8##_TEXT, (ImWchar*)U##_TEXT))
        // Test whether 32bit codepoints are correctly decoded.
        IM_FIRST_CODEPOINT_OP((const char*)u8"\U0001f60d", 0x0001f60d, ==);
#else
#define IM_CHECK_UTF8(_TEXT)   (check_utf8(u8##_TEXT, (ImWchar*)u##_TEXT))
        // Test whether 32bit codepoints are correctly discarded.
        IM_FIRST_CODEPOINT_OP((const char*)u8"\U0001f60d", IM_UNICODE_CODEPOINT_INVALID, ==);
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
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u0660\u0661\u0662\u0663\u0664\u0665\u0666\u0667\u0668\u0669"));

        // Unicode Subscript/Superscript
        // Strings which contain unicode subscripts/superscripts; can cause rendering issues
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u2070\u2074\u2075"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u2080\u2081\u2082"));
        IM_CHECK_NO_RET(IM_CHECK_UTF8("\u2070\u2074\u2075\u2080\u2081\u2082"));

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

        // Removed most of them from this codebase on 2020-11-05 because misleading (we're testing UTF-8 encoding/decoding, not actual contents handling)
        // **Removed** Quotation Marks (Strings which contain misplaced quotation marks; can cause encoding errors)
        // **Removed** Japanese Emoticons (Strings which consists of Japanese-style emoticons which are popular on the web)
        // **Removed** Right-To-Left Strings (Strings which contain text that should be rendered RTL if possible (e.g. Arabic, Hebrew))
        // **Removed** Zalgo Text (Strings which contain "corrupted" text. The corruption will not appear in non-HTML text, however. (via http://www.eeemo.net))
        // **Removed** Unicode Upsidedown (Strings which contain unicode with an "upsidedown" effect (via http://www.upsidedowntext.com))
        // **Removed** iOS Vulnerability (Strings which crashed iMessage in iOS versions 8.3 and earlier)

        // Invalid inputs
        // FIXME-MISC: ImTextCharFromUtf8() returns 0 codepoint when first byte is not valid utf-8. If first byte is valid utf-8 but codepoint is still invalid - IM_UNICODE_CODEPOINT_INVALID is returned.
        IM_FIRST_CODEPOINT_OP("\x80", IM_UNICODE_CODEPOINT_INVALID, ==);         // U+0000 - U+007F   00-7F
        IM_FIRST_CODEPOINT_OP("\xFF", IM_UNICODE_CODEPOINT_INVALID, ==);
        unsigned char valid_ranges[][8] = {
            { 0xC2, 0xDF,  0x80, 0xBF                           }, // U+0080   - U+07FF   C2-DF  80-BF
            { 0xE0, 0xE0,  0xA0, 0xBF,  0x80, 0xBF              }, // U+0800   - U+0FFF   E0     A0-BF  80-BF
            { 0xE1, 0xEC,  0x80, 0xBF,  0x80, 0xBF              }, // U+1000   - U+CFFF   E1-EC  80-BF  80-BF
            { 0xED, 0xED,  0x80, 0x9F,  0x80, 0xBF              }, // U+D000   - U+D7FF   ED     80-9F  80-BF
            { 0xEE, 0xEF,  0x80, 0xBF,  0x80, 0xBF              }, // U+E000   - U+FFFF   EE-EF  80-BF  80-BF
#ifdef IMGUI_USE_WCHAR32
            { 0xF0, 0xF0,  0x90, 0xBF,  0x80, 0xBF,  0x80, 0xBF }, // U+10000  - U+3FFFF  F0     90-BF  80-BF  80-BF
            { 0xF1, 0xF3,  0x80, 0xBF,  0x80, 0xBF,  0x80, 0xBF }, // U+40000  - U+FFFFF  F1-F3  80-BF  80-BF  80-BF
            { 0xF4, 0xF4,  0x80, 0x8F,  0x80, 0xBF,  0x80, 0xBF }, // U+100000 - U+10FFFF F4     80-8F  80-BF  80-BF
#endif
        };
        for (int range_n = 0; range_n < IM_ARRAYSIZE(valid_ranges); range_n++)
        {
            const unsigned char* range = valid_ranges[range_n];
            unsigned char seq[4] = { 0, 0, 0, 0 };

            // 6 bit mask, 2 bits for each of 1-3 bytes in tested sequence.
            for (int mask = 0; mask < (1 << (3 * 2)); mask++)
            {
                // First byte follows a sequence between valid ranges. Use always-valid byte, couple out of range cases are tested manually.
                seq[0] = (mask % 2) == 0 ? range[0] : range[1];

                // 1-3 bytes will be tested as follows: in range, below valid range, above valid range.
                for (int n = 1; n < 4; n++)
                {
                    // Bit 0 - 0: test out of range, 1: test in range.
                    // Bit 1 - 0: test end of range, 1: test start of range.
                    int shift = ((n - 1) * 2);
                    int b = (mask & (3 << shift)) >> shift;
                    int byte_n = n * 2;
                    if (range[byte_n + 0])
                    {
                        seq[n] = (b & 2) ? range[byte_n + 0] : range[byte_n + 1];
                        if (!(b & 1))
                            seq[n] += (b & 2) ? -1 : +1; // Move byte out of valid range
                    }
                    else
                    {
                        seq[n] = (unsigned char)0;
                    }
                }

                //ctx->LogDebug("%02X%02X%02X%02X %d %d", seq[0], seq[1], seq[2], seq[3], range_n, mask);
                const unsigned in_range_mask = (seq[1] ? 1 : 0) | (seq[2] ? 4 : 0) | (seq[3] ? 16 : 0);
                if ((mask & in_range_mask) == in_range_mask) // All bytes were in a valid range.
                    IM_FIRST_CODEPOINT_OP((const char*)seq, IM_UNICODE_CODEPOINT_INVALID, !=);
                else
                    IM_FIRST_CODEPOINT_OP((const char*)seq, IM_UNICODE_CODEPOINT_INVALID, ==);
            }
        }
        IM_FIRST_CODEPOINT_OP("\xC1\x80", IM_UNICODE_CODEPOINT_INVALID, ==);         // Two byte sequence, first byte before valid range.
        IM_FIRST_CODEPOINT_OP("\xF5\x80\x80\x80", IM_UNICODE_CODEPOINT_INVALID, ==); // Four byte sequence, first byte after valid range.

        // Incomplete inputs
        IM_FIRST_CODEPOINT_OP("\xE0\xA0", IM_UNICODE_CODEPOINT_INVALID, ==);
#ifdef IMGUI_USE_WCHAR32
        IM_FIRST_CODEPOINT_OP("\xF0\x90\x80", IM_UNICODE_CODEPOINT_INVALID, ==);
        IM_FIRST_CODEPOINT_OP("\xED\xA0\x80", IM_UNICODE_CODEPOINT_INVALID, ==);
#endif
#undef IM_FIRST_CODEPOINT_OP
#undef IM_CHECK_UTF8
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
        ctx->SetRef("Text filter");
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
        draw_list->AddBezierCubic(wp + points[0], wp + points[1], wp + points[2], wp + points[3], IM_COL32_WHITE, 2.0f, num_segments);

        // Draw point closest to the mouse cursor
        ImVec2 point;
        if (num_segments == 0)
            point = ImBezierCubicClosestPointCasteljau(wp + points[0], wp + points[1], wp + points[2], wp + points[3], mouse_pos, style.CurveTessellationTol);
        else
            point = ImBezierCubicClosestPoint(wp + points[0], wp + points[1], wp + points[2], wp + points[3], mouse_pos, num_segments);
        draw_list->AddCircleFilled(point, 4.0f, IM_COL32(255,0,0,255));

        ImGui::End();
    };

    // ## Ensure Logging as Text output correctly formatted widget
    // FIXME-TESTS: Test/document edge of popping out of initial tree depth.
    // FIXME-TESTS: Add more thorough tests that logging output is not affected by clipping.
    t = IM_REGISTER_TEST(e, "misc", "misc_log_functions");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        // Button
        ImGui::LogToClipboard();
        ImGui::Button("Button");
        ImGui::LogFinish();
        IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "[ Button ]" IM_NEWLINE);

        // Checkbox
        {
            bool b = false;
            ImGui::LogToClipboard();
            ImGui::Checkbox("Checkbox", &b);
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "[ ] Checkbox" IM_NEWLINE);

            b = true;
            ImGui::LogToClipboard();
            ImGui::Checkbox("Checkbox", &b);
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "[x] Checkbox" IM_NEWLINE);

            int i = 1;
            ImGui::LogToClipboard();
            ImGui::CheckboxFlags("Checkbox Flags", &i, 3);
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "[~] Checkbox Flags" IM_NEWLINE);
        }

        // RadioButton
        {
            ImGui::LogToClipboard();
            ImGui::RadioButton("Radio", false);
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "( ) Radio" IM_NEWLINE);

            ImGui::LogToClipboard();
            ImGui::RadioButton("Radio", true);
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "(x) Radio" IM_NEWLINE);
        }

        // Sliders/Drags
        {
            float f = 42.42f;
            ImGui::LogToClipboard();
            ImGui::SliderFloat("Slider Float", &f, 0.0f, 100.0f);
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "{ 42.420 } Slider Float" IM_NEWLINE);

            int i = 64;
            ImGui::LogToClipboard();
            ImGui::DragInt("Drag Int", &i, 0, 100);
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "{ 64 } Drag Int" IM_NEWLINE);
        }

        // Separator
        {
            ImGui::LogToClipboard();
            ImGui::Separator();
            ImGui::Text("Hello World");
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "--------------------------------" IM_NEWLINE "Hello World" IM_NEWLINE);
        }

        // CollapsingHeader
        {
            ImGui::LogToClipboard();
            ImGui::SetNextItemOpen(false);
            if (ImGui::CollapsingHeader("Collapsing Header", ImGuiTreeNodeFlags_NoAutoOpenOnLog))
                ImGui::Text("Closed Header Content");
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "### Collapsing Header ###" IM_NEWLINE);

            ImGui::LogToClipboard();
            ImGui::SetNextItemOpen(true);
            if (ImGui::CollapsingHeader("Collapsing Header"))
                ImGui::Text("Open Header Content");
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "### Collapsing Header ###" IM_NEWLINE "Open Header Content" IM_NEWLINE);
        }

        // TreeNode
        {
            ImGui::LogToClipboard();
            ImGui::SetNextItemOpen(false);
            if (ImGui::TreeNodeEx("TreeNode", ImGuiTreeNodeFlags_NoAutoOpenOnLog))
            {
                ImGui::Text("Closed TreeNode Content");
                ImGui::TreePop();
            }
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "> TreeNode" IM_NEWLINE);

            ImGui::LogToClipboard();
            ImGui::SetNextItemOpen(true);
            if (ImGui::TreeNode("TreeNode"))
            {
                ImGui::Text("Open TreeNode Content");
                ImGui::TreePop();
            }
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "> TreeNode" IM_NEWLINE "    Open TreeNode Content" IM_NEWLINE);

            ImGui::LogToClipboard();
            ImGui::SetNextItemOpen(true);
            bool open = ImGui::TreeNode("TreeNode2");
            ImGui::SameLine();
            ImGui::SmallButton("Button");
            if (open)
            {
                ImGui::Text("Open TreeNode Content");
                ImGui::TreePop();
            }
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "> TreeNode2 [ Button ]" IM_NEWLINE "    Open TreeNode Content" IM_NEWLINE);
        }

        // Advanced / new line behaviors
        {
            ImVec2 p0(100, 100);
            ImVec2 p1(100, 200);

            // (1)
            ImGui::LogToClipboard();
            ImGui::LogRenderedText(NULL, "Hello\nWorld");
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "Hello" IM_NEWLINE "World" IM_NEWLINE);

            ImGui::LogToClipboard();
            ImGui::LogRenderedText(&p0, "Hello\nWorld");
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "Hello" IM_NEWLINE "World" IM_NEWLINE);

            // (2)
            ImGui::LogToClipboard();
            ImGui::LogRenderedText(NULL, "Hello");
            ImGui::LogRenderedText(NULL, "World");
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "Hello World" IM_NEWLINE);

            ImGui::LogToClipboard();
            ImGui::LogRenderedText(&p0, "Hello");
            ImGui::LogRenderedText(&p0, "World");
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "Hello World" IM_NEWLINE);

            ImGui::LogToClipboard();
            ImGui::LogRenderedText(&p0, "Hello");
            ImGui::LogRenderedText(&p1, "World");
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "Hello" IM_NEWLINE "World" IM_NEWLINE);

            // (3) Trailing \n in strings
            ImGui::LogToClipboard();
            ImGui::LogRenderedText(&p0, "Hello\n");
            ImGui::LogRenderedText(&p0, "World");
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "Hello" IM_NEWLINE "World" IM_NEWLINE);

            ImGui::LogToClipboard();
            ImGui::LogRenderedText(&p0, "Hello\n");
            ImGui::LogRenderedText(&p1, "World");
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "Hello" IM_NEWLINE IM_NEWLINE "World" IM_NEWLINE);

            ImGui::LogToClipboard();
            ImGui::LogRenderedText(NULL, "Hello\n");
            ImGui::LogRenderedText(NULL, "World");
            ImGui::LogFinish();
            IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "Hello" IM_NEWLINE "World" IM_NEWLINE);
        }

        ImGui::End();
    };

    // FIXME-TESTS
    t = IM_REGISTER_TEST(e, "demo", "demo_misc_001");
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
        ctx->PopupCloseAll();

        //ctx->ItemClick("Layout & Scrolling");  // FIXME: close popup
        ctx->ItemOpen("Layout & Scrolling");
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
        ctx->SetRef("Dear ImGui Demo");
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

#if 0
        ctx->CaptureScreenshotWindow("Dear ImGui Demo", ImGuiCaptureFlags_StitchFullContents);
#endif
    };

    // ## Coverage: closes everything in demo window
    t = IM_REGISTER_TEST(e, "demo", "demo_cov_auto_close");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
    };

    t = IM_REGISTER_TEST(e, "demo", "demo_cov_001");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Help");
        ctx->ItemOpen("Configuration");
        ctx->ItemOpen("Window options");
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Layout & Scrolling");
        ctx->ItemOpen("Popups & Modal windows");
#ifdef IMGUI_HAS_TABLE
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
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Layout & Scrolling");
        ctx->ItemOpen("Scrolling");
        ctx->ItemCheck("Scrolling/Show Horizontal contents size demo window");   // FIXME-TESTS: maybe ItemXXX functions could do the recursion (e.g. Open all parents first)
        ImGuiTestActionFilter filter;
        filter.MaxPasses = filter.MaxDepth = 1;
        filter.RequireAllStatusFlags = ImGuiItemStatusFlags_Checkable;
        ctx->ItemActionAll(ImGuiTestAction_Click, "/Horizontal contents size demo window", &filter);    // Toggle
        ctx->ItemActionAll(ImGuiTestAction_Click, "/Horizontal contents size demo window", &filter);    // Toggle
        ctx->ItemUncheck("Scrolling/Show Horizontal contents size demo window");

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Tools/About Dear ImGui");
        ctx->SetRef("About Dear ImGui");
        ctx->ItemCheck("Config\\/Build Information");
        ctx->SetRef("Dear ImGui Demo");

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Tools/Style Editor");
        ctx->SetRef("Dear ImGui Style Editor");
        ctx->ItemClick("##tabs/Sizes");
        ctx->ItemClick("##tabs/Colors");
        ctx->ItemClick("##tabs/Fonts");
        ctx->ItemOpenAll("##tabs/Fonts", 2);
        ctx->ItemCloseAll("##tabs/Fonts");
        ctx->ItemClick("##tabs/Rendering");

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Custom rendering");
        ctx->SetRef("Example: Custom rendering");
        ctx->ItemClick("##TabBar/Primitives");
        ctx->ItemClick("##TabBar/Canvas");
        ctx->ItemClick("##TabBar/BG\\/FG draw lists");

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Console");
        ctx->SetRef("Example: Console");
        ctx->ItemClick("Input");
        ctx->KeyCharsAppend("h");
        ctx->KeyPressMap(ImGuiKey_Tab);
        ctx->KeyCharsReplace("cl");
        ctx->KeyPressMap(ImGuiKey_Tab);
        ctx->KeyCharsReplace("cla");
        ctx->KeyPressMap(ImGuiKey_Tab);
        ctx->KeyCharsReplace("zzZZzz");
        ctx->KeyPressMap(ImGuiKey_Tab);
        ctx->KeyCharsReplaceEnter("HELP");
        ctx->KeyCharsReplaceEnter("HISTORY");
        ctx->KeyCharsReplaceEnter("CLEAR");

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuUncheckAll("Tools");

        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Color\\/Picker Widgets");
        ctx->ItemClick("Color\\/Picker Widgets/Palette");
        ctx->ItemClose("Widgets");

        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Popups & Modal windows");
        ctx->ItemOpen("Modals");
        ctx->ItemClick("Modals/Delete..");
        ctx->SetRef("");
        ctx->ItemClick("Delete?/OK");
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemClose("Modals");
        ctx->ItemClose("Popups & Modal windows");

        // TODO: We could add more in-depth tests instead of just click-coverage
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Text Input");
        ctx->ItemOpen("Text Input/Completion, History, Edit Callbacks");
        ctx->SetRef("Dear ImGui Demo/Text Input/Completion, History, Edit Callbacks");
        ctx->ItemClick("Completion");
        ctx->ItemClick("History");
        ctx->ItemClick("Edit");

        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Text Input/Resize Callback");
        ctx->SetRef("Dear ImGui Demo/Text Input/Resize Callback");
        ctx->ItemClick("##MyStr");

        // Ensure X scrolling is working in TestContext
        ImGuiWindow* window = ctx->GetWindowByRef("Dear ImGui Demo");
        ImVec2 backup_size = window->Rect().GetSize();
        ctx->SetRef("Dear ImGui Demo");
        ctx->WindowResize("Dear ImGui Demo", ImVec2(100, backup_size.y));
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Basic");
        ctx->ItemClick("Basic/radio c");
        ctx->WindowResize("Dear ImGui Demo", backup_size);

        // Flex the popup showing in "Circle Tessellation Max Error" (note that style modifications are restored at end of test)
        ctx->ItemOpen("Configuration");
        ctx->ItemOpen("Style");
        ctx->ItemClick("**/Rendering");
        ctx->ItemDoubleClick("**/Circle Tessellation Max Error");
        ctx->KeyCharsReplaceEnter("1.5");
        ctx->ItemDoubleClick("**/Circle Tessellation Max Error");
        ctx->KeyCharsReplaceEnter("1.25");
    };

    t = IM_REGISTER_TEST(e, "demo", "demo_cov_apps");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuClick("Menu/Open Recent/More..");
        ctx->MenuCheckAll("Examples");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuCheckAll("Tools");
        ctx->MenuUncheckAll("Tools");
    };

    t = IM_REGISTER_TEST(e, "demo", "demo_cov_examples");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Documents");

        ctx->SetRef("Example: Documents");
        ctx->ItemCheck("**/Lettuce");
        ctx->ItemClick("##tabs/Lettuce");
        ctx->ItemClick("##tabs/Lettuce/**/Modify");
        ctx->MenuClick("File");
        ctx->SetRef("");
        ctx->ItemClick("##Menu_00/Close All Documents");
        ctx->ItemClick("Save?/Yes");

        // Reopen the Lettuce document in case we re-run the test
        ctx->SetRef("Example: Documents");
        ctx->MenuClick("File/Open/Lettuce");
    };

    // ## Coverage: select all styles via the Style Editor
    t = IM_REGISTER_TEST(e, "demo", "demo_cov_styles");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Tools/Style Editor");

        ImFont* font_backup = ImGui::GetFont();
        ImGuiTestRef ref_window = "Dear ImGui Style Editor";
        ctx->SetRef(ref_window);
        ctx->ComboClickAll("Fonts##Selector");
        ImGui::GetIO().FontDefault = font_backup;

        ImGuiStyle style_backup = ImGui::GetStyle();
        ctx->SetRef(ref_window);
        ctx->ComboClickAll("Colors##Selector");
        ImGui::GetStyle() = style_backup;
    };

    // ## Coverage: exercice some actions in ColorEditOptionsPopup() and ColorPickerOptionsPopup(
    t = IM_REGISTER_TEST(e, "demo", "demo_cov_color_picker");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Basic");

        ctx->MouseMove("Basic/color 2/##ColorButton");
        ctx->MouseClick(1); // Open picker settings popup
        ctx->Yield();

        ctx->SetRef(ctx->GetFocusWindowRef());
        ctx->ItemClick("RGB");
        ctx->ItemClick("HSV");
        ctx->ItemClick("Hex");
        ctx->ItemClick("RGB");
        ctx->ItemClick("0..255");
        ctx->ItemClick("0.00..1.00");
        ctx->ItemClick("0..255");

        ctx->ItemClick("Copy as..");
        ctx->KeyPressMap(ImGuiKey_Escape); // Close popup

        for (int picker_type = 0; picker_type < 2; picker_type++)
        {
            ctx->SetRef("Dear ImGui Demo");
            ctx->MouseMove("Basic/color 2/##ColorButton");
            ctx->MouseClick(0); // Open color picker
            ctx->Yield();

            // Open color picker style chooser
            ctx->SetRef(ctx->GetFocusWindowRef());
            ctx->MouseMoveToPos(ctx->GetWindowByRef("")->Rect().GetCenter());
            ctx->MouseClick(1);
            ctx->Yield();

            // Select picker type
            ctx->SetRef(ctx->GetFocusWindowRef());
            ctx->MouseMove(ctx->GetID("##selectable", ctx->GetIDByInt(picker_type)));
            ctx->MouseClick(0);

            // Interact with picker
            ctx->SetRef(ctx->GetFocusWindowRef());
            if (picker_type == 0)
            {
                ctx->MouseMove("##picker/sv", ImGuiTestOpFlags_MoveToEdgeU | ImGuiTestOpFlags_MoveToEdgeL);
                ctx->MouseDown(0);
                ctx->MouseMove("##picker/sv", ImGuiTestOpFlags_MoveToEdgeD | ImGuiTestOpFlags_MoveToEdgeR);
                ctx->MouseMove("##picker/sv");
                ctx->MouseUp(0);

                ctx->MouseMove("##picker/hue", ImGuiTestOpFlags_MoveToEdgeU);
                ctx->MouseDown(0);
                ctx->MouseMove("##picker/hue", ImGuiTestOpFlags_MoveToEdgeD);
                ctx->MouseMove("##picker/hue", ImGuiTestOpFlags_MoveToEdgeU);
                ctx->MouseUp(0);
            }
            if (picker_type == 1)
            {
                ctx->MouseMove("##picker/hsv");
                ctx->MouseClick(0);
            }

            ctx->PopupCloseAll();
        }
    };

    // ## Coverage: open everything in metrics window
    t = IM_REGISTER_TEST(e, "demo", "demo_cov_metrics");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Ensure Metrics windows is closed when beginning the test
        ctx->SetRef("/Dear ImGui Demo");
        ctx->MenuCheck("Tools/Metrics\\/Debugger");

        // Make tables visible to exercice more of metrics paths
        ctx->ItemOpen("Tables & Columns");
        ctx->ItemOpen("Tables/Advanced");

        ctx->SetRef("/Dear ImGui Metrics\\/Debugger");
        ctx->ItemCloseAll("");

        // FIXME-TESTS: Maybe add status flags filter to GatherItems() ?
        ImGuiTestItemList items;
        ctx->GatherItems(&items, "", 1);
        for (const ImGuiTestItemInfo& item : items)
        {
            if ((item.StatusFlags & ImGuiItemStatusFlags_Openable) == 0)
                continue;
            //if (item.ID != ctx->GetID("Settings")) // [DEBUG]
            //    continue;

            ctx->ItemOpen(item.ID);

            // FIXME-TESTS: Anything and "DrawLists" DrawCmd sub-items are updated when hovering items,
            // they make the tests fail because some "MouseOver" can't find gathered items and make the whole test stop.
            // Maybe make it easier to perform some filtering, aka OpenAll except "XXX"
            // Maybe could add support for ImGuiTestOpFlags_NoError in the ItemOpenAll() path?
            {
                const int max_count_per_depth[] = { 3, 3, 1, 0 };
                ImGuiTestActionFilter filter;
                filter.MaxDepth = 3;
                filter.MaxItemCountPerDepth = max_count_per_depth;
                filter.RequireAllStatusFlags = ImGuiItemStatusFlags_Openable;
                if (item.ID == ctx->GetID("Windows") || item.ID == ctx->GetID("Viewport") || item.ID == ctx->GetID("Viewports"))
                    filter.MaxDepth = 1;
                else if (item.ID == ctx->GetID("DrawLists"))
                    filter.MaxDepth = 1;
                ctx->ItemActionAll(ImGuiTestAction_Open, item.ID, &filter);
            }

            if (item.ID == ctx->GetID("Windows"))
            {
                const int max_count_per_depth[] = { 1, 1, 0 };
                ImGuiTestActionFilter filter;
                filter.RequireAllStatusFlags = ImGuiItemStatusFlags_Openable;
                filter.MaxDepth = 2;
                filter.MaxPasses = 1;
                filter.MaxItemCountPerDepth = max_count_per_depth;
                ctx->ItemActionAll(ImGuiTestAction_Open, "Windows", &filter);
                ctx->ItemActionAll(ImGuiTestAction_Close, "Windows", &filter);
            }

            // Toggle all tools (to enable/disable them, then restore their initial state)
            if (item.ID == ctx->GetID("Tools"))
            {
                ImGuiTestActionFilter filter;
                filter.RequireAllStatusFlags = ImGuiItemStatusFlags_Checkable;
                filter.MaxDepth = filter.MaxPasses = 1;

                ctx->WindowFocus("/Dear ImGui Demo"); // To exercible "Show Tables Rects"
                ctx->ItemActionAll(ImGuiTestAction_Click, "Tools", &filter);
                ctx->WindowFocus("/Dear ImGui Demo"); // To exercible "Show Tables Rects"
                ctx->ItemActionAll(ImGuiTestAction_Click, "Tools", &filter);
            }

            // Open fonts/glyphs
            if (item.ID == ctx->GetID("Fonts"))
            {
                ImGuiTestActionFilter filter;
                filter.RequireAllStatusFlags = ImGuiItemStatusFlags_Openable;
                filter.MaxDepth = filter.MaxPasses = 4;
                ctx->ItemActionAll(ImGuiTestAction_Open, "Fonts", &filter);
                ctx->ItemActionAll(ImGuiTestAction_Close, "Fonts", &filter);
            }

            // FIXME-TESTS: in docking branch this is under Viewports
            if (item.ID == ctx->GetID("DrawLists"))
            {
                const int max_count_per_depth[] = { 3, 3, 0 };
                ImGuiTestActionFilter filter;
                filter.MaxDepth = 2;
                filter.MaxItemCountPerDepth = max_count_per_depth;
                ctx->ItemActionAll(ImGuiTestAction_Open, item.ID, &filter);
                //IM_DEBUG_HALT_TESTFUNC();
                ctx->ItemActionAll(ImGuiTestAction_Hover, item.ID, &filter);
            }

            // Close
            ctx->ItemCloseAll(item.ID);
            ctx->ItemClose(item.ID);
        }

        ctx->WindowClose("");

        ctx->SetRef("/Dear ImGui Demo");
        ctx->ItemClose("Tables/Advanced");
        ctx->ItemClose("Tables & Columns");
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
        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Documents");

        ctx->SetRef("Example: Documents");
        ctx->WindowResize(ctx->RefID, ImVec2(600, 300));    // Ensure no items are clipped, because then they cant be found by item search
        ctx->ItemCheck("**/Tomato");
        ctx->ItemCheck("**/Eggplant");
        ctx->ItemCheck("**/A Rather Long Title");
        ctx->ItemClick("##tabs/Eggplant");
        ctx->SetRef(ctx->GetID("##tabs/Eggplant"));
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
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuCheck("Examples/Simple overlay");
        ctx->SetRef("Example: Simple overlay");
        ImGuiWindow* window_overlay = ctx->GetWindowByRef("");
        IM_CHECK(window_overlay != NULL);

        // FIXME-TESTS: Find last newly opened window? -> cannot rely on NavWindow as menu item maybe was already checked..

        ImVec2 viewport_pos = ctx->GetMainViewportPos();
        ImVec2 viewport_size = ctx->GetMainViewportSize();
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
        ctx->WindowMove("", viewport_pos + ImVec2(pad, viewport_size.y - pad), ImVec2(0.0f, 1.0f));

        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Documents");
        ctx->SetRef("Example: Documents");
        ctx->WindowResize("", ImVec2(fh * 20, fh * 27));
        ctx->WindowMove("", ImVec2(window_custom_rendering->Pos.x + window_custom_rendering->Size.x + pad, window_custom_rendering->Pos.y + pad));

        ctx->LogDebug("Setup Console window...");
        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Console");
        ctx->SetRef("Example: Console");
        ctx->WindowResize("", ImVec2(fh * 40, fh * (34-7)));
        ctx->WindowMove("", window_custom_rendering->Pos + window_custom_rendering->Size * ImVec2(0.30f, 0.60f));
        ctx->ItemClick("Clear");
        ctx->ItemClick("Add Debug Text");
        ctx->ItemClick("Add Debug Error");
        ctx->ItemClick("Input");
        ctx->KeyChars("H");
        ctx->KeyPressMap(ImGuiKey_Tab);
        ctx->KeyCharsAppendEnter("ELP");
        ctx->KeyCharsAppendEnter("hello, imgui world!");

        ctx->LogDebug("Setup Demo window...");
        ctx->SetRef("Dear ImGui Demo");
        ctx->WindowResize("", ImVec2(fh * 35, viewport_size.y - pad * 2.0f));
        ctx->WindowMove("", viewport_pos + ImVec2(viewport_size.x - pad, pad), ImVec2(1.0f, 0.0f));
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Color\\/Picker Widgets");
        ctx->ItemOpen("Layout & Scrolling");
        ctx->ItemOpen("Groups");
        ctx->ScrollToItemY("Layout & Scrolling", 0.8f);

        ctx->LogDebug("Capture screenshot...");
        ctx->SetRef("");

        ImGuiCaptureArgs args;
        ctx->CaptureInitArgs(&args);
        ctx->CaptureAddWindow(&args, "Dear ImGui Demo");
        ctx->CaptureAddWindow(&args, "Example: Simple overlay");
        ctx->CaptureAddWindow(&args, "Example: Custom rendering");
        ctx->CaptureAddWindow(&args, "Example: Simple layout");
        ctx->CaptureAddWindow(&args, "Example: Documents");
        ctx->CaptureAddWindow(&args, "Example: Console");
        ctx->CaptureScreenshotEx(&args);

        // Close everything
        ctx->SetRef("Dear ImGui Demo");
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
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Capture both windows in separate captures
        ImGuiContext& g = *ctx->UiContext;
        for (int n = 0; n < 2; n++)
        {
            ImGuiWindow* window = (n == 0) ? ctx->GetWindowByRef("/Debug##Dark") : ctx->GetWindowByRef("/Debug##Light");
            ctx->SetRef(window);
            ctx->ItemClick("string");
            ctx->KeyCharsReplace("quick brown fox");
            //ctx->KeyPressMap(ImGuiKey_End);
            ctx->MouseMove("float");
            ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(30, -10));

            ctx->CaptureScreenshotWindow(window->Name);
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
        ctx->SetRef("CaptureGif");
        ImGuiCaptureArgs args;
        ctx->CaptureInitArgs(&args);
        ctx->CaptureAddWindow(&args, "CaptureGif");
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

    // ## Capture
    t = IM_REGISTER_TEST(e, "capture", "capture_readme_my_first_tool");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(440, 330));
        ImFont* font = FindFontByName("Roboto-Medium.ttf, 16px"); // FIXME-TESTS
        IM_CHECK_SILENT(font != NULL);
        ImGui::PushFont(font);

        // State
        static bool my_tool_active = true;
        static float my_color[4] = { 0.8f, 0.0f, 0.0f, 1.0f };

        if (my_tool_active)
        {
            // Create a window called "My First Tool", with a menu bar.
            ImGui::Begin("My First Tool", &my_tool_active, ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
                    if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */ }
                    if (ImGui::MenuItem("Close", "Ctrl+W")) { my_tool_active = false; }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            // Edit a color (stored as ~4 floats)
            ImGui::ColorEdit4("Color", my_color);

            // Plot some values
            const float my_values[] = { 0.2f, 0.1f, 1.0f, 0.5f, 0.9f, 2.2f };
            ImGui::PlotLines("Frame Times", my_values, IM_ARRAYSIZE(my_values));

            // Display contents in a scrolling region
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Important Stuff");
            ImGui::BeginChild("Scrolling", ImVec2(0, 0), true);
            for (int n = 0; n < 50; n++)
                ImGui::Text("%04d: Some text", n);
            ImGui::EndChild();
            ImGui::End();
        }

        ImGui::PopFont();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // FIXME-TODO: try to match https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v180/code_sample_04_color.gif
        // - maybe we could control timing scale when gif recording
        // - need scroll via scrollbar

        ctx->SetRef("My First Tool");
        ImGuiWindow* window = ctx->GetWindowByRef("");
        ctx->MouseMoveToPos(window->Rect().GetBR() + ImVec2(50, 50));

        ctx->Sleep(0.5f);
        ctx->MenuClick("File/Save");
        ctx->Sleep(0.5f);
        ctx->MouseMove("Color##X");
        ctx->Sleep(0.5f);
        ctx->MouseMove("Color##Y");
        ctx->Sleep(0.5f);
        ctx->MouseMove("Color##Z");
        ctx->Sleep(0.5f);
        ctx->MouseMove("Color##Y");
        ctx->ItemInput("Color##Y");
        ctx->KeyCharsReplaceEnter("200");

        ctx->Sleep(1.0f);
        ctx->ItemClick("Color##ColorButton");

        ctx->SetRef(ctx->GetFocusWindowRef());
        ctx->Sleep(2.0f);
        ctx->PopupCloseAll();
    };

#ifdef IMGUI_HAS_TABLE
    // ## Capture all tables demo
    t = IM_REGISTER_TEST(e, "capture", "capture_table_demo");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Tables & Columns");
        ctx->ItemClick("Tables/Open all");
        ctx->ItemOpen("Tables/Synced instances", 1);
        ctx->ItemOpen("Tables/Advanced/Options");
        ctx->ItemOpenAll("Tables/Advanced/Options", 1);
        ctx->ItemOpen("Tables/Tree view/**/Root");
        ctx->ItemInput("Tables/Advanced/Options/Other:/items_count");

        ctx->KeyCharsReplaceEnter("50000"); // Fancy
        //ctx->TableOpenContextMenu("Tables/Reorderable, hideable, with headers/##table1", 1);

        ctx->CaptureScreenshotWindow("", ImGuiCaptureFlags_StitchFullContents | ImGuiCaptureFlags_HideMouseCursor);

        ctx->ItemClick("Tables/Close all");
    };
#endif

#ifdef IMGUI_TEST_ENGINE_ENABLE_IMPLOT
    // ## Capture all tables demo
    t = IM_REGISTER_TEST(e, "capture", "capture_implot_demo");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImPlot::ShowDemoWindow();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImPlot Demo");
        ctx->ItemOpenAll("");
        ctx->CaptureScreenshotWindow("ImPlot Demo", ImGuiCaptureFlags_StitchFullContents | ImGuiCaptureFlags_HideMouseCursor);
    };
#endif
}

void RegisterTests(ImGuiTestEngine* e)
{
    extern void RegisterTests_Docking(ImGuiTestEngine * e);     // imgui_tests_docking.cpp
    extern void RegisterTests_Nav(ImGuiTestEngine * e);         // imgui_tests_nav.cpp
    extern void RegisterTests_Perf(ImGuiTestEngine * e);        // imgui_tests_perf.cpp
    extern void RegisterTests_Columns(ImGuiTestEngine * e);     // imgui_tests_tables.cpp
    extern void RegisterTests_Table(ImGuiTestEngine * e);       // imgui_tests_tables.cpp
    extern void RegisterTests_Viewports(ImGuiTestEngine * e);   // imgui_tests_viewports.cpp
    extern void RegisterTests_Widgets(ImGuiTestEngine * e);     // imgui_tests_widgets.cpp
    extern void RegisterTests_PerfLog(ImGuiTestEngine* e);      // imgui_te_perflog.cpp

    // Tests
    RegisterTests_Window(e);
    RegisterTests_Layout(e);
    RegisterTests_Widgets(e);
    RegisterTests_Nav(e);
    RegisterTests_Columns(e);
    RegisterTests_Table(e);
    RegisterTests_Docking(e);
    RegisterTests_Viewports(e);
    RegisterTests_DrawList(e);
    RegisterTests_Misc(e);

    // Captures
    RegisterTests_Capture(e);

    // Performance Benchmarks
    RegisterTests_Perf(e);

    // Tools
    RegisterTests_PerfLog(e);
}
