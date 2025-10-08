// dear imgui
// (tests: widgets)

#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_test_engine/imgui_te_engine.h"      // IM_REGISTER_TEST()
#include "imgui_test_engine/imgui_te_context.h"
#include "imgui_test_engine/imgui_te_utils.h"       // InputText() with Str
#include "imgui_test_engine/thirdparty/Str/Str.h"

// Warnings
#ifdef _MSC_VER
#pragma warning (disable: 4100) // unreferenced formal parameter
#pragma warning (disable: 4127) // conditional expression is constant
#endif

#if __clang__
#pragma clang diagnostic push
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                      // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"                          // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#if IMGUI_VERSION_NUM >= 19096
#define IMGUI_HAS_MULTISELECT
#endif
#if IMGUI_VERSION_NUM < 19104
#define ImGuiChildFlags_Borders ImGuiChildFlags_Border
#endif

//-------------------------------------------------------------------------
// Ideas/Specs for future tests
// It is important we take the habit to write those down.
// - Even if we don't implement the test right away: they allow us to remember edge cases and interesting things to test.
// - Even if they will be hard to actually implement/automate, they still allow us to manually check things.
//-------------------------------------------------------------------------
// TODO: Tests: MenuItemEx() with icon (what to test?)
// TODO: Tests: TabBar: test shrinking large number of tabs, that right-most tab edge touches exactly the edge of the tab bar.
// TODO: Tests: test SetColorEditOptions(0) then restore value
//-------------------------------------------------------------------------


// Helpers
#if IMGUI_VERSION_NUM < 19002
static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs)     { return lhs.x == rhs.x && lhs.y == rhs.y; }    // for IM_CHECK_EQ()
static inline bool operator==(const ImVec4& lhs, const ImVec4& rhs)     { return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w; }
static inline bool operator!=(const ImVec4& lhs, const ImVec4& rhs)     { return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w; }
#endif

#if IMGUI_VERSION_NUM < 19075
typedef ImGuiDataTypeTempStorage ImGuiDataTypeStorage;
#endif
void GetSliderTestRanges(ImGuiDataType data_type, ImGuiDataTypeStorage* min_p, ImGuiDataTypeStorage* max_p)
{
    switch (data_type)
    {
    case ImGuiDataType_S8:
        *(int8_t*)(void*)min_p = INT8_MIN;
        *(int8_t*)(void*)max_p = INT8_MAX;
        break;
    case ImGuiDataType_U8:
        *(uint8_t*)(void*)min_p = 0;
        *(uint8_t*)(void*)max_p = UINT8_MAX;
        break;
    case ImGuiDataType_S16:
        *(int16_t*)(void*)min_p = INT16_MIN;
        *(int16_t*)(void*)max_p = INT16_MAX;
        break;
    case ImGuiDataType_U16:
        *(uint16_t*)(void*)min_p = 0;
        *(uint16_t*)(void*)max_p = UINT16_MAX;
        break;
    case ImGuiDataType_S32:
        *(int32_t*)(void*)min_p = INT32_MIN / 2;
        *(int32_t*)(void*)max_p = INT32_MAX / 2;
        break;
    case ImGuiDataType_U32:
        *(uint32_t*)(void*)min_p = 0;
        *(uint32_t*)(void*)max_p = UINT32_MAX / 2;
        break;
    case ImGuiDataType_S64:
        *(int64_t*)(void*)min_p = INT64_MIN / 2;
        *(int64_t*)(void*)max_p = INT64_MAX / 2;
        break;
    case ImGuiDataType_U64:
        *(uint64_t*)(void*)min_p = 0;
        *(uint64_t*)(void*)max_p = UINT64_MAX / 2;
        break;
    case ImGuiDataType_Float:
        *(float*)(void*)min_p = -1000000000.0f; // Floating point types do not use their min/max supported values because widgets
        *(float*)(void*)max_p = +1000000000.0f; // may not be able to display them due to lossy RoundScalarWithFormatT().
        break;
    case ImGuiDataType_Double:
        *(double*)(void*)min_p = -1000000000.0;
        *(double*)(void*)max_p = +1000000000.0;
        break;
    default:
        IM_ASSERT(0);
    }
}

//-------------------------------------------------------------------------
// Tests: Widgets
//-------------------------------------------------------------------------

void RegisterTests_Widgets(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test basic button presses
    t = IM_REGISTER_TEST(e, "widgets", "widgets_button_press");
    struct ButtonPressTestVars { int ButtonPressCount[6] = { 0 }; };
    t->SetVarsDataType<ButtonPressTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ButtonPressTestVars& vars = ctx->GetVars<ButtonPressTestVars>();

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Button0"))
            vars.ButtonPressCount[0]++;
        if (ImGui::ButtonEx("Button1", ImVec2(0, 0), ImGuiButtonFlags_PressedOnDoubleClick))
            vars.ButtonPressCount[1]++;
        if (ImGui::ButtonEx("Button2", ImVec2(0, 0), ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick))
            vars.ButtonPressCount[2]++;
        if (ImGui::ButtonEx("Button3", ImVec2(0, 0), ImGuiButtonFlags_PressedOnClickReleaseAnywhere))
            vars.ButtonPressCount[3]++;
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        if (ImGui::ButtonEx("Button4", ImVec2(0, 0)))
            vars.ButtonPressCount[4]++;
        ImGui::PopItemFlag();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ButtonPressTestVars& vars = ctx->GetVars<ButtonPressTestVars>();
        ImGuiContext& g = *ctx->UiContext;

        ctx->SetRef("Test Window");
        ctx->ItemClick("Button0");
        IM_CHECK_EQ(vars.ButtonPressCount[0], 1);
        ctx->ItemDoubleClick("Button1");
        IM_CHECK_EQ(vars.ButtonPressCount[1], 1);
        ctx->ItemDoubleClick("Button2");
        IM_CHECK_EQ(vars.ButtonPressCount[2], 2);

        // Test ImGuiButtonFlags_PressedOnClickRelease vs ImGuiButtonFlags_PressedOnClickReleaseAnywhere
        vars.ButtonPressCount[2] = 0;
        ctx->MouseMove("Button2");
        ctx->MouseDown(0);
        ctx->MouseMove("Button0", ImGuiTestOpFlags_NoCheckHoveredId);
        ctx->MouseUp(0);
        IM_CHECK_EQ(vars.ButtonPressCount[2], 0);
        ctx->MouseMove("Button3");
        ctx->MouseDown(0);
        ctx->MouseMove("Button0", ImGuiTestOpFlags_NoCheckHoveredId);
        ctx->MouseUp(0);
        IM_CHECK_EQ(vars.ButtonPressCount[3], 1);

        // Test ImGuiButtonFlags_Repeat
        const float step = ImMin(g.IO.KeyRepeatDelay, g.IO.KeyRepeatRate) * 0.50f;
        ctx->ItemClick("Button4");
        IM_CHECK_EQ(vars.ButtonPressCount[4], 1);
        ctx->MouseDown(0);
#if IMGUI_VERSION_NUM >= 19142
        IM_CHECK_EQ(vars.ButtonPressCount[4], 2);
#else
        IM_CHECK_EQ(vars.ButtonPressCount[4], 1);
#endif
        ctx->SleepNoSkip(g.IO.KeyRepeatDelay, step);
        ctx->SleepNoSkip(g.IO.KeyRepeatRate, step);
        ctx->SleepNoSkip(g.IO.KeyRepeatRate, step);
        ctx->SleepNoSkip(g.IO.KeyRepeatRate, step);
#if IMGUI_VERSION_NUM >= 19142
        IM_CHECK_EQ(vars.ButtonPressCount[4], 2 + 1 + 3);
#elif IMGUI_VERSION_NUM >= 18705
        IM_CHECK_EQ(vars.ButtonPressCount[4], 1 + 1 + 3);
#else
        IM_CHECK_EQ(vars.ButtonPressCount[4], 1 + 1 + 3 * 2); // MouseRepeatRate was double KeyRepeatRate, that's not documented / or that's a bug
#endif
        ctx->MouseUp(0);

        // Test ImGuiButtonFlags_Repeat with Nav
        ctx->NavMoveTo("Button4");
        vars.ButtonPressCount[4] = 0;
        ctx->KeyDown(ImGuiKey_Space);
        IM_CHECK_EQ(vars.ButtonPressCount[4], 1);
        ctx->SleepNoSkip(g.IO.KeyRepeatDelay, step);
        ctx->SleepNoSkip(g.IO.KeyRepeatRate, step);
        ctx->SleepNoSkip(g.IO.KeyRepeatRate, step);
        ctx->SleepNoSkip(g.IO.KeyRepeatRate, step);
        ctx->KeyUp(ImGuiKey_Space);
#if IMGUI_VERSION_NUM >= 18804
        IM_CHECK_EQ(vars.ButtonPressCount[4], 1 + 1 + 3);
#else
        IM_CHECK_EQ(vars.ButtonPressCount[4], 1 + 3 * 2);
#endif
    };

    // ## Test basic button presses
    t = IM_REGISTER_TEST(e, "widgets", "widgets_button_mouse_buttons");
    t->SetVarsDataType<ButtonPressTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ButtonPressTestVars& vars = ctx->GetVars<ButtonPressTestVars>();

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::ButtonEx("ButtonL", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonLeft))
            vars.ButtonPressCount[0]++;
        if (ImGui::ButtonEx("ButtonR", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonRight))
            vars.ButtonPressCount[1]++;
        if (ImGui::ButtonEx("ButtonM", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonMiddle))
            vars.ButtonPressCount[2]++;
        if (ImGui::ButtonEx("ButtonLR", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight))
            vars.ButtonPressCount[3]++;

        if (ImGui::ButtonEx("ButtonL-release", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_PressedOnRelease))
            vars.ButtonPressCount[4]++;
        if (ImGui::ButtonEx("ButtonR-release", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_PressedOnRelease))
        {
            ctx->LogDebug("Pressed!");
            vars.ButtonPressCount[5]++;
        }
        for (int n = 0; n < IM_ARRAYSIZE(vars.ButtonPressCount); n++)
            ImGui::Text("%d: %d", n, vars.ButtonPressCount[n]);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ButtonPressTestVars& vars = ctx->GetVars<ButtonPressTestVars>();

        ctx->SetRef("Test Window");
        ctx->ItemClick("ButtonL", 0);
        IM_CHECK_EQ(vars.ButtonPressCount[0], 1);
        ctx->ItemClick("ButtonR", 1);
        IM_CHECK_EQ(vars.ButtonPressCount[1], 1);
        ctx->ItemClick("ButtonM", 2);
        IM_CHECK_EQ(vars.ButtonPressCount[2], 1);
        ctx->ItemClick("ButtonLR", 0);
        ctx->ItemClick("ButtonLR", 1);
        IM_CHECK_EQ(vars.ButtonPressCount[3], 2);

        vars.ButtonPressCount[3] = 0;
        ctx->MouseMove("ButtonLR");
        ctx->MouseDown(0);
        ctx->MouseDown(1);
        ctx->MouseUp(0);
        ctx->MouseUp(1);
        IM_CHECK_EQ(vars.ButtonPressCount[3], 1);

        vars.ButtonPressCount[3] = 0;
        ctx->MouseMove("ButtonLR");
        ctx->MouseDown(0);
        ctx->MouseMove("ButtonR", ImGuiTestOpFlags_NoCheckHoveredId);
        ctx->MouseDown(1);
        ctx->MouseUp(0);
        ctx->MouseMove("ButtonLR");
        ctx->MouseUp(1);
        IM_CHECK_EQ(vars.ButtonPressCount[3], 0);
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
    t = IM_REGISTER_TEST(e, "widgets", "widgets_button_status");
    struct ButtonStateTestVars
    {
        ButtonStateMachineTestStep NextStep;
        ImGuiTestGenericItemStatus Status;
        ButtonStateTestVars() { NextStep = ButtonStateMachineTestStep_None; }
    };
    t->SetVarsDataType<ButtonStateTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ButtonStateTestVars& vars = ctx->GetVars<ButtonStateTestVars>();
        ImGuiTestGenericItemStatus& status = vars.Status;

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

        // The "Unused" button allows to move the mouse away from the "Test" button
        ImGui::Button("Unused");

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ButtonStateTestVars& vars = ctx->GetVars<ButtonStateTestVars>();
        vars.NextStep = ButtonStateMachineTestStep_None;

        ctx->SetRef("Test Window");

        // Move mouse away from "Test" button
        ctx->MouseMove("Unused");
        vars.NextStep = ButtonStateMachineTestStep_Init;
        ctx->Yield();

        ctx->MouseMove("Test");
        vars.NextStep = ButtonStateMachineTestStep_MovedOver;
        ctx->Yield();

        vars.NextStep = ButtonStateMachineTestStep_MouseDown;
        ctx->MouseDown();

        ctx->MouseMove("Unused", ImGuiTestOpFlags_NoCheckHoveredId);
        vars.NextStep = ButtonStateMachineTestStep_MovedAway;
        ctx->Yield();

        ctx->MouseMove("Test");
        vars.NextStep = ButtonStateMachineTestStep_MovedOverAgain;
        ctx->Yield();

        vars.NextStep = ButtonStateMachineTestStep_MouseUp;
        ctx->MouseUp();

        ctx->MouseMove("Unused");
        vars.NextStep = ButtonStateMachineTestStep_Done;
        ctx->Yield();
    };

    // ## Test checkbox click
    t = IM_REGISTER_TEST(e, "widgets", "widgets_checkbox_001");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Window1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Checkbox("Checkbox", &vars.Bool1);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // We use WindowRef() to ensure the window is uncollapsed.
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        IM_CHECK(vars.Bool1 == false);
        ctx->SetRef("Window1");
        ctx->ItemClick("Checkbox");
        IM_CHECK(vars.Bool1 == true);
    };

    // ## Test all types with DragScalar().
    t = IM_REGISTER_TEST(e, "widgets", "widgets_datatype_1");
    struct DragDatatypeVars { int widget_type = 0; ImGuiDataType data_type = 0; char data_storage[10] = ""; char data_zero[8] = ""; ImGuiTestGenericItemStatus Status; };
    t->SetVarsDataType<DragDatatypeVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DragDatatypeVars& vars = ctx->GetVars<DragDatatypeVars>();
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        bool ret;
        if (vars.widget_type == 0)
            ret = ImGui::DragScalar("Drag", vars.data_type, &vars.data_storage[1], 0.5f);
        else
            ret = ImGui::SliderScalar("Slider", vars.data_type, &vars.data_storage[1], &vars.data_zero, &vars.data_zero);
        vars.Status.QueryInc(ret);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        DragDatatypeVars& vars = ctx->GetVars<DragDatatypeVars>();

        ctx->SetRef("Test Window");
        for (int widget_type = 0; widget_type < 2; widget_type++)
        {
            for (int data_type = 0; data_type < ImGuiDataType_COUNT; data_type++)
            {
#if IMGUI_VERSION_NUM >= 19094
                if (data_type == ImGuiDataType_Bool || data_type == ImGuiDataType_String)
                    continue;
#endif
                size_t data_size = ImGui::DataTypeGetInfo(data_type)->Size;
                IM_ASSERT(data_size + 2 <= sizeof(vars.data_storage));
                memset(vars.data_storage, 0, sizeof(vars.data_storage));
                memset(vars.data_zero, 0, sizeof(vars.data_zero));
                vars.widget_type = widget_type;
                vars.data_type = data_type;
                vars.data_storage[0] = vars.data_storage[1 + data_size] = 42; // Sentinel values
                const char* widget_name = widget_type == 0 ? "Drag" : "Slider";

                if (widget_type == 0)
                {
                    vars.Status.Clear();
                    ctx->MouseMove(widget_name);
                    ctx->MouseDown();
                    ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(30, 0));
                    IM_CHECK(vars.Status.Edited >= 1);
                    vars.Status.Clear();
                    ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(-40, 0));
                    IM_CHECK(vars.Status.Edited >= 1);
                    ctx->MouseUp();
                }

                vars.Status.Clear();
                ctx->ItemInput(widget_name);
                ctx->KeyChars("123");                               // Case fixed by PR #3231
                IM_CHECK_GE(vars.Status.RetValue, 1);
                IM_CHECK_GE(vars.Status.Edited, 1);
                vars.Status.Clear();
                ctx->Yield();
                IM_CHECK_EQ(vars.Status.RetValue, 0);        // Verify it doesn't keep returning as edited.
                IM_CHECK_EQ(vars.Status.Edited, 0);

                vars.Status.Clear();
                ctx->KeyPress(ImGuiKey_Enter);
                IM_CHECK(vars.data_storage[0] == 42);               // Ensure there were no oob writes.
                IM_CHECK(vars.data_storage[1 + data_size] == 42);
            }
        }
    };

    // ## Test DragInt() as InputText
    // ## Test ColorEdit4() as InputText (#2557)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragslider_as_input");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::DragInt("Drag", &vars.Int1);
        ImGui::ColorEdit4("Color", &vars.Color1.x);
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
        IM_CHECK_FLOAT_EQ(vars.Color1.y, 123.0f / 255.0f);
        ctx->KeyPress(ImGuiKey_Tab);
        ctx->KeyCharsAppendEnter("200");
        IM_CHECK_FLOAT_EQ(vars.Color1.x,   0.0f / 255.0f);
        IM_CHECK_FLOAT_EQ(vars.Color1.y, 123.0f / 255.0f);
        IM_CHECK_FLOAT_EQ(vars.Color1.z, 200.0f / 255.0f);
    };

#if IMGUI_VERSION_NUM >= 19161
    // ## Basic test for g.ActiveIdValueOnActivation introduced in #8223
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragslider_initial_value");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SliderFloat("SliderFloat", &vars.Float1, 0.0f, 1000.0f);
        ImGui::DragInt("DragInt", &vars.Int1, 1);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *GImGui;
        auto& vars = ctx->GenericVars;

        ctx->SetRef("Test Window");
        float f_111 = 111.0f;
        int i_42 = 42;
        vars.Float1 = 111.0f;
        vars.Int1 = 42;

        ctx->MouseMove("SliderFloat", ImGuiTestOpFlags_MoveToEdgeL);
        ctx->MouseDown(0);
        IM_CHECK(memcmp(&g.ActiveIdValueOnActivation, &f_111, sizeof(float)) == 0);
        ctx->MouseMove("SliderFloat", ImGuiTestOpFlags_MoveToEdgeR);
        IM_CHECK(memcmp(&g.ActiveIdValueOnActivation, &f_111, sizeof(float)) == 0);
        IM_CHECK(memcmp(&vars.Float1, &f_111, sizeof(float)) != 0);
        ctx->MouseUp(0);

        ctx->MouseMove("DragInt", ImGuiTestOpFlags_MoveToEdgeL);
        ctx->MouseDown(0);
        IM_CHECK(memcmp(&g.ActiveIdValueOnActivation, &i_42, sizeof(int)) == 0);
        ctx->MouseMove("DragInt", ImGuiTestOpFlags_MoveToEdgeR);
        IM_CHECK(memcmp(&g.ActiveIdValueOnActivation, &i_42, sizeof(int)) == 0);
        IM_CHECK(memcmp(&vars.Int1, &i_42, sizeof(int)) != 0);
        ctx->MouseUp(0);
    };
#endif

    // ## Test Sliders and Drags clamping values
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragslider_clamp");
    struct ImGuiDragSliderVars { float DragValue = 0.0f; float DragMin = 0.0f; float DragMax = 1.0f; float SliderValue = 0.0f; float SliderMin = 0.0f; float SliderMax = 0.0f; float ScalarValue = 0.0f; void* ScalarMinP = NULL; void* ScalarMaxP = NULL; ImGuiSliderFlags Flags = ImGuiSliderFlags_None; };
    t->SetVarsDataType<ImGuiDragSliderVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiDragSliderVars& vars = ctx->GetVars<ImGuiDragSliderVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        const char* format = "%.3f";
        ImGui::SliderFloat("Slider", &vars.SliderValue, vars.SliderMin, vars.SliderMax, format, vars.Flags);
        ImGui::DragFloat("Drag", &vars.DragValue, 1.0f, vars.DragMin, vars.DragMax, format, vars.Flags);
        ImGui::DragScalar("Scalar", ImGuiDataType_Float, &vars.ScalarValue, 1.0f, vars.ScalarMinP, vars.ScalarMaxP, NULL, vars.Flags);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ImGui::GetCurrentContext();
        ImGuiDragSliderVars& vars = ctx->GetVars<ImGuiDragSliderVars>();
        ctx->SetRef("Test Window");
#if IMGUI_VERSION_NUM >= 19125
        ImGuiSliderFlags flags[] = { ImGuiSliderFlags_None, ImGuiSliderFlags_ClampOnInput, ImGuiSliderFlags_ClampZeroRange, ImGuiSliderFlags_AlwaysClamp };
#else
        ImGuiSliderFlags flags[] = { ImGuiSliderFlags_None, ImGuiSliderFlags_AlwaysClamp };
#endif
        for (int i = 0; i < IM_ARRAYSIZE(flags); ++i)
        {
#if IMGUI_VERSION_NUM >= 19125
            bool clamp_on_input = (flags[i] & ImGuiSliderFlags_ClampOnInput) != 0;
#else
            bool clamp_on_input = (flags[i] & ImGuiSliderFlags_AlwaysClamp) != 0;
#endif
            vars.Flags = flags[i];

            float slider_min_max[][2] = { {0.0f, 1.0f}, {0.0f, 0.0f} };
            for (int j = 0; j < IM_ARRAYSIZE(slider_min_max); ++j)
            {
                ctx->LogInfo("## Slider %d with Flags = 0x%08X", j, vars.Flags);

                vars.SliderValue = 0.0f;
                vars.SliderMin = slider_min_max[j][0];
                vars.SliderMax = slider_min_max[j][1];

                ctx->ItemInputValue("Slider", 2);
                IM_CHECK_EQ(vars.SliderValue, clamp_on_input ? vars.SliderMax : 2.0f);

                // Check higher bound
                ctx->MouseMove("Slider", ImGuiTestOpFlags_MoveToEdgeR);
                ctx->MouseDown(); // Click will update clamping
                IM_CHECK_EQ(vars.SliderValue, vars.SliderMax);
                ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(100, 0));
                ctx->MouseUp();
                IM_CHECK_EQ(vars.SliderValue, vars.SliderMax);

                ctx->ItemInputValue("Slider", -2);
                IM_CHECK_EQ(vars.SliderValue, clamp_on_input ? vars.SliderMin : -2.0f);

                // Check lower bound
                ctx->MouseMove("Slider", ImGuiTestOpFlags_MoveToEdgeL);
                ctx->MouseDown(); // Click will update clamping
                IM_CHECK_EQ(vars.SliderValue, vars.SliderMin);
                ctx->MouseMoveToPos(g.IO.MousePos - ImVec2(100, 0));
                ctx->MouseUp();
                IM_CHECK_EQ(vars.SliderValue, vars.SliderMin);
            }

            float drag_min_max[][2] = { {0.0f, 1.0f}, {0.0f, 0.0f}, {-FLT_MAX, FLT_MAX} };
            for (int j = 0; j < IM_ARRAYSIZE(drag_min_max); ++j)
            {
                ctx->LogDebug("Drag %d with flags = 0x%08X", j, vars.Flags);

                vars.DragValue = 0.0f;
                vars.DragMin = drag_min_max[j][0];
                vars.DragMax = drag_min_max[j][1];

                // [0,0] is equivalent to [-FLT_MAX, FLT_MAX] range
#if IMGUI_VERSION_NUM >= 19125
                bool unbound = (vars.DragMin == 0.0f && vars.DragMax == 0.0f && !(vars.Flags & ImGuiSliderFlags_ClampZeroRange)) || (vars.DragMin == -FLT_MAX && vars.DragMax == FLT_MAX);
#else
                bool unbound = (vars.DragMin == 0.0f && vars.DragMax == 0.0f) || (vars.DragMin == -FLT_MAX && vars.DragMax == FLT_MAX);
#endif
                float value_before_click = 0.0f;

                ctx->ItemInputValue("Drag", -3);
                IM_CHECK_EQ(vars.DragValue, clamp_on_input && !unbound ? vars.DragMin : -3.0f);

                ctx->ItemInputValue("Drag", 2);
                IM_CHECK_EQ(vars.DragValue, clamp_on_input && !unbound ? vars.DragMax : 2.0f);

                // Check higher bound
                ctx->MouseMove("Drag");
                value_before_click = vars.DragValue;
                ctx->MouseDown(); // Click will not update clamping value
                IM_CHECK_EQ(vars.DragValue, value_before_click);
                ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(100, 0));
                ctx->MouseUp();
                if (unbound)
                    IM_CHECK_GT(vars.DragValue, value_before_click);
                else
                    IM_CHECK_EQ(vars.DragValue, value_before_click);

                // Check higher to lower bound
                value_before_click = vars.DragValue;
                ctx->MouseMove("Drag");
                ctx->MouseDragWithDelta(ImVec2(-100, 0));
                if (unbound)
                    IM_CHECK_LT(vars.DragValue, value_before_click);
                else
                    IM_CHECK_EQ(vars.DragValue, vars.DragMin);

                // Check low to high bound
                value_before_click = vars.DragValue;
                ctx->MouseMove("Drag");
                ctx->MouseDragWithDelta(ImVec2(100, 0));
                if (unbound)
                    IM_CHECK_GT(vars.DragValue, value_before_click);
                else
                    IM_CHECK_EQ(vars.DragValue, vars.DragMax);
            }

            float scalar_min_max[][2] = { {-FLT_MAX, 1.0f}, {0.0f, FLT_MAX} };
            for (int j = 0; j < IM_ARRAYSIZE(scalar_min_max); ++j)
            {
                ctx->LogDebug("Scalar %d with flags = 0x%08X", j, vars.Flags);

                vars.ScalarValue = 0.0f;
                vars.DragMin = scalar_min_max[j][0];    // No harm in reusing DragMin/DragMax.
                vars.DragMax = scalar_min_max[j][1];
                vars.ScalarMinP = (vars.DragMin == -FLT_MAX || vars.DragMin == +FLT_MAX) ? NULL : &vars.DragMin;
                vars.ScalarMaxP = (vars.DragMax == -FLT_MAX || vars.DragMax == +FLT_MAX) ? NULL : &vars.DragMax;

                bool unbound_min = vars.ScalarMinP == NULL;
                bool unbound_max = vars.ScalarMaxP == NULL;
                float value_before_click = 0.0f;

                ctx->ItemInputValue("Scalar", -3);
                IM_CHECK_EQ(vars.ScalarValue, clamp_on_input && !unbound_min ? vars.DragMin : -3.0f);

                ctx->ItemInputValue("Scalar", 2);
                IM_CHECK_EQ(vars.ScalarValue, clamp_on_input && !unbound_max ? vars.DragMax : 2.0f);

                // Check higher bound
                ctx->MouseMove("Scalar");
                value_before_click = vars.ScalarValue;
                ctx->MouseDown(); // Click will not update clamping value
                IM_CHECK_EQ(vars.ScalarValue, value_before_click);
                ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(100, 0));
                ctx->MouseUp();
                if (unbound_max)
                    IM_CHECK_GT(vars.ScalarValue, value_before_click);
                else
                    IM_CHECK_EQ(vars.ScalarValue, value_before_click);

                // Check higher to lower bound
                value_before_click = vars.ScalarValue = 50.0f;
                ctx->MouseMove("Scalar");
                ctx->MouseDragWithDelta(ImVec2(-100, 0));
                if (unbound_min)
                    IM_CHECK_LT(vars.ScalarValue, value_before_click);
                else
                    IM_CHECK_EQ(vars.ScalarValue, vars.DragMin);

                // Check low to high bound
                value_before_click = vars.ScalarValue = 0.0f;
                ctx->MouseMove("Scalar");
                ctx->MouseDragWithDelta(ImVec2(100, 0));
                if (unbound_max)
                    IM_CHECK_GT(vars.ScalarValue, value_before_click);
                else
                    IM_CHECK_EQ(vars.ScalarValue, vars.DragMax);
            }
        }
    };

#if 0
    // ## Testing Scroll on Window with Test Engine helpers
    t = IM_REGISTER_TEST(e, "widgets", "widgets_scrollbar");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
        ImGui::BeginChild("##Child", ImVec2(800, 800), ImGuiChildFlags_Border);
        ImGui::Text("This is some text");
        ImGui::EndChild();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ctx->ScrollTo(ImGuiAxis_Y, 60);
        ctx->ScrollTo(ImGuiAxis_X, 80);
    };
#endif

    // ## Test for IsItemHovered() on BeginChild() and InputTextMultiLine() (#1370, #3851)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_hover");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::BeginChild("Child", ImVec2(100, 100), ImGuiChildFlags_Borders);
        vars.Pos = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(10, 10));
        ImGui::Button("button");
        vars.Id = ImGui::GetItemID();
        ImGui::EndChild();
        vars.Bool1 = ImGui::IsItemHovered();
        ImGui::Text("hovered: %d", vars.Bool1);

        ImGui::InputTextMultiline("##Field", vars.Str1, IM_ARRAYSIZE(vars.Str1), ImVec2(-FLT_MIN, 0.0f));
        vars.Bool2 = ImGui::IsItemHovered();
        ImGui::Text("hovered: %d", vars.Bool2);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");

        // move on child's void
        ctx->MouseMoveToPos(vars.Pos);
        IM_CHECK(vars.Bool1 == true);
        IM_CHECK(vars.Bool2 == false);

        // move on child's item
        ctx->MouseMove(vars.Id);
        IM_CHECK(vars.Bool1 == true);
        IM_CHECK(vars.Bool2 == false);

        // move on InputTextMultiline() = child embedded in group
        ctx->MouseMove("##Field");
        IM_CHECK(vars.Bool1 == false);
        IM_CHECK(vars.Bool2 == true);
    };

#if IMGUI_VERSION_NUM < 19094
#define ImGuiItemFlags_Default_ ImGuiItemFlags_None
#endif

    // ## Test inheritance of ItemFlags
    t = IM_REGISTER_TEST(e, "widgets", "widgets_item_flags_stack");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        IM_CHECK_EQ(g.CurrentItemFlags, ImGuiItemFlags_Default_);

        ImGui::BeginChild("child1", ImVec2(100, 100));
        IM_CHECK_EQ(g.CurrentItemFlags, ImGuiItemFlags_Default_);
        ImGui::Button("enable button in child1");
        ImGui::EndChild();

        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::Button("disabled button in parent");

        IM_CHECK((g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0);
        ImGui::BeginChild("child1"); // Append
        IM_CHECK((g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0);
        ImGui::Button("disabled button in child1");
        ImGui::EndChild();

        ImGui::BeginChild("child2", ImVec2(100, 100)); // New
        IM_CHECK((g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0);
        ImGui::Button("disabled button in child2");
        ImGui::EndChild();

        ImGui::PopItemFlag();

#if IMGUI_VERSION_NUM >= 18207 && IMGUI_VERSION_NUM < 18415
        IM_CHECK_EQ(g.CurrentItemFlags, 0);
        ImGui::Begin("Test Window 2", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::End();
        IM_CHECK_EQ(g.CurrentItemFlags, ImGuiItemFlags_Disabled);
        ImGui::PopItemFlag();
        IM_CHECK_EQ(g.CurrentItemFlags, 0);
#endif

        ImGui::End();
    };

    // ## Test ColorEdit4() and IsItemDeactivatedXXX() functions
    // ## Test that IsItemActivated() doesn't trigger when clicking the color button to open picker
    t = IM_REGISTER_TEST(e, "widgets", "widgets_status_common");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::SliderInt("Step", &vars.Step, 0, 6);

        bool ret = false;
        switch (vars.Step)
        {
        case 0:
            ret = ImGui::Button("Button");
            break;
        case 1:
            ret = ImGui::Checkbox("Checkbox", &vars.Bool1);
            break;
        case 2:
            ret = ImGui::SliderFloat("Slider", &vars.Float1, 0.0f, 1.0f);
            break;
        case 3:
            ret = ImGui::Selectable("Selectable", &vars.Bool1);
            break;
        case 4:
            ret = ImGui::Combo("Combo", &vars.Int1, "Zero\0One\0Two\0\0");
            break;
        case 5:
            ret = ImGui::ColorEdit4("ColorEdit", &vars.Color1.x);
            break;
        case 6:
            ImGui::Text("Text Without An Identifier");
            break;
        }
        vars.Status.QueryInc(ret);
        vars.Status.Draw();
        if (ImGui::Button("Clear counters"))
            vars.Status.Clear();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Accumulate return values over several frames/action into each bool
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiTestGenericItemStatus& status = vars.Status;

        // Testing activation flag being set
        ctx->SetRef("Test Window");

        const char* item_names[6] = { "Button", "Checkbox", "Slider", "Selectable", "Combo", "ColorEdit##X" };
        for (int step = 0; step < 6; step++)
        {
            vars.Step = step;
            ctx->Yield(2);
            status.Clear();

            ctx->MouseMove(item_names[step]);
            IM_CHECK(status.RetValue == 0 && status.Activated == 0 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0 && status.Edited == 0);
            IM_CHECK(status.Hovered >= 1);
            ctx->MouseDown();
            IM_CHECK(status.Activated == 1 && status.Active >= 1 && status.Deactivated == 0);

            status.Clear();
            ctx->Yield();
            IM_CHECK(status.Hovered == 1);

            if (step == 2 || step == 5)
            {
                ctx->MouseMove(item_names[step], ImGuiTestOpFlags_MoveToEdgeL);
                ctx->MouseMove(item_names[step], ImGuiTestOpFlags_MoveToEdgeR);
            }

            ctx->MouseUp();
            IM_CHECK(status.Activated == 0 && status.Deactivated == 1);

            if (step == 4)
            {
                ctx->ComboClick("Combo/One");
                ctx->ComboClick("Combo/Two");
#if !IMGUI_BROKEN_TESTS
                continue; // Currently broken
#endif
            }

#if (IMGUI_VERSION_NUM > 19165) && (IMGUI_VERSION_NUM < 19181)
            if (step == 1 || step == 3) // Checkbox(), Selectable() were broken (#8370)
                continue;
#endif

            if (step != 0) // not triggered by Button
                IM_CHECK(status.DeactivatedAfterEdit == 1);
        }

        // Step 6: Text (item with no identifier) (#8883, #8877)
#if IMGUI_VERSION_NUM >= 19222
        {
            vars.Step = 6;
            ctx->Yield(2);
            status.Clear();
            ctx->Yield();
            IM_CHECK(status.Hovered == 1);
            ctx->MouseDown(0);
            status.Clear();
            ctx->Yield();
            IM_CHECK(status.Hovered == 1);
            ctx->MouseUp(0);
        }
#endif
    };

    // ## Test ColorEdit4() and IsItemDeactivatedXXX() functions
    // ## Test that IsItemActivated() doesn't trigger when clicking the color button to open picker
    t = IM_REGISTER_TEST(e, "widgets", "widgets_status_coloredit");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        bool ret = ImGui::ColorEdit4("Field", &vars.Color1.x, ImGuiColorEditFlags_None);
        vars.Status.QueryInc(ret);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Accumulate return values over several frames/action into each bool
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiTestGenericItemStatus& status = vars.Status;

        // Testing activation flag being set
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field/##ColorButton");
        IM_CHECK(status.RetValue == 0 && status.Activated == 1 && status.Deactivated == 1 && status.DeactivatedAfterEdit == 0 && status.Edited == 0);
        status.Clear();

        ctx->KeyPress(ImGuiKey_Escape);
        IM_CHECK(status.RetValue == 0 && status.Activated == 0 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0 && status.Edited == 0);
        status.Clear();
    };

    // ## Test InputText() and IsItemDeactivatedXXX() functions (mentioned in #2215)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_status_inputtext");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        bool ret = ImGui::InputText("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        vars.Status.QueryInc(ret);
        ImGui::InputText("Sibling", vars.Str2, IM_ARRAYSIZE(vars.Str2));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Accumulate return values over several frames/action into each bool
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiTestGenericItemStatus& status = vars.Status;

        // Testing activation flag being set
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field");
        IM_CHECK(status.RetValue == 0 && status.Activated == 1 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0 && status.Edited == 0);
        status.Clear();

        // Testing deactivated flag being set when canceling with Escape
        ctx->KeyPress(ImGuiKey_Escape);
        IM_CHECK(status.RetValue == 0 && status.Activated == 0 && status.Deactivated == 1 && status.DeactivatedAfterEdit == 0 && status.Edited == 0);
        status.Clear();

        // Testing validation with Return after editing
        ctx->ItemClick("Field");
        IM_CHECK(status.RetValue == 0 && status.Activated && !status.Deactivated && !status.DeactivatedAfterEdit && status.Edited == 0);
        status.Clear();
        ctx->KeyCharsAppend("Hello");
        IM_CHECK(status.RetValue != 0 && !status.Activated && !status.Deactivated && !status.DeactivatedAfterEdit && status.Edited >= 1);
        status.Clear();
        ctx->KeyPress(ImGuiKey_Enter);
        IM_CHECK(status.RetValue == 0 && !status.Activated && status.Deactivated && status.DeactivatedAfterEdit && status.Edited == 0);
        status.Clear();

        // Testing validation with Tab after editing
        ctx->ItemClick("Field");
        ctx->KeyCharsAppend(" World");
        IM_CHECK(status.RetValue != 0 && status.Activated && !status.Deactivated && !status.DeactivatedAfterEdit && status.Edited >= 1);
        status.Clear();
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK(status.RetValue == 0 && !status.Activated && status.Deactivated && status.DeactivatedAfterEdit && status.Edited == 0);
        status.Clear();
    };

#if IMGUI_VERSION_NUM >= 19165
    // ## Test the IsItemDeactivatedXXX() functions while interrupted (#5184, #5904, #6766, #8303, #8004?)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_status_deactivate_interrupted");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::SliderInt("Step", &vars.Step, 0, 2);

        ImGui::ButtonEx("Right click (1)", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonRight);
        ImGui::Text("F1 to ClearActiveID()");
        ImGui::Text("F2 to SetWindowFocus(NULL)");
        ImGui::Text("F3 to Open Modal");
        if (ImGui::Shortcut(ImGuiKey_F1, ImGuiInputFlags_RouteGlobal))
            ImGui::ClearActiveID();
        if (ImGui::Shortcut(ImGuiKey_F2, ImGuiInputFlags_RouteGlobal))
            ImGui::SetWindowFocus(NULL);
        if (ImGui::Shortcut(ImGuiKey_F3, ImGuiInputFlags_RouteGlobal))
            ImGui::OpenPopup("Modal 1");
        if (ImGui::BeginPopupModal("Modal 1"))
        {
            ImGui::Text("Interrupted!");
            if (ImGui::Shortcut(ImGuiKey_Escape))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        bool ret = false;
        //if (ImGui::GetActiveID() != ctx->GetID("//Test Window/Field"))
        //    vars.Str1[0] = 0;

        if (vars.Step == 0)
            ret = ImGui::InputText("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        else if (vars.Step == 1)
            ImGui::InputTextMultiline("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        else if (vars.Step == 2)
            ret = ImGui::SliderFloat3("Slider3", &vars.Color1.x, 0.0f, 1.0f);
        vars.Status.QueryInc(ret);

        bool is_item_activated = ImGui::IsItemActivated();
        if (is_item_activated)
            vars.Str1[0] = 0;

        /*
        if (is_item_activated) IMGUI_DEBUG_LOG("IsItemActivated()\n");
        bool is_item_deactivated = ImGui::IsItemDeactivated();
        if (is_item_deactivated) IMGUI_DEBUG_LOG("IsItemDeactivated(): '%s'\n", vars.Str1);
        bool is_item_deactivated_after_edit = ImGui::IsItemDeactivatedAfterEdit();
        if (is_item_deactivated_after_edit) IMGUI_DEBUG_LOG("IsItemDeactivatedAfterEdit(): '%s'\n", vars.Str1);
        */

        ImGui::ButtonEx("Right click (2)", ImVec2(0, 0), ImGuiButtonFlags_MouseButtonRight);
        ImGui::Text("F5 to ClearActiveID()");
        ImGui::Text("F6 to SetWindowFocus(NULL)");
        ImGui::Text("F7 to Open Modal");
        if (ImGui::Shortcut(ImGuiKey_F5, ImGuiInputFlags_RouteGlobal))
            ImGui::ClearActiveID();
        if (ImGui::Shortcut(ImGuiKey_F6, ImGuiInputFlags_RouteGlobal))
            ImGui::SetWindowFocus(NULL);
        if (ImGui::Shortcut(ImGuiKey_F7, ImGuiInputFlags_RouteGlobal))
            ImGui::OpenPopup("Modal 2");
        if (ImGui::BeginPopupModal("Modal 2"))
        {
            ImGui::Text("Interrupted!");
            if (ImGui::Shortcut(ImGuiKey_Escape))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Accumulate return values over several frames/action into each bool
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiTestGenericItemStatus& status = vars.Status;
        ImGuiContext& g = *GImGui;

        ctx->SetRef("Test Window");
        for (int step = 0; step < 3; step++)
        {
            vars.Step = step;
            const int substeps_count = (step == 2) ? 10 : 8;
            for (int substep = 0; substep < substeps_count; substep++)
            {
                ctx->LogDebug("Step %d,%d", vars.Step, substep);
                vars.Str1[0] = 0;
                vars.Color1 = ImVec4();
                ctx->Yield();

                if (step == 2)
                    ctx->ItemInput("Slider3/$$1");
                else
                    ctx->ItemInput("Field");
                ctx->KeyChars("0.5");
                status.Clear();
                if ((substep / 2) == 0)
                    ctx->ItemClick((substep & 1) ? "Right click (2)" : "Right click (1)", ImGuiMouseButton_Right);
                else if ((substep / 2) == 1)
                    ctx->KeyPress((substep & 1) ? ImGuiKey_F5 : ImGuiKey_F1); // ClearActiveID() before/after active id
                else if ((substep / 2) == 2)
                    ctx->KeyPress((substep & 1) ? ImGuiKey_F6 : ImGuiKey_F2); // SetWindowFocus(NULL) before/after active id
                else if ((substep / 2) == 3)
                    ctx->KeyPress((substep & 1) ? ImGuiKey_F7 : ImGuiKey_F3); // OpenModal before/after active id
                else if ((substep / 2) == 4)
                    ctx->KeyPress((substep & 1) ? (ImGuiMod_Shift | ImGuiKey_Tab) : (ImGuiKey_Tab)); // Go to next/prev field
                IM_CHECK(status.Deactivated == 1 && status.DeactivatedAfterEdit == 1);
                if ((substep / 2) == 4)
                    IM_CHECK_NE(g.ActiveId, 0u); // Tabbed
                else
                    IM_CHECK_EQ(g.ActiveId, 0u); // Interrupted
                if (step == 2)
                    IM_CHECK_EQ(vars.Color1.y, 0.5f); // SliderFloat3
                else
                    IM_CHECK_STR_EQ(vars.Str1, "0.5"); // InputText/InputTextMultiline
                if (substep == 6 || substep == 7)
                    ctx->KeyPress(ImGuiKey_Escape); // Close modal
            }
        }
    };
#endif

    // ## Test the IsItemDeactivatedXXX() functions (e.g. #2550, #1875)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_status_multicomponent");
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
        ImGuiTestGenericItemStatus& status = vars.Status;

        ctx->SetRef("Test Window");
        ImGuiID field_0 = ctx->GetID("Field/$$0");
        ImGuiID field_1 = ctx->GetID("Field/$$1");
        //ImGuiID field_2 = ctx->GetID("Field/$$2"));

        // Testing activation/deactivation flags
        ctx->ItemClick(field_0);
        IM_CHECK(status.RetValue == 0 && status.Activated == 1 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0);
        status.Clear();
        ctx->KeyPress(ImGuiKey_Enter);
        IM_CHECK(status.RetValue == 0 && status.Activated == 0 && status.Deactivated == 1 && status.DeactivatedAfterEdit == 0);
        status.Clear();

        // Testing validation with Return after editing
        ctx->ItemClick(field_0);
        status.Clear();
        ctx->KeyCharsAppend("123");
        IM_CHECK(status.RetValue >= 1 && status.Activated == 0 && status.Deactivated == 0);
        status.Clear();
        ctx->KeyPress(ImGuiKey_Enter);
        IM_CHECK(status.RetValue == 0 && status.Activated == 0 && status.Deactivated == 1);
        status.Clear();

        // Testing validation with Tab after editing
        ctx->ItemClick(field_0);
        ctx->KeyCharsAppend("456");
        status.Clear();
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK(status.RetValue == 0 && status.Activated == 1 && status.Deactivated == 1 && status.DeactivatedAfterEdit == 1);

        // Testing Edited flag on all components
        ctx->ItemClick(field_1); // FIXME-TESTS: Should not be necessary!
        ctx->ItemClick(field_0);
        ctx->KeyCharsAppend("111");
        IM_CHECK(status.Edited >= 1);
        ctx->KeyPress(ImGuiKey_Tab);
        status.Clear();
        ctx->KeyCharsAppend("222");
        IM_CHECK(status.Edited >= 1);
        ctx->KeyPress(ImGuiKey_Tab);
        status.Clear();
        ctx->KeyCharsAppend("333");
        IM_CHECK(status.Edited >= 1);
    };

    // ## Test the IsItemEdited() function when input vs output format are not matching
    t = IM_REGISTER_TEST(e, "widgets", "widgets_status_inputfloat_format_mismatch");
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
        ImGuiTestGenericItemStatus& status = vars.Status;

        // Input "1" which will be formatted as "1.000", make sure we don't report IsItemEdited() multiple times!
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field");
        ctx->KeyCharsAppend("1");
        IM_CHECK(status.RetValue == 1 && status.Edited == 1 && status.Activated == 1 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0);
        ctx->Yield();
        ctx->Yield();
        IM_CHECK(status.Edited == 1);
    };

    // ## Test overlap mode
    t = IM_REGISTER_TEST(e, "widgets", "widgets_overlap_1");
    struct OverlapTestVars { ImGuiTestGenericItemStatus Status[6]; int Int1 = 0; };
    t->SetVarsDataType<OverlapTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        OverlapTestVars& vars = ctx->GetVars<OverlapTestVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImVec2 p = ImGui::GetCursorScreenPos();
#if IMGUI_VERSION_NUM >= 18967
        ImGui::SetNextItemAllowOverlap();
#endif
        bool button_0_pressed = ImGui::ButtonEx("00000", ImVec2(50, 50), ImGuiButtonFlags_AllowOverlap);
#if IMGUI_VERSION_NUM < 18967
        ImGui::SetItemAllowOverlap();
#endif
        vars.Status[0].QueryInc(button_0_pressed);

        ImGui::SetCursorScreenPos(p + ImVec2(20, 25));
        bool button_1_pressed = ImGui::Button("11111", ImVec2(50, 25));
        vars.Status[1].QueryInc(button_1_pressed);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        OverlapTestVars& vars = ctx->GetVars<OverlapTestVars>();
        ctx->SetRef("Test Window");

        ImVec2 p = ctx->ItemInfo("00000").RectFull.Min;

        ctx->MouseMoveToPos(p - ImVec2(10, 10));
        vars.Status[0].Clear();
        vars.Status[1].Clear();

        // Mouse directly to overlap
        ctx->MouseTeleportToPos(p + ImVec2(25, 30));
        ctx->Yield(2);
#if IMGUI_VERSION_NUM >= 18967
        IM_CHECK(vars.Status[0].Hovered == 0);
#elif (IMGUI_VERSION_NUM < 18966) && IMGUI_BROKEN_TESTS
        IM_CHECK(vars.Status[0].Hovered == 1); // Can't be tested yet as IsItemHovered() ignore overlap.
#endif
        IM_CHECK(vars.Status[1].Hovered > 0);

        ctx->MouseClick();
        IM_CHECK(vars.Status[0].RetValue == 0);
        IM_CHECK(vars.Status[0].Active == 0);
        IM_CHECK(vars.Status[1].RetValue == 1);
        IM_CHECK(vars.Status[1].Active > 0);
    };

    // ## Test overlap mode on various widgets
#if IMGUI_VERSION_NUM >= 18967
    t = IM_REGISTER_TEST(e, "widgets", "widgets_overlap_2");
    t->SetVarsDataType<OverlapTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        OverlapTestVars& vars = ctx->GetVars<OverlapTestVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::SetNextItemWidth(200.0f);
        ImGui::SetNextItemAllowOverlap();
        bool ret = ImGui::SliderInt("##Slider", &vars.Int1, 0, 10);
        vars.Status[0].QueryInc(ret);

        ImRect r(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImGui::SetCursorScreenPos({ r.Max.x - 40, r.Min.y });
        ret = ImGui::Button("Button");
        vars.Status[1].QueryInc(ret);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        OverlapTestVars& vars = ctx->GetVars<OverlapTestVars>();
        ctx->SetRef("Test Window");

        ctx->MouseMove("##Slider", ImGuiTestOpFlags_MoveToEdgeL);
        vars.Status[0].Clear();
        vars.Status[1].Clear();
        ctx->MouseDown(0);
        IM_CHECK(vars.Int1 == 0);
        ctx->MouseMove("##Slider", ImGuiTestOpFlags_MoveToEdgeR);
        IM_CHECK(vars.Int1 == 10);
        IM_CHECK(vars.Status[0].Hovered > 0);
        IM_CHECK(vars.Status[1].Hovered == 0);
        ctx->MouseUp(0);
        vars.Status[0].Clear();
        vars.Status[1].Clear();
        ctx->Yield();
        IM_CHECK(vars.Status[0].Hovered == 0);
        IM_CHECK(vars.Status[1].Hovered > 0);
        ctx->MouseClick(0);
        IM_CHECK(vars.Status[0].Activated == 0);
        IM_CHECK(vars.Status[1].Activated > 0);
    };
#endif

    // ## Test timing of overlap mode
    t = IM_REGISTER_TEST(e, "widgets", "widgets_overlap_3_timing");
    t->SetVarsDataType<OverlapTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<OverlapTestVars>();
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos); // Ensure visible as we'll teleport to pos without full on MouseMove() calls
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        // Not overlapping but verify how AllowOverlap mode after timing
        // FIXME-TESTS: Testing IsItemHovered() timing but not button hover color timing. Expand test by using HoveredId
        {
            ImGui::SeparatorText("#7514");
            ImGui::SetNextItemAllowOverlap();
            bool button_0_pressed = ImGui::Button("Button0", { 120.0f, 50.0 });
            bool button_0_hovered = ImGui::IsItemHovered();
            vars.Status[0].QueryInc(button_0_pressed);

            bool button_1_pressed = ImGui::Button("Button1", { 120.0f, 50.0 });
            bool button_1_hovered = ImGui::IsItemHovered();
            vars.Status[1].QueryInc(button_1_pressed);

            ImGui::Text(button_0_hovered ? "Button 0 hovered" : "");
            ImGui::Text(button_1_hovered ? "Button 1 hovered" : "");
        }
        {
            ImGui::SeparatorText("#7515");
            ImVec2 pos = ImGui::GetCursorPos();

            ImGui::SetNextItemAllowOverlap();
            bool button_2_pressed = ImGui::Button("Button2", { 120.0f, 50.0f });
            bool button_2_hovered = ImGui::IsItemHovered();
            vars.Status[2].QueryInc(button_2_pressed);

            pos.y += 25;
            ImGui::SetCursorPos(pos);
            //ImGui::SetNextItemAllowOverlap();
            bool button_3_pressed = ImGui::Button("Button3", { 120.0f, 50.0f });// # button or dummy, irrelevant
            bool button_3_hovered = ImGui::IsItemHovered();
            //ImGui::Dummy({ 50.0f, 50.0f });
            vars.Status[3].QueryInc(button_3_pressed);

            ImGui::Text(button_2_hovered ? "Button 2 hovered" : "");
            ImGui::Text(button_3_hovered ? "Button 3 hovered" : "");
        }
        {
            ImGui::SeparatorText("No SetNextItemAllowOverlap()");
            ImVec2 pos = ImGui::GetCursorPos();

            bool button_4_pressed = ImGui::Button("Button4", { 120.0f, 50.0f });
            bool button_4_hovered = ImGui::IsItemHovered();
            vars.Status[4].QueryInc(button_4_pressed);

            pos.y += 25;
            ImGui::SetCursorPos(pos);
            bool button_5_pressed = ImGui::Button("Button5", { 120.0f, 50.0f });// # button or dummy, irrelevant
            bool button_5_hovered = ImGui::IsItemHovered();
            vars.Status[5].QueryInc(button_5_pressed);

            ImGui::Text(button_4_hovered ? "Button 4 hovered" : "");
            ImGui::Text(button_5_hovered ? "Button 5 hovered" : "");
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<OverlapTestVars>();
        ctx->SetRef("Test Window");

        ImRect button0_rect = ctx->ItemInfo("Button0").RectFull; // Query ahead to avoid messing with precise frame timing
        ImRect button1_rect = ctx->ItemInfo("Button1").RectFull;
        ImRect button2_rect = ctx->ItemInfo("Button2").RectFull;
        //ImRect button3_rect = ctx->ItemInfo("Button3").RectFull;
        ImRect button4_rect = ctx->ItemInfo("Button4").RectFull;
        //ImRect button5_rect = ctx->ItemInfo("Button5").RectFull;

        // Test setup from #7514 (mostly timing)
        ctx->MouseMove("Button0");
        vars.Status[0].Clear();
        vars.Status[1].Clear();
        ctx->Yield(3);
        IM_CHECK_EQ(vars.Status[0].Hovered, 3);
        IM_CHECK_EQ(vars.Status[1].Hovered, 0);

        ctx->MouseTeleportToPos(button1_rect.GetCenter(), ImGuiTestOpFlags_NoYield);
        ctx->Yield();
        IM_CHECK_EQ(vars.Status[0].Hovered, 3);
        IM_CHECK_EQ(vars.Status[1].Hovered, 1);
        ctx->Yield();
        IM_CHECK_EQ(vars.Status[0].Hovered, 3);
        IM_CHECK_EQ(vars.Status[1].Hovered, 2);

        ctx->MouseTeleportToPos(button0_rect.GetCenter(), ImGuiTestOpFlags_NoYield);
        ctx->Yield();
        IM_CHECK_EQ(vars.Status[0].Hovered, 3); // On this frame nothing is hovered (#7514)
        IM_CHECK_EQ(vars.Status[1].Hovered, 2);
        ctx->Yield();
        IM_CHECK_EQ(vars.Status[0].Hovered, 4); // Resume hovering
        IM_CHECK_EQ(vars.Status[1].Hovered, 2);

        // Test setup from #7515 (timing + duplicate hover)
        ctx->MouseTeleportToPos(button2_rect.GetTL()); // Top: not overlapping Button 3
        vars.Status[2].Clear();
        vars.Status[3].Clear();
        ctx->Yield(3);
        IM_CHECK_EQ(vars.Status[2].Hovered, 3);
        IM_CHECK_EQ(vars.Status[3].Hovered, 0);
        ctx->MouseTeleportToPos(ImVec2(button2_rect.GetCenter().x, button2_rect.Max.y - 1), ImGuiTestOpFlags_NoYield); // Overlapping Button 2 and Button 3
        ctx->Yield();
#if IMGUI_BROKEN_TESTS == 0
        IM_CHECK_EQ(vars.Status[2].Hovered, 4); // FIXME: #7515: two items reporting as IsItemHovered()
        IM_CHECK_EQ(vars.Status[3].Hovered, 1); // FIXME: #7515
#endif
        ctx->Yield();
        IM_CHECK_EQ(vars.Status[2].Hovered, 4);
        IM_CHECK_EQ(vars.Status[3].Hovered, 2);

        // Test how IsItemHovered() doesn't mind g.HoveredId, probaaaably should?
        ctx->MouseTeleportToPos(button4_rect.GetTL()); // Top: not overlapping Button 4
        vars.Status[4].Clear();
        vars.Status[5].Clear();
        ctx->Yield(3);
        IM_CHECK_EQ(vars.Status[4].Hovered, 3);
        IM_CHECK_EQ(vars.Status[5].Hovered, 0);
        ctx->MouseTeleportToPos(ImVec2(button4_rect.GetCenter().x, button4_rect.Max.y - 1), ImGuiTestOpFlags_NoYield); // Overlapping Button 4 and Button 5
        ctx->Yield();
#if IMGUI_BROKEN_TESTS == 0
        IM_CHECK_EQ(vars.Status[4].Hovered, 4); // FIXME: Two items always reporting as IsItemHovered()
        IM_CHECK_EQ(vars.Status[5].Hovered, 1); //
        ctx->Yield();
        IM_CHECK_EQ(vars.Status[4].Hovered, 5); // FIXME: Two items always reporting as IsItemHovered()
        IM_CHECK_EQ(vars.Status[5].Hovered, 2);
#endif
    };

    // ## Check input of InputScalar().
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputscalar_input");
    struct InputScalarStepVars { int Int = 0; float Float = 0.0f; double Double = 0.0; };
    t->SetVarsDataType<InputScalarStepVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        InputScalarStepVars& vars = ctx->GetVars<InputScalarStepVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
#if IMGUI_VERSION_NUM >= 18717 // GetID() used to cause interferences by setting g.ActiveIdIsAlive (#5181)
        ImGui::GetID("Int");
        ImGui::GetID("Float");
        ImGui::GetID("Double");
#endif
        ImGui::InputInt("Int", &vars.Int, 2);
        ImGui::InputFloat("Float", &vars.Float, 1.5f);
        ImGui::InputDouble("Double", &vars.Double, 1.5);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        InputScalarStepVars& vars = ctx->GetVars<InputScalarStepVars>();
        ctx->SetRef("Test Window");
        ctx->ItemInputValue("Int", 42);
        IM_CHECK(vars.Int == 42);
        ctx->ItemInputValue("Float", 42.1f);
        IM_CHECK(vars.Float == 42.1f);
        ctx->ItemInputValue("Double", "123.456789");
        IM_CHECK(vars.Double == 123.456789);
    };

    // ## Check step buttons of InputScalar().
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputscalar_step");
    t->SetVarsDataType<InputScalarStepVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        InputScalarStepVars& vars = ctx->GetVars<InputScalarStepVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::InputInt("Int", &vars.Int, 2);
        ImGui::InputFloat("Float", &vars.Float, 1.5f);
        ImGui::InputDouble("Double", &vars.Double, 1.5);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        InputScalarStepVars& vars = ctx->GetVars<InputScalarStepVars>();
        ctx->SetRef("Test Window");

        ctx->ItemClick("Int/+");
        IM_CHECK(vars.Int == +2);
        ctx->ItemClick("Int/-");
        ctx->ItemClick("Int/-");
        IM_CHECK(vars.Int == -2);

        ctx->ItemClick("Float/+");
        IM_CHECK(vars.Float == +1.5f);
        ctx->ItemClick("Float/-");
        ctx->ItemClick("Float/-");
        IM_CHECK(vars.Float == -1.5f);

        ctx->ItemClick("Double/+");
        IM_CHECK(vars.Double == +1.5);
        ctx->ItemClick("Double/-");
        ctx->ItemClick("Double/-");
        IM_CHECK(vars.Double == -1.5);
    };

    // ## Test ImGui::InputScalar() handling overflow for different data types
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputscalar_overflow");
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

    // ## Test ImGui::InputScalar() formatting edgecases.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputscalar_formats");
    struct InputScalarFormatsVars { Str30 Format; int Int = 0; Str30 Text; int CaptureWidget = -1; };
    t->SetVarsDataType<InputScalarFormatsVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        InputScalarFormatsVars& vars = ctx->GetVars<InputScalarFormatsVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ctx->IsGuiFuncOnly())
            ImGui::InputText("Format", &vars.Format);

        for (int i = 0; i < 3; i++)
        {
            ImGui::LogToBuffer();
            if (i == 0)
                ImGui::DragInt("Drag", &vars.Int, 1.0f, 0, 0, vars.Format.c_str());
            else if (i == 1)
                ImGui::InputScalar("Input", ImGuiDataType_S32, &vars.Int, NULL, NULL, vars.Format.c_str());
            else if (i == 2)
                ImGui::SliderInt("Slider", &vars.Int, -10000, +10000, vars.Format.c_str());
            if (vars.CaptureWidget == i)
            {
                vars.Text = g.LogBuffer.c_str();
                vars.CaptureWidget = -1;
            }
            ImGui::LogFinish();
        }

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        InputScalarFormatsVars& vars = ctx->GetVars<InputScalarFormatsVars>();
        ctx->SetRef("Test Window");
        auto capture_widget_value = [ctx, &vars](int i)
        {
            vars.Text.clear();
            vars.CaptureWidget = i;
            ctx->Yield();
            if (char* curly = strstr(vars.Text.c_str(), "}"))
                curly[1] = 0;   // Strip widget name
            return vars.Text.c_str();
        };

        const char* widget_names[] = { "Drag", "Input", "Slider" };
        for (int i = 0; i < 3; i++)
        {
            const char* widget_name = widget_names[i];
            ctx->LogDebug("Widget: %s", widget_name);

#if IMGUI_VERSION_NUM >= 18714
            vars.Int = 0;
            vars.Format = "%03X";
            ctx->ItemInput(widget_name);
            ctx->KeyCharsReplaceEnter("7b");
            IM_CHECK_STR_EQ(capture_widget_value(i), "{ 07B }");
            IM_CHECK_EQ(vars.Int, 0x7B);
#endif

#if IMGUI_VERSION_NUM >= 18715
            vars.Int = 0;
            ctx->ItemInput(widget_name);
            ctx->KeyCharsReplaceEnter("FF");
            IM_CHECK_STR_EQ(capture_widget_value(i), "{ 0FF }");
            IM_CHECK_EQ(vars.Int, 0xFF);

            vars.Int = 0;
            ctx->ItemInput(widget_name);
            ctx->KeyCharsReplaceEnter("1FF");
            IM_CHECK_STR_EQ(capture_widget_value(i), "{ 1FF }");
            IM_CHECK_EQ(vars.Int, 0x1FF);
#endif

            vars.Int = 0;
            vars.Format = "%03d";
            ctx->ItemInput(widget_name);
            ctx->KeyCharsReplaceEnter("1234");
            IM_CHECK_STR_EQ(capture_widget_value(i), "{ 1234 }");
            IM_CHECK_EQ(vars.Int, 1234);

#if IMGUI_VERSION_NUM >= 18715
            vars.Int = 0;
            vars.Format = "%03d";
            ctx->ItemInput(widget_name);
            ctx->KeyCharsReplaceEnter("1235");
            IM_CHECK_STR_EQ(capture_widget_value(i), "{ 1235 }");
            IM_CHECK_EQ(vars.Int, 1235);

            vars.Int = 0;
            vars.Format = "%.03d";
            ctx->ItemInput(widget_name);
            ctx->KeyCharsReplaceEnter("1236");
            IM_CHECK_STR_EQ(capture_widget_value(i), "{ 1236 }");
            IM_CHECK_EQ(vars.Int, 1236);
#endif
        }
    };

    // ## Test that tight tab bar does not create extra drawcalls
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_drawcalls");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTabBar("Tab Drawcalls"))
        {
            for (int i = 0; i < 20; i++)
                if (ImGui::BeginTabItem(Str30f("Tab %d", i).c_str()))
                    ImGui::EndTabItem();
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
        IM_CHECK(draw_calls >= window->DrawList->CmdBuffer.Size); // May create less :)
    };

    // ## Test selection behavior of current tab
    // (this is incomplete, mostly added to regress test for #7914)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_select");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        vars.Int2 = -1; // Open popup
        if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_Reorderable))
        {
            for (int n = 0; n < 4; n++)
            {
                bool open = ImGui::BeginTabItem(Str30f("Tab %d", n).c_str());
                if (open)
                {
                    vars.Int1 = n;
                    ImGui::EndTabItem();
                }

                // Intentionally test for IsMouseClicked() and not IsMouseReleased() to test for #7914 where release reselected and closed popup.
                // Note that #7914 was on docked Begin(), here we test at a lower level. But popup id becomes slightly more awkward if we
                // had this code inside the BeginTabItem() block.
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                    ImGui::OpenPopup(Str30f("Popup %d", n).c_str());
                if (ImGui::BeginPopup(Str30f("Popup %d", n).c_str()))
                {
                    vars.Int2 = n;
                    ImGui::Text("Popup %d", n);
                    ImGui::MenuItem("Close");
                    ImGui::EndPopup();
                }
            }
            ImGui::EndTabBar();

            ImGui::Separator();
            ImGui::Text("selected: %d, open: %d", vars.Int1, vars.Int2);
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->MouseMove("TabBar/Tab 1");
        ctx->MouseClick();
        IM_CHECK(vars.Int1 == 1); // Tab selected
        IM_CHECK(vars.Int2 == -1);// Popup not open
        ctx->MouseMove("TabBar/Tab 0");
        ctx->MouseDown(ImGuiMouseButton_Right);
        ctx->Yield(); // Takes an extra frame
        IM_CHECK(vars.Int1 == 0); // Tab selected
        IM_CHECK(vars.Int2 == 0); // Popup open
        ctx->SleepShort();
        ctx->MouseDown(ImGuiMouseButton_Left);
        IM_CHECK(vars.Int1 == 0); // Tab selected
        IM_CHECK(vars.Int2 == 0); // Popup still open
    };

    // ## Test order of tabs in a tab bar
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_order");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        for (int n = 0; n < 4; n++)
            ImGui::Checkbox(Str30f("Open Tab %d", n).c_str(), &vars.BoolArray[n]);
        if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_Reorderable))
        {
            for (int n = 0; n < 4; n++)
                if (vars.BoolArray[n] && ImGui::BeginTabItem(Str30f("Tab %d", n).c_str(), &vars.BoolArray[n]))
                    ImGui::EndTabItem();
            ImGui::EndTabBar();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ImGuiTabBar* tab_bar = g.TabBars.GetOrAddByKey(ctx->GetID("TabBar")); // FIXME-TESTS: Helper function?
        IM_CHECK(tab_bar != NULL);
        IM_CHECK(tab_bar->Tabs.Size == 0);

        vars.BoolArray[0] = vars.BoolArray[1] = vars.BoolArray[2] = true;
        ctx->Yield();
        ctx->Yield(); // Important: so tab layout are correct for TabClose()
        ctx->Yield(); // Following "widgets_tabbar_select" // FIXME-TESTS: May be worth considering a second attempt in MouseMove() when item has moved?
        IM_CHECK(tab_bar->Tabs.Size == 3);
        IM_CHECK_STR_EQ(ImGui::TabBarGetTabName(tab_bar, &tab_bar->Tabs[0]), "Tab 0");
        IM_CHECK_STR_EQ(ImGui::TabBarGetTabName(tab_bar, &tab_bar->Tabs[1]), "Tab 1");
        IM_CHECK_STR_EQ(ImGui::TabBarGetTabName(tab_bar, &tab_bar->Tabs[2]), "Tab 2");

        ctx->TabClose("TabBar/Tab 1");
        ctx->Yield();
        ctx->Yield();
        IM_CHECK(vars.BoolArray[1] == false);
        IM_CHECK(tab_bar->Tabs.Size == 2);
        IM_CHECK_STR_EQ(ImGui::TabBarGetTabName(tab_bar, &tab_bar->Tabs[0]), "Tab 0");
        IM_CHECK_STR_EQ(ImGui::TabBarGetTabName(tab_bar, &tab_bar->Tabs[1]), "Tab 2");

        vars.BoolArray[1] = true;
        ctx->Yield();
        IM_CHECK(tab_bar->Tabs.Size == 3);
        IM_CHECK_STR_EQ(ImGui::TabBarGetTabName(tab_bar, &tab_bar->Tabs[0]), "Tab 0");
        IM_CHECK_STR_EQ(ImGui::TabBarGetTabName(tab_bar, &tab_bar->Tabs[1]), "Tab 2");
        IM_CHECK_STR_EQ(ImGui::TabBarGetTabName(tab_bar, &tab_bar->Tabs[2]), "Tab 1");
    };

    // ## (Attempt to) Test that tab bar declares its unclipped size.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_size");
    struct TabBarVars { bool HasCloseButton = false; float ExpectedWidth = 0.0f; };
    t->SetVarsDataType<TabBarVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GetVars<TabBarVars>();

        // FIXME-TESTS: Ideally we would test variation of with/without ImGuiTabBarFlags_TabListPopupButton, but we'd need to know its width...
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Checkbox("HasCloseButton", &vars.HasCloseButton);
        if (ImGui::BeginTabBar("TabBar"))
        {
            vars.ExpectedWidth = 0.0f;
            for (int i = 0; i < 3; i++)
            {
                Str30f label("TabItem %d", i);
                bool tab_open = true;
                if (ImGui::BeginTabItem(label.c_str(), vars.HasCloseButton ? &tab_open : NULL))
                    ImGui::EndTabItem();
                if (i > 0)
                    vars.ExpectedWidth += g.Style.ItemInnerSpacing.x;
                vars.ExpectedWidth += ImGui::TabItemCalcSize(label.c_str(), vars.HasCloseButton).x;
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window = ImGui::FindWindowByName("Test Window");
        auto& vars = ctx->GetVars<TabBarVars>();

        vars.HasCloseButton = false;
        ctx->Yield();
        IM_CHECK_EQ(window->DC.CursorStartPos.x + vars.ExpectedWidth, window->DC.IdealMaxPos.x);

        vars.HasCloseButton = true;
        ctx->Yield(); // BeginTabBar() will submit old size --> TabBarLayout update sizes
        ctx->Yield(); // BeginTabBar() will submit new size
        IM_CHECK_EQ(window->DC.CursorStartPos.x + vars.ExpectedWidth, window->DC.IdealMaxPos.x);
    };

    // ## Test TabItemButton behavior
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_tabitem_button");
    struct TabBarButtonVars { int LastClickedButton = -1; };
    t->SetVarsDataType<TabBarButtonVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TabBarButtonVars& vars = ctx->GetVars<TabBarButtonVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTabBar("TabBar"))
        {
            if (ImGui::TabItemButton("1", ImGuiTabItemFlags_None))        { vars.LastClickedButton = 1; }
            if (ImGui::TabItemButton("0", ImGuiTabItemFlags_None))        { vars.LastClickedButton = 0; }
            if (ImGui::BeginTabItem("Tab", NULL, ImGuiTabItemFlags_None)) { ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TabBarButtonVars& vars = ctx->GetVars<TabBarButtonVars>();
        ctx->SetRef("Test Window/TabBar");

        IM_CHECK_EQ(vars.LastClickedButton, -1);
        ctx->ItemClick("1");
        IM_CHECK_EQ(vars.LastClickedButton, 1);
        ctx->ItemClick("Tab");
        IM_CHECK_EQ(vars.LastClickedButton, 1);
        ctx->MouseMove("0");
        ctx->MouseDown();
        IM_CHECK_EQ(vars.LastClickedButton, 1);
        ctx->MouseUp();
        IM_CHECK_EQ(vars.LastClickedButton, 0);
    };

    // ## Test that tab items respects their Leading/Trailing position
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_tabitem_leading_trailing");
    struct TabBarLeadingTrailingVars { bool WindowAutoResize = true; ImGuiTabBarFlags TabBarFlags = 0; ImGuiTabBar* TabBar = NULL; };
    t->SetVarsDataType<TabBarLeadingTrailingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        TabBarLeadingTrailingVars& vars = ctx->GetVars<TabBarLeadingTrailingVars>();
        ImGui::Begin("Test Window", NULL, (vars.WindowAutoResize ? ImGuiWindowFlags_AlwaysAutoResize : 0) | ImGuiWindowFlags_NoSavedSettings);
        ImGui::Checkbox("ImGuiWindowFlags_AlwaysAutoResize", &vars.WindowAutoResize);
        if (ImGui::BeginTabBar("TabBar", vars.TabBarFlags))
        {
            vars.TabBar = g.CurrentTabBar;
            if (ImGui::BeginTabItem("Trailing", NULL, ImGuiTabItemFlags_Trailing)) { ImGui::EndTabItem(); } // Intentionally submit Trailing tab early and Leading tabs at the end
            if (ImGui::BeginTabItem("Tab 0", NULL, ImGuiTabItemFlags_None))        { ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Tab 1", NULL, ImGuiTabItemFlags_None))        { ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Tab 2", NULL, ImGuiTabItemFlags_None))        { ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Leading", NULL, ImGuiTabItemFlags_Leading))   { ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
        ImGui::End();
    };
#if IMGUI_VERSION_NUM < 19213
#define ImGuiTabBarFlags_FittingPolicyShrink ImGuiTabBarFlags_FittingPolicyResizeDown
#endif
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GetVars<TabBarLeadingTrailingVars>();

        vars.TabBarFlags = ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyShrink;
        ctx->Yield();

        ctx->SetRef("Test Window/TabBar");
        const char* tabs[] = { "Leading", "Tab 0", "Tab 1", "Tab 2", "Trailing" };

        // Check that tabs relative order matches what we expect (which is not the same as submission order above)
        float offset_x = -FLT_MAX;
        for (int i = 0; i < IM_ARRAYSIZE(tabs); ++i)
        {
            ctx->MouseMove(tabs[i]);
            IM_CHECK_GT(g.IO.MousePos.x, offset_x);
            offset_x = g.IO.MousePos.x;
        }

        // Test that "Leading" cannot be reordered over "Tab 0" and vice-versa
        ctx->ItemDragAndDrop("Leading", "Tab 0");
        IM_CHECK_EQ(vars.TabBar->Tabs[0].ID, ctx->GetID("Leading"));
        IM_CHECK_EQ(vars.TabBar->Tabs[1].ID, ctx->GetID("Tab 0"));
        ctx->ItemDragAndDrop("Tab 0", "Leading");
        IM_CHECK_EQ(vars.TabBar->Tabs[0].ID, ctx->GetID("Leading"));
        IM_CHECK_EQ(vars.TabBar->Tabs[1].ID, ctx->GetID("Tab 0"));

        // Test that "Trailing" cannot be reordered over "Tab 2" and vice-versa
        ctx->ItemDragAndDrop("Trailing", "Tab 2");
        IM_CHECK_EQ(vars.TabBar->Tabs[4].ID, ctx->GetID("Trailing"));
        IM_CHECK_EQ(vars.TabBar->Tabs[3].ID, ctx->GetID("Tab 2"));
        ctx->ItemDragAndDrop("Tab 2", "Trailing");
        IM_CHECK_EQ(vars.TabBar->Tabs[4].ID, ctx->GetID("Trailing"));
        IM_CHECK_EQ(vars.TabBar->Tabs[3].ID, ctx->GetID("Tab 2"));

        // Resize down
        vars.WindowAutoResize = false;
        ImGuiWindow* window = ctx->GetWindowByRef("//Test Window");
        ctx->WindowResize("//Test Window", ImVec2(window->Size.x * 0.3f, window->Size.y));
        for (int i = 0; i < 2; ++i)
        {
            vars.TabBarFlags = ImGuiTabBarFlags_Reorderable | (i == 0 ? ImGuiTabBarFlags_FittingPolicyShrink : ImGuiTabBarFlags_FittingPolicyScroll);
            ctx->Yield();
            IM_CHECK_GT(ctx->ItemInfo("Leading").RectClipped.GetWidth(), 1.0f);
            IM_CHECK_EQ(ctx->ItemInfo("Tab 0").RectClipped.GetWidth(), 0.0f);
            IM_CHECK_EQ(ctx->ItemInfo("Tab 1").RectClipped.GetWidth(), 0.0f);
            IM_CHECK_EQ(ctx->ItemInfo("Tab 2").RectClipped.GetWidth(), 0.0f);
            IM_CHECK_GT(ctx->ItemInfo("Trailing").RectClipped.GetWidth(), 1.0f);
        }
    };

    // ## Test reordering tabs (and ImGuiTabItemFlags_NoReorder flag)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_reorder");
    struct TabBarReorderVars { ImGuiTabBarFlags Flags = ImGuiTabBarFlags_Reorderable; ImGuiTabBar* TabBar = NULL; };
    t->SetVarsDataType<TabBarReorderVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GetVars<TabBarReorderVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTabBar("TabBar", vars.Flags))
        {
            vars.TabBar = g.CurrentTabBar;
            if (ImGui::BeginTabItem("Tab 0", NULL, ImGuiTabItemFlags_None))      { ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Tab 1", NULL, ImGuiTabItemFlags_None))      { ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Tab 2", NULL, ImGuiTabItemFlags_NoReorder)) { ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Tab 3", NULL, ImGuiTabItemFlags_None))      { ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TabBarReorderVars& vars = ctx->GetVars<TabBarReorderVars>();

        // Reset reorderable flags to ensure tabs are in their submission order
        vars.Flags = ImGuiTabBarFlags_None;
        ctx->Yield();
        vars.Flags = ImGuiTabBarFlags_Reorderable;
        ctx->Yield();

        ctx->SetRef("Test Window/TabBar");

        ctx->ItemDragAndDrop("Tab 0", "Tab 1");
        IM_CHECK_EQ(vars.TabBar->Tabs[0].ID, ctx->GetID("Tab 1"));
        IM_CHECK_EQ(vars.TabBar->Tabs[1].ID, ctx->GetID("Tab 0"));

        ctx->ItemDragAndDrop("Tab 0", "Tab 1");
        IM_CHECK_EQ(vars.TabBar->Tabs[0].ID, ctx->GetID("Tab 0"));
        IM_CHECK_EQ(vars.TabBar->Tabs[1].ID, ctx->GetID("Tab 1"));

        ctx->ItemDragAndDrop("Tab 0", "Tab 2"); // Tab 2 has NoReorder flag
        ctx->ItemDragAndDrop("Tab 0", "Tab 3"); // Tab 2 has NoReorder flag
        ctx->ItemDragAndDrop("Tab 3", "Tab 2"); // Tab 2 has NoReorder flag
        IM_CHECK_EQ(vars.TabBar->Tabs[1].ID, ctx->GetID("Tab 0"));
        IM_CHECK_EQ(vars.TabBar->Tabs[2].ID, ctx->GetID("Tab 2"));
        IM_CHECK_EQ(vars.TabBar->Tabs[3].ID, ctx->GetID("Tab 3"));
    };

    // ## Test nested/recursing Tab Bars (Bug #2371)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_nested");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::BeginTabBar("TabBar 0"))
        {
            if (ImGui::BeginTabItem("TabItem"))
            {
                // If we have many tab bars here, it will invalidate pointers from pooled tab bars
                for (int i = 0; i < 10; i++)
                    if (ImGui::BeginTabBar(Str30f("Inner TabBar %d", i).c_str()))
                    {
                        if (ImGui::BeginTabItem("Inner TabItem"))
                            ImGui::EndTabItem();
                        ImGui::EndTabBar();
                    }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    };

    // ## Test BeginTabBar() append
    struct TabBarMultipleSubmissionVars { bool AppendToTabBar = true; ImVec2 CursorAfterActiveTab; ImVec2 CursorAfterFirstBeginTabBar; ImVec2 CursorAfterFirstWidget; ImVec2 CursorAfterSecondBeginTabBar; ImVec2 CursorAfterSecondWidget; ImVec2 CursorAfterSecondEndTabBar; };
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_append");
    t->SetVarsDataType<TabBarMultipleSubmissionVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GetVars<TabBarMultipleSubmissionVars>();

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Checkbox("AppendToTabBar", &vars.AppendToTabBar);
        if (ImGui::BeginTabBar("TabBar"))
        {
            vars.CursorAfterFirstBeginTabBar = g.CurrentWindow->DC.CursorPos;
            if (ImGui::BeginTabItem("Tab 0"))
            {
                ImGui::Text("Tab 0");
                ImGui::EndTabItem();
                vars.CursorAfterActiveTab = g.CurrentWindow->DC.CursorPos;
            }
            if (ImGui::BeginTabItem("Tab 1"))
            {
                for (int i = 0; i < 3; i++)
                    ImGui::Text("Tab 1 Line %d", i);
                ImGui::EndTabItem();
                vars.CursorAfterActiveTab = g.CurrentWindow->DC.CursorPos;
            }
            if (ImGui::BeginTabItem("Tab 2"))
            {
                ImGui::EndTabItem();
                vars.CursorAfterActiveTab = g.CurrentWindow->DC.CursorPos;
            }
            ImGui::EndTabBar();
        }
        ImGui::Text("After first TabBar submission");
        vars.CursorAfterFirstWidget = g.CurrentWindow->DC.CursorPos;

        if (vars.AppendToTabBar && ImGui::BeginTabBar("TabBar"))
        {
            vars.CursorAfterSecondBeginTabBar = g.CurrentWindow->DC.CursorPos;
            if (ImGui::BeginTabItem("Tab A"))
            {
                ImGui::Text("I'm tab A");
                ImGui::EndTabItem();
                vars.CursorAfterActiveTab = g.CurrentWindow->DC.CursorPos;
            }
            ImGui::EndTabBar();
            vars.CursorAfterSecondEndTabBar = g.CurrentWindow->DC.CursorPos;
        }
        ImGui::Text("After second TabBar submission");
        vars.CursorAfterSecondWidget = g.CurrentWindow->DC.CursorPos;

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GetVars<TabBarMultipleSubmissionVars>();

        ctx->SetRef("Test Window/TabBar");

        const float line_height = g.FontSize + g.Style.ItemSpacing.y;
        for (bool append_to_tab_bar : { false, true })
        {
            vars.AppendToTabBar = append_to_tab_bar;
            ctx->Yield();

            for (const char* tab_name : { "Tab 0", "Tab 1", "Tab A" })
            {
                if (!append_to_tab_bar && strcmp(tab_name, "Tab A") == 0)
                    continue;

                ctx->ItemClick(tab_name);
                ctx->Yield();

                float active_tab_height = line_height;
                if (strcmp(tab_name, "Tab 1") == 0)
                    active_tab_height *= 3;

                IM_CHECK(vars.CursorAfterActiveTab.y == vars.CursorAfterFirstBeginTabBar.y + active_tab_height);
                IM_CHECK(vars.CursorAfterFirstWidget.y == vars.CursorAfterActiveTab.y + line_height);
                if (append_to_tab_bar)
                {
                    IM_CHECK(vars.CursorAfterSecondBeginTabBar.y == vars.CursorAfterFirstBeginTabBar.y);
                    IM_CHECK(vars.CursorAfterSecondEndTabBar.y == vars.CursorAfterFirstWidget.y);
                }
                IM_CHECK(vars.CursorAfterSecondWidget.y == vars.CursorAfterFirstWidget.y + line_height);
            }
        }
    };


#ifdef IMGUI_HAS_DOCK
    // ## Test Dockspace within a TabItem
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_dockspace");
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
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_tabitem_setselected");
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

    // ## Test SetNextItemWidth() with TabItem(). (#5262)
#if IMGUI_VERSION_NUM >= 18732
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_tabitem_setwidth");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        const ImGuiTabItemFlags test_flags[] = { ImGuiTabItemFlags_None, ImGuiTabItemFlags_Trailing, ImGuiTabItemFlags_Leading, ImGuiTabItemFlags_None };
        vars.Count = IM_ARRAYSIZE(test_flags);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::BeginTabBar("TabBar"))
        {
            for (int i = 0; i < IM_ARRAYSIZE(test_flags); i++)
            {
                ImGui::SetNextItemWidth(30.0f + i * 10.0f);
                if (ImGui::BeginTabItem(Str16f("Tab %d", i).c_str(), NULL, test_flags[i]))
                {
                    ImGui::TextUnformatted(Str16f("Content %d", i).c_str());
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        for (int i = 0; i < vars.Count; i++)
        {
            ImGuiTestItemInfo item_info = ctx->ItemInfo(Str30f("TabBar/Tab %d", i).c_str());
            IM_CHECK(item_info.ID != 0);
            IM_CHECK_EQ(item_info.RectFull.GetWidth(), 30.0f + i * 10.0f);
        }
    };
#endif

    // ## Tests: Coverage: TabBar: TabBarTabListPopupButton() and TabBarScrollingButtons()
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_popup_scrolling_button");
    struct TabBarCoveragePopupScrolling { int TabCount = 9; int Selected = -1; };
    t->SetVarsDataType<TabBarCoveragePopupScrolling>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TabBarCoveragePopupScrolling>();
        ImGui::SetNextWindowSize(ImVec2(200, 100));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_TabListPopupButton | ImGuiTabBarFlags_FittingPolicyScroll))
        {
            for (int i = 0; i < vars.TabCount; i++)
                if (ImGui::BeginTabItem(Str16f{ "Tab %d", i }.c_str(), NULL)) { vars.Selected = i; ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TabBarCoveragePopupScrolling>();
        ctx->SetRef("Test Window");
        ctx->ItemClick("TabBar/Tab 0"); // Ensure first tab is selected

        for (int i = 0; i < vars.TabCount; i++)
        {
            ctx->ItemClick("TabBar/##<");
            ctx->Yield();
            IM_CHECK_EQ(vars.Selected, i == 0 ? 0 : i - 1);

            ctx->ItemClick("TabBar/##v");
            ctx->ItemClick(Str64f("//##Combo_00/Tab %d", i).c_str());
            ctx->Yield();
            IM_CHECK_EQ(vars.Selected, i);

            ctx->ItemClick("TabBar/##>");
            ctx->Yield();
            IM_CHECK_EQ(vars.Selected, i == vars.TabCount - 1 ? vars.TabCount - 1 : i + 1);
        }

        // Click on all even tab
        for (int i = 0; i < vars.TabCount / 2; i++)
        {
            const int even_i = i * 2;
            ctx->ItemClick(Str64f("TabBar/Tab %d", even_i).c_str());
            IM_CHECK_EQ(vars.Selected, even_i);
        }

        // Click on all odd tab
        for (int i = 0; i < vars.TabCount / 2; i++)
        {
            const int odd_i = i * 2 + 1;
            ctx->ItemClick(Str64f("TabBar/Tab %d", odd_i).c_str());
            IM_CHECK_EQ(vars.Selected, odd_i);
        }
    };

    // ## Tests: test TabBarQueueFocus() on appearing frame (#8929, #6681)
#if IMGUI_VERSION_NUM >= 19227
    t = IM_REGISTER_TEST(e, "widgets", "widgets_tabbar_focus_tab_on_first_frame");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        // Forcefully clear existing data, if any
        if (ctx->IsFirstGuiFrame())
            if (ImGuiTabBar* tab_bar = ImGui::TabBarFindByID(ImGui::GetID("##TabBar")))
                ImGui::TabBarRemove(tab_bar);

        if (ImGui::BeginTabBar("##TabBar"))
        {
            if (vars.Bool1 == false)
            {
                // Focus once
                ImGui::TabBarQueueFocus(ImGui::GetCurrentTabBar(), "BBB");
                vars.Bool1 = true;
            }
            if (ImGui::BeginTabItem("AAA"))
                ImGui::EndTabItem();
            if (ImGui::BeginTabItem("BBB"))
                ImGui::EndTabItem();
            if (ImGui::BeginTabItem("CCC"))
                ImGui::EndTabItem();
            IM_CHECK_EQ(ImGui::GetCurrentTabBar()->SelectedTabId, ImGui::GetID("BBB")); // GuiFunc without TestFunc will run for 2 frames
            ImGui::EndTabBar();
        }
        ImGui::End();
    };
#endif

    // ## Test various TreeNode flags
    t = IM_REGISTER_TEST(e, "widgets", "widgets_treenode_behaviors");
    struct TreeNodeTestVars
    {
        bool                    Reset = true, IsOpen = false, IsMultiSelect = false;
        int                     ToggleCount = 0;
        int                     DragSourceCount = 0;
        ImGuiTreeNodeFlags      TreeNodeFlags = 0;
#ifdef IMGUI_HAS_MULTISELECT
        ImGuiMultiSelectFlags   MultiSelectFlags = 0;
#endif
    };
    t->SetVarsDataType<TreeNodeTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGui::ColorButton("Color", ImVec4(1.0f, 1.0f, 0.0f, 1.0f), ImGuiColorEditFlags_NoTooltip); // To test open-on-drag-hold

        TreeNodeTestVars& vars = ctx->GetVars<TreeNodeTestVars>();
        if (vars.Reset)
        {
            ImGui::GetStateStorage()->SetInt(ImGui::GetID("AAA"), 0);
            vars.ToggleCount = vars.DragSourceCount = 0;
        }
        vars.Reset = false;

#ifdef IMGUI_HAS_MULTISELECT
        ImGui::Text("TreeNodeFlags: 0x%08X, MultiSelect: %d (flags=%08X)", vars.TreeNodeFlags, vars.IsMultiSelect, vars.MultiSelectFlags);
        ImGui::CheckboxFlags("ImGuiTreeNodeFlags_OpenOnArrow", &vars.TreeNodeFlags, ImGuiTreeNodeFlags_OpenOnArrow);
        ImGui::CheckboxFlags("ImGuiTreeNodeFlags_OpenOnDoubleClick", &vars.TreeNodeFlags, ImGuiTreeNodeFlags_OpenOnDoubleClick);
        ImGui::Checkbox("IsMultiSelect", &vars.IsMultiSelect);
        ImGui::CheckboxFlags("ImGuiMultiSelectFlags_SelectOnClick", &vars.MultiSelectFlags, ImGuiMultiSelectFlags_SelectOnClick);
        ImGui::CheckboxFlags("ImGuiMultiSelectFlags_SelectOnClickRelease", &vars.MultiSelectFlags, ImGuiMultiSelectFlags_SelectOnClickRelease);

        if (vars.IsMultiSelect)
            ImGui::BeginMultiSelect(vars.MultiSelectFlags); // Placeholder, won't interact properly
#else
        ImGui::Text("TreeNodeFlags: 0x%08X, MultiSelect: %d", vars.TreeNodeFlags, vars.IsMultiSelect);
#endif

        if (vars.IsMultiSelect)
            ImGui::SetNextItemSelectionUserData(0); // To enable MultiSelect logic
        vars.IsOpen = ImGui::TreeNodeEx("AAA", vars.TreeNodeFlags);
        if (ImGui::IsItemToggledOpen())
            vars.ToggleCount++;
        if (ImGui::BeginDragDropSource())
        {
            vars.DragSourceCount++;
            ImGui::SetDragDropPayload("_TREENODE", NULL, 0);
            ImGui::Text("Drag Source Tooltip");
            ImGui::EndDragDropSource();
        }
        if (vars.IsOpen)
        {
            ImGui::Text("Contents");
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("BBB", vars.TreeNodeFlags))
            ImGui::TreePop();

#ifdef IMGUI_HAS_MULTISELECT
        if (vars.IsMultiSelect)
            ImGui::EndMultiSelect();
#endif

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TreeNodeTestVars& vars = ctx->GetVars<TreeNodeTestVars>();
        ctx->SetRef("Test Window");

#ifdef IMGUI_HAS_MULTISELECT
        int step_count = 3;
#else
        int step_count = 1;
#endif

        for (int step_n = 0; step_n < step_count; step_n++)
        {
            vars.IsMultiSelect = (step_n == 1 || step_n == 2);
#ifdef IMGUI_HAS_MULTISELECT
            vars.MultiSelectFlags = (step_n == 1 ? ImGuiMultiSelectFlags_SelectOnClickRelease : ImGuiMultiSelectFlags_SelectOnClick);
#endif

            {
                ctx->LogInfo("## ImGuiTreeNodeFlags_None, IsMultiSelect=%d", vars.IsMultiSelect);
                vars.Reset = true;
                vars.TreeNodeFlags = ImGuiTreeNodeFlags_None;
                ctx->Yield();
                IM_CHECK(vars.IsOpen == false && vars.ToggleCount == 0);

                // Click on arrow
                ctx->MouseMove("AAA", ImGuiTestOpFlags_MoveToEdgeL);
                ctx->MouseDown(0); // Toggle on Down when hovering Arrow
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseUp(0);
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 4);

                // Click on main section (_OpenOnArrow is implicit/automatic with MultiSelect, so Main Section won't react)
                if (!vars.IsMultiSelect)
                {
                    vars.ToggleCount = 0;
                    ctx->MouseMove("AAA");
                    ctx->MouseClick(0);
                    IM_CHECK_EQ_NO_RET(vars.IsOpen, true);
                    ctx->MouseClick(0);
                    IM_CHECK_EQ_NO_RET(vars.IsOpen, false);
                    ctx->MouseDoubleClick(0);
                    IM_CHECK_EQ_NO_RET(vars.IsOpen, false);
                    IM_CHECK_EQ_NO_RET(vars.ToggleCount, 4);
                }

                // Test TreeNode as drag source
                IM_CHECK_EQ(vars.DragSourceCount, 0);
                ctx->ItemDragWithDelta("AAA", ImVec2(50, 50));
                IM_CHECK_GT(vars.DragSourceCount, 0);
                IM_CHECK_EQ(vars.IsOpen, false);

                // Test TreeNode opening on drag-hold
                ctx->ItemDragOverAndHold("Color", "AAA");
                IM_CHECK_EQ(vars.IsOpen, true);
            }

            {
                ctx->LogInfo("## ImGuiTreeNodeFlags_OpenOnDoubleClick, IsMultiSelect=%d", vars.IsMultiSelect);
                vars.Reset = true;
                vars.TreeNodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
                ctx->Yield();
                IM_CHECK(vars.IsOpen == false && vars.ToggleCount == 0);

                // Click on arrow (_OpenOnArrow is implicit/automatic with MultiSelect)
                if (!vars.IsMultiSelect)
                {
                    ctx->MouseMove("AAA", ImGuiTestOpFlags_MoveToEdgeL);
                    ctx->MouseDown(0);
                    IM_CHECK_EQ(vars.IsOpen, false);
                    ctx->MouseUp(0);
                    IM_CHECK_EQ(vars.IsOpen, false);
                    ctx->MouseClick(0);
                    IM_CHECK_EQ(vars.IsOpen, false);
                    IM_CHECK_EQ(vars.ToggleCount, 0);
                    ctx->MouseDoubleClick(0);
                    IM_CHECK_EQ(vars.IsOpen, true);
                    ctx->MouseDoubleClick(0);
                    IM_CHECK_EQ(vars.IsOpen, false);
                    IM_CHECK_EQ(vars.ToggleCount, 2);
                }

                // Double-click on main section
                vars.ToggleCount = 0;
                ctx->MouseMove("AAA");
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 0);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 2);

                // Test TreeNode as drag source
                IM_CHECK_EQ(vars.DragSourceCount, 0);
                ctx->ItemDragWithDelta("AAA", ImVec2(50, 50));
                IM_CHECK_GT(vars.DragSourceCount, 0);
                IM_CHECK_EQ(vars.IsOpen, false);

                // Test TreeNode opening on drag-hold
                ctx->ItemDragOverAndHold("Color", "AAA");
                IM_CHECK_EQ(vars.IsOpen, true);
            }

            {
                ctx->LogInfo("## ImGuiTreeNodeFlags_OpenOnArrow, IsMultiSelect=%d", vars.IsMultiSelect);
                vars.Reset = true;
                vars.TreeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
                ctx->Yield();
                IM_CHECK(vars.IsOpen == false && vars.ToggleCount == 0);

                // Click on arrow
                ctx->MouseMove("AAA", ImGuiTestOpFlags_MoveToEdgeL);
                ctx->MouseDown(0);
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseUp(0);
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 2);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 4);

                // Click on main section
                vars.ToggleCount = 0;
                ctx->MouseMove("AAA");
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 0);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 0);

                // Test TreeNode as drag source
                IM_CHECK_EQ(vars.DragSourceCount, 0);
                ctx->ItemDragWithDelta("AAA", ImVec2(50, 50));
                IM_CHECK_GT(vars.DragSourceCount, 0);
                IM_CHECK_EQ(vars.IsOpen, false);

                // Test TreeNode opening on drag-hold
                ctx->ItemDragOverAndHold("Color", "AAA");
                IM_CHECK_EQ(vars.IsOpen, true);
            }

            {
                ctx->LogInfo("## ImGuiTreeNodeFlags_OpenOnArrow|ImGuiTreeNodeFlags_OpenOnDoubleClick, IsMultiSelect=%d", vars.IsMultiSelect);
                vars.Reset = true;
                vars.TreeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
                ctx->Yield();
                IM_CHECK(vars.IsOpen == false && vars.ToggleCount == 0);

                // Click on arrow
                ctx->MouseMove("AAA", ImGuiTestOpFlags_MoveToEdgeL);
                ctx->MouseDown(0);
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseUp(0);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 2);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 4);

                // Click on main section
                vars.ToggleCount = 0;
                ctx->MouseMove("AAA");
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                ctx->MouseClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 0);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, true);
                ctx->MouseDoubleClick(0);
                IM_CHECK_EQ(vars.IsOpen, false);
                IM_CHECK_EQ(vars.ToggleCount, 2);

                // Test TreeNode as drag source
                IM_CHECK_EQ(vars.DragSourceCount, 0);
                ctx->ItemDragWithDelta("AAA", ImVec2(50, 50));
                IM_CHECK_GT(vars.DragSourceCount, 0);
                IM_CHECK_EQ(vars.IsOpen, false);

                // Test TreeNode opening on drag-hold
                ctx->ItemDragOverAndHold("Color", "AAA");
                IM_CHECK_EQ(vars.IsOpen, true);
            }
        }
    };

    // ## Test ImGuiTreeNodeFlags_SpanAvailWidth and ImGuiTreeNodeFlags_SpanFullWidth flags
    t = IM_REGISTER_TEST(e, "widgets", "widgets_treenode_span_width");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Always);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGuiContext& g = *ImGui::GetCurrentContext();
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        ImGui::SetNextItemOpen(true);
        if (ImGui::TreeNodeEx("Parent"))
        {
            // Interaction rect zoes not span entire width of work area.
            IM_CHECK(g.LastItemData.Rect.Max.x < window->WorkRect.Max.x);
            // But it starts at very beginning of WorkRect for first tree level.
            IM_CHECK(g.LastItemData.Rect.Min.x == window->WorkRect.Min.x);
            ImGui::SetNextItemOpen(true);
            if (ImGui::TreeNodeEx("Regular"))
            {
                // Interaction rect does not span entire width of work area.
                IM_CHECK(g.LastItemData.Rect.Max.x < window->WorkRect.Max.x);
                IM_CHECK(g.LastItemData.Rect.Min.x > window->WorkRect.Min.x);
                ImGui::TreePop();
            }
            ImGui::SetNextItemOpen(true);
            if (ImGui::TreeNodeEx("SpanAvailWidth", ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                // Interaction rect matches visible frame rect
                IM_CHECK((g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HasDisplayRect) != 0);
                IM_CHECK(g.LastItemData.DisplayRect.Min == g.LastItemData.Rect.Min);
                IM_CHECK(g.LastItemData.DisplayRect.Max == g.LastItemData.Rect.Max);
                // Interaction rect extends to the end of the available area.
                IM_CHECK(g.LastItemData.Rect.Max.x == window->WorkRect.Max.x);
                ImGui::TreePop();
            }
            ImGui::SetNextItemOpen(true);
            if (ImGui::TreeNodeEx("SpanFullWidth", ImGuiTreeNodeFlags_SpanFullWidth))
            {
                // Interaction rect matches visible frame rect
                IM_CHECK((g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HasDisplayRect) != 0);
                IM_CHECK(g.LastItemData.DisplayRect.Min == g.LastItemData.Rect.Min);
                IM_CHECK(g.LastItemData.DisplayRect.Max == g.LastItemData.Rect.Max);
                // Interaction rect extends to the end of the available area.
                IM_CHECK(g.LastItemData.Rect.Max.x == window->WorkRect.Max.x);
                // ImGuiTreeNodeFlags_SpanFullWidth also extends interaction rect to the left.
                IM_CHECK(g.LastItemData.Rect.Min.x == window->WorkRect.Min.x);
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        ImGui::End();
    };

    // ## Test padding when using TreeNode() + SameLine() idioms (#7505)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_treenode_padding");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        for (int n = 0; n < 2; n++)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, (n == 0) ? ImVec2(4, 3) : ImVec2(11, 3));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, (n == 0) ? ImVec2(8, 4) : ImVec2(9, 4));;
            ImGui::PushID(n);

            float offset = ImGui::GetTreeNodeToLabelSpacing();
            IM_CHECK_EQ(offset, ImGui::GetFontSize() + style.FramePadding.x * 2.0f);

            ImVec2 p = ImGui::GetCursorScreenPos();
            {
                bool is_open = ImGui::TreeNode("Hello1");
                ImGui::SameLine();
                IM_CHECK_EQ(ImGui::GetCursorScreenPos().x, p.x + offset + ImGui::CalcTextSize("Hello1").x + style.ItemSpacing.x);
                ImGui::Text("World");
                if (is_open)
                    ImGui::TreePop();
            }
            {
                bool is_open = ImGui::TreeNode("Hello2");
                ImGui::SameLine(0, 0);
                IM_CHECK_EQ(ImGui::GetCursorScreenPos().x, p.x + offset + ImGui::CalcTextSize("Hello2").x);
                ImGui::Text("World");
                if (is_open)
                    ImGui::TreePop();
            }
#if IMGUI_VERSION_NUM >= 19052
            {
                bool is_open = ImGui::TreeNodeEx("##Hello3", 0 * ImGuiTreeNodeFlags_SpanAvailWidth);
                ImGui::SameLine();
                IM_CHECK_EQ(ImGui::GetCursorScreenPos().x, p.x + offset + style.ItemSpacing.x);
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", "World");
                if (is_open)
                    ImGui::TreePop();
            }
            {
                bool is_open = ImGui::TreeNodeEx("##Hello4", 0 * ImGuiTreeNodeFlags_SpanAvailWidth);
                ImGui::SameLine(0, 0);
                IM_CHECK_EQ(ImGui::GetCursorScreenPos().x, p.x + offset);
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", "World");
                if (is_open)
                    ImGui::TreePop();
            }
#endif
            ImGui::PopID();
            ImGui::PopStyleVar(2);
        }
        ImGui::End();
    };

    // ## Test PlotLines() with a single value (#2387).
    t = IM_REGISTER_TEST(e, "widgets", "widgets_plot_lines_unexpected_input");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        float values[1] = {0.f};
        ImGui::PlotLines("PlotLines 1", NULL, 0);
        ImGui::PlotLines("PlotLines 2", values, 0);
        ImGui::PlotLines("PlotLines 3", values, 1);
        // FIXME-TESTS: If test did not crash - it passed. A better way to check this would be useful.
    };

    // ## Test ColorEdit hex input
    t = IM_REGISTER_TEST(e, "widgets", "widgets_coloredit_hexinput");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::ColorEdit4("ColorEdit1", &vars.Color1.x, ImGuiColorEditFlags_DisplayHex);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        vars.Color1 = ImVec4(1, 0, 0, 1);

        ctx->SetRef("Test Window");

        // Compare with epsilon.
#define IM_EQUAL_EPS(a, b) (ImAbs(a - b) < 0.0000001f)
        auto equal = [](const ImVec4& a, const ImVec4& b) { return IM_EQUAL_EPS(a.x, b.x) && IM_EQUAL_EPS(a.y, b.y) && IM_EQUAL_EPS(a.z, b.z) && IM_EQUAL_EPS(a.w, b.w); };
#undef IM_EQUAL_EPS

        // Test hex inputs.
        ctx->ItemClick("ColorEdit1/##ColorButton");
        ctx->SetRef("//$FOCUSED");
        ctx->ItemInputValue("##picker/##hex/##Text", "112233");
        IM_CHECK(equal(vars.Color1, ImVec4(ImColor(0x11, 0x22, 0x33, 0xFF))));
        ctx->ItemInputValue("##picker/##hex/##Text", "11223344");
        IM_CHECK(equal(vars.Color1, ImVec4(ImColor(0x11, 0x22, 0x33, 0x44))));
        ctx->ItemInputValue("##picker/##hex/##Text", "#112233");
        IM_CHECK(equal(vars.Color1, ImVec4(ImColor(0x11, 0x22, 0x33, 0xFF))));
        ctx->ItemInputValue("##picker/##hex/##Text", "#11223344");
        IM_CHECK(equal(vars.Color1, ImVec4(ImColor(0x11, 0x22, 0x33, 0x44))));
    };

    // ## Test preserving hue and saturation for certain colors having these components undefined.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_coloredit_preserve_hs");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        int& rgbi = vars.Int1;
        ImVec4& rgbf = vars.Color1;
        ImVec4 color = (vars.Step == 0) ? rgbf : ImGui::ColorConvertU32ToFloat4(rgbi);
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Appearing);
        if (ImGui::Begin("Window", NULL, ImGuiWindowFlags_NoSavedSettings))
        {
            bool use_int = (vars.Step == 1);
            if (ImGui::Checkbox("Use int", &use_int))
                vars.Step = use_int ? 1 : 0;
            if (ImGui::ColorEdit4("Color", &color.x, ImGuiColorEditFlags_PickerHueBar))
            {
                if (vars.Step == 0)
                    rgbf = color;
                else
                    rgbi = ImGui::ColorConvertFloat4ToU32(color);
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        struct Color
        {
            float R, G, B;
            float H, S, V;
        };

        auto read_color = [&]()
        {
            int& rgbi = vars.Int1;
            ImVec4& rgbf = vars.Color1;
            ImVec4 rgb = vars.Step == 0 ? rgbf : ImGui::ColorConvertU32ToFloat4(rgbi);
            Color color;
            color.R = rgb.x;
            color.G = rgb.y;
            color.B = rgb.z;
            ImGui::ColorConvertRGBtoHSV(color.R, color.G, color.B, color.H, color.S, color.V);
            return color;
        };

        ImGuiContext& g = *ctx->UiContext;
        ctx->ItemClick("Window/Color/##ColorButton");
        ctx->SetRef("//$FOCUSED");
        ImGuiWindow* popup = g.NavWindow;

        // Variant 0: use float RGB.
        // Variant 1: use int32 RGB.
        for (int variant = 0; variant < 2; variant++)
        {
            vars.Step = variant;
            ctx->LogDebug("Test variant: %d", variant);

            ctx->ItemClick("##picker/sv");                  // Set initial color
            ctx->ItemClick("##picker/hue");

            Color color_start = read_color();
            IM_CHECK_NE(color_start.H, 0.0f);
            IM_CHECK_NE(color_start.S, 0.0f);
            IM_CHECK_NE(color_start.V, 0.0f);

            // Set saturation to 0, hue must be preserved.
            ctx->ItemDragWithDelta("##picker/sv", ImVec2(-popup->Size.x * 0.5f, 0));

            // FIXME-TESTS: This tends to break with other widget sizes, e.g. changing TestSuite font. Needs to investigate.
            Color color = read_color();
            IM_CHECK_EQ(color.S, 0.0f);
            IM_CHECK_EQ(color.H, 0.0f);                     // Hue undefined
            IM_CHECK_EQ(color_start.H, g.ColorEditSavedHue); // Preserved hue matches original color

            // Test saturation preservation during mouse input.
            ctx->ItemClick("##picker/sv");
            ctx->ItemClick("##picker/hue");
            IM_CHECK_NE(read_color().S, 0.0f);
            color_start = read_color();
            ctx->MouseDown();

            // Move mouse across hue slider (extremes)
            ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(0.0f, +popup->Size.y * 0.5f));  // Bottom
            ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(0.0f, -popup->Size.y * 1.0f));  // Top
            ctx->MouseUp();
            IM_CHECK_EQ(read_color().S, color_start.S);
            IM_CHECK_EQ(g.ColorEditSavedSat, color_start.H);

            ctx->ItemClick("##picker/sv");                  // Reset color
            ctx->ItemClick("##picker/hue");
            color_start = read_color();

            // Set value to 0, saturation must be preserved.
            ctx->ItemDragWithDelta("##picker/sv", ImVec2(0, +popup->Size.y * 0.5f));
            color = read_color();
            IM_CHECK_EQ(color.V, 0.0f);
            IM_CHECK_EQ(color.S, 0.0f);                     // Saturation undefined
            IM_CHECK_EQ(color_start.S, g.ColorEditSavedSat);// Preserved saturation matches original color

            // Set color to pure white and verify it can reach (1.0f, 1.0f, 1.0f).
            ctx->ItemDragWithDelta("##picker/sv", ImVec2(-popup->Size.x * 0.5f, -popup->Size.y * 0.5f));
            color = read_color();
            IM_CHECK_EQ(color.R, 1.0f);
            IM_CHECK_EQ(color.G, 1.0f);
            IM_CHECK_EQ(color.B, 1.0f);

            // Move hue to extreme ends, see if it does not wrap around.
            ctx->ItemDragWithDelta("##picker/hue", ImVec2(0, +popup->Size.y * 0.5f));
            IM_CHECK_EQ(read_color().H, 0.0f);              // Hue undefined
            IM_CHECK_EQ(g.ColorEditSavedHue, 1.0f);         // Preserved hue matches just set value
            ctx->ItemDragWithDelta("##picker/hue", ImVec2(0, -popup->Size.y * 0.5f));
            IM_CHECK_EQ(read_color().H, 0.0f);              // Hue undefined
            IM_CHECK_EQ(g.ColorEditSavedHue, 0.0f);         // Preserved hue matches just set value

            // Test hue preservation during mouse input.
            ctx->ItemClick("##picker/hue");
            ctx->ItemClick("##picker/sv");
            IM_CHECK_NE(read_color().H, 0.0f);              // Hue defined
            color_start = read_color();
            ctx->MouseDown();

            // Move mouse across all edges (extremes)
            ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(-popup->Size.x * 0.5f, -popup->Size.y * 0.5f));  // TL
            ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(+popup->Size.x * 1.0f, 0.0f));  // TR
            ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(0.0f, +popup->Size.y * 1.0f));  // BR
            ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(-popup->Size.x * 1.0f, 0.0f));  // BL
            ctx->MouseMoveToPos(g.IO.MousePos + ImVec2(0.0f, -popup->Size.y * 1.0f));  // TL
            ctx->MouseUp();
            IM_CHECK_EQ(color_start.H, g.ColorEditSavedHue); // Hue remains unchanged during all operations
        }
    };

    // ## Test ColorEdit basic Drag and Drop
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragdrop_coloredit");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(300, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::ColorEdit4("ColorEdit1", &vars.Color1.x, ImGuiColorEditFlags_None);
        ImGui::ColorEdit4("ColorEdit2", &vars.Color2.x, ImGuiColorEditFlags_None);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        vars.Color1 = ImVec4(1, 0, 0, 1);
        vars.Color2 = ImVec4(0, 1, 0, 1);

        ctx->SetRef("Test Window");

        IM_CHECK_NE(vars.Color1, vars.Color2);
        ctx->ItemDragAndDrop("ColorEdit1/##ColorButton", "ColorEdit2/##X"); // FIXME-TESTS: Inner items
        IM_CHECK_EQ(vars.Color1, vars.Color2);
    };

    // ## Test BeginDragDropSource() with NULL id.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragdrop_source_null_id");
    struct WidgetDragSourceNullIDData
    {
        ImGuiID SrcViewport = 0;
        ImGuiID DstViewport = 0;
        ImVec2 SrcPos;
        ImVec2 DstPos;
        bool Dropped = false;
    };
    t->SetVarsDataType<WidgetDragSourceNullIDData>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        WidgetDragSourceNullIDData& vars = ctx->GetVars<WidgetDragSourceNullIDData>();

        ImGui::Begin("Null ID Test", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("Null ID");

#ifdef IMGUI_HAS_DOCK
        vars.SrcViewport = ImGui::GetWindowViewport()->ID;
#endif
        vars.SrcPos = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()).GetCenter();

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            int magic = 0xF00;
            ImGui::SetDragDropPayload("MAGIC", &magic, sizeof(int));
            ImGui::EndDragDropSource();
        }
        ImGui::TextUnformatted("Drop Here");
#ifdef IMGUI_HAS_DOCK
        vars.DstViewport = ImGui::GetWindowViewport()->ID;
#endif
        vars.DstPos = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()).GetCenter();

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MAGIC"))
            {
                vars.Dropped = true;
                IM_CHECK_EQ(payload->DataSize, (int)sizeof(int));
                IM_CHECK_EQ(*(int*)payload->Data, 0xF00);
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        WidgetDragSourceNullIDData& vars = ctx->GetVars<WidgetDragSourceNullIDData>();

        // ImGui::TextUnformatted() does not have an ID therefore we can not use ctx->ItemDragAndDrop() as that refers
        // to items by their ID.
#ifdef IMGUI_HAS_DOCK
        ctx->MouseSetViewportID(vars.SrcViewport);
#endif
        ctx->MouseMoveToPos(vars.SrcPos);
        ctx->SleepStandard();
        ctx->MouseDown(0);

#ifdef IMGUI_HAS_DOCK
        ctx->MouseSetViewportID(vars.DstViewport);
#endif
        ctx->MouseMoveToPos(vars.DstPos);
        ctx->SleepStandard();
        ctx->MouseUp(0);

        IM_CHECK(vars.Dropped);
    };

    // ## Test preserving g.ActiveId during drag operation opening tree items.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragdrop_hold_to_open");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        if (ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::Button("Drag");
            if (ImGui::BeginDragDropSource())
            {
                int magic = 0xF00;
                ImGui::SetDragDropPayload("MAGIC", &magic, sizeof(int));
                ImGui::EndDragDropSource();
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;

        ImVec2 main_viewport_pos = ImGui::GetMainViewport()->Pos;
        ctx->WindowMove("Dear ImGui Demo", main_viewport_pos + ImVec2(16, 16));
        ctx->WindowResize("Dear ImGui Demo", ImVec2(400, 800));
        ctx->WindowMove("Test Window", main_viewport_pos = ImVec2(416, 16));

        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->WindowFocus(""); // So it's under TestWindow
        // FIXME-TESTS: This can still fail if "Test Window" covers "Dear ImGui Demo"

        ctx->SetRef("Test Window");
        ImGuiID active_id = ctx->GetID("Drag");
        ctx->MouseMove("Drag");
        ctx->SleepStandard();
        ctx->MouseDown();
        ctx->Yield();
        ctx->MouseLiftDragThreshold();
        IM_CHECK_EQ(g.ActiveId, active_id);

        ctx->SetRef("Dear ImGui Demo");
        ctx->MouseMove("Widgets", ImGuiTestOpFlags_NoFocusWindow);
        ctx->SleepNoSkip(1.0f, 1.0f / 60.0f);

        IM_CHECK(ctx->ItemInfo("Widgets").ID != 0);
        IM_CHECK((ctx->ItemInfo("Widgets").StatusFlags & ImGuiItemStatusFlags_Opened) != 0);
        IM_CHECK_EQ(g.ActiveId, active_id);
        ctx->MouseMove("Tree Nodes", ImGuiTestOpFlags_NoFocusWindow);
        ctx->SleepNoSkip(1.0f, 1.0f / 60.0f);
        IM_CHECK((ctx->ItemInfo("Tree Nodes").StatusFlags & ImGuiItemStatusFlags_Opened) != 0);
        IM_CHECK_EQ(g.ActiveId, active_id);
        ctx->MouseUp(0);
    };

    // ## Test overlapping drag and drop targets. The drag and drop system always prioritize the smaller target.
    // ## Test edge cases related to the overlapping targets submission order. (#6183)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragdrop_overlapping_targets");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Button("Drag");
        if (ImGui::BeginDragDropSource())
        {
            int value = 0xF00D;
            ImGui::SetDragDropPayload("_TEST_VALUE", &value, sizeof(int));
            ImGui::EndDragDropSource();
        }

        auto render_button = [](ImGuiTestContext* ctx, const char* name, const ImVec2& pos, const ImVec2& size)
        {
            ImGuiTestGenericVars& vars = ctx->GenericVars;
            ImGui::SetCursorScreenPos(pos);
            ImGui::Button(name, size);
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_TEST_VALUE"))
                {
                    IM_UNUSED(payload);
                    vars.Id = ImGui::GetItemID();
                    vars.Count++;
                    ctx->LogDebug("Dropped on \"%s\"!", name);
                }
                ImGui::EndDragDropTarget();
            }
        };

        // Render small button after big one
        // Position buttons so that our default "center" aiming behavior doesn't need to be tweaked,
        // otherwise we could expose ImGuiTestOpFlags to ItemDragAndDrop().
        ImVec2 pos = ImGui::GetCursorScreenPos();
        render_button(ctx, "Big1", pos, ImVec2(100, 100));
        render_button(ctx, "Small1", pos + ImVec2(70, 70), ImVec2(20, 20));

        // Render big button after small one
        render_button(ctx, "Small2", pos + ImVec2(0, 110) + ImVec2(70, 70), ImVec2(20, 20));
        render_button(ctx, "Big2", pos + ImVec2(0, 110), ImVec2(100, 100));

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");

        const char* drop_targets[] = { "Big1", "Small1", "Big2", "Small2" };
        //const char* drop_targets[] = { "Small2" };

        for (auto drop_target : drop_targets)
        {
            vars.Id = 0;
            vars.Count = 0;
            ctx->ItemDragAndDrop("Drag", drop_target);
            IM_CHECK(vars.Id == ctx->GetID(drop_target));
#if IMGUI_VERSION_NUM >= 18933
            IM_CHECK(vars.Count == 1);
#endif
            ctx->Yield(2); // Check again. As per #6183 we used to have a bug with overlapping targets.
            IM_CHECK(vars.Id == ctx->GetID(drop_target));
#if IMGUI_VERSION_NUM >= 18933
            IM_CHECK(vars.Count == 1);
#endif
        }
    };

    // ## Test timing of clearing payload right after delivery (#5817)
#if IMGUI_VERSION_NUM >= 18933
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragdrop_clear_payload");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Button("Drag Me");
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("_TEST_VALUE", NULL, 0);
            ImGui::EndDragDropSource();
        }

        ImGui::Button("Drop Here");
        bool just_dropped = false;
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_TEST_VALUE"))
            {
                IM_UNUSED(payload);
                just_dropped = true;
                IM_CHECK(ImGui::IsDragDropActive());
            }
            ImGui::EndDragDropTarget();
            // This is basically the only meaningful test here. Merely testing our basic contract for #5817
            if (just_dropped)
                IM_CHECK(!ImGui::IsDragDropActive());
        }

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->ItemDragAndDrop("//Test Window/Drag Me", "//Test Window/Drop Here");
    };
#endif

    // ## Test drag sources with _SourceNoPreviewTooltip flag not producing a tooltip.
    // ## Test drag target/accept with ImGuiDragDropFlags_AcceptNoPreviewTooltip
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragdrop_no_preview_tooltip");
    struct DragNoPreviewTooltipVars { bool TooltipHasBeenVisible = false; int TooltipLastVisibleFrame = -1; ImGuiDragDropFlags AcceptFlags = 0; };
    t->Flags |= ImGuiTestFlags_NoGuiWarmUp;
    t->SetVarsDataType<DragNoPreviewTooltipVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        DragNoPreviewTooltipVars& vars = ctx->GetVars<DragNoPreviewTooltipVars>();

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::CheckboxFlags("_AcceptNoPreviewTooltip", &vars.AcceptFlags, ImGuiDragDropFlags_AcceptNoPreviewTooltip);
        ImGui::Separator();

        auto create_drag_drop_source = [](ImGuiDragDropFlags flags)
        {
            if (ImGui::BeginDragDropSource(flags))
            {
                int value = 0xF00D;
                ImGui::SetDragDropPayload("_TEST_VALUE", &value, sizeof(int));
                ImGui::EndDragDropSource();
            }
        };

        ImGui::Button("Drag Src No Tooltip");
        create_drag_drop_source(ImGuiDragDropFlags_SourceNoPreviewTooltip);

        ImGui::Button("Drag Extern");
        if (ImGui::IsItemClicked())
            create_drag_drop_source(ImGuiDragDropFlags_SourceNoPreviewTooltip | ImGuiDragDropFlags_SourceExtern);

        ImGui::Button("Drag Accept");
        create_drag_drop_source(0);

        ImGui::Button("Drop");
        if (ImGui::BeginDragDropTarget())
        {
            ImGui::AcceptDragDropPayload("_TEST_VALUE", vars.AcceptFlags);
            ImGui::EndDragDropTarget();
        }

        ImGuiWindow* tooltip = g.TooltipPreviousWindow;
        //ImGuiWindow* tooltip = ctx->GetWindowByRef(Str16f("//##Tooltip_%02d", g.TooltipOverrideCount).c_str());
        if (tooltip != NULL && (tooltip->Active || tooltip->WasActive))
        {
            vars.TooltipLastVisibleFrame = g.FrameCount;
            vars.TooltipHasBeenVisible = true;
        }

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        DragNoPreviewTooltipVars& vars = ctx->GetVars<DragNoPreviewTooltipVars>();
        ctx->SetRef("Test Window");
        ctx->MouseMove("Drag Src No Tooltip");
        vars.TooltipHasBeenVisible = false;
        ctx->ItemDragAndDrop("Drag Src No Tooltip", "Drop");
        IM_CHECK(vars.TooltipHasBeenVisible == false);
        vars.TooltipHasBeenVisible = false;
        ctx->ItemDragAndDrop("Drag Extern", "Drop");
        IM_CHECK(vars.TooltipHasBeenVisible == false);
        vars.TooltipHasBeenVisible = false;
        ctx->Yield();

        vars.AcceptFlags = 0;
        ctx->ItemDragOverAndHold("Drag Accept", "Drop");
        //ctx->Yield();   // A visible tooltip window gets hidden with one frame delay. (due to how we test for Active || WasActive)
        IM_CHECK(vars.TooltipHasBeenVisible == true);
        IM_CHECK_EQ(vars.TooltipLastVisibleFrame + 1, g.FrameCount); // Bit wonky
        vars.TooltipHasBeenVisible = false;

        vars.AcceptFlags = ImGuiDragDropFlags_AcceptNoPreviewTooltip;
        ctx->ItemDragOverAndHold("Drag Accept", "Drop");
        ctx->Yield();   // A visible tooltip window gets hidden with one frame delay (due to how we test for Active || WasActive)
        IM_CHECK(vars.TooltipHasBeenVisible == true);
        IM_CHECK_LT(vars.TooltipLastVisibleFrame + 1, g.FrameCount);
        ctx->MouseUp();
    };

#if IMGUI_VERSION_NUM >= 18808
    // ## Test using mouse wheel while using drag and drop, or dragging from active item..
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragdrop_scroll");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiWindow* window_demo = ctx->GetWindowByRef("Dear ImGui Demo");
        ImGuiWindow* window_hello = ctx->GetWindowByRef("Hello, world!");
        ctx->WindowFocus("Hello, world!");
        ctx->WindowFocus("Dear ImGui Demo");
        ctx->WindowMove("Hello, world!", ImGui::GetMainViewport()->Pos + ImVec2(50.0f, 50.0f));
        ctx->WindowMove("Dear ImGui Demo", window_hello->Rect().GetTR());
        ctx->WindowResize("Hello, world!", ImVec2(400.0f, 200.0f));
        ctx->WindowResize("Dear ImGui Demo", ImVec2(400.0f, 600.0f));
        ctx->ItemOpen("Dear ImGui Demo/Help");

        const char* items[] =
        {
            "clear color/##ColorButton",    // Drag & drop system
            "float"                         // Active item (FIXME: Unspecified if we want that behavior of allowing drag while another is active)
        };
        for (int n = 0; n < IM_ARRAYSIZE(items); n++)
        {
            ctx->ScrollToTop("Dear ImGui Demo");
            IM_CHECK_EQ(window_demo->Scroll.y, 0.0f);
            ctx->MouseMove(Str64f("Hello, world!/%s", items[n]).c_str());
            IM_CHECK(ImGui::IsDragDropActive() == false);
            ctx->MouseDown();
            ctx->MouseMoveToPos(window_demo->Rect().GetCenter());
            if (n == 0)
                IM_CHECK(ImGui::IsDragDropActive() == true);
            ctx->MouseWheelY(-10);
            ctx->MouseUp();
            IM_CHECK_GT(window_demo->Scroll.y, 0.0f);
        }
    };
#endif

    // ## Test drag & drop using three main mouse buttons.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragdrop_mouse_buttons");
    struct DragMouseButtonsVars { bool Pressed = false; bool Dropped = false; };
    t->SetVarsDataType<DragMouseButtonsVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DragMouseButtonsVars& vars = ctx->GetVars<DragMouseButtonsVars>();
        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Appearing);
        if (ImGui::Begin("Window", NULL, ImGuiWindowFlags_NoSavedSettings))
        {
            vars.Pressed |= ImGui::ButtonEx("Button", ImVec2(), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle | ImGuiButtonFlags_MouseButtonRight);

            if (ImGui::BeginDragDropSource())
            {
                for (int button = 0; button < ImGuiMouseButton_COUNT; button++)
                    if (ImGui::IsMouseDown(button))
                        ImGui::Text("Dragged by button %d", button);
                ImGui::SetDragDropPayload("Button", "Works", 6);
                ImGui::EndDragDropSource();
            }

            ImGui::Button("Drop Here");
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Button"))
                    vars.Dropped = payload->Data != NULL && strcmp((const char*)payload->Data, "Works") == 0;
                ImGui::EndDragDropTarget();
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        DragMouseButtonsVars& vars = ctx->GetVars<DragMouseButtonsVars>();
        ctx->SetRef("Window");

        // Clicking still works.
        for (int button = 0; button < 3; button++)
        {
            vars.Pressed = false;
            ctx->ItemClick("Button", button);
            IM_CHECK(vars.Pressed);
        }

        // Drag & drop using all mouse buttons work.
        // FIXME: At this time only left, right and middle mouse buttons are supported for this use-case.
        for (ImGuiMouseButton button = 0; button < 3; button++)
        {
            vars.Dropped = false;
            ctx->ItemDragAndDrop("Button", "Drop Here", button);
            IM_CHECK(vars.Dropped);
        }
    };

#if IMGUI_VERSION_NUM >= 18903
    // ## Test drag & drop trigger on new payload, GetDragDropPayload() returning true. (#5910)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragdrop_new_payloads");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Appearing);
        if (ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::Button("DragSrc");
            if (ImGui::BeginDragDropSource())
            {
                const ImGuiPayload* payload = ImGui::GetDragDropPayload();
                if (vars.Count <= 1)
                    IM_CHECK(payload == NULL);
                else
                    IM_CHECK(payload != NULL);
                if (vars.Count >= 1)
                    ImGui::SetDragDropPayload("Button", "Data", 5);
                vars.Count++;
                ImGui::EndDragDropSource();
            }
            ImGui::Button("DragDst");
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ctx->ItemDragOverAndHold("//Test Window/DragSrc", "//Test Window/DragDst");
        IM_CHECK(vars.Count > 3); // Verify we tested enough (used ItemDragOverAndHold() instead of ItemDragAndDrop() to increase frame count in fast mode)
    };
#endif

    // ## Test using default context menu along with a combo (#4167)
#if IMGUI_VERSION_NUM >= 18211
    t = IM_REGISTER_TEST(e, "widgets", "widgets_combo_context_menu");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::BeginCombo("Combo", "Preview"))
        {
            vars.Bool1 = true;
            ImGui::Selectable("Close");
            ImGui::EndCombo();
        }
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::Button("Close"))
                ImGui::CloseCurrentPopup();
            vars.Bool2 = true;
            ImGui::EndPopup();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->MouseMove("Combo");
        ctx->MouseClick(ImGuiMouseButton_Right);
        IM_CHECK(vars.Bool1 == false && vars.Bool2 == true);
        ctx->ItemClick("//$FOCUSED/Close");
        vars.Bool1 = vars.Bool2 = false;

        ctx->ItemClick("Combo");
        IM_CHECK(vars.Bool1 == true && vars.Bool2 == false);
        ctx->ItemClick("//$FOCUSED/Close");
        vars.Bool1 = vars.Bool2 = false;
    };
#endif

#if IMGUI_VERSION_NUM >= 18308
    // ## Test BeginComboPreview() and ImGuiComboFlags_CustomPreview
    t = IM_REGISTER_TEST(e, "widgets", "widgets_combo_custom_preview");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE" };

        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiContext& g = *ImGui::GetCurrentContext();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Custom preview:");

        if (vars.Step == 2)
            ImGui::SetNextItemWidth(ImGui::GetFrameHeight() * 2.0f);

        const ImVec2 color_square_size = ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize());
        bool open = ImGui::BeginCombo("custom", "", ImGuiComboFlags_CustomPreview);
        if (open)
        {
            for (int n = 0; n < 5; n++)
            {
                ImGui::PushID(n);
                ImGui::ColorButton("##color", ImVec4(1.0f, 0.5f, 0.5f, 1.0f), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, color_square_size);
                ImGui::SameLine();
                ImGui::Selectable(items[n]);
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImRect combo_r = g.LastItemData.Rect;
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        int old_cmd_buffer_size = window->DrawList->CmdBuffer.Size;

        if (ImGui::BeginComboPreview())
        {
            ImGui::ColorButton("##color", ImVec4(1.0f, 0.5f, 0.5f, 1.0f), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, color_square_size);
            ImGui::TextUnformatted(items[0]);
            ImGui::EndComboPreview();
        }

        // Test sameline restore behavior
        ImGui::SameLine();
        IM_CHECK_EQ_NO_RET(combo_r.Min.y, window->DC.CursorPos.y);
        IM_CHECK_EQ(ImGui::GetStyle().FramePadding.y, window->DC.CurrLineTextBaseOffset);
        ImGui::Text("HELLO");

        // Test draw call merging behavior
        if (vars.Step == 1)
            IM_CHECK_EQ(old_cmd_buffer_size, window->DrawList->CmdBuffer.Size);
        if (vars.Step == 2)
            IM_CHECK_EQ(old_cmd_buffer_size + 2, window->DrawList->CmdBuffer.Size);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");

        vars.Step = 0;
        ctx->WindowResize("", ImVec2(400, 400));
        vars.Step = 1;
        ctx->Yield(2);
        vars.Step = 2;
        ctx->Yield(2);
    };
#endif

#if IMGUI_VERSION_NUM >= 18408
    // ## Test null range in TextUnformatted() (#3615, #7391)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_text_null");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        const char* str = "hello world";
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        {
            ImVec2 p0 = ImGui::GetCursorPos();
            ImGui::TextUnformatted(str, str + 5);
            ImGui::TextUnformatted(str + 1, str + 1);
            ImGui::TextUnformatted(NULL);
            ImVec2 p1 = ImGui::GetCursorPos();
            IM_CHECK_EQ(p0.y + ImGui::GetTextLineHeightWithSpacing() * 3, p1.y);
        }
        {
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImGui::TextUnformatted("");
            ImGui::SameLine(0.0f, 0.0f);
            ImVec2 p1 = ImGui::GetCursorScreenPos();
            IM_CHECK_EQ(p0, p1);
        }
#if IMGUI_VERSION_NUM >= 19047
        {
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddText(p0, ImGui::GetColorU32(ImGuiCol_Text), str + 1, str + 1);
            ImGui::GetWindowDrawList()->AddText(p0, ImGui::GetColorU32(ImGuiCol_Text), NULL, NULL);
        }
#endif
        ImGui::End();
    };
#endif

    // ## Test long text rendering in TextUnformatted()
    t = IM_REGISTER_TEST(e, "widgets", "widgets_text_long");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Dear ImGui Demo");
        ctx->MenuCheck("Examples/Long text display");
        ctx->SetRef("Example: Long text display");
        ctx->ItemClick("Add 1000 lines");
        ctx->SleepStandard();

        ImGuiWindow* log_panel = ctx->WindowInfo("Log").Window;
        IM_CHECK(log_panel != NULL);
        ImGui::SetScrollY(log_panel, log_panel->ScrollMax.y);
        ctx->SleepStandard();
        ctx->ItemClick("Clear");
        ctx->ComboClick("Test type/Single call to TextUnformatted()");
        ctx->ComboClick("Test type/Multiple calls to Text(), clipped");
        ctx->ComboClick("Test type/Multiple calls to Text(), not clipped (slow)");
        ctx->WindowClose("");
    };

    // ## Flex code paths that try to avoid formatting when "%s" is used as format.
    // ## Also flex "%.*s" variant (#6846)
    // ## Verify case of passing NULL as string (#7016)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_text_unformatted_fastpath");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ImGui::GetCurrentContext();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("%sLO", "HEL");
#if IMGUI_VERSION_NUM >= 18924
        ImGui::Text("%s", "...");
#endif
        ImGui::TextDisabled("%s", "...");
        ImGui::TextColored(ImColor(IM_COL32_WHITE), "%s", "...");
        ImGui::TextWrapped("%s", "...");
#if IMGUI_VERSION_NUM >= 18727
        IM_CHECK_STR_EQ(g.TempBuffer.Data, "HELLO");
#else
        IM_CHECK_STR_EQ(g.TempBuffer, "HELLO");
#endif

#if IMGUI_VERSION_NUM >= 18993
        ImGui::Text("%.*s!", 10, "STRINGVIEW");
        IM_CHECK_STR_EQ(g.TempBuffer.Data, "STRINGVIEW!");
        ImGui::Text("%.*s", 5, "HELLO");
        IM_CHECK_STR_EQ(g.TempBuffer.Data, "STRINGVIEW!");
#endif

#if IMGUI_VERSION_NUM >= 19001
        ImGui::LogToClipboard();
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-overflow"  // warning: �%s� directive argument is null
#endif
        ImGui::Text("%s", (const char*)NULL);
        ImGui::Text("%.*s", 3, (const char*)NULL);
        ImGui::Text("%.*s", 9999, (const char*)NULL);
        ImGui::LogFinish();
        IM_CHECK_STR_EQ(g.TempBuffer.Data, "STRINGVIEW!");
        IM_CHECK_STR_EQ(ImGui::GetClipboardText(), "(null)" IM_NEWLINE "(nu" IM_NEWLINE "(null)" IM_NEWLINE);

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#endif

        ImGui::End();
    };

#if IMGUI_VERSION_NUM >= 18911
    // ## Basic tests for wrapped text (probably more to add)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_text_wrapped");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ImGui::GetCurrentContext();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
        for (int n = 0; n < 6; n++)
            ImGui::Text("Dummy %d", n);

        // Verify that vertices fit in bounding-box when clipped (#5919)
        // (FIXME: we can't use our WIP capture vtx api as it relies on items having an identifier)
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        unsigned int vtx_0 = draw_list->_VtxCurrentIdx;
        ImGui::TextWrapped("This is a long wrapped text hello world hello world this is some long text");
        ImRect rect = g.LastItemData.Rect;
        rect.Expand(2.0f);
        unsigned int vtx_1 = draw_list->_VtxCurrentIdx;
        for (unsigned int n = vtx_0; n < vtx_1; n++)
            IM_CHECK(rect.Contains(draw_list->VtxBuffer[n].pos));
        for (int n = 0; n < 6; n++)
            ImGui::Text("Dummy %d", n);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ImGui::GetCurrentContext();
        ctx->SetRef("Test Window");
        ctx->WindowResize("", ImVec2(g.FontSize * 15, ImGui::GetTextLineHeightWithSpacing() * 8));
        ctx->ScrollToTop("");
        ctx->ScrollToBottom("");
    };
#endif

    // ## Test for text wrapping with consecutive separators
    // FIXME: This is tricky to test for at least until we finish/merge the drawlist analyzer which can parse characters from vertices.
    // For now we manually call CalcWordWrapPositionA().
    t = IM_REGISTER_TEST(e, "widgets", "widgets_text_wrapped_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
        const char* s1 = "abcde..";
        const char* s2 = "abcde.. That";
        float local_off_x = ImGui::GetCursorPos().x;
        float wrap_width = ImGui::CalcTextSize(s1).x;

        // Visualize output here, no actual test
        ImGui::PushTextWrapPos(wrap_width + local_off_x);
        ImGui::TextUnformatted(s1);
        ImGui::Spacing();
        ImGui::TextUnformatted(s2); // Though the '.' can fit in the first line, it's now at the second line.
        ImGui::PopTextWrapPos();

        // Text wrapping with leading \n
        ImGui::Separator();
        ImGui::TextWrapped("\nHello");
        IM_CHECK_EQ(ImGui::GetItemRectSize(), ImVec2(ImGui::CalcTextSize("Hello").x, ImGui::GetFontSize() * 2));

        ImGui::End();

        // Tests
#if IMGUI_VERSION_NUM >= 19197 || defined(IMGUI_HAS_TEXTURES)
        const char* s1_w = ImGui::GetFont()->CalcWordWrapPosition(ImGui::GetFontSize(), s1, s1 + strlen(s1), wrap_width);
#else
        const char* s1_w = ImGui::GetFont()->CalcWordWrapPositionA(1.0f, s1, s1 + strlen(s1), wrap_width);
#endif
        IM_CHECK_EQ(s1_w, s1 + strlen(s1));

#if IMGUI_BROKEN_TESTS
// #if 1
        // This is broken, see #8139
        const char* s2_w = ImGui::GetFont()->CalcWordWrapPosition(ImGui::GetFontSize(), s2, s2 + strlen(s2), wrap_width);
        IM_CHECK_EQ(s2_w, s2 + strlen("abcde.."));
#endif
    };

    // ## Test LabelText() variants layout (#4004)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_label_text");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ImGui::GetCurrentContext();

        ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGui::Separator();
        ImGui::LabelText("Single line label", "Single line text");
        IM_CHECK_EQ(g.LastItemData.Rect.GetHeight(), ImGui::GetFrameHeight());

        ImGui::Separator();
        ImGui::LabelText("Multi\n line\n label", "Single line text");
        IM_CHECK_EQ(g.LastItemData.Rect.GetHeight(), ImGui::GetTextLineHeight() * 3.0f + ImGui::GetStyle().FramePadding.y * 2.0f);

        ImGui::Separator();
        ImGui::LabelText("Single line label", "Multi\n line\n text");
        IM_CHECK_EQ(g.LastItemData.Rect.GetHeight(), ImGui::GetTextLineHeight() * 3.0f + ImGui::GetStyle().FramePadding.y * 2.0f);

        ImGui::End();
    };

    // ## Test hovering across levels of menu
    t = IM_REGISTER_TEST(e, "widgets", "widgets_menu_hover");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("open"))
            ImGui::OpenPopup("Popup");
        if (ImGui::BeginPopup("Popup"))
        {
            ImGui::MenuItem("AAA");
            ImGui::MenuItem("BBB");
            if (ImGui::BeginMenu("CCC"))
            {
                ImGui::MenuItem("CCC.2");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("DDD"))
            {
                ImGui::MenuItem("DDD.2");
                ImGui::EndMenu();
            }
#if IMGUI_VERSION_NUM >= 18713
            ImGui::Button("EEE"); // Test non-MenuItem() because they have a custom behavior.
#endif
            ImGui::MenuItem("FFF");
            ImGui::EndPopup();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Test Window");
        ctx->ItemClick("open");
        ctx->SetRef("//$FOCUSED");
        ctx->MouseMove("BBB", ImGuiTestOpFlags_NoFocusWindow);
        ctx->MouseMove("CCC", ImGuiTestOpFlags_NoFocusWindow);
        IM_CHECK(g.NavWindow != NULL);
        IM_CHECK((g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu) != 0);
        ctx->MouseMove("DDD", ImGuiTestOpFlags_NoFocusWindow);
        IM_CHECK_EQ(g.HoveredIdPreviousFrame, ctx->GetID("DDD"));
        IM_CHECK((g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu) != 0);
#if IMGUI_VERSION_NUM >= 18713
        ctx->MouseMove("EEE", ImGuiTestOpFlags_NoFocusWindow);
        IM_CHECK_EQ(g.HoveredIdPreviousFrame, ctx->GetID("EEE"));
#endif
        ctx->MouseMove("FFF", ImGuiTestOpFlags_NoFocusWindow);
        IM_CHECK_EQ(g.HoveredIdPreviousFrame, ctx->GetID("FFF"));
        ctx->Yield();
        IM_CHECK((g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu) == 0);
    };

    // ## Test clicking on already opened menu doesn't make it flicker
    t = IM_REGISTER_TEST(e, "widgets", "widgets_menu_reopen");
    struct WindowMenuReopenVars { bool MenuIsVisible = false, MenuWasOnceNotVisible = false, SubmenuIsVisible = false, SubmenuWasOnceNotVisible = false; };
    t->SetVarsDataType<WindowMenuReopenVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<WindowMenuReopenVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            vars.MenuIsVisible = vars.SubmenuIsVisible = false;
            if (ImGui::BeginMenu("Menu"))
            {
                vars.MenuIsVisible = true;
                ImGui::MenuItem("Menu.item");
                if (ImGui::BeginMenu("Submenu"))
                {
                    vars.SubmenuIsVisible = true;
                    ImGui::MenuItem("Submenu.item");
                    ImGui::EndMenu();
                }
                else
                {
                    vars.SubmenuWasOnceNotVisible = true;
                }
                ImGui::EndMenu();
            }
            else
            {
                vars.MenuWasOnceNotVisible = vars.SubmenuWasOnceNotVisible = true;
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // This would likely work with MenuClick("Menu") but going lower-level to ensure we don't mess with focus.
        auto& vars = ctx->GetVars<WindowMenuReopenVars>();
        ctx->SetRef("Test Window");
        ctx->ItemClick("##MenuBar/Menu", 0, ImGuiTestOpFlags_NoFocusWindow);
        IM_CHECK(vars.MenuIsVisible);
        vars.MenuWasOnceNotVisible = false;
        ctx->ItemClick("##MenuBar/Menu", 0, ImGuiTestOpFlags_NoFocusWindow);
        IM_CHECK(vars.MenuIsVisible == false);
        IM_CHECK(vars.MenuWasOnceNotVisible);
        ctx->ItemClick("##MenuBar/Menu", 0, ImGuiTestOpFlags_NoFocusWindow);
        IM_CHECK(vars.MenuIsVisible);

        ctx->SetRef("//$FOCUSED");
        vars.MenuWasOnceNotVisible = false;
        ctx->ItemClick("Submenu", 0, ImGuiTestOpFlags_NoFocusWindow);
        IM_CHECK(vars.MenuIsVisible);
        IM_CHECK(vars.SubmenuIsVisible);
        IM_CHECK(vars.MenuWasOnceNotVisible == false);
        vars.SubmenuWasOnceNotVisible = false;

        ctx->ItemClick("Submenu", 0, ImGuiTestOpFlags_NoFocusWindow);
        IM_CHECK(vars.MenuIsVisible);
        IM_CHECK(vars.SubmenuIsVisible);
        IM_CHECK(vars.MenuWasOnceNotVisible == false);
        IM_CHECK(vars.SubmenuWasOnceNotVisible == false);
    };

    // ## Test menu reopens while moving up or down (#7325 highlighted an order-dependent issue)
#if IMGUI_VERSION_NUM >= 19032
    t = IM_REGISTER_TEST(e, "widgets", "widgets_menu_reopen_2");
    t->SetVarsDataType<WindowMenuReopenVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        if (ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings))
        {
            if (ImGui::BeginMenu("Nested1"))
            {
                if (ImGui::BeginMenu("Nested2"))
                {
                    for (int i = 0; i < 10; i++)
                    {
                        if (ImGui::BeginMenu(Str30f("%d", i).c_str()))
                        {
                            ImGui::Text("Nothing here");
                            ImGui::EndMenu();
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Test Window");
        ctx->MouseMove("Nested1");
        IM_CHECK_EQ(g.OpenPopupStack.Size, 1);
        ctx->MouseMove("//$FOCUSED/Nested2");
        IM_CHECK_EQ(g.OpenPopupStack.Size, 2);
        ctx->MouseMove("//$FOCUSED/0");

        ctx->SetRef("//###Menu_01");

        ctx->MouseMove("0");    // Same
        IM_CHECK_EQ(g.OpenPopupStack.Size, 3);

        // Move down
        ctx->MouseMove("1");
        IM_CHECK_EQ(g.OpenPopupStack.Size, 3);
        ctx->MouseMove("2");
        IM_CHECK_EQ(g.OpenPopupStack.Size, 3);

        // Move up: between 2024/02/08 and 2024/02/20 closure of /2 led to NewFrame() running
        // "if (g.NavWindow && !g.NavWindow->WasActive) FocusTopMostWindowUnderOne(NULL, etc.)"
        // and FocusTopMostWindowUnderOne() only look for top-level windows. (#7325)
        ctx->MouseMove("1");
        IM_CHECK_EQ(g.OpenPopupStack.Size, 3);

        // Move down instantaneously to exercise successive frames opens (#7325)
        ctx->MouseTeleportToPos(ctx->ItemInfo("3").RectClipped.GetCenter(), ImGuiTestOpFlags_NoYield);
        ctx->Yield();
        IM_CHECK_EQ(g.OpenPopupStack.Size, 3);
        ctx->MouseTeleportToPos(ctx->ItemInfo("5").RectClipped.GetCenter(), ImGuiTestOpFlags_NoYield);
        ctx->Yield();
        IM_CHECK_EQ(g.OpenPopupStack.Size, 3);
        ctx->Yield();
        IM_CHECK_EQ(g.OpenPopupStack.Size, 3);
        ctx->MouseTeleportToPos(ctx->ItemInfo("4").RectClipped.GetCenter(), ImGuiTestOpFlags_NoYield);
        ctx->Yield();
        IM_CHECK_EQ(g.OpenPopupStack.Size, 3);
        ctx->MouseTeleportToPos(ctx->ItemInfo("3").RectClipped.GetCenter(), ImGuiTestOpFlags_NoYield);
        ctx->Yield();
        IM_CHECK_EQ(g.OpenPopupStack.Size, 3);
        ctx->Yield();
#if IMGUI_VERSION_NUM >= 19042
        IM_CHECK_EQ(g.OpenPopupStack.Size, 3); // Broken before 2024/02/27
#endif
    };
#endif

    // ## Test menus inside a popup (not a menubar), notably for #43
    t = IM_REGISTER_TEST(e, "window", "widgets_menu_in_popup");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        if (ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings))
        {
            if (ImGui::BeginMenu("Menu1"))
            {
                ImGui::MenuItem("MenuItem1");
                ImGui::MenuItem("MenuItem2");
                ImGui::EndMenu();
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("//Test Window");
        ctx->MenuClick("Menu1/MenuItem1");
        ctx->MenuClick("Menu1/MenuItem2");
    };

    // ## Test navigating menus with mouse button is held down.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_menu_mouse_hold");
    struct PopupMenuHoldVars { ImGuiID QueryVarsBaseId = 0; ImGuiTestGenericItemStatus Status; };
    t->SetVarsDataType<PopupMenuHoldVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        PopupMenuHoldVars& vars = ctx->GetVars<PopupMenuHoldVars>();
        auto CreateMenu = [ctx, &vars]()
        {
            IM_UNUSED(ctx);
            ImGuiID base_id = ImGui::GetID("");
            if (vars.QueryVarsBaseId == base_id)
                vars.Status.Clear();

            if (ImGui::BeginMenu("AAA"))
            {
                if (ImGui::BeginMenu("BBB"))
                {
                    bool clicked = ImGui::MenuItem("CCC");
                    if (vars.QueryVarsBaseId == base_id)
                    {
                        vars.Status.QuerySet();
                        if (!ImGui::IsItemClicked())    // IsItemClicked() fails when mouse is dragged while pressed down
                            vars.Status.Clicked += (int)clicked;
                    }
                    ImGui::EndMenu();
                }
                ImGui::MenuItem("Item"); // FIXME: Add some testing from holding this first
                ImGui::EndMenu();
            }
        };
        auto CreateMenuBar = [CreateMenu]()
        {
            ImGui::BeginMenuBar();
            CreateMenu();
            ImGui::EndMenuBar();
        };

        // Window with menu bar
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
        CreateMenuBar();

        // Popup with menu bar
        if (ImGui::Button("Open Popup"))
            ImGui::OpenPopup("Popup");
        if (ImGui::BeginPopup("Popup", ImGuiWindowFlags_MenuBar))
        {
            CreateMenuBar();
            ImGui::EndPopup();
        }

        // Modal with menu bar
        if (ImGui::Button("Open Modal"))
            ImGui::OpenPopup("Modal");
        if (ImGui::BeginPopupModal("Modal", NULL, ImGuiWindowFlags_MenuBar))
        {
            CreateMenuBar();
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // Popup with menu
        if (ImGui::Button("Open Menu"))
            ImGui::OpenPopup("Menu");
        if (ImGui::BeginPopup("Menu"))
        {
            CreateMenu();
            ImGui::EndPopup();
        }

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        PopupMenuHoldVars& vars = ctx->GetVars<PopupMenuHoldVars>();
        ctx->SetRef("Test Window");

        struct { const char* OpenButton; const char* MenuBase; } test_data[] = {
            { NULL,         "##MenuBar" }, // Test from a window menu bar.
            { "Open Popup", "##MenuBar" }, // Test from popup menu bar.
            { "Open Modal", "##MenuBar" }, // Test from modal menu bar.
            { "Open Menu" , ""          }, // Test from popup menu.
        };

        for (int variant = 0; variant < IM_ARRAYSIZE(test_data); variant++)
        {
            ctx->LogDebug("Variant: %d", variant);
            auto& td = test_data[variant];
            if (td.OpenButton)
                ctx->ItemClick(Str64f("//Test Window/%s", td.OpenButton).c_str());
            else
                ctx->WindowFocus("//Test Window");
            ctx->SetRef("//$FOCUSED");
            vars.QueryVarsBaseId = ctx->GetID(td.MenuBase);
            if (*td.MenuBase)
                ctx->MouseMove(Str16f("%s/AAA", td.MenuBase).c_str());
            else
                ctx->MouseMove("AAA", ImGuiTestOpFlags_NoFocusWindow);
            ctx->MouseDown();
#if !IMGUI_BROKEN_TESTS
            // FIXME-TESTS: Pressing menu item whose menu is already open, causes open menu to flicker.
            // This is a regression since 48f263336bc7651e0b648d9aaacd7828e31b23f8. Yield one frame to
            // give flickering window time to reappear so MenuAction() does not fail.
            ctx->Yield();
#endif
            IM_CHECK_EQ(ctx->UiContext->ActiveId, 0u);
            ctx->MenuAction(ImGuiTestAction_Hover, "AAA/BBB/CCC");
            IM_CHECK_EQ(vars.Status.Hovered, 1);
            ctx->MouseUp();
            IM_CHECK_EQ(vars.Status.Clicked, 1);
        }
    };

#if IMGUI_VERSION_NUM >= 18727
    // ## Test menuset separation.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_menu_menusets");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Dear ImGui Demo");
        ctx->WindowResize("", ImVec2(350, 600));
        ctx->ItemOpen("Popups & Modal windows");
        ctx->ItemOpen("Menus inside a regular window");
        ImGuiWindow* demo_window = ctx->GetWindowByRef("");
        ctx->MouseMoveToPos(demo_window->Pos);   // Do not hover any menus.

        // 1. Open a menu and hover a menu that does not belong to open menuset.
        ctx->MenuClick("Examples");
        IM_CHECK(g.OpenPopupStack.Size == 1);
        ImGuiID popup_parent_id = g.OpenPopupStack.back().OpenParentId;
        ctx->MouseMove("Menus inside a regular window/Menu inside a regular window", ImGuiTestOpFlags_NoFocusWindow | ImGuiTestOpFlags_NoCheckHoveredId);
        IM_CHECK(g.OpenPopupStack.Size == 1);
        IM_CHECK_EQ(g.OpenPopupStack.back().OpenParentId, popup_parent_id);

        // 2. Hover a menu without another opened menuset.
        ctx->MouseMoveToPos(demo_window->Pos);   // Do not hover any menus.
        ctx->PopupCloseAll();
        ctx->MouseMove("Menus inside a regular window/Menu inside a regular window");
        IM_CHECK(g.OpenPopupStack.Size == 1);
        IM_CHECK_NE(g.OpenPopupStack.back().OpenParentId, popup_parent_id);

        // 3. Now do same as 1. except in reverse order.
        ctx->MouseMove("##MenuBar/Examples", ImGuiTestOpFlags_NoFocusWindow | ImGuiTestOpFlags_NoCheckHoveredId);
        IM_CHECK(g.OpenPopupStack.Size == 0);

        // 4. Clicking another menuset should finally open it.
        ctx->MenuClick("Examples");
        IM_CHECK(g.OpenPopupStack.Size == 1);
        IM_CHECK_EQ(g.OpenPopupStack.back().OpenParentId, popup_parent_id);

        // Verify that menu doesn't close while activating an item and dragging back over parent menu (#6869)
        ctx->ItemClose("Popups & Modal windows");
        ctx->MenuClick("Menu");
        ctx->MouseMove("//$FOCUSED/New");
        ImVec2 pos = ImGui::GetMousePos();
        ctx->MenuClick("Menu/Options/Value"); // Click DragFloat()
        ctx->MouseDown();
        ctx->Yield();
        IM_CHECK_EQ(g.OpenPopupStack.Size, 2);
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("//$FOCUSED/Value"));
#if IMGUI_VERSION_NUM >= 18993
        ctx->MouseMoveToPos(pos); // Move back to "New" item
        IM_CHECK_EQ(g.OpenPopupStack.Size, 2);
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("//$FOCUSED/Value"));
#endif
    };
#endif

    // ## Test menu appending (#1207)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_menu_append");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Append Menus", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);
        ImGui::BeginMenuBar();

        // Menu that we will append to.
        if (ImGui::BeginMenu("First Menu"))
        {
            ImGui::MenuItem("1 First");
            if (ImGui::BeginMenu("Second Menu"))
            {
                ImGui::MenuItem("2 First");
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        // Append to first menu.
        if (ImGui::BeginMenu("First Menu"))
        {
            if (ImGui::MenuItem("1 Second"))
                vars.Bool1 = true;
            if (ImGui::BeginMenu("Second Menu"))
            {
                ImGui::MenuItem("2 Second");

                // To test left<>right movement within
                ImGui::Button("AAA");
                ImGui::SameLine();
                ImGui::Button("BBB");

                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Append Menus");
        ctx->MenuClick("First Menu");
        ctx->MenuClick("First Menu/1 First");
        IM_CHECK_EQ(vars.Bool1, false);
        ctx->MenuClick("First Menu/1 Second");
        IM_CHECK_EQ(vars.Bool1, true);
        ctx->MenuClick("First Menu/Second Menu/2 First");
        ctx->MenuClick("First Menu/Second Menu/2 Second");

#if IMGUI_VERSION_NUM >= 18827
        // Test that Left<>Right navigation work on appended menu
        ImGuiContext& g = *ctx->UiContext;
        ctx->MenuClick("First Menu/Second Menu/AAA");
        //ctx->NavMoveTo("//$FOCUSED/AAA");
        ctx->Yield(2);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/AAA"));
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/BBB"));
        ctx->KeyPress(ImGuiKey_LeftArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/AAA"));
#endif
    };

    // ## Test use of ### operator in menu path
    t = IM_REGISTER_TEST(e, "widgets", "widgets_menu_path");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("First"))
            {
                if (ImGui::BeginMenu("Second"))
                {
                    ImGui::MenuItem("Item");
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Third###Hello"))
                {
                    ImGui::MenuItem("Item");
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("File###Menu_File"))
            {
                ImGui::MenuItem("New###Menu_FileNew");
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ctx->MenuClick("First/Second/Item");
        ctx->MenuClick("First/Third###Hello/Item");

        ctx->MenuClick("First/Second");
        ctx->ItemClick("**/Item");

        ctx->MenuClick("###Menu_File");
        ctx->MenuClick("###Menu_File/###Menu_FileNew");

#if IMGUI_BROKEN_TESTS
        ctx->MenuClick("First/Second");
        ctx->MenuClick("**/Item");
#endif
    };

    // ## Test text capture of separator in a menu.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_menu_separator");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            ImGui::LogToBuffer();
            if (ImGui::BeginMenu("File"))
                ImGui::EndMenu();
            ImGui::Separator();
            IM_CHECK_EQ(ImGui::GetItemRectMax().y - ImGui::GetItemRectMin().y, ImGui::GetFrameHeight());
            if (ImGui::BeginMenu("Edit"))
                ImGui::EndMenu();
            ImGui::EndMenuBar();
            ImStrncpy(vars.Str1, ctx->UiContext->LogBuffer.c_str(), IM_ARRAYSIZE(vars.Str1));
            ImGui::LogFinish();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
#if IMGUI_VERSION_NUM >= 19142
        IM_CHECK_STR_EQ(vars.Str1, "[ File ] | [ Edit ]");
#else
        IM_CHECK_STR_EQ(vars.Str1, "File | Edit");
#endif
    };

    // ## Test main menubar appending.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_menu_mainmenubar_append");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // Menu that we will append to.
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("First Menu"))
                ImGui::EndMenu();
            ImGui::EndMainMenuBar();
        }

        // Append to first menu.
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Second Menu"))
            {
                if (ImGui::MenuItem("Second"))
                    vars.Bool1 = true;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("##MainMenuBar");
        ctx->MenuClick("Second Menu/Second");
        IM_CHECK_EQ(vars.Bool1, true);
    };

    // ## Test main menubar navigation
    t = IM_REGISTER_TEST(e, "widgets", "widgets_menu_mainmenubar_navigation");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Menu 1"))
            {
                if (ImGui::BeginMenu("Sub Menu 1"))
                {
                    ImGui::MenuItem("Item 1");
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Menu 2"))
            {
                if (ImGui::BeginMenu("Sub Menu 2-1"))
                {
                    ImGui::MenuItem("Item 2");
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Sub Menu 2-2"))
                {
                    ImGui::MenuItem("Item 3");
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
        ImGuiContext& g = *ImGui::GetCurrentContext();

        ctx->MenuClick("##MainMenuBar/Menu 1");

        // Click doesn't affect g.NavId which is null at this point

        // Test basic key Navigation
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/Sub Menu 1"));
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/Item 1"));
        ctx->KeyPress(ImGuiKey_LeftArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/Sub Menu 1"));
        ctx->KeyPress(ImGuiKey_LeftArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("##MainMenuBar##MenuBar/Menu 1"));

        ctx->MouseMove("##MainMenuBar##MenuBar/Menu 2"); // FIXME-TESTS: Workaround so TestEngine can find again Menu 1

        // Test forwarding of nav event to parent when there's no match in the current menu
        // Going from "Menu 1/Sub Menu 1/Item 1" to "Menu 2"
        ctx->ItemClick("##MainMenuBar##MenuBar/Menu 1");
        ctx->KeyPress(ImGuiKey_DownArrow);
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/Item 1"));
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("##MainMenuBar##MenuBar/Menu 2"));

        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/Sub Menu 2-1"));
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/Item 2"));

        ctx->KeyPress(ImGuiKey_LeftArrow);
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/Sub Menu 2-2"));
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("//$FOCUSED/Item 3"));
    };

    // ## Test main menubar automatically releasing focus + bug overriding layer (#8355)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_menu_mainmenubar_release_focus");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Button("OK");
        ImGui::End();
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                ImGui::MenuItem("Hedhehog");
                ImGui::EndMenu();
            }
            ImGui::Button("BUTTON");
            ImGui::SetNextItemWidth(100);
            ImGui::InputText("InputText1", vars.Str1, IM_ARRAYSIZE(vars.Str1));
            if (ImGui::BeginTable("Table", 1))
            {
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(100);
                ImGui::InputText("InputText2", vars.Str1, IM_ARRAYSIZE(vars.Str1));
                ImGui::EndTable();
            }
            ImGui::EndMainMenuBar();
        }
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* test_window = ctx->GetWindowByRef("/Test Window");
        ctx->ItemClick("/Test Window/OK");
        IM_CHECK_EQ(g.NavWindow, test_window);

        // It's not well specced, but MenuLayer->MainLayer is currently effectively done on FocusWindow(),
        // which closing a menu does, but not e.g. clicking a button. That's why we press Escape.
        // It may be the saner behavior that we limit this to closing menu, so maybe current behavior i the best.
        ctx->SetRef("##MainMenuBar"); // FIXME-TESTS: It's pretty shitty we don't have a clear standard to use that yet
        ctx->MenuClick("BUTTON");
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Menu);
        ctx->KeyPress(ImGuiKey_Escape); // Leave menu layer
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Main);
        IM_CHECK_EQ(g.NavWindow, test_window);

        ctx->MenuClick("InputText1");
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Menu);
        ctx->KeyPress(ImGuiKey_Escape); // Deactivate InputText
        ctx->KeyPress(ImGuiKey_Escape); // Leave menu layer
        ctx->Yield();
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Main);
        IM_CHECK_EQ(g.NavWindow, test_window);

#if IMGUI_BROKEN_TESTS
        ctx->MenuClick("Table/InputText2"); // FIXME-TESTS: This doesn't work because "../Table" in MenuAction doesn't resolve to an item, and even if it did it would try to click it.
#else
        ctx->ItemClick("##MenuBar/Table/InputText2");
#endif
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Menu);
        ctx->KeyPress(ImGuiKey_Escape); // Deactivate InputText
        ctx->KeyPress(ImGuiKey_Escape); // Leave menu layer
        ctx->Yield();
        IM_CHECK_EQ(g.NavLayer, ImGuiNavLayer_Main);
        IM_CHECK_EQ(g.NavWindow, test_window);
    };

#ifdef IMGUI_HAS_MULTISELECT

    static const char* ExampleNames[] =
    {
        "Artichoke", "Arugula", "Asparagus", "Avocado", "Bamboo Shoots", "Bean Sprouts", "Beans", "Beet", "Belgian Endive", "Bell Pepper",
        "Bitter Gourd", "Bok Choy", "Broccoli", "Brussels Sprouts", "Burdock Root", "Cabbage", "Calabash", "Capers", "Carrot", "Cassava",
        "Cauliflower", "Celery", "Celery Root", "Celcuce", "Chayote", "Chinese Broccoli", "Corn", "Cucumber", "AAAB",
    };

    // ## Test MultiSelect API
    struct ExampleSelection : public ImGuiSelectionBasicStorage
    {
        void EmitBasicItems(ImGuiMultiSelectIO* ms_io, int items_count, const char* label_format)
        {
            for (int item_n = 0; item_n < items_count; item_n++)
            {
                const ImGuiID item_id = GetStorageIdFromIndex(item_n);
                const bool item_selected = Contains(item_id);
                ImGui::SetNextItemSelectionUserData(item_n);
                ImGui::Selectable(Str16f(label_format, item_n).c_str(), item_selected);
                //if (ImGui::IsItemToggledSelection())
                //    SetSelected(item_n, !item_selected);
            }
        }
        void EmitBasicLoop(ImGuiMultiSelectFlags flags, int items_count, const char* label_format)
        {
            ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, Size, items_count);
            ApplyRequests(ms_io);
            EmitBasicItems(ms_io, items_count, label_format);
            ms_io = ImGui::EndMultiSelect();
            ApplyRequests(ms_io);
        }
    };
    struct MultiSelectTestVars
    {
        ExampleSelection        Selection0;
        ExampleSelection        Selection1;
        bool                    Test0 = false;
        ImGuiMultiSelectFlags   MultiSelectFlags = ImGuiMultiSelectFlags_None;
    };
    auto multiselect_guifunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;

        const int ITEMS_COUNT = 100;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("(Size = %3d items)", selection.Size);
        ImGui::Separator();

        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(vars.MultiSelectFlags, selection.Size, ITEMS_COUNT);
        selection.ApplyRequests(ms_io);

        if (ctx->Test->ArgVariant == 1)
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));

        ImGuiListClipper clipper;
        clipper.Begin(ITEMS_COUNT);
        if (ms_io->RangeSrcItem > 0)
            clipper.IncludeItemByIndex((int)ms_io->RangeSrcItem);
        while (clipper.Step())
        {
            for (int item_n = clipper.DisplayStart; item_n < clipper.DisplayEnd; item_n++)
            {
                Str64f label("Object %04d", item_n);
                bool item_selected = selection.Contains(item_n);

                ImGui::SetNextItemSelectionUserData(item_n);
                if (ctx->Test->ArgVariant == 0)
                {
                    ImGui::Selectable(label.c_str(), item_selected);
                    //if (ImGui::IsItemToggledSelection())
                    //{
                    //    ImGui::DebugLog("Item %d toggled selection %d->%d\n", item_n, item_selected, !item_selected);
                    //    selection.SetSelected(item_n, !item_selected);
                    //}
                }
                else if (ctx->Test->ArgVariant == 1)
                {
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
                    flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
                    if (item_selected)
                        flags |= ImGuiTreeNodeFlags_Selected;
                    if (ImGui::TreeNodeEx(label.c_str(), flags))
                        ImGui::TreePop();
                    //if (ImGui::IsItemToggledSelection())
                    //{
                    //    ImGui::DebugLog("Item %d toggled selection %d->%d\n", item_n, item_selected, !item_selected);
                    //    selection.SetSelected(item_n, !item_selected);
                    //}
                }
            }
        }

        if (ctx->Test->ArgVariant == 1)
            ImGui::PopStyleVar();

        ms_io = ImGui::EndMultiSelect();
        selection.ApplyRequests(ms_io);

        ImGui::End();
    };
    auto multiselect_testfunc = [](ImGuiTestContext* ctx)
    {
        // We are using lots of MouseMove+MouseDown+MouseUp (instead of ItemClick) because we need to test precise MouseUp vs MouseDown reactions.
        ImGuiContext& g = *ctx->UiContext;
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;

        const bool is_treenode_test = (ctx->Test->ArgVariant == 1);

        ctx->SetRef("Test Window");
        for (int step = 0; step < 2; step++)
        {
            ctx->LogDebug("STEP %d", step);
            vars.MultiSelectFlags = (step == 0) ? ImGuiMultiSelectFlags_SelectOnClick : ImGuiMultiSelectFlags_SelectOnClickRelease;
            vars.MultiSelectFlags |= ImGuiMultiSelectFlags_ClearOnEscape;

            selection.Clear();
            ctx->Yield();
            IM_CHECK_EQ(selection.Size, 0);

            // Single click
            ctx->ItemClick("Object 0000");
            IM_CHECK_EQ(selection.Size, 1);
            IM_CHECK_EQ(selection.Contains(0), true);

            // Verify that click on another item alter selection on MouseDown
            ctx->MouseMove("Object 0001");
            if (vars.MultiSelectFlags & ImGuiMultiSelectFlags_SelectOnClickRelease)
            {
                ctx->MouseDown(0);
                IM_CHECK_EQ(selection.Size, 1);
                IM_CHECK_EQ(selection.Contains(0), true);
                ctx->MouseUp(0);
                IM_CHECK_EQ(selection.Size, 1);
                IM_CHECK_EQ(selection.Contains(1), true);
            }
            else
            {
                ctx->MouseDown(0);
                IM_CHECK_EQ(selection.Size, 1);
                IM_CHECK_EQ(selection.Contains(1), true);
                ctx->MouseUp(0);
            }

            // CTRL-A
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
            IM_CHECK_EQ(selection.Size, 100);

            // Verify that click on selected item clear other items from selection on MouseUp
            ctx->MouseMove("Object 0001");
            ctx->MouseDown(0);
            IM_CHECK_EQ(selection.Size, 100);
            ctx->MouseUp(0);
            IM_CHECK_EQ(selection.Size, 1);
            IM_CHECK_EQ(selection.Contains(1), true);

            // Test SHIFT+Click
            ctx->ItemClick("Object 0001");
            ctx->KeyDown(ImGuiMod_Shift);
            ctx->MouseMove("Object 0006");
            if (vars.MultiSelectFlags & ImGuiMultiSelectFlags_SelectOnClickRelease)
            {
                ctx->MouseDown(0);
                IM_CHECK_EQ(selection.Size, 1);
                ctx->MouseUp(0);
                IM_CHECK_EQ(selection.Size, 6);
            }
            else
            {
                ctx->MouseDown(0);
                IM_CHECK_EQ(selection.Size, 6);
                ctx->MouseUp(0);
            }
            ctx->KeyUp(ImGuiMod_Shift);

            // Test that CTRL+A preserve RangeSrc (which was 0001)
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
            IM_CHECK_EQ(selection.Size, 100);
            ctx->KeyDown(ImGuiMod_Shift);
            ctx->ItemClick("Object 0008");
            ctx->KeyUp(ImGuiMod_Shift);
            IM_CHECK_EQ(selection.Size, 8); // 0001->0008

            // Test reverse clipped SHIFT+Click
            // FIXME-TESTS: ItemInfo query could disable clipper?
            // FIXME-TESTS: We would need to disable clipper because it conveniently rely on cliprect which is affected by actual viewport, so ScrollToBottom() is not enough...
            //ctx->ScrollToBottom();
            ctx->ItemClick("Object 0030");
            ctx->ScrollToTop("");
            ctx->KeyDown(ImGuiMod_Shift);
            ctx->ItemClick("Object 0002");
            ctx->KeyUp(ImGuiMod_Shift);
            IM_CHECK_EQ(selection.Size, 29);

            // Test ESC to clear selection
            // FIXME-TESTS: Feature not implemented
            ctx->KeyPress(ImGuiKey_Escape);
            ctx->Yield();
            IM_CHECK_EQ(selection.Size, 0);

            // Test CTRL+Click
#if IMGUI_VERSION_NUM >= 18730
            ctx->ItemClick("Object 0001");
            IM_CHECK_EQ(selection.Size, 1);
            ctx->KeyDown(ImGuiMod_Ctrl);
            ctx->ItemClick("Object 0006");
            ctx->ItemClick("Object 0007");
            ctx->KeyUp(ImGuiMod_Ctrl);
            IM_CHECK_EQ(selection.Size, 3);
            ctx->ItemClick("Object 0008");
            IM_CHECK_EQ(selection.Size, 1);
#endif

            // Test SHIFT+Arrow
            ctx->ItemClick("Object 0002");
            IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0002"));
            IM_CHECK_EQ(selection.Size, 1);
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_DownArrow);
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_DownArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0004"));
            IM_CHECK_EQ(selection.Size, 3);

            // Test CTRL+Arrow
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_DownArrow);
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_DownArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0006"));
            IM_CHECK_EQ(selection.Size, 3);

            // Test SHIFT+Arrow after a gap
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_DownArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0007"));
            IM_CHECK_EQ(selection.Size, 6);

            // Test SHIFT+Arrow reducing selection
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_UpArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0006"));
            IM_CHECK_EQ(selection.Size, 5);

            // Test CTRL+Shift+Arrow moving or appending without reducing selection
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_UpArrow, 4);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0002"));
            IM_CHECK_EQ(selection.Size, 5);

            // Test SHIFT+Arrow replacing selection
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_UpArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0001"));
            IM_CHECK_EQ(selection.Size, 2);

            // Test Arrow replacing selection
            ctx->KeyPress(ImGuiKey_DownArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0002"));
            IM_CHECK_EQ(selection.Size, 1);
            IM_CHECK_EQ(selection.Contains(2), true);

            // Test keyboard activation (Space/Enter)
            ctx->ItemClick("Object 0001");
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_DownArrow);
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_DownArrow);
            IM_CHECK_EQ(selection.Size, 1);
            IM_CHECK_EQ(selection.Contains(1), true);
            ctx->KeyPress(ImGuiKey_Space); // Over 0003 while not selected
            IM_CHECK_EQ(selection.Size, 1);
            IM_CHECK_EQ(selection.Contains(3), true);
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_DownArrow);
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Space); // Over 0004
            IM_CHECK_EQ(selection.Size, 2);
            IM_CHECK_EQ(selection.Contains(3), true);
            IM_CHECK_EQ(selection.Contains(4), true);
            ctx->KeyPress(ImGuiKey_Space); // Over 0004 while selected
            IM_CHECK_EQ(selection.Size, 1);
            IM_CHECK_EQ(selection.Contains(4), true);
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_DownArrow, 4);
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_Space); // Over 0008 while not selected
            IM_CHECK_EQ(selection.Size, 5);
            IM_CHECK_EQ(selection.Contains(4), true);
            IM_CHECK_EQ(selection.Contains(8), true);

            // Basic test for keyboard open/close
            if (is_treenode_test)
            {
                ctx->ItemClick("Object 0001");
                ctx->ItemClose("Object 0001");
                ctx->KeyPress(ImGuiKey_RightArrow);
                IM_CHECK(ctx->ItemIsOpened("Object 0001") == true);
                ctx->KeyPress(ImGuiKey_LeftArrow);
                IM_CHECK(ctx->ItemIsOpened("Object 0001") == false);
            }

            // Test Home/End
            ctx->KeyPress(ImGuiKey_Home);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0000"));
            IM_CHECK_EQ(selection.Size, 1);
            IM_CHECK_EQ(selection.Contains(0), true);
            ctx->KeyPress(ImGuiKey_End);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0099"));
            IM_CHECK_EQ(selection.Size, 1);
            IM_CHECK_EQ(selection.Contains(99), true); // Would break if clipped by viewport
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Home);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0000"));
            IM_CHECK_EQ(selection.Size, 1);
            IM_CHECK_EQ(selection.Contains(99), true);
            ctx->KeyPress(ImGuiKey_Home);
            IM_CHECK_EQ(selection.Size, 1);
            IM_CHECK_EQ(selection.Contains(99), true); // FIXME: A Home/End/PageUp/PageDown leading to same target doesn't trigger JustMovedTo, may be reasonable.
            ctx->KeyPress(ImGuiKey_Space);
            IM_CHECK_EQ(selection.Size, 1);
            IM_CHECK_EQ(selection.Contains(0), true);
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_End);
            IM_CHECK_EQ(g.NavId, ctx->GetID("Object 0099"));
            IM_CHECK_EQ(selection.Size, 100);
        }
    };

    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_1_selectables");
    t->SetVarsDataType<MultiSelectTestVars>();
    t->GuiFunc = multiselect_guifunc;
    t->TestFunc = multiselect_testfunc;
    t->ArgVariant = 0;

    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_2_treenode");
    t->SetVarsDataType<MultiSelectTestVars>();
    t->GuiFunc = multiselect_guifunc;
    t->TestFunc = multiselect_testfunc;
    t->ArgVariant = 1;

    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_3_multiple");
    t->SetVarsDataType<MultiSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();

        const int ITEMS_COUNT = 10;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        for (int scope_n = 0; scope_n < 2; scope_n++)
        {
            ExampleSelection& selection = (scope_n == 1) ? vars.Selection1 : vars.Selection0;
            ImGui::Text("SCOPE %d\n", scope_n);
            ImGui::PushID(Str16f("Scope %d", scope_n).c_str());
            selection.EmitBasicLoop(ImGuiMultiSelectFlags_None, ITEMS_COUNT, "Object %04d");
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection0 = vars.Selection0;
        ExampleSelection& selection1 = vars.Selection1;

        ctx->SetRef("Test Window");
        selection0.Clear();
        selection1.Clear();

        ctx->ItemClick("Scope 0/Object 0001");
        IM_CHECK_EQ(selection0.Size, 1);
        IM_CHECK_EQ(selection1.Size, 0);

        ctx->ItemClick("Scope 1/Object 0002");
        IM_CHECK_EQ(selection0.Size, 1);
        IM_CHECK_EQ(selection1.Size, 1);

        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
        IM_CHECK_EQ(selection0.Size, 1);
        IM_CHECK_EQ(selection1.Size, 10);

        // Check leaving a scope with keyboard directional move
        // May need to clarify how this behave depending on "leaving" source, as mouse clicks don't impact this.
#if IMGUI_VERSION_NUM >= 18983
        ctx->ItemClick("Scope 1/Object 0000");
        IM_CHECK_EQ(selection0.Size, 1);
        IM_CHECK_EQ(selection1.Size, 1);
        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK_EQ(selection0.Size, 1);
        IM_CHECK_EQ(selection1.Size, 0);
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(selection0.Size, 0);
        IM_CHECK_EQ(selection1.Size, 1);
#endif
    };

    // Test mouse-right click on unfocused window
    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_unfocused");
    t->SetVarsDataType<MultiSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        selection.EmitBasicLoop(ImGuiMultiSelectFlags_ClearOnEscape, 50, "Object %03d");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ctx->ItemClick("//Test Window/Object 001");
        IM_CHECK(selection.Size == 1 && selection.Contains(1));
        ctx->WindowFocus("Dear ImGui Demo");
        ctx->WindowMove("Dear ImGui Demo", ctx->GetWindowByRef("Test Window")->Rect().GetTR());
        IM_CHECK(selection.Size == 1 && selection.Contains(1));
        ctx->MouseMove("//Test Window/Object 006", ImGuiTestOpFlags_NoFocusWindow);
        IM_CHECK(g.NavWindow != NULL);
        IM_CHECK_STR_EQ(g.NavWindow->Name, "Dear ImGui Demo");
        ctx->MouseClick(ImGuiMouseButton_Right);
        IM_CHECK(g.NavWindow != NULL);
        IM_CHECK_STR_EQ(g.NavWindow->Name, "Test Window");
        IM_CHECK(selection.Size == 1);
        IM_CHECK(selection.Contains(6) == true);
    };

    // Test Enter key behaviors
    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_enter");
    t->SetVarsDataType<MultiSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        selection.EmitBasicLoop(ImGuiMultiSelectFlags_ClearOnEscape, 50, "Object %03d");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ctx->SetRef("Test Window");

        // Enter alters selection because current item is not selected
        ctx->ItemClick("Object 000");
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_DownArrow);
        IM_CHECK_EQ(selection.Size, 1);
        IM_CHECK(selection.Contains(0) == true);
        IM_CHECK(selection.Contains(1) == false);
        ctx->KeyPress(ImGuiKey_Enter);
        IM_CHECK_EQ(selection.Size, 1);
        IM_CHECK(selection.Contains(0) == false);
        IM_CHECK(selection.Contains(1) == true);

        // Enter doesn't alter selection because current item is selected
        ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_DownArrow);
        IM_CHECK_EQ(selection.Size, 2);
        IM_CHECK(selection.Contains(1) == true);
        IM_CHECK(selection.Contains(2) == true);
        ctx->KeyPress(ImGuiKey_Enter);
        IM_CHECK_EQ(selection.Size, 2);
        IM_CHECK(selection.Contains(1) == true);
        IM_CHECK(selection.Contains(2) == true);
    };

    // Test interleaving CollapsingHeader() between selection items
    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_disjoint");
    t->SetVarsDataType<MultiSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_ClearOnEscape, selection.Size, 50);
        selection.ApplyRequests(ms_io);
        for (int n = 0; n < 50; n++)
        {
            if ((n % 10) == 0 && ImGui::CollapsingHeader(Str64f("Section %02d-> %02d", n, n + 9).c_str()))
            {
                n += 9;
                continue;
            }
            bool item_selected = selection.Contains(n);
            ImGui::SetNextItemSelectionUserData(n);
            ImGui::Selectable(Str64f("Object %03d", n).c_str(), item_selected);
            //if (ImGui::IsItemToggledSelection())
            //    selection.SetSelected(n, !item_is_selected);
        }
        ms_io = ImGui::EndMultiSelect();
        selection.ApplyRequests(ms_io);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ctx->SetRef("Test Window");
        ctx->ItemClick("Object 000");
        IM_CHECK_EQ(selection.Size, 1);
        IM_CHECK(selection.Contains(0) == true);
        ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_UpArrow);
        IM_CHECK_EQ(selection.Size, 1);
        IM_CHECK(selection.Contains(0) == true);
        ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_DownArrow);
        IM_CHECK_EQ(selection.Size, 1);
        IM_CHECK(selection.Contains(0) == true);
        ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_DownArrow, 10);
        IM_CHECK_EQ(selection.Size, 10);
        ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_DownArrow);
        IM_CHECK_EQ(selection.Size, 11);
    };

    // Test range-select with no RangeSource
    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_nosrc");
    t->SetVarsDataType<MultiSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        selection.EmitBasicLoop(ImGuiMultiSelectFlags_ClearOnEscape, 50, "Object %03d");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ctx->UiContext->NavId = 0;
        ctx->KeyDown(ImGuiMod_Shift);
        ctx->ItemClick("//Test Window/Object 005");
        IM_CHECK(selection.Size == 6);
        IM_CHECK(selection.Contains(0) == true);
        IM_CHECK(selection.Contains(5) == true);
    };

    // Test range-select with ImGuiMultiSelectFlags_SingleSelect flag (previously ImGuiMultiSelectFlags_NoMultiSelect)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_singleselect");
    t->SetVarsDataType<MultiSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
#if IMGUI_VERSION_NUM < 18980
        const ImGuiMultiSelectFlags ImGuiMultiSelectFlags_SingleSelect = ImGuiMultiSelectFlags_NoMultiSelect;
#endif
        selection.EmitBasicLoop(ImGuiMultiSelectFlags_SingleSelect, 50, "Object %03d");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;

        // Shift+Arrow move
        ctx->ItemClick("//Test Window/Object 000");
        ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_DownArrow);
        IM_CHECK(selection.Size == 1);
        IM_CHECK(selection.Contains(0) == false);
        IM_CHECK(selection.Contains(1) == true);

        // Shift+Click
        ctx->KeyDown(ImGuiMod_Shift);
        ctx->ItemClick("//Test Window/Object 005");
        ctx->KeyUp(ImGuiMod_Shift);
        IM_CHECK(selection.Size == 1);
        IM_CHECK(selection.Contains(1) == false);
        IM_CHECK(selection.Contains(5) == true);

        // Ctrl+Click
        ctx->KeyDown(ImGuiMod_Ctrl);
        ctx->ItemClick("//Test Window/Object 006");
        ctx->KeyUp(ImGuiMod_Ctrl);
        IM_CHECK(selection.Size == 1);
        IM_CHECK(selection.Contains(5) == false);
        IM_CHECK(selection.Contains(6) == true);

        // Ctrl+Down, Ctrl+Space
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_DownArrow);
        IM_CHECK(selection.Size == 1);
        IM_CHECK(selection.Contains(6) == true);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Space);
        IM_CHECK(selection.Size == 1);
        IM_CHECK(selection.Contains(6) == false);
        IM_CHECK(selection.Contains(7) == true);
    };

#if IMGUI_VERSION_NUM >= 18996
    // Test nested/recursive BeginMultiSelect()
    // FIXME: This doesn't test any behavior merely that the thing doesn't assert/crash
    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_nested");
    t->SetVarsDataType<MultiSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_None, selection.Size, 50);
        selection.ApplyRequests(ms_io);
        selection.EmitBasicItems(ms_io, 50, "Object %03d");
        if (ImGui::Button("Open Popup"))
            ImGui::OpenPopup("Popup");
        if (ImGui::BeginPopup("Popup"))
        {
            ImGuiMultiSelectIO* ms_io_2 = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_None, selection.Size, 50);
            selection.ApplyRequests(ms_io_2);
            selection.EmitBasicItems(ms_io_2, 50, "Object %03d");
            ms_io_2 = ImGui::EndMultiSelect();
            selection.ApplyRequests(ms_io_2);
            ImGui::EndPopup();
        }
        ms_io = ImGui::EndMultiSelect();
        selection.ApplyRequests(ms_io);
        //IM_CHECK(ms_io_1->RangeSrcItem == ImGuiSelectionUserData_Invalid);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->ItemClick("//Test Window/Open Popup");
    };
#endif

    // Test validity of ImGuiMultiSelectIO data.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_io_lifetime");
    t->SetVarsDataType<MultiSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        ImGuiMultiSelectIO* ms_io_1 = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_None, selection.Size, 50);
        if (vars.Test0)
            IM_CHECK(ms_io_1->RangeSrcItem == 5);
        selection.EmitBasicItems(ms_io_1, 50, "Object %03d");
        selection.ApplyRequests(ms_io_1);
        ImGuiMultiSelectIO* ms_io_2 = ImGui::EndMultiSelect();
        if (vars.Test0)
            IM_CHECK(ms_io_2->RangeSrcItem == 5);
        selection.ApplyRequests(ms_io_2);
        //IM_CHECK(ms_io_1->RangeSrcItem == ImGuiSelectionUserData_Invalid);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ctx->ItemClick("//Test Window/Object 005");
        vars.Test0 = true;
        ctx->Yield(2);
    };

    // Test default selection on NavInit
#if IMGUI_BROKEN_TESTS // NavInit happens once
#if IMGUI_VERSION_NUM >= 18958
    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_init_child");
    t->SetVarsDataType<MultiSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Button("Dummy");
        ImGui::BeginChild("Child", ImVec2(0, 300));
        selection.EmitBasicLoop(ImGuiMultiSelectFlags_None, 50, "Object %03d");
        ImGui::EndChild();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        ctx->KeyPress(ImGuiKey_DownArrow);
        ctx->KeyPress(ImGuiKey_Enter);
        IM_CHECK(selection.Size == 1);
        IM_CHECK(selection.Contains(0) == true);
    };
#endif
#endif

    // ## Test BeginMultiSelect() API with non-selectable checkboxes, to benefit from range-select features
#if IMGUI_VERSION_NUM >= 19044
    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_checkboxes");
    struct MultiSelectCheckboxesVars
    {
        bool    ApplyBeginRequests = false;
        int     ItemsCount = 20;
        ImU64   Mask = 0;

        void Clear() { Mask = 0; }
    };
    t->SetVarsDataType<MultiSelectCheckboxesVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<MultiSelectCheckboxesVars>();

        // Apply multi-select requests over a bool items[] array, where ImGuiSelectionUserData is item index.
        auto ApplyRequestsToBitMask = [](ImGuiMultiSelectIO* ms_io, ImU64& mask, int items_count)
        {
            for (ImGuiSelectionRequest& req : ms_io->Requests)
            {
                if (req.Type == ImGuiSelectionRequestType_SetAll)
                    for (int n = 0; n < items_count; n++)
                        mask = req.Selected ? (mask | ((ImU64)1 << n)) : (mask & ~((ImU64)1 << n));
                if (req.Type == ImGuiSelectionRequestType_SetRange)
                    for (int n = (int)req.RangeFirstItem; n <= (int)req.RangeLastItem; n++)
                        mask = req.Selected ? (mask | ((ImU64)1 << n)) : (mask & ~((ImU64)1 << n));
            }
        };

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_NoAutoClear | ImGuiMultiSelectFlags_NoAutoSelect | ImGuiMultiSelectFlags_ClearOnEscape);
        ImGui::Button("Header");
        if (vars.ApplyBeginRequests) // Verify that system can function without this
            ApplyRequestsToBitMask(ms_io, vars.Mask, vars.ItemsCount);
        for (int n = 0; n < vars.ItemsCount; n++)
        {
            ImGui::SetNextItemSelectionUserData(n);
            ImGui::CheckboxFlags(Str30f("Item %d", n).c_str(), &vars.Mask, (ImU64)1 << n);
        }
        ImGui::EndMultiSelect();
        ApplyRequestsToBitMask(ms_io, vars.Mask, vars.ItemsCount);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<MultiSelectCheckboxesVars>();

#if IMGUI_BROKEN_TESTS
        for (int step = 0; step < 2; step++) // Broken if clipped
#else
        for (int step = 0; step < 1; step++)
#endif
        {
            vars.Clear();
            vars.ApplyBeginRequests = (step == 0);

            // Clicks
            ctx->SetRef("Test Window");
            ctx->ItemClick("Item 1");
            IM_CHECK_EQ(vars.Mask, (ImU64)(0x02));
            ctx->ItemClick("Item 1");
            IM_CHECK_EQ(vars.Mask, (ImU64)0);
            ctx->ItemClick("Item 1");
            ctx->ItemClick("Item 2");
            IM_CHECK_EQ(vars.Mask, (ImU64)(0x02 | 0x04));

            // Shift+Click
            ctx->KeyDown(ImGuiMod_Shift);
            ctx->ItemClick("Item 6"); // Set 2-6
            IM_CHECK_EQ(vars.Mask, (ImU64)(0x02 | 0x04 | 0x08 | 0x10 | 0x20 | 0x40));
            ctx->ItemClick("Item 6"); // Clear 2-6
            IM_CHECK_EQ(vars.Mask, (ImU64)(0x02));
            ctx->ItemClick("Item 6"); // Set 2-6
            ctx->ItemClick("Item 7"); // Set 2-7
            IM_CHECK_EQ(vars.Mask, (ImU64)(0x02 | 0x04 | 0x08 | 0x10 | 0x20 | 0x40 | 0x80));
            ctx->KeyUp(ImGuiMod_Shift);

            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
            IM_CHECK_EQ(vars.Mask, ((ImU64)1 << vars.ItemsCount) - 1);
            ctx->KeyPress(ImGuiKey_Home);
            ctx->KeyPress(ImGuiKey_DownArrow); // Skip Button
            IM_CHECK_EQ(vars.Mask, ((ImU64)1 << vars.ItemsCount) - 1);
            ctx->KeyPress(ImGuiKey_Space);
            IM_CHECK_EQ(vars.Mask, (((ImU64)1 << vars.ItemsCount) - 1) ^ 0x01);
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_End);
            IM_CHECK_EQ(vars.Mask, (ImU64)0);

            // Shift+Arrows
            ctx->KeyPress(ImGuiKey_Home);
            ctx->KeyPress(ImGuiKey_DownArrow); // Skip Button
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_DownArrow);
            IM_CHECK_EQ(vars.Mask, (ImU64)0);
            ctx->KeyPress(ImGuiKey_Space);
            IM_CHECK_EQ(vars.Mask, (ImU64)(0x02));
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_DownArrow);
            IM_CHECK_EQ(vars.Mask, (ImU64)(0x02 | 0x04));
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_DownArrow);
            IM_CHECK_EQ(vars.Mask, (ImU64)(0x02 | 0x04 | 0x08));
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_UpArrow);
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_UpArrow);
            IM_CHECK_EQ(vars.Mask, (ImU64)(0x02 | 0x04 | 0x08));
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_UpArrow);
            IM_CHECK_EQ(vars.Mask, (ImU64)(0x01 | 0x02 | 0x04 | 0x08));

            // Test that pressing arrow updates RangeSrc
            ctx->KeyPress(ImGuiKey_DownArrow, 5);
            IM_CHECK_EQ(ImGui::GetFocusID(), ctx->GetID("Item 5")); // 0x20
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_DownArrow);
            IM_CHECK_EQ(vars.Mask, (ImU64)(0x01 | 0x02 | 0x04 | 0x08));

            // Test ImGuiMultiSelectFlags_ClearOnEscape
            ctx->KeyPress(ImGuiKey_Escape);
            IM_CHECK_EQ(vars.Mask, (ImU64)(0));

            // Test that right-click doesn't do anything
            ctx->ItemClick("Item 2", ImGuiMouseButton_Right);
            IM_CHECK_EQ(vars.Mask, (ImU64)(0));
        }
    };
#endif

#if IMGUI_VERSION_NUM >= 19071
    // ## Test box-selection + exercise Assets Browser a little.
    // This test is probably fragile because are are dealing with Assets Browser instead of duplicating it in our code.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_boxselect_1");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *GImGui;
        ctx->MenuCheck("//Dear ImGui Demo/Examples/Assets Browser");
        ctx->SetRef("Example: Assets Browser");
        ImGuiWindow* child_window = ctx->WindowInfo("Assets").Window;
        IM_CHECK(child_window != NULL);

        ctx->WindowResize("", ImVec2(750, 600)); // FIXME: Calculated for 15 item wide (15 * 32) + (15-1+2) * 10 + Parent/Child Padding + Scrollbar
        ctx->MenuClick("File/Clear items");
        ctx->MenuClick("File/Add 10000 items");

        // Zoom/unzoom
        ctx->SetRef(child_window);
        ctx->ItemClick("$$20");
        ctx->KeyDown(ImGuiMod_Ctrl);
        for (int n = 0; n < 10; n++)
            ctx->MouseWheelY(-1.0f);
        for (int n = 0; n < 10; n++)
            ctx->MouseWheelY(+1.0f);
        ctx->KeyUp(ImGuiMod_Ctrl);
        ctx->KeyPress(ImGuiKey_Escape);

        // Set specific option
        ctx->SetRef("Example: Assets Browser");
        ctx->MenuAction(ImGuiTestAction_Open, "Options");
        ctx->ItemInputValue("//$FOCUSED/Icon Size", 32.0f);
        ctx->ItemInputValue("//$FOCUSED/Icon Spacing", 10);
        ctx->ItemInputValue("//$FOCUSED/Icon Hit Spacing", 4);

        ctx->SetRef(child_window);
        ctx->ScrollToTop("");
        ImGuiMultiSelectState* ms_storage = ImGui::GetMultiSelectState(ctx->GetID(""));
        IM_CHECK(ms_storage != NULL);
        IM_CHECK_EQ(ms_storage->LastSelectionSize, 0);
        //g.DebugLogFlags |= ImGuiDebugLogFlags_OutputToTestEngine;
        ctx->ItemClick("$$7");
        IM_CHECK_EQ(ms_storage->LastSelectionSize, 1);
        ctx->KeyPress(ImGuiKey_Escape);
        IM_CHECK_EQ(ms_storage->LastSelectionSize, 0);

        // Box-select part of a row, then a rectangle
        ctx->ItemDragAndDrop("$$0", "$$5");
        IM_CHECK_EQ(ms_storage->LastSelectionSize, 6);
        ctx->ItemDragAndDrop("$$20", "$$105");
        IM_CHECK_EQ(ms_storage->LastSelectionSize, 6 * 6);
        ctx->KeyPress(ImGuiKey_Escape);

        // Box-select from void
        ImGuiTestItemInfo item_1 = ctx->ItemInfo("$$1");
        ImGuiTestItemInfo item_2 = ctx->ItemInfo("$$2");
        ctx->MouseMoveToPos((item_1.RectFull.GetCenter() + item_2.RectFull.GetCenter()) * 0.5f);
        IM_CHECK(ImGui::IsAnyItemHovered() == false);
        IM_CHECK_EQ(ms_storage->LastSelectionSize, 0);
        ctx->MouseClick(0);
        IM_CHECK_EQ(ms_storage->LastSelectionSize, 0); // Verify we are over void
        ctx->MouseDown(0);
        ctx->MouseMove("$$22", ImGuiTestOpFlags_NoCheckHoveredId);
        ctx->MouseUp(0);
        IM_CHECK_EQ(ms_storage->LastSelectionSize, 2 * 5);

        // Verify that first selected item was turned into NavId
        IM_CHECK_EQ(g.NavId, ctx->GetID("$$2"));

        // Exercise box-select scroll (in a very basic way)
        IM_CHECK_EQ(child_window->Scroll.y, 0.0f);
        ctx->ItemClick("$$0");
        ctx->MouseMove("$$3");
        ctx->MouseDown(0);
        ctx->MouseMoveToPos(ImGui::GetMousePos() + ImVec2(0, 1000));
        ctx->SleepNoSkip(2.0f, 0.1f);
        ctx->MouseUp(0);
        IM_CHECK_GT(child_window->Scroll.y, 0.0f);

        ctx->KeyPress(ImGuiKey_Escape);

        ctx->MenuUncheck("//Dear ImGui Demo/Examples/Assets Browser");
    };
#endif

#if IMGUI_VERSION_NUM >= 19114
    // ## Test box-selection in table with decorations (#7970, #7821)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_boxselect_2");
    struct BoxSelectTestVars { ImGuiTableFlags TableFlags = ImGuiTableFlags_ScrollY; ImGuiSelectionBasicStorage Selection; bool FrozenHeaders = false; };
    t->SetVarsDataType<BoxSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<BoxSelectTestVars>();
        ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::CheckboxFlags("BordersOuter", &vars.TableFlags, ImGuiTableFlags_BordersOuter);
        ImGui::SameLine();
        ImGui::CheckboxFlags("BordersInner", &vars.TableFlags, ImGuiTableFlags_BordersInner);
        if (ImGui::BeginTable("table1", 1, vars.TableFlags))
        {
            auto ms = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_BoxSelect1d, vars.Selection.Size, 1000);
            vars.Selection.ApplyRequests(ms);
            if (vars.FrozenHeaders)
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();
            }
            for (unsigned i = 0; i < 1000; i++)
            {
                char buf[32];
                snprintf(buf, sizeof buf, "Item %03d", i);
                ImGui::SetNextItemSelectionUserData(i);
                ImGui::TableNextColumn();
                ImGui::Selectable(buf, vars.Selection.Contains(i));
            }
            ms = ImGui::EndMultiSelect();
            vars.Selection.ApplyRequests(ms);
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<BoxSelectTestVars>();
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("Test Window/table1"));
        IM_CHECK(table != NULL);

        for (int step = 0; step < 4; step++)
        {
            vars.Selection.Clear();
            vars.TableFlags = (step & 1) ? (vars.TableFlags | ImGuiTableFlags_BordersOuter) : (vars.TableFlags & ~ImGuiTableFlags_BordersOuter);
            vars.FrozenHeaders = (step & 2) != 0;

            ctx->SetRef(table->ID);
            ctx->MouseMove("Item 001");
            ctx->MouseDown(ImGuiMouseButton_Left);
            //ctx->SetRef("//$FOCUSED");
            ctx->MouseMoveToPos(ImGui::GetIO().MousePos + ImVec2(0, ctx->GetWindowByRef("//Test Window")->Size.y));
            ctx->SleepNoSkip(1.0f, 0.05f);
            ctx->MouseUp(ImGuiMouseButton_Left);
            IM_CHECK(vars.Selection.Contains(0) == false);
            int first_selected = 1;
            int last_selected = 1;
            for (int n = 1; n < 1000; n++)
            {
                if (vars.Selection.Contains(n) == false)
                    break;
                last_selected = n;
            }
            IM_CHECK(vars.Selection.Size > 1);
            IM_CHECK_EQ(vars.Selection.Size, last_selected - first_selected + 1); // Check no selection gap (#7970)
        }
    };
#endif

    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_batch_requests");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiSelectionBasicStorage selection;

        ImGuiMultiSelectIO io;
        io.ItemsCount = 100;
        for (int idx = 90; idx >= 10; idx -= 10)
            io.Requests.push_back({ ImGuiSelectionRequestType_SetRange, true, +1, idx, idx });
        selection.ApplyRequests(&io);
        IM_CHECK_EQ(selection.Size, 9);
        io.Requests.clear();
        io.Requests.push_back({ ImGuiSelectionRequestType_SetRange, false, +1, 60, 60 });
        selection.ApplyRequests(&io);
        IM_CHECK_EQ(selection.Size, 8);

        io.Requests.clear();
        io.Requests.push_back({ ImGuiSelectionRequestType_SetRange, true, +1, 12, 15 });
        selection.ApplyRequests(&io);
        IM_CHECK_EQ(selection.Size, 8 + 4);
        io.Requests.clear();
        io.Requests.push_back({ ImGuiSelectionRequestType_SetRange, false, +1, 12, 15 });
        selection.ApplyRequests(&io);
        IM_CHECK_EQ(selection.Size, 8);

        // Multiple set ranges (emulates what BoxSelect can do)
        io.Requests.clear();
        io.Requests.push_back({ ImGuiSelectionRequestType_SetRange, false, +1, 12, 15 }); // Need no effect
        io.Requests.push_back({ ImGuiSelectionRequestType_SetRange, false, +1, 12, 15 }); // Need duplicates
        io.Requests.push_back({ ImGuiSelectionRequestType_SetRange, true, +1, 25, 25 });
        io.Requests.push_back({ ImGuiSelectionRequestType_SetRange, false, +1, 40, 40 });
        io.Requests.push_back({ ImGuiSelectionRequestType_SetRange, true, +1, 65, 65 });
        selection.ApplyRequests(&io);
        IM_CHECK_EQ(selection.Size, 8 + 2 - 1);
    };

    // ## Exercise our workaround for #8250 to call TableEndCell() in BeginMultiSelect(), in order to at minima try to detect if calling TableEndCell() won't introduce later issues.
#if IMGUI_VERSION_NUM >= 19163
    t = IM_REGISTER_TEST(e, "widgets", "widgets_multiselect_cell_width_loss");
    t->SetVarsDataType<MultiSelectTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ImGui::SetNextWindowSize(ImVec2(400, 400));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInner))
        {
            ImGui::TableSetupColumn("Object");
            ImGui::TableSetupColumn("Action");
            ImGui::TableSetupScrollFreeze(0, 1);

            if (ImGui::TableNextColumn())
                ImGui::TextUnformatted("A wide item in column 1");
            if (ImGui::TableNextColumn())
                ImGui::TextUnformatted("A wide item in column 2"); // <-- width of this was lost by BeginMultiSelect()

            const int ITEMS_COUNT = 50;
            auto& selection = vars.Selection0;
            ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;
            ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, selection.Size, ITEMS_COUNT);
            selection.ApplyRequests(ms_io);

            ImGuiListClipper clipper;
            clipper.Begin(ITEMS_COUNT);
            if (ms_io->RangeSrcItem != -1)
                clipper.IncludeItemByIndex((int)ms_io->RangeSrcItem); // Ensure RangeSrc item is not clipped.
            while (clipper.Step())
            {
                for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    bool item_is_selected = selection.Contains((ImGuiID)n);
                    ImGui::SetNextItemSelectionUserData(n);
                    ImGui::Selectable(Str64f("Object %05d: %s", n, ExampleNames[n % IM_ARRAYSIZE(ExampleNames)]).c_str(), item_is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
                    ImGui::TableNextColumn();

                    if (!ctx->IsFirstGuiFrame())
                        IM_CHECK_GE(ImGui::GetContentRegionAvail().x, ImGui::CalcTextSize("A wide item in column 2").x); // <---- verify

                    ImGui::SmallButton("hello");
                }
            }
            ms_io = ImGui::EndMultiSelect();
            selection.ApplyRequests(ms_io);

            ImGui::EndTable();
        }
        ImGui::End();
    };
#endif

    // ## Basic test for GetTypingSelectRequest()
    // Technically this API doesn't require MultiSelect but it's easier for us to reuse that code.
    // (under IMGUI_HAS_MULTISELECT as we also use this API + vars)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_typingselect");
    t->SetVarsDataType<MultiSelectTestVars>();
    struct TypingSelectTestFuncs
    {
        static const char* GetItemName(int n)
        {
            return ExampleNames[n % IM_ARRAYSIZE(ExampleNames)];
        }
    };
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ExampleSelection& selection = vars.Selection0;
        //selection.OptMangleItemData = false;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        const int items_count = IM_ARRAYSIZE(ExampleNames);
        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_None, selection.Size, items_count);
        selection.ApplyRequests(ms_io);

        int request_focus_idx = -1;
        if (ImGui::IsWindowFocused())
            if (ImGuiTypingSelectRequest* req = ImGui::GetTypingSelectRequest(ImGuiTypingSelectFlags_AllowSingleCharMode))
                request_focus_idx = ImGui::TypingSelectFindMatch(req, items_count, [](void*, int idx) { return TypingSelectTestFuncs::GetItemName(idx); }, NULL, (int)ms_io->NavIdItem);

        for (int item_n = 0; item_n < items_count; item_n++)
        {
            bool item_selected = selection.Contains(item_n);
            ImGui::SetNextItemSelectionUserData(item_n);
            ImGui::Selectable(TypingSelectTestFuncs::GetItemName(item_n), item_selected);
            if (item_n == request_focus_idx)
                ImGui::SetKeyboardFocusHere(-1);
        }
        ms_io = ImGui::EndMultiSelect();
        selection.ApplyRequests(ms_io);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *GImGui;
        MultiSelectTestVars& vars = ctx->GetVars<MultiSelectTestVars>();
        ctx->SetRef("Test Window");
        ctx->ItemClick(TypingSelectTestFuncs::GetItemName(1)); // "Arugula"
        IM_CHECK(vars.Selection0.Size == 1 && vars.Selection0.Contains(1));
        ctx->KeyChars("b"); // Start
        // At this point, TypingSelectFindResult() was called, SetKeyboardFocusHere() but we don't have our result yet.
        ctx->Yield();
        // At this point, the nav request succeeded and NavId == "Bamboo Shoots" (Item 4)
        IM_CHECK_EQ(g.NavId, ctx->GetID(TypingSelectTestFuncs::GetItemName(4)));
        IM_CHECK_EQ((int)g.NavLastValidSelectionUserData, 4);
        IM_CHECK(vars.Selection0.Size == 1 && vars.Selection0.Contains(4)); // "Bamboo Shoots"

        ctx->KeyChars("ean"); // Amend
        ctx->Yield();
        IM_CHECK(vars.Selection0.Size == 1 && vars.Selection0.Contains(5)); // "Bean Sprout"

        ctx->KeyChars("s"); // Disambiguation
        ctx->Yield();
        IM_CHECK(vars.Selection0.Size == 1 && vars.Selection0.Contains(6)); // "Beans"

        ctx->KeyChars("ccc"); // Extraneous
        ctx->Yield();
        IM_CHECK(vars.Selection0.Size == 1 && vars.Selection0.Contains(6)); // "Beans"
        ctx->SleepNoSkip(2.0f, 0.5f);

        ctx->KeyChars("c");   // New request
        ctx->Yield();
        IM_CHECK(vars.Selection0.Size == 1 && vars.Selection0.Contains(15)); // "Cabbage"

        ctx->KeyChars("c");   // Single-char mode
        ctx->Yield();
        IM_CHECK(vars.Selection0.Size == 1 && vars.Selection0.Contains(16));
        for (int sel = 17; sel < 28; sel++)
        {
            ctx->KeyChars("c");   // Single-char mode next
            ctx->Yield();
            IM_CHECK(vars.Selection0.Size == 1 && vars.Selection0.Contains(sel));
        }

        ctx->KeyChars("c");   // Single-char mode wrap
        ctx->Yield();
        IM_CHECK(vars.Selection0.Size == 1 && vars.Selection0.Contains(15));

        ctx->KeyChars("a");   // Single-char mode quick exit
        ctx->Yield();
        IM_CHECK(vars.Selection0.Size == 1 && vars.Selection0.Contains(0));

        ctx->KeyChars("a");   // Single-char to full match
        ctx->KeyChars("a");
        ctx->KeyChars("b");
        ctx->Yield();
        IM_CHECK(vars.Selection0.Size == 1 && vars.Selection0.Contains(28)); // "AAAB"

        // Verify that first single-char skips current item
        ctx->ItemClick(TypingSelectTestFuncs::GetItemName(1)); // "Arugula"
        ctx->KeyChars("a");
        ctx->Yield();
        IM_CHECK(vars.Selection0.Size == 1 && vars.Selection0.Contains(2));
    };
#endif // #ifdef IMGUI_HAS_MULTISELECT

    // ## Test Selectable() with ImGuiSelectableFlags_SpanAllColumns inside Columns()
    t = IM_REGISTER_TEST(e, "widgets", "widgets_selectable_span_all_columns");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Columns(3);
        ImGui::Button("C1");
        ImGui::NextColumn();
        ImGui::Selectable("Selectable", &vars.Bool1, ImGuiSelectableFlags_SpanAllColumns);
        vars.Status.QuerySet();
        ImGui::NextColumn();
        ImGui::Button("C3");
        ImGui::Columns();
        ImGui::PopStyleVar();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->MouseMove("C1", ImGuiTestOpFlags_NoCheckHoveredId); // Button itself won't be hovered, Selectable will!
        IM_CHECK(vars.Status.Hovered == 1);
        ctx->MouseMove("C3", ImGuiTestOpFlags_NoCheckHoveredId); // Button itself won't be hovered, Selectable will!
        IM_CHECK(vars.Status.Hovered == 1);
    };

    // ## Test ImGuiSelectableFlags_SpanAllColumns flag when used in a table.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_selectable_span_all_table");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        const int column_count = 3;
        ImGui::BeginTable("table", column_count, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Reorderable);
        for (int i = 0; i < column_count; i++)
            ImGui::TableSetupColumn(Str30f("%d", i + 1).c_str());

        ImGui::TableHeadersRow();
        ImGui::TableNextRow();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::TableSetColumnIndex(0);
        ImGui::Button("C1");
        ImGui::TableSetColumnIndex(1);
        ImGui::Selectable("Selectable", &vars.Bool1, ImGuiSelectableFlags_SpanAllColumns);
        vars.Status.QuerySet();
        ImGui::TableSetColumnIndex(2);
        ImGui::Button("C3");
        ImGui::PopStyleVar();

        ImGui::EndTable();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table"));

        for (int i = 0; i < 2; i++)
        {
            ctx->MouseMove("table/C1", ImGuiTestOpFlags_NoCheckHoveredId); // Button itself won't be hovered, Selectable will!
            IM_CHECK(vars.Status.Hovered == 1);
            ctx->MouseMove("table/C3", ImGuiTestOpFlags_NoCheckHoveredId); // Button itself won't be hovered, Selectable will!
            IM_CHECK(vars.Status.Hovered == 1);

            // Reorder columns and test again
            ctx->ItemDragAndDrop(TableGetHeaderID(table, "1"), TableGetHeaderID(table, "2"));
        }
    };

    // ## Test sliders with inverted ranges.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_slider_ranges");
    struct SliderRangeVars { struct { ImGuiDataTypeStorage Cur, Min, Max; int ResultFlags; } Values[ImGuiDataType_COUNT]; SliderRangeVars() { memset(this, 0, sizeof(*this)); } };
    t->SetVarsDataType<SliderRangeVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        SliderRangeVars& vars = ctx->GetVars<SliderRangeVars>();
        const int OUT_OF_RANGE_FLAG = 1;
        const int MID_OF_RANGE_FLAG = 2;

        // Submit sliders for each data type, with and without inverted range
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        for (ImGuiDataType data_type = 0; data_type < ImGuiDataType_COUNT; data_type++)
        {
#if IMGUI_VERSION_NUM >= 19094
            if (data_type == ImGuiDataType_Bool || data_type == ImGuiDataType_String)
                continue;
#endif
            const ImGuiDataTypeInfo* data_type_info = ImGui::DataTypeGetInfo(data_type);
            for (int invert_range = 0; invert_range < 2; invert_range++)
            {
                auto& v = vars.Values[data_type];
                GetSliderTestRanges(data_type, &v.Min, &v.Max);
                Str30f label("%s%c", data_type_info->Name, invert_range == 0 ? '+' : '-');
                if (invert_range)
                    ImGui::SliderScalar(label.c_str(), data_type, &v.Cur, &v.Max, &v.Min);
                else
                    ImGui::SliderScalar(label.c_str(), data_type, &v.Cur, &v.Min, &v.Max);

                v.ResultFlags = 0;
                if (ImGui::DataTypeCompare(data_type, &v.Cur, &v.Min) < 0 || ImGui::DataTypeCompare(data_type, &v.Cur, &v.Max) > 0)
                    v.ResultFlags |= OUT_OF_RANGE_FLAG;
                if (ImGui::DataTypeCompare(data_type, &v.Cur, &v.Min) > 0 && ImGui::DataTypeCompare(data_type, &v.Cur, &v.Max) < 0)
                    v.ResultFlags |= MID_OF_RANGE_FLAG;
                ImGui::SameLine();
                ImGui::Text("R:%d", v.ResultFlags); // Doesn't make sense to display if not reset
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        SliderRangeVars& vars = ctx->GetVars<SliderRangeVars>();
        const int OUT_OF_RANGE_FLAG = 1;
        const int MID_OF_RANGE_FLAG = 2;

        ctx->SetRef("Test Window");
        for (int data_type = 0; data_type < ImGuiDataType_COUNT; data_type++)
        {
#if IMGUI_VERSION_NUM >= 19094
            if (data_type == ImGuiDataType_Bool || data_type == ImGuiDataType_String)
                continue;
#endif
            for (int invert_range = 0; invert_range < 2; invert_range++)
            {
                auto& v = vars.Values[data_type];
                ImGuiDataTypeStorage v_left = invert_range ? v.Max : v.Min;
                ImGuiDataTypeStorage v_right = invert_range ? v.Min : v.Max;

                const ImGuiDataTypeInfo* data_type_info = ImGui::DataTypeGetInfo(data_type);
                Str30f label("%s%c", data_type_info->Name, invert_range == 0 ? '+' : '-');

                for (int side = 0; side < 2; side++)
                {
                    // Click on center
                    ctx->MouseMove(label.c_str());
                    ctx->MouseDown();
                    IM_CHECK((v.ResultFlags & OUT_OF_RANGE_FLAG) == 0); // Widget value does not fall out of range
                    IM_CHECK((v.ResultFlags & MID_OF_RANGE_FLAG) != 0); // Widget value is between min and max

                    // Drag to left/right edge
                    ctx->MouseMove(label.c_str(), side ? ImGuiTestOpFlags_MoveToEdgeR : ImGuiTestOpFlags_MoveToEdgeL);
                    ctx->MouseUp();

                    // Print values and compare
                    char buf0[32], buf1[32], buf2[32];
                    ImGui::DataTypeFormatString(buf0, 32, data_type, &v.Cur, data_type_info->PrintFmt);
                    ImGui::DataTypeFormatString(buf1, 32, data_type, &v.Min, data_type_info->PrintFmt);
                    ImGui::DataTypeFormatString(buf2, 32, data_type, &v.Max, data_type_info->PrintFmt);
                    ctx->LogInfo("## DataType: %s, Inverted: %d, cur = %s, min = %s, max = %s", data_type_info->Name, invert_range, buf0, buf1, buf2);
                    IM_CHECK((v.ResultFlags & OUT_OF_RANGE_FLAG) == 0); // Widget value does not fall out of range
                    IM_CHECK((v.ResultFlags & MID_OF_RANGE_FLAG) == 0); // Widget value is not between min and max
                    if (side == 0)
                        IM_CHECK(ImGui::DataTypeCompare(data_type, &v.Cur, &v_left) == 0);
                    else
                        IM_CHECK(ImGui::DataTypeCompare(data_type, &v.Cur, &v_right) == 0);
                }
            }
        }

        // Test case where slider knob position calculation would snap to either end of the widget in some cases.
        // Breaks linking with GCC Release mode as those functions in imgui_widgets.cpp gets inlined.
#if 0
        const float s8_ratio_half = ImGui::ScaleRatioFromValueT<ImS32, ImS32, float>(ImGuiDataType_S8, 0, INT8_MIN, INT8_MAX, false, false, false);
        const float u8_ratio_half = ImGui::ScaleRatioFromValueT<ImU32, ImS32, float>(ImGuiDataType_U8, UINT8_MAX / 2, 0, UINT8_MAX, false, false, false);
        const float s8_ratio_half_inv = ImGui::ScaleRatioFromValueT<ImS32, ImS32, float>(ImGuiDataType_S8, 0, INT8_MAX, INT8_MIN, false, false, false);
        const float u8_ratio_half_inv = ImGui::ScaleRatioFromValueT<ImU32, ImS32, float>(ImGuiDataType_U8, UINT8_MAX / 2, UINT8_MAX, 0, false, false, false);
        IM_CHECK(ImAbs(s8_ratio_half - 0.5f) < 0.01f);
        IM_CHECK(ImAbs(u8_ratio_half - 0.5f) < 0.01f);
        IM_CHECK(ImAbs(s8_ratio_half_inv - 0.5f) < 0.01f);
        IM_CHECK(ImAbs(u8_ratio_half_inv - 0.5f) < 0.01f);
#endif
    };

    // ## Test logarithmic slider
    t = IM_REGISTER_TEST(e, "widgets", "widgets_slider_logarithmic");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetNextItemWidth(400);
        ImGui::SliderFloat("slider", &vars.Float1, -10.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::Text("%.4f", vars.Float1);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->ItemClick("slider", 0);
        ctx->Yield();
        IM_CHECK_EQ(vars.Float1, 0.f);

        ctx->ItemClick("slider", 0, ImGuiTestOpFlags_MoveToEdgeR);
        ctx->Yield();
        IM_CHECK_EQ(vars.Float1, 10.f);

        ctx->ItemClick("slider", 0, ImGuiTestOpFlags_MoveToEdgeL);
        ctx->Yield();
        IM_CHECK_EQ(vars.Float1, -10.f);

        // Drag a bit
        ctx->ItemClick("slider", 0);
        float x_offset[] = {50.f,   100.f,  150.f,  190.f};
        float slider_v[] = {0.06f,  0.35f,  2.11f,  8.97f};
        for (float sign : {-1.f, 1.f})
            for (int i = 0; i < IM_ARRAYSIZE(x_offset); i++)
            {
                ctx->ItemDragWithDelta("slider", ImVec2(sign * x_offset[i], 0.f));
                IM_CHECK_GT(vars.Float1, sign * slider_v[i] - (slider_v[i] * 0.15f));  // FIXME-TESTS: Exact values actually depends on GrabSize.
                IM_CHECK_LT(vars.Float1, sign * slider_v[i] + (slider_v[i] * 0.15f));
            }
    };

    // ## Test slider navigation path using keyboard.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_slider_nav");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SliderFloat("Slider 1", &vars.Float1, -100.0f, 100.0f, "%.2f");
        ImGui::SliderInt("Slider 2", &vars.Int1, 0, 100);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ImGui::GetCurrentContext();
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ctx->SetRef("Test Window");

        ctx->ItemNavActivate("Slider 1");
        IM_CHECK_EQ(vars.Float1, 0.f);
        IM_CHECK_EQ(vars.Int1, 0);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Slider 1"));

        // FIXME-TESTS: the step/slow value are technically not exposed and bounds to change. Perhaps this test should only compare the respective magnitude of changes.
        const float slider_float_step = 2.0f;
        const float slider_float_step_slow = 0.2f;
        float slider_float_value = vars.Float1;
        ctx->KeyPress(ImGuiKey_LeftArrow);
        IM_CHECK_EQ(vars.Float1, slider_float_value - slider_float_step);
        ctx->KeyDown(ImGuiMod_Ctrl);
        ctx->KeyPress(ImGuiKey_RightArrow);
        ctx->KeyUp(ImGuiMod_Ctrl);
        IM_CHECK_EQ(vars.Float1, slider_float_value - slider_float_step + slider_float_step_slow);

        ctx->KeyPress(ImGuiKey_DownArrow);
        ctx->KeyPress(ImGuiKey_Space);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Slider 2"));;

        const int slider_int_step = 1;
        const int slider_int_step_slow = 1;
        int slider_int_value = vars.Int1;
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(vars.Int1, slider_float_value + slider_int_step);
        ctx->KeyDown(ImGuiMod_Ctrl);
        ctx->KeyPress(ImGuiKey_LeftArrow);
        ctx->KeyUp(ImGuiMod_Ctrl);
        IM_CHECK_EQ(vars.Int1, slider_int_value + slider_int_step - slider_int_step_slow);

        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Slider 1"));
    };

    // ## Test whether numbers after format specifier do not influence widget value.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_slider_format");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SliderInt("Int", &vars.Int1, 10, 100, "22%d00");
        ImGui::SliderInt("Int2", &vars.Int2, 10, 100, "22%'d"); // May not display correct with all printf implementation, but our test stands
        ImGui::SliderFloat("Float", &vars.Float1, 10.0f, 100.0f, "22%.0f00");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->ItemClick("Int", 0, ImGuiTestOpFlags_MoveToEdgeR);
        IM_CHECK_EQ(vars.Int1, 100);
        ctx->ItemClick("Int2", 0, ImGuiTestOpFlags_MoveToEdgeR);
        IM_CHECK_EQ(vars.Int2, 100);
        ctx->ItemClick("Float", 0, ImGuiTestOpFlags_MoveToEdgeR);
        IM_CHECK_EQ(vars.Float1, 100.0f);
    };

    // ## Test tooltip positioning in various conditions.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_popup_positioning");
    struct TooltipPosVars { ImVec2 Size = ImVec2(50, 50); };
    t->SetVarsDataType<TooltipPosVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TooltipPosVars& vars = ctx->GetVars<TooltipPosVars>();

        auto popup_debug_controls = [&]()
        {
            if (ctx->IsGuiFuncOnly())
            {
                float step = ctx->UiContext->IO.DeltaTime * 500.0f;
                if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) vars.Size.y -= step;
                if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) vars.Size.y += step;
                if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) vars.Size.x -= step;
                if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) vars.Size.x += step;
            }
        };

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav);
        if (ctx->IsGuiFuncOnly())
            ImGui::DragFloat2("Tooltip Size", &vars.Size.x);
        ImGui::Button("Tooltip", ImVec2(100, 0));
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::InvisibleButton("Space", vars.Size);
            popup_debug_controls();
            ImGui::EndTooltip();
        }
        if (ImGui::Button("Popup", ImVec2(100, 0)))
            ImGui::OpenPopup("Popup");
        if (ImGui::BeginPopup("Popup"))
        {
            ImGui::InvisibleButton("Space", vars.Size);
            popup_debug_controls();
            ImGui::EndPopup();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        TooltipPosVars& vars = ctx->GetVars<TooltipPosVars>();
        ImVec2 viewport_pos = ctx->GetMainMonitorWorkPos();
        ImVec2 viewport_size = ctx->GetMainMonitorWorkSize();

        struct TestCase
        {
            ImVec2 Pos;             // Window position
            ImVec2 Pivot;           // Window position pivot
            ImGuiDir DirSmall;      // Expected default tooltip location
            ImGuiDir DirBigH;       // Expected location when tooltip is as wide as viewport
            ImGuiDir DirBigV;       // Expected location when tooltip is as high as viewport
        };

        // Test tooltip positioning around viewport corners
        static TestCase test_cases[] =
        {
            // [0] Top-left corner
            {
                viewport_pos,
                ImVec2(0.0f, 0.0f),
                ImGuiDir_Right,
                ImGuiDir_Down,
                ImGuiDir_Right,
            },
            // [1] Top edge
            {
                viewport_pos + ImVec2(viewport_size.x * 0.5f, 0.0f),
                ImVec2(0.5f, 0.0f),
                ImGuiDir_Right,
                ImGuiDir_Down,
                ImGuiDir_Right,
            },
            // [2] Top-right corner
            {
                viewport_pos + ImVec2(viewport_size.x, 0.0f),
                ImVec2(1.0f, 0.0f),
                ImGuiDir_Down,
                ImGuiDir_Down,
                ImGuiDir_Left,
            },
            // [3] Right edge
            {
                viewport_pos + ImVec2(viewport_size.x, viewport_size.y * 0.5f),
                ImVec2(1.0f, 0.5f),
                ImGuiDir_Down,
                ImGuiDir_Down,
                ImGuiDir_Left,
            },
            // [4] Bottom-right corner
            {
                viewport_pos + viewport_size,
                ImVec2(1.0f, 1.0f),
                ImGuiDir_Up,
                ImGuiDir_Up,
                ImGuiDir_Left,
            },
            // [5] Bottom edge
            {
                viewport_pos + ImVec2(viewport_size.x * 0.5f, viewport_size.y),
                ImVec2(0.5f, 1.0f),
                ImGuiDir_Right,
                ImGuiDir_Up,
                ImGuiDir_Right,
            },
            // [6] Bottom-left corner
            {
                viewport_pos + ImVec2(0.0f, viewport_size.y),
                ImVec2(0.0f, 1.0f),
                ImGuiDir_Right,
                ImGuiDir_Up,
                ImGuiDir_Right,
            },
            // [7] Left edge
            {
                viewport_pos + ImVec2(0.0f, viewport_size.y * 0.5f),
                ImVec2(0.0f, 0.5f),
                ImGuiDir_Right,
                ImGuiDir_Down,
                ImGuiDir_Right,
            },
        };

        ctx->SetRef("Test Window");

        for (int variant = 0; variant < 2; variant++)
        {
            const char* button_name = variant ? "Popup" : "Tooltip";
            ctx->LogInfo("## Test variant: %s", button_name);
            ctx->ItemClick(button_name);        // Force tooltip creation so we can grab the pointer
            ImGuiWindow* tooltip = variant ? g.NavWindow : ctx->GetWindowByRef("//##Tooltip_00");
            IM_CHECK(tooltip != NULL);

            for (auto& test_case : test_cases)
            {
                ctx->LogInfo("## Test case %d", (int)(&test_case - test_cases));
                vars.Size = ImVec2(50, 50);
                ctx->WindowMove("", test_case.Pos, test_case.Pivot);
                ctx->ItemClick(button_name);

                // Check default tooltip location
                IM_CHECK_EQ(g.HoveredIdPreviousFrame, ctx->GetID(button_name));
                IM_CHECK_EQ(tooltip->AutoPosLastDirection, test_case.DirSmall);

                // Check tooltip location when it is real wide and verify that location does not change once it becomes too wide
                // First iteration: tooltip is just wide enough to fit within viewport
                // First iteration: tooltip is wider than viewport
                for (int j = 0; j < 2; j++)
                {
                    vars.Size = ImVec2((j * 0.25f * viewport_size.x) + (viewport_size.x - (g.Style.WindowPadding.x + g.Style.DisplaySafeAreaPadding.x) * 2), 50);
                    ctx->ItemClick(button_name);
                    IM_CHECK(tooltip->AutoPosLastDirection == test_case.DirBigH);
                }

                // Check tooltip location when it is real tall and verify that location does not change once it becomes too tall
                // First iteration: tooltip is just tall enough to fit within viewport
                // First iteration: tooltip is taller than viewport
                for (int j = 0; j < 2; j++)
                {
                    vars.Size = ImVec2(50, (j * 0.25f * viewport_size.x) + (viewport_size.y - (g.Style.WindowPadding.y + g.Style.DisplaySafeAreaPadding.y) * 2));
                    ctx->ItemClick(button_name);
                    IM_CHECK(tooltip->AutoPosLastDirection == test_case.DirBigV);
                }

                IM_CHECK_EQ(g.HoveredIdPreviousFrame, ctx->GetID(button_name));
            }
        }
    };

    // ## Test disabled items setting g.HoveredId and taking clicks.
#if (IMGUI_VERSION_NUM >= 18310)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_disabled");
    struct WidgetsDisabledVars
    {
        bool WidgetsDisabled;
        bool Activated[4];
        bool Hovered[4];
        bool HoveredDisabled[4];
        void Reset() { memset(this, 0, sizeof(*this)); }
        WidgetsDisabledVars() { Reset(); }
    };
    t->SetVarsDataType<WidgetsDisabledVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        WidgetsDisabledVars& vars = ctx->GetVars<WidgetsDisabledVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Menu 1"))
            {
                if (ImGui::BeginMenu("Menu Enabled"))
                {
                    ImGui::MenuItem("Item");
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Menu Disabled", false))
                {
                    ImGui::MenuItem("Item");
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::Checkbox("WidgetsDisabled", &vars.WidgetsDisabled);
        ImGui::Selectable("Enabled A");
        int index = 0;

        if (vars.WidgetsDisabled)
            ImGui::BeginDisabled();

        vars.Activated[index] |= ImGui::Button("Button");
        vars.Hovered[index] |= ImGui::IsItemHovered();
        vars.HoveredDisabled[index] |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
        if (ctx->FrameCount == 0)
            IM_CHECK_EQ(ImGui::GetItemID(), ImGui::GetID("Button"));      // Make sure widget has an ID
        ImGui::SameLine();
        ImGui::Text("%d/%d", vars.Hovered[index], vars.HoveredDisabled[index]);
        index++;

        float f = 0.0f;
        vars.Activated[index] |= ImGui::DragFloat("DragFloat", &f);
        vars.Hovered[index] |= ImGui::IsItemHovered();
        vars.HoveredDisabled[index] |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
        if (ctx->FrameCount == 0)
            IM_CHECK_EQ(ImGui::GetItemID(), ImGui::GetID("DragFloat"));     // Make sure widget has an ID
        ImGui::SameLine();
        ImGui::Text("%d/%d", vars.Hovered[index], vars.HoveredDisabled[index]);
        index++;

        vars.Activated[index] |= ImGui::Selectable("Selectable", false, 0);
        vars.Hovered[index] |= ImGui::IsItemHovered();
        vars.HoveredDisabled[index] |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
        if (ctx->FrameCount == 0)
            IM_CHECK_EQ(ImGui::GetItemID(), ImGui::GetID("Selectable"));    // Make sure widget has an ID
        ImGui::SameLine();
        ImGui::Text("%d/%d", vars.Hovered[index], vars.HoveredDisabled[index]);
        index++;

        if (vars.WidgetsDisabled)
            ImGui::EndDisabled();

        vars.Activated[index] |= ImGui::Selectable("SelectableFlag", false, vars.WidgetsDisabled ? ImGuiSelectableFlags_Disabled : 0);
        vars.Hovered[index] |= ImGui::IsItemHovered();
        vars.HoveredDisabled[index] |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
        if (ctx->FrameCount == 0)
            IM_CHECK_EQ(ImGui::GetItemID(), ImGui::GetID("SelectableFlag"));    // Make sure widget has an ID
        ImGui::SameLine();
        ImGui::Text("%d/%d", vars.Hovered[index], vars.HoveredDisabled[index]);
        index++;

        ImGui::Selectable("Enabled B");

        // Simple nested check (#4655)
        ImGuiStyle& style = ImGui::GetStyle();
        const float alpha0 = style.Alpha;
        ImGui::BeginDisabled();
        IM_CHECK_EQ(alpha0 * style.DisabledAlpha, style.Alpha);
        ImGui::BeginDisabled();
        IM_CHECK_EQ(alpha0 * style.DisabledAlpha, style.Alpha);
        ImGui::EndDisabled();
        IM_CHECK_EQ(alpha0 * style.DisabledAlpha, style.Alpha);
        ImGui::EndDisabled();
        IM_CHECK_EQ(alpha0, style.Alpha);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        WidgetsDisabledVars& vars = ctx->GetVars<WidgetsDisabledVars>();
        const char* disabled_items[] = { "Button", "DragFloat", "Selectable", "SelectableFlag" }; // Matches order of arrays in WidgetsDisabledVars.

        ImGuiWindow* window = ctx->GetWindowByRef("Test Window");
        ctx->SetRef("Test Window");
        vars.WidgetsDisabled = true;

        // Navigating over disabled menu.
        ctx->MenuAction(ImGuiTestAction_Hover, "Menu 1/Menu Enabled/Item");
        IM_CHECK(g.NavWindow != NULL);
        //IM_CHECK_EQ(ImHashStr(g.NavWindow->Name), ImHashStr("###Menu_01")); // 2023/04/26 Incorrect since FindHoveredWindowAtPos() addition, since we don't necessarily focus sub-menu.
        ctx->MenuAction(ImGuiTestAction_Hover, "Menu 1/Menu Disabled");
        IM_CHECK(g.NavWindow != NULL);
        IM_CHECK_EQ(ImHashStr(g.NavWindow->Name), ImHashStr("###Menu_00"));

        // Navigating over a disabled item.
        ctx->ItemClick("Enabled A");
        IM_CHECK_EQ(g.NavId, ctx->GetID("Enabled A"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Enabled B"));              // Make sure we have skipped disabled items.
        ctx->KeyPress(ImGuiKey_Escape);

        // Clicking a disabled item.
        for (int i = 0; i < IM_ARRAYSIZE(disabled_items); i++)
        {
            ctx->MouseMove(disabled_items[i]);
            vars.Reset();
            vars.WidgetsDisabled = true;
            ctx->ItemClick(disabled_items[i]);
            IM_CHECK(vars.Activated[i] == false);                   // Was not clicked because button is disabled.
            IM_CHECK(vars.Hovered[i] == false);                     // Wont report as being hovered because button is disabled.
            IM_CHECK(vars.HoveredDisabled[i] == true);              // IsItemHovered with ImGuiHoveredFlags_AllowWhenDisabled.
            IM_CHECK(g.HoveredId == ctx->GetID(disabled_items[i])); // Will set HoveredId even when disabled.
        }

        // Verify that clicking on a disabled item takes focus
#if IMGUI_VERSION_NUM >= 19135
        for (int i = 0; i < IM_ARRAYSIZE(disabled_items); i++)
        {
            ctx->WindowFocus("//Dear ImGui Demo");
            ctx->ItemClick(disabled_items[i]);
            IM_CHECK_EQ(g.NavWindow, window);
        }
#endif

        // Dragging a disabled item.
        ImVec2 window_pos = window->Pos;
        for (int i = 0; i < IM_ARRAYSIZE(disabled_items); i++)
        {
            ctx->MouseMove(disabled_items[i]);
            vars.Reset();
            vars.WidgetsDisabled = true;
            ctx->ItemDragWithDelta(disabled_items[i], ImVec2(30, 0));
            IM_CHECK(window_pos == window->Pos);                    // Disabled items consume click events.
        }

        // Disable active ID when widget gets disabled while held.
        for (int i = 0; i < IM_ARRAYSIZE(disabled_items); i++)
        {
            vars.WidgetsDisabled = false;
            ctx->MouseMove(disabled_items[i]);
            ctx->MouseDown();
            IM_CHECK(g.ActiveId == ctx->GetID(disabled_items[i]));  // Enabled item is active.
            vars.WidgetsDisabled = true;
            ctx->Yield();
            IM_CHECK(g.ActiveId == 0);                              // Disabling item while it is active deactivates it.
            ctx->MouseUp();
        }
    };
#endif

    // ## Test BeginDisabled()/EndDisabled()
#if IMGUI_VERSION_NUM >= 18405
    t = IM_REGISTER_TEST(e, "widgets", "widgets_disabled_2");
    struct BeginDisabledVars
    {
        struct BeginDisabledItemInfo
        {
            const char* Name = NULL;
            ImGuiTestGenericItemStatus Status = {};
            ImGuiItemFlags FlagsBegin = 0;
            ImGuiItemFlags FlagsEnd = 0;
            float AlphaBegin = 0;
            float AlphaEnd = 0;

            BeginDisabledItemInfo(const char* name) { Name = name; }
        };
        BeginDisabledItemInfo ButtonInfo[6] = { "A", "B", "C", "D", "E", "F" };
    };
    t->SetVarsDataType<BeginDisabledVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        BeginDisabledVars& vars = ctx->GetVars<BeginDisabledVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);

        int index = 0;
        auto begin_disabled = [&](bool disabled = true)
        {
            auto& button_info = vars.ButtonInfo[index];
            button_info.FlagsBegin = g.CurrentItemFlags;
            button_info.AlphaBegin = g.DisabledAlphaBackup;
            ImGui::BeginDisabled(disabled);
            bool flag_enabled = (index % 2) == 0;
            ImGui::PushItemFlag(ImGuiItemFlags_NoNav, flag_enabled);    // Add a random flag to try mix things up.
            index++;
        };
        auto end_disabled = [&]()
        {
            index--;
            auto& button_info = vars.ButtonInfo[index];
            ImGui::PopItemFlag();                                       // ImGuiItemFlags_NoNav
            ImGui::EndDisabled();
            button_info.FlagsEnd = g.CurrentItemFlags;
            button_info.AlphaEnd = g.DisabledAlphaBackup;
        };

        //if (ImGui::GetIO().KeyShift)
        //    printf("");

        begin_disabled();
        vars.ButtonInfo[0].Status.QueryInc(ImGui::Button("A"));
        ImGui::SameLine(); ImGui::Text("%d,%d", vars.ButtonInfo[0].Status.Hovered, vars.ButtonInfo[0].Status.HoveredAllowDisabled);
        begin_disabled();
        vars.ButtonInfo[1].Status.QueryInc(ImGui::Button("B"));
        ImGui::SameLine(); ImGui::Text("%d,%d", vars.ButtonInfo[1].Status.Hovered, vars.ButtonInfo[1].Status.HoveredAllowDisabled);
        end_disabled();
        begin_disabled(false);
        vars.ButtonInfo[2].Status.QueryInc(ImGui::Button("C"));
        ImGui::SameLine(); ImGui::Text("%d,%d", vars.ButtonInfo[2].Status.Hovered, vars.ButtonInfo[2].Status.HoveredAllowDisabled);
        end_disabled();
        vars.ButtonInfo[3].Status.QueryInc(ImGui::Button("D"));
        ImGui::SameLine(); ImGui::Text("%d,%d", vars.ButtonInfo[3].Status.Hovered, vars.ButtonInfo[3].Status.HoveredAllowDisabled);
        end_disabled();

        begin_disabled();
        bool ret = ImGui::Button("E");
        end_disabled();
        vars.ButtonInfo[4].Status.QueryInc(ret);
        ImGui::SameLine(); ImGui::Text("%d,%d", vars.ButtonInfo[4].Status.Hovered, vars.ButtonInfo[4].Status.HoveredAllowDisabled);

        ImGui::BeginDisabled(false);
        vars.ButtonInfo[5].Status.QueryInc(ImGui::Button("F"));
        ImGui::SameLine(); ImGui::Text("%d,%d", vars.ButtonInfo[5].Status.Hovered, vars.ButtonInfo[5].Status.HoveredAllowDisabled);
        ImGui::EndDisabled();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        BeginDisabledVars& vars = ctx->GetVars<BeginDisabledVars>();
        ctx->SetRef("Test Window");
        for (int i = 0; i < 5; i++)
        {
            auto& button_info = vars.ButtonInfo[i];
            ctx->LogDebug("Button %s", button_info.Name);
            button_info.Status.Clear();
            ctx->ItemClick(button_info.Name);
            IM_CHECK(button_info.Status.RetValue == 0);                 // No clicks
            IM_CHECK(button_info.Status.Clicked == 0);
            IM_CHECK_GT(button_info.Status.HoveredAllowDisabled, 0);
            IM_CHECK(g.HoveredId == ctx->GetID(button_info.Name));      // HoveredId is set
            IM_CHECK(button_info.FlagsBegin == button_info.FlagsEnd);   // Flags and Alpha match between Begin/End calls
            IM_CHECK(button_info.AlphaBegin == button_info.AlphaEnd);
            ctx->MouseDown(0);
            for (int all_i = 0; all_i < 5; all_i++)
                vars.ButtonInfo[all_i].Status.Clear();
            ctx->Yield();
            IM_CHECK_EQ(button_info.Status.Hovered, 0);
            IM_CHECK_EQ(button_info.Status.HoveredAllowDisabled, 1);

#if IMGUI_VERSION_NUM >= 19222
            // Check that other disabled items do NOT report as Disabled
            for (int all_i = 0; all_i < 5; all_i++)
                if (all_i != i)
                {
                    IM_CHECK_EQ(vars.ButtonInfo[all_i].Status.Hovered, 0);
                    IM_CHECK_EQ(vars.ButtonInfo[all_i].Status.HoveredAllowDisabled, 0);
                }
#endif
            ctx->MouseUp();
        }
        ctx->ItemClick("E");
        IM_CHECK(vars.ButtonInfo[4].Status.Hovered == 0);               // Ensure we rely on last item storage, not current state
        ctx->ItemClick("F");
        IM_CHECK(vars.ButtonInfo[5].Status.RetValue == 1);              // BeginDisabled(false) does not prevent clicks
        IM_CHECK(vars.ButtonInfo[5].Status.Clicked == 1);
    };
#endif

#if IMGUI_VERSION_NUM >= 19073
    // ## Test nested functions and Alpha values
    t = IM_REGISTER_TEST(e, "widgets", "widgets_disabled_nested");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *GImGui;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.4f);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGui::BeginDisabled();
        IM_CHECK((g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0);
        IM_CHECK_FLOAT_EQ(ImGui::GetStyle().Alpha, 1.0f * 0.4f);

        ImGui::Begin("Nested window call");
#if IMGUI_VERSION_NUM >= 19085
        IM_CHECK((g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0);
        IM_CHECK_FLOAT_NE(ImGui::GetStyle().Alpha, 1.0f);
#else
        IM_CHECK((g.CurrentItemFlags & ImGuiItemFlags_Disabled) == 0);
        IM_CHECK_FLOAT_EQ(ImGui::GetStyle().Alpha, 1.0f);
#endif

        ImGui::End();
        IM_CHECK((g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0);
        IM_CHECK_FLOAT_EQ(ImGui::GetStyle().Alpha, 1.0f * 0.4f);

        ImGui::BeginChild("Nested Child", ImVec2(200, 200));
        IM_CHECK((g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0);
        IM_CHECK_FLOAT_EQ(ImGui::GetStyle().Alpha, 1.0f * 0.4f);

        ImGui::EndChild();
        IM_CHECK((g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0);
        IM_CHECK_FLOAT_EQ(ImGui::GetStyle().Alpha, 1.0f * 0.4f);

        ImGui::EndDisabled();
        IM_CHECK((g.CurrentItemFlags & ImGuiItemFlags_Disabled) == 0);
        IM_CHECK_FLOAT_EQ(ImGui::GetStyle().Alpha, 1.0f);

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
    };
#endif

#if IMGUI_VERSION_NUM >= 19084
    // ## Test nested functions and Alpha values
    t = IM_REGISTER_TEST(e, "widgets", "widgets_disabled_shortcuts");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGui::BeginDisabled();

        // Case 1: SetNextItemShortcut() within disabled block
        ImGui::Text("Ctrl+S");
        ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_S);
        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            vars.Count++;

        // Case 2: Shortcut() within disabled block
        ImGui::Text("Ctrl+T");
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_T))
            vars.Count++;
        ImGui::EndDisabled();

        // Case 3: SetNextItemShortcut() NOT within disabled block but with disabled item
        // this will generally be handled by the 'if (g.LastItemData.ItemFlags & ImGuiItemFlags_Disabled)' check in ItemHandleShortcut()
        ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_U);
        if (ImGui::Selectable("Ctrl+U", false, ImGuiSelectableFlags_Disabled))
            vars.Count++;

        ImGui::Text("Counter = %d", vars.Count);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->WindowFocus("");
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_S);
        IM_CHECK(vars.Count == 0);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_T);
        IM_CHECK(vars.Count == 0);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_U);
        IM_CHECK(vars.Count == 0);
    };
#endif

    // ## Test SetItemUsingMouseWheel() preventing scrolling.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_item_using_mouse_wheel");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(0, 100), ImGuiCond_Always);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Button("You! Shall! Not! Scroll!");
#if IMGUI_VERSION_NUM >= 18837
        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
#else
        ImGui::SetItemUsingMouseWheel();
#endif
        for (int i = 0; i < 10; i++)
            ImGui::Text("Line %d", i + 1);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ctx->ScrollToTop("");
        ImGuiWindow* window = ctx->GetWindowByRef("");
        IM_CHECK(window->Scroll.y == 0.0f);
        ctx->MouseMove("You! Shall! Not! Scroll!");
        ctx->MouseWheelY(-10.0f);
        IM_CHECK(window->Scroll.y == 0.0f);
    };

    // ## Test Splitter().
    t = IM_REGISTER_TEST(e, "widgets", "widgets_splitter");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImVec2& child_size = vars.WindowSize;
        ImGuiAxis axis = (ImGuiAxis)ctx->Test->ArgVariant;

        ImGui::Splitter("splitter", &child_size.x, &child_size.y, axis, +1);

        if (ImGui::BeginChild("Child 1", ImVec2(axis == ImGuiAxis_X ? child_size.x : 0.0f, axis == ImGuiAxis_Y ? child_size.x : 0.0f)))
            ImGui::TextUnformatted("Child 1");
        ImGui::EndChild();

        if (ImGui::BeginChild("Child 2", ImVec2(axis == ImGuiAxis_X ? child_size.y : 0.0f, axis == ImGuiAxis_Y ? child_size.y : 0.0f)))
            ImGui::TextUnformatted("Child 1");
        ImGui::EndChild();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ImGuiWindow* child1 = ctx->WindowInfo("Child 1").Window;
        ImGuiWindow* child2 = ctx->WindowInfo("Child 2").Window;
        IM_CHECK(child1 && child2);
        ImVec2& child_size = vars.WindowSize;
        for (int axis = 0; axis < 2; axis++)
        {
            ctx->LogDebug("Axis: ImGuiAxis_%s", axis ? "Y" : "X");
            ctx->Test->ArgVariant = axis;
            child_size = ImVec2(100, 100);
            for (int i = -1; i < 1; i++)
            {
                ctx->ItemDragWithDelta("splitter", ImVec2(50.0f * i, 50.0f * i));
                IM_CHECK_EQ(axis == ImGuiAxis_X ? child1->Size.x + child2->Size.x : child1->Size.y + child2->Size.y, child_size.x + child_size.y);
            }
        }
    };

#if !defined(IMGUI_DISABLE_OBSOLETE_FUNCTIONS) && (IMGUI_VERSION_NUM < 18830)
    // ## Test legacy float format string patching in DragInt().
    t = IM_REGISTER_TEST(e, "widgets", "widgets_dragint_float_format");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        if (ctx->IsFirstGuiFrame())
            vars.Int1 = 123;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::LogToBuffer();
        ImGui::DragInt("%f", &vars.Int1, 1, 0, 0, "%f");
        ImGui::DragInt("%.0f", &vars.Int1, 1, 0, 0, "%.0f");
        ImGui::DragInt("%.3f", &vars.Int1, 1, 0, 0, "%.3f");
        ImStrncpy(vars.Str1, ctx->UiContext->LogBuffer.c_str(), IM_ARRAYSIZE(vars.Str2));
        ImGui::LogFinish();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        IM_CHECK_STR_EQ(vars.Str1, "{ 123 } %f" IM_NEWLINE "{ 123 } %.0f" IM_NEWLINE "{ 123 } %.3f");
    };
#endif
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#if __clang__
#pragma clang diagnostic pop
#endif
