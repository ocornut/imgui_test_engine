// dear imgui
// (tests)

#define _CRT_SECURE_NO_WARNINGS
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_tests.h"
#include "imgui_te_core.h"
#include "imgui_te_util.h"

//-------------------------------------------------------------------------
// NOTES (also see TODO in imgui_te_core.cpp)
//-------------------------------------------------------------------------
// - Tests can't reliably once ImGuiCond_Once or ImGuiCond_FirstUseEver
// - GuiFunc can't run code that yields. There is an assert for that.
//-------------------------------------------------------------------------

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4100) // unreferenced formal parameter
#pragma warning (disable: 4127) // conditional expression is constant
#endif

// Helper Operators
static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
static inline bool operator!=(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x != rhs.x && lhs.y != rhs.y; }

#define REGISTER_TEST(_CATEGORY, _NAME)    ImGuiTestEngine_RegisterTest(e, _CATEGORY, _NAME, __FILE__, __LINE__);

//-------------------------------------------------------------------------
// Tests: Window
//-------------------------------------------------------------------------

void RegisterTests_Window(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test size of an empty window
    t = REGISTER_TEST("window", "empty");
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
        IM_CHECK(window->Size.x == ImMax(style.WindowMinSize.x, style.WindowPadding.x * 2.0f));
        IM_CHECK(window->Size.y == ImGui::GetFontSize() + style.FramePadding.y * 2.0f + style.WindowPadding.y * 2.0f);
        IM_CHECK(window->Size.y == window->SizeContents.y);
        IM_CHECK(window->Scroll.x == 0.0f && window->Scroll.y == 0.0f);
    };

    // ## Test that a window starting collapsed performs width/contents size measurement on its first few frames.
    t = REGISTER_TEST("window", "window_size_collapsed_1");
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
            IM_CHECK(ImFabs(w - expected_w) < 1.0f);
        }
        ImGui::End();
    };

    // ## Test that non-integer size/position passed to window gets rounded down and not cause any drift.
    t = REGISTER_TEST("window", "window_size_unrounded");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // #2067
        {
            ImGui::SetNextWindowPos(ImVec2(901.0f, 103.0f), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(348.48f, 400.0f), ImGuiCond_Once);
            ImGui::Begin("Issue 2067", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();
            //ctx->Log("%f %f, %f %f\n", pos.x, pos.y, size.x, size.y);
            IM_CHECK_NO_RET(pos.x == 901.0f && pos.y == 103.0f);
            IM_CHECK_NO_RET(size.x == 348.0f && size.y == 400.0f);
            ImGui::End();
        }
        // Test that non-rounded size constraint are not altering pos/size (#2530)
        {
            ImGui::SetNextWindowPos(ImVec2(901.0f, 103.0f), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(348.48f, 400.0f), ImGuiCond_Once);
            ImGui::SetNextWindowSizeConstraints(ImVec2(475.200012f, 0.0f), ImVec2(475.200012f, 100.4f));
            ImGui::Begin("Issue 2530", NULL, ImGuiWindowFlags_NoSavedSettings);
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();
            //ctx->Log("%f %f, %f %f\n", pos.x, pos.y, size.x, size.y);
            IM_CHECK(pos.x == 901.0f && pos.y == 103.0f);
            IM_CHECK(size.x == 475.0f && size.y == 100.0f);
            ImGui::End();
        }
        if (ctx->FrameCount == 2)
            ctx->Finish();
    };

    // ## Test basic window auto resize
    t = REGISTER_TEST("window", "window_auto_resize_basic");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        // FIXME-TESTS: Ideally we'd like a variant with/without the if (Begin) here
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Hello World");
        ImGui::BeginChild("Child", ImVec2(0, 200));
        ImGui::EndChild();
        ImVec2 sz = ImGui::GetWindowSize();
        ImGui::End();
        if (ctx->FrameCount >= 0 && ctx->FrameCount <= 2)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            IM_CHECK((int)sz.x == (int)(ImGui::CalcTextSize("Hello World").x + style.WindowPadding.x * 2.0f));
            IM_CHECK((int)sz.y == (int)(ImGui::GetFrameHeight() + ImGui::CalcTextSize("Hello World").y + style.ItemSpacing.y + 200.0f + style.WindowPadding.y * 2.0f));
        }
    };

    // ## Test that uncollapsing an auto-resizing window does not go through a frame where the window is smaller than expected
    t = REGISTER_TEST("window", "window_auto_resize_uncollapse");
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
        ctx->SetRef("Test Window");
        ctx->WindowSetCollapsed("", false);
        ctx->LogVerbose("Size %f %f, SizeFull %f %f\n", window->Size.x, window->Size.y, window->SizeFull.x, window->SizeFull.y);
        IM_CHECK(window->Size == window->SizeFull);
        ImVec2 size_full_when_uncollapsed = window->SizeFull;
        ctx->WindowSetCollapsed("", true);
        ctx->LogVerbose("Size %f %f, SizeFull %f %f\n", window->Size.x, window->Size.y, window->SizeFull.x, window->SizeFull.y);
        ImVec2 size_collapsed = window->Size;
        IM_CHECK(size_full_when_uncollapsed.y > size_collapsed.y);
        IM_CHECK(size_full_when_uncollapsed == window->SizeFull);
        ctx->WindowSetCollapsed("", false);
        ctx->LogVerbose("Size %f %f, SizeFull %f %f\n", window->Size.x, window->Size.y, window->SizeFull.x, window->SizeFull.y);
        IM_CHECK(window->Size.y == size_full_when_uncollapsed.y && "Window should have restored to full size.");
        ctx->Yield();
        IM_CHECK(window->Size.y == size_full_when_uncollapsed.y);
    };

    // ## Test appending multiple times to a child window (bug #2282)
    t = REGISTER_TEST("window", "window_append");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Line 1");
        ImGui::End();
        ImGui::Begin("Test Window");
        ImGui::Text("Line 2");
        ImGui::BeginChild("Blah", ImVec2(0,50), true);
        ImGui::Text("Line 3");
        ImGui::EndChild();
        ImVec2 pos1 = ImGui::GetCursorScreenPos();
        ImGui::BeginChild("Blah");
        ImGui::Text("Line 4");
        ImGui::EndChild();
        ImVec2 pos2 = ImGui::GetCursorScreenPos();
        IM_CHECK(pos1 == pos2); // Append calls to BeginChild() shouldn't affect CursorPos in parent window
        ImGui::Text("Line 5");
        ImVec2 pos3 = ImGui::GetCursorScreenPos();
        ImGui::BeginChild("Blah");
        ImGui::Text("Line 6");
        ImGui::EndChild();
        ImVec2 pos4 = ImGui::GetCursorScreenPos();
        IM_CHECK(pos3 == pos4); // Append calls to BeginChild() shouldn't affect CursorPos in parent window
        ImGui::End();
    };

    // ## Test basic focus behavior
    // FIXME-TESTS: This in particular when combined with Docking should be tested with and without ConfigDockingTabBarOnSingleWindows
    t = REGISTER_TEST("window", "window_focus_1");
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
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/BBBB"));
        ctx->YieldUntil(19);
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/BBBB"));
        ctx->YieldUntil(20);
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/DDDD"));
        ctx->YieldUntil(30);
        ctx->FocusWindow("/CCCC");
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/CCCC"));
        ctx->YieldUntil(39);
        ctx->YieldUntil(40);
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/CCCC"));

        // When docked, it should NOT takes 1 extra frame to lose focus (fixed 2019/03/28)
        ctx->YieldUntil(41);
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/BBBB"));

        ctx->YieldUntil(49);
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/BBBB"));
        ctx->YieldUntil(50);
        IM_CHECK(g.NavWindow->ID == ctx->GetID("/DDDD"));
    };
    
    // ## Test popup focus and right-click to close popups up to a given level
    t = REGISTER_TEST("window", "window_focus_popup");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("ImGui Demo");
        ctx->ItemOpen("Popups & Modal windows");
        ctx->ItemOpen("Popups");
        ctx->ItemClick("Popups/Toggle..");

        ImGuiWindow* popup_1 = g.NavWindow;
        ctx->SetRef(popup_1->Name);
        ctx->ItemClick("Stacked Popup");
        IM_CHECK(popup_1->WasActive);

        ImGuiWindow* popup_2 = g.NavWindow;
        ctx->MouseMove("Bream", ImGuiTestOpFlags_NoFocusWindow | ImGuiTestOpFlags_NoCheckHoveredId);
        ctx->MouseClick(1); // Close with right-click
        IM_CHECK(popup_1->WasActive);
        IM_CHECK(!popup_2->WasActive);
        IM_CHECK(g.NavWindow == popup_1);
    };
}

//-------------------------------------------------------------------------
// Tests: Button
//-------------------------------------------------------------------------

void RegisterTests_Button(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test basic button presses
    t = REGISTER_TEST("button", "button_press");
    struct ButtonPressTestVars { int ButtonPressCount[4] = { 0 }; };
    t->SetUserDataType<ButtonPressTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ButtonPressTestVars& vars = ctx->GetUserData<ButtonPressTestVars>();

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Button0"))
            vars.ButtonPressCount[0]++;
        if (ImGui::ButtonEx("Button1", ImVec2(0, 0), ImGuiButtonFlags_PressedOnDoubleClick))
            vars.ButtonPressCount[1]++;
        if (ImGui::ButtonEx("Button2", ImVec2(0, 0), ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick))
            vars.ButtonPressCount[2]++;
        if (ImGui::ButtonEx("Button3", ImVec2(0, 0), ImGuiButtonFlags_Repeat))
            vars.ButtonPressCount[3]++;
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ButtonPressTestVars& vars = ctx->GetUserData<ButtonPressTestVars>();

        ctx->SetRef("Test Window");
        ctx->ItemClick("Button0");
        IM_CHECK(vars.ButtonPressCount[0] == 1);
        ctx->ItemDoubleClick("Button1");
        IM_CHECK(vars.ButtonPressCount[1] == 1);
        ctx->ItemDoubleClick("Button2");
        IM_CHECK(vars.ButtonPressCount[2] == 2);

        ctx->ItemClick("Button3");
        IM_CHECK(vars.ButtonPressCount[3] == 1);
        ctx->MouseDown(0);
        IM_CHECK(vars.ButtonPressCount[3] == 1);
        ctx->SleepDebugNoSkip(ctx->UiContext->IO.KeyRepeatDelay + ctx->UiContext->IO.KeyRepeatRate * 3); // FIXME-TESTS: Can we elapse context time without elapsing wall clock time?
        IM_CHECK(vars.ButtonPressCount[3] == 1 + 3 * 2); // FIXME: MouseRepeatRate is double KeyRepeatRate, that's not documented
        ctx->MouseUp(0);
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

    t = REGISTER_TEST("button", "button_states");
    struct ButtonStateTestVars { ButtonStateMachineTestStep NextStep; ImGuiTestGenericStatus Status; };
    t->SetUserDataType<ButtonStateTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ButtonStateTestVars& vars = ctx->GetUserData<ButtonStateTestVars>();
        ImGuiTestGenericStatus& status = vars.Status;

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

        // The "Dummy" button allows to move the mouse away from the "Test" button
        ImGui::Button("Dummy");
        
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ButtonStateTestVars& vars = ctx->GetUserData<ButtonStateTestVars>();
        vars.NextStep = ButtonStateMachineTestStep_None;

        ctx->SetRef("Test Window");

        // Move mouse away from "Test" button
        ctx->MouseMove("Dummy");
        vars.NextStep = ButtonStateMachineTestStep_Init;
        ctx->Yield();

        ctx->MouseMove("Test");
        vars.NextStep = ButtonStateMachineTestStep_MovedOver;
        ctx->Yield();

        vars.NextStep = ButtonStateMachineTestStep_MouseDown;
        ctx->MouseDown();

        ctx->MouseMove("Dummy", ImGuiTestOpFlags_NoCheckHoveredId);
        vars.NextStep = ButtonStateMachineTestStep_MovedAway;
        ctx->Yield();

        ctx->MouseMove("Test");
        vars.NextStep = ButtonStateMachineTestStep_MovedOverAgain;
        ctx->Yield();

        vars.NextStep = ButtonStateMachineTestStep_MouseUp;
        ctx->MouseUp();

        ctx->MouseMove("Dummy");
        vars.NextStep = ButtonStateMachineTestStep_Done;
        ctx->Yield();
    };
}

//-------------------------------------------------------------------------
// Tests: Scrolling
//-------------------------------------------------------------------------

void RegisterTests_Scrolling(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test that basic SetScrollHereY call scrolls all the way (#1804)
    t = REGISTER_TEST("scrolling", "scrolling_001");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Scrolling", NULL, ImGuiWindowFlags_NoSavedSettings);
        for (int n = 0; n < 100; n++)
            ImGui::Text("Hello %d\n", n);
        ImGui::SetScrollHereY(0.0f);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Scrolling");
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        IM_ASSERT(window->SizeContents.y > 0.0f);
        float scroll_y = ImGui::GetScrollY();
        IM_CHECK(scroll_y > 0.0f);
        IM_CHECK(scroll_y == ImGui::GetScrollMaxY());
        ctx->LogVerbose("scroll_y = %f\n", scroll_y);
        ImGui::End();
    };
}

//-------------------------------------------------------------------------
// Tests: Widgets
//-------------------------------------------------------------------------

void RegisterTests_Widgets(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test checkbox click
    t = REGISTER_TEST("widgets", "widgets_checkbox_001");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window1");
        ImGui::Checkbox("Checkbox", &ctx->GenericVars.Bool1);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        IM_CHECK(ctx->GenericVars.Bool1 == false);
        ctx->ItemClick("Window1/Checkbox");
        IM_CHECK(ctx->GenericVars.Bool1 == true);
    };

    // FIXME-TESTS: WIP
    t = REGISTER_TEST("widgets", "widgets_datatype_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        char buf[3] = { 42, 100, 42 };
        ImGui::DragScalar("Drag", ImGuiDataType_S8, &buf[1], 0.5f, NULL, NULL);
        IM_ASSERT(buf[0] == 42 && buf[2] == 42);
        ImGui::End();
    };

    // ## Test InputText widget
    t = REGISTER_TEST("widgets", "widgets_inputtext_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        char* buf = ctx->GenericVars.Str1;

        ctx->SetRef("Test Window");

        // Insert
        strcpy(buf, "Hello");
        ctx->ItemClick("InputText");
        ctx->KeyCharsAppendEnter("World123");
        IM_CHECK(strcmp(buf, "HelloWorld123") == 0);

        // Delete
        ctx->ItemClick("InputText");
        ctx->KeyPressMap(ImGuiKey_End);
        ctx->KeyPressMap(ImGuiKey_Backspace, ImGuiKeyModFlags_None, 3);
        ctx->KeyPressMap(ImGuiKey_Enter);
        IM_CHECK(strcmp(buf, "HelloWorld") == 0);

        // Insert, Cancel
        ctx->ItemClick("InputText");
        ctx->KeyPressMap(ImGuiKey_End);
        ctx->KeyChars("XXXXX");
        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK(strcmp(buf, "HelloWorld") == 0);

        // Delete, Cancel
        ctx->ItemClick("InputText");
        ctx->KeyPressMap(ImGuiKey_End);
        ctx->KeyPressMap(ImGuiKey_Backspace, ImGuiKeyModFlags_None, 5);
        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK(strcmp(buf, "HelloWorld") == 0);
    };

    // ## Test InputText undo/redo ops, in particular related to issue we had with stb_textedit undo/redo buffers
    t = REGISTER_TEST("widgets", "widgets_inputtext_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        if (vars.StrLarge.empty())
            vars.StrLarge.resize(10000, 0);
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 50, 0.0f));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("strlen() = %d", strlen(vars.StrLarge.Data));
        ImGui::InputText("Dummy", "", 0, ImGuiInputTextFlags_None);
        ImGui::InputTextMultiline("InputText", vars.StrLarge.Data, vars.StrLarge.Size, ImVec2(-1, ImGui::GetFontSize() * 20), ImGuiInputTextFlags_None);
        ImGui::End();
        //ImDebugShowInputTextState();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // https://github.com/nothings/stb/issues/321
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        // Start with a 350 characters buffer.
        // For this test we don't inject the characters via pasting or key-by-key in order to precisely control the undo/redo state.
        char* buf = vars.StrLarge.Data;
        IM_CHECK(strlen(buf) == 0);
        for (int n = 0; n < 10; n++)
            strcat(buf, "xxxxxxx abcdefghijklmnopqrstuvwxyz\n");
        IM_CHECK(strlen(buf) == 350);

        ctx->SetRef("Test Window");
        ctx->ItemClick("Dummy"); // This is to ensure stb_textedit_clear_state() gets called (clear the undo buffer, etc.)
        ctx->ItemClick("InputText");

        ImGuiInputTextState& input_text_state = GImGui->InputTextState;
        ImStb::StbUndoState& undo_state = input_text_state.Stb.undostate;
        IM_CHECK(input_text_state.ID == GImGui->ActiveId);
        IM_CHECK(undo_state.undo_point == 0);
        IM_CHECK(undo_state.undo_char_point == 0);
        IM_CHECK(undo_state.redo_point == STB_TEXTEDIT_UNDOSTATECOUNT);
        IM_CHECK(undo_state.redo_char_point == STB_TEXTEDIT_UNDOCHARCOUNT);
        IM_CHECK(STB_TEXTEDIT_UNDOCHARCOUNT == 999); // Test designed for this value

        // Insert 350 characters via 10 paste operations
        // We use paste operations instead of key-by-key insertion so we know our undo buffer will contains 10 undo points.
        //const char line_buf[26+8+1+1] = "xxxxxxx abcdefghijklmnopqrstuvwxyz\n"; // 8+26+1 = 35
        //ImGui::SetClipboardText(line_buf);
        //IM_CHECK(strlen(line_buf) == 35);
        //ctx->KeyPressMap(ImGuiKey_V, ImGuiKeyModFlags_Ctrl, 10);

        // Select all, copy, paste 3 times
        ctx->KeyPressMap(ImGuiKey_A, ImGuiKeyModFlags_Ctrl);    // Select all
        ctx->KeyPressMap(ImGuiKey_C, ImGuiKeyModFlags_Ctrl);    // Copy
        ctx->KeyPressMap(ImGuiKey_End, ImGuiKeyModFlags_Ctrl);  // Go to end, clear selection
        ctx->SleepShort();
        for (int n = 0; n < 3; n++)
        {
            ctx->KeyPressMap(ImGuiKey_V, ImGuiKeyModFlags_Ctrl);// Paste append three times
            ctx->SleepShort();
        }
        size_t len = strlen(vars.StrLarge.Data);
        IM_CHECK(len == 350 * 4);
        IM_CHECK(undo_state.undo_point == 3);
        IM_CHECK(undo_state.undo_char_point == 0);

        // Undo x2
        IM_CHECK(undo_state.redo_point == STB_TEXTEDIT_UNDOSTATECOUNT);
        ctx->KeyPressMap(ImGuiKey_Z, ImGuiKeyModFlags_Ctrl);
        ctx->KeyPressMap(ImGuiKey_Z, ImGuiKeyModFlags_Ctrl);
        len = strlen(vars.StrLarge.Data);
        IM_CHECK(len == 350 * 2);
        IM_CHECK(undo_state.undo_point == 1);
        IM_CHECK(undo_state.redo_point == STB_TEXTEDIT_UNDOSTATECOUNT - 2);
        IM_CHECK(undo_state.redo_char_point == STB_TEXTEDIT_UNDOCHARCOUNT - 350 * 2);

        // Undo x1 should call stb_textedit_discard_redo()
        ctx->KeyPressMap(ImGuiKey_Z, ImGuiKeyModFlags_Ctrl);
        len = strlen(vars.StrLarge.Data);
        IM_CHECK(len == 350 * 1);
    };

    // ## Test InputText vs user ownership of data
    t = REGISTER_TEST("widgets", "widgets_inputtext_3_text_ownership");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::LogToBuffer();
        ImGui::InputText("##InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1)); // Remove label to simplify the capture/comparison
        ImFormatString(vars.Str2, IM_ARRAYSIZE(vars.Str2), "%s", ctx->UiContext->LogBuffer.c_str());
        ImGui::LogFinish();
        ImGui::Text("Captured: \"%s\"", vars.Str2);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        char* buf_user = vars.Str1;
        char* buf_visible = vars.Str2;
        ctx->SetRef("Test Window");

        IM_CHECK(strcmp(buf_visible, "") == 0);
        strcpy(buf_user, "Hello");
        ctx->Yield();
        IM_CHECK(strcmp(buf_visible, "Hello") == 0);
        ctx->ItemClick("##InputText");
        ctx->KeyCharsAppend("1");
        ctx->Yield();
        IM_CHECK(strcmp(buf_user, "Hello1") == 0);
        IM_CHECK(strcmp(buf_visible, "Hello1") == 0);

        // Because the item is active, it owns the source data, so:
        strcpy(buf_user, "Overwritten");
        ctx->Yield();
        IM_CHECK(strcmp(buf_user, "Hello1") == 0);
        IM_CHECK(strcmp(buf_visible, "Hello1") == 0);

        // Lose focus, at this point the InputTextState->ID should be holding on the last active state,
        // so we verify that InputText() is picking up external changes.
        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK(ctx->UiContext->ActiveId == 0);
        strcpy(buf_user, "Hello2");
        ctx->Yield();
        IM_CHECK(strcmp(buf_user, "Hello2") == 0);
        IM_CHECK(strcmp(buf_visible, "Hello2") == 0);
    };

    // ## Test that InputText doesn't go havoc when activated via another item
    t = REGISTER_TEST("widgets", "widgets_inputtext_4_id_conflict");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 50, 0.0f));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ctx->FrameCount < 50)
            ImGui::Button("Hello");
        else
            ImGui::InputText("Hello", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ctx->ItemHoldForFrames("Hello", 100);
    };

    // ## Test InputText() and IsItemDeactivatedXXX() functions (mentioned in #2215)
    t = REGISTER_TEST("widgets", "widgets_status_inputtext");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        bool ret = ImGui::InputText("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        vars.Status.QueryInc(ret);
        ImGui::InputText("Dummy Sibling", vars.Str2, IM_ARRAYSIZE(vars.Str2));
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Accumulate return values over several frames/action into each bool
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiTestGenericStatus& status = vars.Status;

        // Testing activation flag being set
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field");
        IM_CHECK(status.Ret == 0 && status.Activated == 1 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0 && status.Edited == 0);
        status.Clear();

        // Testing deactivated flag being set when canceling with Escape
        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK(status.Ret == 0 && status.Activated == 0 && status.Deactivated == 1 && status.DeactivatedAfterEdit == 0 && status.Edited == 0);
        status.Clear();

        // Testing validation with Return after editing
        ctx->ItemClick("Field");
        IM_CHECK(!status.Ret && status.Activated && !status.Deactivated && !status.DeactivatedAfterEdit && status.Edited == 0);
        status.Clear();
        ctx->KeyCharsAppend("Hello");
        IM_CHECK(status.Ret && !status.Activated && !status.Deactivated && !status.DeactivatedAfterEdit && status.Edited >= 1);
        status.Clear();
        ctx->KeyPressMap(ImGuiKey_Enter);
        IM_CHECK(!status.Ret && !status.Activated && status.Deactivated && status.DeactivatedAfterEdit && status.Edited == 0);
        status.Clear();

        // Testing validation with Tab after editing
        ctx->ItemClick("Field");
        ctx->KeyCharsAppend(" World");
        IM_CHECK(status.Ret && status.Activated && !status.Deactivated && !status.DeactivatedAfterEdit && status.Edited >= 1);
        status.Clear();
        ctx->KeyPressMap(ImGuiKey_Tab);
        IM_CHECK(!status.Ret && !status.Activated && status.Deactivated && status.DeactivatedAfterEdit && status.Edited == 0);
        status.Clear();
    };

    // ## Test the IsItemDeactivatedXXX() functions (e.g. #2550, #1875)
    t = REGISTER_TEST("widgets", "widgets_status_inputfloat2");
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
        ImGuiTestGenericStatus& status = vars.Status;

        // FIXME-TESTS: Better helper to build ids out of various type of data
        ctx->SetRef("Test Window");
        int n;
        n = 0; ImGuiID field_0 = ImHash(&n, sizeof(n), ctx->GetID("Field"));
        n = 1; ImGuiID field_1 = ImHash(&n, sizeof(n), ctx->GetID("Field"));
        //n = 2; ImGuiID field_2 = ImHash(&n, sizeof(n), ctx->GetID("Field"));

        // Testing activation/deactivation flags
        ctx->ItemClick(field_0);
        IM_CHECK(status.Ret == 0 && status.Activated == 1 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0);
        status.Clear();
        ctx->KeyPressMap(ImGuiKey_Enter);
        IM_CHECK(status.Ret == 0 && status.Activated == 0 && status.Deactivated == 1 && status.DeactivatedAfterEdit == 0);
        status.Clear();

        // Testing validation with Return after editing
        ctx->ItemClick(field_0);
        status.Clear();
        ctx->KeyCharsAppend("123");
        IM_CHECK(status.Ret >= 1 && status.Activated == 0 && status.Deactivated == 0);
        status.Clear();
        ctx->KeyPressMap(ImGuiKey_Enter);
        IM_CHECK(status.Ret == 0 && status.Activated == 0 && status.Deactivated == 1);
        status.Clear();

        // Testing validation with Tab after editing
        ctx->ItemClick(field_0);
        ctx->KeyCharsAppend("456");
        status.Clear();
        ctx->KeyPressMap(ImGuiKey_Tab);
        IM_CHECK(status.Ret == 0 && status.Activated == 1 && status.Deactivated == 1 && status.DeactivatedAfterEdit == 1);

        // Testing Edited flag on all components
        ctx->ItemClick(field_1); // FIXME-TESTS: Should not be necessary!
        ctx->ItemClick(field_0);
        ctx->KeyCharsAppend("111");
        IM_CHECK(status.Edited >= 1);
        ctx->KeyPressMap(ImGuiKey_Tab);
        status.Clear();
        ctx->KeyCharsAppend("222");
        IM_CHECK(status.Edited >= 1);
        ctx->KeyPressMap(ImGuiKey_Tab);
        status.Clear();
        ctx->KeyCharsAppend("333");
        IM_CHECK(status.Edited >= 1);
    };

    // ## Test the IsItemEdited() function when input vs output format are not matching
    t = REGISTER_TEST("widgets", "widgets_status_inputfloat");
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
        ImGuiTestGenericStatus& status = vars.Status;

        // Input "1" which will be formatted as "1.000", make sure we don't report IsItemEdited() multiple times!
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field");
        ctx->KeyCharsAppend("1");
        IM_CHECK(status.Ret == 1 && status.Edited == 1 && status.Activated == 1 && status.Deactivated == 0 && status.DeactivatedAfterEdit == 0);
        ctx->Yield();
        ctx->Yield();
        IM_CHECK(status.Edited == 1);
    };

    // ## Test ColorEdit Drag and Drop
    t = REGISTER_TEST("widgets", "widgets_coloredit_drag");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(300, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::ColorEdit4("ColorEdit1", &vars.Vec4Array[0].x, ImGuiColorEditFlags_None);
        ImGui::ColorEdit4("ColorEdit2", &vars.Vec4Array[1].x, ImGuiColorEditFlags_None);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        vars.Vec4Array[0] = ImVec4(1, 0, 0, 1);
        vars.Vec4Array[1] = ImVec4(0, 1, 0, 1);

        ctx->SetRef("Test Window");

        IM_CHECK(memcmp(&vars.Vec4Array[0], &vars.Vec4Array[1], sizeof(ImVec4)) != 0);
        ctx->ItemDragAndDrop("ColorEdit1/##ColorButton", "ColorEdit2/##X"); // FIXME-TESTS: Inner items
        IM_CHECK(memcmp(&vars.Vec4Array[0], &vars.Vec4Array[1], sizeof(ImVec4)) == 0);
    };

    // ## Test that disabled Selectable has an ID but doesn't interfer with navigation
    t = REGISTER_TEST("widgets", "widgets_selectable");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ctx->SetRef("Test Window");
        ImGui::Selectable("Selectable A");
        if (ctx->FrameCount == 0)
            IM_CHECK(ImGui::GetItemID() == ctx->GetID("Selectable A"));
        ImGui::Selectable("Selectable B", false, ImGuiSelectableFlags_Disabled);
        if (ctx->FrameCount == 0)
            IM_CHECK(ImGui::GetItemID() == ctx->GetID("Selectable B")); // Make sure B has an ID
        ImGui::Selectable("Selectable C");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ctx->ItemClick("Selectable A");
        IM_CHECK(ctx->UiContext->NavId == ctx->GetID("Selectable A"));
        ctx->KeyPressMap(ImGuiKey_DownArrow);
        IM_CHECK(ctx->UiContext->NavId == ctx->GetID("Selectable C")); // Make sure we have skipped B
    };

    // ## Test recursing Tab Bars (Bug #2371)
    t = REGISTER_TEST("widgets", "widgets_tabbar_recurse");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::BeginTabBar("TabBar 0"))
        {
            if (ImGui::BeginTabItem("TabItem"))
            {
                // If we have many tab bars here, it will invalidate pointers from pooled tab bars
                for (int i = 0; i < 128; i++)
                {
                    char buf[64];
                    ImFormatString(buf, IM_ARRAYSIZE(buf), "Inner TabBar %d", i);
                    if (ImGui::BeginTabBar(buf))
                    {
                        if (ImGui::BeginTabItem("Inner TabItem"))
                            ImGui::EndTabItem();
                        ImGui::EndTabBar();
                    }
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    };

#ifdef IMGUI_HAS_DOCK
    // ## Test Dockspace within a TabItem
    t = REGISTER_TEST("widgets", "widgets_tabbar_dockspace");
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
}

//-------------------------------------------------------------------------
// Tests: Nav
//-------------------------------------------------------------------------

void RegisterTests_Nav(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test opening a new window from a checkbox setting the focus to the new window.
    // In 9ba2028 (2019/01/04) we fixed a bug where holding ImGuiNavInputs_Activate too long on a button would hold the focus on the wrong window.
    t = REGISTER_TEST("nav", "nav_001");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetInputMode(ImGuiInputSource_Nav);
        ctx->SetRef("Hello, world!");
        ctx->ItemUncheck("Demo Window");
        ctx->ItemCheck("Demo Window");

        ImGuiContext& g = *ctx->UiContext;
        IM_CHECK(g.NavWindow && g.NavWindow->ID == ctx->GetID("/ImGui Demo"));
    };

    // ## Verify that CTRL+Tab steal focus (#2380)
    t = REGISTER_TEST("nav", "nav_ctrl_tab_takes_focus_away");
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
        IM_CHECK(ctx->UiContext->ActiveId == ctx->GetID("InputText"));
        ctx->KeyPressMap(ImGuiKey_Tab, ImGuiKeyModFlags_Ctrl);
        IM_CHECK(ctx->UiContext->ActiveId == 0);
        ctx->Sleep(1.0f);
    };

    // ## Test that ESC deactivate InputText without closing current Popup (#2321, #787)
    t = REGISTER_TEST("nav", "nav_esc_popup");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        bool& b_popup_open = vars.Bool1;
        bool& b_field_active = vars.Bool2;
        ImGuiID& popup_id = vars.Id;

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Open Popup"))
            ImGui::OpenPopup("Popup");

        b_popup_open = ImGui::BeginPopup("Popup");
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

        // FIXME-TESTS: Come up with a better mecanism to get popup ID
        ImGuiID& popup_id = vars.Id;
        popup_id = 0;

        ctx->SetRef("Test Window");
        ctx->ItemClick("Open Popup");

        while (popup_id == 0)
            ctx->Yield();

        ctx->SetRef(popup_id);
        ctx->ItemClick("Field");
        IM_CHECK(b_popup_open && b_field_active);

        ctx->KeyPressMap(ImGuiKey_Escape);
        IM_CHECK(b_popup_open && !b_field_active);
    };
}

//-------------------------------------------------------------------------
// Tests: Columns
//-------------------------------------------------------------------------

void RegisterTests_Columns(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test number of draw calls used by columns
    t = REGISTER_TEST("columns", "columns_draw_calls");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Hello");
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Test: Single column don't consume draw call.
        int cmd_count = draw_list->CmdBuffer.Size;
        ImGui::BeginColumns("columns1", 1);
        ImGui::Text("AAAA"); ImGui::NextColumn();
        ImGui::Text("BBBB"); ImGui::NextColumn();
        ImGui::EndColumns();
        ImGui::Text("Hello");
        IM_CHECK(draw_list->CmdBuffer.Size == cmd_count + 0);

        // Test: Multi-column consume 1 draw call per column + 1 due to conservative overlap expectation (FIXME)
        ImGui::BeginColumns("columns3", 3);
        ImGui::Text("AAAA"); ImGui::NextColumn();
        ImGui::Text("BBBB"); ImGui::NextColumn();
        ImGui::Text("CCCC"); ImGui::NextColumn();
        ImGui::EndColumns();
        ImGui::Text("Hello");
        IM_CHECK(draw_list->CmdBuffer.Size == cmd_count || draw_list->CmdBuffer.Size == cmd_count + 3 + 1);

        // Test: Unused column don't consume a draw call
        cmd_count = draw_list->CmdBuffer.Size;
        ImGui::BeginColumns("columns3", 3);
        ImGui::Text("AAAA"); ImGui::NextColumn();
        ImGui::Text("BBBB"); ImGui::NextColumn(); // Leave one column empty
        ImGui::EndColumns();
        ImGui::Text("Hello");
        IM_CHECK(draw_list->CmdBuffer.Size == cmd_count || draw_list->CmdBuffer.Size == cmd_count + 2 + 1);

        ImGui::End();
    };
};

//-------------------------------------------------------------------------
// Tests: Docking
//-------------------------------------------------------------------------

void RegisterTests_Docking(ImGuiTestEngine* e)
{
#ifdef IMGUI_HAS_DOCK
    ImGuiTest* t = NULL;

    t = REGISTER_TEST("docking", "docking_basic_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ImGuiID ids[3];
        ImGui::Begin("AAAA");
        ids[0] = ImGui::GetWindowDockID();
        ImGui::Text("This is AAAA");
        ImGui::End();

        ImGui::Begin("BBBB");
        ids[1] = ImGui::GetWindowDockID();
        ImGui::Text("This is BBBB");
        ImGui::End();

        ImGui::Begin("CCCC");
        ids[2] = ImGui::GetWindowDockID();
        ImGui::Text("This is CCCC");
        ImGui::End();

        if (ctx->FrameCount == 1)
        {
            IM_CHECK(ctx->DockIdIsUndockedOrStandalone(ids[0]));
            IM_CHECK(ctx->DockIdIsUndockedOrStandalone(ids[1]));
            IM_CHECK(ctx->DockIdIsUndockedOrStandalone(ids[2]));
        }
        if (ctx->FrameCount == 11)
        {
            IM_CHECK(vars.DockId != 0);
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

    // ## Test that docking into a parent node forwarding docking into the central node or last focused node
    t = REGISTER_TEST("docking", "docking_into_parent_node");
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
    t = REGISTER_TEST("docking", "docking_auto_nodes_size");
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
            IM_CHECK(actual_sz == expected_sz);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        bool backup_cfg = ctx->UiContext->IO.ConfigDockingTabBarOnSingleWindows;
        ctx->LogVerbose("ConfigDockingTabBarOnSingleWindows = false\n");
        ctx->UiContext->IO.ConfigDockingTabBarOnSingleWindows = false;
        ctx->YieldFrames(4);
        ctx->LogVerbose("ConfigDockingTabBarOnSingleWindows = true\n");
        ctx->UiContext->IO.ConfigDockingTabBarOnSingleWindows = true;
        ctx->YieldFrames(4);
        ctx->LogVerbose("ConfigDockingTabBarOnSingleWindows = false\n");
        ctx->UiContext->IO.ConfigDockingTabBarOnSingleWindows = false;
        ctx->YieldFrames(4);
        ctx->UiContext->IO.ConfigDockingTabBarOnSingleWindows = backup_cfg;
    };

    // ## Test merging windows by dragging them.
    t = REGISTER_TEST("docking", "docking_drag_merge");
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
        // FIXME-TESTS: Tests doesn't work if already docked
        // FIXME-TESTS: DockSetMulti takes window_name not ref

        ImGuiWindow* window_aaaa = ctx->GetWindowByRef("AAAA");
        ImGuiWindow* window_bbbb = ctx->GetWindowByRef("BBBB");

        // Init state
        ctx->DockMultiClear("AAAA", "BBBB", NULL);
        if (ctx->UiContext->IO.ConfigDockingTabBarOnSingleWindows)
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
        IM_CHECK(window_aaaa->DockNode->Pos == ImVec2(200, 200));
        IM_CHECK(window_aaaa->Pos == ImVec2(200, 200));
        IM_CHECK(window_bbbb->Pos == ImVec2(200, 200));
        ImGuiID dock_id = window_bbbb->DockId;

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
            IM_CHECK(window_aaaa->DockId == dock_id);
            IM_CHECK(window_bbbb->DockId == dock_id);
            IM_CHECK(window_aaaa->Pos == ImVec2(300, 300));
            IM_CHECK(window_bbbb->Pos == ImVec2(300, 300));
            IM_CHECK(window_aaaa->Size == ImVec2(200, 200));
            IM_CHECK(window_bbbb->Size == ImVec2(200, 200));
            IM_CHECK(window_aaaa->DockNode->Pos == ImVec2(300, 300));
            IM_CHECK(window_aaaa->DockNode->Size == ImVec2(200, 200));
        }

        {
            // Undock AAAA, BBBB should still refer/dock to node.
            ctx->DockMultiClear("AAAA", NULL);
            IM_CHECK(ctx->DockIdIsUndockedOrStandalone(window_aaaa->DockId));
            IM_CHECK(window_bbbb->DockId == dock_id);

            // Intentionally move both floating windows away
            ctx->WindowMove("/AAAA", ImVec2(100, 100));
            ctx->WindowMove("/BBBB", ImVec2(200, 200));

            // Dock on the side (BBBB still refers to dock id, making this different from the first docking)
            ctx->DockWindowInto("/AAAA", "/BBBB", ImGuiDir_Left);
            IM_CHECK(window_aaaa->DockNode->ParentNode->ID == dock_id);
            IM_CHECK(window_bbbb->DockNode->ParentNode->ID == dock_id);
            IM_CHECK(window_aaaa->DockNode->ParentNode->Pos == ImVec2(200, 200));
            IM_CHECK(window_aaaa->Pos == ImVec2(200, 200));
            IM_CHECK(window_bbbb->Pos.x > window_aaaa->Pos.x);
            IM_CHECK(window_bbbb->Pos.y == 200);
        }
    };

    // ## Test setting focus on a docked window, and setting focus on a specific item inside. (#2453)
    // ## In particular, while the selected tab is locked down early (causing a frame delay in the tab selection), 
    // ## the window that has requested focus should allow items to be submitted (SkipItems==false) during its hidden frame,
    // ## mimicking the behavior of any newly appearing window.
    t = REGISTER_TEST("docking", "docking_focus_1");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        if (ctx->FrameCount == 0)
            vars.DockId = ctx->DockMultiSetupBasic(0, "AAAA", "BBBB", "CCCC", NULL);

        if (ctx->FrameCount == 10)  ImGui::SetNextWindowFocus();
        ImGui::Begin("AAAA");
        if (ctx->FrameCount == 10)  IM_CHECK(ImGui::IsWindowFocused());
        if (ctx->FrameCount == 10)  ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Input", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        if (ctx->FrameCount == 10)  IM_CHECK(!ImGui::IsItemActive());
        if (ctx->FrameCount == 11)  IM_CHECK(ImGui::IsItemActive());
        ImGui::End();

        if (ctx->FrameCount == 50)  ImGui::SetNextWindowFocus();
        ImGui::Begin("BBBB");
        if (ctx->FrameCount == 50)  IM_CHECK(ImGui::IsWindowFocused());
        if (ctx->FrameCount == 50)  ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Input", vars.Str2, IM_ARRAYSIZE(vars.Str2));
        if (ctx->FrameCount == 50)  IM_CHECK(!ImGui::IsItemActive());
        if (ctx->FrameCount == 51)  IM_CHECK(ImGui::IsItemActive());
        ImGui::End();

        if (ctx->FrameCount == 60)
            ctx->Finish();
    };
#endif
}

//-------------------------------------------------------------------------
// Tests: Misc
//-------------------------------------------------------------------------

void RegisterTests_Misc(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test hash functions and ##/### operators
    t = REGISTER_TEST("misc", "hash_001");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Test hash function for the property we need
        IM_CHECK(ImHashStr("helloworld") == ImHashStr("world", 0, ImHashStr("hello", 0)));  // String concatenation
        IM_CHECK(ImHashStr("hello###world") == ImHashStr("###world"));                      // ### operator reset back to the seed
        IM_CHECK(ImHashStr("hello###world", 0, 1234) == ImHashStr("###world", 0, 1234));    // ### operator reset back to the seed
        IM_CHECK(ImHashStr("helloxxx", 5) == ImHashStr("hello"));                           // String size is honored
        IM_CHECK(ImHashStr("", 0, 0) == 0);                                                 // Empty string doesn't alter hash
        IM_CHECK(ImHashStr("", 0, 1234) == 1234);                                           // Empty string doesn't alter hash
        IM_CHECK(ImHashStr("hello", 5) == ImHashData("hello", 5));                          // FIXME: Do we need to guarantee this?

        const int data[2] = { 42, 50 };
        IM_CHECK(ImHashData(&data[0], sizeof(int) * 2) == ImHashData(&data[1], sizeof(int), ImHashData(&data[0], sizeof(int))));
        IM_CHECK(ImHashData("", 0, 1234) == 1234);                                          // Empty data doesn't alter hash

        // Verify that Test Engine high-level hash wrapper works
        IM_CHECK(ImHashDecoratedPath("Hello/world") == ImHashStr("Helloworld"));            // Slashes are ignored
        IM_CHECK(ImHashDecoratedPath("Hello\\/world") == ImHashStr("Hello/world"));         // Slashes can be inhibited
        IM_CHECK(ImHashDecoratedPath("/Hello", 42) == ImHashDecoratedPath("Hello"));        // Leading / clears seed
    };

    // ## Test ImFontAtlas building with overlapping glyph ranges (#2353, #2233)
    t = REGISTER_TEST("misc", "misc_atlas_build_glyph_overlap");
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

    // FIXME-TESTS
    t = REGISTER_TEST("demo", "demo_misc_001");
    t->GuiFunc = NULL;
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
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
        ctx->PopupClose();

        //ctx->ItemClick("Layout");  // FIXME: close popup
        ctx->ItemOpen("Layout");
        ctx->ItemOpen("Horizontal Scrolling");
        ctx->ItemHold("Horizontal Scrolling/>>", 1.0f);
        ctx->SleepShort();
    };

    // ## Coverage: open everything in demo window
    t = REGISTER_TEST("demo", "demo_cov_auto_open");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->ItemOpenAll("");
    };

    // ## Coverage: closes everything in demo window
    t = REGISTER_TEST("demo", "demo_cov_auto_close");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->ItemCloseAll("");
    };

    t = REGISTER_TEST("demo", "demo_cov_001");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->ItemOpen("Help");
        ctx->ItemOpen("Configuration");
        ctx->ItemOpen("Window options");
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Layout");
        ctx->ItemOpen("Popups & Modal windows");
        ctx->ItemOpen("Columns");
        ctx->ItemOpen("Filtering");
        ctx->ItemOpen("Inputs, Navigation & Focus");
    };

    t = REGISTER_TEST("demo", "demo_cov_apps");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->MenuClick("Menu/Open Recent/More..");
        ctx->MenuCheckAll("Examples");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuCheckAll("Help");
        ctx->MenuUncheckAll("Help");
    };

    // ## Coverage: select all styles via the Style Editor
    t = REGISTER_TEST("demo", "demo_cov_styles");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->MenuAction(ImGuiTestAction_Check, "Help/Style Editor");

        ImGuiTestRef ref_window = "Style Editor";
        ctx->SetRef(ref_window);
        ctx->ItemClick("Colors##Selector");
        ctx->Yield();
        ImGuiTestRef ref_popup = ctx->GetFocusWindowRef();

        ImGuiStyle style_backup = ImGui::GetStyle();
        ImGuiTestItemList items;
        ctx->GatherItems(&items, ref_popup);
        for (int n = 0; n < items.Size; n++)
        {
            ctx->SetRef(ref_window);
            ctx->ItemClick("Colors##Selector");
            ctx->SetRef(ref_popup);
            ctx->ItemClick(items[n]->ID);
        }
        ImGui::GetStyle() = style_backup;
    };
}

//-------------------------------------------------------------------------
// Tests: Perf
//-------------------------------------------------------------------------

void RegisterTests_Perf(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    auto PerfCaptureFunc = [](ImGuiTestContext* ctx) 
    {
        ctx->PerfCapture();
    };

    t = REGISTER_TEST("perf", "perf_demo_all");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->PerfCalcRef();

        ctx->SetRef("ImGui Demo");
        ctx->ItemOpenAll("");
        ctx->MenuCheckAll("Examples");
        ctx->MenuCheckAll("Help");

        IM_CHECK(ctx->UiContext->IO.DisplaySize.x > 820);
        IM_CHECK(ctx->UiContext->IO.DisplaySize.y > 820);

        // FIXME-TESTS: Backup full layout
        ImVec2 pos = ctx->GetMainViewportPos() + ImVec2(20, 20);
        for (ImGuiWindow* window : ctx->UiContext->Windows)
        {
            window->Pos = pos;
            window->SizeFull = ImVec2(800, 800);
        }
        ctx->PerfCapture();

        ctx->SetRef("ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuUncheckAll("Help");
    };

    enum
    {
        DrawPrimFunc_RectStroke,
        DrawPrimFunc_RectStrokeThick,
        DrawPrimFunc_RectFilled,
        DrawPrimFunc_RectRoundedStroke,
        DrawPrimFunc_RectRoundedStrokeThick,
        DrawPrimFunc_RectRoundedFilled,
        DrawPrimFunc_CircleStroke,
        DrawPrimFunc_CircleStrokeThick,
        DrawPrimFunc_CircleFilled,
    };

    auto DrawPrimFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 200 * ctx->PerfStressAmount;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        int segments = 12;
        ImGui::Button("##CircleFilled", ImVec2(200, 200));
        ImVec2 bounds_min = ImGui::GetItemRectMin();
        ImVec2 bounds_size = ImGui::GetItemRectSize();
        ImVec2 center = bounds_min + bounds_size * 0.5f;
        float r = (float)(int)(ImMin(bounds_size.x, bounds_size.y) * 0.8f * 0.5f);
        float rounding = 8.0f;
        ImU32 col = IM_COL32(255, 255, 0, 255);
        switch (ctx->Test->ArgVariant)
        {
        case DrawPrimFunc_RectStroke:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r,r), center + ImVec2(r,r), col, 0.0f, ~0, 1.0f);
            break;
        case DrawPrimFunc_RectStrokeThick:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r, r), center + ImVec2(r, r), col, 0.0f, ~0, 4.0f);
            break;
        case DrawPrimFunc_RectFilled:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRectFilled(center - ImVec2(r, r), center + ImVec2(r, r), col, 0.0f);
            break;
        case DrawPrimFunc_RectRoundedStroke:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r, r), center + ImVec2(r, r), col, rounding, ~0, 1.0f);
            break;
        case DrawPrimFunc_RectRoundedStrokeThick:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r, r), center + ImVec2(r, r), col, rounding, ~0, 4.0f);
            break;
        case DrawPrimFunc_RectRoundedFilled:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRectFilled(center - ImVec2(r, r), center + ImVec2(r, r), col, rounding);
            break;
        case DrawPrimFunc_CircleStroke:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddCircle(center, r, col, segments, 1.0f);
            break;
        case DrawPrimFunc_CircleStrokeThick:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddCircle(center, r, col, segments, 4.0f);
            break;
        case DrawPrimFunc_CircleFilled:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddCircleFilled(center, r, col, segments);
            break;
        default:
            IM_ASSERT(0);
        }
        ImGui::End();
    };

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_stroke");
    t->ArgVariant = DrawPrimFunc_RectStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_stroke_thick");
    t->ArgVariant = DrawPrimFunc_RectStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_filled");
    t->ArgVariant = DrawPrimFunc_RectFilled;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_rounded_stroke");
    t->ArgVariant = DrawPrimFunc_RectRoundedStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_rounded_stroke_thick");
    t->ArgVariant = DrawPrimFunc_RectRoundedStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_rect_rounded_filled");
    t->ArgVariant = DrawPrimFunc_RectRoundedFilled;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_circle_stroke");
    t->ArgVariant = DrawPrimFunc_CircleStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_circle_stroke_thick");
    t->ArgVariant = DrawPrimFunc_CircleStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_prim_circle_filled");
    t->ArgVariant = DrawPrimFunc_CircleFilled;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    auto DrawSplittedFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        int split_count = ctx->Test->ArgVariant;
        int loop_count = 200 * ctx->PerfStressAmount;

        ImGui::Button("##CircleFilled", ImVec2(200, 200));
        ImVec2 bounds_min = ImGui::GetItemRectMin();
        ImVec2 bounds_size = ImGui::GetItemRectSize();
        ImVec2 center = bounds_min + bounds_size * 0.5f;
        float r = (float)(int)(ImMin(bounds_size.x, bounds_size.y) * 0.8f * 0.5f);

        if (ctx->FrameCount == 0)
            ctx->Log("%d primitives over %d channels\n", loop_count, split_count);
        if (split_count != 1)
        {
            IM_CHECK_SILENT((loop_count % split_count) == 0);
            loop_count /= split_count;
            draw_list->ChannelsSplit(split_count);
            for (int n = 0; n < loop_count; n++)
                for (int ch = 0; ch < split_count; ch++)
                {
                    draw_list->ChannelsSetCurrent(ch);
                    draw_list->AddCircleFilled(center, r, IM_COL32(255, 255, 0, 255), 12);
                }
            draw_list->ChannelsMerge();
        }
        else
        {
            for (int n = 0; n < loop_count; n++)
                draw_list->AddCircleFilled(center, r, IM_COL32(255, 255, 0, 255), 12);
        }
        if (ctx->FrameCount == 0)
            ctx->Log("Vertices: %d\n", draw_list->VtxBuffer.Size);

        ImGui::End();
    };

    t = REGISTER_TEST("perf", "perf_draw_split_1");
    t->ArgVariant = 1;
    t->GuiFunc = DrawSplittedFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_draw_split_0");
    t->ArgVariant = 10;
    t->GuiFunc = DrawSplittedFunc;
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_button");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
            ImGui::Button("Hello, world");
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_checkbox");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        bool v1 = false, v2 = true;
        for (int n = 0; n < loop_count / 2; n++)
        {
            ImGui::PushID(n);
            ImGui::Checkbox("Hello, world", &v1);
            ImGui::Checkbox("Hello, world", &v2);
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_coloredit4");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 500 * ctx->PerfStressAmount;
        ImVec4 col(1.0f, 0.0f, 0.0f, 1.0f);
        for (int n = 0; n < loop_count / 2; n++)
        {
            ImGui::PushID(n);
            ImGui::ColorEdit4("Color", &col.x);
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_input_text");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        char buf[32] = "123";
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            ImGui::InputText("InputText", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_None);
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_input_text_multiline");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        char buf[32] = "123";
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            ImGui::InputTextMultiline("InputText", buf, IM_ARRAYSIZE(buf), ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 2), ImGuiInputTextFlags_None);
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_hash");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Hashing..");
        int loop_count = 5000 * ctx->PerfStressAmount;
        char buf[32] = { 0 };
        ImU32 seed = 0;
        for (int n = 0; n < loop_count; n++)
        {
            seed = ImHashData(buf, 32, seed);
            seed = ImHashStr("Hash me tender", 0, seed);
            seed = ImHashStr("Hash me true", 12, seed);
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_list_box");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            if (ImGui::ListBoxHeader("ListBox", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 2)))
            {
                ImGui::MenuItem("Hello");
                ImGui::MenuItem("World");
                ImGui::ListBoxFooter();
            }
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_slider");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        float f = 1.234f;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            ImGui::SliderFloat("SliderFloat", &f, 0.0f, 10.0f);
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_slider2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        ImVec2 v(0.0f, 0.0f);
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            ImGui::SliderFloat2("SliderFloat2", &v.x, 0.0f, 10.0f);
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_text_unformatted_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        const char* buf =
            "0123456789 The quick brown fox jumps over the lazy dog.\n"
            "0123456789   The quick brown fox jumps over the lazy dog.\n"
            "0123456789     The quick brown fox jumps over the lazy dog.\n";
        int loop_count = 1000 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
            ImGui::TextUnformatted(buf);
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_text_unformatted_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        static ImGuiTextBuffer buf;
        if (buf.empty())
        {
            for (int i = 0; i < 1000; i++)
                buf.appendf("%i The quick brown fox jumps over the lazy dog\n", i);
        }
        int loop_count = 100 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
            ImGui::TextUnformatted(buf.begin(), buf.end());
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = REGISTER_TEST("perf", "perf_stress_window");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImVec2 pos = ctx->GetMainViewportPos() + ImVec2(20, 20);
        int loop_count = 200 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            char window_name[32];
            ImFormatString(window_name, IM_ARRAYSIZE(window_name), "Window_%05d", n+1);
            ImGui::SetNextWindowPos(pos);
            ImGui::Begin(window_name, NULL, ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextUnformatted("Opening many windows!");
            ImGui::End();
        }
    };
    t->TestFunc = PerfCaptureFunc;
}

//-------------------------------------------------------------------------
// Tests: Capture
//-------------------------------------------------------------------------

void RegisterTests_Capture(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    t = REGISTER_TEST("capture", "capture_demo_documents");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("ImGui Demo");
        ctx->MenuAction(ImGuiTestAction_Check, "Examples/Documents");

        ctx->SetRef("Examples: Documents");
        // FIXME-TESTS: Locate within stack that uses windows/<pointer>/name
        ctx->ItemCheck("Tomato"); // FIXME: WILL FAIL, NEED TO LOCATE BY NAME (STACK WILDCARD?)
        ctx->ItemCheck("A Rather Long Title");
        ctx->ItemClick("##tabs/Eggplant");
        ctx->MouseMove("##tabs/Eggplant/Modify");
        ctx->Sleep(1.0f);
    };

#if 1

    // TODO: Better position of windows.
    // TODO: Draw in custom rendering canvas
    // TODO: Select color picker mode
    // TODO: Flags document as "Modified"
    // TODO: InputText selection
    t = REGISTER_TEST("capture", "capture_readme_misc");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiIO& io = ImGui::GetIO();
        //ImGuiStyle& style = ImGui::GetStyle();

        ctx->SetRef("ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuCheck("Examples/Simple overlay");
        ctx->SetRef("Example: Simple overlay");
        ImGuiWindow* window_overlay = ctx->GetWindowByRef("");
        IM_CHECK(window_overlay != NULL);

        // FIXME-TESTS: Find last newly opened window?

        float fh = ImGui::GetFontSize();
        float pad = fh;

        ctx->SetRef("ImGui Demo");
        ctx->MenuCheck("Examples/Custom rendering");
        ctx->SetRef("Example: Custom rendering");
        ctx->WindowResize("", ImVec2(fh * 30, fh * 30));
        ctx->WindowMove("", window_overlay->Rect().GetBL() + ImVec2(0.0f, pad));
        ImGuiWindow* window_custom_rendering = ctx->GetWindowByRef("");
        IM_CHECK(window_custom_rendering != NULL);

        ctx->SetRef("ImGui Demo");
        ctx->MenuCheck("Examples/Simple layout");
        ctx->SetRef("Example: Simple layout");
        ctx->WindowResize("", ImVec2(fh * 50, fh * 15));
        ctx->WindowMove("", ImVec2(pad, io.DisplaySize.y - pad), ImVec2(0.0f, 1.0f));

        ctx->SetRef("ImGui Demo");
        ctx->MenuCheck("Examples/Documents");
        ctx->SetRef("Example: Documents");
        ctx->WindowResize("", ImVec2(fh * 20, fh * 27));
        ctx->WindowMove("", ImVec2(window_custom_rendering->Pos.x + window_custom_rendering->Size.x + pad, pad));

        ctx->LogVerbose("Setup Console window...\n");
        ctx->SetRef("ImGui Demo");
        ctx->MenuCheck("Examples/Console");
        ctx->SetRef("Example: Console");
        ctx->WindowResize("", ImVec2(fh * 40, fh * (34-7)));
        ctx->WindowMove("", window_custom_rendering->Pos + window_custom_rendering->Size * ImVec2(0.30f, 0.60f));
        ctx->ItemClick("Clear");
        ctx->ItemClick("Add Dummy Text");
        ctx->ItemClick("Add Dummy Error");
        ctx->ItemClick("Input");
        ctx->KeyChars("H");
        ctx->KeyPressMap(ImGuiKey_Tab);
        ctx->KeyCharsAppendEnter("ELP");
        ctx->KeyCharsAppendEnter("hello, imgui world!");

        ctx->LogVerbose("Setup Demo window...\n");
        ctx->SetRef("ImGui Demo");
        ctx->WindowResize("", ImVec2(fh * 35, io.DisplaySize.y - pad * 2.0f));
        ctx->WindowMove("", ImVec2(io.DisplaySize.x - pad, pad), ImVec2(1.0f, 0.0f));
        ctx->ItemOpen("Widgets");
        ctx->ItemOpen("Color\\/Picker Widgets");
        ctx->ItemOpen("Layout");
        ctx->ItemOpen("Groups");
        ctx->ScrollToY("Layout", 0.8f);

        // FIXME-TESTS: Snap!
        ctx->Sleep(2.0f);

        // Close everything
        ctx->SetRef("ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuUncheckAll("Help");
    };
#endif
}

void RegisterTests(ImGuiTestEngine* e)
{
    RegisterTests_Window(e);
    RegisterTests_Scrolling(e);
    RegisterTests_Widgets(e);
    RegisterTests_Button(e);
    RegisterTests_Nav(e);
    RegisterTests_Columns(e);
    RegisterTests_Docking(e);
    RegisterTests_Misc(e);
    //RegisterTests_Perf(e);
    RegisterTests_Capture(e);
}
