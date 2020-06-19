// dear imgui
// (tests: docking)

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

// Helpers
static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }    // for IM_CHECK_EQ()

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

    t = IM_REGISTER_TEST(e, "docking", "docking_basic_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ImGuiID ids[3];
        ImGui::Begin("AAAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        ids[0] = ImGui::GetWindowDockID();
        ImGui::Text("This is AAAA");
        ImGui::End();

        ImGui::Begin("BBBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        ids[1] = ImGui::GetWindowDockID();
        ImGui::Text("This is BBBB");
        ImGui::End();

        ImGui::Begin("CCCC", NULL, ImGuiWindowFlags_NoSavedSettings);
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
    t = IM_REGISTER_TEST(e, "docking", "docking_basic_set_next_api");
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
    t = IM_REGISTER_TEST(e, "docking", "docking_basic_initial_node_size");
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
    t = IM_REGISTER_TEST(e, "docking", "docking_into_parent_node");
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
    t = IM_REGISTER_TEST(e, "docking", "docking_auto_nodes_size");
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
    t = IM_REGISTER_TEST(e, "docking", "docking_undock_from_dockspace_size");
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
        IM_CHECK_EQ(window_a->DockNode->ID, dockspace_id);

        ctx->UndockNode(dockspace_id);

        IM_CHECK(window_a->DockNode != NULL);
        IM_CHECK(window_b->DockNode != NULL);
        IM_CHECK_EQ(window_a->DockNode, window_b->DockNode);
        IM_CHECK_NE(window_a->DockNode->ID, dockspace_id); // Actually undocked?
        ImVec2 window_a_size_new = window_a->Size;
        ImVec2 window_b_size_new = window_b->Size;
        IM_CHECK_EQ(window_a->DockNode->Size, window_b_size_new);
        IM_CHECK_EQ(window_a_size_old, window_a_size_new);
        IM_CHECK_EQ(window_b_size_old, window_b_size_new);
    };

    // ## Test merging windows by dragging them.
    t = IM_REGISTER_TEST(e, "docking", "docking_drag_merge");
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
    t = IM_REGISTER_TEST(e, "docking", "docking_focus_1");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        if (ctx->FrameCount == 0)
            vars.DockId = ctx->DockMultiSetupBasic(0, "AAAA", "BBBB", "CCCC", NULL);

        if (ctx->FrameCount == 10)  ImGui::SetNextWindowFocus();
        ImGui::Begin("AAAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ctx->FrameCount == 10)  IM_CHECK(ImGui::IsWindowFocused());
        if (ctx->FrameCount == 10)  ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Input", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        if (ctx->FrameCount == 10)  IM_CHECK(!ImGui::IsItemActive());
        if (ctx->FrameCount == 11)  IM_CHECK(ImGui::IsItemActive());
        ImGui::End();

        if (ctx->FrameCount == 50)  ImGui::SetNextWindowFocus();
        ImGui::Begin("BBBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ctx->FrameCount == 50)  IM_CHECK(ImGui::IsWindowFocused());
        if (ctx->FrameCount == 50)  ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Input", vars.Str2, IM_ARRAYSIZE(vars.Str2));
        if (ctx->FrameCount == 50)  IM_CHECK(!ImGui::IsItemActive());
        if (ctx->FrameCount == 51)  IM_CHECK(ImGui::IsItemActive());
        ImGui::End();

        if (ctx->FrameCount == 60)
            ctx->Finish();
    };

    // ## Test window undocking.
    t = IM_REGISTER_TEST(e, "docking", "docking_undock_tabs_and_nodes");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();

        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window1 = ctx->GetWindowByRef("Window 1");
        ImGuiWindow* window2 = ctx->GetWindowByRef("Window 2");
        ImGuiWindow* demo_window = ctx->GetWindowByRef("Dear ImGui Demo");
        ctx->DockMultiClear("Window 1", "Window 2", "Dear ImGui Demo", NULL);

        // Test undocking from tab.
        ctx->DockWindowInto("Window 1", "Window 2");
        IM_CHECK(window1->DockNode != NULL);
        IM_CHECK(window1->DockNode == window2->DockNode);
        ctx->UndockWindow("Window 1");
        IM_CHECK(window1->DockNode == NULL || ctx->DockIdIsUndockedOrStandalone(window1->DockNode->ID));
        IM_CHECK(window2->DockNode == NULL || ctx->DockIdIsUndockedOrStandalone(window2->DockNode->ID));

        // Ensure demo window is visible.
        ctx->WindowMoveToMakePosVisible(demo_window, demo_window->Pos);
        ctx->WindowMoveToMakePosVisible(demo_window, demo_window->Pos + demo_window->Size);

        // Test undocking from collapse button.
        for (int direction = 0; direction < ImGuiDir_COUNT; direction++)
        {
            // Test undocking with one and two windows.
            for (int n = 0; n < 2; n++)
            {
                ctx->DockMultiClear("Window 1", "Window 2", "Dear ImGui Demo", NULL);
                ctx->Yield();
                ctx->DockWindowInto("Window 1", "Dear ImGui Demo", (ImGuiDir)direction);
                if (n)
                    ctx->DockWindowInto("Window 2", "Window 1");
                IM_CHECK(demo_window->DockNode != NULL);
                IM_CHECK(demo_window->DockNode->ParentNode != NULL);
                IM_CHECK(window1->DockNode != NULL);
                if (n)
                {
                    IM_CHECK(window1->DockNode == window2->DockNode);
                    IM_CHECK(window1->DockNode->ParentNode == window2->DockNode->ParentNode);
                }
                IM_CHECK(window1->DockNode->ParentNode == demo_window->DockNode->ParentNode);
                ctx->UndockNode(window1->DockNode->ID);
                IM_CHECK(window1->DockNode != NULL);
                if (n)
                {
                    IM_CHECK(window1->DockNode == window2->DockNode);
                    IM_CHECK(window2->DockNode->ParentNode == NULL);
                }
                IM_CHECK(window1->DockNode->ParentNode == NULL);
            }
        }
    };
#else
    IM_UNUSED(e);
#endif
}
