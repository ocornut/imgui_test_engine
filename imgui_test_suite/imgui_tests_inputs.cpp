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
#if IMGUI_VERSION_NUM < 19002
static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }    // for IM_CHECK_EQ()
static inline bool operator!=(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x != rhs.x || lhs.y != rhs.y; }    // for IM_CHECK_NE()
#endif

#if IMGUI_VERSION_NUM >= 19063
#define ImGuiMod_CtrlAfterMacTranslation    (g.IO.ConfigMacOSXBehaviors ? ImGuiMod_Super : ImGuiMod_Ctrl)
#else
#define ImGuiMod_CtrlAfterMacTranslation    (ImGuiMod_Ctrl)
#endif

#if IMGUI_VERSION_NUM < 19066
#define ImGuiKeyOwner_NoOwner ImGuiKeyOwner_None
#endif

//-------------------------------------------------------------------------
// Tests: Inputs
//-------------------------------------------------------------------------

void RegisterTests_Inputs(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test io.WantCaptureMouse button on frame where a button that's not owned by imgui is released (#1392)
    t = IM_REGISTER_TEST(e, "inputs", "inputs_io_capture_on_release_not_owned");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *GImGui;
        ctx->MouseMoveToVoid();
        ctx->MouseDown(0);
        IM_CHECK_EQ(g.IO.WantCaptureMouse, false);
        IM_CHECK_EQ(g.ActiveId, 0u);

        ctx->MouseMove("Dear ImGui Demo", ImGuiTestOpFlags_NoCheckHoveredId);
        IM_CHECK_EQ(g.IO.WantCaptureMouse, false);
        IM_CHECK_EQ(g.ActiveId, 0u);

        ctx->MouseUp(0); // Include a yield
#if IMGUI_VERSION_NUM >= 19085 // Test bug reported by #1392
        IM_CHECK_EQ(g.IO.WantCaptureMouse, false);
#endif

        ctx->Yield();
        IM_CHECK_EQ(g.IO.WantCaptureMouse, true);
    };

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

        // MouseButton Ctrl+Left = Right alias for macOS X (#2343)
#if IMGUI_VERSION_NUM >= 19064
        bool backup_is_osx = io.ConfigMacOSXBehaviors;
        for (int is_osx = 0; is_osx < 2; is_osx++)
        {
            ctx->LogDebug("Testing Super+LeftClick atlas (osx=%d)", is_osx);
            io.ConfigMacOSXBehaviors = is_osx != 0;
            ctx->Yield();
            if (io.ConfigMacOSXBehaviors)
            {
                io.AddKeyEvent(ImGuiMod_Ctrl, true); // In a twisted plot of fate, because we feed raw input we use Ctrl here and not Super
                io.AddMouseButtonEvent(0, true);
                ctx->Yield();
                IM_CHECK_EQ(io.MouseDown[0], false);
                IM_CHECK_EQ(io.MouseDown[1], true); // Aliased
                ctx->Yield(3);
                IM_CHECK_EQ(io.MouseDown[0], false);
                IM_CHECK_EQ(io.MouseDown[1], true); // Still aliased
                io.AddKeyEvent(ImGuiMod_Ctrl, false);
                ctx->Yield();
                IM_CHECK_EQ(io.MouseDown[1], true); // Still aliased: ensure what's more sensible design for this
                io.AddMouseButtonEvent(0, false);
                ctx->Yield();
                IM_CHECK_EQ(io.MouseDown[1], false); // Still aliased for release
            }
            else
            {
                io.AddKeyEvent(ImGuiMod_Super, true);
                io.AddMouseButtonEvent(0, true);
                ctx->Yield();
                IM_CHECK_EQ(io.MouseDown[0], true);
                IM_CHECK_EQ(io.MouseDown[1], false); // Not aliased
                ctx->Yield(3);
                IM_CHECK_EQ(io.MouseDown[0], true);
                IM_CHECK_EQ(io.MouseDown[1], false); // Still not aliased
                io.AddMouseButtonEvent(0, false);
                ctx->Yield();
                IM_CHECK_EQ(io.MouseDown[0], false); // Still not aliased for release
                io.AddKeyEvent(ImGuiMod_Super, false);
            }
        }
        io.ConfigMacOSXBehaviors = backup_is_osx;
        ctx->Yield();
#endif

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
        IM_CHECK_EQ(io.KeyMods, ImGuiMod_CtrlAfterMacTranslation | ImGuiMod_Shift);
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
        IM_CHECK_EQ(io.KeyMods, ImGuiMod_CtrlAfterMacTranslation | ImGuiMod_Shift);
        io.AddKeyEvent(ImGuiKey_K, false);
        io.AddKeyEvent(ImGuiMod_Ctrl, false);
        io.AddKeyEvent(ImGuiMod_Shift, false);
        ctx->Yield();

        // KeyMods | KeyMods (same) -> 2 frames
        IM_CHECK(io.KeyMods == ImGuiMod_None);
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        io.AddKeyEvent(ImGuiMod_Ctrl, false);
        ctx->Yield();
        IM_CHECK_EQ(io.KeyMods, ImGuiMod_CtrlAfterMacTranslation);
        ctx->Yield();
        IM_CHECK(io.KeyMods == ImGuiMod_None);

        // KeyMods, KeyMods (different) -> 1 frame
        IM_CHECK(io.KeyMods == ImGuiMod_None);
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        io.AddKeyEvent(ImGuiMod_Shift, true);
        io.AddKeyEvent(ImGuiMod_Ctrl, false);
        io.AddKeyEvent(ImGuiMod_Shift, false);
        ctx->Yield();
        IM_CHECK_EQ(io.KeyMods, ImGuiMod_CtrlAfterMacTranslation | ImGuiMod_Shift);
        ctx->Yield();
        IM_CHECK(io.KeyMods == ImGuiMod_None);

        // MousePos, KeyMods -> 1 frame
        io.AddMousePosEvent(200.0f, 200.0f);
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        ctx->Yield();
        IM_CHECK_EQ(io.MousePos, ImVec2(200.0f, 200.0f));
        IM_CHECK(io.KeyMods == ImGuiMod_CtrlAfterMacTranslation);
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
        IM_CHECK(io.KeyMods == ImGuiMod_CtrlAfterMacTranslation);
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

        ImVec2 void_pos;
        if (!ctx->FindExistingVoidPosOnViewport(ImGui::GetMainViewport(), &void_pos))
            void_pos = ImVec2(1.0f, 1.0f);

        io.AddKeyEvent(ImGuiKey_Space, true);
        io.AddMousePosEvent(void_pos.x, void_pos.y);
        io.AddMouseButtonEvent(1, true);
        io.AddMouseWheelEvent(0.0f, 1.0f);
        IM_CHECK(g.InputEventsQueue.Size == 4);

        // May interfere
        io.AddMouseButtonEvent(1, true);
        io.AddMousePosEvent(void_pos.x, void_pos.y);
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
#if IMGUI_VERSION_NUM >= 18912 && IMGUI_VERSION_NUM < 19063
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
#if IMGUI_VERSION_NUM < 19063
            if (g.IO.ConfigMacOSXBehaviors)
            {
                // Verify IsKeyPressed() redirection + merged io.KeyMods
                IM_CHECK(ImGui::IsKeyDown(ImGuiMod_Super) == true);
                IM_CHECK(ImGui::IsKeyDown(ImGuiMod_Ctrl) == false);
                IM_CHECK(g.IO.KeyMods == (ImGuiMod_Super));// | ImGuiMod_Shortcut));
            }
            else
#endif
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

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_L);
        IM_CHECK_EQ(vars.Count, 1);

        vars.Count = 0;
        if (ctx->UiContext->IO.ConfigMacOSXBehaviors)
        {
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_L);
#if IMGUI_VERSION_NUM >= 19063
            IM_CHECK_EQ(vars.Count, 1);
#else
            IM_CHECK_EQ(vars.Count, 0);
#endif
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

#if IMGUI_VERSION_NUM >= 19003
    // ## Test key and key chord repeat behaviors and ImGuiInputFlags_RepeatUntilXXX flags.
    // This has some overlap with "inputs_routing_shortcut".
    // Also see "inputs_repeat_typematic" for lower-level timing calculations.
    t = IM_REGISTER_TEST(e, "inputs", "inputs_repeat_key");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        auto& counters = vars.IntArray;
        const ImGuiKey key = ImGuiKey_A;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        counters[0] += ImGui::IsKeyPressed(key, ImGuiInputFlags_None);
        counters[1] += ImGui::IsKeyPressed(key, ImGuiInputFlags_Repeat);
        counters[2] += ImGui::IsKeyPressed(key, ImGuiInputFlags_RepeatUntilRelease);
        counters[3] += ImGui::IsKeyPressed(key, ImGuiInputFlags_RepeatUntilKeyModsChange);
        counters[4] += ImGui::IsKeyPressed(key, ImGuiInputFlags_RepeatUntilKeyModsChangeFromNone);
        counters[5] += ImGui::IsKeyPressed(key, ImGuiInputFlags_RepeatUntilOtherKeyPress);
        ImGui::Text("%d (_None)", counters[0]);
        ImGui::Text("%d (_Repeat)", counters[1]);
        ImGui::Text("%d (_RepeatUntilRelease)", counters[2]);
        ImGui::Text("%d (_RepeatUntilKeyModsChange)", counters[3]);
        ImGui::Text("%d (_RepeatUntilKeyModsChangeFromNone)", counters[4]);
        ImGui::Text("%d (_RepeatUntilOtherKeyPress)", counters[5]);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GenericVars;
        auto& counters = vars.IntArray;
        const ImGuiKey key = ImGuiKey_A;

        ctx->KeyPress(key);
        for (int n = 0; n < 6; n++)
            IM_CHECK_EQ(counters[n], 1);
        memset(counters, 0, sizeof(int) * 6);

        const float duration = 0.8f;
        const int repeat_count_for_duration = 1 + ImGui::CalcTypematicRepeatAmount(0.0f, duration, g.IO.KeyRepeatDelay, g.IO.KeyRepeatRate);
        const int repeat_count_for_duration_x2 = 1 + ImGui::CalcTypematicRepeatAmount(0.0f, duration * 2, g.IO.KeyRepeatDelay, g.IO.KeyRepeatRate);

        ctx->KeyHold(key, duration);
        IM_CHECK_EQ(counters[0], 1);
        for (int n = 1; n < 6; n++)
            IM_CHECK_EQ(counters[n], repeat_count_for_duration);
        memset(counters, 0, sizeof(int) * 6);

        // Use KeySetEx() as we deal with precise timing (other functions add extra yields)
        // Hold Key and press Ctrl mid-way
        // IMPORTANT: we hold ImGuiMod_Ctrl down but test engine virtual backend doesn't emit a key for this.
        // Exact ImGuiMod_Ctrl <> ImGuiKey_LeftCtrl / ImGuiKey_Right synchronization is technically backend dependent
        // as we relay this ambiguity from OS layers themselves.
        ctx->KeySetEx(key, true, duration);
        for (int n = 1; n < 6; n++)
            IM_CHECK_EQ(counters[n], repeat_count_for_duration);
        ctx->KeySetEx(ImGuiMod_Ctrl, true, duration);           // Press Ctrl
        IM_CHECK_EQ(counters[2], repeat_count_for_duration_x2);
        IM_CHECK_EQ(counters[3], repeat_count_for_duration);    // _RepeatUntilKeyModsChange
        IM_CHECK_EQ(counters[4], repeat_count_for_duration);    // _RepeatUntilKeyModsChangeFromNone
        IM_CHECK_EQ(counters[5], repeat_count_for_duration);    // _RepeatUntilOtherKeyPress // <--- If this fails it can be because ImGuiMod_Ctrl isn't detected as a Key Press in LastKeyboardKeyPressTime handling logic.
        ctx->KeyUp(ImGuiMod_Ctrl | key);
        memset(counters, 0, sizeof(int) * 6);

        // Hold Ctrl+Key and release Ctrl mid-way
        // _RepeatUntilKeyModsChangeFromNone behave differently in this direction, as the behavior is useful for certain shortcut handling.
        ctx->KeySetEx(ImGuiMod_Ctrl | key, true, duration);
        for (int n = 1; n < 6; n++)
            IM_CHECK_EQ(counters[n], repeat_count_for_duration);
        ctx->KeySetEx(ImGuiMod_Ctrl, false, duration);          // Release Ctrl
        IM_CHECK_EQ(counters[2], repeat_count_for_duration_x2);
        IM_CHECK_EQ(counters[3], repeat_count_for_duration);    // _RepeatUntilKeyModsChange
        IM_CHECK_EQ(counters[4], repeat_count_for_duration_x2); // _RepeatUntilKeyModsChangeFromNone
        IM_CHECK_EQ(counters[5], repeat_count_for_duration_x2); // _RepeatUntilOtherKeyPress
        ctx->KeyUp(key);
        memset(counters, 0, sizeof(int) * 6);

        // Hold Key and press another key
        ctx->KeySetEx(key, true, duration);
        for (int n = 1; n < 6; n++)
            IM_CHECK_EQ(counters[n], repeat_count_for_duration);
        ctx->KeySetEx(ImGuiKey_UpArrow, true, duration);        // Press any other key!
        IM_CHECK_EQ(counters[2], repeat_count_for_duration_x2);
        IM_CHECK_EQ(counters[3], repeat_count_for_duration_x2); // _RepeatUntilKeyModsChange
        IM_CHECK_EQ(counters[4], repeat_count_for_duration_x2); // _RepeatUntilKeyModsChangeFromNone
        IM_CHECK_EQ(counters[5], repeat_count_for_duration);    // _RepeatUntilOtherKeyPress
        ctx->KeyUp(key);
        ctx->KeyUp(ImGuiKey_UpArrow);
        memset(counters, 0, sizeof(int) * 6);
    };
#endif

    t = IM_REGISTER_TEST(e, "inputs", "inputs_repeat_typematic");
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

#if IMGUI_VERSION_NUM >= 18968
    t = IM_REGISTER_TEST(e, "inputs", "inputs_mouse_stationary_timer");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImVec2 ref_pos = ImGui::GetMainViewport()->GetCenter();

        ctx->LogDebug("MouseStationaryTimer = %f", g.MouseStationaryTimer);
        ctx->MouseMoveToPos(ref_pos);
        ctx->MouseMoveToPos(ref_pos + ImVec2(10, 10));
        IM_CHECK_LE(g.MouseStationaryTimer, g.IO.DeltaTime * 3);
        ctx->SleepNoSkip(5.0f, 1.0f);
        IM_CHECK_GE(g.MouseStationaryTimer, 5.0f);

        // FIXME-TESTS: As we rework stationary timer logic, will want to test for high-framerates
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
        IM_CHECK_EQ(ImGui::TestKeyOwner(ImGuiKey_Home, ImGuiKeyOwner_NoOwner), true);
        ctx->ItemCheck("Steal ImGuiKey_Home");

        IM_CHECK_EQ(ImGui::TestKeyOwner(ImGuiKey_Home, ImGuiKeyOwner_NoOwner), false);
        IM_CHECK_EQ(ImGui::TestKeyOwner(ImGuiKey_Home, ctx->GetID("hello1")), true);
        IM_CHECK_EQ(ImGui::TestKeyOwner(ImGuiKey_End, ctx->GetID("hello1")), true);
        IM_CHECK_EQ(ImGui::GetKeyOwnerData(&g, ImGuiKey_End)->OwnerCurr, ImGuiKeyOwner_NoOwner);
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
        IM_CHECK_EQ_NO_RET(ImGui::TestKeyOwner(ImGuiKey_A, ImGuiKeyOwner_NoOwner), ctx->IsFirstGuiFrame() ? true : false);
        IM_CHECK_EQ_NO_RET(ImGui::TestKeyOwner(ImGuiKey_A, ImGui::GetItemID()), true);
        ImGui::SetKeyOwner(ImGuiKey_A, ImGui::GetItemID());

        ImGui::Button("Button 2");
        if (!ctx->IsFirstGuiFrame()) // Can't check this on first frame since TestKeyOwner() is not checking OwnerNext
        {
            IM_CHECK_EQ_NO_RET(ImGui::TestKeyOwner(ImGuiKey_A, ImGuiKeyOwner_NoOwner), false);
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
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);

        ImGui::Button("Button 1");
        if (vars.Step >= 1)
            ImGui::SetKeyOwner(ImGuiKey_Space, ImGui::GetItemID());

        const ImGuiKeyOwnerData* owner_data = ImGui::GetKeyOwnerData(&g, ImGuiKey_Space);
        if (vars.Bool1)
        {
            if (vars.Step == 0)
                IM_CHECK_EQ(owner_data->OwnerCurr, ImGuiKeyOwner_NoOwner);
            else if (vars.Step == 1 || vars.Step == 2)
                IM_CHECK_EQ(owner_data->OwnerCurr, ImGui::GetID("Button 1"));
            // [2022-09-20]: changed SetKeyOwner() to alter OwnerCurr as well
            //else if (vars.Step == 2)
            //    IM_CHECK_EQ(owner_data->OwnerCurr, ImGui::GetID("Button 2"));
            if (vars.Step >= 1)
                IM_CHECK_EQ(owner_data->OwnerNext, ImGui::GetID("Button 1"));
        }

        ImGui::Button("Button 2");
        if (vars.Step >= 2)
            ImGui::SetKeyOwner(ImGuiKey_Space, ImGui::GetItemID());
        if (vars.Bool1 && vars.Step >= 2)
            IM_CHECK_EQ(owner_data->OwnerNext, ImGui::GetID("Button 2"));

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
        if (ImGui::IsKeyPressed(ImGuiKey_A, 0, owner_id ^ 0x42424242))
            vars.IntArray[2]++;
        if (ImGui::IsKeyPressed(ImGuiKey_A, 0, owner_id))
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

    // ## Test SetActiveIdUsingAllKeyboardKeys()
    t = IM_REGISTER_TEST(e, "inputs", "inputs_owner_activeid_using_all_keys");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Shortcut(ImGuiKey_W))
            vars.IntArray[0]++;
        ImGui::Button("behavior", ImVec2(100, 100));
        if (ImGui::IsItemActive())
        {
            ImGuiID behaviorId = ImGui::GetItemID();
            ImGui::SetActiveIdUsingAllKeyboardKeys();
            if (ImGui::IsKeyDown(ImGuiKey_W, behaviorId))
                vars.IntArray[1]++;
            if (ImGui::IsKeyDown(ImGuiKey_S, behaviorId))
                vars.IntArray[2]++;
        }
        for (int n = 0; n < 3; n++)
            ImGui::Text("%d", vars.IntArray[n]);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->ItemClick("behavior"); // Dummy click to get focus
        ctx->KeyPress(ImGuiKey_W);
        IM_CHECK_EQ(vars.IntArray[0], 1);
        IM_CHECK_EQ(vars.IntArray[1], 0);
        ctx->KeyPress(ImGuiKey_S);
        IM_CHECK_EQ(vars.IntArray[2], 0); // No-op
        vars.Clear();
        ctx->MouseDown();
        ctx->Yield();
        IM_CHECK_EQ(ImGui::GetActiveID(), ctx->GetID("behavior"));
        ctx->KeyPress(ImGuiKey_W);  // Caught by behavior
#if IMGUI_VERSION_NUM >= 19016
        IM_CHECK_EQ(vars.IntArray[0], 0);
        IM_CHECK_GE(vars.IntArray[1], 1);
#endif
        ctx->KeyPress(ImGuiKey_S);  // Caught by behavior
        IM_CHECK_GE(vars.IntArray[2], 1);
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

#if IMGUI_VERSION_NUM >= 19016
    // ## General test with LeftXXX/RightXXX keys vs ModXXX
    t = IM_REGISTER_TEST(e, "inputs", "inputs_owner_single_mod");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
        vars.OwnerId = ImGui::GetID("OwnerID");
        if (ImGui::BeginMenuBar())
        {
            ImGui::MenuItem("MenuItem");
            ImGui::EndMenuBar();
        }
        ImGui::Button("Button1");

        ImGui::RadioButton("SetKeyOwner(ModAlt)", &vars.Step, 1);
        ImGui::RadioButton("SetKeyOwner(LeftAlt)", &vars.Step, 2);
        ImGui::RadioButton("Shortcut(ModAlt)", &vars.Step, 3);
        ImGui::RadioButton("Shortcut(LeftAlt)", &vars.Step, 4);

        // Case 1: OK
        if (vars.Step == 1)
            ImGui::SetKeyOwner(ImGuiMod_Alt, vars.OwnerId);

        // Case 2: KO: Doesn't inhibit Nav
        if (vars.Step == 2)
            ImGui::SetKeyOwner(ImGuiKey_LeftAlt, vars.OwnerId);

        // Case 3: OK
        if (vars.Step == 3 && ImGui::Shortcut(ImGuiMod_Alt, 0, vars.OwnerId))
        {
            vars.IntArray[3]++;
            ImGui::Text("PRESSED");
        }

        // Case 4: KO : Shortcut works but doesn't inhibit Nav
        if (vars.Step == 4 && ImGui::Shortcut(ImGuiKey_LeftAlt, 0, vars.OwnerId))
        {
            vars.IntArray[4]++;
            ImGui::Text("PRESSED");
        }

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GenericVars;

        ctx->SetRef("Test Window");

        // SetKeyOwner(ModAlt)
        vars.Step = 1;
        ctx->Yield(2);
        IM_CHECK_EQ(ImGui::GetKeyOwner(ImGuiMod_Alt), vars.OwnerId);
        ctx->KeyPress(ImGuiMod_Alt);
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Main);
        ctx->KeyPress(ImGuiKey_LeftAlt);
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Main);

        // SetKeyOwner(LeftAlt)

        // Shortcut(ModAlt)
        vars.Clear();
        vars.Step = 3;
        ctx->Yield(2);
        IM_CHECK_EQ(ImGui::GetKeyOwner(ImGuiMod_Alt), ImGuiKeyOwner_NoOwner);
        ctx->KeyDown(ImGuiMod_Alt);
        IM_CHECK_EQ(ImGui::GetKeyOwner(ImGuiMod_Alt), vars.OwnerId);
        IM_CHECK_EQ(vars.IntArray[3], 1);
        ctx->KeyUp(ImGuiMod_Alt);
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Main);
        ctx->Yield(2);
        ctx->KeyDown(ImGuiKey_LeftAlt);
        IM_CHECK_EQ(ImGui::GetKeyOwner(ImGuiMod_Alt), vars.OwnerId);
        IM_CHECK_EQ(vars.IntArray[3], 2);
        ctx->KeyUp(ImGuiKey_LeftAlt);
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Main);

        // Shortcut(LeftAlt)
        vars.Clear();
        vars.Step = 4;
        ctx->Yield(2);
        IM_CHECK_EQ(ImGui::GetKeyOwner(ImGuiKey_LeftAlt), ImGuiKeyOwner_NoOwner);
        ctx->KeyDown(ImGuiMod_Alt);
        IM_CHECK_EQ(ImGui::GetKeyOwner(ImGuiMod_Alt), ImGuiKeyOwner_NoOwner);
        IM_CHECK_EQ(ImGui::GetKeyOwner(ImGuiKey_LeftAlt), vars.OwnerId);
        IM_CHECK_EQ(vars.IntArray[4], 1);            // Shortcut passed: TestEngine arbitrarily needs to turn loose ImGuiMod_Alt into some Alt key
        ctx->KeyUp(ImGuiMod_Alt);
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Main); // No effect
        ctx->Yield(2);
        ctx->KeyDown(ImGuiKey_LeftAlt);
        IM_CHECK_EQ(ImGui::GetKeyOwner(ImGuiMod_Alt), ImGuiKeyOwner_NoOwner);
        IM_CHECK_EQ(ImGui::GetKeyOwner(ImGuiKey_LeftAlt), vars.OwnerId);
        IM_CHECK_EQ(vars.IntArray[4], 2);            // Shortcut passed
        ctx->KeyUp(ImGuiKey_LeftAlt);
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Main); // No effect
        ctx->Yield(2);
        ctx->KeyDown(ImGuiKey_RightAlt);
        IM_CHECK_EQ(vars.IntArray[4], 2);
        ctx->KeyUp(ImGuiKey_RightAlt);
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Menu); // Toggle layer
    };
#endif

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
        ImGuiInputFlags Flags = 0;
        bool            IsRouting[26] = {};
        bool            StealOwner = false;
        int             PressedCount[26] = {};
        ImGuiKeyChord   KeyChord = ImGuiMod_Ctrl | ImGuiKey_A;
        char            Str[64] = "Hello";
        ImGuiID         OwnerID = 0;

        void            Clear()
        {
            memset(IsRouting, 0, sizeof(IsRouting));
            memset(PressedCount, 0, sizeof(PressedCount));
        }
        void            TestIsRoutingOnly(int idx)
        {
            if (idx != -1)
                IM_CHECK_EQ(IsRouting[idx], true);
            for (int n = 0; n < IM_ARRAYSIZE(IsRouting); n++)
                if (n != idx)
                    IM_CHECK_SILENT(IsRouting[n] == false);
        }
        void            TestIsPressedOnly(int idx)
        {
            if (idx != -1)
                IM_CHECK_GT(PressedCount[idx], 0);
            for (int n = 0; n < IM_ARRAYSIZE(IsRouting); n++)
                if (n != idx)
                    IM_CHECK_SILENT(PressedCount[n] == 0);
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
            bool shortcut_pressed = ImGui::Shortcut(vars.KeyChord, 0, id);// ImGuiInputFlags_RouteAlways); // Use _RouteAlways to poll only, no side-effect
            vars.IsRouting[idx] = is_routing;
            vars.PressedCount[idx] += shortcut_pressed ? 1 : 0;
            ImGui::Text("Routing: %d %s", is_routing, shortcut_pressed ? "PRESSED" : "...");
        };

        ImGui::Begin("WindowA");
#if IMGUI_VERSION_NUM >= 19012
        ImGui::Text("Chord: %s", ImGui::GetKeyChordName(vars.KeyChord));
#endif
        ImGui::Button("WindowA");
        DoRoute('A');

        //if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A))
        //    ImGui::Text("PRESSED 111");

        // TODO: Write a test for that
        //ImGui::Text("Down: %d", ImGui::InputRoutingSubmit2(ImGuiKey_DownArrow));

        ImGui::InputText("InputTextB", vars.Str, IM_ARRAYSIZE(vars.Str));
        DoRouteForItem('B', ImGui::GetItemID());

        ImGui::Button("ButtonC");
        ImGui::BeginChild("ChildD", ImVec2(-FLT_MIN, 100), ImGuiChildFlags_Border);
        ImGui::Button("ChildD");
        ImGui::EndChild();
        ImGui::BeginChild("ChildE", ImVec2(-FLT_MIN, 100), ImGuiChildFlags_Border);
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
        ImGui::BeginChild("ChildI", ImVec2(-FLT_MIN, 100), ImGuiChildFlags_Border);
        ImGui::Button("ChildI");
        ImGui::EndChild();
        ImGui::End();

        // TODO: NodeTabBarL+DockedWindowM

        ImGui::Begin("WindowN");
        ImGui::Button("WindowM");
        DoRoute('M');

        ImGui::BeginChild("ChildN", ImVec2(-FLT_MIN, 300), ImGuiChildFlags_Border);
        ImGui::Button("ChildN");
        DoRoute('N');

        ImGui::BeginChild("ChildO", ImVec2(-FLT_MIN, 150), ImGuiChildFlags_Border);
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

        auto alpha_to_idx = [](char c) { IM_ASSERT(c >= 'A' && c <= 'Z'); return c - 'A'; };

        // A
        ctx->ItemClick("WindowA");
        vars.TestIsRoutingOnly(alpha_to_idx('A'));
        vars.TestIsPressedOnly(-1);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
        vars.TestIsPressedOnly(alpha_to_idx('A'));
        vars.Clear();

        // B: verify catching CTRL+A
        ctx->ItemClick("InputTextB");
        vars.TestIsRoutingOnly(alpha_to_idx('B'));
        vars.TestIsPressedOnly(-1);
        input_state = ImGui::GetInputTextState(ctx->GetID("InputTextB"));
        IM_CHECK(input_state != NULL);
        IM_CHECK(input_state->HasSelection() == false);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
        vars.TestIsPressedOnly(alpha_to_idx('B'));
        input_state = ImGui::GetInputTextState(ctx->GetID("InputTextB"));
        IM_CHECK(input_state != NULL);
        IM_CHECK(input_state->HasSelection() == true);
        vars.Clear();

        // B: verify other shortcut CTRL+B when B is active but not using it.
        vars.KeyChord = ImGuiMod_Ctrl | ImGuiKey_B;
        ctx->Yield(2);
#if IMGUI_VERSION_NUM >= 19012
        // Not caught by anyone
        vars.TestIsRoutingOnly(alpha_to_idx('B')); // Should be None but we call Shortcut() for item making it a side-effect
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_B);
        vars.TestIsPressedOnly(alpha_to_idx('B'));
#else
        // Only caught by A
        vars.TestIsRoutingOnly(alpha_to_idx('A'));
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_B);
        vars.TestIsPressedOnly(alpha_to_idx('A'));
#endif
        vars.KeyChord = ImGuiMod_Ctrl | ImGuiKey_A;
        vars.Clear();

        // D: Focus child which does no polling/routing: parent A gets it: results are same as A
        ctx->ItemClick("**/ChildD");
        ctx->Yield();
        vars.TestIsRoutingOnly(alpha_to_idx('A'));
        vars.TestIsPressedOnly(-1);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
        vars.TestIsPressedOnly(alpha_to_idx('A'));
        vars.Clear();

        // E: Focus child which does its polling/routing
        ctx->ItemClick("**/ChildE");
        vars.TestIsRoutingOnly(alpha_to_idx('E'));
        vars.TestIsPressedOnly(-1);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
        vars.TestIsPressedOnly(alpha_to_idx('E'));
        vars.Clear();

        // F: Open Popup
        ctx->ItemClick("Open Popup");
        ctx->Yield(); // Appearing route requires a frame to establish
        vars.TestIsRoutingOnly(alpha_to_idx('F'));
        vars.TestIsPressedOnly(-1);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
        vars.TestIsPressedOnly(alpha_to_idx('F'));
        vars.Clear();

        // G: verify popup's input text catching CTRL+A
        ctx->ItemClick("**/InputTextG");
        vars.TestIsRoutingOnly(alpha_to_idx('G'));
        vars.TestIsPressedOnly(-1);
        input_state = ImGui::GetInputTextState(ImGui::GetActiveID());
        IM_CHECK(input_state != NULL);
        IM_CHECK(input_state->HasSelection() == false);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
        vars.TestIsPressedOnly(alpha_to_idx('G'));
        input_state = ImGui::GetInputTextState(ImGui::GetActiveID());
        IM_CHECK(input_state != NULL);
        IM_CHECK(input_state->HasSelection() == true);
        vars.Clear();

        // G: verify other shortcut CTRL+B when G is active but not using it, and F is not using it either.
        vars.KeyChord = ImGuiMod_Ctrl | ImGuiKey_B;
        ctx->Yield(2);
#if IMGUI_VERSION_NUM >= 19012
        // Not caught by anyone
        vars.TestIsRoutingOnly(alpha_to_idx('G')); // Should be None but we call Shortcut() for item making it a side-effect
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_B);
        vars.TestIsPressedOnly(alpha_to_idx('G'));
#else
        vars.TestIsRoutingOnly(alpha_to_idx('F'));
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_B);
        vars.TestIsPressedOnly(alpha_to_idx('F'));
#endif
        vars.KeyChord = ImGuiMod_Ctrl | ImGuiKey_A;
        vars.Clear();
    };
#endif

    // ## Additional tests for Shortcut(), notably of underlying ImGuiInputFlags_RepeatUntilKeyModsChange behavior.
    // Notably, test cases of releasing a mod key not triggering the other shortcut when Repeat is on
#if IMGUI_VERSION_NUM >= 19003
    t = IM_REGISTER_TEST(e, "inputs", "inputs_routing_shortcut");
    t->SetVarsDataType<InputRoutingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputRoutingVars>();

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Button("Button0");
        if (ImGui::Shortcut(ImGuiKey_W, vars.OwnerID, ImGuiInputFlags_Repeat))
            vars.PressedCount[0]++;

        ImGui::BeginChild("Child1", ImVec2(100, 100), ImGuiChildFlags_Border);
        ImGui::Button("Button1");
        if (ImGui::Shortcut(ImGuiKey_W | ImGuiMod_Ctrl, vars.OwnerID, ImGuiInputFlags_Repeat))
            vars.PressedCount[1]++;
        ImGui::EndChild();

        ImGui::Text("Count: %d (W)", vars.PressedCount[0]);
        ImGui::Text("Count: %d (Ctrl+W)", vars.PressedCount[1]);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputRoutingVars>();

        ctx->SetRef("Test Window");
        for (int step_n = 0; step_n < 2; step_n++)
        {
            ctx->LogDebug("Step %d: %s", step_n, step_n ? "With owner_id" : "Without owner_id");
            vars.PressedCount[0] = vars.PressedCount[1] = 0;

            ctx->ItemClick("Button0");
            ctx->KeyPress(ImGuiKey_W);
            IM_CHECK_EQ(vars.PressedCount[0], 1);
            IM_CHECK_EQ(vars.PressedCount[1], 0);
            ctx->ItemClick("**/Button1");
            ctx->KeyPress(ImGuiKey_W);
            IM_CHECK_EQ(vars.PressedCount[0], 2);
            IM_CHECK_EQ(vars.PressedCount[1], 0);
            ctx->KeyPress(ImGuiKey_W | ImGuiMod_Ctrl);
            IM_CHECK_EQ(vars.PressedCount[0], 2);
            IM_CHECK_EQ(vars.PressedCount[1], 1);

            // Test Ctrl down, W down, Ctrl up -> verify that W shortcut is not triggered.
            ctx->KeyDown(ImGuiMod_Ctrl);
            ctx->KeyDown(ImGuiKey_W);
            IM_CHECK_EQ(vars.PressedCount[0], 2);
            IM_CHECK_EQ(vars.PressedCount[1], 2);
            ctx->KeyUp(ImGuiMod_Ctrl);
            ctx->SleepNoSkip(0.5f, 0.1f);
            IM_CHECK_EQ(vars.PressedCount[0], 2); // Verify that W shortcut didn't trigger.
            IM_CHECK_EQ(vars.PressedCount[1], 2);
            ctx->KeyDown(ImGuiMod_Ctrl);
            ctx->KeyUp(ImGuiMod_Ctrl);
            IM_CHECK_EQ(vars.PressedCount[0], 2);
            IM_CHECK_EQ(vars.PressedCount[1], 2); // Verify that Ctrl+W shortcut didn't trigger
            ctx->KeyUp(ImGuiKey_W);
        }
    };
#endif

    // ## Test that key ownership is verified along with routing.
    // There's a possibility of a third-party setting key-owner without routing.
#if IMGUI_VERSION_NUM >= 19014
    t = IM_REGISTER_TEST(e, "inputs", "inputs_routing_shortcut_no_owners");
    t->SetVarsDataType<InputRoutingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputRoutingVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        vars.PressedCount[0] += ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_W);
        if (vars.StealOwner)
            ImGui::SetKeyOwner(ImGuiKey_W, ImGui::GetID("SomeOwner"));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputRoutingVars>();
        ctx->SetRef("Test Window");
        ctx->ItemClick("");
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_W);
        IM_CHECK_EQ(vars.PressedCount[0], 1);
        vars.StealOwner = true;
        ctx->Yield();
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_W);
        IM_CHECK_EQ(vars.PressedCount[0], 1);
    };
#endif

    // ## Test how shortcut that could characters are filtered out when an item is active that reads characters e.g. InputText().
#if IMGUI_VERSION_NUM >= 19013
    t = IM_REGISTER_TEST(e, "inputs", "inputs_routing_char_filter");
    t->SetVarsDataType<InputRoutingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputRoutingVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::CheckboxFlags("ImGuiInputFlags_RouteGlobal", &vars.Flags, ImGuiInputFlags_RouteGlobal);
        ImGui::InputText("buf", vars.Str, IM_ARRAYSIZE(vars.Str));
        vars.PressedCount[0] += ImGui::Shortcut(ImGuiKey_G, vars.Flags);                     // Filtered
        vars.PressedCount[1] += ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_G, vars.Flags);     // Pass
        vars.PressedCount[2] += ImGui::Shortcut(ImGuiMod_Alt | ImGuiKey_G, vars.Flags);      // Filtered
        vars.PressedCount[3] += ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_G, vars.Flags);  // Currently pass / Should be filtered?
        vars.PressedCount[4] += ImGui::Shortcut(ImGuiMod_Shift | ImGuiMod_Alt | ImGuiKey_G, vars.Flags); // Currently pass / Should be filtered?
        vars.PressedCount[5] += ImGui::Shortcut(ImGuiKey_F1, vars.Flags);                    // Pass
        ImGui::Text("%d - Shortcut(ImGuiKey_G)", vars.PressedCount[0]);
        ImGui::Text("%d - Shortcut(ImGuiMod_Ctrl | ImGuiKey_G)", vars.PressedCount[1]);
        ImGui::Text("%d - Shortcut(ImGuiMod_Alt | ImGuiKey_G)", vars.PressedCount[2]);
        ImGui::Text("%d - Shortcut(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_G)", vars.PressedCount[3]);
        ImGui::Text("%d - Shortcut(ImGuiMod_Shift | ImGuiMod_Alt | ImGuiKey_G)", vars.PressedCount[4]);
        ImGui::Text("%d - Shortcut(ImGuiKey_F1)", vars.PressedCount[5]);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *GImGui;
        auto& vars = ctx->GetVars<InputRoutingVars>();
        ctx->SetRef("Test Window");

        for (int step = 0; step < 4; step++)
        {
#if IMGUI_VERSION_NUM < 19067
            if (step >= 2)
                continue;
#endif
            ctx->LogDebug("Step %d", step);

            // Emit presses with no active id
            bool is_active = (step & 1) != 0;
            bool is_global = (step & 2) != 0;

            vars.Flags = is_global ? ImGuiInputFlags_RouteGlobal : ImGuiInputFlags_RouteFocused;
            ctx->Yield(2);

            ctx->ItemClick("buf");
            if (!is_active)
                ctx->KeyPress(ImGuiKey_Escape);

            ctx->KeyPress(ImGuiKey_G);
            vars.TestIsPressedOnly(is_active ? -1 : 0);
            vars.Clear();
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_G);
            vars.TestIsPressedOnly(1);
            vars.Clear();
            ctx->KeyPress(ImGuiMod_Alt | ImGuiKey_G);
            vars.TestIsPressedOnly(is_active ? -1 : 2);
            vars.Clear();

            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_G);
#if IMGUI_BROKEN_TESTS
            vars.TestIsPressedOnly(is_active ? -1 : 3); // Technically more correct but too aggressive behavior for rare user-base?
#else
            vars.TestIsPressedOnly(3);
#endif

            // Alt is eaten on OSX (unless Ctrl is also pressed)
            if (g.IO.ConfigMacOSXBehaviors == false)
            {
                vars.Clear();
                ctx->KeyPress(ImGuiMod_Shift | ImGuiMod_Alt | ImGuiKey_G);
    #if IMGUI_BROKEN_TESTS
                vars.TestIsPressedOnly(is_active ? -1 : 4); // Technically more correct but too aggressive behavior for rare user-base?
    #else
                vars.TestIsPressedOnly(4);
    #endif
            }

            vars.Clear();
            ctx->KeyPress(ImGuiKey_F1);
            vars.TestIsPressedOnly(5);
            vars.Clear();
        }
    };
#endif

#if IMGUI_VERSION_NUM >= 19064
    t = IM_REGISTER_TEST(e, "inputs", "inputs_keychord_name");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        IM_CHECK_STR_EQ(ImGui::GetKeyChordName(ImGuiKey_None), "None");
        IM_CHECK_STR_EQ(ImGui::GetKeyChordName(ImGuiKey_A), "A");
        IM_CHECK_STR_EQ(ImGui::GetKeyChordName(ImGuiKey_A | ImGuiMod_Ctrl), "Ctrl+A");
        IM_CHECK_STR_EQ(ImGui::GetKeyChordName(ImGuiKey_LeftCtrl), "LeftCtrl");
        IM_CHECK_STR_EQ(ImGui::GetKeyChordName(ImGuiMod_Ctrl | ImGuiKey_LeftCtrl), "LeftCtrl");
        IM_CHECK_STR_EQ(ImGui::GetKeyChordName(ImGuiMod_Ctrl), "Ctrl");
        IM_CHECK_STR_EQ(ImGui::GetKeyChordName(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_LeftCtrl), "Shift+LeftCtrl");
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
            ImGui::BeginChild(Str16f("Window%d", window_n).c_str(), ImVec2(200, 150), ImGuiChildFlags_Border);
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

        ImGui::BeginChild("Window2", ImVec2(320, 330), ImGuiChildFlags_Border);
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

        ImGui::BeginChild("Window3", ImVec2(240, 200), ImGuiChildFlags_Border);
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
            ImGui::BeginChild("Window4", ImVec2(220, 200), ImGuiChildFlags_Border);
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
