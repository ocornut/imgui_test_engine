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
#ifdef IMGUI_HAS_DOCK

struct DockingTestingVars
{
    int         Step = 0;
    ImGuiID     DockSpaceID = 0;
    bool        ShowDockspace = true;
    bool        ShowWindow[10];

    DockingTestingVars() { for (int n = 0; n < IM_ARRAYSIZE(ShowWindow); n++) ShowWindow[n] = true; }
};

static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }    // for IM_CHECK_EQ()
#endif

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

    // ## Test merging windows by dragging them.
    t = IM_REGISTER_TEST(e, "docking", "docking_basic_1");
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
                IM_CHECK(ctx->WindowIsUndockedOrStandalone(window_aaaa));
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
                IM_CHECK(ctx->WindowIsUndockedOrStandalone(window_aaaa));
                IM_CHECK_EQ(window_bbbb->DockId, dock_id);

                // Intentionally move both floating windows away
                ctx->WindowMove("/AAAA", ImVec2(100, 100));
                ctx->WindowMove("/BBBB", ImVec2(200, 200));

                // Dock on the side (BBBB still refers to dock id, making this different from the first docking)
                ctx->DockWindowInto("/AAAA", "/BBBB", ImGuiDir_Left);
                IM_CHECK(window_aaaa->DockNode != NULL);
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

    // ## Test dock with DocKBuilder - via ctx->DockMultiSetupBasic() helper.
    t = IM_REGISTER_TEST(e, "docking", "docking_builder_1");
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
    t = IM_REGISTER_TEST(e, "docking", "docking_api_set_next");
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
#if IMGUI_BROKEN_TESTS
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

    // ## Test that docking into a parent node forwards docking into the central node or last focused node
    t = IM_REGISTER_TEST(e, "docking", "docking_into_parent_node");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ImGuiID dockspace_id = ImHashStr("Dockspace");

        if (ctx->IsFirstTestFrame())
        {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderDockWindow("AAAA", 0);
            ImGui::DockBuilderDockWindow("BBBB", 0);
        }
        if (ctx->FrameCount == 2)
        {
            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.5f, &vars.IdArray[0], &vars.IdArray[1]);
            IM_CHECK(vars.IdArray[0] != 0 && vars.IdArray[1]);
            ImGui::DockBuilderDockWindow("BBBB", dockspace_id); // Try docking BBBB into root dockspace
            ImGuiWindow* window = ImGui::FindWindowByName("BBBB");
            IM_CHECK(window->DockId != dockspace_id);           // -> verify that we've been forwarded.
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
        ctx->Yield(4);
        ctx->LogDebug("ConfigDockingAlwaysTabBar = true");
        ctx->UiContext->IO.ConfigDockingAlwaysTabBar = true;
        ctx->Yield(4);
        ctx->LogDebug("ConfigDockingAlwaysTabBar = false");
        ctx->UiContext->IO.ConfigDockingAlwaysTabBar = false;
        ctx->Yield(4);
        ctx->UiContext->IO.ConfigDockingAlwaysTabBar = backup_cfg;
    };

    // ## Test that undocking a whole _node_ doesn't lose/reset size
    t = IM_REGISTER_TEST(e, "docking", "docking_undock_from_dockspace_size");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiID dockspace_id = ctx->GetID("/Test Window/Dockspace");
        if (ctx->IsFirstTestFrame())
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
        for (int i = 0; i < 4; i++)
        {
            ImGui::SetNextWindowSize(ImVec2(400, 200));
            ImGui::Begin(Str16f("Window %d", i + 1).c_str(), NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::TextUnformatted("lorem ipsum");
            ImGui::End();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window1 = ctx->GetWindowByRef("Window 1");
        ImGuiWindow* window2 = ctx->GetWindowByRef("Window 2");
        ImGuiWindow* window3 = ctx->GetWindowByRef("Window 3");
        ImGuiWindow* window4 = ctx->GetWindowByRef("Window 4");
        ImGuiWindow* demo_window = ctx->GetWindowByRef("Dear ImGui Demo");
        ctx->DockMultiClear("Window 1", "Window 2", "Dear ImGui Demo", NULL);

        //IM_DEBUG_HALT_TESTFUNC();

        // Test undocking from tab.
        ctx->DockWindowInto("Window 1", "Window 2");
        IM_CHECK(window1->DockNode != NULL);
        IM_CHECK(window1->DockNode == window2->DockNode);
        ctx->UndockWindow("Window 1");
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window1));
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window2));

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

        const float h = window1->TitleBarHeight();
        ImVec2 pos;

        for (int n = 0; n < 3; n++)
        {
            ctx->DockMultiClear("Window 1", "Window 2", "Window 3", "Window 4", NULL);
            ctx->DockWindowInto("Window 2", "Window 1");
            ctx->DockWindowInto("Window 3", "Window 2", ImGuiDir_Right);
            ctx->DockWindowInto("Window 4", "Window 3");

            ImGuiDockNode* node1 = window1->DockNode;
            ImGuiDockNode* node2 = window2->DockNode;
            ImGuiDockNode* node3 = window3->DockNode;
            ImGuiDockNode* node4 = window4->DockNode;
            IM_CHECK(node1 != NULL);
            IM_CHECK(node2 != NULL);
            IM_CHECK(node3 != NULL);
            IM_CHECK(node4 != NULL);

            switch (n)
            {
            case 0:
                ctx->LogDebug("Undocking one tab...");
                ctx->UndockWindow("Window 4");
                IM_CHECK(ctx->WindowIsUndockedOrStandalone(window4));   // Dragged window got undocked
                IM_CHECK(window1->DockNode == node1);                   // Dock nodes of other windows remain same
                IM_CHECK(window2->DockNode == node2);
                IM_CHECK(window3->DockNode == node3);
                IM_CHECK(window2->DockNode->HostWindow == window1->DockNode->HostWindow);
                IM_CHECK(window3->DockNode->HostWindow == window1->DockNode->HostWindow);
                break;
            case 1:
                ctx->LogDebug("Undocking entire node...");
                ctx->ItemDragWithDelta(ImHashDecoratedPath("#COLLAPSE", NULL, window4->DockNode->ID), ImVec2(h, h) * -2);
                IM_CHECK(window1->DockNode != NULL);                                       // Dock nodes may have changed, but no window was undocked
                IM_CHECK(window2->DockNode != NULL);
                IM_CHECK(window3->DockNode != NULL);
                IM_CHECK(window4->DockNode != NULL);
                IM_CHECK(window1->DockNode->HostWindow == window2->DockNode->HostWindow);   // Window 0 and Window 1 are in one dock tree
                IM_CHECK(window3->DockNode->HostWindow == window4->DockNode->HostWindow);   // Window 2 and Window 3 are in one dock tree
                IM_CHECK(window1->DockNode->HostWindow != window3->DockNode->HostWindow);   // And both window groups belong to separate trees
                break;
            case 2:
                // Dock state of all windows did not change
                ctx->LogDebug("Moving entire node...");
                IM_CHECK(window1->DockNode == node1);
                IM_CHECK(window2->DockNode == node2);
                IM_CHECK(window3->DockNode == node3);
                IM_CHECK(window4->DockNode == node4);
                IM_CHECK(window1->DockNode->HostWindow == window2->DockNode->HostWindow);
                IM_CHECK(window3->DockNode->HostWindow == window4->DockNode->HostWindow);
                IM_CHECK(window1->DockNode->HostWindow == window3->DockNode->HostWindow);
                pos = window4->DockNode->TabBar->BarRect.Max - ImVec2(1, h * 0.5f);
                ctx->WindowTeleportToMakePosVisibleInViewport(window4->DockNode->HostWindow, pos + ImVec2(10, 10));     // Make space for next drag operation
                pos = window4->DockNode->HostWindow->Pos;
                ctx->MouseMoveToPos(window4->DockNode->TabBar->BarRect.Max - ImVec2(1, h * 0.5f));
                ctx->MouseDragWithDelta(ImVec2(10, 10));
                IM_CHECK_EQ(window4->DockNode->HostWindow->Pos, pos + ImVec2(10, 10));      // Entire dock tree was moved
                break;
            default:
                IM_ASSERT(false);
            }
        }
    };

    // ## Test passthrough docking node.
    t = IM_REGISTER_TEST(e, "docking", "docking_hover_passthru_node");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::SetNextWindowBgAlpha(0.1f); // Not technically necessary but helps understand the purpose of this test
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking);
        ImGui::DockSpace(ImGui::GetID("DockSpace"), ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Window 2", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking);
        ImGui::End();

    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiWindow* window1 = ctx->GetWindowByRef("Window 1");
        ImGuiWindow* window2 = ctx->GetWindowByRef("Window 2");
        ctx->WindowFocus("Window 1");
        ctx->MouseMoveToPos(window1->Rect().GetCenter());
        IM_CHECK_EQ(g.NavWindow, window1);
        IM_CHECK_EQ(g.HoveredWindow, window2);
        IM_CHECK_EQ(g.HoveredDockNode, (ImGuiDockNode*)NULL);
        ctx->MouseClick();
        IM_CHECK_EQ(g.NavWindow, window2);
    };

    // ## Test _KeepAlive dockspace flag.
    //  "Step 1: Window A has dockspace A2. Dock window B into A2.
    //  "Step 2: Window A code call DockSpace with _KeepAlive only when collapsed. Verify that window B is still docked into A2 (and verify that both are HIDDEN at this point).
    //  "Step 3: window A stop submitting the DockSpace() A2. verify that window B is now undocked."
    t = IM_REGISTER_TEST(e, "docking", "docking_dockspace_keep_alive");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
        ImGui::Begin("Window A", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (vars.Step == 0)
            ImGui::DockSpace(ImGui::GetID("A2"), ImVec2(0, 0), 0);
        else if (vars.Step == 1)
            ImGui::DockSpace(ImGui::GetID("A2"), ImVec2(0, 0), ImGuiDockNodeFlags_KeepAliveOnly);
        ImGui::End();

        ImGui::SetNextWindowSize(ImVec2(150, 100), ImGuiCond_Always);
        ImGui::Begin("Window B", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window1 = ctx->GetWindowByRef("Window A");
        ImGuiWindow* window2 = ctx->GetWindowByRef("Window B");
        ImGuiID dock_id = ctx->GetID("Window A/A2");

        if (ImGui::GetIO().ConfigDockingAlwaysTabBar)
        {
            ctx->LogWarning("Cannot run test with io.ConfigDockingAlwaysTabBar == true");
            return;
        }

        ctx->OpFlags |= ImGuiTestOpFlags_NoAutoUncollapse;
        ctx->WindowCollapse(window1, false);
        ctx->WindowCollapse(window2, false);
        ctx->DockMultiClear("Window B", "Window A", NULL);
        ctx->DockWindowInto("Window B", Str64f("Window A\\/DockSpace_%08X", dock_id).c_str());
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window1));           // Window A is not docked
        IM_CHECK_EQ(window2->DockId, dock_id);                          // Window B was docked into a dockspace

        // Start collapse window and start submitting  _KeepAliveOnly flag
        ctx->WindowCollapse(window1, true);
        ctx->GenericVars.Step = 1;
        ctx->Yield();
        IM_CHECK_EQ(window1->Collapsed, true);                          // Window A got collapsed
        IM_CHECK_EQ(window1->DockIsActive, false);                      // and remains undocked
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window1));           //
        IM_CHECK_EQ(window2->Collapsed, false);                         // Window B was not collapsed
        IM_CHECK_EQ(window2->DockIsActive, true);                       // Dockspace is being kept alive
        IM_CHECK_EQ(window2->DockId, dock_id);                          // window remains docked
        IM_CHECK_NE(window2->DockNode, (ImGuiDockNode*)NULL);
        IM_CHECK_EQ(window2->Hidden, true);                             // but invisible
                                                                        // Stop submitting dockspace
        ctx->GenericVars.Step = 2;
        ctx->Yield();
        IM_CHECK_EQ(window1->Collapsed, true);                          // Window A got collapsed
        IM_CHECK_EQ(window1->DockIsActive, false);                      // and remains undocked
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window1));           //
        IM_CHECK_EQ(window2->Collapsed, false);                         // Window B was not collapsed
        IM_CHECK_EQ(window2->DockIsActive, false);                      // Dockspace is no longer kept alive
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window2));           // and window gets undocked
        IM_CHECK_EQ(window2->Hidden, false);                            // Window B shows up
    };

    // ## Test focus retention during undocking when new window appears.
    //  "Dock A and B, when A is visible it also shows NON-child A2.
    //  "Focus B, undock B -> verify that while dragging B it stays focused, even thought A2 just appears. (#3392)"
    t = IM_REGISTER_TEST(e, "docking", "docking_undock_focus_retention");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 100));
        if (ImGui::Begin("Window A", NULL, ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::TextUnformatted("lorem ipsum");

            ImGui::Begin("Window A2", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::TextUnformatted("lorem ipsum");
            ImGui::End();
        }
        ImGui::End();

        ImGui::SetNextWindowSize(ImVec2(200, 100));
        ImGui::Begin("Window B", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("lorem ipsum");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->WindowCollapse(ctx->GetWindowByRef("Window A"), false);
        ctx->DockMultiClear("Window A", "Window B", NULL);
        ImGuiWindow* windowB = ctx->GetWindowByRef("Window B");
        ImGuiWindow* windowA2 = ctx->GetWindowByRef("Window A2");

        // Window A2 is visible when all windows are undocked.
        IM_CHECK(windowA2->Active);
        ctx->WindowFocus("Window A");
        ctx->DockWindowInto("Window B", "Window A");

        // Window A2 gets hidden when Window B is docked as it becomes the active window in the dock.
        IM_CHECK(!windowA2->Active);

        // Undock Window B.
        // FIXME-TESTS: This block is a slightly modified version of the UndockWindow() code which itself is using ItemDragWithDelta(),
        // we cannot use UndockWindow() only because we perform our checks in the middle of the operation.
        {
            ctx->KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift); // Disable docking
            ctx->MouseMove(windowB->Name, ImGuiTestOpFlags_NoCheckHoveredId);
            ctx->MouseDown(0);

            const float h = windowB->TitleBarHeight();
            ctx->MouseMoveToPos(g.IO.MousePos + (ImVec2(h, h) * -2));
            ctx->Yield();

            // Window A2 becomes visible during undock operation, but does not capture focus.
            IM_CHECK(windowA2->Active);
            IM_CHECK_EQ(g.MovingWindow, windowB);
            IM_CHECK_EQ(g.WindowsFocusOrder[g.WindowsFocusOrder.Size - 1], windowB);

            ctx->MouseUp(0);
            ctx->KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
        }
    };

    // ## Test hide/unhiding tab bar
    // "On a 2-way split, test using Hide Tab Bar on both node, then unhiding the tab bar."
    t = IM_REGISTER_TEST(e, "docking", "docking_hide_tabbar");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window A", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("lorem ipsum");
        ImGui::End();

        ImGui::Begin("Window B", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("lorem ipsum");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* windows[] = { ctx->GetWindowByRef("Window A"), ctx->GetWindowByRef("Window B") };

        for (int i = 0; i < IM_ARRAYSIZE(windows); i++)
        {
            ImGuiWindow* window = windows[i];
            ctx->DockMultiClear("Window B", "Window A", NULL);
            ctx->DockWindowInto("Window B", "Window A", ImGuiDir_Right);

            // Two way split. Ensure window is docked and tab bar is visible.
            IM_CHECK(window->DockIsActive);
            IM_CHECK(!window->DockNode->IsHiddenTabBar());

            // Hide tab bar.
            ctx->DockNodeHideTabBar(window->DockNode, true);

            // Unhide tab bar.
            ctx->DockNodeHideTabBar(window->DockNode, false);
        }
    };

    // ## Test dock node retention when second window in two-way split is undocked.
    t = IM_REGISTER_TEST(e, "docking", "docking_undock_simple");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        for (int i = 0; i < 2; i++)
        {
            ImGui::Begin(Str16f("Window %d", i).c_str(), NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextUnformatted("lorem ipsum");
            ImGui::End();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window0 = ctx->GetWindowByRef("Window 0");
        ImGuiWindow* window1 = ctx->GetWindowByRef("Window 1");
        ctx->DockMultiClear("Window 0", "Window 1", NULL);
        ctx->DockWindowInto("Window 1", "Window 0", ImGuiDir_Right);
        IM_CHECK_NE(window0->DockNode, (ImGuiDockNode*)NULL);
        IM_CHECK_NE(window1->DockNode, (ImGuiDockNode*)NULL);
        ImGuiDockNode* original_node = window0->DockNode->ParentNode;
        ctx->UndockWindow("Window 1");
        IM_CHECK_EQ(window0->DockNode, original_node);          // Undocking Window 1 keeps a parent dock node in Window 0
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window1));   // Undocked window
    };

    auto test_docking_over_gui = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 200));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        switch (ctx->Test->ArgVariant)
        {
        case 0:
            ImGui::BeginChild("Child", ImVec2(300, 200 - ImGui::GetCurrentWindow()->TitleBarHeight()));
            ImGui::EndChild();
            break;
        case 1:
            ctx->GenericVars.DockId = ImGui::DockSpace(ImGui::GetID("TestDockspace"));
            break;
        default:
            IM_ASSERT(false);
        }
        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::SetNextWindowSize(ImVec2(100, 200));
        ImGui::Begin("Dock Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
    };

    auto test_docking_over_test = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* dock_window = ctx->GetWindowByRef("Dock Window");
        ImGuiWindow* test_window = ctx->GetWindowByRef("Test Window");

        ctx->DockMultiClear("Dock Window", "Test Window", NULL);
        switch (ctx->Test->ArgVariant)
        {
        case 0:
            ctx->DockWindowInto("Dock Window", "Test Window");
            IM_CHECK(dock_window->DockNode != NULL);
            IM_CHECK(test_window->DockNode != NULL);
            IM_CHECK(test_window->DockNode->HostWindow == dock_window->DockNode->HostWindow);
            break;
        case 1:
        {
            ImGuiID dock_id = ctx->GenericVars.DockId;
            ImGuiDockNode* dock_node = ImGui::DockBuilderGetNode(dock_id);
            ctx->DockWindowInto("Dock Window", dock_node->HostWindow->ID);
            IM_CHECK(dock_window->DockNode != NULL);
            IM_CHECK(ctx->WindowIsUndockedOrStandalone(test_window));
            IM_CHECK(!ctx->WindowIsUndockedOrStandalone(dock_window));
            IM_CHECK(dock_window->DockNode->ID == dock_id);
            break;
        }
        default:
            IM_ASSERT(false);
        }
    };

    // ## Test docking into a window which is entirely covered by a child window.
    t = IM_REGISTER_TEST(e, "docking", "docking_over_child");
    t->GuiFunc = test_docking_over_gui;
    t->TestFunc = test_docking_over_test;
    t->ArgVariant = 0;

    // ## Test docking into a floating dockspace.
    t = IM_REGISTER_TEST(e, "docking", "docking_over_dockspace");
    t->GuiFunc = test_docking_over_gui;
    t->TestFunc = test_docking_over_test;
    t->ArgVariant = 1;

    // ## Test whether docked window tabs are in right order.
    t = IM_REGISTER_TEST(e, "docking", "docking_tab_order");
    t->SetUserDataType<DockingTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DockingTestingVars& vars = ctx->GetUserData<DockingTestingVars>();

        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (vars.ShowDockspace)
        {
            vars.DockSpaceID = ImGui::GetID("dockspace");
            ImGui::DockSpace(vars.DockSpaceID);
        }
        ImGui::End();

        if (vars.ShowWindow[0])
        {
            ImGui::Begin("AAA", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::End();
        }
        if (vars.ShowWindow[1])
        {
            ImGui::Begin("BBB", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::End();
        }
        if (vars.ShowWindow[2])
        {
            ImGui::Begin("CCC", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImGui::End();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        DockingTestingVars& vars = ctx->GetUserData<DockingTestingVars>();

        for (int step = 0; step < 3; step++)
        {
            vars.Step = step;
            vars.ShowDockspace = (vars.Step == 1);

            ctx->DockMultiClear("Test Window", "AAA", "BBB", "CCC", NULL);
            ctx->WindowResize("Test Window", ImVec2(300, 200)); // FIXME-TESTS: Aiming at left-most tab fails with very small node because CollapseButton() hit test is too large.
            ctx->WindowResize("AAA", ImVec2(300, 200));
            ctx->LogInfo("STEP %d", step);

            // All into a dockspace
            if (step == 1)
                ctx->DockWindowIntoEx("AAA", (ImGuiID)0, vars.DockSpaceID);

            // All into a split node of Test Window
            if (step == 2)
                ctx->DockWindowInto("AAA", "Test Window", ImGuiDir_Right);

            ctx->DockWindowInto("BBB", "AAA");
            ctx->DockWindowInto("CCC", "AAA");

            ImGuiWindow* window_aaa = ctx->GetWindowByRef("AAA");
            ImGuiWindow* window_bbb = ctx->GetWindowByRef("BBB"); IM_UNUSED(window_bbb);
            ImGuiWindow* window_ccc = ctx->GetWindowByRef("CCC"); IM_UNUSED(window_ccc);
            IM_CHECK(window_aaa->DockNode != NULL);

            // Verify initial tab order.
            ImGuiTabBar* tab_bar = window_aaa->DockNode->TabBar;
            IM_CHECK(tab_bar != NULL);
            const char* tab_order_initial[] = { "AAA", "BBB", "CCC", NULL };
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar, tab_order_initial));

            // Verify that drag operation past edge of the tab, but not entering other tab does not trigger reorder.
            ctx->ItemDragWithDelta("AAA", ImVec2((tab_bar->Tabs[0].Width + g.Style.ItemInnerSpacing.x) * +0.5f, 0.0f));
            tab_bar = window_aaa->DockNode->TabBar;
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar, tab_order_initial));
            ctx->ItemDragWithDelta("BBB", ImVec2((tab_bar->Tabs[0].Width + g.Style.ItemInnerSpacing.x) * -0.5f, 0.0f));
            tab_bar = window_aaa->DockNode->TabBar;
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar, tab_order_initial));

            ImGuiID ccc_id = tab_bar->Tabs[2].ID;

            // Mix tabs, expected order becomes CCC, BBB, AAA. This also verifies drag operations way beyond tab bar rect.
            ctx->ItemDragWithDelta("AAA", ImVec2(+tab_bar->Tabs[2].Offset + tab_bar->Tabs[2].Width, 0.0f));
            ctx->ItemDragWithDelta("CCC", ImVec2(-tab_bar->Tabs[1].Offset - tab_bar->Tabs[1].Width, 0.0f));
            IM_CHECK(window_aaa->DockNode != NULL);     // Avoid crashes if tab gets undocked.
            tab_bar = window_aaa->DockNode->TabBar;
            const char* tab_order_rearranged[] = { "CCC", "BBB", "AAA", NULL };
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar, tab_order_rearranged));

            // Hide CCC and BBB and show them together on the same frame.
            vars.ShowWindow[1] = vars.ShowWindow[2] = false;
            ctx->Yield(2);
            vars.ShowWindow[1] = vars.ShowWindow[2] = true;
            ctx->Yield(2);

            // After reappearing windows that were hidden maintain same order relative to each other, and go to the end of tab bar together.
            // FIXME-TESTS: This check would fail due to a bug.
            const char* tab_order_reappeared[] = { "AAA", "CCC", "BBB", NULL };
            IM_UNUSED(tab_order_reappeared);
#if IMGUI_BROKEN_TESTS
            if (true)
#else
            if (step == 0)
#endif
            {
                tab_bar = window_aaa->DockNode->TabBar;
                IM_CHECK(ctx->TabBarCompareOrder(tab_bar, tab_order_reappeared));
            }

            // Focus CCC tab.
            ctx->ItemClick("CCC");

            // Hide all tabs and show them together on the same frame.
            vars.ShowWindow[1] = vars.ShowWindow[2] = false;
            ctx->Yield(2);
            vars.ShowWindow[1] = vars.ShowWindow[2] = true;
            ctx->Yield(2);

            // CCC should maintain focus.
            // FIXME-TESTS: This check would fail due to a missing feature (#2304)
            IM_UNUSED(ccc_id);
#if IMGUI_BROKEN_TESTS
            tab_bar = window_aaa->DockNode->TabBar;
            IM_CHECK_EQ(tab_bar->SelectedTabId, ccc_id);
            //IM_CHECK_EQ(g.NavWindow, ....);
#endif
        }
    };

    // ## Test dockspace padding. (#3733)
    t = IM_REGISTER_TEST(e, "docking", "docking_dockspace_over_viewport_padding");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Left", NULL, ImGuiWindowFlags_NoSavedSettings);ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Right", NULL, ImGuiWindowFlags_NoSavedSettings);ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Up", NULL, ImGuiWindowFlags_NoSavedSettings);ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Down", NULL, ImGuiWindowFlags_NoSavedSettings);ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGuiWindow* window = ctx->GetWindowByRef(Str30f("DockSpaceViewport_%08X", viewport->ID).c_str());
        ctx->DockMultiClear("Left", "Up", "Right", "Down", NULL);

        // Empty dockspace has no padding.
        IM_CHECK_EQ(window->HitTestHoleOffset.x, 0);
        IM_CHECK_EQ(window->HitTestHoleOffset.y, 0);
        IM_CHECK_EQ(window->HitTestHoleSize.x, viewport->Size.x);
        IM_CHECK_EQ(window->HitTestHoleSize.y, viewport->Size.y);

        ImGuiID dockspace_id = ctx->GetID(Str16f("%s/DockSpace", window->Name).c_str());
        ctx->DockWindowInto("Left", Str30f("%s\\/DockSpace_%08X", window->Name, dockspace_id).c_str(), ImGuiDir_Left);
        ctx->DockWindowInto("Up", Str30f("%s\\/DockSpace_%08X", window->Name, dockspace_id).c_str(), ImGuiDir_Up);
        ctx->DockWindowInto("Right", Str30f("%s\\/DockSpace_%08X", window->Name, dockspace_id).c_str(), ImGuiDir_Right);
        ctx->DockWindowInto("Down", Str30f("%s\\/DockSpace_%08X", window->Name, dockspace_id).c_str(), ImGuiDir_Down);
        ctx->Yield();

        // Dockspace with windows docked around it reduces central hole by their size + some padding.
        ImGuiWindow* left_window = ctx->GetWindowByRef("Left");
        ImGuiWindow* up_window = ctx->GetWindowByRef("Up");
        ImGuiWindow* right_window = ctx->GetWindowByRef("Right");
        ImGuiWindow* down_window = ctx->GetWindowByRef("Down");
        IM_CHECK_GT(window->HitTestHoleOffset.x, left_window->Pos.x + left_window->Size.x);
        IM_CHECK_GT(window->HitTestHoleOffset.y, up_window->Pos.y + up_window->Size.y);
        IM_CHECK_LT(window->HitTestHoleOffset.x + window->HitTestHoleSize.x, right_window->Pos.x);
        IM_CHECK_LT(window->HitTestHoleOffset.y + window->HitTestHoleSize.y, down_window->Pos.y);
    };

    // ## Test preserving docking information of closed windows. (#3716)
    t = IM_REGISTER_TEST(e, "docking", "docking_preserve_docking_info");
    t->SetUserDataType<DockingTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DockingTestingVars& vars = ctx->GetUserData<DockingTestingVars>();
        ImGui::SetNextWindowSize(ImVec2(500,500), ImGuiCond_Appearing);
        ImGui::Begin("Host", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Checkbox("1", &vars.ShowWindow[1]); ImGui::SameLine();
        ImGui::Checkbox("2", &vars.ShowWindow[2]); ImGui::SameLine();
        ImGui::Checkbox("3", &vars.ShowWindow[3]);
        ImGui::DockSpace(ImGui::GetID("Dockspace"));
        ImGui::End();

        if (vars.ShowWindow[1])
        {
            ImGui::SetNextWindowSize(ImVec2(300.0f, 200.0f), ImGuiCond_Appearing);
            ImGui::Begin("Window 1", &vars.ShowWindow[1], ImGuiWindowFlags_NoSavedSettings);
            ImGui::Text("This is window 1");
            ImGui::End();
        }
        if (vars.ShowWindow[2])
        {
            ImGui::SetNextWindowSize(ImVec2(300.0f, 200.0f), ImGuiCond_Appearing);
            ImGui::Begin("Window 2", &vars.ShowWindow[2], ImGuiWindowFlags_NoSavedSettings);
            ImGui::Text("This is window 2");
            ImGui::End();
        }
        if (vars.ShowWindow[3])
        {
            ImGui::SetNextWindowSize(ImVec2(300.0f, 200.0f), ImGuiCond_Appearing);
            ImGui::Begin("Window 3", &vars.ShowWindow[3], ImGuiWindowFlags_NoSavedSettings);
            ImGui::Text("This is window 3");
            ImGui::End();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        DockingTestingVars& vars = ctx->GetUserData<DockingTestingVars>();
        ImGuiWindow* window1 = ctx->GetWindowByRef("Window 1");
        ImGuiWindow* window2 = ctx->GetWindowByRef("Window 2");
        ctx->DockMultiClear("Window 1", "Window 2", "Window 3", NULL);
        ctx->DockWindowInto("Window 1", Str30f("Host\\/DockSpace_%08X", ctx->GetID("Host/Dockspace")).c_str(), ImGuiDir_Left);
        ctx->DockWindowInto("Window 2", "Window 1");
        vars.ShowWindow[1] = false;
        ctx->Yield(3);
        ctx->DockWindowInto("Window 3", "Window 2", ImGuiDir_Down);
        vars.ShowWindow[1] = true;
        ctx->Yield(3);
        IM_CHECK(window1->DockId != 0);
        IM_CHECK(window1->DockId == window2->DockId);
    };

    // ## Test docked window focusing from the menu.
    t = IM_REGISTER_TEST(e, "docking", "docking_focus_from_menu");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 100));
        ImGui::Begin("AAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(200, 100));
        ImGui::Begin("BBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(200, 100));
        ImGui::Begin("CCC", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiWindow* windows[] = { ctx->GetWindowByRef("AAA"), ctx->GetWindowByRef("BBB"), ctx->GetWindowByRef("CCC") };
        ctx->DockMultiClear("AAA", "BBB", "CCC", NULL);
        ctx->DockWindowInto("BBB", "AAA");
        ctx->DockWindowInto("CCC", "BBB");

        for (int i = 0; i < IM_ARRAYSIZE(windows); i++)
        {
            ctx->ItemClick(ImHashDecoratedPath("#COLLAPSE", NULL, windows[i]->RootWindowDockTree->DockNodeAsHost->ID));
            ctx->SetRef(g.NavWindow);
            ctx->ItemClick(windows[i]->Name);
            IM_CHECK(g.NavWindow == windows[i]);
        }
    };

    // ## Test docked window focusing.
    t = IM_REGISTER_TEST(e, "docking", "docking_focus_from_host");
    t->SetUserDataType<DockingTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 100));
        ImGui::Begin("AAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(200, 100));
        ImGui::Begin("BBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiWindow* windows[2] = { ctx->GetWindowByRef("AAA"), ctx->GetWindowByRef("BBB") };
        ctx->DockMultiClear("AAA", "BBB", NULL);
        ctx->DockWindowInto("BBB", "AAA", ImGuiDir_Right);

        // Test focusing window by clicking on it.
        for (int i = 0; i < IM_ARRAYSIZE(windows); i++)
        {
            ctx->MouseMoveToPos(windows[i]->Rect().GetCenter());
            ctx->MouseClick();
            IM_CHECK(g.NavWindow == windows[i]);
        }

        // Test focusing window by clicking its tab.
        for (int i = 0; i < IM_ARRAYSIZE(windows); i++)
        {
            ctx->ItemClick(windows[i]->Name);
            IM_CHECK(g.NavWindow == windows[i]);
        }

        // Test focusing window by clicking empty space in its tab bar.
        for (int i = 0; i < IM_ARRAYSIZE(windows); i++)
        {
            ImRect bar_rect = windows[i]->DockNode->TabBar->BarRect;
            ctx->MouseMoveToPos(bar_rect.GetCenter() + ImVec2(bar_rect.GetWidth() * 0.4f, 0.0f));
            ctx->MouseClick();
            //ctx->MouseDragWithDelta(ImVec2(3.0f * (i % 2 == 0 ? -1.0f : +1.0f), 0.0f));
            IM_CHECK(g.NavWindow == windows[i]);
        }

        // Check whether collapse button interaction focuses window.
        for (int i = 0; i < IM_ARRAYSIZE(windows); i++)
        {
            // FIXME-TESTS: Standardize access to window/collapse menu ID (there are 6 instances in tests)
            ctx->ItemClick(ImHashDecoratedPath("#COLLAPSE", NULL, windows[i]->DockNode->ID));
            ctx->KeyPressMap(ImGuiKey_Escape);
            IM_CHECK(g.NavWindow == windows[i]);
        }

        // Check whether focus is maintained after splitter interaction.
        ImGuiWindow* last_focused_window = g.NavWindow;

        ImGuiWindow* root_dock = last_focused_window->RootWindowDockTree;
        ImGuiID splitter_id = ctx->GetID("##Splitter", ctx->GetIDByInt(root_dock->DockNodeAsHost->ID, ctx->GetID(root_dock->Name)));
        ctx->MouseMove(splitter_id);
        ctx->MouseDragWithDelta(ImVec2(0.0f, 10.0f));
        // FIXME-TESTS: This would fail due to a bug (#3820)
#if IMGUI_BROKEN_TESTS
        IM_CHECK(g.NavWindow == last_focused_window);
#endif
        IM_UNUSED(last_focused_window);
    };

    // ## Test restoring dock tab state in independent and split windows.
    t = IM_REGISTER_TEST(e, "docking", "docking_tab_state");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        bool& hide_all = ctx->GenericVars.Bool1;
        if (hide_all)
            return;

        // FIXME-DOCKING
#if IMGUI_BROKEN_TESTS
        // Very interestingly, without the DockMultiClear() and with io.ConfigDockingAlwaysTabBar=true,
        // we get a crash when running a "docking_tab_state", "docking_focus_from_host", "docking_tab_state" sequence.
        // Begin("CCC") -> BeginDocked() binds CCC to an HostWindow which wasn't active at beginning of the frame, and it asserts in Begin()
        // Need to clarify into a simpler repro
#endif
        if (ctx->IsFirstGuiFrame())
            ctx->DockMultiClear("AAA", "BBB", "CCC", "DDD", "EEE", "FFF", NULL);

        ImVec2 size = ImVec2(300, 150);
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("AAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("BBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("CCC", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("DDD", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("EEE", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("FFF", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        bool& hide_all = ctx->GenericVars.Bool1;
        ImGuiWindow* window_aaa = ctx->GetWindowByRef("AAA");
        ImGuiWindow* window_ddd = ctx->GetWindowByRef("DDD");

        for (int variant = 0; variant < 2; variant++)
        {
            ctx->LogDebug("Variant %d", variant);
            ctx->DockMultiClear("AAA", "BBB", "CCC", "DDD", "EEE", "FFF", NULL);

            if (variant == 1)
                ctx->DockWindowInto("DDD", "AAA", ImGuiDir_Right);
            ctx->DockWindowInto("BBB", "AAA");
            ctx->DockWindowInto("CCC", "BBB");
            ctx->DockWindowInto("EEE", "DDD");
            ctx->DockWindowInto("FFF", "EEE");

            ImGuiTabBar* tab_bar_aaa = window_aaa->DockNode->TabBar;
            ImGuiTabBar* tab_bar_ddd = window_ddd->DockNode->TabBar;

            // Rearrange tabs and set active tab.
            ctx->ItemDragAndDrop("CCC", "AAA");
            ctx->ItemDragAndDrop("DDD", "FFF");
            ctx->ItemClick("AAA");
            ctx->ItemClick("FFF");

            // Verify initial order.
            const char* tab_order_1[] = { "CCC", "AAA", "BBB", NULL };
            const char* tab_order_2[] = { "EEE", "FFF", "DDD", NULL };
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar_aaa, tab_order_1));
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar_ddd, tab_order_2));

            IM_CHECK(tab_bar_aaa->Tabs.Size >= 2);
            IM_CHECK(tab_bar_aaa->VisibleTabId == tab_bar_aaa->Tabs[1].ID);
            IM_CHECK(tab_bar_ddd->VisibleTabId == tab_bar_ddd->Tabs[1].ID);

            // Hide and show all windows.
            hide_all = true;
            ctx->Yield(2);
            hide_all = false;
            ctx->Yield(2);

            // Verify tab order is preserved.
            tab_bar_aaa = window_aaa->DockNode->TabBar;
            tab_bar_ddd = window_ddd->DockNode->TabBar;
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar_aaa, tab_order_1));
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar_ddd, tab_order_2));

            // Verify active window is preserved.
            // FIXME-TESTS: This fails due to a bug.
#if IMGUI_BROKEN_TESTS
            IM_CHECK(tab_bar_aaa->VisibleTabId == tab_bar_aaa->Tabs[1].ID);
            IM_CHECK(tab_bar_ddd->VisibleTabId == tab_bar_ddd->Tabs[1].ID);
#endif
        }
    };

    // ## Test undocking window from dockspace covering entire window.
    t = IM_REGISTER_TEST(e, "docking", "docking_undock_large");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Appearing);
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("This is window 1");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window = ctx->GetWindowByRef("Window 1");
        IM_CHECK(window->DockNode != NULL);

        ImVec2 expect_size = ImGui::GetMainViewport()->Size;
        IM_CHECK(window->Size.x == expect_size.x);
        IM_CHECK(window->Size.y == expect_size.y);

        ctx->ItemDragWithDelta("Window 1", ImVec2(50.0f, 50.0f));
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window));

        expect_size = ImGui::GetMainViewport()->Size * ImVec2(0.90f, 0.90f);
        IM_CHECK(window->Size.x <= expect_size.x);
        IM_CHECK(window->Size.y <= expect_size.y);
    };

#else
    IM_UNUSED(e);
#endif
}
