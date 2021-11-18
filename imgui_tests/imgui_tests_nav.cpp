// dear imgui
// (tests: tables, columns)

#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_tests.h"
#include "shared/imgui_utils.h"
#include "imgui_test_engine/imgui_te_engine.h"      // IM_REGISTER_TEST()
#include "imgui_test_engine/imgui_te_context.h"
#include "imgui_test_engine/thirdparty/Str/Str.h"

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
    // ## Test that toggling layer steals active id.
    // ## Test that toggling layer is canceled by character typing (#370)
    // ## Test that ESC closes a menu
    // All those are performed on child and popups windows as well.
    t = IM_REGISTER_TEST(e, "nav", "nav_menu_alt_key");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto menu_content = [ctx]()
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
        auto window_content = [ctx]()
        {
            ImGui::InputText("Input", ctx->GenericVars.Str1, IM_ARRAYSIZE(ctx->GenericVars.Str1));
            ctx->GenericVars.Id = ImGui::GetItemID();
        };

        switch (ctx->GenericVars.Step)
        {
        case 0:
            ImGui::Begin("Test window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
            menu_content();
            window_content();
            ImGui::End();
            break;
        case 1:
            ImGui::Begin("Test window", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::BeginChild("Child", ImVec2(500, 500), true, ImGuiWindowFlags_MenuBar);
            menu_content();
            window_content();
            ImGui::EndChild();
            ImGui::End();
            break;
        case 2:
            if (!ImGui::IsPopupOpen("Test window"))
                ImGui::OpenPopup("Test window");
            if (ImGui::BeginPopupModal("Test window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar))
            {
                menu_content();
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
        //ctx->SetRef("Test window");

        for (int step = 0; step < 3; step++)
        {
            ctx->LogDebug("Step %d", step);
            ctx->GenericVars.Step = step; // Enable modal popup?
            ctx->Yield();

            const ImGuiID input_id = ctx->GenericVars.Id; // "Input"
            ctx->NavMoveTo(input_id);
            ctx->SetRef(g.NavWindow->ID);

            IM_CHECK(g.OpenPopupStack.Size == (step == 2));
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

            // Test that toggling layer steals active id.
#if IMGUI_VERSION_NUM >= 18206
            ctx->NavMoveTo(input_id);
            ctx->NavInput();
            IM_CHECK_EQ(g.ActiveId, input_id);
            ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);
            IM_CHECK_EQ(g.ActiveId, (ImGuiID)0);
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Menu);
            ctx->KeyPressMap(ImGuiKey_Escape); // ESC to leave layer
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);
            IM_CHECK(g.NavId == input_id);

            // Test that toggling layer is canceled by character typing (#370)
            ctx->NavMoveTo(input_id);
            ctx->NavInput();
            ctx->KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);
            ctx->KeyChars("ABC");
            ctx->KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);
            IM_CHECK_EQ(g.ActiveId, input_id);
#endif
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
        ctx->NavEnableForWindow();

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
        ctx->NavEnableForWindow();
        IM_CHECK(g.NavId == ctx->GetID("/##Menu_00/New"));
        ctx->NavKeyPress(ImGuiNavInput_KeyUp_);
        IM_CHECK(g.NavId == ctx->GetID("/##Menu_00/Quit"));
        ctx->NavKeyPress(ImGuiNavInput_KeyDown_);
        IM_CHECK(g.NavId == ctx->GetID("/##Menu_00/New"));
    };

    // ## Test menu closing with left arrow key. (#4510)
    t = IM_REGISTER_TEST(e, "nav", "nav_menu_close");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(600, 600));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Menu"))
            {
                if (ImGui::BeginMenu("Submenu"))
                {
                    bool use_child = ctx->Test->ArgVariant == 1;
                    if (!use_child || ImGui::BeginChild("Child", ImVec2(100.f, 30.f), true))
                    {
                        if (ImGui::BeginTabBar("Tabs"))
                        {
                            if (ImGui::BeginTabItem("Tab 1"))
                                ImGui::EndTabItem();
                            if (ImGui::BeginTabItem("Tab 2"))
                                ImGui::EndTabItem();
                            ImGui::EndTabBar();
                        }
                    }
                    if (use_child)
                        ImGui::EndChild();
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
        ImGuiID child_id = ctx->GetChildWindowID("##Menu_01", "Child");

        for (int variant = 0; variant < 2; variant++)
        {
            ctx->Test->ArgVariant = variant;
            ctx->LogDebug("Variant: %d", variant);
            ctx->SetRef("Test Window");
            ctx->MenuClick("Menu/Submenu");
            if (variant == 0)
                ctx->SetRef("##Menu_01");
            else
                ctx->SetRef(child_id);
            ctx->ItemClick("Tabs/Tab 1");
            ctx->NavKeyPress(ImGuiNavInput_KeyRight_);          // Activate nav, navigate to next tab
#if IMGUI_VERSION_NUM < 18414
            if (g.NavId == ctx->GetID("Tabs/Tab 1"))
                ctx->NavKeyPress(ImGuiNavInput_KeyRight_);      // FIXME-NAV: NavInit() prevents navigation to Tab 2 on the very first try.
#endif
            IM_CHECK(g.NavId == ctx->GetID("Tabs/Tab 2"));
            ctx->NavKeyPress(ImGuiNavInput_KeyLeft_);           // Navigate to first tab, not closing menu
            IM_CHECK(g.NavId == ctx->GetID("Tabs/Tab 1"));
            if (variant == 1)
            {
                ctx->NavKeyPress(ImGuiNavInput_KeyLeft_);       // Navigation fails, not closing menu
                IM_CHECK(g.NavId == ctx->GetID("Tabs/Tab 1"));
                ctx->KeyPressMap(ImGuiKey_Escape);              // Exit child window nav
            }
            ctx->NavKeyPress(ImGuiNavInput_KeyLeft_);           // Close 2nd level menu
            IM_CHECK_STR_EQ(g.NavWindow->Name, "##Menu_00");
            ctx->NavKeyPress(ImGuiNavInput_KeyLeft_);           // Close 1st level menu
            IM_CHECK_STR_EQ(g.NavWindow->Name, "Test Window");
        }
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

        // FIXME-TESTS: Facilitate usage of variants
        const int test_count = ctx->HasDock ? 2 : 1;
        for (int test_n = 0; test_n < test_count; test_n++)
        {
            ctx->LogDebug("TEST CASE %d", test_n);
#ifdef IMGUI_HAS_DOCK
            ctx->DockClear("Window 1", "Window 2", NULL);
            if (test_n == 1)
                ctx->DockInto("Window 1", "Window 2");
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
            ctx->ItemClick(ctx->GetID("Button In", ctx->GetChildWindowID("Window 2", "Child")));

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
            ctx->DockClear("Window 1", "Window 2", NULL);
            if (test_n == 1)
                ctx->DockInto("Window 2", "Window 1");
#endif

            ImGuiTestRef win1_button_ref("Window 1/Button 1");
            ImGuiTestRef win2_button_ref(ctx->GetID("Button 2", ctx->GetChildWindowID("Window 2", "Child")));

            // Focus Window 1, navigate to the button
            ctx->WindowFocus("Window 1");
            ctx->NavMoveTo(win1_button_ref);

            // Focus Window 2, ensure nav id was changed, navigate to the button
            ctx->WindowFocus("Window 2");
            IM_CHECK_NE(ctx->GetID(win1_button_ref), g.NavId);
            ctx->NavMoveTo(win2_button_ref);

            // Ctrl+Tab back to previous window, check if nav id was restored
            ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Ctrl);
            IM_CHECK_EQ(ctx->GetID(win1_button_ref), g.NavId);

            // Ctrl+Tab back to previous window, check if nav id was restored
            ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Ctrl);
            IM_CHECK_EQ(ctx->GetID(win2_button_ref), g.NavId);
        }
    };

    // ## Test that CTRL+Tabbing into a window select its nav layer if it is the only one available
    // FIXME: window appearing currently doesn't do the same
    t = IM_REGISTER_TEST(e, "nav", "nav_ctrl_tab_auto_menu_layer");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
                ImGui::EndMenu();
            if (ImGui::BeginMenu("Edit"))
                ImGui::EndMenu();
            ImGui::EndMenuBar();
        }
        ImGui::Text("Hello");
        ImGui::End();

        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Button("Hello");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->WindowFocus("Window 1");
        ctx->WindowFocus("Window 2");
        ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Ctrl);
        IM_CHECK_STR_EQ(g.NavWindow->Name, "Window 1");
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Menu);
        ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Ctrl);
        IM_CHECK_STR_EQ(g.NavWindow->Name, "Window 2");
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Main);
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

    // ## Test remote ActivateItem()
    t = IM_REGISTER_TEST(e, "nav", "nav_activate");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGuiID activate_target = ctx->GetID((vars.Step == 0) ? "/Test Window/Button" : "/Test Window/InputText");

        if (ImGui::Button("Activate Src 0"))
            ImGui::ActivateItem(activate_target);
        if (ImGui::Button("Button"))
            vars.Int1++;
        ImGui::SameLine();
        ImGui::Text("%d", vars.Int1);
        ImGui::InputText("InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        if (ImGui::Button("Activate Src 1"))
            ImGui::ActivateItem(activate_target);
        ImGui::End();

        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Activate Src 2"))
            ImGui::ActivateItem(activate_target);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GenericVars;
        IM_CHECK(vars.Int1 == 0);
        ctx->SetRef("Test Window");

        vars.Step = 0;
        ctx->Yield();
        ctx->ItemClick("Activate Src 0");
        IM_CHECK(vars.Int1 == 1);
        ctx->ItemClick("Activate Src 1");
        IM_CHECK(vars.Int1 == 2);
        ctx->ItemClick("/Window 2/Activate Src 2");
        IM_CHECK(vars.Int1 == 3);

        vars.Step = 1;
        ctx->Yield();
        ctx->ItemClick("Activate Src 0");
        IM_CHECK(g.NavId == ctx->GetID("/Test Window/InputText"));
        IM_CHECK(g.ActiveId == ctx->GetID("/Test Window/InputText"));
        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK(g.ActiveId == 0);
        ctx->ItemClick("/Window 2/Activate Src 2");
        IM_CHECK(g.NavId == ctx->GetID("/Test Window/InputText"));
        IM_CHECK(g.ActiveId == ctx->GetID("/Test Window/InputText"));
    };

#if IMGUI_VERSION_NUM >= 18412
    // ## Test NavInput (#2321 and many others)
    t = IM_REGISTER_TEST(e, "nav", "nav_input");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        if (ImGui::Button("Button"))
            vars.Int1++;
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GenericVars;

        ctx->SetRef("Test Window");
        ctx->NavMoveTo("InputText");

        // Cross/Activate to apply + release ActiveId
        ctx->NavInput();
        IM_CHECK(g.ActiveId == ctx->GetID("/Test Window/InputText"));
        ctx->KeyCharsReplaceEnter("0");
        IM_CHECK_STR_EQ(vars.Str1, "0");
        ctx->NavInput();
        ctx->KeyCharsReplace("123");
        IM_CHECK_STR_EQ(vars.Str1, "123");
        ctx->NavKeyPress(ImGuiNavInput_Activate);
        IM_CHECK(g.ActiveId == 0);
        IM_CHECK_STR_EQ(vars.Str1, "123");

        // Circle/Cancel to revert + release ActiveId
        ctx->NavInput();
        ctx->KeyCharsReplaceEnter("0");
        IM_CHECK_STR_EQ(vars.Str1, "0");
        ctx->NavInput();
        ctx->KeyCharsReplace("123");
        ctx->NavKeyPress(ImGuiNavInput_Cancel);
        IM_CHECK(g.ActiveId == 0);
        IM_CHECK_STR_EQ(vars.Str1, "0");

        // Triangle/Input again to release active id + apply?
        ctx->NavInput();
        ctx->KeyCharsReplaceEnter("0");
        IM_CHECK_STR_EQ(vars.Str1, "0");
        ctx->NavInput();
        ctx->KeyCharsReplace("123");
        ctx->NavInput();
        IM_CHECK(g.ActiveId == 0);
        IM_CHECK_STR_EQ(vars.Str1, "123");

        // Verify that NavInput doesn't trigger other fields
        vars.Int1 = 0;
        ctx->NavMoveTo("Button");
        ctx->NavInput();
        IM_CHECK(vars.Int1 == 0);
    };
#endif

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
            ctx->DockClear("Window 1", "Window 2", NULL);
            if (test_n == 1)
                ctx->DockInto("Window 2", "Window 1");
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
            ctx->DockClear("Dear ImGui Demo", "Hello, world!", NULL);
            if (test_n == 0)
                ctx->DockInto("Dear ImGui Demo", "Hello, world!");
#endif
            ctx->SetRef("Dear ImGui Demo");
            ctx->ItemCloseAll("");

            // Test simple focus restoration.
            ctx->NavMoveTo("Configuration");                            // Focus item.
            ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);     // Focus menu.
            IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Menu);
            ctx->NavActivate();                                         // Open menu, focus first item in the menu.
            ctx->NavActivate();                                         // Activate first item in the menu.
            IM_CHECK_EQ(g.NavId, ctx->GetID("Configuration"));          // Verify NavId was restored to initial value.

            // Test focus restoration to the item within a child window.
            // FIXME-TESTS: Feels like this would be simpler with our own GuiFunc.
            ctx->NavMoveTo("Layout & Scrolling");                       // Navigate to the child window.
            ctx->NavActivate();
            ctx->NavMoveTo("Scrolling");
            ctx->NavActivate(); // FIXME-TESTS: Could query current g.NavWindow instead of making names?
            ImGuiWindow* child_window = ctx->GetWindowByRef(ctx->GetChildWindowID("Scrolling/scrolling"));
            ctx->SetRef(child_window->ID);
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

        for (int mouse_button = 0; mouse_button < 2; mouse_button++)
        {
            ctx->ItemOpen("Help");
            ctx->ItemClose("Help");
            IM_CHECK_EQ(g.NavId, ctx->GetID("Help"));
            IM_CHECK_EQ(g.NavWindow, ctx->GetWindowByRef(ctx->RefID));
            IM_CHECK(g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard); // FIXME-TESTS: Should test for both cases.
            IM_CHECK(g.IO.WantCaptureMouse == true);
            // FIXME-TESTS: This depends on ImGuiConfigFlags_NavNoCaptureKeyboard being cleared. Should test for both cases.
            IM_CHECK(g.IO.WantCaptureKeyboard == true);

            ctx->MouseClickOnVoid(mouse_button);
            if (mouse_button == 0)
            {
                //IM_CHECK(g.NavId == 0); // Clarify specs
                IM_CHECK(g.NavWindow == NULL);
                IM_CHECK(g.IO.WantCaptureMouse == false);
                IM_CHECK(g.IO.WantCaptureKeyboard == false);
            }
        }
    };

    // ## Test inheritance (and lack of) of FocusScope
    // FIXME-TESTS: could test for actual propagation of focus scope from<>into nav data
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_scope");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

#if IMGUI_VERSION_NUM >= 18308
        ImGuiID unset_scope_id = ImGui::GetID("#FOCUSSCOPE");
#else
        ImGuiID unset_scope_id = 0u;
#endif
        ImGuiID focus_scope_id = ImGui::GetID("MyScope");

        IM_CHECK_EQ(ImGui::GetFocusScope(), unset_scope_id);
        ImGui::BeginChild("Child 1", ImVec2(100, 100));
        IM_CHECK_EQ(ImGui::GetFocusScope(), unset_scope_id);
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
        IM_CHECK_EQ(ImGui::GetFocusScope(), ImGui::GetID("#FOCUSSCOPE"));
        ImGui::End();

        ImGui::PopFocusScope();

        IM_CHECK_EQ(ImGui::GetFocusScope(), unset_scope_id);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
    };

    // ## Check setting default focus.
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_default");
    struct DefaultFocusVars { bool ShowWindows = true; bool SetFocus = false; bool MenuLayer = false; };
    t->SetUserDataType<DefaultFocusVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DefaultFocusVars& vars = ctx->GetUserData<DefaultFocusVars>();
        if (!vars.ShowWindows)
            return;

        ImGui::SetNextWindowSize(ImVec2(100, 100));
        ImGui::Begin("Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            ImGui::MenuItem("Item 1");
            ImGui::MenuItem("Item 2");
            if (vars.SetFocus && vars.MenuLayer)
                ImGui::SetItemDefaultFocus();
            ImGui::EndMenuBar();
        }
        static char buf[32];
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 200);
        ImGui::InputText("##1", buf, IM_ARRAYSIZE(buf));
        ImGui::InputText("##2", buf, IM_ARRAYSIZE(buf));
        if (vars.SetFocus && !vars.MenuLayer)
            ImGui::SetItemDefaultFocus();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        DefaultFocusVars& vars = ctx->GetUserData<DefaultFocusVars>();
        ImGuiWindow* window = ctx->GetWindowByRef("Window");

        // Variant 0: verify ##2 is not scrolled into view and ##1 is focused (SetItemDefaultFocus() not used).
        // Variant 1: verify ##2 is scrolled into view and focused (SetItemDefaultFocus() is used).
        for (int variant = 0; variant < 2; variant++)
        {
            ctx->LogDebug("Test variant: %s default focus override", variant ? "with" : "without");
            vars.SetFocus = (variant == 1);
            vars.ShowWindows = true;
            ctx->Yield(2);
            ctx->NavEnableForWindow();
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);
            if (!vars.SetFocus)
                IM_CHECK_EQ(g.NavId, ctx->GetID("Window/##1"));
            else
                IM_CHECK_EQ(g.NavId, ctx->GetID("Window/##2"));
            ImGuiTestItemInfo* input = ctx->ItemInfo("Window/##2");
            IM_CHECK_EQ(window->InnerClipRect.Contains(input->RectFull), vars.SetFocus);
            ImGui::SetScrollY(window, 0);   // Reset scrolling.
            vars.ShowWindows = false;
            ctx->Yield(2);
        }

#if IMGUI_BROKEN_TESTS
        // Verify that SetItemDefaultFocus() works in menu layer
        ctx->LogDebug("Test variant: in menu");
        vars.ShowWindows = vars.SetFocus = vars.MenuLayer = true;
        ctx->Yield(2);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window/##menubar/Item 2"));
        IM_CHECK(g.NavLayer == ImGuiNavLayer_Menu);
#endif
    };

    // ## Check setting default focus for multiple windows simultaneously.
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_default_multi");
    t->SetUserDataType<DefaultFocusVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DefaultFocusVars& vars = ctx->GetUserData<DefaultFocusVars>();
        if (!vars.ShowWindows)
            return;

        for (int n = 0; n < 4; n++)
        {
            ImGui::Begin(Str16f("Window %d", n + 1).c_str(), NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
            if (n < 3)
            {
                ImGui::Button("Button 1");
                ImGui::Button("Button 2");
                if (n >= 1)
                    ImGui::SetItemDefaultFocus();
                ImGui::Button("Button 3");
            }
            ImGui::End();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        DefaultFocusVars& vars = ctx->GetUserData<DefaultFocusVars>();

        ctx->NavEnableForWindow();
        vars.ShowWindows = true;
        ctx->Yield(2);
#if IMGUI_BROKEN_TESTS
        ImGuiWindow* window1 = ctx->GetWindowByRef("Window 1");
        ImGuiWindow* window2 = ctx->GetWindowByRef("Window 2");
        ImGuiWindow* window3 = ctx->GetWindowByRef("Window 3");
        IM_CHECK_EQ_NO_RET(window1->NavLastIds[0], ctx->GetID("Window 1/Button 1"));
        IM_CHECK_EQ_NO_RET(window2->NavLastIds[0], ctx->GetID("Window 2/Button 2"));
        IM_CHECK_EQ_NO_RET(window3->NavLastIds[0], ctx->GetID("Window 3/Button 3"));
#endif
        ImGuiWindow* window4 = ctx->GetWindowByRef("Window 4");
        IM_CHECK_EQ_NO_RET(window4->NavLastIds[0], (ImGuiID)0);
    };

    // ## Check usage of _NavFlattened flag
    t = IM_REGISTER_TEST(e, "nav", "nav_flattened");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        const auto& output_item = [](const char* label) { ImGui::Button(label); };
        //const auto& output_item = [](const char* label) { static char buf[32]; ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5); ImGui::InputText(label, buf, 32); };

        ImGui::BeginGroup();
        output_item("Button 1");
        output_item("Button 2");
        output_item("Button 3");
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::BeginChild("Child 1", ImVec2(200, 200), false, ImGuiWindowFlags_NavFlattened);
        output_item("Child 1 Button 1");
        output_item("Child 1 Button 2");
        output_item("Child 1 Button 3");
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Child 2", ImVec2(200, 200), false, ImGuiWindowFlags_NavFlattened);
        output_item("Child 2 Button 1");
        output_item("Child 2 Button 2");
        output_item("Child 2 Button 3");
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Child 3", ImVec2(200, 200), false, ImGuiWindowFlags_NavFlattened);
        ImGui::BeginChild("Child 3B", ImVec2(0, 0), false, ImGuiWindowFlags_NavFlattened);
        output_item("Child 3B Button 1");
        output_item("Child 3B Button 2");
        output_item("Child 3B Button 3");
        ImGui::EndChild();
        ImGui::EndChild();

        // FIXME-NAV: To test PageUp/PageDown/Home/End later
        //ImGui::BeginChild("Child 4", ImVec2(200, 200), false, ImGuiWindowFlags_NavFlattened);
        //output_item("Child 4 Button 1");
        //output_item("Child 4 Button 2");
        //output_item("Child 4 Button 3");
        //ImGui::EndChild();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->NavEnableForWindow();

        // Navigating in child
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window 1/Button 1"));
        ctx->KeyPressMap(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window 1/Button 2"));
        ctx->KeyPressMap(ImGuiKey_RightArrow);

        // Parent -> Child
        IM_CHECK_EQ(g.NavId, ctx->GetID("Child 1 Button 2", g.NavWindow->ID));
        ctx->KeyPressMap(ImGuiKey_UpArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Child 1 Button 1", g.NavWindow->ID));

        // Child -> Parent
        ctx->KeyPressMap(ImGuiKey_LeftArrow);
        ctx->KeyPressMap(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window 1/Button 2"));

        // Parent -> Child -> Child
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Child 2 Button 2", g.NavWindow->ID));

        // Child -> nested Child
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Child 3B Button 2", g.NavWindow->ID));

        // FIXME: test PageUp/PageDown on child
    };

    // ## Check default focus with _NavFlattened flag
    t = IM_REGISTER_TEST(e, "nav", "nav_flattened_focus_default");
    t->SetUserDataType<DefaultFocusVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DefaultFocusVars& vars = ctx->GetUserData<DefaultFocusVars>();
        if (!vars.ShowWindows)
            return;

        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::BeginChild("Child 0", ImVec2(200, 200), false, ImGuiWindowFlags_NavFlattened);
        ImGui::Button("Child 0 Button 1");
        ImGui::Button("Child 0 Button 2");
        ImGui::Button("Child 0 Button 3");
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Button("Button 1");
        ImGui::Button("Button 2");
        ImGui::Button("Button 3");
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::BeginChild("Child 1", ImVec2(200, 200), false, ImGuiWindowFlags_NavFlattened);
        ImGui::Button("Child 1 Button 1");
        ImGui::Button("Child 1 Button 2");
        if (vars.SetFocus)
            ImGui::SetItemDefaultFocus();
        ImGui::Button("Child 1 Button 3");
        ImGui::EndChild();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        DefaultFocusVars& vars = ctx->GetUserData<DefaultFocusVars>();
        ctx->NavEnableForWindow();

        // Test implicit default init on flattened window
        vars.ShowWindows = true;
        vars.SetFocus = false;
        ctx->Yield(2);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Child 0 Button 1", ctx->GetChildWindowID("Window 1", "Child 0")));
        vars.ShowWindows = false;
        ctx->Yield(2);

        // Test using SetItemDefaultFocus() on flattened window
        vars.ShowWindows = true;
        vars.SetFocus = true;
        ctx->Yield(2);
#if IMGUI_VERSION_NUM >= 18205
        IM_CHECK_EQ(g.NavId, ctx->GetID("Child 1 Button 2", ctx->GetChildWindowID("Window 1", "Child 1")));
#endif
    };

    // ## Check nav keyboard/mouse highlight flags
    t = IM_REGISTER_TEST(e, "nav", "nav_highlight");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Dear ImGui Demo");
        ctx->MouseMove("Help");
        IM_CHECK(g.NavDisableHighlight == true);
        IM_CHECK(g.NavDisableMouseHover == false);
        ctx->NavMoveTo("Configuration");
        IM_CHECK(g.NavDisableHighlight == false);
        IM_CHECK(g.NavDisableMouseHover == true);
        ctx->NavKeyPress(ImGuiNavInput_KeyUp_);
        IM_CHECK(g.NavId == ctx->GetID("Help"));
        IM_CHECK(g.NavDisableHighlight == false);
        IM_CHECK(g.NavDisableMouseHover == true);
        ctx->MouseMove("Help");
        IM_CHECK(g.NavDisableHighlight == false); // Moving mouse doesn't set this to true: rect will be visible but NavId not marked as "hovered"
        IM_CHECK(g.NavDisableMouseHover == false);

        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Alt);
        IM_CHECK(g.NavDisableHighlight == false);
        IM_CHECK(g.NavDisableMouseHover == true);
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
#if IMGUI_VERSION_NUM < 18504
            ImRect item_rect = window->NavRectRel[ImGuiNavLayer_Main];
            item_rect.Translate(window->Pos);
            return item_rect;
#else
            return ImGui::WindowRectRelToAbs(window, window->NavRectRel[ImGuiNavLayer_Main]);
#endif
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

    // ## Test PageUp/PageDown/Home/End/arrow keys
    t = IM_REGISTER_TEST(e, "nav", "nav_page_home_end_arrows");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        if (ctx->RunFlags & ImGuiTestRunFlags_GuiFuncOnly)
        {
            ImGui::Begin("Config", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::Checkbox("UseClipper", &vars.UseClipper);
            ImGui::End();
        }

        ImGui::SetNextWindowSize(ImVec2(100, vars.Size.y), ImGuiCond_Always);

        // FIXME-NAV: Lack of ImGuiWindowFlags_NoCollapse breaks window scrolling without activable items.
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoCollapse);
        vars.Size.y = ImFloor(ImGui::GetCursorPosY() + ImGui::GetTextLineHeightWithSpacing() * 20.8f + ImGui::GetStyle().ScrollbarSize);
        if (vars.UseClipper)
        {
            ImGuiListClipper clipper;
            clipper.Begin(200);
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    ImGui::SmallButton(Str16f("OK %d", i).c_str());
                    ImGui::SameLine();
                    ImGui::SmallButton(Str16f("OK B%d", i).c_str());
                }
            }
        }
        else
        {
            for (int i = 0; i < 200; i++)
            {
                ImGui::SmallButton(Str16f("OK %d", i).c_str());
                ImGui::SameLine();
                ImGui::SmallButton(Str16f("OK B%d", i).c_str());
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ctx->SetRef("Test Window");
        ImGuiWindow* window = ctx->GetWindowByRef("");

        for (int step = 0; step < 2; step++)
        {
            vars.UseClipper = (step == 1);
            ctx->LogInfo("STEP %d", step);

            // Test page up/page down/home/end keys WITH navigable items.
            ctx->Yield(2);
            g.NavId = 0;
            //memset(window->NavRectRel, 0, sizeof(window->NavRectRel));
            ctx->ScrollToTop();

#if IMGUI_VERSION_NUM < 18503
            ctx->KeyPressMap(ImGuiKey_PageDown);
            IM_CHECK(g.NavId == ctx->GetID("OK 0"));            // FIXME-NAV: Main layer had no focus, key press simply focused it. We preserve this behavior across multiple test runs in the same session by resetting window->NavRectRel.
#endif
            ctx->KeyPressMap(ImGuiKey_PageDown);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 20"));          // Now focus changes to a last visible button.
            IM_CHECK(window->Scroll.y == 0.0f);                 // Window is not scrolled.
            ctx->KeyPressMap(ImGuiKey_PageDown);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 40"));          // Focus item on the next "page".
            IM_CHECK(window->Scroll.y > 0.0f);
            ctx->KeyPressMap(ImGuiKey_End);                     // Focus last item.
            IM_CHECK(window->Scroll.y == window->ScrollMax.y);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 199"));
            ctx->KeyPressMap(ImGuiKey_PageUp);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 179"));         // Focus first item visible in the last section.
            IM_CHECK(window->Scroll.y == window->ScrollMax.y);  // Window is not scrolled.
            ctx->KeyPressMap(ImGuiKey_PageUp);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 159"));         // Focus first item of previous "page".
            IM_CHECK(0 < window->Scroll.y && window->Scroll.y < window->ScrollMax.y); // Window is not scrolled.
            ctx->KeyPressMap(ImGuiKey_RightArrow);
            IM_CHECK(window->Scroll.x > 0.0f);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK B159"));        // Focus first item in a second column, scrollbar moves.
            ctx->KeyPressMap(ImGuiKey_LeftArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 159"));         // Focus first item in a second column, scrollbar moves.
            ctx->KeyPressMap(ImGuiKey_Home);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 0"));           // Focus very first item.
            IM_CHECK_EQ(window->Scroll.y, 0.0f);                // Window is scrolled to the start.

            // Test arrows keys WITH navigable items.
            // (This is a duplicate of most basic nav tests)
            ctx->KeyPressMap(ImGuiKey_DownArrow);
            IM_CHECK(window->Scroll.y == 0.0f);
            IM_CHECK(g.NavId == ctx->GetID("OK 1"));            // Focus second item without moving scrollbar.
            ctx->KeyPressMap(ImGuiKey_UpArrow);
            IM_CHECK(window->Scroll.y == 0.0f);
            IM_CHECK(g.NavId == ctx->GetID("OK 0"));            // Focus first item without moving scrollbar.
            IM_CHECK(window->Scroll.x == 0.0f);
            ctx->KeyPressMap(ImGuiKey_RightArrow);
            IM_CHECK(window->Scroll.x > 0.0f);
            IM_CHECK(g.NavId == ctx->GetID("OK B0"));           // Focus first item in a second column, scrollbar moves.
            ctx->KeyPressMap(ImGuiKey_LeftArrow);
            IM_CHECK_EQ(window->Scroll.x, 0.0f);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 0"));           // Focus first item in first column, scrollbar moves back.
        }
    };

    // ## Test PageUp/PageDown/Home/End/arrow keys
    t = IM_REGISTER_TEST(e, "nav", "nav_page_home_end_arrows_scroll_only");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        if (ctx->RunFlags & ImGuiTestRunFlags_GuiFuncOnly)
        {
            ImGui::Begin("Config", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::Checkbox("UseClipper", &vars.UseClipper);
            ImGui::End();
        }

        // FIXME-NAV: Lack of ImGuiWindowFlags_NoCollapse breaks window scrolling without activable items.
        ImGui::SetNextWindowSize(ImVec2(100, vars.Size.y), ImGuiCond_Always);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoCollapse);
        vars.Size.y = ImFloor(ImGui::GetCursorPosY() + ImGui::GetFrameHeightWithSpacing() * 3.0f + ImGui::GetStyle().ScrollbarSize);
        if (vars.UseClipper)
        {
            ImGuiListClipper clipper;
            clipper.Begin(20);
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    ImGui::TextUnformatted(Str16f("OK %d", i).c_str());
                    ImGui::SameLine();
                    ImGui::TextUnformatted(Str16f("OK 5%d", i).c_str());
                }
            }
        }
        else
        {
            for (int i = 0; i < 20; i++)
            {
                ImGui::TextUnformatted(Str16f("OK %d", i).c_str());
                ImGui::SameLine();
                ImGui::TextUnformatted(Str16f("OK 5%d", i).c_str());
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiWindow* window = ctx->GetWindowByRef("");

        for (int step = 0; step < 2; step++)
        {
            vars.UseClipper = (step == 1);
            ctx->LogInfo("STEP %d", step);

            // Test page up/page down/home/end keys WITHOUT any navigable items.
            IM_CHECK(window->ScrollMax.y > 0.0f);               // We have a scrollbar
            ImGui::SetScrollY(window, 0.0f);                    // Reset starting position.
            ImGui::SetScrollX(window, 0.0f);

            ctx->KeyHoldMap(ImGuiKey_PageDown, 0, 0.1f);        // Scrolled down some, but not to the bottom.
            IM_CHECK(0 < window->Scroll.y && window->Scroll.y < window->ScrollMax.y);
            ctx->KeyPressMap(ImGuiKey_End);                     // Scrolled all the way to the bottom.
            IM_CHECK_EQ(window->Scroll.y, window->ScrollMax.y);
            float last_scroll = window->Scroll.y;               // Scrolled up some, but not all the way to the top.
            ctx->KeyHoldMap(ImGuiKey_PageUp, 0, 0.1f);
            IM_CHECK(0 < window->Scroll.y && window->Scroll.y < last_scroll);
            ctx->KeyPressMap(ImGuiKey_Home);                    // Scrolled all the way to the top.
            IM_CHECK_EQ(window->Scroll.y, 0.0f);

            // Test arrows keys WITHOUT any navigable items
            ctx->KeyHoldMap(ImGuiKey_DownArrow, 0, 0.1f);       // Scrolled window down by one tick.
            IM_CHECK_GT(window->Scroll.y, 0.0f);
            ctx->KeyHoldMap(ImGuiKey_UpArrow, 0, 0.1f);         // Scrolled window up by one tick (back to the top).
            IM_CHECK_EQ(window->Scroll.y, 0.0f);
            IM_CHECK_EQ(window->Scroll.x, 0.0f);
            ctx->KeyHoldMap(ImGuiKey_RightArrow, 0, 0.1f);      // Scrolled window right by one tick.
            IM_CHECK_GT(window->Scroll.x, 0.0f);
            ctx->KeyHoldMap(ImGuiKey_LeftArrow, 0, 0.1f);       // Scrolled window left by one tick (back to the start).
            IM_CHECK_EQ(window->Scroll.x, 0.0f);
        }
    };

    // ## Test using TAB to cycle through items
    t = IM_REGISTER_TEST(e, "nav", "nav_tabbing_basic");
    struct TabbingVars { int Step = 0; int WidgetType = 0; float Floats[5] = { 0, 0, 0, 0, 0 }; char Bufs[256][5] = { "buf0", "buf1", "buf2", "buf3", "buf4" }; };
    t->SetUserDataType<TabbingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetUserData<TabbingVars>();

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;
        if (vars.Step == 0)
        {
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos + ImVec2(10, 10));
            flags |= ImGuiWindowFlags_AlwaysAutoResize;
        }
        if (vars.Step == 1)
            ImGui::SetNextWindowSize(ImVec2(100, 1)); // Ensure items are clipped
        ImGui::Begin("Test Window", NULL, flags);

        //ImGui::SetNextItemWidth(50);
        //ImGui::SliderInt("WidgetType", &vars.WidgetType, 0, 3);
        for (int n = 0; n < 10; n++)
            ImGui::Text("Filler");

        auto out_widget = [&vars](const char* label, int n)
        {
            if (vars.WidgetType == 0)
                ImGui::InputText(label, vars.Bufs[n], IM_ARRAYSIZE(vars.Bufs[n]));
            if (vars.WidgetType == 1)
                ImGui::SliderFloat(label, &vars.Floats[n], 0.0f, 1.0f);
            if (vars.WidgetType == 2)
                ImGui::DragFloat(label, &vars.Floats[n], 0.1f, 0.0f, 1.0f);
            if (vars.WidgetType == 3)
                ImGui::InputTextMultiline(label, vars.Bufs[n], IM_ARRAYSIZE(vars.Bufs[n]), ImVec2(0, ImGui::GetTextLineHeight() * 2));
        };

        out_widget("Item0", 0);
        out_widget("Item1", 1);
        if (vars.Step == 1)
            IM_CHECK(!ImGui::IsItemVisible());
        ImGui::PushAllowKeyboardFocus(false);
        out_widget("Item2", 2);
        ImGui::PopAllowKeyboardFocus();
        out_widget("Item3", 3);
        out_widget("Item4", 4);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GetUserData<TabbingVars>();

        ctx->SetRef("Test Window");
        for (vars.WidgetType = 0; vars.WidgetType < 4; vars.WidgetType++)
            for (vars.Step = 0; vars.Step < 2; vars.Step++)
            {
#if !IMGUI_BROKEN_TESTS
                if (vars.Step == 1) // Tabbing through clipped items is yet unsupported
                    continue;
#endif

                // Step 0: Make sure tabbing works on unclipped widgets
                // Step 1: Make sure tabbing works on clipped widgets
                ctx->LogDebug("STEP %d WIDGET %d", vars.Step, vars.WidgetType);

                IM_CHECK_EQ(g.ActiveId, (ImGuiID)0);
#if IMGUI_VERSION_NUM >= 18208
                ctx->KeyPressMap(ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item0"));
#endif
                ctx->KeyPressMap(ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item1"));
                ctx->KeyPressMap(ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item3"));
                ctx->KeyPressMap(ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item4"));

                // Test wrapping
                ctx->KeyPressMap(ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item0"));

                // Test Shift+Tab
                ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Shift);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item4"));
                ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Shift);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item3"));
                ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Shift);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item1"));
                ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Shift);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item0"));

                ctx->KeyPressMap(ImGuiKey_Escape);
            }
    };

    // ## Test tabbing through clipped/non-visible items (#4449) + using ImGuiListClipper
#if IMGUI_VERSION_NUM >= 18508
    t = IM_REGISTER_TEST(e, "nav", "nav_tabbing_clipped");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ImGui::SetNextWindowSize(ImVec2(400, 100)); // Ensure items are clipped
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        /*if (vars.Step == 0)
        {
            for (int n = 0; n < 50; n++)
                ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);
        }
        else*/
        {
            ImGuiListClipper clipper;
            clipper.Begin(50);
            while (clipper.Step())
            {
                for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                    ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);
            }

        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Test Window");
        for (int n = 0; n < 52; n++)
        {
            ctx->KeyPressMap(ImGuiKey_Tab);
            IM_CHECK_EQ(g.ActiveId, ctx->GetID(Str30f("Input%d", n % 50).c_str()));
        }
        // Should be on 51
        for (int n = 0; n < 4; n++)
        {
            IM_CHECK_EQ(g.ActiveId, ctx->GetID(Str30f("Input%d", (51 - n) % 50).c_str()));
            ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Shift);
            IM_CHECK_EQ(g.ActiveId, ctx->GetID(Str30f("Input%d", (50 - n) % 50).c_str()));
        }
    };
#endif

    // ## Test tabbing through flattened windows (cad790d4)
    // ## Also test for clipped windows item not being clipped when tabbing through a normally clipped child (e790fc0e)
#if IMGUI_VERSION_NUM >= 18510
    t = IM_REGISTER_TEST(e, "nav", "nav_tabbing_flattened");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        for (int n = 0; n < 4; n++)
            ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);

        ImGui::BeginChild("Child 1", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 3), true, ImGuiWindowFlags_NavFlattened);
        for (int n = 4; n < 8; n++)
            ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);
        ImGui::EndChild();

        for (int n = 8; n < 10; n++)
            ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);

        ImGui::BeginChild("Child 2", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 3), true, ImGuiWindowFlags_NavFlattened);
        for (int n = 10; n < 14; n++)
            ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);
        ImGui::BeginChild("Child 3", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 3), true, ImGuiWindowFlags_NavFlattened);
        for (int n = 14; n < 18; n++)
            ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);
        ImGui::EndChild();
        for (int n = 18; n < 20; n++)
            ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);
        ImGui::EndChild();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestItemInfo* item_info = NULL;
        ctx->SetRef("Test Window");
        for (int n = 0; n < 22; n++)
        {
            ctx->KeyPressMap(ImGuiKey_Tab);
            item_info = ctx->ItemInfo(g.ActiveId);
            IM_CHECK(item_info != NULL);
            IM_CHECK_STR_EQ(item_info->DebugLabel, Str30f("Input%d", n % 20).c_str());
        }
        // Should be on 21
        for (int n = 0; n < 4; n++)
        {
            item_info = ctx->ItemInfo(g.ActiveId);
            IM_CHECK(item_info != NULL);
            IM_CHECK_STR_EQ(item_info->DebugLabel, Str30f("Input%d", (21 - n) % 20).c_str());

            ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Shift);

            item_info = ctx->ItemInfo(g.ActiveId);
            IM_CHECK(item_info != NULL);
            IM_CHECK_STR_EQ(item_info->DebugLabel, Str30f("Input%d", (20 - n) % 20).c_str());
        }
    };
#endif

    // ## Test SetKeyboardFocusHere()
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_api");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGuiTestGenericVars& vars = ctx->GenericVars;
        if (vars.Step == 1)
        {
            ImGui::SetKeyboardFocusHere();
            ImGui::InputText("Text1", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            vars.Status.QuerySet();
        }
        else if (vars.Step == 2)
        {
            ImGui::InputText("Text2", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            vars.Status.QuerySet();
            ImGui::SetKeyboardFocusHere(-1);
        }
        else if (vars.Step == 3)
        {
            // Multiple overriding calls in same frame (last gets it)
            ImGui::SetKeyboardFocusHere();
            ImGui::InputText("Text1", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            ImGui::SetKeyboardFocusHere();
            ImGui::InputText("Text2", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        }
        else if (vars.Step == 4)
        {
            // Sub-component
            ImGui::SetKeyboardFocusHere(2);
            ImGui::SliderFloat4("Float4", &vars.FloatArray[0], 0.0f, 1.0f);
        }
        else if (vars.Step == 5)
        {
            if (vars.Bool1)
            {
                ImGui::SetKeyboardFocusHere();
                ImGui::InputText("Text1", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            }
            ImGui::InputText("NoFocus1", vars.Str2, IM_ARRAYSIZE(vars.Str2));
            vars.Status.QuerySet();
        }
        else if (vars.Step == 6)
        {
            if (vars.Bool1)
            {
                ImGui::InputText("Text1", vars.Str1, IM_ARRAYSIZE(vars.Str1));
                ImGui::SetKeyboardFocusHere(-1);
            }
            ImGui::InputText("NoFocus1", vars.Str2, IM_ARRAYSIZE(vars.Str2));
            vars.Status.QuerySet();
        }
        else if (vars.Step == 7)
        {
            ImGui::PushAllowKeyboardFocus(false);
            ImGui::SetKeyboardFocusHere();
            ImGui::InputText("Text1", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            vars.Status.QuerySet();
            ImGui::PopAllowKeyboardFocus();
        }
        else if (vars.Step == 8)
        {
            ImGui::SetKeyboardFocusHere();
            ImGui::Button("Button1");
            vars.Status.QuerySet();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Test Window");

        // Test focusing next item with SetKeyboardFocusHere(0)
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        vars.Step = 1;
        ctx->Yield();
        IM_CHECK(g.ActiveId == 0);
        IM_CHECK(vars.Status.Activated == 0);
        ctx->Yield();
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("Text1"));
        IM_CHECK(vars.Status.Activated == 1);

        // Test that ActiveID gets cleared when not alive
        vars.Step = 0;
        ctx->Yield(2);
        IM_CHECK(g.ActiveId == 0);

        // Test focusing previous item with SetKeyboardFocusHere(-1)
        vars.Step = 2;
        ctx->Yield();
        IM_CHECK(g.ActiveId == 0);
        IM_CHECK(vars.Status.Activated == 0);
        ctx->Yield();
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("Text2"));
        IM_CHECK_EQ(vars.Status.Activated, 1);

        // Test multiple overriding calls to SetKeyboardFocusHere() in same frame (last gets it)
        vars.Step = 0;
        ctx->Yield(2);
        vars.Step = 3;
        ctx->Yield(2);
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("Text2"));

        // Test accessing a sub-component
        vars.Step = 0;
        ctx->Yield(2);
        vars.Step = 4;
        ctx->Yield(2);
        int field_idx = 2;
        IM_CHECK_EQ(g.ActiveId, ImHashData(&field_idx, sizeof(int), ctx->GetID("Float4")));

#if IMGUI_VERSION_NUM >= 18420
        // Test focusing next item when it disappears (#432)
        vars.Step = 5;
        vars.Bool1 = true;
        ctx->Yield(2);
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("Text1"));
        IM_CHECK_EQ(vars.Status.Activated, 0);
        vars.Bool1 = false;
        ctx->Yield();
        IM_CHECK_EQ(vars.Status.Activated, 0);
        vars.Step = 0;
        ctx->Yield();

        // Test focusing previous item when it disappears.
        vars.Step = 6;
        vars.Bool1 = true;
        ctx->Yield(2);
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("Text1"));
        IM_CHECK_EQ(vars.Status.Activated, 0);
        vars.Bool1 = false;
        ctx->Yield();
#if IMGUI_VERSION_NUM >= 18507
        ctx->Yield();
#endif
        IM_CHECK_EQ(g.ActiveId, 0u);
        IM_CHECK_EQ(vars.Status.Activated, 0);
#endif

        // Test API focusing an item that has PushAllowKeyboardFocus(false)
        vars.Step = 7;
        ctx->Yield(2);
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("Text1"));
        IM_CHECK_EQ(vars.Status.Activated, 1);

        // While we are there verify that we can use the InputText() while repeatedly refocusing it (issue #4682)
#if IMGUI_VERSION_NUM >= 18507
        vars.Str1[0] = 0;
        //ctx->ItemNavActivate("Text1"); // Unnecessary and anyway will assert as focus request already in progress, conflicting
        ctx->KeyCharsReplaceEnter("Hello");
        IM_CHECK_STR_EQ(vars.Str1, "Hello");
        //ctx->ItemNavActivate("Text1");
        ctx->KeyCharsReplaceEnter("");
        IM_CHECK_STR_EQ(vars.Str1, "");
#endif

        // Test that SetKeyboardFocusHere() on a Button() does not triggers clear.
#if IMGUI_VERSION_NUM >= 18508
        vars.Step = 8;
        vars.Bool1 = true;
        ctx->Yield(2);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Button1"));
        IM_CHECK_EQ(vars.Status.Activated, 0);
#endif
    };

#if IMGUI_VERSION_NUM >= 18420
    // ## Test SetKeyboardFocusHere() on clipped items (#343, #4079, #2352)
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_api_clipped");
    struct SetFocusVars { char Str1[256] = ""; int Step = 0; ImGuiTestGenericStatus Status[3]; };
    t->SetUserDataType<SetFocusVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        SetFocusVars& vars = ctx->GetUserData<SetFocusVars>();
        for (int n = 0; n < 50; n++)
            ImGui::Text("Dummy %d", n);

        ImGui::InputText("Text1", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        vars.Status[0].QuerySet(false);
        if (vars.Step == 1)
            ImGui::SetKeyboardFocusHere(-1);

        for (int n = 0; n < 50; n++)
            ImGui::Text("Dummy %d", n);

        ImGui::InputText("Text2", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        vars.Status[1].QuerySet(false);
        if (vars.Step == 2)
            ImGui::SetKeyboardFocusHere(-1);

        for (int n = 0; n < 50; n++)
            ImGui::Text("Dummy %d", n);

        ImGui::InputText("Text3", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        vars.Status[2].QuerySet(false);
        if (vars.Step == 3)
            ImGui::SetKeyboardFocusHere(-1);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Test Window");

        SetFocusVars& vars = ctx->GetUserData<SetFocusVars>();

        IM_CHECK(g.ActiveId == 0);
        vars.Step = 1;
        ctx->Yield(); // Takes two frame
        IM_CHECK(g.ActiveId == 0);
        ctx->Yield();
        IM_CHECK(g.ActiveId == ctx->GetID("Text1"));
        IM_CHECK(vars.Status[0].Visible == 1);
        IM_CHECK(vars.Status[1].Visible == 0);
        IM_CHECK(vars.Status[2].Visible == 0);
        ctx->Yield(2);

        vars.Step = 2;
        ctx->Yield(2);
        IM_CHECK(g.ActiveId == ctx->GetID("Text2"));
        IM_CHECK(vars.Status[0].Visible == 0);
        IM_CHECK(vars.Status[1].Visible == 1);
        IM_CHECK(vars.Status[2].Visible == 0);

        vars.Step = 3;
        ctx->Yield(2);
        IM_CHECK(g.ActiveId == ctx->GetID("Text3"));
        IM_CHECK(vars.Status[0].Visible == 0);
        IM_CHECK(vars.Status[1].Visible == 0);
        IM_CHECK(vars.Status[2].Visible == 1);

        vars.Step = 1;
        ctx->Yield();
        vars.Step = 0;
        ctx->Yield();
        IM_CHECK(g.ActiveId == ctx->GetID("Text1"));
        IM_CHECK(vars.Status[0].Visible == 1);
        IM_CHECK(vars.Status[1].Visible == 0);
        IM_CHECK(vars.Status[2].Visible == 0);
    };
#endif

    // ## Test wrapping behavior
    t = IM_REGISTER_TEST(e, "nav", "nav_wrapping");
    struct NavWrappingWars { ImGuiNavMoveFlags WrapFlags = ImGuiNavMoveFlags_WrapY; };
    t->SetUserDataType<NavWrappingWars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        auto& vars = ctx->GetUserData<NavWrappingWars>();
        for (int ny = 0; ny < 4; ny++)
        {
            for (int nx = 0; nx < 4; nx++)
            {
                if (ny == 3 && nx >= 2)
                    continue;
                if (nx > 0)
                    ImGui::SameLine();
                ImGui::Selectable(Str30f("%d,%d", ny, nx).c_str(), true, 0, ImVec2(40,40));
            }
        }
        if (vars.WrapFlags != 0)
            ImGui::NavMoveRequestTryWrapping(ImGui::GetCurrentWindow(), vars.WrapFlags);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GetUserData<NavWrappingWars>();
        ctx->SetRef("Test Window");

        vars.WrapFlags = ImGuiNavMoveFlags_None;
        ctx->ItemClick("0,0");
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK(g.NavId == ctx->GetID("0,1"));
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK(g.NavId == ctx->GetID("0,3"));
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK(g.NavId == ctx->GetID("0,3"));

        vars.WrapFlags = ImGuiNavMoveFlags_WrapX;
        ctx->ItemClick("0,0");
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK(g.NavId == ctx->GetID("0,1"));
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK(g.NavId == ctx->GetID("0,3"));
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK(g.NavId == ctx->GetID("1,0"));
        for (int n = 0; n < 50; n++) // Mash to get to the end of list
            ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK(g.NavId == ctx->GetID("3,1"));

        vars.WrapFlags = ImGuiNavMoveFlags_LoopX;
        ctx->ItemClick("0,0");
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK(g.NavId == ctx->GetID("0,1"));
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK(g.NavId == ctx->GetID("0,3"));
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK(g.NavId == ctx->GetID("0,0"));

        vars.WrapFlags = ImGuiNavMoveFlags_LoopY;
        ctx->ItemClick("0,0");
        ctx->KeyPressMap(ImGuiKey_RightArrow);
        IM_CHECK(g.NavId == ctx->GetID("0,1"));
        ctx->KeyPressMap(ImGuiKey_DownArrow);
        ctx->KeyPressMap(ImGuiKey_DownArrow);
        ctx->KeyPressMap(ImGuiKey_DownArrow);
        IM_CHECK(g.NavId == ctx->GetID("3,1"));
        ctx->KeyPressMap(ImGuiKey_DownArrow);
        IM_CHECK(g.NavId == ctx->GetID("0,1"));

#if IMGUI_BROKEN_TESTS
        // FIXME: LoopY up from 0,2 to 2,2 fails, goes to 3.1 -> maybe looping/wrapping should apply a bias (scoring weight) on the other axis?
        // FIXME: LoopY down from 2,2 to 0,2 fails, goes to 3.1 -> maybe looping/wrapping should apply a bias (scoring weight) on the other axis?
#endif
    };

    // ## Test wrapping behavior with clipper (test modeled after "nav_focus_api_clipped")
    // This works because CalcListClipping() adds NavScoringRect but is currently not super efficient.
    t = IM_REGISTER_TEST(e, "nav", "nav_wrapping_clipped");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        auto& vars = ctx->GenericVars;
        ImGuiListClipper clipper;
        clipper.Begin(50);
        while (clipper.Step())
        {
            for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);
        }
        ImGui::NavMoveRequestTryWrapping(ImGui::GetCurrentWindow(), ImGuiNavMoveFlags_LoopY);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Test Window");
        for (int n = 0; n < 52; n++)
        {
            IM_CHECK_EQ(g.NavId, ctx->GetID(Str30f("Input%d", (n) % 50).c_str()));
            ctx->KeyPressMap(ImGuiKey_DownArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID(Str30f("Input%d", (n + 1) % 50).c_str()));
        }
        // Should be on 51
        for (int n = 0; n < 4; n++)
        {
            IM_CHECK_EQ(g.NavId, ctx->GetID(Str30f("Input%d", (52 - n) % 50).c_str()));
            ctx->KeyPressMap(ImGuiKey_UpArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID(Str30f("Input%d", (51 - n) % 50).c_str()));
        }
    };
}

//-------------------------------------------------------------------------
