// dear imgui
// (tests: tables, columns)

#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
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
// Ideas/Specs for future tests
// It is important we take the habit to write those down.
// - Even if we don't implement the test right away: they allow us to remember edge cases and interesting things to test.
// - Even if they will be hard to actually implement/automate, they still allow us to manually check things.
//-------------------------------------------------------------------------
// TODO: Tests: Nav: keep calling SetKeyboardFocusHere() see how MouseDisabledHover is not affected.
// TODO: Tests: Nav: nav to button, verify that io.WantSetMousePos is set and MousePos changed
// TODO: Tests: Nav: use Alt to toggle layer back and forth, verify that io.WantSetMousePos is set and MousePos changed
// TODO: Tests: Nav: PageUp/PageDown/Home/End through flattened child (currently failing)
// TODO: Tests: Nav: Ctrl+tab on window with only Menu layer should switch to menu layer when focusing new window.
// TODO: Tests: Nav: focus on window with only Menu layer should switch layer (not currently the case)
// TODO: Tests: Nav, Cov: Gamepad version of Ctrl+Tab (hold triangle + L/R) is never exercised.
// TODO: Tests: Nav: add "widgets_drag_nav" (much simpler than "widgets_slider_nav"), just exercicing Left<>Right key, shift to make faster, alt to make slower
//-------------------------------------------------------------------------

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
        ctx->SetInputMode(ImGuiInputSource_Keyboard);
        ctx->SetRef("Hello, world!");
        ctx->ItemUncheck("Demo Window");
        ctx->ItemCheck("Demo Window");

        ImGuiContext& g = *ctx->UiContext;
        IM_CHECK(g.NavWindow && g.NavWindow->ID == ctx->GetID("//Dear ImGui Demo"));
    };

    // ## Test basic scoring
 #if IMGUI_VERSION_NUM >= 18953 // As simple at it is, before our 2023/04/24 the clamping would make this fail with very small windows.
    t = IM_REGISTER_TEST(e, "nav", "nav_scoring_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::CollapsingHeader("Window options"))
        {
            // 3 way layout, encore we transition to AAAA and not to center most
            if (ImGui::BeginTable("table", 3))
            {
                bool b = false;
                ImGui::TableNextColumn(); ImGui::Checkbox("AAAA", &b);
                ImGui::TableNextColumn(); ImGui::Checkbox("BBBB", &b);
                ImGui::TableNextColumn(); ImGui::Checkbox("CCCC", &b);
                ImGui::TableNextColumn(); ImGui::Checkbox("DDDD", &b);
                ImGui::TableNextColumn(); ImGui::Checkbox("EEEE", &b);
                ImGui::TableNextColumn(); ImGui::Checkbox("FFFF", &b);
                ImGui::EndTable();
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Test Window");
        ctx->ItemClick("Window options");
        ctx->ItemOpen("Window options");
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window options"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/AAAA"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/DDDD"));
        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/AAAA"));
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/BBBB"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/EEEE"));
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/FFFF"));
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/FFFF"));
        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/CCCC"));

        // Test recorded preferred scoring pos
#if IMGUI_VERSION_NUM >= 18956
        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window options"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/CCCC"));
        ctx->KeyPress(ImGuiKey_LeftArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/BBBB"));
        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window options"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/BBBB"));
        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window options"));

        // Failing move resets preferred pos on this axis
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window options"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/AAAA"));

        // Test that preferred pos is saved/restored on focus change
        ctx->KeyPress(ImGuiKey_RightArrow);
        ctx->KeyPress(ImGuiKey_UpArrow);
        ctx->WindowFocus("//Dear ImGui Demo"); // Focus other window
        ctx->ItemOpen("//Dear ImGui Demo/Help");
        ctx->KeyPress(ImGuiKey_DownArrow);
        ctx->ItemClose("//Dear ImGui Demo/Help");
        ctx->KeyPress(g.ConfigNavWindowingKeyNext); // Ctrl+Tab
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window options"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("table/BBBB"));
#endif
    };
#endif

    // ## Test that ESC deactivate InputText without closing current Popup (#2321, #787, #5400)
    t = IM_REGISTER_TEST(e, "nav", "nav_esc_popup");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Open Popup"))
            ImGui::OpenPopup("Popup");
        if (ImGui::Button("Open Modal"))
            ImGui::OpenPopup("Modal");

        if (ImGui::BeginPopup("Popup", ImGuiWindowFlags_NoSavedSettings) || ImGui::BeginPopupModal("Modal", NULL, ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::InputText("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            if (ctx->IsGuiFuncOnly() && ImGui::IsKeyPressed(ImGuiKey_Escape) && !g.IO.NavVisible)
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        for (int variant = 0; variant < 2; variant++)
        {
            ctx->LogDebug("Variant: %s", variant == 0 ? "popup" : "modal");
            ctx->SetRef("Test Window");

            ctx->NavMoveTo(variant == 0 ? "Open Popup" : "Open Modal");
            ctx->NavActivate();
            ImGuiWindow* popup = g.NavWindow;
            if (variant == 0)
                IM_CHECK((popup->Flags & ImGuiWindowFlags_Popup) != 0);
            else
                IM_CHECK((popup->Flags & ImGuiWindowFlags_Modal) != 0);

            ctx->NavInput();    // Activate "Field"
            IM_CHECK(popup->Active);
            IM_CHECK(g.ActiveId == ctx->GetID("Field", popup->ID));

            ctx->KeyPress(ImGuiKey_Escape);
            IM_CHECK(popup->Active);
            IM_CHECK(g.ActiveId == 0);
            IM_CHECK(g.IO.NavVisible);

            ctx->KeyPress(ImGuiKey_Escape);
            if (variant == 0)
            {
                // Ordinary popups are closed immediately.
                IM_CHECK(!popup->Active);
            }
            else
            {
                // Modals do not auto-close, instead they deactivate navigation.
                IM_CHECK(popup->Active);
#if IMGUI_VERSION_NUM >= 18731
                IM_CHECK(!g.IO.NavVisible);
#endif
            }
        }
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
            IM_UNUSED(ctx);
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
        case 3:
            // Test toggling between nav layers when menu layer only has generic buttons. (#5463, #4792)
            ImGui::Begin("Test window", &ctx->GenericVars.Bool1, ImGuiWindowFlags_NoSavedSettings);
            window_content();
            ImGui::End();
            break;
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // FIXME-TESTS: Fails if window is resized too small
        ImGuiContext& g = *ctx->UiContext;
        IM_CHECK(g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard);
        //ctx->SetInputMode(ImGuiInputSource_Keyboard);
        //ctx->SetRef("Test window");

        for (int step = 0; step < 4; step++)
        {
            ctx->LogDebug("Step %d", step);
            ctx->GenericVars.Step = step; // Enable modal popup?
            ctx->PopupCloseAll();
            ctx->Yield();

            const ImGuiID input_id = ctx->GenericVars.Id; // "Input"
            ctx->NavMoveTo(input_id);
            ctx->SetRef("//$FOCUSED");

            IM_CHECK(g.OpenPopupStack.Size == (step == 2));
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);
            ctx->KeyPress(ImGuiMod_Alt);
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Menu);
            const char* menu_item_1 = step == 3 ? "#COLLAPSE" : "##menubar/File";
            const char* menu_item_2 = step == 3 ? "#CLOSE" : "##menubar/Edit";
            IM_CHECK_EQ(g.NavId, ctx->GetID(menu_item_1));
            ctx->KeyPress(ImGuiKey_RightArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID(menu_item_2));

            ctx->KeyPress(ImGuiMod_Alt);
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiMod_Alt);
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);
            ctx->KeyPress(ImGuiMod_Alt); // Verify nav id is reset for menu layer
            IM_CHECK_EQ(g.NavId, ctx->GetID(menu_item_1));
            ctx->KeyPress(ImGuiMod_Alt);

            // Test that toggling layer steals active id.
#if IMGUI_VERSION_NUM >= 18206
            ctx->NavMoveTo(input_id);
            ctx->NavInput();
            IM_CHECK_EQ(g.ActiveId, input_id);
            ctx->KeyPress(ImGuiMod_Alt);
            if (g.IO.ConfigMacOSXBehaviors)
            {
                // On OSX, InputText() claims Alt mod
                IM_CHECK_EQ(g.ActiveId, input_id);
                ctx->KeyPress(ImGuiKey_Escape); // ESC to leave layer
            }
            else
            {
                IM_CHECK_EQ(g.ActiveId, (ImGuiID)0);
                IM_CHECK(g.NavLayer == ImGuiNavLayer_Menu);
                ctx->KeyPress(ImGuiKey_Escape); // ESC to leave layer
            }
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);
            IM_CHECK(g.NavId == input_id);

            // Test that toggling layer is canceled by character typing (#370)
            ctx->NavMoveTo(input_id);
            ctx->NavInput();
            IM_CHECK_EQ(g.ActiveId, input_id);
            ctx->KeyDown(ImGuiMod_Alt);
            ctx->KeyChars("ABC");
            ctx->KeyUp(ImGuiMod_Alt);
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
        ctx->SetRef("Test Window");
        ctx->SetInputMode(ImGuiInputSource_Keyboard);

        IM_CHECK(g.NavId == window->GetID("Button 0"));
        IM_CHECK(window->Scroll.y == 0);
        // Navigate to the middle of window
        for (int i = 0; i < 5; i++)
            ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK(g.NavId == window->GetID("Button 5"));
        IM_CHECK(window->Scroll.y > 0 && window->Scroll.y < window->ScrollMax.y);
        // From the middle to the end
        ctx->KeyPress(ImGuiKey_End);
        IM_CHECK(g.NavId == window->GetID("Button 9"));
        IM_CHECK(window->Scroll.y == window->ScrollMax.y);
        // From the end to the start
        ctx->KeyPress(ImGuiKey_Home);
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
        IM_CHECK(g.NavId == ctx->GetID("//##Menu_00/New"));
        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK(g.NavId == ctx->GetID("//##Menu_00/Quit"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK(g.NavId == ctx->GetID("//##Menu_00/New"));
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
                if (ImGui::BeginMenu("Submenu1"))
                {
                    ImGui::MenuItem("A");
                    ImGui::MenuItem("B");
                    ImGui::MenuItem("C");
                    ImGui::MenuItem("D");
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Submenu2"))
                {
                    bool use_child = ctx->Test->ArgVariant == 1;
                    if (!use_child || ImGui::BeginChild("Child", ImVec2(100.f, 30.f), ImGuiChildFlags_Border))
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

        for (int variant = 0; variant < 2; variant++)
        {
            ctx->Test->ArgVariant = variant;
            ctx->LogDebug("Variant: %d", variant);
            ctx->SetRef("Test Window");
            ctx->MenuClick("Menu/Submenu1");
            ctx->KeyPress(ImGuiKey_RightArrow);
#if IMGUI_VERSION_NUM >= 19018
            IM_CHECK(g.NavId == ctx->GetID("//##Menu_01/A"));
            IM_CHECK(g.NavDisableHighlight == false);
#endif
            ctx->KeyPress(ImGuiKey_DownArrow);
            IM_CHECK(g.NavId == ctx->GetID("//##Menu_01/B"));
            ctx->KeyPress(ImGuiKey_LeftArrow);
            IM_CHECK(g.NavId == ctx->GetID("//##Menu_00/Submenu1")); // Ensure navigation doesn't get us to Submenu2 which is at same Y position as B

            ctx->MenuClick("Menu/Submenu2");
            if (variant == 0)
            {
                ctx->SetRef("##Menu_01");
            }
            else
            {
                ImGuiID child_id = ctx->WindowInfo("//##Menu_01/Child").ID;
                ctx->SetRef(child_id);
            }
            ctx->ItemClick("Tabs/Tab 1");
            ctx->KeyPress(ImGuiKey_RightArrow);         // Activate nav, navigate to next tab
#if IMGUI_VERSION_NUM < 18414
            if (g.NavId == ctx->GetID("Tabs/Tab 1"))
                ctx->KeyPress(ImGuiKey_RightArrow);     // FIXME-NAV: NavInit() prevents navigation to Tab 2 on the very first try.
#endif
            IM_CHECK(g.NavId == ctx->GetID("Tabs/Tab 2"));
            ctx->KeyPress(ImGuiKey_LeftArrow);          // Navigate to first tab, not closing menu
            IM_CHECK(g.NavId == ctx->GetID("Tabs/Tab 1"));
            if (variant == 1)
            {
                ctx->KeyPress(ImGuiKey_LeftArrow);      // Navigation fails, not closing menu
                IM_CHECK(g.NavId == ctx->GetID("Tabs/Tab 1"));
                ctx->KeyPress(ImGuiKey_Escape);         // Exit child window nav
            }
            ctx->KeyPress(ImGuiKey_LeftArrow);          // Close 2nd level menu
            IM_CHECK_STR_EQ(g.NavWindow->Name, "##Menu_00");
            ctx->KeyPress(ImGuiKey_LeftArrow);          // Close 1st level menu
            IM_CHECK_STR_EQ(g.NavWindow->Name, "Test Window");
        }

        // Test current menu behaviors. They are inconsistent, further discussion at https://github.com/ocornut/imgui_private/issues/19.
        ctx->SetRef("Test Window");
        ctx->MenuClick("Menu/Submenu1");
        IM_CHECK_EQ(g.OpenPopupStack.Size, 2);
        ctx->KeyPress(ImGuiKey_LeftArrow);          // Left key closes a menu if menu item was clicked.
        IM_CHECK_EQ(g.OpenPopupStack.Size, 0);

        ctx->MenuClick("Menu");
        ctx->ItemAction(ImGuiTestAction_Hover, "//$FOCUSED/Submenu1", ImGuiTestOpFlags_NoFocusWindow);
        IM_CHECK_EQ(g.OpenPopupStack.Size, 2);
        ctx->KeyPress(ImGuiKey_RightArrow);         // Right key closes a menu if item was hovered.
        IM_CHECK_EQ(g.OpenPopupStack.Size, 0);

        ctx->MenuClick("Menu/Submenu1");
        IM_CHECK_EQ(g.OpenPopupStack.Size, 2);
        ctx->KeyPress(ImGuiKey_RightArrow);         // Right key maintains submenu open if menu item was clicked.
        IM_CHECK_EQ(g.OpenPopupStack.Size, 2);
#if IMGUI_VERSION_NUM >= 18813
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/A"));
#endif
        ctx->KeyPress(ImGuiKey_DownArrow);          // Down key correctly moves to a second item in submenu.
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/B"));

        ctx->PopupCloseAll();
        ctx->MenuClick("Menu");
        ctx->ItemAction(ImGuiTestAction_Hover, "//$FOCUSED/Submenu1", ImGuiTestOpFlags_NoFocusWindow);
        IM_CHECK_EQ(g.OpenPopupStack.Size, 2);
        ctx->KeyPress(ImGuiKey_LeftArrow);          // Right key maintains submenu open if menu item was hovered.
        IM_CHECK_EQ(g.OpenPopupStack.Size, 2);
#if IMGUI_VERSION_NUM >= 18813
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/A"));
#endif
        ctx->KeyPress(ImGuiKey_DownArrow);          // Down key correctly moves to a second item in submenu.
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/B"));
    };

    // ## Test navigation across menuset_is_open and ImGuiItemFlags_NoWindowHoverableCheck (#5730)
    t = IM_REGISTER_TEST(e, "nav", "nav_menu_menuset");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        if (ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings))
        {
            if (ImGui::BeginMenu("Menu1"))
            {
                ImGui::MenuItem("MenuItem1");
                ImGui::Text("hovered: %d", ImGui::IsItemHovered());
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Menu2"))
            {
                static float ff = 0.5;
                ImGui::SliderFloat("float1", &ff, 0.f, 1.f);
                ImGui::MenuItem("MenuItem2");
                ImGui::Text("hovered: %d", ImGui::IsItemHovered());
                ImGui::MenuItem("MenuItem2b");
                ImGui::MenuItem("MenuItem2c");
                ImGui::EndMenu();
            }
            ImGui::Text("hovered: %d", ImGui::IsItemHovered());
            if (ImGui::BeginMenu("Menu3"))
            {
                ImGui::MenuItem("MenuItem3");
                ImGui::EndMenu();
            }
            ImGui::MenuItem("MenuItemOutside");
#if IMGUI_VERSION_NUM >= 18832
            ImGui::Text("hovered: %d", ImGui::IsItemHovered()); // FIXME-TESTS: Support fixed in 18832, need test.
#endif
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("//Test Window");
        ctx->MouseMove("MenuItemOutside"); // Get out of the way
        ctx->NavMoveTo("Menu1"); // FIXME: ItemClick() would open menu and take focus to sub-menu
        //IM_SUSPEND_TESTFUNC();
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Menu2"));
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/float1"));
        ctx->KeyPress(ImGuiKey_DownArrow);
#if IMGUI_VERSION_NUM >= 18826
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/MenuItem2"));
#endif
#if IMGUI_VERSION_NUM >= 18827
        ctx->KeyPress(ImGuiKey_LeftArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Menu2"));
#endif
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
        ImGui::BeginChild("Child", ImVec2(50, 50), ImGuiChildFlags_Border);
        ImGui::Button("Button In");
        ImGui::EndChild();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiKeyChord chord_ctrl_tab = g.ConfigNavWindowingKeyNext; // Generally ImGuiMod_Ctrl | ImGuiKey_Tab. Using _Super on Mac.

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

            ctx->KeyPress(chord_ctrl_tab);
            IM_CHECK(g.NavWindow == ctx->GetWindowByRef("Window 1"));

            // Intentionally perform a "SLOW" ctrl-tab to make sure the UI appears!
            //ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Tab);
            ctx->KeyDown(chord_ctrl_tab & ImGuiMod_Mask_);
            ctx->KeyPress(chord_ctrl_tab & ~ImGuiMod_Mask_);
            ctx->SleepNoSkip(0.5f, 0.1f);
            IM_CHECK(g.NavWindowingTarget == ctx->GetWindowByRef("Window 2"));
            IM_CHECK(g.NavWindowingTarget->SkipItems == false);
            ctx->KeyUp(chord_ctrl_tab & ImGuiMod_Mask_);
            IM_CHECK(g.NavWindow == ctx->GetWindowByRef("Window 2"));

            // Test previous windowing key combo with UI
            ctx->KeyDown(chord_ctrl_tab & ImGuiMod_Mask_);
            ctx->KeyPress(chord_ctrl_tab & ~ImGuiMod_Mask_);
            ctx->SleepNoSkip(0.5f, 0.1f);
            IM_CHECK(g.NavWindowingTarget == ctx->GetWindowByRef("Window 1"));
            IM_CHECK(g.NavWindowingTarget->SkipItems == false);
            ctx->KeyPress(ImGuiMod_Shift | (chord_ctrl_tab & ~ImGuiMod_Mask_)); // FIXME: In theory should use g.ConfigNavWindowingKeyPrev
            IM_CHECK(g.NavWindowingTarget == ctx->GetWindowByRef("Window 2"));
            IM_CHECK(g.NavWindowingTarget->SkipItems == false);
            ctx->KeyUp(chord_ctrl_tab & ImGuiMod_Mask_);
            IM_CHECK(g.NavWindow == ctx->GetWindowByRef("Window 2"));

            // Set up window focus order, focus child window.
            ctx->WindowFocus("Window 1");
            ctx->WindowFocus("Window 2"); // FIXME: Needed for case when docked
            ctx->ItemClick(ctx->GetID("Button In", ctx->WindowInfo("Window 2/Child").ID));

            ctx->KeyPress(chord_ctrl_tab);
            IM_CHECK(g.NavWindow == ctx->GetWindowByRef("Window 1"));
        }
    };

    // ## Test CTRL+TAB window focusing with open popup
    t = IM_REGISTER_TEST(e, "nav", "nav_ctrl_tab_popups");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextUnformatted("Not empty space");
        if (ImGui::Button("Popup 1"))
            ImGui::OpenPopup("Popup 1");
        if (ImGui::BeginPopup("Popup 1"))
        {
            ImGui::Text("This is Popup 1");
            ImGui::EndPopup();
        }
        if (ImGui::Button("Popup 2"))
            ImGui::OpenPopup("Popup 2");
        if (ImGui::BeginPopup("Popup 2", ImGuiWindowFlags_ChildWindow))
        {
            ImGui::Text("This is Popup 2");
            ImGui::EndPopup();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiKeyChord chord_ctrl_tab = g.ConfigNavWindowingKeyNext; // Generally ImGuiMod_Ctrl | ImGuiKey_Tab. Using _Super on Mac.

#ifdef IMGUI_HAS_DOCK
        ctx->DockClear("Dear ImGui Demo", "Test Window", NULL);
#endif
        ctx->WindowFocus("Dear ImGui Demo");
        ctx->WindowFocus("Test Window");
        ctx->ItemClick("Test Window/Popup 1");
        ctx->KeyDown(chord_ctrl_tab & ImGuiMod_Mask_);
        ctx->KeyPress(ImGuiKey_Tab); // to Test Window
        ctx->KeyPress(ImGuiKey_Tab); // to Dear ImGui Demo
        ctx->KeyUp(chord_ctrl_tab & ImGuiMod_Mask_);
        ctx->Yield(2);
        IM_CHECK(g.NavWindow == ctx->GetWindowByRef("Dear ImGui Demo"));
        IM_CHECK(g.OpenPopupStack.Size == 0);

        ctx->KeyPress(chord_ctrl_tab);
        IM_CHECK(g.NavWindow == ctx->GetWindowByRef("Test Window"));
        ctx->ItemClick("Test Window/Popup 2");
        ctx->KeyPress(chord_ctrl_tab); // To Dear ImGui Demo
        ctx->Yield(2);
        IM_CHECK(g.NavWindow == ctx->GetWindowByRef("Dear ImGui Demo"));
#if IMGUI_VERSION_NUM >= 19018
        IM_CHECK(g.OpenPopupStack.Size == 0);
#endif
    };

    // ## Test NavID restoration during CTRL+TAB focusing
    t = IM_REGISTER_TEST(e, "nav", "nav_ctrl_tab_nav_id_restore");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Button("Button 1");
        ImGui::End();

        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::BeginChild("Child", ImVec2(50, 50), ImGuiChildFlags_Border);
        ImGui::Button("Button 2");
        ImGui::EndChild();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiKeyChord chord_ctrl_tab = g.ConfigNavWindowingKeyNext; // Generally ImGuiMod_Ctrl | ImGuiKey_Tab. Using _Super on Mac.

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
            ImGuiTestRef win2_button_ref(ctx->GetID("Button 2", ctx->WindowInfo("Window 2/Child").ID));

            // Focus Window 1, navigate to the button
            ctx->WindowFocus("Window 1");
            ctx->NavMoveTo(win1_button_ref);

            // Focus Window 2, ensure nav id was changed, navigate to the button
            ctx->WindowFocus("Window 2");
            IM_CHECK_NE(ctx->GetID(win1_button_ref), g.NavId);
            ctx->NavMoveTo(win2_button_ref);

            // Ctrl+Tab back to previous window, check if nav id was restored
            ctx->KeyPress(chord_ctrl_tab);
            IM_CHECK_EQ(ctx->GetID(win1_button_ref), g.NavId);

            // Ctrl+Tab back to previous window, check if nav id was restored
            ctx->KeyPress(chord_ctrl_tab);
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
        ImGuiKeyChord chord_ctrl_tab = g.ConfigNavWindowingKeyNext; // Generally ImGuiMod_Ctrl | ImGuiKey_Tab. Using _Super on Mac.

        ctx->WindowFocus("Window 1");
        ctx->WindowFocus("Window 2");
        ctx->KeyPress(chord_ctrl_tab);
        IM_CHECK_STR_EQ(g.NavWindow->Name, "Window 1");
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Menu);
        ctx->KeyPress(chord_ctrl_tab);
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
        ImGuiContext& g = *ctx->UiContext;
        ImGuiKeyChord chord_ctrl_tab = g.ConfigNavWindowingKeyNext; // Generally ImGuiMod_Ctrl | ImGuiKey_Tab. Using _Super on Mac.
        ctx->SetInputMode(ImGuiInputSource_Keyboard);
        ctx->SetRef("Test window 1");
        ctx->ItemInput("InputText");
        ctx->KeyCharsAppend("123");
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("InputText"));
        ctx->KeyPress(chord_ctrl_tab);
        IM_CHECK_EQ(g.ActiveId, (ImGuiID)0);
        ctx->Sleep(1.0f);
    };

#if IMGUI_VERSION_NUM >= 18944
    // ## Test CTRL+TAB customization (#4828)
    t = IM_REGISTER_TEST(e, "nav", "nav_ctrl_tab_customization");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("Oh dear");
        ImGui::End();

        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("Oh dear");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;

        // Configure custom windowing key combinations
        g.ConfigNavWindowingKeyNext = ImGuiMod_Alt | ImGuiKey_N;
        g.ConfigNavWindowingKeyPrev = ImGuiMod_Alt | ImGuiKey_P;

        // Dock windows together
        // This tests edge case where the alt gets grabbed by the docking tab bar instead of windowing
        // See https://github.com/ocornut/imgui_test_engine/pull/17#issuecomment-1478643518
#ifdef IMGUI_HAS_DOCK
        ctx->DockClear("Window 1", "Window 2", NULL);
        ctx->DockInto("Window 1", "Window 2");
#endif

        // Set up window focus order.
        ctx->WindowFocus("Window 1");
        ctx->WindowFocus("Window 2");

        // Press default windowing keys and ensure window focus is unchanged
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavWindow, ctx->GetWindowByRef("Window 2"));

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavWindow, ctx->GetWindowByRef("Window 2"));

        // Press custom windowing key and ensure focus changed
        ctx->KeyPress(ImGuiMod_Alt | ImGuiKey_N);
        IM_CHECK_EQ(g.NavWindow, ctx->GetWindowByRef("Window 1"));

        // Intentionally perform a "SLOW" windowing key combo to make sure the UI appears!
        ctx->KeyDown(ImGuiMod_Alt);
        ctx->KeyPress(ImGuiKey_N);
        ctx->SleepNoSkip(0.5f, 0.1f);
        IM_CHECK_EQ(g.NavWindowingTarget, ctx->GetWindowByRef("Window 2"));
        IM_CHECK_EQ(g.NavWindowingTarget->SkipItems, false);
        ctx->KeyUp(ImGuiMod_Alt);
        IM_CHECK_EQ(g.NavWindow, ctx->GetWindowByRef("Window 2"));

        // Test previous windowing key combo with UI
        ctx->KeyDown(ImGuiMod_Alt);
        ctx->KeyPress(ImGuiKey_N);
        ctx->SleepNoSkip(0.5f, 0.1f);
        IM_CHECK_EQ(g.NavWindowingTarget, ctx->GetWindowByRef("Window 1"));
        IM_CHECK_EQ(g.NavWindowingTarget->SkipItems, false);
        ctx->KeyPress(ImGuiKey_P);
        IM_CHECK_EQ(g.NavWindowingTarget, ctx->GetWindowByRef("Window 2"));
        IM_CHECK_EQ(g.NavWindowingTarget->SkipItems, false);
        ctx->KeyUp(ImGuiMod_Alt);
        IM_CHECK_EQ(g.NavWindow, ctx->GetWindowByRef("Window 2"));

        //---------------------------------------------------------------------
        // Disable windowing key combinations
        g.ConfigNavWindowingKeyNext = 0;
        g.ConfigNavWindowingKeyPrev = 0;

        // Set up window focus order.
        ctx->WindowFocus("Window 1");
        ctx->WindowFocus("Window 2");

        // Press Ctrl+Tab and Ctrl+Shift+Tab and ensure window focus is unchanged
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavWindow, ctx->GetWindowByRef("Window 2"));

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavWindow, ctx->GetWindowByRef("Window 2"));
    };
#endif

    // ## Test Ctrl+Tab with callback at end of drawlist (crash fixed in 18973)
#if IMGUI_VERSION_NUM >= 18972
    t = IM_REGISTER_TEST(e, "nav", "nav_ctrl_tab_callback");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImDrawCallback dummy_callback = [](const ImDrawList* parent_list, const ImDrawCmd* cmd) {};
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Button("Button 1");
        ImGui::GetWindowDrawList()->AddCallback(dummy_callback, NULL);
        ImGui::End();

        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Button("Button 2");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowFocus("Window 1");
        ctx->WindowFocus("Window 2");
        ctx->KeyDown(ImGuiMod_Ctrl);
        ctx->SleepNoSkip(1.0f, 0.50f);
        ctx->KeyPress(ImGuiKey_Tab);
        ctx->SleepNoSkip(1.0f, 0.50f);
        ctx->KeyPress(ImGuiKey_Tab);
        ctx->SleepNoSkip(1.0f, 0.50f);
        ctx->KeyPress(ImGuiKey_Tab);
        ctx->KeyUp(ImGuiMod_Ctrl);
    };
#endif

    // ## Test remote ActivateItemByID()
    t = IM_REGISTER_TEST(e, "nav", "nav_activate");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGuiID activate_target = ctx->GetID((vars.Step == 0) ? "//Test Window/Button" : "//Test Window/InputText");

        if (ImGui::Button("Activate Src 0"))
#if IMGUI_VERSION_NUM >= 18961
            ImGui::ActivateItemByID(activate_target);
#else
            ImGui::ActivateItem(activate_target);
#endif
        if (ImGui::Button("Button"))
            vars.Int1++;
        ImGui::SameLine();
        ImGui::Text("%d", vars.Int1);
        ImGui::InputText("InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        if (ImGui::Button("Activate Src 1"))
#if IMGUI_VERSION_NUM >= 18961
            ImGui::ActivateItemByID(activate_target);
#else
            ImGui::ActivateItem(activate_target);
#endif
        ImGui::End();

        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Activate Src 2"))
#if IMGUI_VERSION_NUM >= 18961
            ImGui::ActivateItemByID(activate_target);
#else
            ImGui::ActivateItem(activate_target);
#endif
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        IM_CHECK(vars.Int1 == 0);
        ctx->SetRef("Test Window");

        vars.Step = 0;
        ctx->Yield();
        ctx->ItemClick("Activate Src 0");
        IM_CHECK(vars.Int1 == 1);
        ctx->ItemClick("Activate Src 1");
        IM_CHECK(vars.Int1 == 2);
        ctx->ItemClick("//Window 2/Activate Src 2");
        IM_CHECK(vars.Int1 == 3);

        vars.Step = 1;
        ctx->Yield();
        ctx->ItemClick("Activate Src 0");
        IM_CHECK_EQ(g.NavId, ctx->GetID("//Test Window/InputText"));
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("//Test Window/InputText"));
        ctx->Yield(); // Wait for route
        ctx->KeyPress(ImGuiKey_Escape);
        IM_CHECK_EQ(g.ActiveId, 0u);
        ctx->ItemClick("//Window 2/Activate Src 2");
        IM_CHECK_EQ(g.NavId, ctx->GetID("//Test Window/InputText"));
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("//Test Window/InputText"));
    };

#if IMGUI_VERSION_NUM >= 18412
    // ## Test NavInput (#2321 and many others)
    t = IM_REGISTER_TEST(e, "nav", "nav_input");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        if (ImGui::Button("Button"))
            vars.Int1++;
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ctx->SetRef("Test Window");
        ctx->NavMoveTo("InputText");

        // Cross/Activate to apply + release ActiveId
        ctx->NavInput();
        IM_CHECK(g.ActiveId == ctx->GetID("//Test Window/InputText"));
        ctx->KeyCharsReplaceEnter("0");
        IM_CHECK_STR_EQ(vars.Str1, "0");
        ctx->NavInput();
        ctx->KeyCharsReplace("123");
        IM_CHECK_STR_EQ(vars.Str1, "123");
        ctx->KeyPress(ImGuiKey_Enter);
        IM_CHECK(g.ActiveId == 0);
        IM_CHECK_STR_EQ(vars.Str1, "123");

        // Circle/Cancel to revert + release ActiveId
        ctx->NavInput();
        ctx->KeyCharsReplaceEnter("0");
        IM_CHECK_STR_EQ(vars.Str1, "0");
        ctx->NavInput();
        ctx->KeyCharsReplace("123");
        ctx->KeyPress(ImGuiKey_Escape);
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

        // Verify that NavInput triggers buttons (#5606)
        vars.Int1 = 0;
        ctx->NavMoveTo("Button");
        ctx->NavInput();
#if IMGUI_VERSION_NUM >= 18935
        IM_CHECK(vars.Int1 > 0);
#else
        IM_CHECK(vars.Int1 == 0);
#endif
    };
#endif

    // ## Test NavID restoration when focusing another window or STOPPING to submit another world
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_restore_on_missing_window");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Button("Button 1");
        ImGui::Button("Button 2");
        ImGui::BeginChild("Child", ImVec2(100, 100), ImGuiChildFlags_Border);
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
        ctx->KeyPress(ImGuiKey_DownArrow);                      // Manipulate something
        //IM_CHECK(g.NavId == ctx->ItemInfo("//**/Button 4")->ID); // Can't easily include child window name in ID because the / gets inhibited...
        IM_CHECK(g.NavId == ctx->GetID("//$FOCUSED/Button 4"));
        ctx->NavActivate();
        ctx->KeyPress(ImGuiKey_Escape);                         // Leave child window
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
            ctx->NavMoveTo("Configuration");                    // Focus item.
            ctx->KeyPress(ImGuiMod_Alt);                        // Focus menu.
            IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Menu);
            ctx->NavActivate();                                 // Open menu, focus first item in the menu.
            ctx->NavActivate();                                 // Activate first item in the menu.
            IM_CHECK_EQ(g.NavId, ctx->GetID("Configuration"));  // Verify NavId was restored to initial value.

            // Test focus restoration to the item within a child window.
            // FIXME-TESTS: Feels like this would be simpler with our own GuiFunc.
            ctx->NavMoveTo("Layout & Scrolling");                       // Navigate to the child window.
            ctx->NavActivate();
            ctx->NavMoveTo("Scrolling");
            ctx->NavActivate(); // FIXME-TESTS: Could query current g.NavWindow instead of making names?
            ImGuiWindow* child_window = ctx->WindowInfo("Scrolling/scrolling").Window;
            IM_CHECK(child_window != NULL);
            ctx->SetRef(child_window->ID);
            ctx->ScrollTo(demo_window->ID, ImGuiAxis_Y, (child_window->Pos - demo_window->Pos).y);  // Required because buttons do not register their IDs when out of view (SkipItems == true).
            ctx->NavMoveTo("$$1/1");                    // Focus item within a child window.
            ctx->KeyPress(ImGuiMod_Alt);                // Focus menu
            ctx->NavActivate();                         // Open menu, focus first item in the menu.
            ctx->NavActivate();                         // Activate first item in the menu.
            IM_CHECK_EQ(g.NavId, ctx->GetID("$$1/1"));  // Verify NavId was restored to initial value.

            ctx->SetRef("Dear ImGui Demo");
            ctx->ItemClose("Scrolling");
            ctx->ItemClose("Layout & Scrolling");
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
            IM_CHECK_EQ(g.NavWindow, ctx->GetWindowByRef(""));
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
#if IMGUI_VERSION_NUM >= 18835
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_scope_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGuiID focus_scope_id = ImGui::GetID("MyScope");

        IM_CHECK_EQ(ImGui::GetCurrentFocusScope(), ImGui::GetID(""));
        ImGui::BeginChild("Child 1", ImVec2(100, 100));
        IM_CHECK_EQ(ImGui::GetCurrentFocusScope(), ImGui::GetID(""));
        ImGui::EndChild();

        ImGui::PushFocusScope(focus_scope_id);
        IM_CHECK_EQ(ImGui::GetCurrentFocusScope(), focus_scope_id);
        ImGui::BeginChild("Child 1", ImVec2(100, 100));
        IM_CHECK_EQ(ImGui::GetCurrentFocusScope(), ImGui::GetID("")); // Append
        ImGui::EndChild();
        ImGui::BeginChild("Child 2", ImVec2(100, 100));
        IM_CHECK_EQ(ImGui::GetCurrentFocusScope(), ImGui::GetID("")); // New child
        ImGui::EndChild();
        IM_CHECK_EQ(ImGui::GetCurrentFocusScope(), focus_scope_id);

        // Should not inherit
        ImGui::Begin("Test Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
        IM_CHECK_EQ(ImGui::GetCurrentFocusScope(), ImGui::GetID(""));
        ImGui::End();

        // ...Unless flattened child
#if IMGUI_VERSION_NUM >= 19002
        ImGui::BeginChild("Child 3", ImVec2(100, 100), ImGuiChildFlags_Border | ImGuiChildFlags_NavFlattened);
        IM_CHECK_EQ(ImGui::GetCurrentFocusScope(), focus_scope_id); // New child
        ImGui::EndChild();
#endif
        ImGui::PopFocusScope();

        IM_CHECK_EQ(ImGui::GetCurrentFocusScope(), ImGui::GetID(""));
        ImGui::End();
    };
#endif

    // Also see "nav_tabbing_flattened"
#if IMGUI_VERSION_NUM >= 19002
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_scope_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGui::Button("0");

        ImGui::Separator();
        ImGui::PushFocusScope(ImGui::GetID("Scope 1"));
        ImGui::Button("1A");
        ImGui::Button("1B");
        ImGui::Button("1C");
        ImGui::PopFocusScope();

        ImGui::Separator();
        ImGui::PushFocusScope(ImGui::GetID("Scope 2"));
        ImGui::Button("2A");
        ImGui::Button("2B");
        ImGui::Button("2C");
        ImGui::PopFocusScope();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Test Window");
        ctx->ItemClick("0");
        IM_CHECK_EQ(g.NavId, ctx->GetID("0"));
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavId, ctx->GetID("0"));

        ctx->ItemClick("1A");
#if 1
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavId, ctx->GetID("1A")); // Highlight reappears after we mouse-click
#endif
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavId, ctx->GetID("1B"));
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavId, ctx->GetID("1C"));
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavId, ctx->GetID("1A"));
        ctx->KeyPress(ImGuiKey_Tab | ImGuiMod_Shift);
        IM_CHECK_EQ(g.NavId, ctx->GetID("1C"));

        ctx->ItemClick("2C");
#if 1
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavId, ctx->GetID("2C")); // Highlight reappears after we mouse-click
#endif
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavId, ctx->GetID("2A"));
    };
#endif

    // ## Check setting default focus.
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_default");
    struct DefaultFocusVars { bool ShowWindows = true; bool SetFocus = false; bool MenuLayer = false; };
    t->SetVarsDataType<DefaultFocusVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DefaultFocusVars& vars = ctx->GetVars<DefaultFocusVars>();
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
        DefaultFocusVars& vars = ctx->GetVars<DefaultFocusVars>();
        ImGuiWindow* window = ctx->GetWindowByRef("Window");

        // Variant 0: verify ##2 is not scrolled into view and ##1 is focused (SetItemDefaultFocus() not used).
        // Variant 1: verify ##2 is scrolled into view and focused (SetItemDefaultFocus() is used).
        for (int variant = 0; variant < 2; variant++)
        {
            ctx->LogDebug("Test variant: %s default focus override", variant ? "with" : "without");
            vars.SetFocus = (variant == 1);
            vars.ShowWindows = true;
            ctx->Yield(2);
            IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);
            if (!vars.SetFocus)
                IM_CHECK_EQ(g.NavId, ctx->GetID("Window/##1"));
            else
                IM_CHECK_EQ(g.NavId, ctx->GetID("Window/##2"));
            ImGuiTestItemInfo input = ctx->ItemInfo("Window/##2");
            IM_CHECK_EQ(window->InnerClipRect.Contains(input.RectFull), vars.SetFocus);
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
    t->SetVarsDataType<DefaultFocusVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DefaultFocusVars& vars = ctx->GetVars<DefaultFocusVars>();
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
        DefaultFocusVars& vars = ctx->GetVars<DefaultFocusVars>();

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
        ImGui::BeginChild("Child 1", ImVec2(200, 200), ImGuiChildFlags_NavFlattened);
        output_item("Child 1 Button 1");
        output_item("Child 1 Button 2");
        output_item("Child 1 Button 3");
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Child 2", ImVec2(200, 200), ImGuiChildFlags_NavFlattened);
        output_item("Child 2 Button 1");
        output_item("Child 2 Button 2");
        output_item("Child 2 Button 3");
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Child 3", ImVec2(200, 200), ImGuiChildFlags_NavFlattened);
        ImGui::BeginChild("Child 3B", ImVec2(0, 0), ImGuiChildFlags_NavFlattened);
        output_item("Child 3B Button 1");
        output_item("Child 3B Button 2");
        output_item("Child 3B Button 3");
        ImGui::EndChild();
        ImGui::EndChild();

        // FIXME-NAV: To test PageUp/PageDown/Home/End later
        //ImGui::BeginChild("Child 4", ImVec2(200, 200), ImGuiChildFlags_NavFlattened);
        //output_item("Child 4 Button 1");
        //output_item("Child 4 Button 2");
        //output_item("Child 4 Button 3");
        //ImGui::EndChild();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;

        // Navigating in child
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window 1/Button 1"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window 1/Button 2"));
        ctx->KeyPress(ImGuiKey_RightArrow);

        // Parent -> Child
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/Child 1 Button 2"));
        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/Child 1 Button 1"));

        // Child -> Parent
        ctx->KeyPress(ImGuiKey_LeftArrow);
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Window 1/Button 2"));

        // Parent -> Child -> Child
        ctx->KeyPress(ImGuiKey_RightArrow);
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/Child 2 Button 2"));

        // Child -> nested Child
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/Child 3B Button 2"));

        // FIXME: test PageUp/PageDown on child
    };

    // ## Check default focus with _NavFlattened flag
    t = IM_REGISTER_TEST(e, "nav", "nav_flattened_focus_default");
    t->SetVarsDataType<DefaultFocusVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DefaultFocusVars& vars = ctx->GetVars<DefaultFocusVars>();
        if (!vars.ShowWindows)
            return;

        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::BeginChild("Child 0", ImVec2(200, 200), ImGuiChildFlags_NavFlattened);
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
        ImGui::BeginChild("Child 1", ImVec2(200, 200), ImGuiChildFlags_NavFlattened);
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
        DefaultFocusVars& vars = ctx->GetVars<DefaultFocusVars>();

        // Test implicit default init on flattened window
        vars.ShowWindows = true;
        vars.SetFocus = false;
        ctx->Yield(2);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Child 0 Button 1", ctx->WindowInfo("Window 1/Child 0").ID));
        vars.ShowWindows = false;
        ctx->Yield(2);

        // Test using SetItemDefaultFocus() on flattened window
        vars.ShowWindows = true;
        vars.SetFocus = true;
        ctx->Yield(2);
#if IMGUI_VERSION_NUM >= 18205
        IM_CHECK_EQ(g.NavId, ctx->GetID("Child 1 Button 2", ctx->WindowInfo("Window 1/Child 1").ID));
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
        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK(g.NavId == ctx->GetID("Help"));
        IM_CHECK(g.NavDisableHighlight == false);
        IM_CHECK(g.NavDisableMouseHover == true);
        ctx->MouseMove("Help");
        IM_CHECK(g.NavDisableHighlight == false); // Moving mouse doesn't set this to true: rect will be visible but NavId not marked as "hovered"
        IM_CHECK(g.NavDisableMouseHover == false);

        ctx->KeyPress(ImGuiMod_Alt);
        IM_CHECK(g.NavDisableHighlight == false);
        IM_CHECK(g.NavDisableMouseHover == true);

#if IMGUI_VERSION_NUM >= 18992
        // Switching from Mouse to keyboard
        ctx->ItemOpen("Inputs & Focus");
        ctx->ItemOpen("Tabbing");
        ctx->ItemClick("Tabbing/2");
        IM_CHECK(g.NavDisableHighlight == true);
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Tabbing/3"));
        IM_CHECK(g.NavDisableHighlight == false);
#endif
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
        ctx->KeyPress(ImGuiMod_Alt); // FIXME
        ctx->SetRef(ctx->UiContext->NavWindow);

        // Navigate to "c" item.
        IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("a"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("c"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("a"));
    };

    // ## Test nav from non-visible focus with keyboard/gamepad. With gamepad, the navigation resumes on the next visible item instead of next item after focused one.
    t = IM_REGISTER_TEST(e, "nav", "nav_from_clipped_item");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *GImGui;
        auto& vars = ctx->GenericVars;

        ImGui::SetNextWindowSize(vars.WindowSize, ImGuiCond_Always);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
        for (int y = 0; y < 6; y++)
            for (int x = 0; x < 6; x++)
            {
                ImGui::Button(Str16f("Button %d,%d", x, y).c_str());
                if (x < 5)
                    ImGui::SameLine();

                // Window size is such that only 3x3 buttons are visible at a time.
                if (vars.WindowSize.y == 0.0f && y == 3 && x == 2)
                    vars.WindowSize = ImGui::GetCursorPos() + ImVec2(g.Style.ScrollbarSize, g.Style.ScrollbarSize); // FIXME: Calculate Window Size from Decoration Size
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
            ctx->SetInputMode(input_source);
            //ctx->UiContext->NavInputSource = input_source;  // FIXME-NAV: Should be set by above ctx->SetInputMode(ImGuiInputSource_Gamepad) call, but those states are a mess.

            // Down
            ctx->NavMoveTo("Button 0,0");
            ctx->ScrollToX("", 0.0f);
            ctx->ScrollToBottom("");
            ctx->KeyPress((input_source == ImGuiInputSource_Keyboard) ? ImGuiKey_DownArrow : ImGuiKey_GamepadDpadDown);
            if (input_source == ImGuiInputSource_Keyboard)
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 0,1"));     // Started Nav from Button 0,0 (Previous NavID)
            else
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 0,3"));     // Started Nav from Button 0,2 (Visible)

            ctx->KeyPress(ImGuiKey_UpArrow);                                    // Move to opposite direction than previous operation in order to trigger scrolling focused item into view
            IM_CHECK(window->InnerRect.Contains(get_focus_item_rect(window)));  // Ensure item scrolled into view is fully visible

            // Up
            ctx->NavMoveTo("Button 0,4");
            ctx->ScrollToX("", 0);
            ctx->ScrollToTop("");
            ctx->KeyPress((input_source == ImGuiInputSource_Keyboard) ? ImGuiKey_UpArrow : ImGuiKey_GamepadDpadUp);
            if (input_source == ImGuiInputSource_Keyboard)
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 0,3"));
            else
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 0,2"));

            ctx->KeyPress(ImGuiKey_DownArrow);
            IM_CHECK(window->InnerRect.Contains(get_focus_item_rect(window)));

            // Right
            ctx->NavMoveTo("Button 0,0");
            ctx->ScrollToX("", window->ScrollMax.x);
            ctx->ScrollToTop("");
            ctx->KeyPress((input_source == ImGuiInputSource_Keyboard) ? ImGuiKey_RightArrow : ImGuiKey_GamepadDpadRight);
            if (input_source == ImGuiInputSource_Keyboard)
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 1,0"));
            else
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 3,0"));

            ctx->KeyPress(ImGuiKey_LeftArrow);
            IM_CHECK(window->InnerRect.Contains(get_focus_item_rect(window)));

            // Left
            ctx->NavMoveTo("Button 4,0");
            ctx->ScrollToX("", 0);
            ctx->ScrollToTop("");
            ctx->KeyPress((input_source == ImGuiInputSource_Keyboard) ? ImGuiKey_LeftArrow : ImGuiKey_GamepadDpadLeft);
            if (input_source == ImGuiInputSource_Keyboard)
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 3,0"));
            else
                IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Button 2,0"));

            ctx->KeyPress((input_source == ImGuiInputSource_Keyboard) ? ImGuiKey_RightArrow : ImGuiKey_GamepadDpadRight);
            IM_CHECK(window->InnerRect.Contains(get_focus_item_rect(window)));
        }
    };

    // ## Test PageUp/PageDown/Home/End/arrow keys
    t = IM_REGISTER_TEST(e, "nav", "nav_page_home_end_arrows");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        if (ctx->IsGuiFuncOnly())
        {
            ImGui::Begin("Config", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
            ImGui::Checkbox("UseClipper", &vars.UseClipper);
            ImGui::End();
        }

        ImGui::SetNextWindowSize(ImVec2(100, vars.WindowSize.y), ImGuiCond_Always);

        // FIXME-NAV: Lack of ImGuiWindowFlags_NoCollapse breaks window scrolling without activable items.
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoCollapse);
        vars.WindowSize.y = ImFloor(ImGui::GetCursorPosY() + ImGui::GetTextLineHeightWithSpacing() * 20.8f + ImGui::GetStyle().ScrollbarSize);
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
            ctx->ScrollToX("", 0.0f);
            ctx->ScrollToY("", 0.0f);

#if IMGUI_VERSION_NUM < 18503
            ctx->KeyPress(ImGuiKey_PageDown);
            IM_CHECK(g.NavId == ctx->GetID("OK 0"));            // FIXME-NAV: Main layer had no focus, key press simply focused it. We preserve this behavior across multiple test runs in the same session by resetting window->NavRectRel.
#endif
            ctx->KeyPress(ImGuiKey_PageDown);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 20"));          // Now focus changes to a last visible button.
            IM_CHECK(window->Scroll.y == 0.0f);                 // Window is not scrolled.
            ctx->KeyPress(ImGuiKey_PageDown);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 40"));          // Focus item on the next "page".
            IM_CHECK(window->Scroll.y > 0.0f);
            ctx->KeyPress(ImGuiKey_End);                     // Focus last item.
            IM_CHECK(window->Scroll.y == window->ScrollMax.y);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 199"));
            ctx->KeyPress(ImGuiKey_PageUp);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 179"));         // Focus first item visible in the last section.
            IM_CHECK(window->Scroll.y == window->ScrollMax.y);  // Window is not scrolled.
            ctx->KeyPress(ImGuiKey_PageUp);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 159"));         // Focus first item of previous "page".
            IM_CHECK(0 < window->Scroll.y && window->Scroll.y < window->ScrollMax.y); // Window is not scrolled.
            ctx->KeyPress(ImGuiKey_RightArrow);
            IM_CHECK(window->Scroll.x > 0.0f);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK B159"));        // Focus first item in a second column, scrollbar moves.
            ctx->KeyPress(ImGuiKey_LeftArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 159"));         // Focus first item in a second column, scrollbar moves.
            ctx->KeyPress(ImGuiKey_Home);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 0"));           // Focus very first item.
            IM_CHECK_EQ(window->Scroll.y, 0.0f);                // Window is scrolled to the start.

            // Test arrows keys WITH navigable items.
            // (This is a duplicate of most basic nav tests)
            ctx->KeyPress(ImGuiKey_DownArrow);
            IM_CHECK(window->Scroll.y == 0.0f);
            IM_CHECK(g.NavId == ctx->GetID("OK 1"));            // Focus second item without moving scrollbar.
            ctx->KeyPress(ImGuiKey_UpArrow);
            IM_CHECK(window->Scroll.y == 0.0f);
            IM_CHECK(g.NavId == ctx->GetID("OK 0"));            // Focus first item without moving scrollbar.
            IM_CHECK(window->Scroll.x == 0.0f);
            ctx->KeyPress(ImGuiKey_RightArrow);
            IM_CHECK(window->Scroll.x > 0.0f);
            IM_CHECK(g.NavId == ctx->GetID("OK B0"));           // Focus first item in a second column, scrollbar moves.
            ctx->KeyPress(ImGuiKey_LeftArrow);
            IM_CHECK_EQ(window->Scroll.x, 0.0f);
            IM_CHECK_EQ(g.NavId, ctx->GetID("OK 0"));           // Focus first item in first column, scrollbar moves back.
        }
    };

    // ## Test PageUp/PageDown/Home/End/arrow keys
    t = IM_REGISTER_TEST(e, "nav", "nav_scroll_when_no_items");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        if (ctx->IsGuiFuncOnly())
        {
            ImGui::Begin("Config", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::Checkbox("UseClipper", &vars.UseClipper);
            ImGui::End();
        }

        // FIXME-NAV: Lack of ImGuiWindowFlags_NoCollapse breaks window scrolling without activable items.
        ImGui::SetNextWindowSize(ImVec2(100, vars.WindowSize.y), ImGuiCond_Always);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoCollapse);
        vars.WindowSize.y = ImFloor(ImGui::GetCursorPosY() + ImGui::GetFrameHeightWithSpacing() * 3.0f + ImGui::GetStyle().ScrollbarSize);
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

            ctx->KeyHold(ImGuiKey_PageDown, 0.1f);           // Scrolled down some, but not to the bottom.
            IM_CHECK(0 < window->Scroll.y && window->Scroll.y < window->ScrollMax.y);
            ctx->KeyPress(ImGuiKey_End);                     // Scrolled all the way to the bottom.
            IM_CHECK_EQ(window->Scroll.y, window->ScrollMax.y);
            float last_scroll = window->Scroll.y;            // Scrolled up some, but not all the way to the top.
            ctx->KeyHold(ImGuiKey_PageUp, 0.1f);
            IM_CHECK(0 < window->Scroll.y && window->Scroll.y < last_scroll);
            ctx->KeyPress(ImGuiKey_Home);                    // Scrolled all the way to the top.
            IM_CHECK_EQ(window->Scroll.y, 0.0f);

            // Test arrows keys WITHOUT any navigable items
            ctx->KeyHold(ImGuiKey_DownArrow, 0.1f);          // Scrolled window down by one tick.
            IM_CHECK_GT(window->Scroll.y, 0.0f);
            ctx->KeyHold(ImGuiKey_UpArrow, 0.1f);            // Scrolled window up by one tick (back to the top).
            IM_CHECK_EQ(window->Scroll.y, 0.0f);
            IM_CHECK_EQ(window->Scroll.x, 0.0f);
            ctx->KeyHold(ImGuiKey_RightArrow, 0.1f);         // Scrolled window right by one tick.
            IM_CHECK_GT(window->Scroll.x, 0.0f);
            ctx->KeyHold(ImGuiKey_LeftArrow, 0.1f);          // Scrolled window left by one tick (back to the start).
            IM_CHECK_EQ(window->Scroll.x, 0.0f);
        }
    };

    // ## Test using TAB to cycle through items
    t = IM_REGISTER_TEST(e, "nav", "nav_tabbing_basic");
    struct TabbingVars { int Step = 0; int WidgetType = 0; float Floats[5] = { 0, 0, 0, 0, 0 }; char Bufs[256][5] = { "buf0", "buf1", "buf2", "buf3", "buf4" }; };
    t->SetVarsDataType<TabbingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TabbingVars>();

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
        ImGui::PushTabStop(false);
        out_widget("Item2", 2);
        ImGui::PopTabStop();
        out_widget("Item3", 3);
        out_widget("Item4", 4);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GetVars<TabbingVars>();

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
                //if (vars.WidgetType == 0 && vars.Step == 0)
                    ctx->KeyPress(ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item0"));
#endif
                ctx->KeyPress(ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item1"));
                ctx->KeyPress(ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item3"));
                ctx->KeyPress(ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item4"));

                // Test wrapping
                ctx->KeyPress(ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item0"));

                // Test Shift+Tab
                ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item4"));
                ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item3"));
                ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item1"));
                ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item0"));
                ctx->KeyPress(ImGuiKey_Escape);

#if IMGUI_VERSION_NUM >= 18936
                // Test departing from a TabStop=0 item
                ctx->ItemInput("Item2");
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item2"));
                ctx->KeyPress(ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item3"));
                ctx->ItemInput("Item2");
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item2"));
                ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_Tab);
                IM_CHECK_EQ(g.ActiveId, ctx->GetID("Item1"));

                ctx->ItemInput("Item0");
                ctx->KeyPress(ImGuiKey_Escape);
#endif
            }
    };

    // ## Test tabbing through clipped/non-visible items (#4449) + using ImGuiListClipper
#if IMGUI_VERSION_NUM >= 18508
    t = IM_REGISTER_TEST(e, "nav", "nav_tabbing_clipped");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

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
            ctx->KeyPress(ImGuiKey_Tab);
            IM_CHECK_EQ(g.ActiveId, ctx->GetID(Str30f("Input%d", n % 50).c_str()));
        }
        // Should be on 51
        for (int n = 0; n < 4; n++)
        {
            IM_CHECK_EQ(g.ActiveId, ctx->GetID(Str30f("Input%d", (51 - n) % 50).c_str()));
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_Tab);
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
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        for (int n = 0; n < 4; n++)
            ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);

        ImGui::BeginChild("Child 1", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 3), ImGuiChildFlags_Border | ImGuiChildFlags_NavFlattened);
        for (int n = 4; n < 8; n++)
            ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);
        ImGui::EndChild();

        for (int n = 8; n < 10; n++)
            ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);

        ImGui::BeginChild("Child 2", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 3), ImGuiChildFlags_Border | ImGuiChildFlags_NavFlattened);
        for (int n = 10; n < 14; n++)
            ImGui::InputInt(Str30f("Input%d", n).c_str(), &vars.Int1, 0, 0);
        ImGui::BeginChild("Child 3", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 3), ImGuiChildFlags_Border | ImGuiChildFlags_NavFlattened);
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
        ImGuiTestItemInfo item_info;
        ctx->SetRef("Test Window");
        for (int n = 0; n < 22; n++)
        {
            ctx->KeyPress(ImGuiKey_Tab);
            item_info = ctx->ItemInfo(g.ActiveId);
            IM_CHECK(item_info.ID != 0);
            IM_CHECK_STR_EQ(item_info.DebugLabel, Str30f("Input%d", n % 20).c_str());
        }
        // Should be on 21
        for (int n = 0; n < 4; n++)
        {
            item_info = ctx->ItemInfo(g.ActiveId);
            IM_CHECK(item_info.ID != 0);
            IM_CHECK_STR_EQ(item_info.DebugLabel, Str30f("Input%d", (21 - n) % 20).c_str());

            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_Tab);

            item_info = ctx->ItemInfo(g.ActiveId);
            IM_CHECK(item_info.ID != 0);
            IM_CHECK_STR_EQ(item_info.DebugLabel, Str30f("Input%d", (20 - n) % 20).c_str());
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
            ImGui::PushTabStop(false);
            ImGui::SetKeyboardFocusHere();
            ImGui::InputText("Text1", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            vars.Status.QuerySet();
            ImGui::PopTabStop();
        }
        else if (vars.Step == 8)
        {
            ImGui::SetKeyboardFocusHere();
            ImGui::Button("Button1");
            vars.Status.QuerySet();
        }
        else if (vars.Step == 9) // #4761
        {
            if (ImGui::Button("Focus"))
            //if (!ImGui::IsAnyItemActive())
                ImGui::SetKeyboardFocusHere();
            ImGui::InputTextMultiline("TextMultiline1", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            vars.Status.QuerySet();
        }
        else if (vars.Step == 10) // #4761
        {
            bool focus = ImGui::Button("Focus");
            ImGui::InputTextMultiline("TextMultiline2", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            vars.Status.QuerySet();
            if (focus)
                //if (!ImGui::IsAnyItemActive())
                ImGui::SetKeyboardFocusHere(-1);
        }
        else if (vars.Step == 11) // Test non-wrapping of SetKeyboardFocusHere(0)
        {
            ImGui::InputText("Text1", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            ImGui::InputText("Text2", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            ImGui::SetKeyboardFocusHere(0);
            ImGui::Begin("Test Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::InputText("Text3", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            ImGui::End();
        }
        else if (vars.Step == 12) // Test non-wrapping of SetKeyboardFocusHere(-1)
        {
            ImGui::SetKeyboardFocusHere(-1);
            ImGui::InputText("Text1", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            ImGui::InputText("Text2", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            ImGui::Begin("Test Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::InputText("Text3", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            ImGui::End();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");

        // Test focusing next item with SetKeyboardFocusHere(0)
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

#if IMGUI_VERSION_NUM >= 18512
        vars.Step = 0;
        ctx->Yield(2);

        // Test focusing next item with SetKeyboardFocusHere(0)
        vars.Step = 9;
        ctx->Yield();
        IM_CHECK(g.ActiveId == 0);
        IM_CHECK(vars.Status.Active == 0);
        ctx->ItemClick("Focus");
        ctx->Yield(2);
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("TextMultiline1"));
        IM_CHECK(vars.Status.Active == 1);

        // Test that ActiveID gets cleared when not alive
        vars.Step = 0;
        ctx->Yield(2);
        IM_CHECK(g.ActiveId == 0);

        // Test focusing previous item with SetKeyboardFocusHere(-1)
        vars.Step = 10;
        //IM_SUSPEND_TESTFUNC();
        ctx->Yield();
        IM_CHECK(g.ActiveId == 0);
        IM_CHECK(vars.Status.Active == 0);
        ctx->ItemClick("Focus");
        ctx->Yield(2);
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("TextMultiline2"));
        IM_CHECK_EQ(vars.Status.Active, 1);
#endif

#if IMGUI_VERSION_NUM >= 18728
        // Test non-wrapping of SetKeyboardFocusHere(0)
        vars.Step = 11;
        ctx->Yield(3);
        IM_CHECK(g.ActiveId == 0);

        // Test non-wrapping of SetKeyboardFocusHere(-1)
        vars.Step = 12;
        ctx->Yield(3);
        IM_CHECK(g.ActiveId == 0);
#endif

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
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("Float4/$$2"));

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

        // Test API focusing an item that has PushTabStop(false)
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

        // Test that SetKeyboardFocusHere() on a Button() does not trigger it.
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
    struct SetFocusVars { char Str1[256] = ""; int Step = 0; ImGuiTestGenericItemStatus Status[3]; };
    t->SetVarsDataType<SetFocusVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        SetFocusVars& vars = ctx->GetVars<SetFocusVars>();
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

        SetFocusVars& vars = ctx->GetVars<SetFocusVars>();

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

#if IMGUI_VERSION_NUM < 19002 || IMGUI_VERSION_NUM >= 19012
    // ## Test SetKeyboardFocusHere() accross windows (#7226)
    t = IM_REGISTER_TEST(e, "nav", "nav_focus_api_remote");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        int set_focus = -1;
        ImGui::Begin("Test Window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Focus A")) { set_focus = 0; }
        if (ImGui::Button("Focus B")) { set_focus = 1; }
        ImGui::End();

        ImGui::Begin("Test Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Focus A")) { set_focus = 0; }
        if (ImGui::Button("Focus B")) { set_focus = 1; }
        ImGui::End();

        char dummy[16];
        ImGui::Begin("Test Window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (set_focus == 0)
            ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Item A", dummy, IM_ARRAYSIZE(dummy));
        ImGui::End();
        ImGui::Begin("Test Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (set_focus == 1)
            ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Item B", dummy, IM_ARRAYSIZE(dummy));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->ItemClick("//Test Window 1/Focus A");
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("//Test Window 1/Item A"));
        ctx->ItemClick("//Test Window 1/Focus B");
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("//Test Window 2/Item B"));
        ctx->ItemClick("//Test Window 2/Focus A");
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("//Test Window 1/Item A"));
        ctx->ItemClick("//Test Window 2/Focus B");
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("//Test Window 2/Item B"));
    };
#endif

    // ## Test wrapping behavior
    t = IM_REGISTER_TEST(e, "nav", "nav_wrapping");
    struct NavWrappingWars { ImGuiNavMoveFlags WrapFlags = ImGuiNavMoveFlags_WrapY; bool UseButton = false; bool AltLayout = false; };
    t->SetVarsDataType<NavWrappingWars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<NavWrappingWars>();

        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Appearing);

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        for (int ny = 0; ny < 4; ny++)
        {
            for (int nx = 0; nx < 4; nx++)
            {
                if (vars.AltLayout == false && ny == 3 && nx >= 2)
                    continue;
                if (vars.AltLayout == true && nx == 3 && ny >= 2)
                    continue;
                if (nx > 0)
                    ImGui::SameLine();
                if (vars.UseButton)
                    ImGui::Button(Str30f("%d,%d", ny, nx).c_str(), ImVec2(40, 40));
                else
                    ImGui::Selectable(Str30f("%d,%d", ny, nx).c_str(), true, 0, ImVec2(40,40));
            }
        }
        if (vars.WrapFlags != 0)
            ImGui::NavMoveRequestTryWrapping(ImGui::GetCurrentWindow(), vars.WrapFlags);
        ImVec2 options_pos = ImGui::GetCurrentWindow()->Rect().GetBL();
        ImGui::End();

        // Options
        //if (ctx->IsGuiFuncOnly())
        {
            ImGui::SetNextWindowPos(options_pos);
            ImGui::Begin("Test Options", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
            ImGui::RadioButton("ImGuiNavMoveFlags_None", &vars.WrapFlags, ImGuiNavMoveFlags_None);
            ImGui::RadioButton("ImGuiNavMoveFlags_WrapX", &vars.WrapFlags, ImGuiNavMoveFlags_WrapX);
            ImGui::RadioButton("ImGuiNavMoveFlags_WrapY", &vars.WrapFlags, ImGuiNavMoveFlags_WrapY);
            ImGui::RadioButton("ImGuiNavMoveFlags_LoopX", &vars.WrapFlags, ImGuiNavMoveFlags_LoopX);
            ImGui::RadioButton("ImGuiNavMoveFlags_LoopY", &vars.WrapFlags, ImGuiNavMoveFlags_LoopY);
            ImGui::Checkbox("AltLayout", &vars.AltLayout);
            ImGui::End();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GetVars<NavWrappingWars>();
        vars.UseButton = false;
        ctx->SetRef("Test Window");

        for (int input_source = 0; input_source < 2; input_source++)
        {
            ImGuiKey key_up = (input_source == 0) ? ImGuiKey_UpArrow : ImGuiKey_GamepadDpadUp;
            ImGuiKey key_down = (input_source == 0) ? ImGuiKey_DownArrow : ImGuiKey_GamepadDpadDown;
            ImGuiKey key_left = (input_source == 0) ? ImGuiKey_LeftArrow : ImGuiKey_GamepadDpadLeft;
            ImGuiKey key_right = (input_source == 0) ? ImGuiKey_RightArrow : ImGuiKey_GamepadDpadRight;

            vars.WrapFlags = ImGuiNavMoveFlags_None;
            ctx->ItemClick("0,0");
            ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,1"));
            ctx->KeyPress(key_right);
            ctx->KeyPress(key_right);
            IM_CHECK(g.NavId == ctx->GetID("0,3"));
            ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,3"));
#if IMGUI_VERSION_NUM >= 18956
            ctx->ItemClick("3,1");
            ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("3,1"));
            ctx->ItemClick("2,2");
            ctx->KeyPress(key_down);
#if 1       // FIXME: 2023/05/15: It is debatable whether pressing down here should reach (3,1). The opposite (3,1) + Right currently doesn't reach (2.2)
            IM_CHECK_EQ(g.NavId, ctx->GetID("3,1"));
            ctx->KeyPress(key_up);
#endif
            IM_CHECK_EQ(g.NavId, ctx->GetID("2,2")); // Restore preferred pos
#endif

            vars.WrapFlags = ImGuiNavMoveFlags_WrapX;
            ctx->ItemClick("0,0");
            ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,1"));
            ctx->KeyPress(key_right);
            ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,3"));
            ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("1,0"));
            for (int n = 0; n < 50; n++) // Mash to get to the end of list
                ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("3,1"));
#if IMGUI_VERSION_NUM >= 18957
            ctx->KeyPress(key_left);
            ctx->KeyPress(key_left);
            IM_CHECK_EQ(g.NavId, ctx->GetID("2,3"));
#endif

            vars.WrapFlags = ImGuiNavMoveFlags_LoopX;
            ctx->ItemClick("0,0");
            ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,1"));
            ctx->KeyPress(key_right);
            ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,3"));
            ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,0"));
            ctx->KeyPress(key_left);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,3"));
            ctx->ItemClick("3,0");
            ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("3,1"));
            ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("3,0"));
            ctx->KeyPress(key_left);
            IM_CHECK_EQ(g.NavId, ctx->GetID("3,1"));
            ctx->KeyPress(key_left);
            IM_CHECK_EQ(g.NavId, ctx->GetID("3,0"));

            // FIXME: LoopY/WrapY currently have some issues easy to see with AltLayout=false
            // Presumably due to Y bias since the equivalent problem with AltLayout=true doesn't happen with LoopX/WrapX
            vars.WrapFlags = ImGuiNavMoveFlags_LoopY;
            ctx->ItemClick("0,0");
            ctx->KeyPress(key_right);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,1"));
            ctx->KeyPress(key_down);
            ctx->KeyPress(key_down);
            ctx->KeyPress(key_down);
            IM_CHECK_EQ(g.NavId, ctx->GetID("3,1"));
            ctx->KeyPress(key_down);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,1"));
            ctx->KeyPress(key_up);
            IM_CHECK_EQ(g.NavId, ctx->GetID("3,1"));
        }

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
        ImGuiTestGenericVars& vars = ctx->GenericVars;
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

        for (int input_source = 0; input_source < 2; input_source++)
        {
#if IMGUI_VERSION_NUM < 18613
            if (input_source == 1)
                continue;
#endif
            ImGuiKey key_up = (input_source == 0) ? ImGuiKey_UpArrow : ImGuiKey_GamepadDpadUp;
            ImGuiKey key_down = (input_source == 0) ? ImGuiKey_DownArrow : ImGuiKey_GamepadDpadDown;

            for (int n = 0; n < 52; n++)
            {
                IM_CHECK_EQ(g.NavId, ctx->GetID(Str30f("Input%d", (n) % 50).c_str()));
                ctx->KeyPress(key_down);
                IM_CHECK_EQ(g.NavId, ctx->GetID(Str30f("Input%d", (n + 1) % 50).c_str()));
            }
            // Should be on 51
            for (int n = 0; n < 4; n++)
            {
                IM_CHECK_EQ(g.NavId, ctx->GetID(Str30f("Input%d", (52 - n) % 50).c_str()));
                ctx->KeyPress(key_up);
                IM_CHECK_EQ(g.NavId, ctx->GetID(Str30f("Input%d", (51 - n) % 50).c_str()));
            }

            ctx->NavMoveTo("Input0");
        }
    };

    // ## Test clipping behavior within columns which are not "push-scrollable" containers (#2221)
#if IMGUI_VERSION_NUM >= 18952
    t = IM_REGISTER_TEST(e, "nav", "nav_columns_clip");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImVec2 sz_large = ImVec2(400, 0.0f);
        ImVec2 sz_small = ImVec2(0.0f, 0.0f); // Auto
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize({ 200, 500 });
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGui::Columns(2);
        ImGui::Button("0-SMALL", sz_small);
        ImGui::NextColumn();
        ImGui::Button("0-LARGE", sz_large);
        ImGui::NextColumn();
        ImGui::Button("1-LARGE", sz_large);
        ImGui::NextColumn();
        ImGui::Button("1-SMALL", sz_small);
        ImGui::Columns(1);

        ImGui::Separator();
        if (ImGui::BeginTable("table", 2))
        {
            ImGui::TableNextColumn();
            ImGui::Button("2-SMALL", sz_small);
            ImGui::TableNextColumn();
            ImGui::Button("2-LARGE", sz_large);
            ImGui::TableNextColumn();
            ImGui::Button("3-LARGE", sz_large);
            ImGui::TableNextColumn();
            ImGui::Button("3-SMALL", sz_small);
            ImGui::EndTable();
        }
        ImGui::End();
        ImGui::PopStyleVar();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Test Window");

        for (int set_n = 0; set_n < 4; set_n++)
        {
            const char* item_L = NULL;
            const char* item_R = NULL;
            switch (set_n)
            {
            case 0: item_L = "0-SMALL"; item_R = "0-LARGE"; break;
            case 1: item_L = "1-LARGE"; item_R = "1-SMALL"; break;
            case 2: item_L = "table/2-SMALL"; item_R = "table/2-LARGE"; break;
            case 3: item_L = "table/3-LARGE"; item_R = "table/3-SMALL"; break;
            }
            ctx->ItemClick(item_L);
            IM_CHECK_EQ(g.NavId, ctx->GetID(item_L));
            ctx->KeyPress(ImGuiKey_RightArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID(item_R));
            ctx->KeyPress(ImGuiKey_RightArrow); // No-op
            IM_CHECK_EQ(g.NavId, ctx->GetID(item_R));
            ctx->KeyPress(ImGuiKey_LeftArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID(item_L));
            ctx->KeyPress(ImGuiKey_LeftArrow); // No-op
            IM_CHECK_EQ(g.NavId, ctx->GetID(item_L));
        }
    };
#endif
}

//-------------------------------------------------------------------------
