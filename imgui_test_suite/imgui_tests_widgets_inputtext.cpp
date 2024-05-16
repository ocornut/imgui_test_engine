// dear imgui
// (tests: widgets: input text)

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
// TODO: Tests: InputText: read-only + callback (#4762)
// TODO: Tests: InputTextMultiline(): (ImDrawList) position of text (#4794)
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// Tests: Widgets: Input Text
//-------------------------------------------------------------------------

struct StrVars { Str str; };

void RegisterTests_WidgetsInputText(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test InputText widget
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_basic");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1), vars.Bool1 ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiInputTextState& state = ctx->UiContext->InputTextState;
        char* buf = vars.Str1;

        ctx->SetRef("Test Window");

        // Insert
        strcpy(buf, "Hello");
        ctx->ItemClick("InputText");
        ctx->KeyCharsAppendEnter(u8"World123\u00A9");
        IM_CHECK_STR_EQ(buf, u8"HelloWorld123\u00A9");
        IM_CHECK_EQ(state.CurLenA, 15);
        IM_CHECK_EQ(state.CurLenW, 14);

        // Delete
        ctx->ItemClick("InputText");
        ctx->KeyPress(ImGuiKey_End);
        ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_LeftArrow, 2);    // Select last two characters
        ctx->KeyPress(ImGuiKey_Backspace, 3);     // Delete selection and two more characters
        ctx->KeyPress(ImGuiKey_Enter);
        IM_CHECK_STR_EQ(buf, "HelloWorld");
        IM_CHECK_EQ(state.CurLenA, 10);
        IM_CHECK_EQ(state.CurLenW, 10);

        // Insert, Cancel
        ctx->ItemClick("InputText");
        ctx->KeyPress(ImGuiKey_End);
        ctx->KeyChars("XXXXX");
        ctx->KeyPress(ImGuiKey_Escape);
        IM_CHECK_STR_EQ(buf, "HelloWorld");
        IM_CHECK_EQ(state.CurLenA, 10);
        IM_CHECK_EQ(state.CurLenW, 10);

        // Delete, Cancel
        ctx->ItemClick("InputText");
        ctx->KeyPress(ImGuiKey_End);
        ctx->KeyPress(ImGuiKey_Backspace, 5);
        ctx->KeyPress(ImGuiKey_Escape);
        IM_CHECK_STR_EQ(buf, "HelloWorld");
        IM_CHECK_EQ(state.CurLenA, 10);
        IM_CHECK_EQ(state.CurLenW, 10);

        // Read-only mode
        strcpy(buf, "Some read-only text.");
        vars.Bool1 = true;
        ctx->Yield();
        ctx->ItemClick("InputText");
        ctx->KeyCharsAppendEnter("World123");
        IM_CHECK_STR_EQ(buf, vars.Str1);
        IM_CHECK_EQ(state.CurLenA, 20);
        IM_CHECK_EQ(state.CurLenW, 20);

        // Space as key (instead of Space as character) -> check not conflicting with Nav Activate (#4552)
        vars.Bool1 = false;
        ctx->ItemClick("InputText");
        ctx->KeyCharsReplace("Hello");
        IM_CHECK(ImGui::GetActiveID() == ctx->GetID("InputText"));
        ctx->KeyPress(ImGuiKey_Space); // Should not add text, should not validate
        IM_CHECK(ImGui::GetActiveID() == ctx->GetID("InputText"));
        IM_CHECK_STR_EQ(vars.Str1, "Hello");
    };

    // ## Test InputText undo/redo ops, in particular related to issue we had with stb_textedit undo/redo buffers
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_undo_redo");
    struct InputTextUndoRedoVars
    {
        char Str1[256];
        ImVector<char> StrLarge;
    };
    t->SetVarsDataType<InputTextUndoRedoVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputTextUndoRedoVars>();
        if (vars.StrLarge.empty())
            vars.StrLarge.resize(10000, 0);
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 50, 0.0f));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("strlen() = %d", (int)strlen(vars.StrLarge.Data));
        ImGui::InputText("Other", vars.Str1, IM_ARRAYSIZE(vars.Str1), ImGuiInputTextFlags_None);
        ImGui::InputTextMultiline("InputText", vars.StrLarge.Data, (size_t)vars.StrLarge.Size, ImVec2(-1, ImGui::GetFontSize() * 20), ImGuiInputTextFlags_None);
        ImGui::End();
        //DebugInputTextState();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // https://github.com/nothings/stb/issues/321
        auto& vars = ctx->GetVars<InputTextUndoRedoVars>();
        ImGuiContext& g = *ctx->UiContext;

        // Start with a 350 characters buffer.
        // For this test we don't inject the characters via pasting or key-by-key in order to precisely control the undo/redo state.
        char* buf = vars.StrLarge.Data;
        IM_CHECK_EQ((int)strlen(buf), 0);
        for (int n = 0; n < 10; n++)
            strcat(buf, "xxxxxxx abcdefghijklmnopqrstuvwxyz\n");
        IM_CHECK_EQ((int)strlen(buf), 350);

        ctx->SetRef("Test Window");
        ctx->ItemClick("Other"); // This is to ensure stb_textedit_clear_state() gets called (clear the undo buffer, etc.)
        ctx->ItemClick("InputText");

#if IMGUI_VERSION_NUM < 19001
        int IMSTB_TEXTEDIT_UNDOSTATECOUNT = STB_TEXTEDIT_UNDOSTATECOUNT;
        int IMSTB_TEXTEDIT_UNDOCHARCOUNT = STB_TEXTEDIT_UNDOCHARCOUNT;
#endif

        ImGuiInputTextState& input_text_state = g.InputTextState;
        ImStb::StbUndoState& undo_state = input_text_state.Stb.undostate;
        IM_CHECK_EQ(input_text_state.ID, g.ActiveId);
        IM_CHECK_EQ(undo_state.undo_point, 0);
        IM_CHECK_EQ(undo_state.undo_char_point, 0);
        IM_CHECK_EQ(undo_state.redo_point, IMSTB_TEXTEDIT_UNDOSTATECOUNT);
        IM_CHECK_EQ(undo_state.redo_char_point, IMSTB_TEXTEDIT_UNDOCHARCOUNT);
        IM_CHECK_EQ(IMSTB_TEXTEDIT_UNDOCHARCOUNT, 999); // Test designed for this value

        // Insert 350 characters via 10 paste operations
        // We use paste operations instead of key-by-key insertion so we know our undo buffer will contains 10 undo points.
        //const char line_buf[26+8+1+1] = "xxxxxxx abcdefghijklmnopqrstuvwxyz\n"; // 8+26+1 = 35
        //ImGui::SetClipboardText(line_buf);
        //IM_CHECK(strlen(line_buf) == 35);
        //ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_V, 10);

        // Select all, copy, paste 3 times
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);    // Select all
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_C);    // Copy
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_End);  // Go to end, clear selection
        ctx->SleepStandard();
        for (int n = 0; n < 3; n++)
        {
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_V);// Paste append three times
            ctx->SleepStandard();
        }
        int len = (int)strlen(vars.StrLarge.Data);
        IM_CHECK_EQ(len, 350 * 4);
        IM_CHECK_EQ(undo_state.undo_point, 3);
        IM_CHECK_EQ(undo_state.undo_char_point, 0);

        // Undo x2
        IM_CHECK(undo_state.redo_point == IMSTB_TEXTEDIT_UNDOSTATECOUNT);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        len = (int)strlen(vars.StrLarge.Data);
        IM_CHECK_EQ(len, 350 * 2);
        IM_CHECK_EQ(undo_state.undo_point, 1);
        IM_CHECK_EQ(undo_state.redo_point, IMSTB_TEXTEDIT_UNDOSTATECOUNT - 2);
        IM_CHECK_EQ(undo_state.redo_char_point, IMSTB_TEXTEDIT_UNDOCHARCOUNT - 350 * 2);

        // Undo x1 should call stb_textedit_discard_redo()
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        len = (int)strlen(vars.StrLarge.Data);
        IM_CHECK_EQ(len, 350 * 1);
    };

#if IMGUI_VERSION_NUM >= 18727
    // ## Test undo stack reset when contents change while widget is inactive. (#4947's second bug, #2890)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_undo_reset");
    t->SetVarsDataType<StrVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        StrVars& vars = ctx->GetVars<StrVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("Field1", &vars.str, ImGuiInputTextFlags_CallbackHistory);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        StrVars& vars = ctx->GetVars<StrVars>();
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field1");
        ctx->KeyCharsAppend("Hello, world!");
        IM_CHECK_GT(g.InputTextState.Stb.undostate.undo_point, 0);
        ctx->KeyPress(ImGuiKey_Escape);
        vars.str = "Foobar";
        ctx->ItemClick("Field1");
        IM_CHECK_EQ(g.InputTextState.Stb.undostate.undo_point, 0);
    };
#endif

#if IMGUI_VERSION_NUM >= 18727
    // ## Test undo/redo operations with modifications from callback. (#4947, #4949)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_undo_callback");
    t->SetVarsDataType<StrVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto callback = [](ImGuiInputTextCallbackData* data)
        {
            if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
                data->InsertChars(data->CursorPos, ", world!");
            return 0;
        };

        StrVars& vars = ctx->GetVars<StrVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("Field1", &vars.str, ImGuiInputTextFlags_CallbackHistory, callback);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        StrVars& vars = ctx->GetVars<StrVars>();
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field1");
        ctx->KeyCharsAppend("Hello");
        ctx->KeyPress(ImGuiKey_DownArrow);                      // Trigger modification from callback.
        IM_CHECK_STR_EQ(vars.str.c_str(), "Hello, world!");
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        IM_CHECK_STR_EQ(vars.str.c_str(), "Hello");
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Y);
        IM_CHECK_STR_EQ(vars.str.c_str(), "Hello, world!");
    };
#endif

    // ## Test InputText vs user ownership of data
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_text_ownership");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::LogToBuffer();
        ImGui::InputText("##InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1)); // Remove label to simplify the capture/comparison
        ImStrncpy(vars.Str2, ctx->UiContext->LogBuffer.c_str(), IM_ARRAYSIZE(vars.Str2));
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

        IM_CHECK_STR_EQ(buf_visible, "{ }");
        strcpy(buf_user, "Hello");
        ctx->Yield();
        IM_CHECK_STR_EQ(buf_visible, "{ Hello }");
        ctx->ItemClick("##InputText");
        ctx->KeyCharsAppend("1");
        ctx->Yield();
        IM_CHECK_STR_EQ(buf_user, "Hello1");
        IM_CHECK_STR_EQ(buf_visible, "{ Hello1 }");

        // Because the item is active, it owns the source data, so:
        strcpy(buf_user, "Overwritten");
        ctx->Yield();
        IM_CHECK_STR_EQ(buf_user, "Hello1");
        IM_CHECK_STR_EQ(buf_visible, "{ Hello1 }");

#if IMGUI_VERSION_NUM >= 19018
        // ## Test ImGuiInputTextState::ReloadUserBufXXX functions (#2890)
        strcpy(buf_user, "OverwrittenAgain");
        ImGuiInputTextState* input_state = ImGui::GetInputTextState(ctx->GetID("##InputText"));
        IM_CHECK(input_state != NULL);
        input_state->ReloadUserBufAndSelectAll();
        ctx->Yield();
        IM_CHECK_STR_EQ(buf_user, "OverwrittenAgain");
        IM_CHECK_STR_EQ(buf_visible, "{ OverwrittenAgain }");
#endif

        // Verify reverted value
        ctx->KeyPress(ImGuiKey_Escape);
        IM_CHECK_EQ(ctx->UiContext->ActiveId, (unsigned)0);
#if IMGUI_VERSION_NUM >= 19018
        IM_CHECK_STR_EQ(buf_user, "Hello"); // ReloadUserBufAndSelectAll() shouldn't have overrided revert
#endif

        // At his point the InputTextState->ID should be holding on the last active state,
        // so we verify that InputText() is picking up external changes.
        strcpy(buf_user, "Hello2");
        ctx->Yield();
        IM_CHECK_STR_EQ(buf_user, "Hello2");
        IM_CHECK_STR_EQ(buf_visible, "{ Hello2 }");
    };

    // ## Test that InputText doesn't go havoc when activated via another item
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_id_conflict");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 50, 0.0f));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (vars.Step == 0)
        {
            if (ctx->FrameCount < 50)
                ImGui::Button("Hello");
            else
                ImGui::InputText("Hello", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        }
        if (vars.Step == 1)
        {
            if (vars.Int1 == 0)
            {
                if (ImGui::BeginCombo("Hello", "Previews"))
                {
                    ImGui::Selectable("Dummy");
                    ImGui::EndCombo();
                }
            }
            else
            {
                ImGui::InputInt("Hello", &vars.Int1, 0, 100, ImGuiInputTextFlags_CharsNoBlank);
            }
        }
        if (vars.Step == 2)
        {
            ImGui::InputTextMultiline("Hello", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        // Toggling from Button() to InputText()
        ctx->SetRef("Test Window");
        ctx->ItemHoldForFrames("Hello", 100);
        ctx->ItemClick("Hello");
        ImGuiInputTextState* state = ImGui::GetInputTextState(ctx->GetID("Hello"));
        IM_CHECK(state != NULL);
        IM_CHECK(state->Stb.single_line == 1);

        // Toggling from InputInt() to BeginCombo() (#6304)
#if IMGUI_VERSION_NUM >= 18949
        vars.Step = 1;
        vars.Int1 = 1;
        ctx->Yield();
        ctx->ItemClick("Hello");
        ctx->KeyCharsReplace("0");
        ctx->Yield(2);
#endif

        // Toggling from single to multiline is a little bit ill-defined
        vars.Step = 2;
        ctx->Yield();
        ctx->ItemClick("Hello");
        state = ImGui::GetInputTextState(ctx->GetID("Hello"));
        IM_CHECK(state != NULL);
        IM_CHECK(state->Stb.single_line == 0);
    };

    // ## Test that InputText doesn't append two tab characters if the backend supplies both tab key and character
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_tab_double_insertion");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1), ImGuiInputTextFlags_AllowTabInput);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field");
        ctx->UiContext->IO.AddInputCharacter((ImWchar)'\t');
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_STR_EQ(vars.Str1, "\t");
    };

    // ## Test input clearing action (ESC key) being undoable (#3008).
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_esc_revert");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        ImGui::End();

    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        // FIXME-TESTS: Facilitate usage of variants
        const int test_count = ctx->HasDock ? 2 : 1;
        for (int test_n = 0; test_n < test_count; test_n++)
        {
            ctx->LogDebug("TEST CASE %d", test_n);
            const char* initial_value = (test_n == 0) ? "" : "initial";
            strcpy(vars.Str1, initial_value);
            ctx->SetRef("Test Window");
            ctx->ItemInput("Field");
            ctx->KeyCharsReplace("text");
            IM_CHECK_STR_EQ(vars.Str1, "text");
            ctx->KeyPress(ImGuiKey_Escape);                      // Reset input to initial value.
            IM_CHECK_STR_EQ(vars.Str1, initial_value);
            ctx->ItemInput("Field");
            ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);       // Undo
            IM_CHECK_STR_EQ(vars.Str1, "text");
            ctx->KeyPress(ImGuiKey_Enter);                       // Unfocus otherwise test_n==1 strcpy will fail
        }
    };

#if IMGUI_VERSION_NUM >= 18965
    // ## Test ImGuiInputTextFlags_EscapeClearsAll (#5688)
    // FIXME-TESTS: Possible edge cases when mismatch of internal vs user buffer.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_esc_clear");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        bool ret_val = ImGui::InputText("InputText", vars.Str1, IM_ARRAYSIZE(vars.Str1), ImGuiInputTextFlags_EscapeClearsAll);
        vars.Status.QueryInc(ret_val);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->ItemClick("InputText");
        ctx->KeyChars("H");
        ctx->KeyChars("ello");
        IM_CHECK_GE(vars.Status.RetValue, 2);
        IM_CHECK_STR_EQ(vars.Str1, "Hello");
        vars.Status.Clear();

        // First ESC press clears buffer, notify of change if any
        ctx->KeyPress(ImGuiKey_Escape);
        IM_CHECK_EQ(vars.Status.RetValue, 1);
        IM_CHECK_EQ(vars.Status.Deactivated, 0);
        IM_CHECK_STR_EQ(vars.Str1, "");
        IM_CHECK_EQ(g.ActiveId, ctx->GetID("InputText"));
        vars.Status.Clear();

        // Second ESC deactivate item
        ctx->KeyPress(ImGuiKey_Escape);
        IM_CHECK_EQ(vars.Status.RetValue, 0);
        IM_CHECK_EQ(vars.Status.Deactivated, 1);
        IM_CHECK_STR_EQ(vars.Str1, "");
        IM_CHECK_EQ(g.ActiveId, 0u);
    };
#endif

    // ## Test input text multiline cursor movement: left, up, right, down, origin, end, ctrl+origin, ctrl+end, page up, page down
    // TODO ## Test input text multiline cursor with selection: left, up, right, down, origin, end, ctrl+origin, ctrl+end, page up, page down
    // TODO ## Test input text multiline scroll movement only: ctrl + (left, up, right, down)
    // TODO ## Test input text multiline page up/page down history ?
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_cursor");
    struct InputTextCursorVars { Str str; int Cursor = 0; int LineCount = 10; Str64 Password; };
    t->SetVarsDataType<InputTextCursorVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        InputTextCursorVars& vars = ctx->GetVars<InputTextCursorVars>();

        float height = vars.LineCount * 0.5f * ImGui::GetFontSize();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::InputTextMultiline("Field", &vars.str, ImVec2(300, height), ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGuiInputTextState* state = ImGui::GetInputTextState(ctx->GetID("//Test Window/Field")))
            ImGui::Text("Stb Cursor: %d", state->Stb.cursor);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        InputTextCursorVars& vars = ctx->GetVars<InputTextCursorVars>();
        ctx->SetRef("Test Window");

        vars.str.clear();
        const int char_count_per_line = 10;
        for (int n = 0; n < vars.LineCount; n++)
        {
            for (int c = 0; c < char_count_per_line - 1; c++) // \n is part of our char_count_per_line
                vars.str.appendf("%d", n);
            if (n < vars.LineCount - 1)
                vars.str.append("\n");
        }
        ctx->ItemInput("Field");

        ImGuiInputTextState* state = ImGui::GetInputTextState(ctx->GetID("Field"));
        IM_CHECK(state != NULL);
        ImStb::STB_TexteditState& stb = state->Stb;
        vars.Cursor = stb.cursor;

        const int page_size = (vars.LineCount / 2) - 1;

        const int cursor_pos_begin_of_first_line = 0;
        const int cursor_pos_end_of_first_line = char_count_per_line - 1;
        const int cursor_pos_middle_of_first_line = char_count_per_line / 2;
        const int cursor_pos_end_of_last_line = vars.str.length();
        const int cursor_pos_begin_of_last_line = cursor_pos_end_of_last_line - char_count_per_line + 1;
        //const int cursor_pos_middle_of_last_line = cursor_pos_end_of_last_line - char_count_per_line / 2;
        const int cursor_pos_middle = vars.str.length() / 2;

        auto SetCursorPosition = [&stb](int cursor) { stb.cursor = cursor; stb.has_preferred_x = 0; };

        // Do all the test twice: with no trailing \n, and with.
        for (int i = 0; i < 2; i++)
        {
            bool has_trailing_line_feed = (i == 1);
            if (has_trailing_line_feed)
            {
                SetCursorPosition(cursor_pos_end_of_last_line);
                ctx->KeyCharsAppend("\n");
            }
            int eof = vars.str.length();

            // Begin of File
            SetCursorPosition(0); ctx->KeyPress(ImGuiKey_UpArrow);
            IM_CHECK_EQ(stb.cursor, 0);
            SetCursorPosition(0); ctx->KeyPress(ImGuiKey_LeftArrow);
            IM_CHECK_EQ(stb.cursor, 0);
            SetCursorPosition(0); ctx->KeyPress(ImGuiKey_DownArrow);
            IM_CHECK_EQ(stb.cursor, char_count_per_line);
            SetCursorPosition(0); ctx->KeyPress(ImGuiKey_RightArrow);
            IM_CHECK_EQ(stb.cursor, 1);

            // End of first line
            SetCursorPosition(cursor_pos_end_of_first_line); ctx->KeyPress(ImGuiKey_UpArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_end_of_first_line);
            SetCursorPosition(cursor_pos_end_of_first_line); ctx->KeyPress(ImGuiKey_LeftArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_end_of_first_line - 1);
            SetCursorPosition(cursor_pos_end_of_first_line); ctx->KeyPress(ImGuiKey_DownArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_end_of_first_line + char_count_per_line);
            SetCursorPosition(cursor_pos_end_of_first_line); ctx->KeyPress(ImGuiKey_RightArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_end_of_first_line + 1);

            // Begin of last line
            SetCursorPosition(cursor_pos_begin_of_last_line); ctx->KeyPress(ImGuiKey_UpArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_begin_of_last_line - char_count_per_line);
            SetCursorPosition(cursor_pos_begin_of_last_line); ctx->KeyPress(ImGuiKey_LeftArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_begin_of_last_line - 1);
            SetCursorPosition(cursor_pos_begin_of_last_line); ctx->KeyPress(ImGuiKey_DownArrow);
            IM_CHECK_EQ(stb.cursor, has_trailing_line_feed ? eof : cursor_pos_begin_of_last_line);
            SetCursorPosition(cursor_pos_begin_of_last_line); ctx->KeyPress(ImGuiKey_RightArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_begin_of_last_line + 1);

            // End of last line
            SetCursorPosition(cursor_pos_end_of_last_line); ctx->KeyPress(ImGuiKey_UpArrow);
#if IMGUI_VERSION_NUM >= 18917
            IM_CHECK_EQ(stb.cursor, cursor_pos_end_of_last_line - char_count_per_line); // Issue #6000
#endif
            SetCursorPosition(cursor_pos_end_of_last_line); ctx->KeyPress(ImGuiKey_LeftArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_end_of_last_line - 1);
            SetCursorPosition(cursor_pos_end_of_last_line); ctx->KeyPress(ImGuiKey_DownArrow);
            IM_CHECK_EQ(stb.cursor, has_trailing_line_feed ? eof : cursor_pos_end_of_last_line);
            SetCursorPosition(cursor_pos_end_of_last_line); ctx->KeyPress(ImGuiKey_RightArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_end_of_last_line + (has_trailing_line_feed ? 1 : 0));

            // In the middle of the content
            SetCursorPosition(cursor_pos_middle); ctx->KeyPress(ImGuiKey_UpArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_middle - char_count_per_line);
            SetCursorPosition(cursor_pos_middle); ctx->KeyPress(ImGuiKey_LeftArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_middle - 1);
            SetCursorPosition(cursor_pos_middle); ctx->KeyPress(ImGuiKey_DownArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_middle + char_count_per_line);
            SetCursorPosition(cursor_pos_middle); ctx->KeyPress(ImGuiKey_RightArrow);
            IM_CHECK_EQ(stb.cursor, cursor_pos_middle + 1);

            // Home/End to go to beginning/end of the line
            SetCursorPosition(cursor_pos_middle); ctx->KeyPress(ImGuiKey_Home);
            IM_CHECK_EQ(stb.cursor, ((vars.LineCount / 2) - 1) * char_count_per_line);
            SetCursorPosition(cursor_pos_middle); ctx->KeyPress(ImGuiKey_End);
            IM_CHECK_EQ(stb.cursor, (vars.LineCount / 2) * char_count_per_line - 1);

            // Ctrl+Home/End to go to beginning/end of the text
            SetCursorPosition(cursor_pos_middle); ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Home);
            IM_CHECK_EQ(stb.cursor, 0);
            SetCursorPosition(cursor_pos_middle); ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_End);
            IM_CHECK_EQ(stb.cursor, cursor_pos_end_of_last_line + (has_trailing_line_feed ? 1 : 0));

            // PageUp/PageDown
            SetCursorPosition(cursor_pos_begin_of_first_line); ctx->KeyPress(ImGuiKey_PageDown);
            IM_CHECK_EQ(stb.cursor, cursor_pos_begin_of_first_line + char_count_per_line * page_size);
            ctx->KeyPress(ImGuiKey_PageUp);
            IM_CHECK_EQ(stb.cursor, cursor_pos_begin_of_first_line);

            SetCursorPosition(cursor_pos_middle_of_first_line);
            ctx->KeyPress(ImGuiKey_PageDown);
            IM_CHECK_EQ(stb.cursor, cursor_pos_middle_of_first_line + char_count_per_line * page_size);
            ctx->KeyPress(ImGuiKey_PageDown);
            IM_CHECK_EQ(stb.cursor, cursor_pos_middle_of_first_line + char_count_per_line * page_size * 2);
            ctx->KeyPress(ImGuiKey_PageDown);
            IM_CHECK_EQ(stb.cursor, has_trailing_line_feed ? eof : eof - (char_count_per_line / 2) + 1);

            // We started PageDown from the middle of a line, so even if we're at the end (with X = 0),
            // PageUp should bring us one page up to the middle of the line
            int cursor_pos_begin_current_line = (stb.cursor / char_count_per_line) * char_count_per_line; // Round up cursor position to decimal only
            ctx->KeyPress(ImGuiKey_PageUp);
            IM_CHECK_EQ(stb.cursor, cursor_pos_begin_current_line - (page_size * char_count_per_line) + (char_count_per_line / 2));
                //eof - (char_count_per_line * page_size) + (char_count_per_line / 2) + (has_trailing_line_feed ? 0 : 1));
        }

        // Cursor positioning after new line. Broken line indexing may produce incorrect results in such case.
        ctx->KeyCharsReplaceEnter("foo");
        IM_CHECK_EQ(stb.cursor, 4);

        // Empty buffer
        ctx->KeyCharsReplace("");
        IM_CHECK_EQ(stb.cursor, 0);
        IM_CHECK_EQ(state->CurLenA, 0);
        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK_EQ(stb.cursor, 0);
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(stb.cursor, 0);
        ctx->KeyPress(ImGuiKey_LeftArrow);
        IM_CHECK_EQ(stb.cursor, 0);
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(stb.cursor, 0);

#if IMGUI_VERSION_NUM >= 18991
        // Extra test for #6783, long line trailed with \n, pressing down.
        // This is almost exercised elsewhere but didn't crash with small values until we added bound-check in STB_TEXTEDIT_GETCHAR() too.
        ctx->KeyCharsReplace("Click this text then press down-arrow twice to cause an assert.\n");
        IM_CHECK_EQ(stb.cursor, state->CurLenA);
        ctx->KeyPress(ImGuiKey_DownArrow);
#endif
    };

    // ## Test CTRL+arrow and other word boundaries functions
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_cursor_prevnext_words");
    t->SetVarsDataType<InputTextCursorVars>([](auto* ctx, auto& vars) { vars.str = "Hello world. Foo.bar!!!"; });
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        InputTextCursorVars& vars = ctx->GetVars<InputTextCursorVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::InputTextMultiline("Field", &vars.str);
        ImGuiInputTextState* state = ImGui::GetInputTextState(ctx->GetID("//Test Window/Field"));
        ImGui::Text("CursorPos: %d", state ? state->GetCursorPos() : -1);
        ImGui::Checkbox("ConfigMacOSXBehaviors", &ctx->UiContext->IO.ConfigMacOSXBehaviors);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        InputTextCursorVars& vars = ctx->GetVars<InputTextCursorVars>();
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field");
        ImGuiInputTextState* state = ImGui::GetInputTextState(ctx->GetID("//Test Window/Field"));
        IM_CHECK(state != NULL);
        auto KeyPressAndDebugPrint = [&](ImGuiKeyChord key_chord)
        {
            ctx->KeyPress(key_chord);
            int cursor = state->GetCursorPos();
            ctx->LogDebug("\"%.*s|%s\"", cursor, vars.str.c_str(), vars.str.c_str() + cursor); // Only valid when using ASCII only.
        };

        /*
        // Action   VS(Win)                     Firefox(Win)        XCode(OSX)                  Firefox(OSX)
        //------------------------------------------------------------------------------------------------------
        //         |Hello                      |Hello              |Hello                       ==
        // Ctrl->   Hello |world                ==                  Hello|                      ==
        // Ctrl->   Hello world|.               (skip)              Hello world|.               ==
        // Ctrl->   Hello world. |Foo           ==                  (skip)                      (skip)
        // Ctrl->   Hello world. Foo|.bar       (skip)              Hello world. Foo|.bar       (skip)
        // Ctrl->   Hello world. Foo.|bar       ==                  (skip)                      (skip)
        // Ctrl->   Hello world. Foo.bar|       (skip)              Hello world. Foo.bar|       ==
        // Ctrl->   Hello world. Foo.bar!!!|    ==                  (next line word)            (eol)
        //
        //          Hello world. Foo.bar!!!|    ==                  Hello world. Foo.bar!!!|    ==
        // Ctrl<-   Hello world. Foo.bar|!!!    (skip)              (skip)                      (skip)
        // Ctrl<-   Hello world. Foo.|bar!!!    ==                  Hello world. Foo.|bar!!!    (skip)
        // Ctrl<-   Hello world. Foo|.bar!!!    (skip)              (skip)                      (skip)
        // Ctrl<-   Hello world. |Foo.bar!!!    ==                  Hello world. |Foo.bar!!!    ==
        // Ctrl<-   Hello world|. Foo.bar!!!    (skip)              (skip)                      (skip)
        // Ctrl<-   Hello |world. Foo.bar!!!    ==                  Hello |world. Foo.bar!!!    ==
        // Ctrl<-   |Hello world. Foo.bar!!!    ==                  Hello world. Foo.bar!!!     ==
        //
        // (*) Firefox: tested GitHub, Google, WhatsApp text fields with same results.
        */

        //Hello world. Foo.bar!!!

        for (int os_mode = 0; os_mode < 2; os_mode++)
        {
            g.IO.ConfigMacOSXBehaviors = (os_mode == 0) ? false : true;
#if IMGUI_VERSION_NUM < 18945 // Fixed in #6067
            if (g.IO.ConfigMacOSXBehaviors)
                continue;
#endif
            ctx->Yield(); // for behavior change to update
            const bool is_osx = g.IO.ConfigMacOSXBehaviors;
            const ImGuiKeyChord chord_word_prev = (is_osx ? ImGuiMod_Alt : ImGuiMod_Ctrl) | ImGuiKey_LeftArrow;
            const ImGuiKeyChord chord_word_next = (is_osx ? ImGuiMod_Alt : ImGuiMod_Ctrl) | ImGuiKey_RightArrow;
            ctx->LogDebug("## Testing with io.ConfigMacOSXBehaviors = %d", is_osx);

            // [SET 1]
            ctx->KeyCharsReplace("Hello world. Foo.bar!!!");
            KeyPressAndDebugPrint(ImGuiKey_Home);
            IM_CHECK_EQ(state->GetCursorPos(), 0); // "|Hello "
            if (!is_osx)
            {
                // Windows
                KeyPressAndDebugPrint(chord_word_next);
                IM_CHECK_EQ(state->GetCursorPos(), 6); //  "Hello |"
                KeyPressAndDebugPrint(chord_word_prev);
                IM_CHECK_EQ(state->GetCursorPos(), 0); // "|Hello "
                for (int n = 0; n < 5; n++)
                    KeyPressAndDebugPrint(ImGuiKey_RightArrow);
                IM_CHECK_EQ(state->GetCursorPos(), 5); // "Hello| "
                KeyPressAndDebugPrint(chord_word_next);
                IM_CHECK_EQ(state->GetCursorPos(), 6); //  "Hello |"

                KeyPressAndDebugPrint(chord_word_next);
#if IMGUI_VERSION_NUM >= 18945
                IM_CHECK_EQ(state->GetCursorPos(), 6 + 5);      //  "Hello world|." // VS(Win) does this, GitHub-Web(Win) doesn't.
                KeyPressAndDebugPrint(chord_word_next);
#endif
                IM_CHECK_EQ(state->GetCursorPos(), 6 + 5 + 2);  //  "Hello world. |"

                KeyPressAndDebugPrint(chord_word_next);
#if IMGUI_VERSION_NUM >= 18945
                IM_CHECK_EQ(state->GetCursorPos(), 6 + 5 + 2 + 3);              //  "Hello world. Foo|."    // VS-Win: STOP, GitHubWeb-Win: SKIP
                KeyPressAndDebugPrint(chord_word_next);
                IM_CHECK_EQ(state->GetCursorPos(), 6 + 5 + 2 + 3 + 1);          //  "Hello world. Foo.|"

                KeyPressAndDebugPrint(chord_word_next);
                IM_CHECK_EQ(state->GetCursorPos(), 6 + 5 + 2 + 3 + 1 + 3);      //  "Hello world. Foo.bar|" // VS-Win: STOP, GitHubWeb-Win: SKIP

                KeyPressAndDebugPrint(chord_word_next);
#endif
                IM_CHECK_EQ(state->GetCursorPos(), 6 + 5 + 2 + 3 + 1 + 3 + 3);  //  "Hello world. Foo.bar!!!|"

                ctx->KeyPress(ImGuiKey_End);
                KeyPressAndDebugPrint(chord_word_prev);
#if IMGUI_VERSION_NUM >= 18945
                IM_CHECK_EQ(state->GetCursorPos(), 6 + 5 + 2 + 3 + 1 + 3);  //  "Hello world. Foo.bar|!!!" // VS-Win: STOP, GitHubWeb-Win: SKIP

                KeyPressAndDebugPrint(chord_word_prev);
                IM_CHECK_EQ(state->GetCursorPos(), 6 + 5 + 2 + 3 + 1);      //  "Hello world. Foo.|bar!!!"

                KeyPressAndDebugPrint(chord_word_prev);
                IM_CHECK_EQ(state->GetCursorPos(), 6 + 5 + 2 + 3);          //  "Hello world. Foo|.bar!!!" // VS-Win: STOP, GitHubWeb-Win: SKIP

                KeyPressAndDebugPrint(chord_word_prev);
#endif
                IM_CHECK_EQ(state->GetCursorPos(), 6 + 5 + 2);              //  "Hello world. |Foo.bar!!!"

                KeyPressAndDebugPrint(chord_word_prev);
#if IMGUI_VERSION_NUM >= 18945
                IM_CHECK_EQ(state->GetCursorPos(), 6 + 5);                  //  "Hello world|. Foo.bar!!!" // VS-Win: STOP, GitHubWeb-Win: SKIP
                KeyPressAndDebugPrint(chord_word_prev);
#endif
                IM_CHECK_EQ(state->GetCursorPos(), 6);                      //  "Hello |world. Foo.bar!!!"

                KeyPressAndDebugPrint(chord_word_prev);
                IM_CHECK_EQ(state->GetCursorPos(), 0);                      // "|Hello world. Foo.bar!!!"

                KeyPressAndDebugPrint(chord_word_prev);
                IM_CHECK_EQ(state->GetCursorPos(), 0);                      // "|Hello world. Foo.bar!!!" // (no-op)
            }
            else
            {
                // OSX
                KeyPressAndDebugPrint(chord_word_next);
                IM_CHECK_EQ(state->GetCursorPos(), 5); //  "Hello|"
                KeyPressAndDebugPrint(chord_word_prev);
                IM_CHECK_EQ(state->GetCursorPos(), 0); // "|Hello "
                for (int n = 0; n < 5; n++)
                    KeyPressAndDebugPrint(ImGuiKey_RightArrow);
                IM_CHECK_EQ(state->GetCursorPos(), 5); // "Hello| "

                KeyPressAndDebugPrint(chord_word_next);
                IM_CHECK_EQ(state->GetCursorPos(), 5 + 1 + 5 + 1); // "Hello world.| " // FIXME: Not matching desirable OSX behavior.

                // FIXME-TODO: More tests needed.
            }

            // [SET 2]
            // Delete all, Extra Test with Multiple Spaces
            ctx->KeyCharsReplace("Hello     world.....HELLO");
            KeyPressAndDebugPrint(ImGuiKey_Home);
            IM_CHECK_EQ(state->GetCursorPos(), 0); // "|Hello     World"
            if (!is_osx)
            {
                // Windows
                KeyPressAndDebugPrint(chord_word_next);
                IM_CHECK_EQ(state->GetCursorPos(), 10); // "Hello     |world"
                KeyPressAndDebugPrint(chord_word_next);
#if IMGUI_VERSION_NUM >= 18945
                IM_CHECK_EQ(state->GetCursorPos(), 15); // "Hello     world|"   // VS-Win: STOP, GitHubWeb-Win: SKIP
                KeyPressAndDebugPrint(chord_word_next);
                IM_CHECK_EQ(state->GetCursorPos(), 20); // "Hello     world.....|"
                KeyPressAndDebugPrint(chord_word_next);
#endif
                IM_CHECK_EQ(state->GetCursorPos(), 25); // "Hello     world.....HELLO|"
                KeyPressAndDebugPrint(chord_word_prev);
#if IMGUI_VERSION_NUM >= 18945
                IM_CHECK_EQ(state->GetCursorPos(), 20); // "Hello     world.....|"
                KeyPressAndDebugPrint(chord_word_prev);
                IM_CHECK_EQ(state->GetCursorPos(), 15); // "Hello     world|"   // VS-Win: STOP, GitHubWeb-Win: SKIP
                KeyPressAndDebugPrint(chord_word_prev);
#endif
                IM_CHECK_EQ(state->GetCursorPos(), 10); // "Hello     |world"
                KeyPressAndDebugPrint(chord_word_prev);
                IM_CHECK_EQ(state->GetCursorPos(), 0); // "|Hello     World"
            }
            else
            {
                // FIXME-TODO
            }
        }
    };

    // ## Verify that text selection does not leak spaces in password fields. (#4155)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_password");
    t->SetVarsDataType<InputTextCursorVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        InputTextCursorVars& vars = ctx->GetVars<InputTextCursorVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::InputText("Password", &vars.Password, ImGuiInputTextFlags_Password);
        if (ImGuiInputTextState* state = ImGui::GetInputTextState(ctx->GetID("//Test Window/Password")))
            ImGui::Text("Stb Cursor: %d", state->Stb.cursor);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ctx->ItemClick("Password");
        ctx->KeyCharsAppendEnter("Totally not Password123");
        ctx->ItemDoubleClick("Password");
        ImGuiInputTextState* state = ImGui::GetInputTextState(ctx->GetID("Password"));
        IM_CHECK(state != NULL);
#if IMGUI_VERSION_NUM >= 18825
        IM_CHECK((state->Flags& ImGuiInputTextFlags_Password) != 0); // Verify flags are persistent (#5724)
#endif
        ImStb::STB_TexteditState& stb = state->Stb;
        IM_CHECK_EQ(stb.select_start, 0);
        IM_CHECK_EQ(stb.select_end, 23);
        IM_CHECK_EQ(stb.cursor, 23);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_LeftArrow);
        IM_CHECK_EQ(stb.cursor, 0);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_RightArrow);
        IM_CHECK_EQ(stb.cursor, 23);
    };

    // ## Test that scrolling preserve cursor and selection
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_scrolling");
    t->SetVarsDataType<InputTextCursorVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        InputTextCursorVars& vars = ctx->GetVars<InputTextCursorVars>();

        float height = 5 * ImGui::GetFontSize();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::InputTextMultiline("Field", &vars.str, ImVec2(300, height), ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGuiInputTextState* state = ImGui::GetInputTextState(ctx->GetID("//Test Window/Field")))
            ImGui::Text("CursorPos: %d", state->GetCursorPos());
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ctx->ItemInput("Field");
        for (int n = 0; n < 10; n++)
            ctx->KeyCharsAppend(Str30f("Line %d\n", n).c_str());
#if IMGUI_VERSION_NUM >= 18991
        ctx->KeyPress(ImGuiKey_DownArrow); // Extra test for #6783
#endif
        ctx->KeyDown(ImGuiMod_Shift);
        ctx->KeyPress(ImGuiKey_UpArrow);
        ctx->KeyUp(ImGuiMod_Shift);

        ImGuiWindow* child_window = ctx->WindowInfo("//Test Window/Field").Window;
        IM_CHECK(child_window != NULL);
        const int selection_len = (int)strlen("Line 9\n");

        for (int n = 0; n < 3; n++)
        {
            ImGuiInputTextState* state = ImGui::GetInputTextState(ctx->GetID("Field"));
            IM_CHECK(state != NULL);
            IM_CHECK(state->HasSelection());
            IM_CHECK(ImAbs(state->Stb.select_end - state->Stb.select_start) == selection_len);
            IM_CHECK(state->Stb.select_end == state->Stb.cursor);
            IM_CHECK(state->Stb.cursor == state->CurLenW - selection_len);
            if (n == 1)
                ctx->ScrollToBottom(child_window->ID);
            else
                ctx->ScrollToTop(child_window->ID);
        }

        ImGuiInputTextState* state = ImGui::GetInputTextState(ctx->GetID("Field"));
        IM_CHECK(state != NULL);
        IM_CHECK(child_window->Scroll.y == 0.0f);
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(state->Stb.cursor, state->CurLenW);
        IM_CHECK_EQ(child_window->Scroll.y, child_window->ScrollMax.y);
    };

    // ## Test named filters
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_filters");
    struct InputTextFilterVars { Str64 Default; Str64 Decimal; Str64 Scientific;  Str64 Hex; Str64 Uppercase; Str64 NoBlank; Str64 Custom; };
    t->SetVarsDataType<InputTextFilterVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        InputTextFilterVars& vars = ctx->GetVars<InputTextFilterVars>();
        struct TextFilters
        {
            // Return 0 (pass) if the character is 'i' or 'm' or 'g' or 'u' or 'i'
            static int FilterImGuiLetters(ImGuiInputTextCallbackData* data)
            {
                if (data->EventChar < 256 && strchr("imgui", (char)data->EventChar))
                    return 0;
                return 1;
            }
        };

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("default", &vars.Default);
        ImGui::InputText("decimal", &vars.Decimal, ImGuiInputTextFlags_CharsDecimal);
        ImGui::InputText("scientific", &vars.Scientific, ImGuiInputTextFlags_CharsScientific);
        ImGui::InputText("hexadecimal", &vars.Hex, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
        ImGui::InputText("uppercase", &vars.Uppercase, ImGuiInputTextFlags_CharsUppercase);
        ImGui::InputText("no blank", &vars.NoBlank, ImGuiInputTextFlags_CharsNoBlank);
        ImGui::InputText("\"imgui\" letters", &vars.Custom, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterImGuiLetters);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        InputTextFilterVars& vars = ctx->GetVars<InputTextFilterVars>();
        const char* input_text = "Some fancy Input Text in 0.. 1.. 2.. 3!";
        ctx->SetRef("Test Window");
        ctx->ItemClick("default");
        ctx->KeyCharsAppendEnter(input_text);
        IM_CHECK_STR_EQ(vars.Default.c_str(), input_text);

        ctx->ItemClick("decimal");
        ctx->KeyCharsAppendEnter(input_text);
        IM_CHECK_STR_EQ(vars.Decimal.c_str(), "0..1..2..3");

        // , are replaced by . in decimal and scientific fields (#6719, #2278)
#if IMGUI_VERSION_NUM >= 18983
        ctx->ItemClick("decimal");
        ctx->KeyCharsReplaceEnter(".,.,");
        IM_CHECK_STR_EQ(vars.Decimal.c_str(), "....");
#endif

        ctx->ItemClick("scientific");
        ctx->KeyCharsAppendEnter(input_text);
        IM_CHECK_STR_EQ(vars.Scientific.c_str(), "ee0..1..2..3");

        ctx->ItemClick("hexadecimal");
        ctx->KeyCharsAppendEnter(input_text);
        IM_CHECK_STR_EQ(vars.Hex.c_str(), "EFACE0123");

        ctx->ItemClick("uppercase");
        ctx->KeyCharsAppendEnter(input_text);
        IM_CHECK_STR_EQ(vars.Uppercase.c_str(), "SOME FANCY INPUT TEXT IN 0.. 1.. 2.. 3!");

        ctx->ItemClick("no blank");
        ctx->KeyCharsAppendEnter(input_text);
        IM_CHECK_STR_EQ(vars.NoBlank.c_str(), "SomefancyInputTextin0..1..2..3!");

        ctx->ItemClick("\"imgui\" letters");
        ctx->KeyCharsAppendEnter(input_text);
        IM_CHECK_STR_EQ(vars.Custom.c_str(), "mui");
    };

    // ## Test completion and history
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_callback_misc");
    struct InputTextCallbackHistoryVars { Str CompletionBuffer; Str HistoryBuffer; Str EditBuffer; int EditCount = 0; };
    t->SetVarsDataType<InputTextCallbackHistoryVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        struct Funcs
        {
            static int MyCallback(ImGuiInputTextCallbackData* data)
            {
                if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
                {
                    // Append lots of text to avoid relying on existing capacity
                    data->InsertChars(data->CursorPos, ".......................................");
                }
                else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
                {
                    if (data->EventKey == ImGuiKey_UpArrow)
                    {
                        data->DeleteChars(0, data->BufTextLen);
                        data->InsertChars(0, "Pressed Up!");
                        data->SelectAll();
                    }
                    else if (data->EventKey == ImGuiKey_DownArrow)
                    {
                        data->DeleteChars(0, data->BufTextLen);
                        data->InsertChars(0, "Pressed Down!");
                        data->SelectAll();
                    }
                }
                else if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit)
                {
                    // Toggle casing of first character
                    char c = data->Buf[0];
                    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) data->Buf[0] ^= 32;
                    data->BufDirty = true;

                    // Increment a counter
                    int* p_edit_count = (int*)data->UserData;
                    *p_edit_count = *p_edit_count + 1;
                }
                return 0;
            }
        };

        InputTextCallbackHistoryVars& vars = ctx->GetVars<InputTextCallbackHistoryVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::InputText("Completion", &vars.CompletionBuffer, ImGuiInputTextFlags_CallbackCompletion, Funcs::MyCallback);
        ImGui::InputText("History", &vars.HistoryBuffer, ImGuiInputTextFlags_CallbackHistory, Funcs::MyCallback);
        ImGui::InputText("Edit", &vars.EditBuffer, ImGuiInputTextFlags_CallbackEdit, Funcs::MyCallback, (void*)&vars.EditCount);
        ImGui::SameLine(); ImGui::Text("(%d)", vars.EditCount);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        InputTextCallbackHistoryVars& vars = ctx->GetVars<InputTextCallbackHistoryVars>();

        ctx->SetRef("Test Window");
        ctx->ItemClick("Completion");
        ctx->KeyCharsAppend("Hello World");
        IM_CHECK_STR_EQ(vars.CompletionBuffer.c_str(), "Hello World");
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_STR_EQ(vars.CompletionBuffer.c_str(), "Hello World.......................................");

        // FIXME: Not testing History callback :)
        ctx->ItemClick("History");
        ctx->KeyCharsAppend("ABCDEF");
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        IM_CHECK_STR_EQ(vars.HistoryBuffer.c_str(), "ABCDE");
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Z);
        IM_CHECK_STR_EQ(vars.HistoryBuffer.c_str(), "ABC");
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Y);
        IM_CHECK_STR_EQ(vars.HistoryBuffer.c_str(), "ABCD");
        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK_STR_EQ(vars.HistoryBuffer.c_str(), "Pressed Up!");
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_STR_EQ(vars.HistoryBuffer.c_str(), "Pressed Down!");

        ctx->ItemClick("Edit");
        IM_CHECK_STR_EQ(vars.EditBuffer.c_str(), "");
        IM_CHECK_EQ(vars.EditCount, 0);
        ctx->KeyCharsAppend("h");
        IM_CHECK_STR_EQ(vars.EditBuffer.c_str(), "H");
        IM_CHECK_EQ(vars.EditCount, 1);
        ctx->KeyCharsAppend("e");
        IM_CHECK_STR_EQ(vars.EditBuffer.c_str(), "he");
        IM_CHECK_EQ(vars.EditCount, 2);
        ctx->KeyCharsAppend("llo");
        IM_CHECK_STR_EQ(vars.EditBuffer.c_str(), "Hello");
        IM_CHECK_LE(vars.EditCount, (ctx->EngineIO->ConfigRunSpeed == ImGuiTestRunSpeed_Fast) ? 3 : 5); // If running fast, "llo" will be considered as one edit only
    };

    // ## Test character replacement in callback (inspired by https://github.com/ocornut/imgui/pull/3587)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_callback_replace");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        auto callback = [](ImGuiInputTextCallbackData* data)
        {
            if (data->CursorPos >= 3 && strcmp(data->Buf + data->CursorPos - 3, "abc") == 0)
            {
                data->DeleteChars(data->CursorPos - 3, 3);
                data->InsertChars(data->CursorPos, "\xE5\xA5\xBD"); // HAO
                data->SelectionStart = data->CursorPos - 3;
                data->SelectionEnd = data->CursorPos;
                return 1;
            }
            return 0;
        };
        ImGui::InputText("Hello", vars.Str1, IM_ARRAYSIZE(vars.Str1), ImGuiInputTextFlags_CallbackAlways, callback);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ctx->ItemInput("Hello");
        ImGuiInputTextState* state = &ctx->UiContext->InputTextState;
        IM_CHECK(state && state->ID == ctx->GetID("Hello"));
        ctx->KeyCharsAppend("ab");
        IM_CHECK(state->CurLenA == 2);
        IM_CHECK(state->CurLenW == 2);
        IM_CHECK(strcmp(state->TextA.Data, "ab") == 0);
        IM_CHECK(state->Stb.cursor == 2);
        ctx->KeyCharsAppend("c");
        IM_CHECK(state->CurLenA == 3);
        IM_CHECK(state->CurLenW == 1);
        IM_CHECK(strcmp(state->TextA.Data, "\xE5\xA5\xBD") == 0);
        IM_CHECK(state->TextW.Data[0] == 0x597D);
        IM_CHECK(state->TextW.Data[1] == 0);
        IM_CHECK(state->Stb.cursor == 1);
        IM_CHECK(state->Stb.select_start == 0 && state->Stb.select_end == 1);
    };

    // ## Test resize callback (#3009, #2006, #1443, #1008)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_callback_resize");
    t->SetVarsDataType<StrVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        StrVars& vars = ctx->GetVars<StrVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::InputText("Field1", &vars.str, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            IM_CHECK_EQ(vars.str.capacity(), 4 + 5 + 1);
            IM_CHECK_STR_EQ(vars.str.c_str(), "abcdhello");
        }
        Str str_local_unsaved = "abcd";
        if (ImGui::InputText("Field2", &str_local_unsaved, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            IM_CHECK_EQ(str_local_unsaved.capacity(), 4 + 5 + 1);
            IM_CHECK_STR_EQ(str_local_unsaved.c_str(), "abcdhello");
        }
        ImGui::End();

    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        StrVars& vars = ctx->GetVars<StrVars>();
        vars.str.set("abcd");
        IM_CHECK_EQ(vars.str.capacity(), 4 + 1);
        ctx->SetRef("Test Window");
        ctx->ItemInput("Field1");
        ctx->KeyCharsAppendEnter("hello");
        ctx->ItemInput("Field2");
        ctx->KeyCharsAppendEnter("hello");
    };

    // ## Test resize callback being triggered from within callback (#4784)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_callback_resize2");
    t->SetVarsDataType<StrVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto callback = [](ImGuiInputTextCallbackData* data)
        {
            Str* str = (Str*)data->UserData;
            if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
            {
                data->InsertChars(0, "foo");
            }
            else if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
            {
                IM_ASSERT(data->Buf == str->c_str());
                str->reserve(data->BufTextLen + 1);
                data->Buf = (char*)str->c_str();
            }
            return 0;
        };

        StrVars& vars = ctx->GetVars<StrVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("Field1", vars.str.c_str(), vars.str.capacity(), ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackResize, callback, (void*)&vars.str);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        StrVars& vars = ctx->GetVars<StrVars>();
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field1");
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_STR_EQ(vars.str.c_str(), "foo");
    };

    // ## Test for Nav interference
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_nav");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImVec2 sz(50, 0);
        ImGui::Button("UL", sz); ImGui::SameLine();
        ImGui::Button("U", sz); ImGui::SameLine();
        ImGui::Button("UR", sz);
        ImGui::Button("L", sz); ImGui::SameLine();
        ImGui::SetNextItemWidth(sz.x);
        ImGui::InputText("##Field", vars.Str1, IM_ARRAYSIZE(vars.Str1), ImGuiInputTextFlags_AllowTabInput);
        ImGui::SameLine();
        ImGui::Button("R", sz);
        ImGui::Button("DL", sz); ImGui::SameLine();
        ImGui::Button("D", sz); ImGui::SameLine();
        ImGui::Button("DR", sz);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ctx->ItemClick("##Field");
        ctx->KeyPress(ImGuiKey_LeftArrow);
        IM_CHECK_EQ(ctx->UiContext->NavId, ctx->GetID("##Field"));
        ctx->KeyPress(ImGuiKey_RightArrow);
        IM_CHECK_EQ(ctx->UiContext->NavId, ctx->GetID("##Field"));
        ctx->KeyPress(ImGuiKey_UpArrow);
        IM_CHECK_EQ(ctx->UiContext->NavId, ctx->GetID("U"));
        ctx->KeyPress(ImGuiKey_DownArrow);
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(ctx->UiContext->NavId, ctx->GetID("D"));
    };

    // ## Test InputText widget with user a buffer on stack, reading/writing past end of buffer.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_temp_buffer");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        char buf[10] = "foo\0xxxxx";
        if (ImGui::InputText("Field", buf, 7, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            IM_CHECK_STR_EQ(buf, "foobar");
            IM_CHECK_STR_EQ(buf + 7, "xx"); // Buffer padding not overwritten
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field");
        ctx->KeyCharsAppendEnter("barrr");
    };

    // ## Test InputText clipboard functions.
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_clipboard");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputText("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        char* text = vars.Str1;
        const char* clipboard_text = ImGui::GetClipboardText();
        IM_CHECK_STR_EQ(clipboard_text, "");
        ctx->SetRef("Test Window");

        for (int variant = 0; variant < 2; variant++)
        {
            // State reset.
            ImGui::ClearActiveID();
            strcpy(text, "Hello, world!");

            // Copying without selection.
            ctx->ItemClick("Field");
            if (variant == 0)
                ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_C);
            else
                ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Insert);
            clipboard_text = ImGui::GetClipboardText();
            IM_CHECK_STR_EQ(clipboard_text, "Hello, world!");

            // Copying with selection.
            ctx->ItemClick("Field");
            ctx->KeyPress(ImGuiKey_Home);
            for (int i = 0; i < 5; i++) // Seek to and select first word
                ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_RightArrow);
            if (variant == 0)
                ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_C);
            else
                ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_Insert);
            clipboard_text = ImGui::GetClipboardText();
            IM_CHECK_STR_EQ(clipboard_text, "Hello");

            // Cut a selection.
            ctx->ItemClick("Field");
            ctx->KeyPress(ImGuiKey_Home);
            for (int i = 0; i < 5; i++) // Seek to and select first word
                ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_RightArrow);
            if (variant == 0)
                ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_X);
            else
                ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_Delete);
            clipboard_text = ImGui::GetClipboardText();
            IM_CHECK_STR_EQ(clipboard_text, "Hello");
            IM_CHECK_STR_EQ(text, ", world!");

            // Paste over selection.
            ctx->ItemClick("Field");
            ImGui::SetClipboardText("h\xc9\x99\xcb\x88l\xc5\x8d");  // həˈlō
            ctx->KeyPress(ImGuiKey_Home);
            if (variant == 0)
                ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_V);
            else
                ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_Insert);
            IM_CHECK_STR_EQ(text, "h\xc9\x99\xcb\x88l\xc5\x8d, world!");
        }
    };

    // ## Test for IsItemHovered() on InputTextMultiline() (#1370, #3851)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_hover");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputTextMultiline("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1));
#if IMGUI_VERSION_NUM >= 18511
        IM_CHECK_EQ(ImGui::GetItemID(), ImGui::GetID("Field"));
#endif
        vars.Bool1 = ImGui::IsItemHovered();
        ImGui::Text("hovered: %d", vars.Bool1);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->MouseMove("Field");
        IM_CHECK(vars.Bool1 == true);
    };

    // ## Test for IsItemXXX() calls on InputTextMultiline()
#if IMGUI_VERSION_NUM >= 18512
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_multiline_status");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputTextMultiline("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        //IM_CHECK_EQ(ImGui::GetItemID(), ImGui::GetID("Field"));
        vars.Status.QuerySet();
        ImGui::Text("IsItemActive: %d", vars.Status.Active);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        IM_CHECK(vars.Bool1 == false);
        ctx->ItemClick("Field");
        ImGuiID input_id = ctx->GetID("Field");
        IM_CHECK_EQ(g.ActiveId, input_id);
        IM_CHECK(vars.Status.Active == 1);
        ctx->KeyCharsReplace("1\n2\n3\n4\n5\n6\n\7\n8\n9\n10\n11\n12\n13\n14\n15\n");
        ImGuiWindow* window = ctx->WindowInfo("Field").Window;
        IM_CHECK(window != NULL);
        ImGuiID scrollbar_id = ImGui::GetWindowScrollbarID(window, ImGuiAxis_Y);
        ctx->MouseMove(scrollbar_id);
        ctx->MouseDown(ImGuiMouseButton_Left);
        IM_CHECK_EQ(g.ActiveId, scrollbar_id);
        IM_CHECK(vars.Status.Active == 1);
        ctx->MouseUp(ImGuiMouseButton_Left);
        IM_CHECK_EQ(g.ActiveId, input_id);
        IM_CHECK(vars.Status.Active == 1);
    };
#endif

#if IMGUI_VERSION_NUM >= 18992
    // ## Test for Enter key in InputTextMultiline() used for both entering child and input
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_multiline_enter");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Button("Above");
        ImGui::InputTextMultiline("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1));
        ImGui::Button("Below");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ImGuiID input_id = ctx->GetID("Field");
        ctx->ItemClick("Above");
        ctx->KeyPress(ImGuiKey_DownArrow);
        IM_CHECK_EQ(g.ActiveId, 0u);
        ctx->KeyPress(ImGuiKey_Enter);
        IM_CHECK_EQ(g.ActiveId, input_id);
        ctx->KeyChars("Hello");
        ctx->KeyPress(ImGuiKey_Enter);
        ctx->KeyChars("World");
        IM_CHECK_STR_EQ(vars.Str1, "Hello\nWorld");
        ctx->KeyPress(ImGuiKey_Enter | ImGuiMod_Ctrl);
        ctx->Yield(2);
        IM_CHECK_EQ(g.ActiveId, 0u);
        IM_CHECK_EQ(g.NavId, input_id);

        ctx->KeyPress(ImGuiKey_Tab);
        ctx->KeyPress(ImGuiKey_Tab | ImGuiMod_Shift);
        IM_CHECK_EQ(g.ActiveId, input_id);
        ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_A);
        ctx->KeyPress(ImGuiKey_Delete);
        IM_CHECK_STR_EQ(vars.Str1, "");
        ctx->KeyChars("Hello");
        ctx->KeyPress(ImGuiKey_Enter);
        ctx->KeyChars("World");
        IM_CHECK_STR_EQ(vars.Str1, "Hello\nWorld");
    };
#endif

#if IMGUI_VERSION_NUM >= 19002
    // ## Test nav Tabbing and Tab character InputTextMultiline()
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_multiline_tab");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Button("Above");
        ImGuiInputTextFlags input_text_flags = vars.Bool1 ? ImGuiInputTextFlags_AllowTabInput : ImGuiInputTextFlags_None;
        ImGui::InputTextMultiline("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1), ImVec2(0,0), input_text_flags);
        ImGui::Button("Below");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ImGuiID input_id = ctx->GetID("Field");

        vars.Bool1 = false; // ImGuiInputTextFlags_None
        ctx->ItemClick("Above");
        ctx->KeyPress(ImGuiKey_Tab); // Highlight Appears
        ctx->KeyPress(ImGuiKey_Tab);
        ctx->Yield();
        IM_CHECK_EQ(g.ActiveId, input_id);
        IM_CHECK_EQ(g.NavId, input_id);
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Below"));
        IM_CHECK_EQ(g.ActiveId, 0u);

        vars.Bool1 = true; // ImGuiInputTextFlags_None
        ctx->ItemClick("Above");
        ctx->KeyPress(ImGuiKey_Tab); // Highlight Appears
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavId, input_id);
        IM_CHECK_EQ(g.ActiveId, 0u); // NOT activated
        ctx->KeyPress(ImGuiKey_Tab);
        IM_CHECK_EQ(g.NavId, ctx->GetID("Below"));
        IM_CHECK_EQ(g.ActiveId, 0u);
    };
#endif

    // ## Test handling of Tab/Enter/Space keys events also emitting text events. (#2467, #1336)
    // Backends are inconsistent in behavior: some don't send a text event for Tab and Enter (but still send it for Space)
#if IMGUI_VERSION_NUM >= 18711
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_special_key_chars");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputTextMultiline("Field", vars.Str1, IM_ARRAYSIZE(vars.Str1), ImVec2(), ImGuiInputTextFlags_AllowTabInput);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GenericVars;

        char* field_text = vars.Str1;
        ctx->SetRef("Test Window");
        ctx->ItemClick("Field");
        ctx->RunFlags |= ImGuiTestRunFlags_EnableRawInputs; // Disable TestEngine submitting inputs events
        ctx->Yield();

        g.IO.AddKeyEvent(ImGuiKey_Tab, true);
        g.IO.AddInputCharacter('\t');
        ctx->Yield();
        g.IO.AddKeyEvent(ImGuiKey_Tab, false);
        ctx->Yield();
        IM_CHECK_STR_EQ(field_text, "\t");
        g.IO.AddKeyEvent(ImGuiKey_Backspace, true);
        ctx->Yield();
        g.IO.AddKeyEvent(ImGuiKey_Backspace, false);
        ctx->Yield();

        g.IO.AddKeyEvent(ImGuiKey_Enter, true);
        g.IO.AddInputCharacter('\r');
        ctx->Yield();
        g.IO.AddKeyEvent(ImGuiKey_Enter, false);
        ctx->Yield();
        IM_CHECK_STR_EQ(field_text, "\n");
        g.IO.AddKeyEvent(ImGuiKey_Backspace, true);
        ctx->Yield();
        g.IO.AddKeyEvent(ImGuiKey_Backspace, false);
        ctx->Yield();

        g.IO.AddKeyEvent(ImGuiKey_Space, true);
        g.IO.AddInputCharacter(' ');
        ctx->Yield();
        g.IO.AddKeyEvent(ImGuiKey_Space, false);
        ctx->Yield();
        IM_CHECK_STR_EQ(field_text, " ");
    };
#endif

    // ## Test application of value on DeactivateAfterEdit frame (#4714)
    t = IM_REGISTER_TEST(e, "widgets", "widgets_inputtext_deactivate_apply");
    struct InputTextDeactivateVars
    {
        bool UseTempVar;
        ImVec4 Value;
        int ActivatedFrame = -1, ActivatedField = -1;
        int EditedRetFrame = -1, EditedRetField = -1;
        int EditedQueryFrame = -1, EditedQueryField = -1;
        int DeactivatedFrame = -1, DeactivatedField = -1;
    };
    t->SetVarsDataType<InputTextDeactivateVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<InputTextDeactivateVars>();

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Checkbox("Use Temp Var", &vars.UseTempVar);

        ImVec4 temp_var = vars.Value;
        ImVec4* p = vars.UseTempVar ? &temp_var : &vars.Value;

        if (ImGui::InputFloat("x", &p->x, 0, 0, "%.3f", 0)) { vars.EditedRetFrame = ImGui::GetFrameCount(); vars.EditedRetField = 0; }
        if (ImGui::IsItemEdited()) { vars.EditedQueryFrame = ImGui::GetFrameCount(); vars.EditedQueryField = 0; }
        if (ImGui::IsItemActivated()) { vars.ActivatedFrame = ImGui::GetFrameCount(); vars.ActivatedField = 0; }
        if (ImGui::IsItemDeactivatedAfterEdit()) { vars.DeactivatedFrame = ImGui::GetFrameCount(); vars.DeactivatedField = 0; }
        if (ImGui::IsItemDeactivatedAfterEdit() && vars.UseTempVar)
            vars.Value = temp_var;

        if (ImGui::InputFloat("y", &p->y, 0, 0, "%.3f", 0)) { vars.EditedRetFrame = ImGui::GetFrameCount(); vars.EditedRetField = 1; }
        if (ImGui::IsItemEdited()) { vars.EditedQueryFrame = ImGui::GetFrameCount(); vars.EditedQueryField = 1; }
        if (ImGui::IsItemActivated()) { vars.ActivatedFrame = ImGui::GetFrameCount(); vars.ActivatedField = 1; }
        if (ImGui::IsItemDeactivatedAfterEdit()) { vars.DeactivatedFrame = ImGui::GetFrameCount(); vars.DeactivatedField = 1; }
        if (ImGui::IsItemDeactivatedAfterEdit() && vars.UseTempVar)
            vars.Value = temp_var;

        if (ImGui::InputFloat("z", &p->z, 0, 0, "%.3f", 0)) { vars.EditedRetFrame = ImGui::GetFrameCount(); vars.EditedRetField = 2; }
        if (ImGui::IsItemEdited()) { vars.EditedQueryFrame = ImGui::GetFrameCount(); vars.EditedQueryField = 2; }
        if (ImGui::IsItemActivated()) { vars.ActivatedFrame = ImGui::GetFrameCount(); vars.ActivatedField = 2; }
        if (ImGui::IsItemDeactivatedAfterEdit()) { vars.DeactivatedFrame = ImGui::GetFrameCount(); vars.DeactivatedField = 2; }
        if (ImGui::IsItemDeactivatedAfterEdit() && vars.UseTempVar)
            vars.Value = temp_var;

        ImGui::Text("Value %.3f %.3f %.3f %.3f", vars.Value.x, vars.Value.y, vars.Value.z, vars.Value.w);
        ImGui::Text("temp_var %.3f %.3f %.3f %.3f", temp_var.x, temp_var.y, temp_var.z, temp_var.w);
        ImGui::Text("Activated frame %d, field %d", vars.ActivatedFrame, vars.ActivatedField);
        ImGui::Text("Edited (ret) frame %d, field %d", vars.EditedRetFrame, vars.EditedRetField);
        ImGui::Text("Edited (query) frame %d, field %d", vars.EditedQueryFrame, vars.EditedQueryField);
        ImGui::Text("DeactivatedAfterEdit frame %d, field %d", vars.DeactivatedFrame, vars.DeactivatedField);

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        //ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GetVars<InputTextDeactivateVars>();

        ctx->SetRef("Test Window");

        for (int step = 0; step < 3; step++)
        {
            ctx->LogDebug("## Step %d", step);

            vars.UseTempVar = (step > 0);
            if (step == 0)
                ctx->WindowResize("", ImVec2(300, ImGui::GetFrameHeight() * 20));

            // For step 2 we test an edge case where deactivated InputText() is immediately clipped out.
            // FIXME-TESTS: Since this is solved by ItemAdd() clipping rules, it's possible a similar issue can be recreated with clipper.
            if (step == 2)
                ctx->WindowResize("", ImVec2(300, ImGui::GetFrameHeight()));

            vars.Value = ImVec4();
            ctx->ItemClick("y");
            ctx->KeyCharsReplace("123.0"); // Input value but don't press enter
            if (vars.UseTempVar == false)
                IM_CHECK_EQ(vars.Value.y, 123.0f);
            ctx->ItemClick("z"); // Click Next item
            IM_CHECK_EQ(vars.Value.y, 123.0f);
            IM_CHECK(vars.DeactivatedField == 1);
            ctx->KeyPress(ImGuiKey_Escape);

            vars.Value = ImVec4();
            ctx->ItemClick("y");
            ctx->KeyCharsReplace("123.0"); // Input value but don't press enter
            ctx->ItemClick("x"); // Click Previous item
#if IMGUI_VERSION_NUM < 18942
            if (step == 0)
#endif
            IM_CHECK_EQ(vars.Value.y, 123.0f);
            IM_CHECK(vars.DeactivatedField == 1);
            ctx->KeyPress(ImGuiKey_Escape);

            vars.Value = ImVec4();
            ctx->ItemClick("y");
            ctx->KeyCharsReplace("123.0"); // Input value but don't press enter
            ctx->KeyPress(ImGuiKey_Tab); // Tab to Next Item
#if IMGUI_VERSION_NUM < 18942
            if (step == 0)
#endif
            IM_CHECK_EQ(vars.Value.y, 123.0f);
            IM_CHECK(vars.DeactivatedField == 1);
            ctx->KeyPress(ImGuiKey_Escape);

            vars.Value = ImVec4();
            ctx->ItemClick("y");
            ctx->KeyCharsReplace("123.0"); // Input value but don't press enter
            ctx->KeyPress(ImGuiMod_Shift | ImGuiKey_Tab); // Tab to previous item
#if IMGUI_VERSION_NUM < 18942
            if (step == 0)
#endif
            IM_CHECK_EQ(vars.Value.y, 123.0f);
            IM_CHECK(vars.DeactivatedField == 1);
            ctx->KeyPress(ImGuiKey_Escape);
        }
    };
}
