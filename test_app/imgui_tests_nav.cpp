// dear imgui
// (tests: tables, columns)

#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_tests.h"
#include "shared/imgui_utils.h"
#include "test_engine/imgui_te_engine.h"      // IM_REGISTER_TEST()
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

//-------------------------------------------------------------------------
// Tests: Navigation
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
        ctx->SetRef("Hello, world!");
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
        ctx->SetRef("Test window 1");
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

        b_popup_open = ImGui::BeginPopup("Popup", ImGuiWindowFlags_NoSavedSettings);
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

    // ## Test that Alt toggle layer, test that AltGr doesn't.
    t = IM_REGISTER_TEST(e, "nav", "nav_menu_alt_key");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto window_content = []()
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    ImGui::Text("Blah");
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit"))
                {
                    ImGui::Text("Blah");
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
        };

        switch (ctx->GenericVars.Step)
        {
        case 0:
            ImGui::Begin("Test window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
            window_content();
            ImGui::End();
            break;
        case 1:
            if (!ImGui::IsPopupOpen("Test window"))
                ImGui::OpenPopup("Test window");
            if (ImGui::BeginPopupModal("Test window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar))
            {
                window_content();
                ImGui::EndPopup();
            }
            break;
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // FIXME-TESTS: Fails if window is resized too small
        ImGuiContext& g = *ctx->UiContext;
        IM_CHECK(g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard);
        //ctx->SetInputMode(ImGuiInputSource_Nav);
        ctx->SetRef("Test window");

        for (int step = 0; step < 2; step++)
        {
            ctx->GenericVars.Step = step; // Enable modal popup?
            ctx->Yield();

            IM_CHECK(g.OpenPopupStack.Size == step);
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);
            ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Menu);
            IM_CHECK(g.NavId == ctx->GetID("##menubar/File"));
            ctx->KeyPressMap(ImGuiKey_RightArrow);
            IM_CHECK(g.NavId == ctx->GetID("##menubar/Edit"));

            ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);
            ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt | ImGuiKeyModFlags_Ctrl);
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);
            ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt); // Verify nav id is reset for menu layer
            IM_CHECK(g.NavId == ctx->GetID("##menubar/File"));
            ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);
        }
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
        ImGuiContext& g = *ctx->UiContext;
        IM_CHECK(g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard);
        ImGuiWindow* window = ImGui::FindWindowByName("Test Window");
        ctx->SetRef("Test window");
        ctx->SetInputMode(ImGuiInputSource_Nav);

        // FIXME-TESTS: This should not be required but nav init request is not applied until we start navigating, this is a workaround
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);

        IM_CHECK(g.NavId == window->GetID("Button 0"));
        IM_CHECK(window->Scroll.y == 0);
        // Navigate to the middle of window
        for (int i = 0; i < 5; i++)
            ctx->KeyPressMap(ImGuiKey_DownArrow);
        IM_CHECK(g.NavId == window->GetID("Button 5"));
        IM_CHECK(window->Scroll.y > 0 && window->Scroll.y < window->ScrollMax.y);
        // From the middle to the end
        ctx->KeyPressMap(ImGuiKey_End);
        IM_CHECK(g.NavId == window->GetID("Button 9"));
        IM_CHECK(window->Scroll.y == window->ScrollMax.y);
        // From the end to the start
        ctx->KeyPressMap(ImGuiKey_Home);
        IM_CHECK(g.NavId == window->GetID("Button 0"));
        IM_CHECK(window->Scroll.y == 0);
    };

    // ## Test vertical wrap-around in menus/popups
    t = IM_REGISTER_TEST(e, "nav", "nav_menu_wraparound");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuClick("Menu");
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt); // FIXME
        IM_CHECK(g.NavId == ctx->GetID("/##Menu_00/New"));
        ctx->NavKeyPress(ImGuiNavInput_KeyUp_);
        IM_CHECK(g.NavId == ctx->GetID("/##Menu_00/Quit"));
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
        IM_CHECK(g.NavId == ctx->GetID("/##Menu_00/New"));
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

            // Intentionally perform a "SLOW" ctrl-tab to make sure the UI appears!
            //ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Ctrl);
            ctx->KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Ctrl);
            ctx->KeyPressMap(ImGuiKey_Tab);
            ctx->SleepNoSkip(0.5f, 0.1f);
            ctx->KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Ctrl);
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
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_restore_on_missing_window");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Button("Button 1");
        ImGui::Button("Button 2");
        ImGui::BeginChild("Child", ImVec2(100, 100), true);
        ImGui::Button("Button 3");
        ImGui::Button("Button 4");
        ImGui::EndChild();
        ImGui::Button("Button 5");
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
            ctx->Yield(2);
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
            ctx->Yield(2);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Window 1/Button 1"));
        }

        // Test entering child window and leaving it.
        ctx->WindowFocus("Window 1");
        ctx->NavMoveTo("Window 1/Child");
        IM_CHECK(g.NavId == ctx->GetID("Window 1/Child"));
        IM_CHECK((g.NavWindow->Flags & ImGuiWindowFlags_ChildWindow) == 0);

        ctx->NavActivate();                                     // Enter child window
        IM_CHECK((g.NavWindow->Flags & ImGuiWindowFlags_ChildWindow) != 0);
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);               // Manipulate something
        //IM_CHECK(g.NavId == ctx->ItemInfo("/**/Button 4")->ID); // Can't easily include child window name in ID because the / gets inhibited...
        IM_CHECK(g.NavId == ctx->GetID("Button 4", g.NavWindow->ID));
        ctx->NavActivate();
        ctx->NavKeyPress(ImGuiNavInput_Cancel);                 // Leave child window
        IM_CHECK(g.NavId == ctx->GetID("Window 1/Child"));      // Focus resumes last location before entering child window
    };

    // ## Test NavID restoration after activating menu item.
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_restore_menu");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiWindow* demo_window = ctx->GetWindowByRef("Dear ImGui Demo");

        // FIXME-TESTS: Facilitate usage of variants
        const int test_count = ctx->HasDock ? 2 : 1;
        for (int test_n = 0; test_n < test_count; test_n++)
        {
            ctx->LogDebug("TEST CASE %d", test_n);
#ifdef IMGUI_HAS_DOCK
            ctx->SetRef(ImGuiTestRef());
            ctx->DockMultiClear("Dear ImGui Demo", "Hello, world!", NULL);
            if (test_n == 0)
                ctx->DockWindowInto("Dear ImGui Demo", "Hello, world!");
#endif
            ctx->SetRef("Dear ImGui Demo");
            ctx->ItemCloseAll("");

            // Test simple focus restoration.
            ctx->NavMoveTo("Configuration");                            // Focus item.
            ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);     // Focus menu.
            ctx->NavActivate();                                         // Open menu, focus first item in the menu.
            ctx->NavActivate();                                         // Activate first item in the menu.
            IM_CHECK_EQ(g.NavId, ctx->GetID("Configuration"));          // Verify NavId was restored to initial value.

            // Test focus restoration to the item within a child window.
            // FIXME-TESTS: Feels like this would be simpler with our own GuiFunc.
            ctx->NavMoveTo("Layout & Scrolling");                       // Navigate to the child window.
            ctx->NavActivate();
            ctx->NavMoveTo("Scrolling");
            ctx->NavActivate(); // FIXME-TESTS: Could query current g.NavWindow instead of making names?
            Str30f child_window_name("/Dear ImGui Demo\\/scrolling_%08X", ctx->GetID("Scrolling/scrolling"));
            ImGuiWindow* child_window = ctx->GetWindowByRef(child_window_name.c_str());
            ctx->SetRef(child_window_name.c_str());
            ctx->ScrollTo(demo_window, ImGuiAxis_Y, (child_window->Pos - demo_window->Pos).y);  // Required because buttons do not register their IDs when out of view (SkipItems == true).
            ctx->NavMoveTo(ctx->GetID("1", ctx->GetIDByInt(1)));        // Focus item within a child window.
            ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);     // Focus menu.
            ctx->NavActivate();                                         // Open menu, focus first item in the menu.
            ctx->NavActivate();                                         // Activate first item in the menu.
            IM_CHECK_EQ(g.NavId, ctx->GetID("1", ctx->GetIDByInt(1)));  // Verify NavId was restored to initial value.
        }
    };

    // ## Test loss of navigation focus when clicking on empty viewport space (#3344).
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_clear_on_void");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpen("Help");
        ctx->ItemClose("Help");
        IM_CHECK_EQ(g.NavId, ctx->GetID("Help"));
        IM_CHECK_EQ(g.NavWindow, ctx->GetWindowByRef(ctx->RefID));
        IM_CHECK(g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard); // FIXME-TESTS: Should test for both cases.
        IM_CHECK(g.IO.WantCaptureMouse == true);
        // FIXME-TESTS: This depends on ImGuiConfigFlags_NavNoCaptureKeyboard being cleared. Should test for both cases.
        IM_CHECK(g.IO.WantCaptureKeyboard == true);

        ctx->MouseClickOnVoid(0);
        //IM_CHECK(g.NavId == 0); // Clarify specs
        IM_CHECK(g.NavWindow == NULL);
        IM_CHECK(g.IO.WantCaptureMouse == false);
        IM_CHECK(g.IO.WantCaptureKeyboard == false);
    };

    // ## Test inheritance (and lack of) of FocusScope
    // FIXME-TESTS: could test for actual propagation of focus scope from<>into nav data
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_scope");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGuiID focus_scope_id = ImGui::GetID("MyScope");

        IM_CHECK_EQ(ImGui::GetFocusScope(), 0u);
        ImGui::BeginChild("Child 1", ImVec2(100, 100));
        IM_CHECK_EQ(ImGui::GetFocusScope(), 0u);
        ImGui::EndChild();

        ImGui::PushFocusScope(focus_scope_id);
        IM_CHECK_EQ(ImGui::GetFocusScope(), focus_scope_id);
        ImGui::BeginChild("Child 1", ImVec2(100, 100));
        IM_CHECK_EQ(ImGui::GetFocusScope(), focus_scope_id); // Append
        ImGui::EndChild();
        ImGui::BeginChild("Child 2", ImVec2(100, 100));
        IM_CHECK_EQ(ImGui::GetFocusScope(), focus_scope_id); // New child
        ImGui::EndChild();
        IM_CHECK_EQ(ImGui::GetFocusScope(), focus_scope_id);

        // Should not inherit
        ImGui::Begin("Test Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
        IM_CHECK_EQ(ImGui::GetFocusScope(), 0u);
        ImGui::End();

        ImGui::PopFocusScope();

        IM_CHECK_EQ(ImGui::GetFocusScope(), 0u);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
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
        ctx->SetRef("##MainMenuBar");

        // Open menu, focus first "a" item.
        ctx->MenuClick("Menu");
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt); // FIXME
        ctx->SetRef(ctx->UiContext->NavWindow);

        // Navigate to "c" item.
        IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("a"));
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
        IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("c"));
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
        IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("a"));
    };

    // ## Test nav from non-visible focus with keyboard/gamepad. With gamepad, the navigation resumes on the next visible item instead of next item after focused one.
    t = IM_REGISTER_TEST(e, "nav", "nav_from_clipped_item");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *GImGui;

        ImGui::SetNextWindowSize(ctx->GenericVars.Vec2, ImGuiCond_Always);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
        for (int y = 0; y < 6; y++)
            for (int x = 0; x < 6; x++)
            {
                ImGui::Button(Str16f("Button %d,%d", x, y).c_str());
                if (x < 5)
                    ImGui::SameLine();

                // Window size is such that only 3x3 buttons are visible at a time.
                if (ctx->GenericVars.Vec2.y == 0.0f && y == 3 && x == 2)
                    ctx->GenericVars.Vec2 = ImGui::GetCursorPos() + ImVec2(g.Style.ScrollbarSize, g.Style.ScrollbarSize); // FIXME: Calculate Window Size from Decoration Size
            }
        ImGui::End();

    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ImGuiWindow* window = ctx->GetWindowByRef("");

        auto get_focus_item_rect = [](ImGuiWindow* window)
        {
            ImRect item_rect = window->NavRectRel[ImGuiNavLayer_Main];
            item_rect.Translate(window->Pos);
            return item_rect;
        };

        // Test keyboard & gamepad behaviors
        for (int n = 0; n < 2; n++)
        {
            const ImGuiInputSource input_source = (n == 0) ? ImGuiInputSource_Keyboard : ImGuiInputSource_Gamepad;
            ctx->SetInputMode(ImGuiInputSource_Nav);
            ctx->UiContext->NavInputSource = input_source;  // FIXME-NAV: Should be set by above ctx->SetInputMode(ImGuiInputSource_Gamepad) call, but those states are a mess.

            // Down
            ctx->NavMoveTo("Button 0,0");
            ctx->ScrollToX(0);
            ctx->ScrollToBottom();
            ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
            if (input_source == ImGuiInputSource_Keyboard)
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 0,1"));     // Started Nav from Button 0,0 (Previous NavID)
            else
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 0,3"));     // Started Nav from Button 0,2 (Visible)

            ctx->NavKeyPress(ImGuiNavInput_KeyUp_);                             // Move to opposite direction than previous operation in order to trigger scrolling focused item into view
            IM_CHECK(window->InnerRect.Contains(get_focus_item_rect(window)));  // Ensure item scrolled into view is fully visible

            // Up
            ctx->NavMoveTo("Button 0,4");
            ctx->ScrollToX(0);
            ctx->ScrollToTop();
            ctx->NavKeyPress(ImGuiNavInput_KeyUp_);
            if (input_source == ImGuiInputSource_Keyboard)
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 0,3"));
            else
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 0,2"));

            ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
            IM_CHECK(window->InnerRect.Contains(get_focus_item_rect(window)));

            // Right
            ctx->NavMoveTo("Button 0,0");
            ctx->ScrollToX(window->ScrollMax.x);
            ctx->ScrollToTop();
            ctx->NavKeyPress(ImGuiNavInput_KeyRight_);
            if (input_source == ImGuiInputSource_Keyboard)
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 1,0"));
            else
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 3,0"));

            ctx->NavKeyPress(ImGuiNavInput_KeyLeft_);
            IM_CHECK(window->InnerRect.Contains(get_focus_item_rect(window)));

            // Left
            ctx->NavMoveTo("Button 4,0");
            ctx->ScrollToX(0);
            ctx->ScrollToTop();
            ctx->NavKeyPress(ImGuiNavInput_KeyLeft_);
            if (input_source == ImGuiInputSource_Keyboard)
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 3,0"));
            else
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 2,0"));

            ctx->NavKeyPress(ImGuiNavInput_KeyRight_);
            IM_CHECK(window->InnerRect.Contains(get_focus_item_rect(window)));
        }
    };

    // ## Test PageUp/PageDown/Home/End keys.
    t = IM_REGISTER_TEST(e, "nav", "nav_page_end_home_keys");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(100, ctx->GenericVars.Float1), ImGuiCond_Always);

        // FIXME-NAV: Lack of ImGuiWindowFlags_NoCollapse breaks window scrolling without activable items.
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoCollapse);
        for (int i = 0; i < 20; i++)
        {
            if (ctx->GenericVars.Step == 0)
                ImGui::TextUnformatted(Str16f("OK %d", i).c_str());
            else
                ImGui::Button(Str16f("OK %d", i).c_str());
            if (i == 2)
                ctx->GenericVars.Float1 = ImGui::GetCursorPosY();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ImGuiContext& g = *ctx->UiContext;
        ImGuiWindow* window = ctx->GetWindowByRef("");

        // Test page up/page down/home/end keys WITHOUT any navigable items.
        ctx->GenericVars.Step = 0;
        IM_CHECK(window->ScrollMax.y > 0.0f);               // We have a scrollbar
        ImGui::SetScrollY(window, 0.0f);                    // Reset starting position.

        ctx->KeyPressMap(ImGuiKey_PageDown);                // Scrolled down some, but not to the bottom.
        IM_CHECK(0 < window->Scroll.y && window->Scroll.y < window->ScrollMax.y);
        ctx->KeyPressMap(ImGuiKey_End);                     // Scrolled all the way to the bottom.
        IM_CHECK(window->Scroll.y == window->ScrollMax.y);
        float last_scroll = window->Scroll.y;               // Scrolled up some, but not all the way to the top.
        ctx->KeyPressMap(ImGuiKey_PageUp);
        IM_CHECK(0 < window->Scroll.y && window->Scroll.y < last_scroll);
        ctx->KeyPressMap(ImGuiKey_Home);                    // Scrolled all the way to the top.
        IM_CHECK(window->Scroll.y == 0.0f);

        // Test page up/page down/home/end keys WITH navigable items.
        ctx->GenericVars.Step = 1;
        ctx->Yield(2);
        //g.NavId = 0;
        memset(window->NavRectRel, 0, sizeof(window->NavRectRel));
        ctx->KeyPressMap(ImGuiKey_PageDown);
        IM_CHECK(g.NavId == ctx->GetID("OK 0"));            // FIXME-NAV: Main layer had no focus, key press simply focused it. We preserve this behavior across multiple test runs in the same session by resetting window->NavRectRel.
        ctx->KeyPressMap(ImGuiKey_PageDown);
        IM_CHECK(g.NavId == ctx->GetID("OK 2"));            // Now focus changes to a last visible button.
        IM_CHECK(window->Scroll.y == 0.0f);                 // Window is not scrolled.
        ctx->KeyPressMap(ImGuiKey_PageDown);
        IM_CHECK(g.NavId == ctx->GetID("OK 5"));            // Focus item on the next "page".
        IM_CHECK(window->Scroll.y > 0.0f);
        ctx->KeyPressMap(ImGuiKey_End);                     // Focus last item.
        IM_CHECK(window->Scroll.y == window->ScrollMax.y);
        IM_CHECK(g.NavId == ctx->GetID("OK 19"));
        ctx->KeyPressMap(ImGuiKey_PageUp);
        IM_CHECK(g.NavId == ctx->GetID("OK 17"));           // Focus first item visible in the last section.
        IM_CHECK(window->Scroll.y == window->ScrollMax.y);  // Window is not scrolled.
        ctx->KeyPressMap(ImGuiKey_PageUp);
        IM_CHECK(g.NavId == ctx->GetID("OK 14"));           // Focus first item of previous "page".
        IM_CHECK(0 < window->Scroll.y && window->Scroll.y < window->ScrollMax.y); // Window is not scrolled.
        ctx->KeyPressMap(ImGuiKey_Home);
        IM_CHECK(g.NavId == ctx->GetID("OK 0"));           // Focus very first item.
        IM_CHECK(window->Scroll.y == 0.0f);                // Window is scrolled to the start.
    };
}

//-------------------------------------------------------------------------
