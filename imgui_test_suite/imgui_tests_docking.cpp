// dear imgui
// (tests: docking)

// Test comment conventions:
//   AAA, BBB               two windows docked in same node (same tab bar)
//   AAA | BBB              two windows in a split dock node (vertical or horizontal).
//   AAA + BBB              two separate floating windows.
// [ AAA | BBB ]            two windows in a split dock node in a dockspace
//   AAA, BBB | CCC + DDD   four windows, AAA and BBB are docked together the left side, CCC window on the right side of the split. DDD is independent floating window.

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
// TODO: Tests: Docking: clicking on a tab sets NavWindow to the tab window
// TODO: Tests: Docking: clicking on a node title bar (outside of tab) sets NavWindow to the selected tab window
// TODO: Tests: Docking: dragging collapse menu sets NavWindow to selected tab window + allow to move
//-------------------------------------------------------------------------

// Helpers
#ifdef IMGUI_HAS_DOCK

struct DockingTestsGenericVars
{
    int         Step = 0;
    ImGuiID     DockSpaceID = 0;
    ImGuiID     NodesId[10];
    bool        ShowDockspace = true;
    bool        ShowWindow[10];
    bool        ShowWindowGroups[2];    // One per 5 windows
    int         AppearingCount[10];

    DockingTestsGenericVars()
    {
        for (int n = 0; n < IM_ARRAYSIZE(NodesId); n++)
            NodesId[n] = 0;
        for (int n = 0; n < IM_ARRAYSIZE(ShowWindow); n++)
            ShowWindow[n] = false;
        for (int n = 0; n < IM_ARRAYSIZE(ShowWindowGroups); n++)
            ShowWindowGroups[n] = true;
        for (int n = 0; n < IM_ARRAYSIZE(ShowWindow); n++)
            AppearingCount[n] = false;
    }

    void SetShowWindows(int count, bool show)
    {
        IM_ASSERT(count < IM_ARRAYSIZE(ShowWindow));
        for (int i = 0; i < count; i++)
            ShowWindow[i] = show;
    }
};

// Return "AAA", "BBB", "CCC", ..., "ZZZ"
static Str16 DockingTestsGetWindowName(int n)
{
    IM_ASSERT(n <= 26);
    Str16 name;
    name[0] = name[1] = name[2] = (char)('A' + n);
    name[3] = 0;
    return name;
}

static void DockingTestsGenericGuiFunc(ImGuiTestContext* ctx)
{
    DockingTestsGenericVars& vars = ctx->GetVars<DockingTestsGenericVars>();
    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Appearing);

    if (ctx->IsFirstGuiFrame())
    {
        ctx->DockClear("AAA", "BBB", "CCC", NULL);  // Fix SetNextWindowSize() on next test run. // FIXME-TESTS
    }

    if (ctx->IsGuiFuncOnly())
    {
        ImGui::Begin("Test Config");
        ImGui::Checkbox("Show Dockspace", &vars.ShowDockspace);
        for (int n = 0; n < IM_ARRAYSIZE(vars.ShowWindow); n++)
        {
            if ((n % 5) == 0)
            {
                // Toggling group UI to toggle visibility of multiple windows simultaneously
                int group_n = n / 5;
                Str16f group_name("Group %d", group_n);
                ImGui::Checkbox(group_name.c_str(), &vars.ShowWindowGroups[group_n]);
            }
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
            ImGui::Checkbox(DockingTestsGetWindowName(n).c_str(), &vars.ShowWindow[n]);
        }
        ImGui::End();
    }

    ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
    if (vars.ShowDockspace)
    {
        vars.DockSpaceID = ImGui::GetID("dockspace");
        ImGui::DockSpace(vars.DockSpaceID);
    }
    ImGui::End();

    for (int n = 0; n < IM_ARRAYSIZE(vars.ShowWindow); n++)
        if (vars.ShowWindow[n] && vars.ShowWindowGroups[n / 5])
        {
            ImGui::SetNextWindowSize(ImVec2(300.0f, 200.0f), ImGuiCond_Appearing);

            Str16 window_name = DockingTestsGetWindowName(n);
            ImGui::Begin(window_name.c_str(), NULL, ImGuiWindowFlags_NoSavedSettings);
            vars.AppearingCount[n] += ImGui::IsWindowAppearing();

            ImGui::Text("This is '%s'", window_name.c_str());
            ImGui::Text("ID = %08X, DockID = %08X", ImGui::GetCurrentWindow()->ID, ImGui::GetWindowDockID());
            ImGui::Text("AppearingCount = %d", vars.AppearingCount[n]);
            ImGui::End();
        }
}

#if IMGUI_VERSION_NUM < 19002
static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }    // for IM_CHECK_EQ()
#endif
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
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, auto& vars) { vars.SetShowWindows(2, true); });
    t->GuiFunc = DockingTestsGenericGuiFunc;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiIO& io = ImGui::GetIO();
        for (int test_case_n = 0; test_case_n < 2; test_case_n++)
        {
            // FIXME-TESTS: Tests doesn't work if already docked
            // FIXME-TESTS: DockSetMulti takes window_name not ref
            io.ConfigDockingAlwaysTabBar = (test_case_n == 1);
            ctx->LogDebug("## TEST CASE %d: with ConfigDockingAlwaysTabBar = %d", test_case_n, io.ConfigDockingAlwaysTabBar);

            ImGuiWindow* window_aaa = ctx->GetWindowByRef("AAA");
            ImGuiWindow* window_bbb = ctx->GetWindowByRef("BBB");
            ImVec2 viewport_pos = ImGui::GetMainViewport()->Pos;

            // Init state
            ctx->DockClear("AAA", "BBB", NULL);
            if (ctx->UiContext->IO.ConfigDockingAlwaysTabBar)
                IM_CHECK(window_aaa->DockId != 0 && window_bbb->DockId != 0 && window_aaa->DockId != window_bbb->DockId);
            else
                IM_CHECK(window_aaa->DockId == 0 && window_bbb->DockId == 0);
            ctx->WindowResize("//AAA", ImVec2(200, 200));
            ctx->WindowMove("//AAA", viewport_pos + ImVec2(100, 100));
            ctx->WindowResize("//BBB", ImVec2(200, 200));
            ctx->WindowMove("//BBB", viewport_pos + ImVec2(200, 200));

            // Dock Once
            ctx->DockInto("AAA", "BBB");
            IM_CHECK(window_aaa->DockNode != NULL);
            IM_CHECK(window_aaa->DockNode == window_bbb->DockNode);
            IM_CHECK_EQ(window_aaa->DockNode->Pos, viewport_pos + ImVec2(200, 200));
            IM_CHECK_EQ(window_aaa->Pos, viewport_pos + ImVec2(200, 200));
            IM_CHECK_EQ(window_bbb->Pos, viewport_pos + ImVec2(200, 200));
            ImGuiID dock_id = window_bbb->DockId;
            ctx->Sleep(0.5f);

            {
                // Undock AAA, BBB should still refer/dock to node.
                ctx->DockClear("AAA", NULL);
                IM_CHECK(ctx->WindowIsUndockedOrStandalone(window_aaa));
                IM_CHECK(window_bbb->DockId == dock_id);

                // Intentionally move both floating windows away
                ctx->WindowMove("//AAA", viewport_pos + ImVec2(100, 100));
                ctx->WindowResize("//AAA", ImVec2(100, 100));
                ctx->WindowMove("//BBB", viewport_pos + ImVec2(300, 300));
                ctx->WindowResize("//BBB", ImVec2(200, 200)); // Should already the case

                // Dock again (BBB still refers to dock id, making this different from the first docking)
                ctx->DockInto("//AAA", "//BBB", ImGuiDir_None);
                IM_CHECK_EQ(window_aaa->DockId, dock_id);
                IM_CHECK_EQ(window_bbb->DockId, dock_id);
                IM_CHECK_EQ(window_aaa->Pos, viewport_pos + ImVec2(300, 300));
                IM_CHECK_EQ(window_bbb->Pos, viewport_pos + ImVec2(300, 300));
                IM_CHECK_EQ(window_aaa->Size, ImVec2(200, 200));
                IM_CHECK_EQ(window_bbb->Size, ImVec2(200, 200));
                IM_CHECK_EQ(window_aaa->DockNode->Pos, viewport_pos + ImVec2(300, 300));
                IM_CHECK_EQ(window_aaa->DockNode->Size, ImVec2(200, 200));
            }

            {
                // Undock AAA, BBB should still refer/dock to node.
                ctx->DockClear("AAA", NULL);
                IM_CHECK(ctx->WindowIsUndockedOrStandalone(window_aaa));
                IM_CHECK_EQ(window_bbb->DockId, dock_id);

                // Intentionally move both floating windows away
                ctx->WindowMove("//AAA", viewport_pos + ImVec2(100, 100));
                ctx->WindowMove("//BBB", viewport_pos + ImVec2(200, 200));

                // Dock on the side (BBBB still refers to dock id, making this different from the first docking)
                ctx->DockInto("//AAA", "//BBB", ImGuiDir_Left);
                IM_CHECK(window_aaa->DockNode != NULL);
                IM_CHECK_EQ(window_aaa->DockNode->ParentNode->ID, dock_id);
                IM_CHECK_EQ(window_bbb->DockNode->ParentNode->ID, dock_id);
                IM_CHECK_EQ(window_aaa->DockNode->ParentNode->Pos, viewport_pos + ImVec2(200, 200));
                IM_CHECK_EQ(window_aaa->Pos, viewport_pos + ImVec2(200, 200));
                IM_CHECK_GT(window_bbb->Pos.x, window_aaa->Pos.x);
                IM_CHECK_EQ(window_bbb->Pos.y, viewport_pos.y + 200);
            }
        }
    };

    // ## Test basic use of DockBuilder api
    t = IM_REGISTER_TEST(e, "docking", "docking_api_builder_1");
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
        ctx->DockClear("AAAA", "BBBB", "CCCC", NULL);
        while (ctx->FrameCount < 10)
            ctx->Yield();

        vars.DockId = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_None);
        ImGui::DockBuilderSetNodePos(vars.DockId, ImGui::GetMainViewport()->Pos + ImVec2(100, 100));
        ImGui::DockBuilderSetNodeSize(vars.DockId, ImVec2(200, 200));
        ImGui::DockBuilderDockWindow("AAAA", vars.DockId);
        ImGui::DockBuilderDockWindow("BBBB", vars.DockId);
        ImGui::DockBuilderDockWindow("CCCC", vars.DockId);
        while (ctx->FrameCount < 20)
            ctx->Yield();
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
        ctx->LogDebug("ConfigDockingAlwaysTabBar = false");
        ctx->UiContext->IO.ConfigDockingAlwaysTabBar = false;
        ctx->Yield(4);
        ctx->LogDebug("ConfigDockingAlwaysTabBar = true");
        ctx->UiContext->IO.ConfigDockingAlwaysTabBar = true;
        ctx->Yield(4);
        ctx->LogDebug("ConfigDockingAlwaysTabBar = false");
        ctx->UiContext->IO.ConfigDockingAlwaysTabBar = false;
        ctx->Yield(4);
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
        {
            vars.DockId = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_None);
            ImGui::DockBuilderSetNodePos(vars.DockId, ImGui::GetMainViewport()->Pos + ImVec2(100, 100));
            ImGui::DockBuilderSetNodeSize(vars.DockId, ImVec2(200, 200));
            ImGui::DockBuilderDockWindow("AAAA", vars.DockId);
            ImGui::DockBuilderDockWindow("BBBB", vars.DockId);
            ImGui::DockBuilderDockWindow("CCCC", vars.DockId);
        }

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
        ImVec2 viewport_pos = ImGui::GetMainViewport()->Pos;
        ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
        ImGuiWindow* window1 = ctx->GetWindowByRef("Window 1");
        ImGuiWindow* window2 = ctx->GetWindowByRef("Window 2");
        ImGuiWindow* window3 = ctx->GetWindowByRef("Window 3");
        ImGuiWindow* window4 = ctx->GetWindowByRef("Window 4");
        ImGuiWindow* demo_window = ctx->GetWindowByRef("Dear ImGui Demo");

        ctx->DockClear("Window 1", "Window 2", "Dear ImGui Demo", NULL);
        ctx->WindowMove("Dear ImGui Demo", viewport_pos + viewport_size * 0.5f, ImVec2(0.5f, 0.5f));
        ctx->WindowMove("Window 1", viewport_pos + viewport_size * 0.5f, ImVec2(0.5f, 0.5f));
        ctx->WindowMove("Window 2", viewport_pos + viewport_size * 0.5f, ImVec2(0.5f, 0.5f));
        //IM_SUSPEND_TESTFUNC();

        // Test undocking from tab.
        ctx->DockInto("Window 1", "Window 2");
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
                ctx->DockClear("Window 1", "Window 2", "Dear ImGui Demo", NULL);
                ctx->Yield();
                ctx->DockInto("Window 1", "Dear ImGui Demo", (ImGuiDir)direction);
                if (n)
                    ctx->DockInto("Window 2", "Window 1");
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

        const float h = window1->TitleBarHeight;
        ImVec2 pos;

        for (int n = 0; n < 3; n++)
        {
            ctx->DockClear("Window 1", "Window 2", "Window 3", "Window 4", NULL);
            ctx->DockInto("Window 2", "Window 1");
            ctx->DockInto("Window 3", "Window 2", ImGuiDir_Right);
            ctx->DockInto("Window 4", "Window 3");

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
                ctx->ItemDragWithDelta(ImGui::DockNodeGetWindowMenuButtonId(window4->DockNode), ImVec2(h, h) * -2);
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
                ctx->WindowTeleportToMakePosVisible(window4->DockNode->HostWindow->ID, pos + ImVec2(10, 10));     // Make space for next drag operation
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

    // ## Test hide/unhiding tab bar
    // "On a 2-way split, test using Hide Tab Bar on both node, then unhiding the tab bar."
    t = IM_REGISTER_TEST(e, "docking", "docking_hide_tabbar");
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, auto& vars) { vars.SetShowWindows(2, true); });
    t->GuiFunc = DockingTestsGenericGuiFunc;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* windows[] = { ctx->GetWindowByRef("AAA"), ctx->GetWindowByRef("BBB") };

        for (int i = 0; i < IM_ARRAYSIZE(windows); i++)
        {
            ImGuiWindow* window = windows[i];
            ctx->DockClear("BBB", "AAA", NULL);
            ctx->DockInto("BBB", "AAA", ImGuiDir_Right);

            // Two way split. Ensure window is docked and tab bar is visible.
            IM_CHECK(window->DockIsActive);
            IM_CHECK(!window->DockNode->IsHiddenTabBar());

            // Hide tab bar.
            ctx->DockNodeHideTabBar(window->DockNode, true);

            // Unhide tab bar.
            ctx->DockNodeHideTabBar(window->DockNode, false);
        }
    };

    auto test_docking_over_gui = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 200));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        switch (ctx->Test->ArgVariant)
        {
        case 0:
            ImGui::BeginChild("Child", ImVec2(300, 200 - ImGui::GetCurrentWindow()->TitleBarHeight));
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

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos + ImVec2(100, 100), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Dock Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
    };

    auto test_docking_over_test = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* dock_window = ctx->GetWindowByRef("Dock Window");
        ImGuiWindow* test_window = ctx->GetWindowByRef("Test Window");

        ctx->DockClear("Dock Window", "Test Window", NULL);
        switch (ctx->Test->ArgVariant)
        {
        case 0:
            ctx->DockInto("Dock Window", "Test Window");
            IM_CHECK(dock_window->DockNode != NULL);
            IM_CHECK(test_window->DockNode != NULL);
            IM_CHECK(test_window->DockNode->HostWindow == dock_window->DockNode->HostWindow);
            break;
        case 1:
        {
            ImGuiID dock_id = ctx->GenericVars.DockId;
            ImGuiDockNode* dock_node = ImGui::DockBuilderGetNode(dock_id);
            ctx->DockInto("Dock Window", dock_node->ID);
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
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, auto& vars) { vars.SetShowWindows(3, true); });
    t->GuiFunc = DockingTestsGenericGuiFunc;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        DockingTestsGenericVars& vars = ctx->GetVars<DockingTestsGenericVars>();

        for (int step = 0; step < 3; step++)
        {
            vars.Step = step;
            vars.ShowDockspace = (vars.Step == 1);

            ctx->DockClear("Test Window", "AAA", "BBB", "CCC", NULL);
            ctx->WindowResize("Test Window", ImVec2(300, 200)); // FIXME-TESTS: Aiming at left-most tab fails with very small node because CollapseButton() hit test is too large.
            ctx->WindowResize("AAA", ImVec2(300, 200));
            ctx->LogInfo("STEP %d", step);

            // All into a dockspace
            if (step == 1)
                ctx->DockInto("AAA", vars.DockSpaceID);

            // All into a split node of Test Window
            if (step == 2)
                ctx->DockInto("AAA", "Test Window", ImGuiDir_Right);

            ctx->DockInto("BBB", "AAA");
            ctx->DockInto("CCC", "AAA");

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
            ctx->ItemDragWithDelta("AAA/#TAB", ImVec2((tab_bar->Tabs[0].Width + g.Style.ItemInnerSpacing.x) * +0.5f, 0.0f));
            tab_bar = window_aaa->DockNode->TabBar;
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar, tab_order_initial));
            ctx->ItemDragWithDelta("BBB/#TAB", ImVec2((tab_bar->Tabs[0].Width + g.Style.ItemInnerSpacing.x) * -0.5f, 0.0f));
            tab_bar = window_aaa->DockNode->TabBar;
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar, tab_order_initial));
            IM_CHECK(tab_bar->Tabs[2].ID == window_ccc->TabId);

            // Mix tabs, expected order becomes CCC, BBB, AAA. This also verifies drag operations way beyond tab bar rect.
            ctx->ItemDragWithDelta("AAA/#TAB", ImVec2(+tab_bar->Tabs[2].Offset + tab_bar->Tabs[2].Width, 0.0f));
            ctx->ItemDragWithDelta("CCC/#TAB", ImVec2(-tab_bar->Tabs[1].Offset - tab_bar->Tabs[1].Width, 0.0f));
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
            tab_bar = window_aaa->DockNode->TabBar;
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar, tab_order_reappeared));
        }
    };

    // ## Test tab order related to edge case with 1 window node which don't have a tab bar
    t = IM_REGISTER_TEST(e, "docking", "docking_tab_order_hidden_tabbar");
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, auto& vars)
    {
        vars.ShowDockspace = false;
        vars.SetShowWindows(4, true);
    });
    t->GuiFunc = DockingTestsGenericGuiFunc;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        DockingTestsGenericVars& vars = ctx->GetVars<DockingTestsGenericVars>();
        ctx->DockClear("AAA", "BBB", "CCC", "DDD", NULL);
        ctx->DockInto("BBB", "AAA");
        ctx->DockInto("DDD", "CCC");
        vars.ShowWindow[3] = false;
        ctx->Yield();

        ImGuiWindow* window_aaa = ctx->GetWindowByRef("AAA");
        ImGuiWindow* window_bbb = ctx->GetWindowByRef("BBB"); IM_UNUSED(window_bbb);
        ImGuiWindow* window_ccc = ctx->GetWindowByRef("CCC"); IM_UNUSED(window_ccc);
        IM_CHECK(window_ccc->DockId != 0);
        IM_CHECK(window_ccc->DockNode != NULL);
        IM_CHECK(window_ccc->DockId != window_aaa->DockId);
        ctx->DockInto(window_aaa->DockNode->ID, window_ccc->ID);
        IM_CHECK(window_aaa->DockNode == window_ccc->DockNode);

        const char* expected_tab_order[] = { "CCC", "AAA", "BBB", NULL };
        IM_UNUSED(expected_tab_order);
        ImGuiTabBar* tab_bar = window_ccc->DockNode->TabBar;
        IM_CHECK(ctx->TabBarCompareOrder(tab_bar, expected_tab_order));
    };

    // ## Test preserving of tab order when entire dock node is moved.
    t = IM_REGISTER_TEST(e, "docking", "docking_tab_order_preserve");
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, auto& vars)
    {
        vars.ShowDockspace = false;
        vars.SetShowWindows(5, true);
    });
    t->GuiFunc = DockingTestsGenericGuiFunc;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window_bbb = ctx->GetWindowByRef("BBB");
        ImGuiWindow* window_ccc = ctx->GetWindowByRef("CCC");
        ctx->DockClear("AAA", "BBB", "CCC", "DDD", "EEE", NULL);
        ctx->WindowMove("EEE", window_bbb->Rect().GetTR() + ImVec2(10.0f, 0.0f));
        ctx->DockInto("AAA", "BBB");
        ctx->DockInto("CCC", "EEE", ImGuiDir_Right);
        ctx->DockInto("DDD", "CCC");

        ImGuiTabBar* tab_bar = window_ccc->DockNode->TabBar;
        ctx->ItemDragWithDelta("CCC/#TAB", ImVec2(tab_bar->Tabs[1].Offset + tab_bar->Tabs[1].Width, 0.0f));
        IM_CHECK(window_ccc->DockNode != NULL);
        ctx->DockInto(window_ccc->DockNode->ID, "AAA");
        const char* expected_tab_order[] = { "BBB", "AAA", "DDD", "CCC", NULL };
        IM_CHECK(ctx->TabBarCompareOrder(window_bbb->DockNode->TabBar, expected_tab_order));
    };

    // Test focus order restore (#2304)
    t = IM_REGISTER_TEST(e, "docking", "docking_tab_focus_restore");
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, auto& vars) {
        vars.SetShowWindows(5, true);
    });
    t->GuiFunc = DockingTestsGenericGuiFunc;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        DockingTestsGenericVars& vars = ctx->GetVars<DockingTestsGenericVars>();
        ctx->MenuUncheck("//Dear ImGui Demo/Examples/Dockspace"); // Conflict with ShowDockspace=true

        for (int step = 0; step < 3; step++)
        {
            vars.Step = step;
            vars.ShowDockspace = (vars.Step == 1);

            ctx->DockClear("Test Window", "AAA", "BBB", "CCC", "DDD", "EEE", NULL);
            ctx->WindowResize("Test Window", ImVec2(300, 200));
            ctx->WindowResize("AAA", ImVec2(300, 200));
            ctx->LogInfo("STEP %d", step);

            // All into a dockspace
            if (step == 1)
                ctx->DockInto("AAA", vars.DockSpaceID);

            // All into a split node of Test Window
            if (step == 2)
                ctx->DockInto("AAA", "Test Window", ImGuiDir_Right);

            ctx->DockInto("BBB", "AAA");
            ctx->DockInto("CCC", "AAA");

            // Dock DDD/EEE separately because Focus restore issues are different when node carry final focused node or not.
            ctx->DockInto("DDD", "AAA", ImGuiDir_Down);
            ctx->DockInto("EEE", "DDD");

            ImGuiWindow* window_aaa = ctx->GetWindowByRef("AAA");
            ImGuiWindow* window_bbb = ctx->GetWindowByRef("BBB");
            ImGuiWindow* window_ccc = ctx->GetWindowByRef("CCC");
            ImGuiWindow* window_ddd = ctx->GetWindowByRef("DDD");
            ImGuiWindow* window_eee = ctx->GetWindowByRef("EEE");
            ImGuiDockNode* node1 = window_aaa->DockNode; IM_UNUSED(node1);
            ImGuiDockNode* node2 = window_ddd->DockNode; IM_UNUSED(node2);
            IM_CHECK(window_aaa->DockNode != NULL);
            IM_CHECK(window_ddd->DockNode != NULL);
            IM_CHECK(node1 != node2);

            ctx->ItemClick("DDD");
            IM_CHECK_EQ(node2->SelectedTabId, window_ddd->TabId);

            ctx->ItemClick("BBB");
            ImGuiTabBar* tabbar_1 = node1->TabBar;
            IM_CHECK_EQ(node1->SelectedTabId, window_bbb->TabId);
            IM_CHECK_EQ(tabbar_1->SelectedTabId, window_bbb->TabId);
            IM_CHECK(g.NavWindow == window_bbb);

            IM_CHECK(window_aaa->Hidden == true);   // Node 1
            IM_CHECK(window_bbb->Hidden == false);
            IM_CHECK(window_ccc->Hidden == true);
            IM_CHECK(window_ddd->Hidden == false);  // Node 2
            IM_CHECK(window_eee->Hidden == true);
            const float w = tabbar_1->WidthAllTabs;

            // Hide all tabs and show them together on the same frame.
            vars.SetShowWindows(5, false);
            ctx->Yield();
            IM_CHECK(window_aaa->Active == false);
            IM_CHECK(window_bbb->Active == false);
            IM_CHECK(window_ccc->Active == false);
            IM_CHECK(window_ddd->Active == false);
            IM_CHECK(window_eee->Active == false);
            ctx->Yield();

            vars.ShowWindow[3] = true; // Show "DDD" and "EEE" first
            vars.ShowWindow[4] = true;
            ctx->Yield();
            IM_CHECK(window_ddd->Active == true);
            IM_CHECK(window_eee->Active == true);
            IM_CHECK(window_ddd->Hidden == true);
            IM_CHECK(window_eee->Hidden == true);
            vars.SetShowWindows(5, true);   // Then "AAA" "BBB" "CCC"
            ctx->Yield();
            IM_CHECK(window_aaa->Active == true);
            IM_CHECK(window_bbb->Active == true);
            IM_CHECK(window_ccc->Active == true);
            IM_CHECK(window_ddd->Active == true);
            IM_CHECK(window_eee->Active == true);
            IM_CHECK(window_aaa->Hidden == true);
            IM_CHECK(window_bbb->Hidden == true);
            IM_CHECK(window_ccc->Hidden == true);

            // BBB should have focus.
            ctx->Yield();
            tabbar_1 = window_aaa->DockNode->TabBar;
            IM_CHECK_EQ(tabbar_1->WidthAllTabs, w);
            ImGuiTabBar* tabbar_2 = window_ddd->DockNode->TabBar;

#if IMGUI_VERSION_NUM >= 18992
            // Test focus restore (#2304)... for node with no final focus
            IM_CHECK(window_ddd->Hidden == false);
            IM_CHECK(window_eee->Hidden == true);
            IM_CHECK_EQ(node2->SelectedTabId, window_ddd->TabId);
            IM_CHECK_EQ(tabbar_2->SelectedTabId, window_ddd->TabId);

            // Test focus restore (#2304)... for node that has final focus
            // FIXME-TESTS: This check would fail due to a missing feature (#2304)
            IM_CHECK_EQ(node1->SelectedTabId, window_bbb->TabId);
            IM_CHECK_EQ(tabbar_1->SelectedTabId, window_bbb->TabId);
            IM_CHECK(window_aaa->Hidden == true);
            IM_CHECK(window_bbb->Hidden == false);
            IM_CHECK(window_ccc->Hidden == true);
#if IMGUI_BROKEN_TESTS
            IM_CHECK(g.NavWindow == window_bbb);
            ctx->Yield();
            IM_CHECK_EQ(node1->SelectedTabId, window_bbb->TabId);
            IM_CHECK_EQ(tabbar_1->SelectedTabId, window_bbb->TabId);
            IM_CHECK(window_aaa->Hidden == true);
            IM_CHECK(window_bbb->Hidden == false);
            IM_CHECK(window_ccc->Hidden == true);
#endif
#endif

            // FIXME-TESTS: Now close CCC and verify the focused windows (should be "AAA" or "BBB" depending on which we last focused before "CCC": should test both cases!)
        }
    };

    // ## Test appending to dock tab bar.
    t = IM_REGISTER_TEST(e, "docking", "docking_tab_amend");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        int& button_clicks = ctx->GenericVars.Int1;
        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Appearing);
        ImGui::Begin("AAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("Not empty.");
        ImGuiWindow* window = g.CurrentWindow;
        if (window->DockNode != NULL)
        {
            if (ImGui::DockNodeBeginAmendTabBar(window->DockNode))
            {
                if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
                    button_clicks++;
                ImGui::DockNodeEndAmendTabBar();
            }
        }
        ImGui::End();

        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Appearing);
        ImGui::Begin("BBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("Not empty.");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        int& button_clicks = ctx->GenericVars.Int1;
        ImGuiWindow* window = ctx->GetWindowByRef("AAA");
        ctx->DockClear("AAA", "BBB", NULL);
        ctx->DockInto("BBB", "AAA");
        IM_CHECK_EQ(button_clicks, 0);
        IM_CHECK(window->DockId != 0);
        ctx->ItemClick(ctx->GetID("+", window->DockId));
        IM_CHECK_EQ(button_clicks, 1);
    };

    // ## Test IsItemHovered() on tabs and clipped contents
    t = IM_REGISTER_TEST(e, "docking", "docking_tab_clipped_is_hovered");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ImGui::Begin("AAA", NULL, ImGuiWindowFlags_NoSavedSettings);
        vars.BoolArray[0] = ImGui::IsItemHovered();
        ImGui::Text("IsItemHovered: %d", vars.BoolArray[0]);
        ImGui::Button("Button");
        vars.BoolArray[1] = ImGui::IsItemHovered();
        ImGui::Text("IsItemHovered: %d", vars.BoolArray[1]);
        ImGui::End();

        ImGui::Begin("BBB", NULL, ImGuiWindowFlags_NoSavedSettings);
        vars.BoolArray[2] = ImGui::IsItemHovered();
        ImGui::Text("IsItemHovered: %d", vars.BoolArray[2]);
        ImGui::Button("Button");
        vars.BoolArray[3] = ImGui::IsItemHovered();
        ImGui::Text("IsItemHovered: %d", vars.BoolArray[3]);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ctx->DockClear("AAA", "BBB", NULL);
        ctx->DockInto("BBB", "AAA");

        ctx->ItemClick("AAA/#TAB");
        IM_CHECK(vars.BoolArray[0] == true);
        IM_CHECK(vars.BoolArray[1] == false); // Button not reporting as hovered
        IM_CHECK(vars.BoolArray[2] == false);
        IM_CHECK(vars.BoolArray[3] == false); // Button not reporting as hovered
        ctx->MouseDown(0);
        ctx->Yield();
        ctx->Yield();
        IM_CHECK(vars.BoolArray[0] == true);
        IM_CHECK(vars.BoolArray[1] == false); // Button not reporting as hovered
        IM_CHECK(vars.BoolArray[2] == false);
        IM_CHECK(vars.BoolArray[3] == false); // Button not reporting as hovered
        ctx->MouseUp(0);

        ctx->ItemClick("BBB/#TAB");
        IM_CHECK(vars.BoolArray[0] == false);
        IM_CHECK(vars.BoolArray[1] == false); // Button not reporting as hovered
        IM_CHECK(vars.BoolArray[2] == true);
        IM_CHECK(vars.BoolArray[3] == false); // Button not reporting as hovered
    };

    // ## Test basic Item function on DockSpace() which is based but not using BeginChild()/EndChild() (#6217)
#if IMGUI_VERSION_NUM >= 18943
    t = IM_REGISTER_TEST(e, "docking", "docking_dockspace_item_query");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGuiID id = ImGui::GetID("foo");
        ImGui::DockSpace(id, ImVec2(200, 300));
        ImVec2 sz = ImGui::GetItemRectSize();
        ImGuiID id2 = ImGui::GetItemID();
        IM_CHECK(sz.x == 200.0f && sz.y == 300.0f);
        IM_CHECK(id == id2);
        ImGui::End();
    };
#endif

    // ## Test _KeepAlive dockspace flag.
    t = IM_REGISTER_TEST(e, "docking", "docking_dockspace_keep_alive");
    struct DockspaceKeepAliveVars { ImGuiDockNodeFlags Flags = 0; bool ShowDockspace = true; bool ShowMainMenuBar = true; };
    t->SetVarsDataType<DockspaceKeepAliveVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DockspaceKeepAliveVars& vars = ctx->GetVars<DockspaceKeepAliveVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);

        if (vars.ShowMainMenuBar)
        {
            ImGui::BeginMainMenuBar();
            ImGui::EndMainMenuBar();
        }

        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Appearing);
        ImGui::Begin("Window A", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (vars.ShowDockspace)
            ImGui::DockSpace(ImGui::GetID("A2"), ImVec2(0, 0), vars.Flags);
        ImGui::End();

        ImGui::SetNextWindowSize(ImVec2(150, 100), ImGuiCond_Always);
        ImGui::Begin("Window B", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        DockspaceKeepAliveVars& vars = ctx->GetVars<DockspaceKeepAliveVars>();
        ImGuiWindow* window1 = ctx->GetWindowByRef("Window A");
        ImGuiWindow* window2 = ctx->GetWindowByRef("Window B");
        ImGuiID dock_id = ctx->GetID("Window A/A2");

        if (ImGui::GetIO().ConfigDockingAlwaysTabBar)
        {
            ctx->LogWarning("Cannot run test with io.ConfigDockingAlwaysTabBar == true");
            return;
        }

        // "Window A" has dockspace "A2". Dock "window B" into "A2".
        ctx->OpFlags |= ImGuiTestOpFlags_NoAutoUncollapse;
        ctx->DockClear("Window B", "Window A", NULL);
        ctx->WindowCollapse("Window A", false);
        ctx->WindowCollapse("Window B", false);
        ctx->DockInto("Window B", dock_id);
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window1));           // "Window A" is not docked
        IM_CHECK_EQ(window2->DockId, dock_id);                          // "Window B" was docked into a dockspace

        // "Window A" code calls DockSpace with _KeepAlive. Verify that "Window B" is still docked into "A2" (and verify that both are HIDDEN at this point).
        vars.Flags = ImGuiDockNodeFlags_KeepAliveOnly;
        ctx->Yield();
        IM_CHECK_EQ(window1->DockIsActive, false);                      // "Window A" remains undocked
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window1));           //
        IM_CHECK_EQ(window2->Collapsed, false);                         // "Window B" was not collapsed
        IM_CHECK_EQ(window2->DockIsActive, true);                       // Dockspace is being kept alive
        IM_CHECK_EQ(window2->DockId, dock_id);                          // window remains docked
        IM_CHECK_NE(window2->DockNode, (ImGuiDockNode*)NULL);
        IM_CHECK_EQ(window2->Hidden, true);                             // but invisible

        // "Window A" is collapsed, and regular call to DockSpace() without KeepAlive (KeepAlive becomes automatic).
        vars.Flags = 0;
        ctx->WindowCollapse("Window A", true);
        IM_CHECK_EQ(window1->Collapsed, true);                          // "Window A" got collapsed
        IM_CHECK_EQ(window1->DockIsActive, false);                      // and remains undocked
        IM_CHECK_EQ(window2->Collapsed, false);                         // "Window B" was not collapsed
        IM_CHECK_EQ(window2->DockIsActive, true);                       // Dockspace is being kept alive
        IM_CHECK_EQ(window2->DockId, dock_id);                          // window remains docked
        IM_CHECK_NE(window2->DockNode, (ImGuiDockNode*)NULL);
        IM_CHECK_EQ(window2->Hidden, true);                             // but invisible

        // A window submitted before "Window A" is hidden (#4757).
        vars.ShowMainMenuBar = false;
        ctx->Yield();
        IM_CHECK_EQ(window1->Collapsed, true);                          // "Window A" is still collapsed
        IM_CHECK_EQ(window1->DockIsActive, false);                      // and remains undocked
        IM_CHECK_EQ(window2->Collapsed, false);                         // "Window B" was not collapsed
        IM_CHECK_EQ(window2->DockIsActive, true);                       // Dockspace is being kept alive
        IM_CHECK_EQ(window2->DockId, dock_id);                          // window remains docked
        IM_CHECK_NE(window2->DockNode, (ImGuiDockNode*)NULL);
        IM_CHECK_EQ(window2->Hidden, true);                             // but invisible

        // "Window A" stops submitting the DockSpace() "A2". verify that "window B" is now undocked.
        vars.ShowDockspace = false;
        ctx->Yield();
        IM_CHECK_EQ(window1->Collapsed, true);                          // "Window A" got collapsed
        IM_CHECK_EQ(window1->DockIsActive, false);                      // and remains undocked
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window1));           //
        IM_CHECK_EQ(window2->Collapsed, false);                         // "Window B" was not collapsed
        IM_CHECK_EQ(window2->DockIsActive, false);                      // Dockspace is no longer kept alive
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window2));           // and window gets undocked
        IM_CHECK_EQ(window2->Hidden, false);                            // "Window B" shows up
    };

    // ## Test passthrough docking node.
    t = IM_REGISTER_TEST(e, "docking", "docking_dockspace_passthru_hover");
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
        ctx->MouseSetViewportID(0); // Auto
        ctx->WindowFocus("Window 1");
        ctx->MouseMoveToPos(window1->Rect().GetCenter());
        IM_CHECK_EQ(g.NavWindow, window1);
        IM_CHECK_EQ(g.HoveredWindow, window2);
        IM_CHECK_EQ(g.DebugHoveredDockNode, (ImGuiDockNode*)NULL);
        ctx->MouseClick();
        IM_CHECK_EQ(g.NavWindow, window2);
    };

    // ## Test dockspace padding. (#3733)
    t = IM_REGISTER_TEST(e, "docking", "docking_dockspace_passthru_padding");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        vars.DockId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Left", NULL, ImGuiWindowFlags_NoSavedSettings); ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Right", NULL, ImGuiWindowFlags_NoSavedSettings); ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Up", NULL, ImGuiWindowFlags_NoSavedSettings); ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Down", NULL, ImGuiWindowFlags_NoSavedSettings); ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGuiWindow* window = ctx->GetWindowByRef(Str30f("WindowOverViewport_%08X", viewport->ID).c_str());
        ctx->DockClear("Left", "Up", "Right", "Down", NULL);

        // Empty dockspace has no padding.
        IM_CHECK(window != NULL);
        IM_CHECK_EQ(window->HitTestHoleOffset.x, 0);
        IM_CHECK_EQ(window->HitTestHoleOffset.y, 0);
        IM_CHECK_EQ(window->HitTestHoleSize.x, viewport->Size.x);
        IM_CHECK_EQ(window->HitTestHoleSize.y, viewport->Size.y);

        ctx->DockInto("Left", vars.DockId, ImGuiDir_Left, true);
        ctx->DockInto("Up", vars.DockId, ImGuiDir_Up, true);
        ctx->DockInto("Right", vars.DockId, ImGuiDir_Right, true);
        ctx->DockInto("Down", vars.DockId, ImGuiDir_Down, true);
        ctx->Yield();

        // Dockspace with windows docked around it reduces central hole by their size + some padding.
        ImGuiWindow* left_window = ctx->GetWindowByRef("Left");
        ImGuiWindow* up_window = ctx->GetWindowByRef("Up");
        ImGuiWindow* right_window = ctx->GetWindowByRef("Right");
        ImGuiWindow* down_window = ctx->GetWindowByRef("Down");
        IM_CHECK_GT(viewport->Pos.x + window->HitTestHoleOffset.x, left_window->Pos.x + left_window->Size.x);
        IM_CHECK_GT(viewport->Pos.y + window->HitTestHoleOffset.y, up_window->Pos.y + up_window->Size.y);
        IM_CHECK_LT(viewport->Pos.x + window->HitTestHoleOffset.x + window->HitTestHoleSize.x, right_window->Pos.x);
        IM_CHECK_LT(viewport->Pos.y + window->HitTestHoleOffset.y + window->HitTestHoleSize.y, down_window->Pos.y);
    };

    // ## Test preserving docking information of closed windows. (#3716)
    t = IM_REGISTER_TEST(e, "docking", "docking_preserve_docking_info");
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, auto& vars) {
        vars.SetShowWindows(3, true);
    });
    t->GuiFunc = DockingTestsGenericGuiFunc;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        DockingTestsGenericVars& vars = ctx->GetVars<DockingTestsGenericVars>();

        ImGuiWindow* window1 = ctx->GetWindowByRef("AAA");
        ImGuiWindow* window2 = ctx->GetWindowByRef("BBB");
        ctx->DockClear("AAA", "BBB", "CCC", NULL);
        ctx->DockInto("AAA", vars.DockSpaceID, ImGuiDir_Left);
        ctx->DockInto("BBB", "AAA");
        vars.ShowWindow[0] = false;
        ctx->Yield(3);
        ctx->DockInto("CCC", "BBB", ImGuiDir_Down);
        vars.ShowWindow[0] = true;
        ctx->Yield(3);
        IM_CHECK(window1->DockId != 0);
        IM_CHECK(window1->DockId == window2->DockId);
    };

    // ## Test docked window focusing from the menu.
    t = IM_REGISTER_TEST(e, "docking", "docking_focus_from_menu");
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, auto& vars) { vars.SetShowWindows(3, true); });
    t->GuiFunc = DockingTestsGenericGuiFunc;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiWindow* windows[] = { ctx->GetWindowByRef("AAA"), ctx->GetWindowByRef("BBB"), ctx->GetWindowByRef("CCC") };
        ctx->DockClear("AAA", "BBB", "CCC", NULL);
        ctx->DockInto("BBB", "AAA");
        ctx->DockInto("CCC", "BBB");

        for (int i = 0; i < IM_ARRAYSIZE(windows); i++)
        {
            ctx->ItemClick(ImGui::DockNodeGetWindowMenuButtonId(windows[i]->RootWindowDockTree->DockNodeAsHost));
            ctx->SetRef("//$FOCUSED");
            ctx->ItemClick(windows[i]->Name);
            IM_CHECK(g.NavWindow == windows[i]);
        }
    };

    // ## Test docked window focusing.
    t = IM_REGISTER_TEST(e, "docking", "docking_focus_from_host");
    t->SetVarsDataType<DockingTestsGenericVars>();
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, auto& vars) { vars.SetShowWindows(2, true); });
    t->GuiFunc = DockingTestsGenericGuiFunc;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiWindow* windows[2] = { ctx->GetWindowByRef("AAA"), ctx->GetWindowByRef("BBB") };
        ctx->DockClear("AAA", "BBB", NULL);
        ctx->DockInto("BBB", "AAA", ImGuiDir_Right);

        // Test focusing window by clicking on it.
        for (int i = 0; i < IM_ARRAYSIZE(windows); i++)
        {
            ctx->MouseSetViewport(windows[i]);
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
            ctx->MouseSetViewport(windows[i]);
            ctx->MouseMoveToPos(bar_rect.GetCenter() + ImVec2(bar_rect.GetWidth() * 0.4f, 0.0f));
            ctx->MouseClick();
            //ctx->MouseDragWithDelta(ImVec2(3.0f * (i % 2 == 0 ? -1.0f : +1.0f), 0.0f));
            IM_CHECK(g.NavWindow == windows[i]);
        }

        // Check whether collapse button interaction focuses window.
        for (int i = 0; i < IM_ARRAYSIZE(windows); i++)
        {
            // FIXME-TESTS: Standardize access to window/collapse menu ID (there are 6 instances in tests)
            ctx->ItemClick(ImGui::DockNodeGetWindowMenuButtonId(windows[i]->DockNode));
            ctx->KeyPress(ImGuiKey_Escape);
            IM_CHECK(g.NavWindow == windows[i]);
        }

        // Check whether focus is maintained after splitter interaction.
        ImGuiWindow* last_focused_window = g.NavWindow;

        ImGuiWindow* root_dock = last_focused_window->RootWindowDockTree;
        ImGuiID splitter_id = ctx->GetID(Str64f("%s/$$%d/##Splitter", root_dock->Name, root_dock->DockNodeAsHost->ID).c_str());
        ctx->MouseMove(splitter_id);
        ctx->MouseDragWithDelta(ImVec2(0.0f, 10.0f));
        // FIXME-TESTS: This would fail due to a bug (#3820)
#if IMGUI_BROKEN_TESTS
        IM_CHECK(g.NavWindow == last_focused_window);
#endif
        IM_UNUSED(last_focused_window);
    };

    // # Test dock node focused flag.
    // # Test with child window injected into host window
    t = IM_REGISTER_TEST(e, "docking", "docking_focus_nodes_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        vars.DockId = ImGui::GetID("Dockspace");
        ImGui::DockSpace(vars.DockId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("A", NULL, ImGuiWindowFlags_NoSavedSettings); ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("B", NULL, ImGuiWindowFlags_NoSavedSettings); ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("C", NULL, ImGuiWindowFlags_NoSavedSettings); ImGui::End();
        if (ImGuiDockNode* node = ImGui::DockBuilderGetCentralNode(vars.DockId))
        {
            ImGui::SetCursorScreenPos(node->Pos + ImVec2(10, 10));
            ImGui::BeginChild("InjectedChild", node->Size - ImVec2(200, 20), ImGuiChildFlags_Border);
            ImGui::EndChild();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->WindowResize("Test Window", ImVec2(600.0f, 600.0f));
        ctx->DockClear("A", "B", "C", NULL);
        ctx->DockInto("A", vars.DockId, ImGuiDir_Left, true);
        ctx->DockInto("B", vars.DockId, ImGuiDir_Down, true);
        ctx->DockInto("C", "B");
        ImGuiWindow* window = ctx->GetWindowByRef("A");
        ImGuiWindow* window_child = ctx->WindowInfo("Test Window/InjectedChild").Window;
        ImGuiDockNode* node_root = ImGui::DockNodeGetRootNode(window->DockNode);
        ImGuiDockNode* node_split = node_root->ChildNodes[0];
        ImGuiDockNode* node_a = node_split->ChildNodes[0];
        ImGuiDockNode* node_central = node_split->ChildNodes[1];
        ImGuiDockNode* node_bc = node_root->ChildNodes[1];

        // Verify that only clicked node is focused.
        ctx->MouseMoveToPos(ctx->GetWindowTitlebarPoint("A"));
        ctx->MouseClick();
        IM_CHECK_EQ(node_central->IsFocused, false);
        IM_CHECK_EQ(node_root->IsFocused, false);
        IM_CHECK_EQ(node_split->IsFocused, false);
        IM_CHECK_EQ(node_a->IsFocused, true);
        IM_CHECK_EQ(node_bc->IsFocused, false);

        ctx->MouseMoveToPos(ctx->GetWindowTitlebarPoint("B"));
        ctx->MouseClick();
        IM_CHECK_EQ(node_central->IsFocused, false);
        IM_CHECK_EQ(node_root->IsFocused, false);
        IM_CHECK_EQ(node_split->IsFocused, false);
        IM_CHECK_EQ(node_a->IsFocused, false);
        IM_CHECK_EQ(node_bc->IsFocused, true);

        ctx->MouseMoveToPos(ctx->GetWindowTitlebarPoint("C"));
        ctx->MouseClick();
        IM_CHECK_EQ(node_central->IsFocused, false);
        IM_CHECK_EQ(node_root->IsFocused, false);
        IM_CHECK_EQ(node_split->IsFocused, false);
        IM_CHECK_EQ(node_a->IsFocused, false);
        IM_CHECK_EQ(node_bc->IsFocused, true);

        // Clicking another node and then central node does not focus any other nodes.
        ctx->MouseMoveToPos(window_child->Rect().GetTR() + ImVec2(10.0f, 10.0f));
        ctx->MouseClick();
        IM_CHECK_EQ(node_root->IsFocused, false);
        IM_CHECK_EQ(node_split->IsFocused, false);
        IM_CHECK_EQ(node_a->IsFocused, false);
        IM_CHECK_EQ(node_bc->IsFocused, false);
#if IMGUI_BROKEN_TESTS
        // Clicking central node focuses it even with _PassthruCentralNode flag.
        IM_CHECK_EQ(node_central->IsFocused, true);
#endif

        // Clicking another node and then central node does not focus any other node flags.
        ctx->MouseMoveToPos(ctx->GetWindowTitlebarPoint("C"));
        ctx->MouseClick();
        ctx->MouseMoveToPos(window_child->Rect().GetCenter());
        ctx->MouseClick();
        IM_CHECK_EQ(node_root->IsFocused, false);
        IM_CHECK_EQ(node_split->IsFocused, false);
        IM_CHECK_EQ(node_a->IsFocused, false);
        IM_CHECK_EQ(node_bc->IsFocused, false);
        IM_CHECK_EQ(node_central->IsFocused, false);
    };

    t = IM_REGISTER_TEST(e, "docking", "docking_focus_nodes_nested");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGui::DockSpace(ImGui::GetID("Dockspace1"));

        ImGui::Begin("A1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("SpacerSpacerSpacer-SpacerSpacerSpacer");
        ImGui::Text("SpacerSpacerSpacer-SpacerSpacerSpacer");
        ImGui::DockSpace(ImGui::GetID("Dockspace2"));
        ImGui::End();
        ImGui::Begin("A2", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("SpacerSpacerSpacer");
        ImGui::Text("SpacerSpacerSpacer");
        ImGui::End();

        ImGui::Begin("B1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("SpacerSpacerSpacer");
        ImGui::Text("SpacerSpacerSpacer");
        ImGui::DockSpace(ImGui::GetID("Dockspace3"));
        ImGui::End();
        ImGui::Begin("B2", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("SpacerSpacerSpacer");
        ImGui::Text("SpacerSpacerSpacer");
        ImGui::End();

        ImGui::Begin("C1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("SpacerSpacerSpacer");
        ImGui::Text("SpacerSpacerSpacer");
        ImGui::End();
        ImGui::Begin("C2", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("SpacerSpacerSpacer");
        ImGui::Text("SpacerSpacerSpacer");
        ImGui::End();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowResize("Test Window", ImVec2(600.0f, 600.0f));
        ctx->DockClear("A1", "A2", "B1", "B2", "C1", "C2", NULL);
        ctx->DockInto("A1", "//Test Window/Dockspace1", ImGuiDir_Left, true);
        ctx->DockInto("A2", "//Test Window/Dockspace1", ImGuiDir_Down, true);
        ctx->DockInto("B1", "//A1/Dockspace2", ImGuiDir_Left, true);
        ctx->DockInto("B2", "//A1/Dockspace2", ImGuiDir_Down, true);
        ctx->DockInto("C1", "//B1/Dockspace3", ImGuiDir_Left, true);
        ctx->DockInto("C2", "//B1/Dockspace3", ImGuiDir_Down, true); // Very small node made us land over the splitter, which prior to 2022/10/24 was broken.

        ctx->ItemClick("A1");
        IM_CHECK(ctx->GetWindowByRef("A1")->DockNode->IsFocused == true);
        IM_CHECK(ctx->GetWindowByRef("B1")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("C1")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("A2")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("B2")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("C2")->DockNode->IsFocused == false);

        ctx->ItemClick("C1");
        IM_CHECK(ctx->GetWindowByRef("A1")->DockNode->IsFocused == true);
        IM_CHECK(ctx->GetWindowByRef("B1")->DockNode->IsFocused == true);
        IM_CHECK(ctx->GetWindowByRef("C1")->DockNode->IsFocused == true);
        IM_CHECK(ctx->GetWindowByRef("A2")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("B2")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("C2")->DockNode->IsFocused == false);

        ctx->ItemClick("A2");
        IM_CHECK(ctx->GetWindowByRef("A1")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("B1")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("C1")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("A2")->DockNode->IsFocused == true);
        IM_CHECK(ctx->GetWindowByRef("B2")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("C2")->DockNode->IsFocused == false);

        ctx->ItemClick("B2");
        IM_CHECK(ctx->GetWindowByRef("A1")->DockNode->IsFocused == true);
        IM_CHECK(ctx->GetWindowByRef("B1")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("C1")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("A2")->DockNode->IsFocused == false);
        IM_CHECK(ctx->GetWindowByRef("B2")->DockNode->IsFocused == true);
        IM_CHECK(ctx->GetWindowByRef("C2")->DockNode->IsFocused == false);
    };

    // ## Test restoring dock tab state in independent and split windows.
    t = IM_REGISTER_TEST(e, "docking", "docking_tab_state");
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, auto& vars) { vars.SetShowWindows(6, true); });
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // FIXME-DOCKING
#if IMGUI_BROKEN_TESTS
        // Very interestingly, without the DockMultiClear() and with io.ConfigDockingAlwaysTabBar=true,
        // we get a crash when running a "docking_tab_state", "docking_focus_from_host", "docking_tab_state" sequence.
        // Begin("CCC") -> BeginDocked() binds CCC to an HostWindow which wasn't active at beginning of the frame, and it asserts in Begin()
        // Need to clarify into a simpler repro
#endif
        if (ctx->IsFirstGuiFrame())
            ctx->DockClear("AAA", "BBB", "CCC", "DDD", "EEE", "FFF", NULL);

        DockingTestsGenericGuiFunc(ctx);
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        DockingTestsGenericVars& vars = ctx->GetVars<DockingTestsGenericVars>();
        ImGuiWindow* window_aaa = ctx->GetWindowByRef("AAA");
        ImGuiWindow* window_ddd = ctx->GetWindowByRef("DDD");

        for (int variant = 0; variant < 2; variant++)
        {
            ctx->LogDebug("## TEST CASE %d", variant);
            ctx->DockClear("AAA", "BBB", "CCC", "DDD", "EEE", "FFF", NULL);

            if (variant == 1)
                ctx->DockInto("DDD", "AAA", ImGuiDir_Right);
            ctx->DockInto("BBB", "AAA");
            ctx->DockInto("CCC", "BBB");
            ctx->DockInto("EEE", "DDD");
            ctx->DockInto("FFF", "EEE");

            ImGuiTabBar* tab_bar_aaa = window_aaa->DockNode->TabBar;
            ImGuiTabBar* tab_bar_ddd = window_ddd->DockNode->TabBar;

            // Rearrange tabs and set active tab.
            ctx->ItemDragAndDrop("CCC/#TAB", "AAA/#TAB");
            ctx->ItemDragAndDrop("DDD/#TAB", "FFF/#TAB");
            ctx->ItemClick("AAA/#TAB");
            ctx->ItemClick("FFF/#TAB");

            // Verify initial order.
            const char* tab_order_1[] = { "CCC", "AAA", "BBB", NULL };
            const char* tab_order_2[] = { "EEE", "FFF", "DDD", NULL };
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar_aaa, tab_order_1));
            IM_CHECK(ctx->TabBarCompareOrder(tab_bar_ddd, tab_order_2));

            IM_CHECK(tab_bar_aaa->Tabs.Size >= 2);
            IM_CHECK(tab_bar_aaa->VisibleTabId == tab_bar_aaa->Tabs[1].ID);
            IM_CHECK(tab_bar_ddd->VisibleTabId == tab_bar_ddd->Tabs[1].ID);

            // Hide and show all windows.
            vars.SetShowWindows(7, false);
            ctx->Yield(2);
            vars.SetShowWindows(7, true);
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

    // ## Test dock node retention when second window in two-way split is undocked.
    t = IM_REGISTER_TEST(e, "docking", "docking_undock_simple");
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, auto& vars) { vars.SetShowWindows(2, true); });
    t->GuiFunc = DockingTestsGenericGuiFunc;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window0 = ctx->GetWindowByRef("AAA");
        ImGuiWindow* window1 = ctx->GetWindowByRef("BBB");
        ctx->DockClear("AAA", "BBB", NULL);
        ctx->DockInto("BBB", "AAA", ImGuiDir_Right);
        IM_CHECK_NE(window0->DockNode, (ImGuiDockNode*)NULL);
        IM_CHECK_NE(window1->DockNode, (ImGuiDockNode*)NULL);
        ImGuiDockNode* original_node = window0->DockNode->ParentNode;
        ctx->UndockWindow("BBB");
        IM_CHECK_EQ(window0->DockNode, original_node);          // Undocking BBB keeps a parent dock node in AAA
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window1));   // Undocked window
    };

    // ## Test undocking window from dockspace covering entire window.
    t = IM_REGISTER_TEST(e, "docking", "docking_undock_large");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Appearing);
        ImGui::Begin("Window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("This is window 1");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        const ImGuiPlatformMonitor* monitor = ImGui::GetViewportPlatformMonitor(main_viewport);
        ImGuiWindow* window = ctx->GetWindowByRef("Window 1");
        IM_CHECK(window->DockNode != NULL);

        ImVec2 initial_size = main_viewport->Size;
        IM_CHECK(window->Size.x == initial_size.x);
        IM_CHECK(window->Size.y == initial_size.y);

        ctx->ItemDragWithDelta("Window 1/#TAB", ImVec2(50.0f, 50.0f));
        IM_CHECK(ctx->WindowIsUndockedOrStandalone(window));

        ImVec2 expect_size;
        ImVec2 min_size = main_viewport->Size * ImVec2(0.90f, 0.90f);
        if (ctx->UiContext->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            expect_size = ImMin(ImMax(window->Size, min_size), monitor->WorkSize);  // Viewport can be bigger than main viewport, but no larger than containing monitor
        else
            expect_size = min_size;                                                 // Always smaller than main viewport

        IM_CHECK(window->Size.x <= expect_size.x);
        IM_CHECK(window->Size.y <= expect_size.y);
    };

    // ## Test that undocking a whole _node_ doesn't lose/reset size
    t = IM_REGISTER_TEST(e, "docking", "docking_undock_from_dockspace_size");
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, auto& vars) { vars.SetShowWindows(2, true); });
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        if (ctx->IsFirstTestFrame())
        {
            ImGuiID dockspace_id = ctx->GetID("//Test Window/dockspace");
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderDockWindow("AAA", dockspace_id);
            ImGui::DockBuilderDockWindow("BBB", dockspace_id);
        }
        DockingTestsGenericGuiFunc(ctx);
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiID dockspace_id = ctx->GetID("//Test Window/dockspace");

        ImGuiWindow* window_a = ImGui::FindWindowByName("AAA");
        ImGuiWindow* window_b = ImGui::FindWindowByName("BBB");
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
        ctx->WindowCollapse("Window A", false);
        ctx->DockClear("Window A", "Window B", NULL);
        ImGuiWindow* windowB = ctx->GetWindowByRef("Window B");
        ImGuiWindow* windowA2 = ctx->GetWindowByRef("Window A2");

        // Window A2 is visible when all windows are undocked.
        IM_CHECK(windowA2->Active);
        ctx->WindowFocus("Window A");
        ctx->DockInto("Window B", "Window A");

        // Window A2 gets hidden when Window B is docked as it becomes the active window in the dock.
        IM_CHECK(!windowA2->Active);

        // Undock Window B.
        // FIXME-TESTS: This block is a slightly modified version of the UndockWindow() code which itself is using ItemDragWithDelta(),
        // we cannot use UndockWindow() only because we perform our checks in the middle of the operation.
        {
            if (!g.IO.ConfigDockingWithShift)
                ctx->KeyDown(ImGuiMod_Shift); // Disable docking
            ctx->MouseMove(windowB->TabId, ImGuiTestOpFlags_NoCheckHoveredId);
            ctx->MouseDown(0);

            const float h = windowB->TitleBarHeight;
            ctx->MouseMoveToPos(g.IO.MousePos + (ImVec2(h, h) * -2));
            ctx->Yield();

            // Window A2 becomes visible during undock operation, but does not capture focus.
            IM_CHECK(windowA2->Active);
            IM_CHECK_EQ(g.MovingWindow, windowB);
            IM_CHECK_EQ(g.WindowsFocusOrder[g.WindowsFocusOrder.Size - 1], windowB);

            ctx->MouseUp(0);
            if (!g.IO.ConfigDockingWithShift)
                ctx->KeyUp(ImGuiMod_Shift);
        }
    };

    // ## Test resizing
    t = IM_REGISTER_TEST(e, "docking", "docking_sizing_1");
    t->SetVarsDataType<DockingTestsGenericVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<DockingTestsGenericVars>();

        ImGuiID& dockMainId = vars.NodesId[0];
        ImGuiID& dockTopRightId = vars.NodesId[1];
        ImGuiID& dockTopLeftId = vars.NodesId[2];
        ImGuiID& dockDownId = vars.NodesId[3];

        if (ctx->FirstGuiFrame)
        {
            ImGuiID root_id;
            root_id = dockMainId = ImGui::GetID("Test Node");
            ImGui::DockBuilderRemoveNode(dockMainId);
            ImGui::DockBuilderAddNode(dockMainId, ImGuiDockNodeFlags_CentralNode);
            ImGui::DockBuilderSetNodePos(dockMainId, ImGui::GetMainViewport()->Pos + ImVec2(20, 20));
            ImGui::DockBuilderSetNodeSize(dockMainId, ImVec2(1000, 500));

            ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, 0.20f, &dockTopRightId, &dockMainId);
            ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Left, 0.20f / 0.80f + 0.002f, &dockTopLeftId, &dockMainId);
            ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Down, 0.20f, &dockDownId, &dockMainId);
            ImGui::DockBuilderDockWindow("dockMainId", dockMainId);
            ImGui::DockBuilderDockWindow("dockTopRightId", dockTopRightId);
            ImGui::DockBuilderDockWindow("dockTopLeftId", dockTopLeftId);
            ImGui::DockBuilderDockWindow("dockDownId", dockDownId);
            ImGui::DockBuilderFinish(root_id);
        }

        // FIXME-TESTS: DockBuilderDockWindow() doesn't work on windows that haven't been created and use ImGuiWindowFlags_NoSavedSettings
        static const float DOCKING_SPLITTER_SIZE = 2.0f; // FIXME-TESTS
        {
            ImGui::Begin("dockMainId", NULL, ImGuiWindowFlags_NoSavedSettings * 0);
            ImVec2 sz = ImGui::GetWindowSize();
            //if (ctx->FirstGuiFrame) // FIXME-DOCK: window size will be pulled from last settings until node appears
            if (vars.Step == 1)
            {
                IM_CHECK(ImGui::GetWindowDockID() == dockMainId);
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.x, 1000 - 200 - 200, DOCKING_SPLITTER_SIZE * 2);
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.y, 500 - 100, DOCKING_SPLITTER_SIZE * 2);
            }
            if (vars.Step == 2)
            {
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.x, 1000 - 300 - 300, DOCKING_SPLITTER_SIZE * 2);
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.y, 400 - 100, DOCKING_SPLITTER_SIZE * 2);
            }
            ImGui::Text("(%.1f,%.1f)", sz.x, sz.y);
            ImGui::End();
        }
        {
            ImGui::Begin("dockTopRightId", NULL, ImGuiWindowFlags_NoSavedSettings * 0);
            ImVec2 sz = ImGui::GetWindowSize();
            //if (ctx->FirstGuiFrame) // FIXME-DOCK: window size will be pulled from last settings until node appears
            if (vars.Step == 1)
            {
                IM_CHECK(ImGui::GetWindowDockID() == dockTopRightId);
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.x, 200, DOCKING_SPLITTER_SIZE * 2);
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.y, 500, DOCKING_SPLITTER_SIZE * 2);
            }
            if (vars.Step == 2)
            {
#if IMGUI_BROKEN_TESTS
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.x, 100, DOCKING_SPLITTER_SIZE * 2); // Docking size application is depth-first, so L|C|R will not resize neatly yet
#endif
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.y, 400, DOCKING_SPLITTER_SIZE * 2);
            }
            ImGui::Text("(%.1f,%.1f)", sz.x, sz.y);
            ImGui::End();
        }
        {
            ImGui::Begin("dockTopLeftId", NULL, ImGuiWindowFlags_NoSavedSettings * 0);
            ImVec2 sz = ImGui::GetWindowSize();
            //if (ctx->FirstGuiFrame) // FIXME-DOCK: window size will be pulled from last settings until node appears
            if (vars.Step == 1)
            {
                IM_CHECK(ImGui::GetWindowDockID() == dockTopLeftId);
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.x, 200, DOCKING_SPLITTER_SIZE * 2);
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.y, 500, DOCKING_SPLITTER_SIZE * 2);
            }
            if (vars.Step == 2)
            {
#if IMGUI_BROKEN_TESTS
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.x, 100, DOCKING_SPLITTER_SIZE * 2); // Docking size application is depth-first, so L|C|R will not resize neatly yet
#endif
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.y, 400, DOCKING_SPLITTER_SIZE * 2);
            }
            ImGui::Text("(%.1f,%.1f)", sz.x, sz.y);
            ImGui::End();
        }
        {
            ImGui::Begin("dockDownId", NULL, ImGuiWindowFlags_NoSavedSettings * 0);
            ImVec2 sz = ImGui::GetWindowSize();
            //if (ctx->FirstGuiFrame) // FIXME-DOCK: window size will be pulled from last settings until node appears
            if (vars.Step == 1)
            {
                IM_CHECK(ImGui::GetWindowDockID() == dockDownId);
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.y, 100, DOCKING_SPLITTER_SIZE * 2);
            }
            if (vars.Step == 2)
            {
                IM_CHECK_FLOAT_NEAR_NO_RET(sz.y, 100, DOCKING_SPLITTER_SIZE * 2);
            }

            ImGui::Text("(%.1f,%.1f)", sz.x, sz.y);
            ImGui::End();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<DockingTestsGenericVars>();
        vars.Step = 0;
        ctx->Yield(2);

        vars.Step = 1;
        ImGuiDockNode* root_node = ImGui::DockNodeGetRootNode(ImGui::DockBuilderGetNode(vars.NodesId[0]));
        IM_CHECK(root_node != NULL);
        ImGuiWindow* host_window = root_node->HostWindow;
        IM_CHECK(host_window != NULL);
        IM_CHECK_EQ(host_window->Size, ImVec2(1000, 500));
        ctx->Yield(2);

        vars.Step = 0;
        ctx->SetRef(host_window->Name);
        ctx->WindowResize("", ImVec2(800, 400));
        vars.Step = 2;
        ctx->Yield(2);
    };

    // ## Test transferring full node payload, even with hidden windows.
    t = IM_REGISTER_TEST(e, "docking", "docking_split_payload");
    t->SetVarsDataType<DockingTestsGenericVars>([](auto* ctx, DockingTestsGenericVars& vars) { vars.SetShowWindows(3, true); });
    t->GuiFunc = DockingTestsGenericGuiFunc;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        DockingTestsGenericVars& vars = ctx->GetVars<DockingTestsGenericVars>();
        ImGuiWindow* window_aaa = ctx->GetWindowByRef("AAA");
        ImGuiWindow* window_bbb = ctx->GetWindowByRef("BBB");
        ImGuiWindow* window_ccc = ctx->GetWindowByRef("CCC");

        // Test variants:
        // BBB | CCC
        // Hide BBB
        // Dock CCC into AAA (0) or dockspace (1)
        // Undock CCC (2, 3)
        // Show BBB
        for (int variant = 0; variant < 4; variant++)
        {
            ctx->Test->ArgVariant = variant;
            vars.ShowDockspace = (variant & 1) != 0;
            vars.SetShowWindows(3, true);
            ctx->LogDebug("## TEST CASE: %d", variant);
            ctx->DockClear("AAA", "BBB", "CCC", NULL);
            ctx->DockInto("BBB", "CCC", ImGuiDir_Left);                         // BBB | CCC
            vars.ShowWindow[1] = false;                                         // Hide BBB
            ctx->Yield();

            ImGuiID dock_dest = vars.ShowDockspace ? vars.DockSpaceID : window_aaa->ID;
#if IMGUI_BROKEN_TESTS
            // FIXME-TESTS: Docking from single visible tab (part of a hidden iceberg of node) and docking from collapse button do not behave the same the moment.
            // This currently will undock CCC away from the hidden nodes, which is incorrect but desirable as currently single-window-in-node have various regression compared to single-window.
            ctx->DockInto("CCC", dock_dest);                                    // Dock CCC into AAA
#else
            ctx->DockInto(window_ccc->DockId, dock_dest);
#endif

            // Undock CCC with hidden BBB.
            if (variant >= 2)
                ctx->UndockWindow("CCC");

            // variant 0: (BBB) | AAA
            // variant 1: [ (BBB) | CCC ]   node CCC is central
            // variant 2: (BBB) | AAA + CCC
            // variant 3: [ ]  + (BBB) | CCC  desirable but currently BBB is simply undocked
            vars.ShowWindow[1] = true; // Show BBB
            ctx->Yield();

            ImGuiDockNode* root_node = NULL;
            switch (variant)
            {
            case 0:
                // variant 0: (BBB) | AAA, CCC ----> BBB | AAA, CCC
                root_node = ImGui::DockNodeGetRootNode(window_aaa->DockNode);
                IM_CHECK_EQ(root_node->ChildNodes[0]->Windows.Size, 1);
                IM_CHECK_EQ(root_node->ChildNodes[0]->ID, window_bbb->DockId);
                IM_CHECK_EQ(root_node->ChildNodes[1]->Windows.Size, 2);
                IM_CHECK_EQ(root_node->ChildNodes[1]->ID, window_aaa->DockId);
                IM_CHECK_EQ(root_node->ChildNodes[1]->ID, window_ccc->DockId);
                break;
            case 1:
                // variant 1: [ (BBB) | CCC ] ----> [ BBB | CCC ]
                root_node = ImGui::DockNodeGetRootNode(window_ccc->DockNode);
                IM_CHECK(root_node->IsDockSpace());
                IM_CHECK_EQ(root_node->ChildNodes[0]->Windows.Size, 1);
                IM_CHECK_EQ(root_node->ChildNodes[0]->ID, window_bbb->DockId);
                IM_CHECK_EQ(root_node->ChildNodes[1]->Windows.Size, 1);
                IM_CHECK_EQ(root_node->ChildNodes[1]->ID, window_ccc->DockId);
                IM_CHECK(root_node->ChildNodes[1]->IsCentralNode());
                break;
            case 2:
                // variant 2: (BBB) | AAA + CCC ----> BBB | AAA + CCC
                root_node = ImGui::DockNodeGetRootNode(window_aaa->DockNode);
                IM_CHECK_EQ(root_node->ChildNodes[0]->ID, window_bbb->DockId);
                IM_CHECK_EQ(root_node->ChildNodes[1]->ID, window_aaa->DockId);
                IM_CHECK_NE(root_node->ChildNodes[1]->ID, window_ccc->DockId);  // !=
                break;
            case 3:
                // variant 3: [ ]  + BBB | CCC  desirable but current [ BBB | ... ] + CCC
                root_node = ImGui::DockBuilderGetNode(dock_dest);               // DockNode of dockspace is left empty
                IM_CHECK(root_node->IsDockSpace());
#if IMGUI_BROKEN_TESTS
                // Expected
                IM_CHECK(root_node->IsEmpty());
                root_node = ImGui::DockNodeGetRootNode(window_ccc->DockNode);
                IM_CHECK_EQ(root_node->ChildNodes[0]->Windows.Size, 1);
                IM_CHECK_EQ(root_node->ChildNodes[0]->ID, window_bbb->DockId);
                IM_CHECK_EQ(root_node->ChildNodes[1]->Windows.Size, 1);
                IM_CHECK_EQ(root_node->ChildNodes[1]->ID, window_ccc->DockId);
#else
                // Current
                IM_CHECK(root_node->ChildNodes[0] != NULL);
                IM_CHECK(root_node->ChildNodes[0]->ID == window_bbb->DockId);
                IM_CHECK(window_ccc->DockId == 0);
#endif
                break;
            }
        }
    };

    // ## Test transferring full node payload, even with hidden windows.
    t = IM_REGISTER_TEST(e, "docking", "docking_window_appearing");
    struct DockingWindowAppearingVars { bool ShowAAA = false; bool ShowBBB = false; int AppearingAAA = 0; int AppearingBBB = 0; bool WithWindowAppending = false; ImGuiID SetNextBBBDockID = 0; };
    t->SetVarsDataType<DockingWindowAppearingVars>();
    t->GuiFunc =  [](ImGuiTestContext* ctx)
    {
        DockingWindowAppearingVars& vars = ctx->GetVars<DockingWindowAppearingVars>();
        if (vars.ShowAAA)
        {
            ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Appearing);
            if (ImGui::Begin("AAA"))
            {
                vars.AppearingAAA += ImGui::IsWindowAppearing();
                ImGui::TextUnformatted("AAA");
                ImGui::Text("AppearingCount: %d, DockNode: %x", vars.AppearingAAA, ImGui::GetWindowDockID());
            }
            ImGui::End();
            if (vars.WithWindowAppending)
            {
                ImGui::Begin("AAA");
                ImGui::Text("(appending)");
                ImGui::End();
            }
        }
        if (vars.ShowBBB)
        {
            if (vars.SetNextBBBDockID != 0)
            {
                ImGui::SetNextWindowDockID(vars.SetNextBBBDockID, ImGuiCond_Appearing);
                vars.SetNextBBBDockID = 0;
            }
            ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Appearing);
            bool ret = ImGui::Begin("BBB");
            bool appearing = ImGui::IsWindowAppearing();
            ctx->LogDebug("BBB ret %d appearing %d", ret, appearing);
            //if (ImGui::Begin("BBB"))// {} // FIXME: Try with and without
            if (ret)
            {
                vars.AppearingBBB += appearing;
                ImGui::TextUnformatted("BBB");
                ImGui::Text("AppearingCount: %d, DockNode: %x", vars.AppearingBBB, ImGui::GetWindowDockID());
            }
            ImGui::End();
            if (vars.WithWindowAppending)
            {
                ImGui::Begin("BBB");
                ImGui::Text("(appending)");
                ImGui::End();
            }
        }
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Appearing);
        ImGui::Begin("CCC");
        ImGui::TextUnformatted("CCC");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        DockingWindowAppearingVars& vars = ctx->GetVars<DockingWindowAppearingVars>();
        for (int variant = 0; variant < 2; variant++)
        {
            vars.ShowAAA = vars.ShowBBB = false;
            vars.WithWindowAppending = (variant == 1);
            vars.AppearingAAA = vars.AppearingBBB = 0;
            ctx->LogInfo("WithWindowAppending = %d", vars.WithWindowAppending);

            ctx->DockClear("AAA", "BBB", "CCC", NULL);

            auto reappear_window = [](ImGuiTestContext* ctx, bool* show_window)
            {
                *show_window = false;
                ctx->Yield(2);
                *show_window = true;
                ctx->Yield(2);
            };

            // Not docked
            vars.ShowAAA = true;
            ctx->Yield(2);
            IM_CHECK_EQ(vars.AppearingAAA, 1);
            vars.ShowBBB = true;
            ctx->Yield(2);
            IM_CHECK_EQ(vars.AppearingBBB, 1);

            // Docked as tabs
            ctx->DockInto("BBB", "AAA");
            IM_CHECK_EQ(vars.AppearingAAA, 1);
            IM_CHECK_EQ(vars.AppearingBBB, 1);
            reappear_window(ctx, &vars.ShowBBB);
            IM_CHECK_EQ(vars.AppearingBBB, 2);

            // Docked as splits
            ctx->DockClear("AAA", "BBB", NULL);
            IM_CHECK_EQ(vars.AppearingAAA, 1);
            IM_CHECK_EQ(vars.AppearingBBB, 2);
            //IM_SUSPEND_TESTFUNC();
            ctx->DockInto("BBB", "AAA", ImGuiDir_Right);
            IM_CHECK_EQ(vars.AppearingAAA, 1);
            IM_CHECK_EQ(vars.AppearingBBB, 2);
            reappear_window(ctx, &vars.ShowBBB);
            IM_CHECK_EQ(vars.AppearingAAA, 1);
            IM_CHECK_EQ(vars.AppearingBBB, 3); // <---

            // Docked using SetNextWindowDockID()
            ImGuiWindow* window_aaa = ctx->GetWindowByRef("AAA");
            vars.SetNextBBBDockID = window_aaa->DockId;
            vars.ShowBBB = false;
            ctx->DockClear("AAA", "BBB", "CCC", NULL);
            ctx->DockInto("CCC", "AAA");
            reappear_window(ctx, &vars.ShowBBB);
            IM_CHECK_EQ(vars.AppearingBBB, 4);

            // Switching tabs
            ctx->WindowFocus("AAA");
            ctx->Yield();
            IM_CHECK_EQ(vars.AppearingAAA, 2);
        }
    };

    // ## Test reappearing windows and how they affect layout of the other windows across frames
    // Among other things, this test how DockContextBindNodeToWindow() calls DockNodeTreeUpdatePosSize() to evaluate upcoming node pos/size mid-frame (improved 2021-12-02)
    t = IM_REGISTER_TEST(e, "docking", "docking_window_appearing_layout");
    struct DockingWindowAppearingVars2 { bool ShowAAA = false; bool ShowBBB = false; bool Log = false; bool SwapShowOrder = false; ImVec2 PosA, PosB, SizeA, SizeB; };
    t->SetVarsDataType<DockingWindowAppearingVars2>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DockingWindowAppearingVars2& vars = ctx->GetVars<DockingWindowAppearingVars2>();

        for (int n = 0; n < 2; n++)
        {
            if (vars.ShowAAA && ((vars.SwapShowOrder == false && n == 0) || (vars.SwapShowOrder == true && n == 1)))
            {
                ImGui::Begin("AAA");
                ImGui::TextUnformatted("AAA");
                vars.PosA = ImGui::GetWindowPos();
                vars.SizeA = ImGui::GetWindowSize();
                ImGui::Text("Pos (%.1f,%.1f) Size (%.1f,%.1f)", vars.PosA.x, vars.PosA.y, vars.SizeA.x, vars.SizeA.y);
                if (vars.Log)
                    ctx->LogInfo("AAA Size (%.1f,%.1f)", vars.SizeA.x, vars.SizeA.y);
                ImGui::End();
            }
            if (vars.ShowBBB && ((vars.SwapShowOrder == true && n == 0) || (vars.SwapShowOrder == false && n == 1)))
            {
                ImGui::Begin("BBB");
                ImGui::TextUnformatted("BBB");
                vars.PosB = ImGui::GetWindowPos();
                vars.SizeB = ImGui::GetWindowSize();
                ImGui::Text("Pos (%.1f,%.1f) Size (%.1f,%.1f)", vars.PosB.x, vars.PosB.y, vars.SizeB.x, vars.SizeB.y);
                if (vars.Log)
                    ctx->LogInfo("BBB Size (%.1f,%.1f)", vars.SizeB.x, vars.SizeB.y);
                ImGui::End();
            }
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        DockingWindowAppearingVars2& vars = ctx->GetVars<DockingWindowAppearingVars2>();
        vars.ShowAAA = vars.ShowBBB = true;
        ctx->DockClear("AAA", "BBB", NULL);
        ctx->WindowResize("AAA", ImVec2(400, 800));
        ctx->WindowResize("BBB", ImVec2(400, 800));
        ctx->DockInto("BBB", "AAA", ImGuiDir_Down);

        ImGuiWindow* window_AAA = ctx->GetWindowByRef("AAA");
        ImGuiWindow* window_BBB = ctx->GetWindowByRef("BBB");

        vars.Log = true;

        for (int variant = 0; variant < 2; variant++) // Test both submission orders
        {
            vars.SwapShowOrder = (variant == 1);
            ctx->LogDebug("Variant %d", variant);
            IM_CHECK(vars.SizeA.x == 400 && vars.SizeA.y <= 400);
            IM_CHECK(vars.SizeB.x == 400 && vars.SizeB.y <= 400);

            // Hiding AAA (top)
            ctx->LogDebug("Hide AAA");
            vars.ShowAAA = false;
            ctx->Yield();
            IM_CHECK_NO_RET(vars.SizeB.y <= 400);
            ctx->Yield();
            IM_CHECK_NO_RET(vars.SizeB.y == 800);

            // Showing AAA (top)
            ctx->LogDebug("Show AAA");
            vars.ShowAAA = true;
            window_AAA->Size = window_AAA->SizeFull = vars.SizeA = ImGui::DockBuilderGetNode(window_AAA->DockId)->Size = ImVec2(666.0f, 666.0f); // Zealously clear trace of old size to further test that it will be recalculated
            ctx->Yield();
            IM_CHECK_NO_RET(vars.SizeA.y <= 400);
            IM_CHECK_NO_RET(vars.SizeB.y == 800); // Behavior changed/fixed on 2021/12/02
            ctx->Yield();
            IM_CHECK_NO_RET(vars.SizeA.y <= 400);
            IM_CHECK_NO_RET(vars.SizeB.y <= 400);

            // Hiding BBB (bottom)
            ctx->LogDebug("Hide BBB");
            vars.ShowBBB = false;
            ctx->Yield();
            IM_CHECK_NO_RET(vars.SizeA.y <= 400);
            ctx->Yield();
            IM_CHECK_NO_RET(vars.SizeA.y == 800);

            // Showing BBB (bottom)
            ctx->LogDebug("Show BBB");
            vars.ShowBBB = true;
            window_BBB->Size = window_BBB->SizeFull = vars.SizeB = ImGui::DockBuilderGetNode(window_BBB->DockId)->Size = ImVec2(666.0f, 666.0f); // Zealously clear trace of old size to further test that it will be recalculated
            ctx->Yield();
            IM_CHECK_NO_RET(vars.SizeA.y == 800); // Behavior changed/fixed on 2021/12/02
            IM_CHECK_NO_RET(vars.SizeB.y <= 400);
            ctx->Yield();
            IM_CHECK_NO_RET(vars.SizeA.y <= 400);
            IM_CHECK_NO_RET(vars.SizeB.y <= 400);
        }
    };

    // ## Test an edge case with FindBlockingModal() requiring ParentWindowInBeginStack to be set (#5401)
#if IMGUI_VERSION_NUM >= 18729
    t = IM_REGISTER_TEST(e, "docking", "docking_popup_parent");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGuiID dockspace_id = ImGui::GetID("RootDockspace");
        if (ctx->IsFirstGuiFrame())
        {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockSpace(dockspace_id);
            ImGuiID node_1 = 0;
            ImGuiID node_2 = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.42f, NULL, &node_1);
            ImGui::DockBuilderDockWindow("WindowWithPopup", node_1);
            ImGui::DockBuilderDockWindow("WindowAfterPopup", node_2);
            ImGui::DockBuilderFinish(dockspace_id);
        }
        else
        {
            ImGui::DockSpace(dockspace_id);
        }
        ImGui::End(); // Root Window

        ImGui::Begin("WindowWithPopup");
        if (!ImGui::IsPopupOpen("popup"))
            ImGui::OpenPopup("popup");
        if (ImGui::BeginPopupModal("popup"))
            ImGui::EndPopup();

        ImGui::End();

        // Crash occurred here (see #5401)
        ImGui::Begin("WindowAfterPopup");
        ImGui::End();
    };
#endif

    // ## Test DockNodeBeginAmendTabBar() and TabItemButton() use with a dockspace. (#5515)
    t = IM_REGISTER_TEST(e, "docking", "docking_dockspace_tab_amend");
    struct DockspaceTabButtonVars { ImGuiID DockId; int ButtonClicks = 0; };
    t->SetVarsDataType<DockspaceTabButtonVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DockspaceTabButtonVars& vars = ctx->GetVars<DockspaceTabButtonVars>();
        vars.DockId = ImGui::DockSpaceOverViewport();
        ImGuiWindow* window_demo = ctx->GetWindowByRef("//Dear ImGui Demo");
        IM_CHECK(window_demo != NULL);

        // FIXME: It can be confusing which dock node we are supposed to use. While under normal circumstances using
        //  dockspace ID works, is simple and intuitive thing to do, it breaks when dockspace already has docked windows
        //  that are hidden. For example if we run "docking_dockspace_passthru_padding,docking_dockspace_tab_button"
        //  tests, first test inserts four window into dockspace over viewport and they disappear when test stops. This
        //  test uses same dockspace and docking demo window into central node actually docks it way down the dock node
        //  tree. Since other windows are invisible, this new dock node which is multiple levels deep into dock node
        //  tree contains a tab bar instead of root dockspace node.
        //ImGuiDockNode* node = ImGui::DockContextFindNodeByID(ctx->UiContext, vars.DockId);
        ImGuiDockNode* node = window_demo->DockNode;
        if (node && ImGui::DockNodeBeginAmendTabBar(node))
        {
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Leading))
                vars.ButtonClicks++;
            ImGui::DockNodeEndAmendTabBar();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        DockspaceTabButtonVars& vars = ctx->GetVars<DockspaceTabButtonVars>();
        ImGuiWindow* window_demo = ctx->GetWindowByRef("Dear ImGui Demo");
        ctx->DockInto("Dear ImGui Demo", vars.DockId);
        IM_CHECK(g.WindowsFocusOrder.back() == window_demo);

        // Appended button is functional.
        ctx->ItemClick(ctx->GetID("+", window_demo->DockNode->ID /*vars.DockId*/)); // FIXME: See GuiFunc.
        IM_CHECK(vars.ButtonClicks == 1);
#if IMGUI_BROKEN_TESTS
        IM_CHECK(g.WindowsFocusOrder.back() == window_demo);  // Dock host remains focused.
#endif
        // TabItemButton() does not trigger an assert (e926a6).
        ctx->ItemClick(ctx->GetID("#CLOSE", window_demo->DockNode->ID));
        ctx->ItemCheck("Hello, world!/Demo Window");

        // TabItemButton() did cause a dockspace child window get inserted into g.WindowsFocusOrder which eventually
        // caused a crash (#5515, 0e95cf)..
        ctx->MenuCheck("//Dear ImGui Demo/Tools/Metrics\\/Debugger");
        ctx->WindowClose("Dear ImGui Metrics\\/Debugger");
        for (ImGuiWindow* window : g.WindowsFocusOrder)
            if (!window->DockNodeIsVisible) // Docked windows are converted to child windows and are valid in this list
            { // VS2015 throws a "error C2059: syntax error: '}'" when using for-range/if/do-while, I don't understand it. VS2019 is ok.
                // FIXME-TESTS: Test is not totally correct, see "window_popup_child_without_input_blocking"
                IM_CHECK_SILENT((window->Flags & ImGuiWindowFlags_ChildWindow) == 0 || (window->Flags & ImGuiWindowFlags_Popup));
            }
    };

#else
    IM_UNUSED(e);
#endif
}
