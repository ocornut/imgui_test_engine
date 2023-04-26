// dear imgui
// (tests:  inputs)

#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_test_suite.h"
#include "imgui_test_engine/imgui_te_engine.h"      // IM_REGISTER_TEST()
#include "imgui_test_engine/imgui_te_context.h"
#include "imgui_test_engine/imgui_te_utils.h"       // TableXXX hepers, FindFontByName(), etc.

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
static inline bool operator!=(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x != rhs.x || lhs.y != rhs.y; }    // for IM_CHECK_NE()

//-------------------------------------------------------------------------
// Tests: Inputs
//-------------------------------------------------------------------------

void RegisterTests_Inputs(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test input queue trickling
    t = IM_REGISTER_TEST(e, "inputs", "inputs_io_inputqueue");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::SetNextWindowPos(ImVec2(80, 80));
        ImGui::SetNextWindowSize(ImVec2(500, 500));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);
        ctx->UiContext->WantTextInputNextFrame = vars.Bool1; // Simulate InputText() without eating inputs
        //ImGui::InputText("InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        //if (vars.Bool1)
        //    ImGui::SetKeyboardFocusHere(-1);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGuiContext& g = *ctx->UiContext;
        ImGuiIO& io = g.IO;

        ctx->RunFlags |= ImGuiTestRunFlags_EnableRawInputs; // Disable TestEngine submitting inputs events
        ctx->Yield();

        // MousePos -> 1 frame
        io.AddMousePosEvent(100.0f, 100.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(100.0f, 100.0f));

        // MousePos x3 -> 1 frame
        io.AddMousePosEvent(110.0f, 110.0f);
        io.AddMousePosEvent(120.0f, 120.0f);
        io.AddMousePosEvent(130.0f, 130.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(130.0f, 130.0f));

        // MousePos, MouseButton -> 1 frame
        io.AddMousePosEvent(140.0f, 140.0f);
        io.AddMouseButtonEvent(0, true);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(140.0f, 140.0f));
        IM_CHECK_EQ(io.MouseDown[0], true);
        io.AddMouseButtonEvent(0, false);
        ctx->Yield();

        // MouseButton | MousePos -> 2 frames
        io.AddMouseButtonEvent(0, true);
        io.AddMousePosEvent(150.0f, 150.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(140.0f, 140.0f));
        IM_CHECK_EQ(io.MouseDown[0], true);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(150.0f, 150.0f));
        IM_CHECK_EQ(io.MouseDown[0], true);
        io.AddMouseButtonEvent(0, false);
        ctx->Yield();

        // MousePos, MouseButton | MousePos -> 2 frames
        io.AddMousePosEvent(100.0f, 100.0f);
        io.AddMouseButtonEvent(0, true);
        io.AddMousePosEvent(110.0f, 110.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(100.0f, 100.0f));
        IM_CHECK_EQ(io.MouseDown[0], true);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(110.0f, 110.0f));
        IM_CHECK_EQ(io.MouseDown[0], true);
        io.AddMouseButtonEvent(0, false);
        ctx->Yield();

        // MouseButton down 0 | up 0 -> 2 frames
        io.AddMouseButtonEvent(0, true);
        io.AddMouseButtonEvent(0, false);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[0], true);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[0], false);
        ctx->Yield();

        // MouseButton down 0, down 1, up 0, up 1 -> 2 frames
        io.AddMouseButtonEvent(0, true);
        io.AddMouseButtonEvent(1, true);
        io.AddMouseButtonEvent(0, false);
        io.AddMouseButtonEvent(1, false);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[0], true);
        IM_CHECK_EQ(io.MouseDown[1], true);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[0], false);
        IM_CHECK_EQ(io.MouseDown[1], false);
        ctx->Yield();

        // MouseButton down 0 | up 0 | down 0 -> 3 frames
        io.AddMouseButtonEvent(0, true);
        io.AddMouseButtonEvent(0, false);
        io.AddMouseButtonEvent(0, true);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[0], true);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[0], false);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[0], true);
        io.AddMouseButtonEvent(0, false);
        ctx->Yield();

        // MouseButton double-click -> 4 frames
        ctx->SleepNoSkip(1.0f, 1.0f); // Ensure previous clicks are not counted
        io.AddMouseButtonEvent(0, true);
        io.AddMouseButtonEvent(0, false);
        io.AddMouseButtonEvent(0, true);
        io.AddMouseButtonEvent(0, false);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[0], true);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[0], false);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[0], true);
        IM_CHECK(ImGui::IsMouseDoubleClicked(0));
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[0], false);

#if IMGUI_VERSION_NUM >= 18722
        // MousePos | MouseWheel -> 2 frame
        io.AddMousePosEvent(100.0f, 100.0f);
        io.AddMouseWheelEvent(0.0f, +1.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(100.0f, 100.0f));
        IM_CHECK_EQ(io.MouseWheel, 0.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseWheel, 1.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseWheel, 0.0f);

        // MouseWheel, MouseWheel -> 1 frame
        io.AddMouseWheelEvent(0.0f, +1.0f);
        io.AddMouseWheelEvent(0.0f, +1.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseWheel, 2.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseWheel, 0.0f);
#else
        // MousePos, MouseWheel -> 1 frame
        io.AddMousePosEvent(100.0f, 100.0f);
        io.AddMouseWheelEvent(0.0f, +1.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(100.0f, 100.0f));
        IM_CHECK_EQ(io.MouseWheel, 1.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseWheel, 0.0f);
#endif

        // MouseWheel | MousePos -> 2 frames
        io.AddMouseWheelEvent(0.0f, +2.0f);
        io.AddMousePosEvent(110.0f, 110.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(100.0f, 100.0f));
        IM_CHECK_EQ(io.MouseWheel, 2.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(110.0f, 110.0f));
        IM_CHECK_EQ(io.MouseWheel, 0.0f);
        ctx->Yield();

        // MouseWheel | MouseButton -> 2 frames
        io.AddMouseWheelEvent(0.0f, +2.0f);
        io.AddMouseButtonEvent(1, true);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseWheel, 2.0f);
        IM_CHECK_EQ(io.MouseDown[1], false);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseWheel, 0.0f);
        IM_CHECK_EQ(io.MouseDown[1], true);
        io.AddMouseButtonEvent(1, false);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[1], false);

        // MouseButton | MouseWheel -> 2 frames
        io.AddMouseButtonEvent(1, true);
        io.AddMouseWheelEvent(0.0f, +3.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[1], true);
        IM_CHECK_EQ(io.MouseWheel, 0.0f);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseWheel, 3.0f);
        IM_CHECK_EQ(io.MouseDown[1], true);
        ctx->Yield();
        io.AddMouseButtonEvent(1, false);
        ctx->Yield();
        IM_CHECK_EQ(io.MouseDown[1], false);

        // MousePos, Key -> 1 frame
        io.AddMousePosEvent(120.0f, 120.0f);
        io.AddKeyEvent(ImGuiKey_F, true);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(120.0f, 120.0f));
        IM_CHECK_EQ(ImGui::IsKeyPressed(ImGuiKey_F), true);
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyPressed(ImGuiKey_F), false);
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_F), true);
        io.AddKeyEvent(ImGuiKey_F, false);
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_F), false);

        // Key | MousePos -> 2 frames
        io.AddKeyEvent(ImGuiKey_G, true);
        io.AddMousePosEvent(130.0f, 130.0f);
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyPressed(ImGuiKey_G), true);
        IM_CHECK_NE(io.MousePos, ImVec2(130.0f, 130.0f));
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyPressed(ImGuiKey_G), false);
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_G), true);
        IM_CHECK_EQ(io.MousePos, ImVec2(130.0f, 130.0f));
        io.AddKeyEvent(ImGuiKey_G, false);
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_G), false);

        // Key Down | Key Up | Key Down -> 3 frames
        io.AddKeyEvent(ImGuiKey_H, true);
        io.AddKeyEvent(ImGuiKey_H, false);
        io.AddKeyEvent(ImGuiKey_H, true);
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_H), true);
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_H), false);
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_H), true);
        io.AddKeyEvent(ImGuiKey_H, false);
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_H), false);

        // Key down, Key down (other) -> 1 frame
        io.AddKeyEvent(ImGuiKey_I, true);
        io.AddKeyEvent(ImGuiKey_J, true);
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_I), true);
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_J), true);
        io.AddKeyEvent(ImGuiKey_I, false);
        io.AddKeyEvent(ImGuiKey_J, false);
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_I), false);
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_J), false);

        // Char -> 1 frame
        ImGui::ClearActiveID();
        io.AddInputCharacter('L');
        ctx->Yield();
        IM_CHECK(io.InputQueueCharacters.Size == 1 && io.InputQueueCharacters[0] == 'L');
        ctx->Yield();
        IM_CHECK(io.InputQueueCharacters.Size == 0);

#if IMGUI_VERSION_NUM >= 18709
        const int INPUT_TEXT_STEPS = 2;
#else
        const int INPUT_TEXT_STEPS = 1;
#endif
        for (int step = 0; step < INPUT_TEXT_STEPS; step++)
        {
            vars.Bool1 = (step == 0); // Simulate activated InputText()
            ctx->Yield();

            // Key down | Char -> 2 frames when InputText() is active
            // Key down, Char -> 1 frames otherwise
            io.AddKeyEvent(ImGuiKey_K, true);
            io.AddInputCharacter('L');
            ctx->Yield();
            IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_K), true);
            if (step == 0)
            {
                IM_CHECK(io.InputQueueCharacters.Size == 0);
                ctx->Yield();
            }
            IM_CHECK(io.InputQueueCharacters.Size == 1 && io.InputQueueCharacters[0] == 'L');
            io.AddKeyEvent(ImGuiKey_K, false);
            ctx->Yield();
            IM_CHECK(io.InputQueueCharacters.Size == 0);

            // Char | Key -> 2 frames when InputText() is active
            // Char, Key -> 1 frame otherwise
            io.AddInputCharacter('L');
            io.AddKeyEvent(ImGuiKey_K, true);
            ctx->Yield();
            IM_CHECK(io.InputQueueCharacters.Size == 1 && io.InputQueueCharacters[0] == 'L');
            if (step == 0)
            {
                IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_K), false);
                ctx->Yield();
                IM_CHECK(io.InputQueueCharacters.Size == 0);
            }
            IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_K), true);
            ctx->Yield();
            io.AddKeyEvent(ImGuiKey_K, false);
            ctx->Yield();
        }

        // Key, KeyMods -> 1 frame
        IM_CHECK(io.KeyMods == ImGuiMod_None);
        io.AddKeyEvent(ImGuiKey_K, true);
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        io.AddKeyEvent(ImGuiMod_Shift, true);
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_K), true);
        IM_CHECK(io.KeyMods == (ImGuiMod_Ctrl | ImGuiMod_Shift));
        io.AddKeyEvent(ImGuiKey_K, false);
        io.AddKeyEvent(ImGuiMod_Ctrl, false);
        io.AddKeyEvent(ImGuiMod_Shift, false);
        ctx->Yield();

        // KeyMods, Key -> 1 frame
        IM_CHECK(io.KeyMods == ImGuiMod_None);
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        io.AddKeyEvent(ImGuiMod_Shift, true);
        io.AddKeyEvent(ImGuiKey_K, true);
        ctx->Yield();
        IM_CHECK_EQ(ImGui::IsKeyDown(ImGuiKey_K), true);
        IM_CHECK(io.KeyMods == (ImGuiMod_Ctrl | ImGuiMod_Shift));
        io.AddKeyEvent(ImGuiKey_K, false);
        io.AddKeyEvent(ImGuiMod_Ctrl, false);
        io.AddKeyEvent(ImGuiMod_Shift, false);
        ctx->Yield();

        // KeyMods | KeyMods (same) -> 2 frames
        IM_CHECK(io.KeyMods == ImGuiMod_None);
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        io.AddKeyEvent(ImGuiMod_Ctrl, false);
        ctx->Yield();
        IM_CHECK(io.KeyMods == ImGuiMod_Ctrl);
        ctx->Yield();
        IM_CHECK(io.KeyMods == ImGuiMod_None);

        // KeyMods, KeyMods (different) -> 1 frame
        IM_CHECK(io.KeyMods == ImGuiMod_None);
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        io.AddKeyEvent(ImGuiMod_Shift, true);
        io.AddKeyEvent(ImGuiMod_Ctrl, false);
        io.AddKeyEvent(ImGuiMod_Shift, false);
        ctx->Yield();
        IM_CHECK(io.KeyMods == (ImGuiMod_Ctrl | ImGuiMod_Shift));
        ctx->Yield();
        IM_CHECK(io.KeyMods == ImGuiMod_None);

        // MousePos, KeyMods -> 1 frame
        io.AddMousePosEvent(200.0f, 200.0f);
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(200.0f, 200.0f));
        IM_CHECK(io.KeyMods == ImGuiMod_Ctrl);
        io.AddKeyEvent(ImGuiMod_Ctrl, false);
        ctx->Yield();
        IM_CHECK(io.InputQueueCharacters.Size == 0);
        IM_CHECK(io.KeyMods == ImGuiMod_None);

        // KeyMods | MousePos -> 2 frames
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        io.AddMousePosEvent(210.0f, 210.0f);
        io.AddKeyEvent(ImGuiMod_Ctrl, false);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(200.0f, 200.0f));
        IM_CHECK(io.KeyMods == ImGuiMod_Ctrl);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(210.0f, 210.0f));
        IM_CHECK(io.KeyMods == ImGuiMod_None);

        // MousePos | Char -> 2 frames
        io.AddMousePosEvent(220.0f, 220.0f);
        io.AddInputCharacter('B');
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(220.0f, 220.0f));
        IM_CHECK(io.InputQueueCharacters.Size == 0);
        ctx->Yield();
        IM_CHECK(io.InputQueueCharacters.Size == 1 && io.InputQueueCharacters[0] == 'B');
        ctx->Yield();
        IM_CHECK(io.InputQueueCharacters.Size == 0);
    };

    // ## Test IO with multiple-context (#6199, #6256)
#if IMGUI_VERSION_NUM >= 18943
    t = IM_REGISTER_TEST(e, "misc", "inputs_io_inputqueue_multi_context");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext* c1 = ImGui::GetCurrentContext();
        ImGuiContext* c2 = ImGui::CreateContext();
        ImGuiContext* c3 = ImGui::CreateContext();

        IM_CHECK(c1->InputEventsQueue.Size == 0 && c2->InputEventsQueue.Size == 0 && c3->InputEventsQueue.Size == 0);
        ImGui::SetCurrentContext(NULL);
        c1->IO.AddKeyEvent(ImGuiKey_1, true);
        c2->IO.AddKeyEvent(ImGuiKey_2, true);
        c2->IO.AddKeyEvent(ImGuiKey_2, false);
        c3->IO.AddKeyEvent(ImGuiKey_3, true);

        IM_CHECK(c1->InputEventsQueue.Size == 1 && c2->InputEventsQueue.Size == 2 && c3->InputEventsQueue.Size == 1);
        ImGui::DestroyContext(c3);
        ImGui::DestroyContext(c2);
        ImGui::SetCurrentContext(c1);
    };
#endif

    // ## Test input queue filtering of duplicates (#5599)
#if IMGUI_VERSION_NUM >= 18828
    t = IM_REGISTER_TEST(e, "inputs", "inputs_io_inputqueue_filtering");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiIO& io = g.IO;

        ctx->RunFlags |= ImGuiTestRunFlags_EnableRawInputs; // Disable TestEngine submitting inputs events
        ctx->Yield();

        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        ctx->Yield();
        IM_CHECK(g.InputEventsQueue.Size == 0);

        io.AddKeyEvent(ImGuiKey_Space, true);
        io.AddMousePosEvent(100.0f, 100.0f);
        io.AddMouseButtonEvent(1, true);
        io.AddMouseWheelEvent(0.0f, 1.0f);
        IM_CHECK(g.InputEventsQueue.Size == 4);

        io.AddMouseButtonEvent(1, true);
        io.AddMousePosEvent(100.0f, 100.0f);
        io.AddKeyEvent(ImGuiKey_Space, true);
        io.AddMouseWheelEvent(0.0f, 0.0f);
        IM_CHECK(g.InputEventsQueue.Size == 4);

        ctx->Yield(4);
        IM_CHECK(g.InputEventsQueue.Size == 0);
        IM_CHECK(io.AppFocusLost == false);
        io.AddFocusEvent(true);
        io.AddFocusEvent(true);
        IM_CHECK(g.InputEventsQueue.Size == 0);
    };
#endif

    // ## Test ImGuiMod_Shortcut redirect (#5923)
#if IMGUI_VERSION_NUM >= 18912
    t = IM_REGISTER_TEST(e, "inputs", "inputs_io_mod_shortcut");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GenericVars;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Shortcut+L count: %d", vars.Count);
        ImGui::Text("io.KeyMods = 0x%04X", g.IO.KeyMods);

        if (ImGui::Shortcut(ImGuiMod_Shortcut | ImGuiKey_L))
        {
            // Verify whatever Shortcut does with routing
            vars.Count++;
            IM_CHECK(ImGui::IsKeyDown(ImGuiMod_Shortcut));
            if (g.IO.ConfigMacOSXBehaviors)
            {
                // Verify IsKeyPressed() redirection + merged io.KeyMods
                IM_CHECK(ImGui::IsKeyDown(ImGuiMod_Super) == true);
                IM_CHECK(ImGui::IsKeyDown(ImGuiMod_Ctrl) == false);
                IM_CHECK(g.IO.KeyMods == (ImGuiMod_Super));// | ImGuiMod_Shortcut));
            }
            else
            {
                IM_CHECK(ImGui::IsKeyDown(ImGuiMod_Ctrl) == true);
                IM_CHECK(ImGui::IsKeyDown(ImGuiMod_Super) == false);
                IM_CHECK(g.IO.KeyMods == (ImGuiMod_Ctrl));// | ImGuiMod_Shortcut));
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ctx->SetRef("Test Window");
        ctx->WindowFocus("");

        ctx->KeyPress(ImGuiMod_Shortcut | ImGuiKey_L);
        IM_CHECK_EQ(vars.Count, 1);

        vars.Count = 0;
        if (ctx->UiContext->IO.ConfigMacOSXBehaviors)
        {
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_L);
            IM_CHECK_EQ(vars.Count, 0);
            ctx->KeyPress(ImGuiMod_Super | ImGuiKey_L);
            IM_CHECK_EQ(vars.Count, 1);
        }
        else
        {
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_L);
            IM_CHECK_EQ(vars.Count, 1);
            ctx->KeyPress(ImGuiMod_Super | ImGuiKey_L);
            IM_CHECK_EQ(vars.Count, 1);
        }
    };
#endif

#if IMGUI_VERSION_NUM >= 18837
    // ## Test SetKeyOwner(), TestKeyOwner()
    t = IM_REGISTER_TEST(e, "inputs", "inputs_owner_basic_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            ImGui::MenuItem("MenuItem");
            ImGui::EndMenuBar();
        }
        ImGui::Button("Button Up");
        ImGui::Checkbox("Steal ImGuiKey_Home", &vars.Bool1);
        ImGui::ColorButton("hello1", ImVec4(0.4f, 0.4f, 0.8f, 1.0f), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(128, 128));
        if (vars.Bool1)
            ImGui::SetKeyOwner(ImGuiKey_Home, ImGui::GetItemID());
        ImGui::Button("Button Down");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Test Window");
        IM_CHECK_EQ(ImGui::TestKeyOwner(ImGuiKey_Home, ImGuiKeyOwner_None), true);
        ctx->ItemCheck("Steal ImGuiKey_Home");

        IM_CHECK_EQ(ImGui::TestKeyOwner(ImGuiKey_Home, ImGuiKeyOwner_None), false);
        IM_CHECK_EQ(ImGui::TestKeyOwner(ImGuiKey_Home, ctx->GetID("hello1")), true);
        IM_CHECK_EQ(ImGui::TestKeyOwner(ImGuiKey_End, ctx->GetID("hello1")), true);
        IM_CHECK_EQ(ImGui::GetKeyOwnerData(&g, ImGuiKey_End)->OwnerCurr, ImGuiKeyOwner_None);
        ctx->KeyPress(ImGuiKey_End);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Button Down"));
        ctx->KeyPress(ImGuiKey_Home);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Button Down"));

        ctx->ItemUncheck("Steal ImGuiKey_Home");
        ctx->KeyPress(ImGuiKey_Home);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Button Up"));
    };

    // ## Test SetKeyOwner(), TestKeyOwner() same frame behavior
    t = IM_REGISTER_TEST(e, "inputs", "inputs_owner_basic_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
        ImGui::Button("Button 1");
        IM_CHECK_EQ_NO_RET(ImGui::TestKeyOwner(ImGuiKey_A, ImGuiKeyOwner_None), ctx->IsFirstGuiFrame() ? true : false);
        IM_CHECK_EQ_NO_RET(ImGui::TestKeyOwner(ImGuiKey_A, ImGui::GetItemID()), true);
        ImGui::SetKeyOwner(ImGuiKey_A, ImGui::GetItemID());

        ImGui::Button("Button 2");
        if (!ctx->IsFirstGuiFrame()) // Can't check this on first frame since TestKeyOwner() is not checking OwnerNext
        {
            IM_CHECK_EQ_NO_RET(ImGui::TestKeyOwner(ImGuiKey_A, ImGuiKeyOwner_None), false);
            IM_CHECK_EQ_NO_RET(ImGui::TestKeyOwner(ImGuiKey_A, ImGui::GetItemID()), false);
        }

        ImGui::End();
    };

    // ## Test release event of a Selectable() in a popup from being caught by parent window
    t = IM_REGISTER_TEST(e, "inputs", "inputs_owner_popup_overlap");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Open Popup"))
            ImGui::OpenPopup("Popup");

        ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
        if (ImGui::BeginPopup("Popup"))
        {
            // Will take ownership on mouse down aka frame of double-click
            if (ImGui::Selectable("Front", false, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_AllowDoubleClick, ImVec2(200, 200)))
                if (ImGui::IsMouseDoubleClicked(0))
                    ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // NB: This does not rely on SetKeyOwner() applying in same frame (since using _SelectOnRelease)
        // Same frame discard would happen because of ActiveId/HoveredId being set.
        if (ImGui::Selectable("Back", false, ImGuiSelectableFlags_SelectOnRelease, ImVec2(200, 200)))
            vars.Count++;
        ImGui::Text("Counter %d", vars.Count);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->ItemClick("Open Popup");
        ctx->SetRef("//$FOCUSED");
        ctx->ItemDoubleClick("Front");
        IM_CHECK(vars.Count == 0);
    };

    // ## Test overriding and next frame behavior
    t = IM_REGISTER_TEST(e, "inputs", "inputs_owner_override");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);

        ImGui::Button("Button 1");
        if (vars.Step >= 1)
            ImGui::SetKeyOwner(ImGuiKey_Space, ImGui::GetItemID());

        const ImGuiKeyOwnerData* owner_data = ImGui::GetKeyOwnerData(&g, ImGuiKey_Space);
        if (vars.Bool1)
        {
            if (vars.Step == 0)
                IM_CHECK_EQ(owner_data->OwnerCurr, ImGuiKeyOwner_None);
            else if (vars.Step == 1 || vars.Step == 2)
                IM_CHECK_EQ(owner_data->OwnerCurr, ctx->GetID("Button 1"));
            // [2022-09-20]: changed SetKeyOwner() to alter OwnerCurr as well
            //else if (vars.Step == 2)
            //    IM_CHECK_EQ(owner_data->OwnerCurr, ctx->GetID("Button 2"));
            if (vars.Step >= 1)
                IM_CHECK_EQ(owner_data->OwnerNext, ctx->GetID("Button 1"));
        }

        ImGui::Button("Button 2");
        if (vars.Step >= 2)
            ImGui::SetKeyOwner(ImGuiKey_Space, ImGui::GetItemID());
        if (vars.Bool1 && vars.Step >= 2)
            IM_CHECK_EQ(owner_data->OwnerNext, ctx->GetID("Button 2"));

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        for (int n = 0; n < 3; n++)
        {
            ctx->LogInfo("STEP %d", n);
            vars.Step = n;
            vars.Bool1 = false; // Let run without checking
            ctx->Yield(2);
            vars.Bool1 = true; // Start checking
            ctx->Yield(2);
        }
    };

    // ## Test ImGuiInputFlags_LockThisFrame for stealing inputs EVEN for calls that not requesting ownership test
    t = IM_REGISTER_TEST(e, "inputs", "inputs_owner_lock_this_frame");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Button("Button 1");
        ImGuiID owner_id = ImGui::GetItemID();

        if (ImGui::IsKeyPressed(ImGuiKey_A))
            vars.IntArray[0]++;
        ImGui::SetKeyOwner(ImGuiKey_A, (vars.Step == 0) ? owner_id : ImGuiKeyOwner_Any, ImGuiInputFlags_LockThisFrame);
        if (ImGui::IsKeyPressed(ImGuiKey_A))
            vars.IntArray[1]++;
        if (ImGui::IsKeyPressed(ImGuiKey_A, owner_id ^ 0x42424242))
            vars.IntArray[2]++;
        if (ImGui::IsKeyPressed(ImGuiKey_A, owner_id))
            vars.IntArray[3]++;
        ImGui::Text("%d %d %d", vars.IntArray[0], vars.IntArray[1], vars.IntArray[2]);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        for (int step = 0; step < 2; step++)
        {
            vars.Clear();
            vars.Step = step;
            ctx->Yield();
            ctx->KeyPress(ImGuiKey_A);
            ctx->KeyPress(ImGuiKey_A);
            IM_CHECK_EQ_NO_RET(vars.IntArray[0], 2);
            IM_CHECK_EQ_NO_RET(vars.IntArray[1], 0);
            IM_CHECK_EQ_NO_RET(vars.IntArray[2], 0);
            IM_CHECK_EQ_NO_RET(vars.IntArray[3], (vars.Step == 0) ? 2 : 0);
        }
    };

    // ## Test ImGuiInputFlags_LockUntilRelease
    t = IM_REGISTER_TEST(e, "inputs", "inputs_owner_lock_until_release");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Button("Button 1");
        ImGuiID owner_id = ImGui::GetItemID();

        if (ImGui::IsKeyPressed(ImGuiKey_A))
        {
            vars.IntArray[0]++;
            ImGui::SetKeyOwner(ImGuiKey_A, (vars.Step == 0) ? owner_id : ImGuiKeyOwner_Any, ImGuiInputFlags_LockUntilRelease);
        }
        if (ImGui::IsKeyDown(ImGuiKey_A))
            vars.IntArray[1]++;
        if (ImGui::IsKeyDown(ImGuiKey_A, owner_id ^ 0x42424242))
            vars.IntArray[2]++;
        if (ImGui::IsKeyDown(ImGuiKey_A, owner_id))
            vars.IntArray[3]++;
        ImGui::Text("%d %d %d", vars.IntArray[0], vars.IntArray[1], vars.IntArray[2]);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        for (int step = 0; step < 2; step++)
        {
            vars.Clear();
            vars.Step = step;
            ctx->Yield();
            ctx->KeyPress(ImGuiKey_A);
            IM_CHECK_EQ_NO_RET(vars.IntArray[0], 1);
            IM_CHECK_EQ_NO_RET(vars.IntArray[1], 0);
            IM_CHECK_EQ_NO_RET(vars.IntArray[2], 0);
            IM_CHECK_EQ_NO_RET(vars.IntArray[3], (vars.Step == 0) ? 1 : 0);
            ctx->KeyHold(ImGuiKey_A, 1.0f);
            IM_CHECK_EQ_NO_RET(vars.IntArray[0], 2);
            IM_CHECK_EQ_NO_RET(vars.IntArray[1], 0);
            IM_CHECK_EQ_NO_RET(vars.IntArray[2], 0);
            if (vars.Step == 0)
                IM_CHECK_GT_NO_RET(vars.IntArray[3], 2);
            else
                IM_CHECK_EQ_NO_RET(vars.IntArray[3], 0);
        }
    };

    // ## General test claiming Alt to prevent menu opening
    t = IM_REGISTER_TEST(e, "inputs", "inputs_owner_mod_alt");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        //auto& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            ImGui::MenuItem("MenuItem");
            ImGui::EndMenuBar();
        }
        ImGui::Button("Button1");
        ImGui::Button("Button2");
        ImGui::SetItemKeyOwner(ImGuiMod_Alt);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Test Window");

        // Test Alt mod being captured
        ctx->MouseMove("Button1");
        ctx->KeyPress(ImGuiMod_Alt);
        IM_CHECK(g.NavLayer == ImGuiNavLayer_Menu);
        ctx->KeyPress(ImGuiMod_Alt);
        IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);

        ctx->MouseMove("Button2");
        ctx->KeyPress(ImGuiMod_Alt);
        IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);

        // Also test with ImGuiKey_ModAlt (does specs enable allows that?)
        ctx->MouseMove("Button1");
        ctx->KeyPress(ImGuiMod_Alt);
        IM_CHECK(g.NavLayer == ImGuiNavLayer_Menu);
        ctx->KeyPress(ImGuiMod_Alt);
        IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);

        ctx->MouseMove("Button2");
        ctx->KeyPress(ImGuiMod_Alt);
        IM_CHECK(g.NavLayer == ImGuiNavLayer_Main);
    };

    // ## Test special ButtonBehavior() flags
    t = IM_REGISTER_TEST(e, "inputs", "inputs_owner_button_behavior");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);

        ImVec2 sz = ImVec2(400, 0);

        vars.IntArray[0] += (int)ImGui::ButtonEx("Button _None##A1", sz);
        ImGui::SameLine(); ImGui::Text("%d", vars.IntArray[0]);
        vars.IntArray[1] += (int)ImGui::ButtonEx("Button _PressedOnRelease##A2", sz, ImGuiButtonFlags_PressedOnRelease);
        ImGui::SameLine(); ImGui::Text("%d", vars.IntArray[1]);
        vars.IntArray[2] += (int)ImGui::ButtonEx("Button _None##A3", sz); // testing order dependency
        ImGui::SameLine(); ImGui::Text("%d", vars.IntArray[2]);
        ImGui::Spacing();

        vars.IntArray[3] += (int)ImGui::ButtonEx("Button _NoSetKeyOwner##B1", sz, ImGuiButtonFlags_NoSetKeyOwner);
        ImGui::SameLine(); ImGui::Text("%d", vars.IntArray[3]);
        vars.IntArray[4] += (int)ImGui::ButtonEx("Button _PressedOnRelease##B2", sz, ImGuiButtonFlags_PressedOnRelease);
        ImGui::SameLine(); ImGui::Text("%d", vars.IntArray[4]);
        vars.IntArray[5] += (int)ImGui::ButtonEx("Button _NoSetKeyOwner##B3", sz, ImGuiButtonFlags_NoSetKeyOwner);
        ImGui::SameLine(); ImGui::Text("%d", vars.IntArray[5]);
        ImGui::Spacing();

        vars.IntArray[6] += (int)ImGui::ButtonEx("Button _None##C1", sz);
        ImGui::SameLine(); ImGui::Text("%d", vars.IntArray[6]);
        vars.IntArray[7] += (int)ImGui::ButtonEx("Button _PressedOnRelease | _NoTestKeyOwner##C2", sz, ImGuiButtonFlags_PressedOnRelease | ImGuiButtonFlags_NoTestKeyOwner);
        ImGui::SameLine(); ImGui::Text("%d", vars.IntArray[7]);
        vars.IntArray[8] += (int)ImGui::ButtonEx("Button _None##C3", sz);
        ImGui::SameLine(); ImGui::Text("%d", vars.IntArray[8]);
        ImGui::Spacing();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->ItemClick("Button _None##A1");
        IM_CHECK_EQ(vars.IntArray[0], 1);
        vars.Clear();

        ctx->ItemDragAndDrop("Button _None##A1", "Button _PressedOnRelease##A2");
        IM_CHECK_EQ(vars.IntArray[0], 0);
        IM_CHECK_EQ(vars.IntArray[1], 0);
        IM_CHECK_EQ(vars.IntArray[2], 0);

        ctx->ItemDragAndDrop("Button _None##A3", "Button _PressedOnRelease##A2");
        IM_CHECK_EQ(vars.IntArray[0], 0);
        IM_CHECK_EQ(vars.IntArray[1], 0);
        IM_CHECK_EQ(vars.IntArray[2], 0);

        ctx->ItemDragAndDrop("Button _NoSetKeyOwner##B1", "Button _PressedOnRelease##B2");
        IM_CHECK_EQ(vars.IntArray[3], 0);
        IM_CHECK_EQ(vars.IntArray[4], 1);
        IM_CHECK_EQ(vars.IntArray[5], 0);
        vars.Clear();

        ctx->ItemDragAndDrop("Button _NoSetKeyOwner##B3", "Button _PressedOnRelease##B2");
        IM_CHECK_EQ(vars.IntArray[3], 0);
        IM_CHECK_EQ(vars.IntArray[4], IMGUI_BROKEN_TESTS ? 1 : 0); // FIXME: Order matters because ActiveId is taken (not because of Owner)
        IM_CHECK_EQ(vars.IntArray[5], 0);

        ctx->ItemDragAndDrop("Button _None##C1", "Button _PressedOnRelease | _NoTestKeyOwner##C2");
        IM_CHECK_EQ(vars.IntArray[6], 0);
        IM_CHECK_EQ(vars.IntArray[7], 1);
        IM_CHECK_EQ(vars.IntArray[8], 0);
        vars.Clear();

        ctx->ItemDragAndDrop("Button _None##C3", "Button _PressedOnRelease | _NoTestKeyOwner##C2");
        IM_CHECK_EQ(vars.IntArray[6], 0);
        IM_CHECK_EQ(vars.IntArray[7], IMGUI_BROKEN_TESTS ? 1 : 0); // FIXME: Order matters because ActiveId is taken (not because of Owner)
        IM_CHECK_EQ(vars.IntArray[8], 0);
    };
    #endif

    t = IM_REGISTER_TEST(e, "issues", "issues_0024");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // Drawing
        static bool show_app_overlay = false;
        static double end_overlay;

        const ImGuiViewport* const viewport = ImGui::GetMainViewport();

        // Drawing
        if (show_app_overlay)
        {
            ImGui::SetNextWindowBgAlpha(0.35f);
            ImGui::SetNextWindowPos({ viewport->WorkPos.x + viewport->WorkSize.x - 10.f, viewport->WorkPos.y + 30.f }, ImGuiCond_Always, { 1.f, 0.f });
            if (ImGui::Begin("Overlay", &show_app_overlay, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
            {

                const double current_time = ImGui::GetTime();
                if (ImGui::IsWindowFocused() || end_overlay < current_time)
                    show_app_overlay = false;
                ImGui::TextColored(ImVec4{ 1.f, 0.f, 0.f, 1.f }, "%f", end_overlay - current_time);
            } ImGui::End();
        }

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        if (ImGui::Begin("QAP", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus))
        {
            if (ImGui::Button("Launch"))
            {
                show_app_overlay = true;
                end_overlay = ImGui::GetTime() + 5.;
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->ItemClick("//QAP/Launch");
        ctx->MouseMove("//Overlay");
        ctx->MouseClick();
    };

#if IMGUI_VERSION_NUM > 18902
    // ## Test SetActiveIdUsingAllKeyboardKeys() (via window dragging) dropping modifiers and blocking input-owner-unaware code from accessing keys (#5888)
    t = IM_REGISTER_TEST(e, "inputs", "inputs_owner_all_keys_w_owner_unaware");
    struct InputsOwnerAllkeysTestVars { bool CtrlDown; bool ShiftDown; bool ADown; bool LeftMouseDown; };
    t->SetVarsDataType<InputsOwnerAllkeysTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputsOwnerAllkeysTestVars>();
        ImGui::SetNextWindowSize({ 100.f, 200.f });
        ImGui::Begin("Test", nullptr, ImGuiWindowFlags_NoSavedSettings);
        vars.CtrlDown = ImGui::IsKeyDown(ImGuiMod_Ctrl);
        vars.ShiftDown = ImGui::IsKeyDown(ImGuiMod_Shift);
        vars.ADown = ImGui::IsKeyDown(ImGuiKey_A);
        vars.LeftMouseDown = ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Left);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputsOwnerAllkeysTestVars>();
        ImGuiIO& io = ImGui::GetIO();

        // Start dragging the window
        // This is a slightly minified version of ImGuiTestContext::WindowMove(), we can't use it since we want the mouse to stay down
        ImGuiWindow* window = ctx->GetWindowByRef("Test");
        IM_CHECK_NE(window, nullptr);
        ImGuiTestRef ref = window->ID;
        ctx->WindowFocus(ref);
        ctx->WindowCollapse(ref, false);
        ctx->MouseSetViewport(window);
        ctx->MouseMoveToPos(ctx->GetWindowTitlebarPoint(ref));

        // Hold shift before drag to disable docking (and test shift)
#ifdef IMGUI_HAS_DOCK
        IM_CHECK(!io.ConfigDockingWithShift);
#endif
        ctx->KeyDown(ImGuiMod_Shift);

        ImVec2 old_pos = window->Pos;
        ctx->MouseDown(ImGuiMouseButton_Left);
        ctx->MouseMoveToPos(io.MousePos + ImVec2(10.f, 0.f));
        IM_CHECK_NE(old_pos, window->Pos);

        // Hold keys and ensure they're detected along with shift and left mouse (both held earlier)
        ctx->KeyDown(ImGuiMod_Ctrl);
        ctx->KeyDown(ImGuiKey_A);
        vars = { };
        ctx->Yield();
        IM_CHECK(vars.CtrlDown);
        IM_CHECK(vars.ShiftDown);
        IM_CHECK(vars.ADown);
        IM_CHECK(vars.LeftMouseDown);

        ctx->KeyUp(ImGuiKey_A);
        ctx->KeyUp(ImGuiMod_Ctrl);
        ctx->MouseUp(ImGuiMouseButton_Left);
        ctx->KeyUp(ImGuiMod_Shift);
    };
#endif

#if IMGUI_VERSION_NUM >= 18837
    t = IM_REGISTER_TEST(e, "inputs", "inputs_routing_1");
    struct InputRoutingVars
    {
        bool            IsRouting[26] = {};
        int             PressedCount[26] = {};
        ImGuiKeyChord   KeyChord = ImGuiMod_Shortcut | ImGuiKey_A;
        char            Str[64] = "Hello";

        void            Clear()
        {
            memset(IsRouting, 0, sizeof(IsRouting));
            memset(PressedCount, 0, sizeof(PressedCount));
        }
        void            TestIsRoutingOnly(char c)
        {
            int idx = c - 'A';
            for (int n = 0; n < IM_ARRAYSIZE(IsRouting); n++)
            {
                if (c != 0 && n == idx)
                    IM_CHECK_EQ(IsRouting[n], true);
                else
                    IM_CHECK_SILENT(IsRouting[n] == false);
            }
        }
        void            TestIsPressedOnly(char c)
        {
            int idx = c - 'A';
            for (int n = 0; n < IM_ARRAYSIZE(IsRouting); n++)
            {
                if (c != 0 && n == idx)
                    IM_CHECK_GT(PressedCount[n], 0);
                else
                    IM_CHECK_SILENT(PressedCount[n] == 0);
            }
        }
    };
    t->SetVarsDataType<InputRoutingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputRoutingVars>();

        auto DoRoute = [&vars](char scope_name_c)
        {
            IM_ASSERT(scope_name_c >= 'A' && scope_name_c <= 'Z');
            const int idx = scope_name_c - 'A';
            bool is_routing = ImGui::TestShortcutRouting(vars.KeyChord, 0);// , ImGuiInputFlags_RouteFocused);
            bool shortcut_pressed = ImGui::Shortcut(vars.KeyChord);
            vars.IsRouting[idx] = is_routing;
            vars.PressedCount[idx] += shortcut_pressed ? 1 : 0;
            ImGui::Text("Routing: %d %s", is_routing, shortcut_pressed ? "PRESSED" : "...");
        };
        auto DoRouteForItem = [&vars](char scope_name_c, ImGuiID id)
        {
            IM_ASSERT(scope_name_c >= 'A' && scope_name_c <= 'Z');
            const int idx = scope_name_c - 'A';
            bool is_routing = ImGui::TestShortcutRouting(vars.KeyChord, id);
            bool shortcut_pressed = ImGui::Shortcut(vars.KeyChord, id, ImGuiInputFlags_RouteAlways); // No side-effect
            vars.IsRouting[idx] = is_routing;
            vars.PressedCount[idx] += shortcut_pressed ? 1 : 0;
            ImGui::Text("Routing: %d %s", is_routing, shortcut_pressed ? "PRESSED" : "...");
        };

        ImGui::Begin("WindowA");

        char chord[32];
        ImGui::GetKeyChordName(vars.KeyChord, chord, IM_ARRAYSIZE(chord));
        ImGui::Text("Chord: %s", chord);

        ImGui::Button("WindowA");
        DoRoute('A');

        //if (ImGui::Shortcut(ImGuiMod_Shortcut | ImGuiKey_A))
        //    ImGui::Text("PRESSED 111");

        // TODO: Write a test for that
        //ImGui::Text("Down: %d", ImGui::InputRoutingSubmit2(ImGuiKey_DownArrow));

        ImGui::InputText("InputTextB", vars.Str, IM_ARRAYSIZE(vars.Str));
        DoRouteForItem('B', ImGui::GetItemID());

        ImGui::Button("ButtonC");
        ImGui::BeginChild("ChildD", ImVec2(-FLT_MIN, 100), true);
        ImGui::Button("ChildD");
        ImGui::EndChild();
        ImGui::BeginChild("ChildE", ImVec2(-FLT_MIN, 100), true);
        ImGui::Button("ChildE");
        DoRoute('E');
        ImGui::EndChild();
        if (ImGui::Button("Open Popup"))
            ImGui::OpenPopup("PopupF");

        if (ImGui::BeginPopup("PopupF"))
        {
            ImGui::Button("PopupF");
            DoRoute('F');
            ImGui::InputText("InputTextG", vars.Str, IM_ARRAYSIZE(vars.Str));
            DoRouteForItem('G', ImGui::GetItemID());
            ImGui::EndPopup();
        }

        ImGui::End();

        /*
        ImGui::Begin("WindowH");
        ImGui::Button("WindowH");
        DoRoute('H');
        ImGui::BeginChild("ChildI", ImVec2(-FLT_MIN, 100), true);
        ImGui::Button("ChildI");
        ImGui::EndChild();
        ImGui::End();

        // TODO: NodeTabBarL+DockedWindowM

        ImGui::Begin("WindowN");
        ImGui::Button("WindowM");
        DoRoute('M');

        ImGui::BeginChild("ChildN", ImVec2(-FLT_MIN, 300), true);
        ImGui::Button("ChildN");
        DoRoute('N');

        ImGui::BeginChild("ChildO", ImVec2(-FLT_MIN, 150), true);
        ImGui::Button("ChildO");
        DoRoute('O');
        ImGui::EndChild();

        ImGui::EndChild();
        ImGui::ColorButton("##Color", ImVec4(1, 0, 0, 1));
        ImGui::End();
        */

    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputRoutingVars>();
        ImGuiInputTextState* input_state = NULL;

        ctx->SetRef("WindowA");

        // A
        ctx->ItemClick("WindowA");
        vars.TestIsRoutingOnly('A');
        vars.TestIsPressedOnly(0);
        ctx->KeyPress(ImGuiMod_Shortcut | ImGuiKey_A);
        vars.TestIsPressedOnly('A');
        vars.Clear();

        // B: verify catching CTRL+A
        ctx->ItemClick("InputTextB");
        vars.TestIsRoutingOnly('B');
        vars.TestIsPressedOnly(0);
        input_state = ImGui::GetInputTextState(ctx->GetID("InputTextB"));
        IM_CHECK(input_state != NULL);
        IM_CHECK(input_state->HasSelection() == false);
        ctx->KeyPress(ImGuiMod_Shortcut | ImGuiKey_A);
        vars.TestIsPressedOnly('B');
        input_state = ImGui::GetInputTextState(ctx->GetID("InputTextB"));
        IM_CHECK(input_state != NULL);
        IM_CHECK(input_state->HasSelection() == true);
        vars.Clear();

        // B: verify that CTRL+B is caught by A but not B
        vars.KeyChord = ImGuiMod_Shortcut | ImGuiKey_B;
        ctx->Yield(2);
        vars.TestIsRoutingOnly('A');
        ctx->KeyPress(ImGuiMod_Shortcut | ImGuiKey_B);
        vars.TestIsPressedOnly('A');
        vars.KeyChord = ImGuiMod_Shortcut | ImGuiKey_A;
        vars.Clear();

        // D: Focus child which does no polling/routing: parent A gets it: results are same as A
        ctx->ItemClick("**/ChildD");
        vars.TestIsRoutingOnly('A');
        vars.TestIsPressedOnly(0);
        ctx->KeyPress(ImGuiMod_Shortcut | ImGuiKey_A);
        vars.TestIsPressedOnly('A');
        vars.Clear();

        // E: Focus child which does its polling/routing
        ctx->ItemClick("**/ChildE");
        vars.TestIsRoutingOnly('E');
        vars.TestIsPressedOnly(0);
        ctx->KeyPress(ImGuiMod_Shortcut | ImGuiKey_A);
        vars.TestIsPressedOnly('E');
        vars.Clear();

        // F: Open Popup
        ctx->ItemClick("Open Popup");
        ctx->Yield(); // Appearing route requires a frame to establish
        vars.TestIsRoutingOnly('F');
        vars.TestIsPressedOnly(0);
        ctx->KeyPress(ImGuiMod_Shortcut | ImGuiKey_A);
        vars.TestIsPressedOnly('F');
        vars.Clear();

        // G: verify popup's input text catching CTRL+A
        ctx->ItemClick("**/InputTextG");
        vars.TestIsRoutingOnly('G');
        vars.TestIsPressedOnly(0);
        input_state = ImGui::GetInputTextState(ImGui::GetActiveID());
        IM_CHECK(input_state != NULL);
        IM_CHECK(input_state->HasSelection() == false);
        ctx->KeyPress(ImGuiMod_Shortcut | ImGuiKey_A);
        vars.TestIsPressedOnly('G');
        input_state = ImGui::GetInputTextState(ImGui::GetActiveID());
        IM_CHECK(input_state != NULL);
        IM_CHECK(input_state->HasSelection() == true);
        vars.Clear();

        // G: verify that CTRL+B is caught by F but not G
        vars.KeyChord = ImGuiMod_Shortcut | ImGuiKey_B;
        ctx->Yield(2);
        vars.TestIsRoutingOnly('F');
        ctx->KeyPress(ImGuiMod_Shortcut | ImGuiKey_B);
        vars.TestIsPressedOnly('F');
        vars.KeyChord = ImGuiMod_Shortcut | ImGuiKey_A;
        vars.Clear();
    };
#endif

#if 0
    t = IM_REGISTER_TEST(e, "inputs", "inputs_routing_basic");
    enum InputFunc { InputFunc_IsKeyPressed = 0, InputFunc_IsShortcutPressed, InputFunc_IsKeyDown, InputFunc_IsKeyReleased };
    struct InputRoutingVars { int Pressed[4][5] = {}; bool EnableDynamicOwner = false; int Func = 2; ImGuiReadFlags InputFlags = 0; };
    t->SetVarsDataType<InputRoutingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputRoutingVars>();
        ImGui::Begin("Input Routing Basic");
        ImGui::CheckboxFlags("ImGuiInputFlags_Repeat", &vars.InputFlags, ImGuiInputFlags_Repeat);
        ImGui::Separator();

        if (ctx->IsGuiFuncOnly())
            memset(&vars.Pressed, 0, sizeof(vars.Pressed));

        for (int window_n = 0; window_n < 3; window_n++)
        {
            ImGui::BeginChild(Str16f("Window%d", window_n).c_str(), ImVec2(200, 150), true);
            ImGui::Text("Window %d 0x%08X\n", window_n, ImGui::IsWindowFocused(), ImGui::GetID(""));
            for (int flag_n = 0; flag_n < 3; flag_n++)
            {
                ImGuiInputFlags flags = ((flag_n == 0) ? ImGuiInputFlags_ServeAll : (flag_n == 1) ? ImGuiInputFlags_ServeFirst : ImGuiInputFlags_ServeLast) | (vars.InputFlags & ImGuiInputFlags_Repeat);
                bool ret = false;
                switch (vars.Func)
                {
                case InputFunc_IsKeyPressed:
                    ret = ImGui::IsKeyPressed(ImGuiKey_B, 0, flags);
                    break;
                case InputFunc_IsShortcutPressed:
                    ret = ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_B, 0, flags);
                    break;
                case InputFunc_IsKeyDown:
                    ret = ImGui::IsKeyDown(ImGuiKey_B, 0, flags);
                    break;
                case InputFunc_IsKeyReleased:
                    ret = ImGui::IsKeyReleased(ImGuiKey_B, 0, flags);
                    break;
                }
                vars.Pressed[window_n][flag_n] += ret ? 1 : 0;
            }
            ImGui::Text("B all %s", vars.Pressed[window_n][0] ? "Pressed" : "");
            ImGui::Text("B first %s", vars.Pressed[window_n][1] ? "Pressed" : "");
            ImGui::Text("B last %s", vars.Pressed[window_n][2] ? "Pressed" : "");
            ImGui::EndChild();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputRoutingVars>();
        for (int variant = 0; variant < 4; variant++)
        {
            ctx->LogInfo("## Step %d", variant);
            memset(&vars.Pressed, 0, sizeof(vars.Pressed));

            vars.Func = (InputFunc)variant;
            vars.InputFlags = false;//(variant >= 4) ? ImGuiInputFlags_Repeat : 0;

            memset(&vars.Pressed, 0, sizeof(vars.Pressed));
            ctx->Yield();

            /*
            if (variant >= 2)
            {
                if (vars.UseShortcut)
                    ctx->KeyHold(ImGuiKey_B | ImGuiMod_Ctrl, ctx->UiContext->IO.KeyRepeatDelay + 0.01f);
                else
                    ctx->KeyHold(ImGuiKey_B, ctx->UiContext->IO.KeyRepeatDelay + 0.01f);
            }
            else
            */
            {
                if (vars.Func == InputFunc_IsShortcutPressed)
                    ctx->KeyPress(ImGuiKey_B | ImGuiMod_Ctrl);
                else
                    ctx->KeyPress(ImGuiKey_B);
            }

            if (vars.InputFlags & ImGuiInputFlags_Repeat)
            {
                IM_CHECK_GT_NO_RET(vars.Pressed[0][0], 1); // All
                IM_CHECK_GT_NO_RET(vars.Pressed[0][1], 1); // First
                IM_CHECK_EQ_NO_RET(vars.Pressed[0][2], 0); // Last
                IM_CHECK_GT_NO_RET(vars.Pressed[1][0], 1); // All
                IM_CHECK_EQ_NO_RET(vars.Pressed[1][1], 0); // First
                IM_CHECK_EQ_NO_RET(vars.Pressed[1][2], 0); // Last
                IM_CHECK_GT_NO_RET(vars.Pressed[2][0], 1); // All
                IM_CHECK_EQ_NO_RET(vars.Pressed[2][1], 0); // First
                IM_CHECK_GT_NO_RET(vars.Pressed[2][2], 1); // Last
            }
            else
            {
                IM_CHECK_EQ_NO_RET(vars.Pressed[0][0], 1); // All
                IM_CHECK_EQ_NO_RET(vars.Pressed[0][1], 1); // First
                IM_CHECK_EQ_NO_RET(vars.Pressed[0][2], 0); // Last
                IM_CHECK_EQ_NO_RET(vars.Pressed[1][0], 1); // All
                IM_CHECK_EQ_NO_RET(vars.Pressed[1][1], 0); // First
                IM_CHECK_EQ_NO_RET(vars.Pressed[1][2], 0); // Last
                IM_CHECK_EQ_NO_RET(vars.Pressed[2][0], 1); // All
                IM_CHECK_EQ_NO_RET(vars.Pressed[2][1], 0); // First
                IM_CHECK_EQ_NO_RET(vars.Pressed[2][2], 1); // Last
            }
        }
    };

    t = IM_REGISTER_TEST(e, "inputs", "inputs_routing_shortcuts");
    t->SetVarsDataType<InputRoutingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputRoutingVars>();
        ImGui::Begin("Input Routing", NULL, ImGuiWindowFlags_NoSavedSettings);
        {
            ImGui::Text("Window 1 (focused=%d)\n(want 1+2+4. catch input without focus)", ImGui::IsWindowFocused());
            ImGui::Button("Button 1"); // Used to focus window
            if (true) // ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows)
            {
                vars.Pressed[0][1] += ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_1, 0, vars.InputFlags);
                vars.Pressed[0][2] += ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_2, 0, vars.InputFlags);
                vars.Pressed[0][4] += ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_4, 0, vars.InputFlags);
            }
            ImGui::Text("[Ctrl+1] %d", vars.Pressed[0][1]); ImGui::SameLine();
            ImGui::Text("[2] %d", vars.Pressed[0][2]); ImGui::SameLine();
            ImGui::TextDisabled("[3] %d", vars.Pressed[0][3]); ImGui::SameLine();
            ImGui::Text("[4] %d", vars.Pressed[0][4]);
        }

        ImGui::BeginChild("Window2", ImVec2(320, 330), true);
        {
            ImGui::Text("Window 2 (focused=%d)\n(want 1+3+4)", ImGui::IsWindowFocused());
            ImGui::Button("Button 2"); // Used to focus window
            if (ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows))
            {
                vars.Pressed[1][1] += ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_1, 0, vars.InputFlags);
                vars.Pressed[1][3] += ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_3, 0, vars.InputFlags);
                vars.Pressed[1][4] += ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_4, 0, vars.InputFlags);
            }
            ImGui::Text("[Ctrl+1] %d", vars.Pressed[1][1]); ImGui::SameLine();
            ImGui::TextDisabled("[2] %d", vars.Pressed[1][2]); ImGui::SameLine();
            ImGui::Text("[3] %d", vars.Pressed[1][3]); ImGui::SameLine();
            ImGui::Text("[4] %d", vars.Pressed[1][4]);
        }

        ImGui::BeginChild("Window3", ImVec2(240, 200), true);
        {
            ImGui::Text("Window 3 (focused=%d)\n(want 2+3)", ImGui::IsWindowFocused());
            ImGui::Button("Button 3"); // Used to focus window
            if (ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows))
            {
                vars.Pressed[2][2] += ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_2, 0, vars.InputFlags);
                vars.Pressed[2][3] += ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_3, 0, vars.InputFlags);
                vars.Pressed[2][4] += ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_4, 0, vars.InputFlags);
            }
            ImGui::TextDisabled("[Ctrl+1] %d", vars.Pressed[2][1]); ImGui::SameLine();
            ImGui::Text("[2] %d", vars.Pressed[2][2]); ImGui::SameLine();
            ImGui::Text("[3] %d", vars.Pressed[2][3]); ImGui::SameLine();
            ImGui::Text("[4] %d", vars.Pressed[2][4]);
        }

        ImGui::Checkbox("Dynamic Owner", &vars.EnableDynamicOwner);
        if (vars.EnableDynamicOwner)
        {
            ImGui::BeginChild("Window4", ImVec2(220, 200), true);
            {
                ImGui::Text("Window 4 (focused=%d)\n(want 2+3)", ImGui::IsWindowFocused());
                ImGui::Button("Button 4"); // Used to focus window
                if (ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows))
                {
                    vars.Pressed[3][2] += ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_2, 0, vars.InputFlags);
                    vars.Pressed[3][3] += ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_3, 0, vars.InputFlags);
                    vars.Pressed[3][4] += ImGui::IsShortcutPressed(ImGuiMod_Ctrl | ImGuiKey_4, 0, vars.InputFlags);
                }
                ImGui::TextDisabled("[Ctrl+1] %d", vars.Pressed[2][1]); ImGui::SameLine();
                ImGui::Text("[2] %d", vars.Pressed[3][2]); ImGui::SameLine();
                ImGui::Text("[3] %d", vars.Pressed[3][3]); ImGui::SameLine();
                ImGui::Text("[4] %d", vars.Pressed[3][4]);
            }
            ImGui::EndChild();
        }

        ImGui::EndChild();
        ImGui::EndChild();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputRoutingVars>();

        for (int variant = 0; variant < 3; variant++)
        {
            ctx->LogInfo("## Step %d", variant);
            memset(&vars.Pressed, 0, sizeof(vars.Pressed));
            vars.InputFlags = (variant == 0) ? ImGuiInputFlags_ServeAll : (variant == 1) ? ImGuiInputFlags_ServeFirst : ImGuiInputFlags_ServeLast;

            ctx->ItemClick("**/Button 1");
            ctx->KeyPress(ImGuiKey_1 | ImGuiMod_Ctrl, 2);
            ctx->KeyPress(ImGuiKey_2 | ImGuiMod_Ctrl, 2);
            ctx->KeyPress(ImGuiKey_3 | ImGuiMod_Ctrl, 2); // will be ignored
            ctx->KeyPress(ImGuiKey_4 | ImGuiMod_Ctrl, 2);
            for (int window_n = 0; window_n < 3; window_n++)
                for (int key_n = 1; key_n <= 4; key_n++)
                {
                    const bool disabled = (window_n == 0 && key_n == 3) || (window_n == 1 && key_n == 2) || (window_n == 2 && key_n == 1) || (window_n == 3 && key_n == 1);
                    const int count = vars.Pressed[window_n][key_n];
                    {
                        if (window_n == 0 && !disabled)
                            IM_CHECK_EQ(count, 2);
                        else
                            IM_CHECK_EQ(count, 0);
                    }
                }
            memset(&vars.Pressed, 0, sizeof(vars.Pressed));

            ctx->ItemClick("**/Button 2");
            ctx->Yield(2);
            ctx->KeyPress(ImGuiKey_1 | ImGuiMod_Ctrl, 1);
            ctx->KeyPress(ImGuiKey_2 | ImGuiMod_Ctrl, 1); // will be caught by Window 1
            ctx->KeyPress(ImGuiKey_3 | ImGuiMod_Ctrl, 1);
            ctx->KeyPress(ImGuiKey_4 | ImGuiMod_Ctrl, 1);
            // Variant: hold CTRL before (verify it doesn't after ownership stealing)
            ctx->KeyDown(ImGuiMod_Ctrl);
            ctx->KeyPress(ImGuiKey_1, 1);
            ctx->KeyPress(ImGuiKey_2, 1); // will be caught by Window 1
            ctx->KeyPress(ImGuiKey_3, 1);
            ctx->KeyPress(ImGuiKey_4, 1);
            ctx->KeyUp(ImGuiMod_Ctrl);
            for (int window_n = 0; window_n < 3; window_n++)
                for (int key_n = 1; key_n <= 4; key_n++)
                {
                    const bool disabled = (window_n == 0 && key_n == 3) || (window_n == 1 && key_n == 2) || (window_n == 2 && key_n == 1) || (window_n == 3 && key_n == 1);
                    const int count = vars.Pressed[window_n][key_n];
                    if (vars.InputFlags & ImGuiInputFlags_ServeAll)
                    {
                        if (window_n <= 1 && !disabled)
                            IM_CHECK_EQ(count, 2);
                        else
                            IM_CHECK_EQ(count, 0);
                    }
                    if (vars.InputFlags & ImGuiInputFlags_ServeFirst)
                    {
                        if (window_n == 0 && !disabled)
                            IM_CHECK_EQ(count, 2);
                        else if (window_n == 1 && key_n == 3)
                            IM_CHECK_EQ(count, 2);
                        else
                            IM_CHECK_EQ(count, 0);
                    }
                    if (vars.InputFlags & ImGuiInputFlags_ServeLast)
                    {
                        if (window_n == 0 && key_n == 2)
                            IM_CHECK_EQ(count, 2);
                        else if (window_n == 1 && !disabled)
                            IM_CHECK_EQ(count, 2);
                        else
                            IM_CHECK_EQ(count, 0);
                    }
                }
            memset(&vars.Pressed, 0, sizeof(vars.Pressed));

            ctx->ItemClick("**/Button 3");
            ctx->Yield(2);
            ctx->KeyPress(ImGuiKey_1 | ImGuiMod_Ctrl, 2);
            ctx->KeyPress(ImGuiKey_2 | ImGuiMod_Ctrl, 2);
            ctx->KeyPress(ImGuiKey_3 | ImGuiMod_Ctrl, 2);
            ctx->KeyPress(ImGuiKey_4 | ImGuiMod_Ctrl, 2);
            for (int window_n = 0; window_n < 3; window_n++)
                for (int key_n = 1; key_n <= 4; key_n++)
                {
                    const bool disabled = (window_n == 0 && key_n == 3) || (window_n == 1 && key_n == 2) || (window_n == 2 && key_n == 1) || (window_n == 3 && key_n == 1);
                    const int count = vars.Pressed[window_n][key_n];
                    if (vars.InputFlags & ImGuiInputFlags_ServeAll)
                    {
                        if (window_n <= 2 && !disabled)
                            IM_CHECK_EQ(count, 2);
                        else
                            IM_CHECK_EQ(count, 0);
                    }
                    if (vars.InputFlags & ImGuiInputFlags_ServeFirst)
                    {
                        if (window_n == 1 && key_n == 3)
                            IM_CHECK_EQ(count, 2);
                        else if (window_n == 0 && !disabled)
                            IM_CHECK_EQ(count, 2);
                        else
                            IM_CHECK_EQ(count, 0);
                    }
                    if (vars.InputFlags & ImGuiInputFlags_ServeLast)
                    {
                        if (window_n == 2 && !disabled)
                            IM_CHECK_EQ(count, 2);
                        else if (window_n == 1 && key_n == 1)
                            IM_CHECK_EQ(count, 2);
                        else
                            IM_CHECK_EQ(count, 0);
                    }
                }
            memset(&vars.Pressed, 0, sizeof(vars.Pressed));

            ctx->ItemClick("**/Button 3");
        }
    };
#endif
}

//-------------------------------------------------------------------------
