// dear imgui
// (tests: tables, columns)

#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_test_suite.h"
#include "imgui_test_engine/imgui_te_engine.h"      // IM_REGISTER_TEST()
#include "imgui_test_engine/imgui_te_context.h"
#include "imgui_test_engine/imgui_te_utils.h"       // TableXXX hepers, FindFontByName(), etc.
#include "imgui_test_engine/thirdparty/Str/Str.h"

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
#endif

//-------------------------------------------------------------------------
// Ideas/Specs for future tests
// It is important we take the habit to write those down.
// - Even if we don't implement the test right away: they allow us to remember edge cases and interesting things to test.
// - Even if they will be hard to actually implement/automate, they still allow us to manually check things.
//-------------------------------------------------------------------------
// TODO: Tests: Tables: shouldn't be able to hover resize line under frozen section (#3678)
// TODO: Tests: Tables: Test that stuck resize is non-destructive
// TODO: Tests: Tables: test the RefScale change path (change font, verify that fixed columns are scaled accordingly)
// TODO: Tests: Tables: Test late 'table->ColumnsAutoFitWidth' update.
// TODO: Tests: Tables: explicit outer-size + non-scrolling + double tables with SameLine()
// TODO: Tests: Tables: outer_size.x == 0.0f (~NoHostExtendX) + sameline, NoHostExtendX tests auto-fit
// TODO: Tests: Tables: test that clipped table doesn't emit any vertices/draw calls (amend "table_draw_calls")
// TODO: Tests: Tables: test that clicking on a column and dragging up (to remove drag threshold) doesn't change its width, with all 4 borders combination
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// Tests: Tables
//-------------------------------------------------------------------------

struct TableTestingVars
{
    ImGuiTableFlags         TableFlags = ImGuiTableFlags_None;
    ImVec2                  OuterSize = ImVec2(-FLT_MIN, 0.0f);
    ImGuiWindowFlags        WindowFlags = ImGuiWindowFlags_None;
    ImVec2                  WindowSize = ImVec2(0.0f, 0.0f);
    ImVec2                  ItemSize = ImVec2(0.0f, 0.0f);
    int                     ColumnsCount = 3;
    ImGuiTableColumnFlags   ColumnFlags[6] = {};
    bool                    DebugShowBounds = true;

    ImRect                  OutTableItemRect;
    ImVec2                  OutTableLayoutSize;
    ImVec2                  OutOuterCursorMaxPosOnEndTable;
    ImVec2                  OutOuterIdealMaxPosOnEndTable;
    bool                    OutTableIsItemHovered;

    int                     Step = 0;
};

static void HelperDrawAndFillBounds(TableTestingVars* vars)
{
    ImGuiWindow* outer_window = ImGui::GetCurrentWindow();

    const ImU32 COL_CURSOR_MAX_POS = IM_COL32(255, 0, 255, 200);
    const ImU32 COL_IDEAL_MAX_POS = IM_COL32(0, 255, 0, 200);
    const ImU32 COL_ITEM_RECT = IM_COL32(255, 255, 0, 255);
    //const ImU32 COL_ROW_BG = IM_COL32(0, 255, 0, 20);

    // Display item rect, output cursor position and last line position
    vars->OutTableIsItemHovered = ImGui::IsItemHovered();
    vars->OutTableItemRect.Min = ImGui::GetItemRectMin();
    vars->OutTableItemRect.Max = ImGui::GetItemRectMax();
    vars->OutTableLayoutSize = vars->OutTableItemRect.GetSize();
    vars->OutOuterCursorMaxPosOnEndTable = outer_window->DC.CursorMaxPos;
    vars->OutOuterIdealMaxPosOnEndTable = ImMax(outer_window->DC.CursorMaxPos, outer_window->DC.IdealMaxPos);

    if (vars->DebugShowBounds)
    {
        ImGui::GetForegroundDrawList()->AddRect(ImGui::GetItemRectMin(), vars->OutOuterIdealMaxPosOnEndTable, COL_IDEAL_MAX_POS, 0.0f, 0, 7.0f);
        ImGui::GetForegroundDrawList()->AddRect(ImGui::GetItemRectMin(), vars->OutOuterCursorMaxPosOnEndTable, COL_CURSOR_MAX_POS, 0.0f, 0, 4.0f);
        ImGui::GetForegroundDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), COL_ITEM_RECT, 0.0f, 0, 1.0f);
        ImGui::GetForegroundDrawList()->AddCircleFilled(ImGui::GetCursorScreenPos(), 3.0f, COL_ITEM_RECT);
        ImGui::GetForegroundDrawList()->AddCircleFilled(ImGui::GetCurrentWindow()->DC.CursorPosPrevLine, 3.0f, COL_ITEM_RECT);
    }
}

static void HelperTableSubmitCellsCustom(ImGuiTestContext* ctx, int count_w, int count_h, void(*cell_cb)(ImGuiTestContext* ctx, int column, int line))
{
    IM_ASSERT(cell_cb != NULL);
    for (int line = 0; line < count_h; line++)
    {
        ImGui::TableNextRow();
        for (int column_n = 0; column_n < count_w; column_n++)
        {
            if (!ImGui::TableSetColumnIndex(column_n) && column_n > 0)
                continue;
            cell_cb(ctx, column_n, line);
        }
    }
}

static void HelperTableSubmitCellsButtonFill(int count_w, int count_h)
{
    HelperTableSubmitCellsCustom(NULL, count_w, count_h, [](ImGuiTestContext*, int column, int line) { ImGui::Button(Str16f("%d,%d", line, column).c_str(), ImVec2(-FLT_MIN, 0.0f)); });
}

static void HelperTableSubmitCellsButtonFix(int count_w, int count_h)
{
    HelperTableSubmitCellsCustom(NULL, count_w, count_h, [](ImGuiTestContext*, int column, int line) { ImGui::Button(Str16f("%d,%d", line, column).c_str(), ImVec2(100.0f, 0.0f)); });
}

//static
void HelperTableSubmitCellsText(int count_w, int count_h)
{
    HelperTableSubmitCellsCustom(NULL, count_w, count_h, [](ImGuiTestContext*, int column, int line) { ImGui::Text("%d,%d", line, column); });
}

// columns_desc = "WWW", "FFW", "FAA" etc.
static void HelperTableWithResizingPolicies(const char* table_id, ImGuiTableFlags table_flags, const char* columns_desc)
{
    int columns_count = (int)strlen(columns_desc);
    IM_ASSERT(columns_count < 26); // Because we are using alphabetical letters for names
    if (!ImGui::BeginTable(table_id, columns_count, table_flags))
        return;
    for (int column = 0; column < columns_count; column++)
    {
        const char policy = columns_desc[column];
        ImGuiTableColumnFlags column_flags = ImGuiTableColumnFlags_None;
        if      (policy >= 'a' && policy <= 'z') { column_flags |= ImGuiTableColumnFlags_DefaultHide; }
        if      (policy == 'f' || policy == 'F') { column_flags |= ImGuiTableColumnFlags_WidthFixed; }
        else if (policy == 'w' || policy == 'W') { column_flags |= ImGuiTableColumnFlags_WidthStretch; }
        //else if (policy == 'a' || policy == 'A') { column_flags |= ImGuiTableColumnFlags_WidthAuto; }
        else IM_ASSERT(0);
        ImGui::TableSetupColumn(Str16f("%c%d", policy, column + 1).c_str(), column_flags);
    }
    ImFont* font = ImGui::FindFontByPrefix(TEST_SUITE_ALT_FONT_NAME);
    if (!font)
        IM_CHECK_NO_RET(font != NULL);
    ImGui::PushFont(font);
    ImGui::TableHeadersRow();
    ImGui::PopFont();
    for (int row = 0; row < 2; row++)
    {
        ImGui::TableNextRow();
        for (int column = 0; column < columns_count; column++)
        {
            ImGui::TableSetColumnIndex(column);
            const char* column_desc = "Unknown";
            const char policy = columns_desc[column];
            if (policy == 'F' || policy == 'f') { column_desc = "Fixed"; }
            if (policy == 'W' || policy == 'w') { column_desc = "Stretch"; }
            //if (policy == 'A' || policy == 'a') { column_desc = "Auto"; }
            ImGui::Text("%s %d,%d", column_desc, row, column);
        }
    }
    ImGui::EndTable();
}

static void EditTableSizingFlags(ImGuiTableFlags* p_flags)
{
    struct EnumDesc { ImGuiTableFlags Value; const char* Name; const char* Tooltip; };
    static const EnumDesc policies[] =
    {
        { ImGuiTableFlags_None,               "Default",                            "Use default sizing policy:\n- ImGuiTableFlags_SizingFixedFit if ScrollX is on or if host window has ImGuiWindowFlags_AlwaysAutoResize.\n- ImGuiTableFlags_SizingStretchSame otherwise." },
        { ImGuiTableFlags_SizingFixedFit,     "ImGuiTableFlags_SizingFixedFit",     "Columns default to _WidthFixed (if resizable) or _WidthAuto (if not resizable), matching contents width." },
        { ImGuiTableFlags_SizingFixedSame,    "ImGuiTableFlags_SizingFixedSame",    "Columns are all the same width, matching the maximum contents width.\nImplicitly disable ImGuiTableFlags_Resizable and enable ImGuiTableFlags_NoKeepColumnsVisible." },
        { ImGuiTableFlags_SizingStretchProp,  "ImGuiTableFlags_SizingStretchProp",  "Columns default to _WidthStretch with weights proportional to their widths." },
        { ImGuiTableFlags_SizingStretchSame,  "ImGuiTableFlags_SizingStretchSame",  "Columns default to _WidthStretch with same weights." }
    };
    int idx;
    for (idx = 0; idx < IM_ARRAYSIZE(policies); idx++)
        if (policies[idx].Value == (*p_flags & ImGuiTableFlags_SizingMask_))
            break;
    const char* preview_text = (idx < IM_ARRAYSIZE(policies)) ? policies[idx].Name + (idx > 0 ? strlen("ImGuiTableFlags") : 0) : "";
    if (ImGui::BeginCombo("Sizing Policy", preview_text))
    {
        for (int n = 0; n < IM_ARRAYSIZE(policies); n++)
            if (ImGui::Selectable(policies[n].Name, idx == n))
                *p_flags = (*p_flags & ~ImGuiTableFlags_SizingMask_) | policies[n].Value;
        ImGui::EndCombo();
    }
}

void RegisterTests_Table(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Table: measure draw calls count
    // FIXME-TESTS: Resize window width to e.g. ideal size first, then resize down
    // Important: HelperTableSubmitCells uses Button() with -1 width which will CPU clip text, so we don't have interference from the contents here.
    t = IM_REGISTER_TEST(e, "table", "table_draw_calls");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(8, 8));
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Checkbox("Enable checks", &vars.Bool1);
        ImGui::Checkbox("Enable borders", &vars.Bool2);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        const ImGuiTableFlags border_flags = (vars.Bool2 ? ImGuiTableFlags_Borders : 0); // #4843, #4844 horizontal border had accidental effect on stripping draw commands.

        ImGui::Text("Text before");
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_NoClip | border_flags, ImVec2(400, 0)))
            {
                HelperTableSubmitCellsButtonFill(4, 5);
                ImGui::EndTable();
            }
            ImGui::Text("Some text after");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            ImGui::Text("%d -> %d", cmd_size_before, cmd_size_after);
            if (vars.Bool1)
                IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("table2", 4, border_flags, ImVec2(400, 0)))
            {
                HelperTableSubmitCellsButtonFill(4, 4);
                ImGui::EndTable();
            }
            int cmd_size_after = draw_list->CmdBuffer.Size;
            ImGui::Text("%d -> %d", cmd_size_before, cmd_size_after);
            if (vars.Bool1)
                IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("table3", 4, border_flags, ImVec2(400, 0)))
            {
                ImGui::TableSetupColumn("One");
                ImGui::TableSetupColumn("TwoTwo");
                ImGui::TableSetupColumn("ThreeThreeThree");
                ImGui::TableSetupColumn("FourFourFourFour");
                ImGui::TableHeadersRow();
                HelperTableSubmitCellsButtonFill(4, 4);
                ImGui::EndTable();
            }
            int cmd_size_after = draw_list->CmdBuffer.Size;
            ImGui::Text("%d -> %d", cmd_size_before, cmd_size_after);
            if (vars.Bool1)
                IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("table4", 3, border_flags))
            {
                ImGui::TableSetupColumn("One");
                ImGui::TableSetupColumn("TwoTwo");
                ImGui::TableSetupColumn("ThreeThreeThree");
                ImGui::TableHeadersRow();
                HelperTableSubmitCellsButtonFill(3, 4);
                ImGui::EndTable();
            }
            int cmd_size_after = draw_list->CmdBuffer.Size;
            ImGui::Text("%d -> %d", cmd_size_before, cmd_size_after);
            if (vars.Bool1)
                IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("table5", 3, border_flags | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("One");
                ImGui::TableSetupColumn("TwoTwo");
                ImGui::TableSetupColumn("ThreeThreeThree");
                ImGui::TableHeadersRow();
                for (int line = 0; line < 4; line++)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Selectable("Selectable Spanning", false, ImGuiSelectableFlags_SpanAllColumns);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Column 1");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("Column 2");
                }
                ImGui::EndTable();
            }
            int cmd_size_after = draw_list->CmdBuffer.Size;
            ImGui::Text("%d -> %d", cmd_size_before, cmd_size_after);
            if (vars.Bool1)
                IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Test with/without clipping
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        vars.Bool2 = true; // With border
        vars.Bool1 = false;
        ctx->WindowResize("Test window 1", ImVec2(500, 800));
        vars.Bool1 = true;
        ctx->Yield();
        ctx->Yield();
#if IMGUI_VERSION_NUM >= 18725
        vars.Bool2 = false; // Without border
        ctx->Yield();
        ctx->Yield();
#endif

        vars.Bool2 = true; // With border
        vars.Bool1 = false;
        ctx->WindowResize("Test window 1", ImVec2(ImGui::GetStyle().WindowPadding.x, 800));
        vars.Bool1 = true;
        ctx->Yield();
        ctx->Yield();
#if IMGUI_VERSION_NUM >= 18725
        vars.Bool2 = false; // Without border
        ctx->Yield();
        ctx->Yield();
#endif
    };

    // ## Table: test effect of specifying or not a width in TableSetupColumn(), the effect of toggling _Resizable or setting _Fixed/_Auto widths.
    t = IM_REGISTER_TEST(e, "table", "table_width_explicit");
    t->SetVarsDataType<TableTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableTestingVars>();
        ImGui::SetNextWindowSize(ImVec2(600.0f, 0.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::CheckboxFlags("ImGuiTableFlags_Resizable", &vars.TableFlags, ImGuiTableFlags_Resizable);
        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));
        if (ImGui::BeginTable("table1", 5, vars.TableFlags | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersV))
        {
            ImGui::TableSetupColumn("Set100", 0, 100.0f);                       // Contents: 50.0f
            ImGui::TableSetupColumn("Def");                                     // Contents: 60.0f
            ImGui::TableSetupColumn("Fix", ImGuiTableColumnFlags_WidthFixed);   // Contents: 70.0f
            ImGui::TableSetupColumn("Auto", vars.ColumnFlags[3] ? vars.ColumnFlags[3] : ImGuiTableColumnFlags_NoResize); // Contents: 80.0f
            ImGui::TableSetupColumn("Stretch", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            for (int row_n = 0; row_n < 5; row_n++)
            {
                ImGui::TableNextRow();

                for (int column_n = 0; column_n < 4; column_n++)
                {
                    ImGui::TableNextColumn();
                    if (row_n == 0)
                        ImGui::Text("%.1f", ImGui::GetContentRegionAvail().x);
                    //else if (column_n == 4)
                    //    ImGui::Button(Str16f("Fill %d,%d", row_n, column_n).c_str(), ImVec2(-FLT_MIN, 0.0f));
                    else
                        ImGui::Button(Str16f("Width %d", 50 + column_n * 10).c_str(), ImVec2(50.0f + column_n * 10.0f, 0.0f));
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableTestingVars>();
        ctx->SetRef("Test window 1");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));

        for (int n = 0; n < 2; n++)
        {
            if (n == 0)
                vars.TableFlags = ImGuiTableFlags_None;
            else if (n == 1)
                vars.TableFlags |= ImGuiTableFlags_Resizable;
            ctx->Yield();
            IM_CHECK_EQ(table->Columns[0].WidthRequest, 100.0f);
            if (vars.TableFlags & ImGuiTableFlags_Resizable)
                IM_CHECK_EQ(table->Columns[0].WidthAuto, 50.0f);
            else
                IM_CHECK_EQ(table->Columns[0].WidthAuto, 100.0f);
            IM_CHECK_EQ(table->Columns[0].WidthGiven, 100.0f);
            IM_CHECK_EQ(table->Columns[1].WidthAuto, 60.0f);
            IM_CHECK_EQ(table->Columns[1].WidthGiven, 60.0f);
            IM_CHECK_EQ(table->Columns[2].WidthAuto, 70.0f);
            IM_CHECK_EQ(table->Columns[2].WidthGiven, 70.0f);
            IM_CHECK_EQ(table->Columns[3].WidthAuto, 80.0f);
            IM_CHECK_EQ(table->Columns[3].WidthGiven, 80.0f);
            IM_CHECK_GT(table->Columns[4].WidthAuto, 1.0f);
        }

        // Resize down, check for restored widths after clearing the _Resizable flag
        vars.ColumnFlags[3] = ImGuiTableColumnFlags_WidthFixed;
        ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 0), ImVec2(-20.0f, 0));
        ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 1), ImVec2(-30.0f, 0));
        ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 2), ImVec2(-40.0f, 0));
        ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 3), ImVec2(-50.0f, 0));
        IM_CHECK_EQ(table->Columns[0].WidthRequest, 100.0f - 20.0f);    // Verify resizes
        IM_CHECK_EQ(table->Columns[1].WidthRequest, 60.0f - 30.0f);
        IM_CHECK_EQ(table->Columns[2].WidthRequest, 70.0f - 40.0f);
        IM_CHECK_EQ(table->Columns[3].WidthRequest, 80.0f - 50.0f);
        vars.ColumnFlags[3] = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize;
        ctx->Yield();
        IM_CHECK_EQ_NO_RET(table->Columns[3].WidthGiven, 80.0f);        // Auto: restore
        vars.TableFlags &= ~ImGuiTableFlags_Resizable;
        ctx->Yield();
        IM_CHECK_EQ_NO_RET(table->Columns[0].WidthGiven, 100.0f);       // Explicit: restore
        IM_CHECK_EQ_NO_RET(table->Columns[1].WidthGiven, 60.0f);        // Default: restore
        //IM_CHECK_EQ_NO_RET(table->Columns[2].WidthGiven, 70.0f - 40.0f);// Fixed: keep whatever width it had (would allow e.g. explicit call to TableSetColumnWidth)
        IM_CHECK_EQ_NO_RET(table->Columns[2].WidthGiven, 70.0f);        // Fixed: restore
        IM_CHECK_EQ_NO_RET(table->Columns[3].WidthGiven, 80.0f);        // Auto: restore

        // Resize up, check for restored widths after clearing the _Resizable flag
        vars.TableFlags |= ImGuiTableFlags_Resizable;
        vars.ColumnFlags[3] = ImGuiTableColumnFlags_WidthFixed;
        ctx->Yield();
        ctx->ItemDoubleClick(ImGui::TableGetColumnResizeID(table, 2));
        IM_CHECK_EQ(table->Columns[2].WidthGiven, 70.0f);               // Fixed: restore
        ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 0), ImVec2(+20.0f, 0));
        ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 1), ImVec2(+30.0f, 0));
        ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 2), ImVec2(+40.0f, 0));
        ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 3), ImVec2(+50.0f, 0));
        IM_CHECK_EQ(table->Columns[0].WidthRequest, 100.0f + 20.0f);    // Verify resizes
        IM_CHECK_EQ(table->Columns[1].WidthRequest, 60.0f + 30.0f);
        IM_CHECK_EQ(table->Columns[2].WidthRequest, 70.0f + 40.0f);
        IM_CHECK_EQ(table->Columns[3].WidthRequest, 80.0f + 50.0f);
        vars.ColumnFlags[3] = ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize;
        ctx->Yield();
        IM_CHECK_EQ_NO_RET(table->Columns[3].WidthGiven, 80.0f);        // Auto: restore
        vars.TableFlags &= ~ImGuiTableFlags_Resizable;
        ctx->Yield();
        IM_CHECK_EQ_NO_RET(table->Columns[0].WidthGiven, 100.0f);       // Explicit: restore
        IM_CHECK_EQ_NO_RET(table->Columns[1].WidthGiven, 60.0f);        // Default: restore
        //IM_CHECK_EQ_NO_RET(table->Columns[2].WidthGiven, 70.0f + 40.0f);// Fixed: keep whatever width it had (would allow e.g. explicit call to TableSetColumnWidth)
        IM_CHECK_EQ_NO_RET(table->Columns[2].WidthGiven, 70.0f);        // Fixed: restore
        IM_CHECK_EQ_NO_RET(table->Columns[3].WidthGiven, 80.0f);        // Auto: restore
    };

    // ## Table: test effect of specifying a weight in TableSetupColumn()
    t = IM_REGISTER_TEST(e, "table", "table_width_explicit_weight");
    t->SetVarsDataType<TableTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableTestingVars>();
        ImGui::SetNextWindowSize(ImVec2(600.0f, 0.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::CheckboxFlags("ImGuiTableFlags_Resizable", &vars.TableFlags, ImGuiTableFlags_Resizable);
        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));
        if (ImGui::BeginTable("table1", 3, 0 | ImGuiTableFlags_BordersV))
        {
            ImGui::TableSetupColumn("1.0f", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("2.0f", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("3.0f", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableHeadersRow();
            for (int row_n = 0; row_n < 4 * 3; row_n++)
            {
                ImGui::TableNextColumn();
                ImGui::Text("%.1f", ImGui::GetContentRegionAvail().x);
            }
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableTestingVars>();
        ctx->SetRef("Test window 1");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));

        for (int n = 0; n < 2; n++)
        {
            if (n == 0)
                vars.TableFlags = ImGuiTableFlags_None;
            else if (n == 1)
                vars.TableFlags |= ImGuiTableFlags_Resizable;
            ctx->Yield();
            IM_CHECK_EQ(table->Columns[0].StretchWeight, 1.0f);
            IM_CHECK_EQ(table->Columns[1].StretchWeight, 2.0f);
            IM_CHECK_EQ(table->Columns[2].StretchWeight, 3.0f);
            IM_CHECK((table->Columns[0].WidthRequest / table->Columns[0].StretchWeight) - (table->Columns[1].WidthRequest / table->Columns[1].StretchWeight) <= 1.0f);
            IM_CHECK((table->Columns[1].WidthRequest / table->Columns[1].StretchWeight) - (table->Columns[2].WidthRequest / table->Columns[2].StretchWeight) <= 1.0f);
        }
    };

    // ## Table: measure equal width
    t = IM_REGISTER_TEST(e, "table", "table_width_distrib");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos + ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        struct TestCase
        {
            int             ColumnCount;
            ImGuiTableFlags Flags;
        };

        TestCase test_cases[] =
        {
            { 1, ImGuiTableFlags_None },
            { 2, ImGuiTableFlags_None },
            { 3, ImGuiTableFlags_None },
            { 9, ImGuiTableFlags_None },
            { 1, ImGuiTableFlags_BordersInnerV },
            { 2, ImGuiTableFlags_BordersInnerV },
            { 3, ImGuiTableFlags_BordersInnerV },
            { 1, ImGuiTableFlags_BordersOuterV },
            { 2, ImGuiTableFlags_BordersOuterV },
            { 3, ImGuiTableFlags_BordersOuterV },
            { 1, ImGuiTableFlags_Borders },
            { 2, ImGuiTableFlags_Borders },
            { 3, ImGuiTableFlags_Borders },
            { 9, ImGuiTableFlags_Borders },
        };

        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::Checkbox("ImGuiTableFlags_PreciseStretchWidths", &vars.Bool1);

        const float max_variance = vars.Bool1 ? 0.0f : 1.0f;

        ImGui::Text("(width variance should be <= %.2f)", max_variance);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, ImTrunc(ImGui::GetStyle().ItemSpacing.y * 0.80f)));
        for (int test_case_n = 0; test_case_n < IM_ARRAYSIZE(test_cases); test_case_n++)
        {
            const TestCase& tc = test_cases[test_case_n];
            ImGui::PushID(test_case_n);

            ImGui::Spacing();
            //ImGui::Spacing();
            ImGui::Button("..", ImVec2(-FLT_MIN, 5.0f));
            ImGuiTableFlags table_flags = tc.Flags | ImGuiTableFlags_BordersOuterH;
            if (vars.Bool1)
                table_flags |= ImGuiTableFlags_PreciseWidths;
            if (ImGui::BeginTable("table1", tc.ColumnCount, table_flags))
            {
                ImGui::TableNextRow();

                float min_w = FLT_MAX;
                float max_w = -FLT_MAX;
                for (int n = 0; n < tc.ColumnCount; n++)
                {
                    ImGui::TableSetColumnIndex(n);
                    float w = ImGui::GetContentRegionAvail().x;
                    min_w = ImMin(w, min_w);
                    max_w = ImMax(w, max_w);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Width %.2f", w);
                    ImGui::Button("..", ImVec2(-FLT_MIN, 5.0f));
                }
                float w_variance = max_w - min_w;
                IM_CHECK_LE_NO_RET(w_variance, max_variance);
                ImGui::EndTable();

                if (w_variance > max_variance)
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
                ImGui::Text("#%02d: variance %.2f (min %.2f max %.2f)%s%s",
                    test_case_n, w_variance, min_w, max_w,
                    (tc.Flags & ImGuiTableFlags_BordersOuterV) ? " OuterV" : "", (tc.Flags & ImGuiTableFlags_BordersInnerV) ? " InnerV" : "");
                if (w_variance > max_variance)
                    ImGui::PopStyleColor();
            }
            ImGui::PopID();
        }
        ImGui::PopStyleVar();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->GenericVars.Bool1 = false;
        ctx->Yield(2);
        ctx->GenericVars.Bool1 = true;
        ctx->Yield(2);
    };

    // ## Table: test code keeping columns visible keep visible
    t = IM_REGISTER_TEST(e, "table", "table_width_keep_visible");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        if (vars.Width == 0.0f)
            vars.Width = 150.0f;

        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ctx->IsFirstGuiFrame())
            for (int test_n = 0; test_n < 5; test_n++)
                TableDiscardInstanceAndSettings(ImGui::GetID(Str16f("table%d", test_n).c_str()));

        // First test is duplicate of table_width_distrib
        ctx->LogDebug("TEST CASE 0");
        if (ImGui::BeginTable("table0", 2))
        {
            for (int row = 0; row < 2; row++)
            {
                ImGui::TableNextColumn();
                ImGui::Text("Width %.2f", ImGui::GetContentRegionAvail().x);
                ImVec2 p;
                p = ImGui::GetCursorScreenPos();
                ImGui::GetForegroundDrawList()->AddLine(p, p + ImVec2(ImGui::GetContentRegionAvail().x, 0.0f), IM_COL32(0, 200, 0, 128));
                //ImGui::TableSetBgColor(row == 0 ? ImGuiTableBgTarget_CellBg : ImGuiTableBgTarget_RowBg0, IM_COL32(0, 128, 0, 100));
                ImGui::TableNextColumn();
                ImGui::Text("Width %.2f", ImGui::GetContentRegionAvail().x);
                p = ImGui::GetCursorScreenPos();
                ImGui::GetForegroundDrawList()->AddLine(p, p + ImVec2(ImGui::GetContentRegionAvail().x, 0.0f), IM_COL32(0, 200, 0, 128));
                //ImGui::TableSetBgColor(row == 0 ? ImGuiTableBgTarget_CellBg : ImGuiTableBgTarget_RowBg0, IM_COL32(0, 128, 0, 100));
            }
            ImGuiTable* table = ctx->UiContext->CurrentTable;
            IM_CHECK_LE_NO_RET(ImAbs(table->Columns[0].WidthGiven - table->Columns[1].WidthGiven), 1.0f);
            ImGui::EndTable();
        }

        ImGui::DragFloat("Width", &vars.Width, 1.0f, 10.0f, 500.0f);
        const ImGuiTableFlags test_flags[4] = { ImGuiTableFlags_Borders, ImGuiTableFlags_BordersOuter, ImGuiTableFlags_BordersInner, ImGuiTableFlags_None };
        for (int test_n = 0; test_n < 4; test_n++)
            if (ImGui::BeginTable(Str16f("table%d", test_n + 1).c_str(), 4, test_flags[test_n], ImVec2(vars.Width, 100))) // Total width smaller than sum of all columns width
            {
                ctx->LogDebug("TEST CASE %d", test_n + 1);
                ImGui::TableSetupColumn("One", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                ImGui::TableSetupColumn("Two", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("Three", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                ImGui::TableSetupColumn("Four", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                HelperTableSubmitCellsText(4, 4);
                ImGuiTable* table = ctx->UiContext->CurrentTable;
                for (int n = 0; n < 4; n++)                                                                         // Check that all allocated width are smaller than requested
                    if ((table->Columns[n].WidthGiven < table->Columns[n].WidthRequest) == false)
                        IM_CHECK_LT_NO_RET(table->Columns[n].WidthGiven, table->Columns[n].WidthRequest);
                IM_CHECK_EQ_NO_RET(table->Columns[1].WidthGiven, table->Columns[2].WidthGiven);                    // Check that widths are equals for columns 1-3
                IM_CHECK_EQ_NO_RET(table->Columns[1].ClipRect.GetWidth(), table->Columns[2].ClipRect.GetWidth());  // But also that visible areas width will match for columns 1-3
                IM_CHECK_EQ_NO_RET(table->Columns[1].WidthGiven, table->Columns[3].WidthGiven);
#if 0
                IM_CHECK_EQ_NO_RET(table->Columns[1].ClipRect.GetWidth(), table->Columns[3].ClipRect.GetWidth());
#endif
                ImGui::EndTable();
            }
        ImGui::End();
    };

    // ## Table: test ImGuiTableFlags_SameWidths
    t = IM_REGISTER_TEST(e, "table", "table_width_same");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Once);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        for (int test_n = 0; test_n < 3; test_n++)
        {
            ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable * 0;
            if (test_n == 0 || test_n == 2)
                table_flags |= ImGuiTableFlags_SizingFixedSame;
            if (test_n == 1)
                table_flags |= ImGuiTableFlags_SizingStretchSame;

            ImGui::Text("TEST CASE %d", test_n);
            if (ImGui::BeginTable(Str16f("table%d", test_n).c_str(), 4, table_flags))
            {
                if (test_n == 2)
                {
                    // Mixed: this is not very well defined.
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
                }

                ImGui::TableNextColumn();
                ImGui::Text("AAA");
                ImGui::TableNextColumn();
                ImGui::Text("AAAA");
                ImGui::TableNextColumn();
                ImGui::TableNextColumn();
                ImGui::Text("AA");

                // FIXME-TESTS: Should WidthAuto be set as well?
                ImGuiTable* table = ctx->UiContext->CurrentTable;
                if (!ctx->IsFirstGuiFrame())
                {
                    if (test_n == 0)
                        for (int n = 0; n < 4; n++)
                            IM_CHECK_EQ(table->Columns[n].WidthGiven, ImGui::CalcTextSize("AAAA").x);

                    if (test_n == 0 || test_n == 1)
                        for (int n = 1; n < 4; n++)
                            IM_CHECK_LE((ImAbs(table->Columns[0].WidthGiven - table->Columns[n].WidthGiven)), 1.0f);

                    if (test_n == 2)
                    {
                        IM_CHECK_EQ(table->Columns[0].WidthGiven, ImGui::CalcTextSize("AAA").x);
                        IM_CHECK_EQ(table->Columns[2].WidthGiven, ImGui::CalcTextSize("AAA").x);
                        IM_CHECK_LE((ImAbs(table->Columns[0].WidthGiven - table->Columns[2].WidthGiven)), 1.0f);
                        IM_CHECK_LE((ImAbs(table->Columns[1].WidthGiven - table->Columns[3].WidthGiven)), 1.0f);
                    }
                }

                ImGui::EndTable();
                ImGui::Spacing();
            }
        }
        ImGui::End();
    };

    // ## Table: test auto-fit functions
    t = IM_REGISTER_TEST(e, "table", "table_width_autofit");
    struct TableWidthAutofitVars { ImGuiTableFlags Table2Flags = 0; };
    t->SetVarsDataType<TableWidthAutofitVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TableWidthAutofitVars& vars = ctx->GetVars<TableWidthAutofitVars>();
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Once);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ctx->IsFirstGuiFrame())
        {
            TableDiscardInstanceAndSettings(ImGui::GetID("table0"));
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));
        }

        if (ImGui::BeginTable("table0", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("A");
            ImGui::TableSetupColumn("B");
            ImGui::TableSetupColumn("C");
            ImGui::TableSetupColumn("F99", ImGuiTableColumnFlags_WidthFixed, 99.0f); // Initial width
            ImGui::TableHeadersRow();
            ImGui::TableNextColumn();
            ImGui::Text("AA");
            ImGui::TableNextColumn();
            ImGui::Text("AAAA");
            ImGui::TableNextColumn();
            ImGui::Text("AAAAAA");
            ImGui::TableNextColumn();
            ImGui::Text("AAAAAAAA");
            ImGui::EndTable();
        }

        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableSetupColumn("A");
            ImGui::TableSetupColumn("B");
            ImGui::TableSetupColumn("C");
            ImGui::TableSetupColumn("F99", ImGuiTableColumnFlags_WidthFixed, 99.0f); // Initial width
            ImGui::TableHeadersRow();
            ImGui::TableNextColumn();
            ImGui::Text("AA");
            ImGui::TableNextColumn();
            ImGui::Text("AAAA");
            ImGui::TableNextColumn();
            ImGui::Text("AAAAAA");
            ImGui::TableNextColumn();
            ImGui::Text("AAAAAAAA");
            ImGui::EndTable();
        }

#if IMGUI_VERSION_NUM >= 18719
        if (ImGui::BeginTable("table2", 3, vars.Table2Flags))
        {
            ImGui::TableSetupColumn("A");
            ImGui::TableSetupColumn("B");
            ImGui::TableSetupColumn("C");
            ImGui::TableHeadersRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted("Short text.");
            ImGui::TableNextColumn(); ImGui::TextUnformatted("A very long text that may not fit.");
            ImGui::TableNextColumn(); ImGui::TextUnformatted("Short text.");
            ImGui::EndTable();
        }
#endif
        ImGui::End();

    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // FIXME-TESTS: fix inconsistent references/pointers all over the place
        TableWidthAutofitVars& vars = ctx->GetVars<TableWidthAutofitVars>();

        // Test "Size All" on fixed columns
        {
            ctx->SetRef("Test window 1");
            ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table0"));
            IM_CHECK_EQ(table->Columns[3].WidthGiven, 99.0f);
            ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 0), ImVec2(+50.0f, 0));
            ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 1), ImVec2(-20.0f, 0));
            ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 2), ImVec2(+20.0f, 0));
            ctx->TableOpenContextMenu("table0");
            ctx->SetRef("//$FOCUSED");
            ctx->ItemClick("###SizeAll");
            IM_CHECK_EQ(table->Columns[0].WidthGiven, ImGui::CalcTextSize("AA").x);
            IM_CHECK_EQ(table->Columns[1].WidthGiven, ImGui::CalcTextSize("AAAA").x);
            IM_CHECK_EQ(table->Columns[2].WidthGiven, ImGui::CalcTextSize("AAAAAA").x);
            //IM_CHECK_EQ(table->Columns[3].WidthGiven, 99.0f); // SizeAll here is a best-fit size, not a revert to default
            IM_CHECK_EQ(table->Columns[3].WidthGiven, ImGui::CalcTextSize("AAAAAAAA").x);

            // Test double click
            ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 0), ImVec2(+50.0f, 0));
            ctx->MouseDoubleClick();
            IM_CHECK_EQ(table->Columns[0].WidthGiven, ImGui::CalcTextSize("AA").x);
        }

        // Test "Size One" and "Size All" on stretch columns
        {
            ctx->SetRef("Test window 1");
            ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));
            IM_CHECK_LE(ImAbs(table->Columns[0].WidthGiven - table->Columns[2].WidthGiven), 1.0f);
            IM_CHECK_EQ(table->Columns[3].WidthGiven, 99.0f);
            ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 0), ImVec2(+50.0f, 0));
            ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 1), ImVec2(-20.0f, 0));

            ctx->SetRef("Test window 1");
            ctx->TableOpenContextMenu("table1", 0);
            ctx->SetRef("//$FOCUSED");
            ctx->ItemClick("###SizeOne");
            IM_CHECK_EQ(table->Columns[0].WidthGiven, ImGui::CalcTextSize("AA").x);
            IM_CHECK_EQ(table->Columns[3].WidthGiven, 99.0f);

            ctx->SetRef("Test window 1");
            ctx->TableOpenContextMenu("table1", 1);
            ctx->SetRef("//$FOCUSED");
            ctx->ItemClick("###SizeOne");
            IM_CHECK_EQ(table->Columns[1].WidthGiven, ImGui::CalcTextSize("AAAA").x);
            IM_CHECK_EQ(table->Columns[3].WidthGiven, 99.0f);

            ctx->SetRef("Test window 1");
            ctx->TableOpenContextMenu("table1", 2);
            ctx->SetRef("//$FOCUSED");
            ctx->ItemClick("###SizeOne");
            IM_CHECK_EQ(table->Columns[2].WidthGiven, ImGui::CalcTextSize("AAAAAA").x);

            ctx->SetRef("Test window 1");
            ctx->TableOpenContextMenu("table1", 0);
            ctx->SetRef("//$FOCUSED");
            ctx->ItemClick("###SizeAll");
            IM_CHECK_EQ(table->Columns[0].StretchWeight, 1.0f);
            IM_CHECK_EQ(table->Columns[1].StretchWeight, 1.0f);
            IM_CHECK_EQ(table->Columns[2].StretchWeight, 1.0f);
            //IM_CHECK_EQ(table->Columns[3].WidthGiven, 99.0f); // FIXME: SizeAll mixed is a best-fit size, not a revert to default
            IM_CHECK_EQ(table->Columns[3].WidthGiven, ImGui::CalcTextSize("AAAAAAAA").x);
        }

#if IMGUI_VERSION_NUM >= 18719
        // Test auto-sizing a window containing a table with stretch columns with equal weights (#5276)
        for (int i = 0; i < 2; i++)
        {
            if (i == 0)
                vars.Table2Flags = ImGuiTableFlags_Borders;
            else if (i == 1)
#if IMGUI_BROKEN_TESTS
                vars.Table2Flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;
#else
                break;
#endif
            ctx->Yield();
            const char* very_long_text = "A very long text that may not fit.";
            ctx->SetRef("Test window 1");
            ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table2"));
            ctx->WindowResize("", ImVec2(800, 200));
            IM_CHECK_GT(table->Columns[1].WidthGiven, ImGui::CalcTextSize(very_long_text).x);
            ctx->ItemDoubleClick(ImGui::GetWindowResizeCornerID(ctx->GetWindowByRef(""), 0));
            IM_CHECK_GE(table->Columns[1].WidthGiven, ImGui::CalcTextSize(very_long_text).x);
        }
#endif
    };

    // ## Test Padding
    t = IM_REGISTER_TEST(e, "table", "table_padding");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        const float cell_padding = g.Style.CellPadding.x;
        const float border_size = 1.0f;

        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetNextItemWidth(30.0f);
        ImGui::SliderInt("Step", &vars.Step, 0, 3);

        ImGuiTableFlags table_flags = ImGuiTableFlags_SizingFixedFit;
        if (vars.Step == 0)
        {
            vars.Width = 50.0f + 100.0f + cell_padding * 2.0f;
        }
        if (vars.Step == 1)
        {
            table_flags |= ImGuiTableFlags_BordersInnerV;
            vars.Width = 50.0f + 100.0f + cell_padding * 2.0f + border_size;
        }
        if (vars.Step == 2)
        {
            table_flags |= ImGuiTableFlags_BordersOuterV;
            vars.Width = 50.0f + 100.0f + cell_padding * 4.0f + border_size * 2.0f;
        }
        if (vars.Step == 3)
        {
            table_flags |= ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterV;
            vars.Width = 50.0f + 100.0f + cell_padding * 4.0f + border_size * 3.0f;
        }
        if (ImGui::BeginTable("table1", 2, table_flags))
        {
            ImGui::TableSetupColumn("0", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("1", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Button("50", ImVec2(50, 0));
            ImGui::Text("(%.2f)", ImGui::GetContentRegionAvail().x);
            ImGui::Button("...", ImVec2(ImGui::GetContentRegionAvail().x, 0));

            ImGui::TableSetColumnIndex(1);
            ImGui::Button("100", ImVec2(100, 0));
            ImGui::Text("(%.2f)", ImGui::GetContentRegionAvail().x);
            ImGui::Button("...", ImVec2(ImGui::GetContentRegionAvail().x, 0));

            ImGui::EndTable();
        }
        //IMGUI_DEBUG_LOG("%f\n", ImGui::GetWindowContentRegionWidth());

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test window 1");
        ImGuiWindow* window = ctx->GetWindowByRef("");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));
        for (int step = 0; step < 4; step++)
        {
            ctx->LogDebug("Step %d", step);
            vars.Step = step;
            ctx->Yield(); // previous step previously submitted, window contents width reflect old step, outer rect reflects old step, reported ideal width reflects new step
            IM_CHECK_EQ(table->Columns[0].ContentMaxXUnfrozen - table->Columns[0].WorkMinX, 50.0f);
            IM_CHECK_EQ(table->Columns[1].ContentMaxXUnfrozen - table->Columns[1].WorkMinX, 100.0f);
            IM_CHECK_EQ(table->ColumnsAutoFitWidth, vars.Width);
            ctx->Yield(); // inner-window update to correct ideal width and auto-resize to it
            IM_CHECK_EQ(window->ContentSizeIdeal.x, vars.Width);
            ctx->Yield(); // inner-window update to correct contents width
            IM_CHECK_EQ(window->ContentSize.x, vars.Width);
        }
    };

    // ## Test behavior of some Table functions without BeginTable
    t = IM_REGISTER_TEST(e, "table", "table_functions_without_table");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        IM_CHECK_EQ(ImGui::TableGetColumnIndex(), 0);
        IM_CHECK_EQ(ImGui::TableSetColumnIndex(42), false);
        IM_CHECK_EQ(ImGui::TableNextColumn(), false);
        IM_CHECK_EQ(ImGui::TableGetColumnFlags(0), 0);
        IM_CHECK_EQ(ImGui::TableGetColumnName(), (const char*)NULL);
        ImGui::End();
    };

    t = IM_REGISTER_TEST(e, "table", "table_sizing_policies");
    t->SetVarsDataType<TableTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableTestingVars>();
        ImGui::SetNextWindowSize(ImVec2(500.0f, 500.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        {
            ImGui::Begin("Table Options");
            ImGui::SetNextItemWidth(200.0f);
            ImGui::SliderInt("Step", &vars.Step, 0, 1, "%d");
            ImGui::SetNextItemWidth(200.0f);
            EditTableSizingFlags(&vars.TableFlags);
            //vars.TableFlags = ImGuiTableFlags_SizingFixedFit;
            if (vars.Step == 1)
            {
                ImGui::SameLine();
                ImGui::Text("Column 1 Fixed to 99.0f");
            }
            ImGui::End();
        }

        if (ImGui::BeginTable("table1", 3, vars.TableFlags, vars.OuterSize))
        {
            if (vars.Step == 1)
            {
                ImGui::TableSetupColumn("", 0);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 99.0f);
                ImGui::TableSetupColumn("", 0);
            }
            for (int row = 0; row < 3; row++)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("Oh dear");
                ImGui::TableNextColumn(); ImGui::Text("Oh dear");
                ImGui::TableNextColumn(); ImGui::Text("Oh dear");
            }
            ImGui::EndTable();
            //HelperDrawAndFillBounds(&vars);
        }
        if (ImGui::BeginTable("table2", 3, vars.TableFlags, vars.OuterSize))
        {
            if (vars.Step == 1)
            {
                ImGui::TableSetupColumn("", 0);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 99.0f);
                ImGui::TableSetupColumn("", 0);
            }
            for (int row = 0; row < 3; row++)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("AAAA");
                ImGui::TableNextColumn(); ImGui::Text("BBBBBBBB");
                ImGui::TableNextColumn(); ImGui::Text("CCCCCCCCCCCC");
            }
            ImGui::EndTable();
            //HelperDrawAndFillBounds(&vars);
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableTestingVars>();
        const ImGuiTableFlags sizing_policy_flags[4] = { ImGuiTableFlags_SizingFixedFit, ImGuiTableFlags_SizingFixedSame, ImGuiTableFlags_SizingStretchProp, ImGuiTableFlags_SizingStretchSame };
        ctx->SetRef("Test window 1");
        for (int step_n = 0; step_n < 4 * 2 && !ctx->IsError(); step_n++)
        {
            const bool fixed_fill = (step_n & 1);
            const ImGuiTableFlags sizing_policy = sizing_policy_flags[step_n / 2];
            vars.TableFlags = sizing_policy;
            vars.OuterSize = ImVec2(fixed_fill ? -FLT_MIN : 0.0f, 0.0f);
            vars.Step = 0;

            ImGuiTable* table1 = ImGui::TableFindByID(ctx->GetID("table1"));
            ImGuiTable* table2 = ImGui::TableFindByID(ctx->GetID("table2"));

            for (int resize_n = 0; resize_n < 2 && !ctx->IsError(); resize_n++)
            {
                ctx->LogInfo("Step %d resize %d", step_n, resize_n);

                if (resize_n == 1)
                {
                    vars.TableFlags |= ImGuiTableFlags_Resizable;
                    ctx->Yield(2);
                    ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table1, 0), ImVec2(+20.0f, 0));
                    ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table1, 1), ImVec2(+20.0f, 0));
                    ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table2, 0), ImVec2(+20.0f, 0));
                    ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table2, 1), ImVec2(+20.0f, 0));
                    if (ctx->IsError())
                        break;
                    vars.TableFlags &= ~ImGuiTableFlags_Resizable;
                }
                ctx->Yield(2);

                if (sizing_policy == ImGuiTableFlags_SizingFixedFit || sizing_policy == ImGuiTableFlags_SizingFixedSame)
                {
                    IM_CHECK_EQ(table1->Columns[0].WidthRequest, ImGui::CalcTextSize("Oh dear").x);
                    IM_CHECK_EQ(table1->Columns[1].WidthRequest, ImGui::CalcTextSize("Oh dear").x);
                    IM_CHECK_EQ(table1->Columns[2].WidthRequest, ImGui::CalcTextSize("Oh dear").x);
                }
                if (sizing_policy == ImGuiTableFlags_SizingFixedFit)
                {
                    IM_CHECK_EQ(table2->Columns[0].WidthRequest, ImGui::CalcTextSize("AAAA").x);
                    IM_CHECK_EQ(table2->Columns[1].WidthRequest, ImGui::CalcTextSize("BBBBBBBB").x);
                    IM_CHECK_EQ(table2->Columns[2].WidthRequest, ImGui::CalcTextSize("CCCCCCCCCCCC").x);
                }
                if (sizing_policy == ImGuiTableFlags_SizingFixedSame)
                {
                    IM_CHECK_EQ(table2->Columns[0].WidthRequest, ImGui::CalcTextSize("CCCCCCCCCCCC").x);
                    IM_CHECK_EQ(table2->Columns[1].WidthRequest, ImGui::CalcTextSize("CCCCCCCCCCCC").x);
                    IM_CHECK_EQ(table2->Columns[2].WidthRequest, ImGui::CalcTextSize("CCCCCCCCCCCC").x);
                }
                if (sizing_policy == ImGuiTableFlags_SizingStretchSame || sizing_policy == ImGuiTableFlags_SizingStretchProp)
                {
                    IM_CHECK_EQ(table1->Columns[0].StretchWeight, 1.0f);
                    IM_CHECK_EQ(table1->Columns[1].StretchWeight, 1.0f);
                    IM_CHECK_EQ(table1->Columns[1].StretchWeight, 1.0f);
                }
                if (sizing_policy == ImGuiTableFlags_SizingStretchSame)
                {
                    IM_CHECK_EQ(table2->Columns[0].StretchWeight, 1.0f);
                    IM_CHECK_EQ(table2->Columns[1].StretchWeight, 1.0f);
                    IM_CHECK_EQ(table2->Columns[1].StretchWeight, 1.0f);
                }
                if (sizing_policy == ImGuiTableFlags_SizingStretchProp)
                {
                    IM_CHECK_FLOAT_EQ_EPS(table2->Columns[1].StretchWeight / table2->Columns[0].StretchWeight, ImGui::CalcTextSize("BBBBBBBB").x / ImGui::CalcTextSize("AAAA").x);
                    IM_CHECK_FLOAT_EQ_EPS(table2->Columns[2].StretchWeight / table2->Columns[0].StretchWeight, ImGui::CalcTextSize("CCCCCCCCCCCC").x / ImGui::CalcTextSize("AAAA").x);
                    IM_CHECK_FLOAT_EQ_EPS(table2->Columns[2].StretchWeight / table2->Columns[1].StretchWeight, ImGui::CalcTextSize("CCCCCCCCCCCC").x / ImGui::CalcTextSize("BBBBBBBB").x);
                }
            }

            ctx->Sleep(0.5f);
            vars.Step = 1;
            ctx->Sleep(0.5f);
            IM_CHECK_EQ(table1->Columns[1].WidthRequest, 99.0f);
            IM_CHECK_EQ(table2->Columns[1].WidthRequest, 99.0f);
        }
    };

    // ## Resizing test-bed (not an actual automated test)
    t = IM_REGISTER_TEST(e, "table", "table_resizing_behaviors");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos + ImVec2(20, 5), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(400.0f, 0.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        if (ctx->IsFirstGuiFrame())
        {
            for (int n = 0; n < 10; n++)
                TableDiscardInstanceAndSettings(ImGui::GetID(Str16f("table%d", n).c_str()));
            ctx->GenericVars.TableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Borders | ImGuiTableFlags_NoSavedSettings;
        }

        ImGui::CheckboxFlags("ImGuiTableFlags_Resizable", &ctx->GenericVars.TableFlags, ImGuiTableFlags_Resizable);
        ImGuiTableFlags flags = ctx->GenericVars.TableFlags;

        ImGui::BulletText("OK: Resize from F1| or F2|");    // ok: alter ->WidthRequested of Fixed column. Subsequent columns will be offset.
        ImGui::BulletText("OK: Resize from F3|");           // ok: alter ->WidthRequested of Fixed column. If active, ScrollX extent can be altered.
        HelperTableWithResizingPolicies("table1", flags, "FFFff");
        ImGui::Spacing();

        ImGui::BulletText("OK: Resize from F1| or F2|");    // ok: alter ->WidthRequested of Fixed column. If active, ScrollX extent can be altered, but it doesn't make much sense as the Weighted column will always be minimal size.
        ImGui::BulletText("OK: Resize from W3| (off)");     // ok: no-op (disabled by Rule A)
        HelperTableWithResizingPolicies("table2", flags, "FFW");
        ImGui::Spacing();

        ImGui::BulletText("ok: Resize from W1| or W2|");    // ok-ish
        ImGui::BulletText("OK: Resize from W3| (off)");     // ok: no-op (disabled by Rule A)
        HelperTableWithResizingPolicies("table3", flags, "WWWw");
        ImGui::Spacing();

        // Need F2w + F3w to be stable to avoid moving W1
        // lock F2L
        // move F2R
        // move F3L
        // lock F3R
        ImGui::BulletText("OK: Resize from W1| (fwd)");     // ok: forward to resizing |F2. F3 will not move.
        ImGui::BulletText("KO: Resize from F2| or F3|");    // FIXME should resize F2, F3 and not have effect on W1 (Weighted columns are _before_ the Fixed column).
        ImGui::BulletText("OK: Resize from F4| (off)");     // ok: no-op (disabled by Rule A)
        HelperTableWithResizingPolicies("table4", flags, "WFFF");
        ImGui::Spacing();

        ImGui::BulletText("OK: Resize from W1| (fwd)");     // ok: forward to resizing |F2
        ImGui::BulletText("OK: Resize from F2| (off)");     // ok: no-op (disabled by Rule A)
        HelperTableWithResizingPolicies("table5", flags, "WF");
        ImGui::Spacing();

        ImGui::BulletText("KO: Resize from W1|");           // FIXME
        ImGui::BulletText("KO: Resize from W2|");           // FIXME
        HelperTableWithResizingPolicies("table6", flags, "WWF");
        ImGui::Spacing();

        ImGui::BulletText("KO: Resize from W1|");           // FIXME
        ImGui::BulletText("KO: Resize from F2|");           // FIXME
        HelperTableWithResizingPolicies("table7", flags, "WFW");
        ImGui::Spacing();

        ImGui::BulletText("OK: Resize from W2| (fwd)");     // ok: forward
        HelperTableWithResizingPolicies("table8", flags, "FWF");
        ImGui::Spacing();

        ImGui::BulletText("OK: Resize from ?");
        HelperTableWithResizingPolicies("table9", flags, "WWWFFWWW");
        ImGui::Spacing();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTable* table = NULL;
        ImVector<float> initial_column_width;

        ctx->SetRef("Test window 1");
        table = ImGui::TableFindByID(ctx->GetID("table1"));    // Columns: FFF, do not span entire width of the table
        IM_CHECK_LT(table->ColumnsGivenWidth + 1.0f, table->InnerWindow->ContentRegionRect.GetWidth());
        initial_column_width.resize(table->ColumnsCount);
        for (int column_n = 0; column_n >= 0; column_n = table->Columns[column_n].NextEnabledColumn)
        {
            const ImGuiTableColumn* col_curr = &table->Columns[column_n];
            const ImGuiTableColumn* col_prev = col_curr->PrevEnabledColumn >= 0 ? &table->Columns[col_curr->PrevEnabledColumn] : NULL;
            const ImGuiTableColumn* col_next = col_curr->NextEnabledColumn >= 0 ? &table->Columns[col_curr->NextEnabledColumn] : NULL;
            const float width_curr = col_curr->WidthGiven;
            const float width_prev = col_prev ? col_prev->WidthGiven : 0;
            const float width_next = col_next ? col_next->WidthGiven : 0;
            const float width_total = table->ColumnsGivenWidth;
            initial_column_width[column_n] = col_curr->WidthGiven;              // Save initial column size for next test

            // Resize a column
            const float move_by = -30.0f;
            ImGuiID handle_id = ImGui::TableGetColumnResizeID(table, column_n);
            ctx->ItemDragWithDelta(handle_id, ImVec2(move_by, 0));

            IM_CHECK(!col_prev || col_prev->WidthGiven == width_prev);      // Previous column width does not change
            IM_CHECK(col_curr->WidthGiven == width_curr + move_by);         // Current column expands
            IM_CHECK(!col_next || col_next->WidthGiven == width_next);      // Next column width does not change
            IM_CHECK(table->ColumnsGivenWidth == width_total + move_by);    // Empty space after last column shrinks
        }
        IM_CHECK(table->ColumnsGivenWidth + 1 < table->InnerWindow->ContentRegionRect.GetWidth());  // All columns span entire width of the table

        // Test column fitting
        {
            // Ensure columns are smaller than their contents due to previous tests on table1
            for (int column_n = 0; column_n >= 0; column_n = table->Columns[column_n].NextEnabledColumn)
                IM_CHECK(table->Columns[column_n].WidthGiven < initial_column_width[column_n]);

            // Fit right-most column
            int column_n = table->RightMostEnabledColumn;
            const ImGuiTableColumn* col_curr = &table->Columns[column_n];

            // Fit column.
            ctx->SetRef("Test window 1");
            ctx->ItemClick(TableGetHeaderID(table, "F3"), ImGuiMouseButton_Right);
            ctx->SetRef("//$FOCUSED");
            ctx->ItemClick("###SizeOne");
            IM_CHECK(col_curr->WidthGiven == initial_column_width[column_n]);  // Column restored original size

            // Ensure columns other than right-most one were not affected
            for (column_n = 0; column_n >= 0 && column_n < table->RightMostEnabledColumn; column_n = table->Columns[column_n].NextEnabledColumn)
                IM_CHECK(table->Columns[column_n].WidthGiven < initial_column_width[column_n]);

            // Test fitting rest of the columns
            ctx->SetRef("Test window 1");
            ctx->ItemClick(TableGetHeaderID(table, "F3"), ImGuiMouseButton_Right);
            ctx->SetRef("//$FOCUSED");
            ctx->ItemClick("###SizeAll");

            // Ensure all columns fit to contents
            for (column_n = 0; column_n >= 0; column_n = table->Columns[column_n].NextEnabledColumn)
                IM_CHECK(table->Columns[column_n].WidthGiven == initial_column_width[column_n]);
        }

        ctx->SetRef("Test window 1");
        table = ImGui::TableFindByID(ctx->GetID("table2"));     // Columns: FFW, do span entire width of the table
        IM_CHECK(table->ColumnsGivenWidth == table->InnerWindow->ContentRegionRect.GetWidth());

        // Iterate visible columns and check existence of resize handles
        for (int column_n = 0; column_n >= 0; column_n = table->Columns[column_n].NextEnabledColumn)
        {
            ImGuiID handle_id = ImGui::TableGetColumnResizeID(table, column_n);
            if (column_n == table->RightMostEnabledColumn)
                IM_CHECK(ctx->ItemInfo(handle_id, ImGuiTestOpFlags_NoError).ID == 0); // W
            else
                IM_CHECK(ctx->ItemInfo(handle_id, ImGuiTestOpFlags_NoError).ID != 0); // FF
        }
        IM_CHECK(table->ColumnsGivenWidth == table->InnerWindow->ContentRegionRect.GetWidth());
    };

    // ## Test Visible flag
    t = IM_REGISTER_TEST(e, "table", "table_clip");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_ScrollX | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders, ImVec2(200, 200)))
        {
            ImGui::TableSetupColumn("One", 0, 80);
            ImGui::TableSetupColumn("Two", 0, 80);
            ImGui::TableSetupColumn("Three", 0, 80);
            ImGui::TableSetupColumn("Four", 0, 80);
            for (int row = 0; row < 2; row++)
            {
                ImGui::TableNextRow();
                bool visible_0 = ImGui::TableSetColumnIndex(0);
                ImGui::Text(visible_0 ? "1" : "0");
                bool visible_1 = ImGui::TableSetColumnIndex(1);
                ImGui::Text(visible_1 ? "1" : "0");
                bool visible_2 = ImGui::TableSetColumnIndex(2);
                ImGui::Text(visible_2 ? "1" : "0");
                bool visible_3 = ImGui::TableSetColumnIndex(3);
                ImGui::Text(visible_3 ? "1" : "0");
                if (ctx->FrameCount > 1)
                {
                    IM_CHECK_EQ(visible_0, true);
                    IM_CHECK_EQ(visible_1, true);
                    IM_CHECK_EQ(visible_2, true); // Half Visible
                    IM_CHECK_EQ(visible_3, false);
                    ctx->Finish();
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();
    };

    // ## Test that host AlwaysAutoResize doesn't prevent table coarse clipping
#if IMGUI_VERSION_NUM >= 18992
    t = IM_REGISTER_TEST(e, "table", "table_clip_auto_resize");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos + ImVec2(0.0f, ImGui::GetMainViewport()->Size.y - 80.0f));
#ifdef IMGUI_HAS_DOCK
        ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
#endif
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Dummy(ImVec2(100, 100));
        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_ScrollX | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders, ImVec2(200, 200)))
        {
            vars.Int1++;
            ImGui::TableNextColumn();
            ImGui::Text("Hello");
            ImGui::EndTable();
        }
        ImGui::End();
        if (ctx->FrameCount >= 10)
        {
            IM_CHECK_LE(vars.Int1, 1); // 0 or 1
            ctx->Finish();
        }
    };
#endif

    // ## Test using Clipper with a Table which is small enough to only have visible scrollbar. Fixed 2023/09/12.
#if IMGUI_VERSION_NUM >= 18992
    t = IM_REGISTER_TEST(e, "table", "table_clip_all_columns");
    struct TableClipAllColumnsVars { bool TableIsVisible = false; int ColumnsVisibleMask = 0x00; };
    t->SetVarsDataType<TableClipAllColumnsVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableClipAllColumnsVars>();
        vars.TableIsVisible = false;
        vars.ColumnsVisibleMask = 0x00;

        ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0)); // Clipper Step() used to assert in this case because of zero Y advance.
        ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImGui::GetStyle().WindowMinSize); // Offset all
        if (ImGui::BeginTable("table_scrolly", 3, ImGuiTableFlags_ScrollY))
        {
            vars.TableIsVisible = true;
            ImGuiListClipper clipper;
            clipper.Begin(100);
            while (clipper.Step())
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                    for (int column = 0; column < 3; column++)
                    {
                        if (!ImGui::TableNextColumn()) // User eagerly/correct always testing this.
                            continue;
                        vars.ColumnsVisibleMask |= (1 << ImGui::TableGetColumnIndex());
                        ImGui::Text("Hello %d,%d", column, row);
                    }
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        auto& vars = ctx->GetVars<TableClipAllColumnsVars>();
        ctx->SetRef("Test Window");
        ctx->WindowResize("", ImVec2(500, 500)); // All visible
        IM_CHECK_EQ(vars.TableIsVisible, true);
        IM_CHECK_EQ(vars.ColumnsVisibleMask, 0x07);
        ctx->WindowResize("", style.WindowMinSize); // Nothing visible
        IM_CHECK_EQ(vars.TableIsVisible, false);
        IM_CHECK_EQ(vars.ColumnsVisibleMask, 0x00);

        // Only scrollbar visible: this crashed with clipper and zero CellPadding prior to 18992
        ctx->WindowResize("", style.WindowMinSize + ImVec2(style.WindowPadding.x * 2 + style.ScrollbarSize, 500.0f));
        IM_CHECK_EQ(vars.TableIsVisible, true);
        IM_CHECK_EQ(vars.ColumnsVisibleMask, 0x01); // Per spec BeginTable() always makes column visible if none should be.
    };
#endif

    // ## Test that BeginTable/EndTable can be queried with ItemInfo() even when clipped.
#if IMGUI_VERSION_NUM >= 19065
    t = IM_REGISTER_TEST(e, "table", "table_clipped_outer_query");
    struct TableClippedOuterContainerQueryVars { bool EmptyInside = false; bool MultiInstances = false; };
    t->SetVarsDataType<TableClippedOuterContainerQueryVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableClippedOuterContainerQueryVars>();
        ImGui::SetNextWindowSize(ImVec2(300.0f, 500.0f));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Checkbox("EmptyInside", &vars.EmptyInside);
        ImGui::Checkbox("Multi-instances", &vars.MultiInstances);

        for (int n = 0; n < 6; n++)
        {
            ImGui::Text("table%d", n);

            ImGui::BeginChild(Str30f("child%d", n).c_str(), ImVec2(-FLT_MIN, 100), ImGuiChildFlags_Border | ImGuiChildFlags_FrameStyle);
            if (!vars.EmptyInside) // This affects EndChild() submission path
                ImGui::Button("Button");
            ImGui::EndChild();

            const int instances_count = vars.MultiInstances ? 3 : 1;
            for (int instance = 0; instance < instances_count; instance++)
            {
                if (ImGui::BeginTable(Str30f("table%d", n).c_str(), 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders, ImVec2(-FLT_MIN, 200)))
                {
                    if (!vars.EmptyInside) // This affects EndChild() submission path
                    {
                        ImGui::TableNextColumn();
                        ImGui::Button("Button");
                    }
                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableClippedOuterContainerQueryVars>();
        ctx->SetRef("Test Window");
        for (int step = 0; step < 3; step++)
        {
            ctx->LogDebug("Step %d", step);
            vars.EmptyInside = (step == 1);
            vars.MultiInstances = (step == 2);
            ctx->Yield(4); // FIXME-TESTS: Investigate why yielding for 3 frames makes EndTable()'s child NavLayersActiveMask != 0, needed to stress both paths
            for (int n = 0; n < 6; n++)
            {
                //ImGuiTestItemInfo info1 = ctx->WindowInfo(Str30f("child%d", n).c_str());
                //ImGuiTestItemInfo info2 = ctx->ItemInfo(Str30f("table%d", n).c_str());
                IM_CHECK_EQ(ctx->ItemExists(Str30f("table%d", n).c_str()), true);
            }
            // Double down as a child test
            for (int n = 0; n < 6; n++)
                IM_CHECK_EQ(ctx->ItemExists(Str30f("child%d", n).c_str()), true);
        }
        ctx->ScrollToItemY("table4");
    };
#endif

    // ## Test that BeginTable/EndTable with no contents doesn't fail
    t = IM_REGISTER_TEST(e, "table", "table_empty");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTable("table1", 3))
            ImGui::EndTable();
        ImGui::End();
    };

    // ## Test default sort order assignment
    t = IM_REGISTER_TEST(e, "table", "table_default_sort_order");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));
        ImGui::BeginTable("table1", 4, ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti);
        ImGui::TableSetupColumn("0", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("1", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("1", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortAscending);
        ImGui::TableSetupColumn("2", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending);
        const ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
        ImGui::TableHeadersRow();
        IM_CHECK_EQ(sort_specs->SpecsCount, 3);
        IM_CHECK_EQ(sort_specs->Specs[0].ColumnIndex, 1);
        IM_CHECK_EQ(sort_specs->Specs[1].ColumnIndex, 2);
        IM_CHECK_EQ(sort_specs->Specs[2].ColumnIndex, 3);
        IM_CHECK_EQ(sort_specs->Specs[0].SortDirection, ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(sort_specs->Specs[1].SortDirection, ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(sort_specs->Specs[2].SortDirection, ImGuiSortDirection_Descending);
        //ImGuiTable* table = ctx->UiContext->CurrentTable;
        //IM_CHECK_EQ(table->SortSpecsCount, 3);
        ImGui::EndTable();
        ImGui::End();
    };

    // ## Test using the maximum of columns (#3058, #6094)
    t = IM_REGISTER_TEST(e, "table", "table_max_columns");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 400));
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        const int columns_count = IMGUI_TABLE_MAX_COLUMNS - 1;
        ImGui::BeginTable("table1", columns_count, ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY);
        ImGui::TableSetupScrollFreeze(0, 10);
        for (int n = 0; n < columns_count; n++)
            ImGui::TableSetupColumn(Str16f("C%03d", n).c_str());
        ImGui::TableHeadersRow();
        for (int i = 0; i < 20; i++)
        {
            ImGui::TableNextRow();
            for (int n = 0; n < columns_count; n++)
            {
                ImGui::TableNextColumn();
                ImGui::Text("Data");
            }
        }

        if (ctx->FrameCount < 1)
            ImGui::SetScrollHereY(1.0f);
        if (ctx->FrameCount == 2)
        {
//#if IMGUI_VERSION_NUM >= 18923
//            const int IMGUI_TABLE_MAX_DRAW_CHANNELS = 4 + columns_count * 2;
//#endif
//            IM_CHECK_EQ(ImGui::GetCurrentTable()->DrawSplitter->_Channels.Size, IMGUI_TABLE_MAX_DRAW_CHANNELS);
            ctx->Finish();
        }
        ImGui::EndTable();
        ImGui::End();
    };

    // ## Test rendering two tables with same ID (multi-instance, synced tables) (#5557)
    t = IM_REGISTER_TEST(e, "table", "table_synced_1");
    struct MultiInstancesVars { bool MultiWindow = false, SideBySide = false, DifferSizes = false, Retest = false; int ClickCounters[3] = {}; };
    t->SetVarsDataType<MultiInstancesVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        MultiInstancesVars& vars = ctx->GetVars<MultiInstancesVars>();
        const int col_count = 3;
        float column_widths[col_count] = {};
        float first_instance_width = 0.0f;
        ImGuiWindow* first_window = NULL;

        for (int instance_n = 0; instance_n < 2; instance_n++)
        {
            float width = 300.0f + (vars.DifferSizes ? instance_n : 0) * 50.0f;
            ImGui::SetNextWindowSize(ImVec2(width, vars.MultiWindow ? 210.0f : 320.0f), ImGuiCond_Always);

            if (vars.MultiWindow && instance_n == 1)
            {
                if (vars.SideBySide)
                    ImGui::SetNextWindowPos(first_window->Rect().GetTR() + ImVec2(10.0f, 0.0f), ImGuiCond_Always);
                else
                    ImGui::SetNextWindowPos(first_window->Rect().GetBL() + ImVec2(0.0f, 10.0f), ImGuiCond_Always);
            }

            if (vars.MultiWindow || instance_n == 0)
            {
                ImGui::Begin(Str16f("Test window - %d", instance_n).c_str(), NULL, ImGuiWindowFlags_NoSavedSettings);
                ImGui::Checkbox("Multi-window table", &vars.MultiWindow);
                ImGui::Checkbox("Windows side by side", &vars.SideBySide);
                ImGui::Checkbox("Windows of different sizes", &vars.DifferSizes);
            }

            if (instance_n == 0)
                first_window = ImGui::GetCurrentWindow();

            // Use a shared is
            if (vars.MultiWindow)
                ImGui::PushOverrideID(123);

            ImGui::BeginTable("table", col_count, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_Reorderable);
            for (int c = 0; c < col_count; c++)
                ImGui::TableSetupColumn("Header", c ? ImGuiTableColumnFlags_WidthFixed : ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            for (int r = 0; r < 5; r++)
            {
                ImGui::TableNextRow();

                for (int c = 0; c < col_count; c++)
                {
                    // Second table contains larger data, attempting to confuse column sync.
                    ImGui::TableNextColumn();
                    ImGui::Text(instance_n ? "Long Data" : "Data");

                    if (r == 0 && c == 0)
                        if (ImGui::Button("Button"))
                            vars.ClickCounters[instance_n]++;
                }
            }

            if (ctx->IsFirstTestFrame() || vars.Retest)
            {
                // Perform actual test.
                ImGuiContext& g = *ctx->UiContext;
                ImGuiTable* table = g.CurrentTable;

                if (instance_n == 0)
                {
                    // Save column widths of table during first iteration.
                    for (int c = 0; c < col_count; c++)
                        column_widths[c] = table->Columns[c].WidthGiven;
                    first_instance_width = table->WorkRect.GetWidth();
                }
                else
                {
                    // Verify column widths match during second iteration.
                    // Columns preserve proportions across instances.
                    float table_width = table->WorkRect.GetWidth();
                    for (int c = 0; c < col_count; c++)
                        IM_CHECK(first_instance_width / column_widths[c] == table_width / table->Columns[c].WidthGiven);
                }
            }

            ImGui::EndTable();

            if (vars.MultiWindow)
                ImGui::PopID();

            if (vars.MultiWindow || instance_n == 1)
                ImGui::End();
        }

        vars.Retest = false;
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        MultiInstancesVars& vars = ctx->GetVars<MultiInstancesVars>();
        for (int variant = 0; variant < 2 * 2 * 2; variant++)
        {
            vars.MultiWindow = (variant & (1 << 0)) != 0;
            vars.SideBySide = (variant & (1 << 1)) != 0;
            vars.DifferSizes = (variant & (1 << 2)) != 0;

            if (!vars.MultiWindow && (vars.SideBySide || vars.DifferSizes))
                continue;   // Not applicable.

#if !IMGUI_BROKEN_TESTS
            // FIXME-TABLE: Column resize happens within first table instance, but depends on table->WorkRect of resized instance, which isn't available. See TableGetMaxColumnWidth().
            if (vars.MultiWindow && vars.SideBySide)
                continue;
            if (vars.DifferSizes)
                continue;
#endif
            ctx->LogDebug("Multi-window: %d", vars.MultiWindow);
            ctx->LogDebug("Side by side: %d", vars.SideBySide);
            ctx->LogDebug("Differ sizes: %d", vars.DifferSizes);
            ctx->Yield(2);
            ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table", vars.MultiWindow ? 123 : ctx->GetID("//Test window - 0")));
            IM_CHECK(table != NULL);
            for (int instance_no = 0; instance_no < 2; instance_no++)
            {
                // Click header
                ctx->ItemClick(TableGetHeaderID(table, 2, instance_no));

                // Resize a column in the second table. It is not important whether we increase or reduce column size.
                // Changing direction ensures resize happens around the first third of the table and does not stick to
                // either side of the table across multiple test runs.
                float direction = (table->ColumnsGivenWidth * 0.3f) < table->Columns[0].WidthGiven ? -1.0f : 1.0f;
                float length = 30.0f + 10.0f * instance_no; // Different length for different table instances
                float column_width = table->Columns[0].WidthGiven;
                ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 0, instance_no), ImVec2(length * direction, 0.0f));
                vars.Retest = true;                 // Retest again
                ctx->Yield();                       // Render one more frame to retest column widths
                IM_CHECK_EQ(table->Columns[0].WidthGiven, column_width + length * direction);
            }

            // Test references
#if IMGUI_VERSION_NUM >= 18927
            vars.ClickCounters[0] = vars.ClickCounters[1] = 0;
            ctx->ItemClick(ctx->GetID("Button", ImGui::TableGetInstanceID(table, 0)));
            IM_CHECK(vars.ClickCounters[0] == 1 && vars.ClickCounters[1] == 0);
            ctx->ItemClick(ctx->GetID("Button", ImGui::TableGetInstanceID(table, 1)));
            IM_CHECK(vars.ClickCounters[0] == 1 && vars.ClickCounters[1] == 1);
            if (!vars.MultiWindow)
            {
                vars.ClickCounters[0] = vars.ClickCounters[1] = 0;
                ctx->ItemClick("//Test window - 0/table/Button");
                IM_CHECK(vars.ClickCounters[0] == 1 && vars.ClickCounters[1] == 0);
                ctx->ItemClick("//Test window - 0/table/##Instances/$$1/Button");
                IM_CHECK(vars.ClickCounters[0] == 1 && vars.ClickCounters[1] == 1);
            }
#endif
        }
    };

#if IMGUI_VERSION_NUM >= 18974
    // ## Test shared decoration width in synced instance (#5920)
    // FIXME-TESTS: currently only testing a subset for a fix done for #6619. Should test more.
    t = IM_REGISTER_TEST(e, "table", "table_synced_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTable("tab", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 0.0f)))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (vars.Step == 1)
                vars.Width = ImGui::GetContentRegionAvail().x;
            if (vars.Step == 2)
                IM_CHECK_EQ(vars.Width, ImGui::GetContentRegionAvail().x);
            ImGui::BeginChild("foo", ImVec2(0, -2)); // in IMGUI_VERSION_NUM < 18974 this leads to parent column width flickering.
            ImGui::EndChild();
            ImGui::TableNextColumn();
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ctx->WindowResize("", ImVec2(300, 300));
        vars.Step = 1; // write
        ctx->Yield(2);
        vars.Step = 2; // Check
        ctx->WindowResize("", ImVec2(300, 200)); // Resize on Y axis only
        ctx->WindowResize("", ImVec2(300, 100));
    };
#endif

#if IMGUI_VERSION_NUM >= 19047
    // ## Test auto-fit with synced instance (#7218)
    t = IM_REGISTER_TEST(e, "table", "table_synced_3_autofit");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        bool do_checks = !ctx->IsFirstGuiFrame();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTable("test_table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextColumn();
            if (do_checks)
                IM_CHECK_EQ_NO_RET(ImGui::GetContentRegionAvail().x, ImGui::CalcTextSize("Very very long text 1").x);
            ImGui::Text("Very very long text 1");
            ImGui::TableNextColumn();
            if (do_checks)
                IM_CHECK_EQ_NO_RET(ImGui::GetContentRegionAvail().x, ImGui::CalcTextSize("Text 1").x);
            ImGui::Text("Text 1");
            ImGui::EndTable();
        }
        for (int n = 0; n < 2; n++)
        {
            // Third table is indented. Because our table width computation logic stores absolute coordinates
            // for efficiency, it makes sense to double-check this case as well.
            if (n == 1)
                ImGui::Indent();
            if (ImGui::BeginTable("test_table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
            {
                ImGui::TableNextColumn();
                if (do_checks)
                    IM_CHECK_EQ_NO_RET(ImGui::GetContentRegionAvail().x, ImGui::CalcTextSize("Very very long text 1").x);
                ImGui::Text("Text 1");
                ImGui::TableNextColumn();
                if (do_checks)
                    IM_CHECK_EQ_NO_RET(ImGui::GetContentRegionAvail().x, ImGui::CalcTextSize("Text 2").x);
                ImGui::Text("Text 2");
                ImGui::EndTable();
            }
        }
        ImGui::End();
    };
#endif

    // ## Test two tables in a tooltip continuously expanding tooltip size (#3162)
    t = IM_REGISTER_TEST(e, "table", "table_two_tables_in_tooltip");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Bug Report", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Button("Test Tooltip");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            for (int i = 0; i < 2; i++)
                if (ImGui::BeginTable(Str16f("table%d", i).c_str(), 2))
                {
                    ImGui::TableSetupColumn("Header1");
                    ImGui::TableSetupColumn("Header2");
                    ImGui::TableHeadersRow();
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Test1");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted("Test2");
                    ImGui::EndTable();
                }
            ctx->GenericVars.Float1 = ImGui::GetCurrentWindow()->Size.x;
            ImGui::EndTooltip();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Bug Report");
        ctx->MouseMove("Test Tooltip");
        ctx->SleepStandard();
        const float tooltip_width = ctx->GenericVars.Float1;
        for (int n = 0; n < 3; n++)
            IM_CHECK_EQ(ctx->GenericVars.Float1, tooltip_width);
    };

    // ## Test saving and loading table settings.
    t = IM_REGISTER_TEST(e, "table", "table_settings_1");
    struct TableSettingsVars { ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoSavedSettings; bool call_get_sort_specs = false; };
    t->SetVarsDataType<TableSettingsVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TableSettingsVars& vars = ctx->GetVars<TableSettingsVars>();

        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Table Settings", NULL, vars.window_flags);
        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable))
        {
            ImGui::TableSetupColumn("Col1");
            ImGui::TableSetupColumn("Col2");
            ImGui::TableSetupColumn("Col3", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Col4", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            if (vars.call_get_sort_specs) // Test against TableGetSortSpecs() having side effects
                ImGui::TableGetSortSpecs();
            HelperTableSubmitCellsButtonFill(4, 3);
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TableSettingsVars& vars = ctx->GetVars<TableSettingsVars>();

        ImGui::TableGcCompactSettings();

        ctx->SetRef("Table Settings");
        ImGuiID table_id = ctx->GetID("table1");
        ImGuiTable* table = ImGui::TableFindByID(table_id);

        // Number of tested settings == number of bits used in the mask for iterating all combinations.
        const int number_of_settings = 6;
        for (unsigned int mask = 0, end = (1 << number_of_settings); mask < end && !ctx->IsError(); mask++)
        {
            vars.call_get_sort_specs = ((mask >> 0) & 1) == 0;
            const bool col0_sorted_desc = (mask >> 1) & 1;
            const bool col1_hidden = (mask >> 2) & 1;
            const bool col0_reordered = (mask >> 3) & 1;
            const bool col2_resized = (mask >> 4) & 1;
            const bool col3_resized = (mask >> 5) & 1;

            // Verify that settings were indeed reset.
            auto check_initial_settings = [](ImGuiTable* table)
            {
                IM_CHECK_EQ_NO_RET(table->Columns[0].SortOrder, 0);
                IM_CHECK_EQ_NO_RET(table->Columns[1].SortOrder, -1);
                IM_CHECK_EQ_NO_RET(table->Columns[2].SortOrder, -1);
                IM_CHECK_EQ_NO_RET(table->Columns[3].SortOrder, -1);
                IM_CHECK_EQ_NO_RET(table->Columns[0].SortDirection, ImGuiSortDirection_Ascending);

                IM_CHECK_EQ_NO_RET(table->Columns[0].IsEnabled, true);
                IM_CHECK_EQ_NO_RET(table->Columns[1].IsEnabled, true);
                IM_CHECK_EQ_NO_RET(table->Columns[2].IsEnabled, true);
                IM_CHECK_EQ_NO_RET(table->Columns[3].IsEnabled, true);

                IM_CHECK_EQ_NO_RET(table->Columns[0].DisplayOrder, 0);
                IM_CHECK_EQ_NO_RET(table->Columns[1].DisplayOrder, 1);
                IM_CHECK_EQ_NO_RET(table->Columns[2].DisplayOrder, 2);
                IM_CHECK_EQ_NO_RET(table->Columns[3].DisplayOrder, 3);

                IM_CHECK_EQ_NO_RET(table->Columns[2].WidthRequest, 50.0f);
                IM_CHECK_EQ_NO_RET(table->Columns[2].WidthGiven, 50.0f);
                IM_CHECK_EQ_NO_RET(table->Columns[3].StretchWeight, 1.0f);
            };
            auto check_modified_settings = [&](ImGuiTable* table)
            {
                IM_CHECK_EQ_NO_RET(table->Columns[0].SortOrder, 0);
                IM_CHECK_EQ_NO_RET(table->Columns[0].SortDirection, col0_sorted_desc ? ImGuiSortDirection_Descending : ImGuiSortDirection_Ascending);
                IM_CHECK_EQ_NO_RET(table->Columns[1].IsEnabled, !col1_hidden);
                IM_CHECK_EQ_NO_RET(table->Columns[0].DisplayOrder, col0_reordered ? (col1_hidden ? 2 : 1) : 0);
                IM_CHECK_EQ_NO_RET(table->Columns[1].DisplayOrder, col0_reordered ? 0 : 1);
                IM_CHECK_EQ_NO_RET(table->Columns[2].DisplayOrder, col0_reordered ? (col1_hidden ? 1 : 2) : 2);
                IM_CHECK_EQ_NO_RET(table->Columns[2].WidthRequest, (col2_resized ? 60.0f : 50.0f));
                IM_CHECK_EQ_NO_RET(table->Columns[2].WidthGiven, (col2_resized ? 60.0f : 50.0f));
                IM_CHECK_EQ_NO_RET(table->Columns[3].StretchWeight, col3_resized ? 0.2f : 1.0f);
            };

            // Discard previous table state.
            TableDiscardInstanceAndSettings(table_id);
            ctx->Yield(); // Rebuild a table on next frame

            ctx->LogInfo("Permutation %d: col0_sorted_desc:%d col1_hidden:%d col0_reordered:%d col2_resized:%d col3_resized:%d", mask,
                col0_sorted_desc, col1_hidden, col0_reordered, col2_resized, col3_resized);

            // Check initial settings
            ctx->LogInfo("1/4 Check initial settings");
            table = ImGui::TableFindByID(table_id);
            check_initial_settings(table);

            // Modify table. Each bit in the mask corresponds to a single tested function.
            // Modify number_of_settings when new tests are added.
            // Modify expected_test_states adding new test states when new tests are added.
            if (col0_sorted_desc)
            {
                // FIXME-TESTS: Later we should try to simulate inputs at user level
                table->Columns[0].SortOrder = 0;
                table->Columns[0].SortDirection = ImGuiSortDirection_Descending;
            }

            if (col1_hidden)
            {
                // FIXME-TESTS: Later we should try to simulate inputs at user level.. Cannot use TableSetColumnIsEnabled() from pointer.
                table->Columns[1].IsEnabled = table->Columns[1].IsUserEnabledNextFrame = false;
                ctx->Yield();   // Must come into effect before reordering.
            }

            if (col0_reordered)
            {
                // FIXME-TESTS: Later we should try to simulate inputs at user level
                table->ReorderColumn = table->HeldHeaderColumn = 0;
                table->ReorderColumnDir = +1;
            }

            if (col2_resized)
            {
                // FIXME-TESTS: Later we should try to simulate inputs at user level
                table->Columns[2].WidthRequest = 60;
                table->Columns[2].AutoFitQueue = 0x00;
            }

            if (col3_resized)
            {
                // FIXME-TESTS: Later we should try to simulate inputs at user level
                table->Columns[3].StretchWeight = 0.2f;
                table->Columns[3].AutoFitQueue = 0x00;
            }

            ctx->Yield();

            ctx->LogInfo("2/4 Check modified settings");
            check_modified_settings(table);

            // Save table settings and destroy the table.
            table->Flags &= ~ImGuiTableFlags_NoSavedSettings;
            ImGui::TableSaveSettings(table);
            const char* all_settings = ImGui::SaveIniSettingsToMemory();
            Str64f table_ini_section("[Table][0x%08X,%d]", table->ID, table->ColumnsCount);
            ImVector<char> table_settings;
            IM_CHECK_NO_RET(ImParseFindIniSection(all_settings, table_ini_section.c_str(), &table_settings) == true);
            all_settings = NULL;

            // Recreate table with no settings.
            TableDiscardInstanceAndSettings(table_id);
            ctx->Yield();
            table = ImGui::TableFindByID(table_id);

            // Verify that settings were indeed reset.
            ctx->LogInfo("3/4 Check initial settings (after clear)");
            check_initial_settings(table);
            if (ctx->IsError())
                break;

            // Load saved settings.
            ImGui::LoadIniSettingsFromMemory(table_settings.Data);
            vars.window_flags = ImGuiWindowFlags_None;              // Allow loading settings for test window and table.
            ctx->Yield();                                           // Load settings, also a chance to mess up loaded state.
            vars.window_flags = ImGuiWindowFlags_NoSavedSettings;   // Prevent saving settings for test window and table when testing is done.
            ctx->Yield();                                           // One more frame, so _NoSavedSettings get saved.

            // Verify that table has settings that were saved.
            ctx->LogInfo("4/4 Check modified settings (after reload)");
            check_modified_settings(table);
        }

        // Ensure table settings do not leak in case of errors.
        ImGui::TableGcCompactSettings();
        TableDiscardInstanceAndSettings(table_id);
    };


    // ## Test saving and loading table settings (more)
    t = IM_REGISTER_TEST(e, "table", "table_settings_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Table Settings", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTable("table1", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable))
        {
            ImGui::TableSetupColumn("Col1");
            ImGui::TableSetupColumn("Col2");
            ImGui::TableSetupColumn("Col3", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableHeadersRow();
            HelperTableSubmitCellsButtonFix(3, 3);
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Table Settings");
        ImGuiID table_id = ctx->GetID("table1");

        TableDiscardInstanceAndSettings(table_id);
        ctx->Yield();
        ImGuiTable* table = ImGui::TableFindByID(table_id);
        IM_CHECK_EQ(table->Columns[0].IsEnabled, true);
        IM_CHECK_EQ(table->Columns[1].IsEnabled, true);
        IM_CHECK_EQ(table->Columns[2].IsEnabled, false);
        ctx->Yield();
        IM_CHECK_EQ(table->Columns[0].WidthAuto, 100.0f);
        IM_CHECK_EQ(table->Columns[1].WidthAuto, 100.0f);
        IM_CHECK_LE(table->Columns[2].WidthRequest, 0.0f);
        IM_CHECK_LE(table->Columns[2].WidthAuto, 0.0f);
        table->Columns[2].IsUserEnabledNextFrame = true;
        ctx->Yield();
        IM_CHECK_LE(table->Columns[2].WidthAuto, 0.0f);
        ctx->Yield();
        IM_CHECK_EQ(table->Columns[2].WidthAuto, 100.0f);

        // Check that WidthAuto is preserved on Reset
        ImGui::TableResetSettings(table);
        ctx->Yield();
        IM_CHECK_EQ(table->Columns[2].IsEnabled, false);
        IM_CHECK_EQ(table->Columns[2].WidthAuto, 100.0f);
        table->Columns[2].IsUserEnabledNextFrame = true;
        ctx->Yield();
        IM_CHECK_EQ(table->Columns[2].WidthAuto, 100.0f);

        // Full discard: WidthAuto is lost but AutoFit should kick-in as soon as columns is re-enabled
        TableDiscardInstanceAndSettings(table_id);
        ctx->Yield(10); // Make sure we are way past traces of auto-fitting
        table = ImGui::TableFindByID(table_id);
        IM_CHECK_EQ(table->Columns[2].IsEnabled, false);
        IM_CHECK_LT(table->Columns[2].WidthRequest, 0.0f);
        IM_CHECK_EQ(table->Columns[2].WidthAuto, 0.0f);
        table->Columns[2].IsUserEnabledNextFrame = true;
        ctx->Yield();
        IM_CHECK_LE(table->Columns[2].WidthAuto, 0.0f);
        ctx->Yield();
        IM_CHECK_EQ(table->Columns[2].WidthAuto, 100.0f);
    };

    // ## Test table behavior with ItemWidth
    t = IM_REGISTER_TEST(e, "table", "table_item_width");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ImGui::SetNextWindowSize(ImVec2(300, 200));
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImGui::PushItemWidth(50.0f);
        IM_CHECK_EQ(window->DC.ItemWidth, 50.0f);
        if (ImGui::BeginTable("table1", 2, ImGuiTableFlags_Borders))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            float column_width = ImGui::GetContentRegionAvail().x;
            ImGui::DragInt("row 1 item 1", &vars.Int1);
            ImGui::PushItemWidth(-FLT_MIN);
            IM_CHECK_EQ(ImGui::CalcItemWidth(), column_width);
            ImGui::DragInt("##row 1 item 2", &vars.Int1);
            IM_CHECK_EQ(g.LastItemData.Rect.GetWidth(), column_width);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::DragInt("##row 1 item 1", &vars.Int1);
            IM_CHECK_EQ(ImGui::CalcItemWidth(), column_width);
            IM_CHECK_EQ(g.LastItemData.Rect.GetWidth(), column_width);
            //ImGui::PopItemWidth(); // Intentionally left out as we currently allow this to be ignored

            ImGui::EndTable();
        }
        IM_CHECK_EQ(window->DC.ItemWidth, 50.0f);
        IM_CHECK_EQ(window->DC.ItemWidthStack.Size, 1);
        ImGui::PopItemWidth();

        // Test for #3760 that PopItemWidth() on bottom of the stack doesn't restore a wrong default
        if (ImGui::BeginTable("table2", 2, ImGuiTableFlags_Borders))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            float default_column_width = window->DC.ItemWidth;

            ImGui::PushItemWidth(60.0f);
            IM_CHECK_EQ(ImGui::CalcItemWidth(), 60.0f);
            ImGui::DragInt2("##row 1 item 1", &vars.IntArray[0]);
            IM_CHECK_EQ(g.LastItemData.Rect.GetWidth(), 60.0f);
            IM_CHECK_EQ(window->DC.ItemWidth, 60.0f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            IM_CHECK_EQ(window->DC.ItemWidth, 60.0f);
            ImGui::PopItemWidth();
            IM_CHECK_EQ(default_column_width, window->DC.ItemWidth);

            ImGui::EndTable();
        }
        ImGui::End();
    };

    // ## Test table sorting behaviors.
    t = IM_REGISTER_TEST(e, "table", "table_sorting");
    t->SetVarsDataType<TableTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TableTestingVars& vars = ctx->GetVars<TableTestingVars>();

        ImGui::SetNextWindowSize(ImVec2(600, 80), ImGuiCond_Appearing); // FIXME-TESTS: Why?
        ImGui::Begin("Test window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ctx->IsFirstTestFrame())
        {
            vars.TableFlags = ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti;
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));
        }

        if (ImGui::BeginTable("table1", 6, vars.TableFlags))
        {
            ImGui::TableSetupColumn("Default", vars.ColumnFlags[0]);
            ImGui::TableSetupColumn("PreferSortAscending", ImGuiTableColumnFlags_PreferSortAscending);
            ImGui::TableSetupColumn("PreferSortDescending", ImGuiTableColumnFlags_PreferSortDescending);
            ImGui::TableSetupColumn("NoSort", ImGuiTableColumnFlags_NoSort);
            ImGui::TableSetupColumn("NoSortAscending", ImGuiTableColumnFlags_NoSortAscending);
            ImGui::TableSetupColumn("NoSortDescending", ImGuiTableColumnFlags_NoSortDescending);
            ImGui::TableHeadersRow();
            HelperTableSubmitCellsButtonFill(6, 1);
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test window");

        ImGuiTestRef table_ref("table1");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID(table_ref));
        TableTestingVars& vars = ctx->GetVars<TableTestingVars>();
        vars.TableFlags = ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti;
        ctx->Yield();

        const ImGuiTableSortSpecs* sort_specs = NULL;

        // Table has no default sorting flags. Check for implicit default sorting.
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK(sort_specs != NULL);
        IM_CHECK_EQ(sort_specs->SpecsCount, 1);
        IM_CHECK_EQ(sort_specs->Specs[0].ColumnIndex, 0);
        IM_CHECK_EQ(sort_specs->Specs[0].SortOrder, 0);
        IM_CHECK_EQ(sort_specs->Specs[0].SortDirection, ImGuiSortDirection_Ascending);

        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default"), ImGuiSortDirection_Descending);   // Sorted implicitly by calling TableGetSortSpecs().
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default"), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "PreferSortAscending"), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "PreferSortAscending"), ImGuiSortDirection_Descending);

        // Not holding shift does not perform multi-sort.
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK(sort_specs != NULL);
        IM_CHECK_EQ(sort_specs->SpecsCount, 1);

        // Holding shift includes all sortable columns in multi-sort.
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "PreferSortDescending", ImGuiMod_Shift), ImGuiSortDirection_Descending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "PreferSortDescending", ImGuiMod_Shift), ImGuiSortDirection_Ascending);
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 2);

        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "NoSort", ImGuiMod_Shift), ImGuiSortDirection_None);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "NoSort", ImGuiMod_Shift), ImGuiSortDirection_None);
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 2);

        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "NoSortAscending", ImGuiMod_Shift), ImGuiSortDirection_Descending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "NoSortAscending", ImGuiMod_Shift), ImGuiSortDirection_Descending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "NoSortDescending", ImGuiMod_Shift), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "NoSortDescending", ImGuiMod_Shift), ImGuiSortDirection_Ascending);
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 4);

        // Disable multi-sort and ensure there is only one sorted column left.
        vars.TableFlags = ImGuiTableFlags_Sortable;
        ctx->Yield();
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 1);

        // Disable sorting completely. Sort spec should not be returned.
        vars.TableFlags = ImGuiTableFlags_None;
        ctx->Yield();
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK(sort_specs == NULL);

        // Test SortTristate mode
        vars.TableFlags = ImGuiTableFlags_Sortable | ImGuiTableFlags_SortTristate;
        ctx->Yield();
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        //IM_CHECK(sort_specs == NULL); // We don't test for this because settings are preserved in columns so restored on the _None -> _Sortable transition
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default"), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default"), ImGuiSortDirection_Descending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default"), ImGuiSortDirection_None);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default"), ImGuiSortDirection_Ascending);

        vars.TableFlags = ImGuiTableFlags_Sortable | ImGuiTableFlags_SortTristate | ImGuiTableFlags_SortMulti;
        ctx->Yield();

        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "PreferSortAscending"), ImGuiSortDirection_Ascending);
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 1);

        // Shift + triple-click to turn a second column back into non-sorting
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default", ImGuiMod_Shift), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default", ImGuiMod_Shift), ImGuiSortDirection_Descending);
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 2);
        IM_CHECK(sort_specs->Specs[0].ColumnIndex == 1);
        IM_CHECK(sort_specs->Specs[1].ColumnIndex == 0);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default", ImGuiMod_Shift), ImGuiSortDirection_None);
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 1);
        IM_CHECK(sort_specs->Specs[0].ColumnIndex == 1);

        // Shift + triple-click to turn a first column back into non-sorting, while preserving second (making it first)
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default", ImGuiMod_Shift), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "PreferSortAscending", ImGuiMod_Shift), ImGuiSortDirection_Descending);
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK_EQ(sort_specs->SpecsCount, 2);
        IM_CHECK_EQ(sort_specs->Specs[0].ColumnIndex, 1);
        IM_CHECK_EQ(sort_specs->Specs[1].ColumnIndex, 0);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "PreferSortAscending", ImGuiMod_Shift), ImGuiSortDirection_None);
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK_EQ(sort_specs->SpecsCount, 1);
        IM_CHECK_EQ(sort_specs->Specs[0].ColumnIndex, 0);

        // Test Tristate with per-column ImGuiTableColumnFlags_NoSortAscending / ImGuiTableColumnFlags_NoSortDescending
        vars.ColumnFlags[0] = ImGuiTableColumnFlags_NoSortAscending;
        ctx->Yield();
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK_EQ(sort_specs->Specs[0].ColumnIndex, 0);
        IM_CHECK_EQ(sort_specs->Specs[0].SortDirection, ImGuiSortDirection_Descending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default"), ImGuiSortDirection_None);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default"), ImGuiSortDirection_Descending);
        vars.ColumnFlags[0] = ImGuiTableColumnFlags_NoSortDescending;
        ctx->Yield();
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK_EQ(sort_specs->Specs[0].ColumnIndex, 0);
        IM_CHECK_EQ(sort_specs->Specs[0].SortDirection, ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default"), ImGuiSortDirection_None);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default"), ImGuiSortDirection_Ascending);

        // Disable all sorting
        vars.ColumnFlags[0] = ImGuiTableColumnFlags_None;
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default"), ImGuiSortDirection_Descending);
        IM_CHECK_EQ(ctx->TableClickHeader(table_ref, "Default"), ImGuiSortDirection_None);
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 0);

        // Disable SortTristate flag
        vars.TableFlags &= ~ImGuiTableFlags_SortTristate;
        ctx->Yield();
        sort_specs = ctx->TableGetSortSpecs(table_ref);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 1);

        // Test updating sorting direction on column flag change.
        ImGuiTableColumn* col0 = &table->Columns[0];
        vars.TableFlags = ImGuiTableFlags_Sortable;
        IM_CHECK((col0->Flags & ImGuiTableColumnFlags_NoSort) == 0);
        vars.ColumnFlags[0] = ImGuiTableColumnFlags_NoSortAscending | ImGuiTableColumnFlags_NoSortDescending;
        ctx->Yield();
        IM_CHECK((col0->Flags & ImGuiTableColumnFlags_NoSort) != 0);

        col0->SortDirection = ImGuiSortDirection_Ascending;
        vars.ColumnFlags[0] = ImGuiTableColumnFlags_NoSortAscending;
        ctx->Yield();
        IM_CHECK_EQ(col0->SortDirection, ImGuiSortDirection_Descending);

        col0->SortDirection = ImGuiSortDirection_Descending;
        vars.ColumnFlags[0] = ImGuiTableColumnFlags_NoSortDescending;
        ctx->Yield();
        IM_CHECK_EQ(col0->SortDirection, ImGuiSortDirection_Ascending);
    };

    // ## Test freezing of table rows and columns.
    t = IM_REGISTER_TEST(e, "table", "table_freezing");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        memset(ctx->GenericVars.BoolArray, 0, sizeof(ctx->GenericVars.BoolArray));
        const int column_count = 15;
        ImGuiTableFlags flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
        if (ImGui::BeginTable("table1", column_count, flags))
        {
            ImGui::TableSetupScrollFreeze(ctx->GenericVars.Int1, ctx->GenericVars.Int2);
            for (int i = 0; i < column_count; i++)
                ImGui::TableSetupColumn(Str16f("Col%d", i).c_str());
            ImGui::TableHeadersRow();

            for (int line = 0; line < 15; line++)
            {
                ImGui::TableNextRow();
                for (int column = 0; column < column_count; column++)
                {
                    if (!ImGui::TableSetColumnIndex(column))
                        continue;
                    ImGui::TextUnformatted(Str16f("%d,%d", line, column).c_str());
                    if (line < 5)
                        ctx->GenericVars.BoolArray[line] |= ImGui::IsItemVisible();
                }
            }

            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));

        // Reset scroll, if any.
        // FIXME-TESTS: 2020/09/28 running nav_from_clipped_item followed by this breaks if we don't reset scroll of outer window
        ctx->SetRef(table->OuterWindow);
        ctx->ScrollToX("", 0.0f);
        ctx->ScrollToY("", 0.0f);
        ctx->SetRef(table->InnerWindow);
        ctx->ScrollToX("", 0.0f);
        ctx->ScrollToY("", 0.0f);
        ctx->Yield();

        // No initial freezing.
        IM_CHECK(table->FreezeColumnsRequest == 0);
        IM_CHECK(table->FreezeColumnsCount == 0);
        IM_CHECK(table->FreezeRowsRequest == 0);
        IM_CHECK(table->FreezeRowsCount == 0);

        // First five columns and rows are visible at the start.
        const float column_0_width = table->Columns[0].WidthGiven;
        for (int i = 0; i < 5; i++)
        {
            IM_CHECK_EQ(table->Columns[i].IsVisibleX, true);
            IM_CHECK_EQ(ctx->GenericVars.BoolArray[i], true);
        }

        // Scroll to the bottom-right of the table.
        ctx->ScrollToX("", table->InnerWindow->ScrollMax.x);
        ctx->ScrollToY("", table->InnerWindow->ScrollMax.y);
        ctx->Yield();

        // First five columns and rows are no longer visible
        for (int i = 0; i < 5; i++)
        {
            IM_CHECK_EQ(table->Columns[i].IsVisibleX, false);
            IM_CHECK_EQ(ctx->GenericVars.BoolArray[i], false);
        }

        // Check that clipped column 0 didn't resize down
        IM_CHECK_EQ(column_0_width, table->Columns[0].WidthGiven);

        // Test row freezing.
        for (int freeze_count = 1; freeze_count <= 3; freeze_count++)
        {
            ctx->GenericVars.Int1 = 0;
            ctx->GenericVars.Int2 = freeze_count;
            ctx->Yield();
            IM_CHECK_EQ(table->FreezeRowsRequest, freeze_count);
            IM_CHECK_EQ(table->FreezeRowsCount, freeze_count);

            // Test whether frozen rows are visible. First row is headers.
            if (freeze_count >= 1)
                IM_CHECK(table->IsUsingHeaders == true);

            // Other rows are content.
            if (freeze_count >= 2)
            {
                for (int row_n = 0; row_n < 3; row_n++)
                    IM_CHECK_EQ(ctx->GenericVars.BoolArray[row_n], (row_n < freeze_count - 1));
            }
        }

        // Test column freezing.
        for (int freeze_count = 1; freeze_count <= 3; freeze_count++)
        {
            ctx->GenericVars.Int1 = freeze_count;
            ctx->GenericVars.Int2 = 0;
            ctx->Yield();
            IM_CHECK_EQ(table->FreezeColumnsRequest, freeze_count);
            IM_CHECK_EQ(table->FreezeColumnsCount, freeze_count);
            ctx->Yield();
            IM_CHECK_EQ(column_0_width, table->Columns[0].WidthGiven);

            // Test whether frozen columns are visible.
            for (int column_n = 0; column_n < 4; column_n++)
                IM_CHECK_EQ(table->Columns[column_n].IsVisibleX, (column_n < freeze_count));
        }
    };

    // ## Test nav scrolling with frozen rows/columns (#5143, #4868, #3692)
    t = IM_REGISTER_TEST(e, "table", "table_freezing_scroll");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
#if IMGUI_BROKEN_TESTS
        ImGuiTableFlags table_extra_flags = ImGuiTableFlags_Resizable; // FIXME-NAV: Nav break when overloading buttons
#else
        ImGuiTableFlags table_extra_flags = ImGuiTableFlags_None;
#endif
        if (ImGui::BeginTable("table1", 2, ImGuiTableFlags_ScrollY | table_extra_flags, ImVec2(300, 300)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("AAA");
            ImGui::TableSetupColumn("BBB");
            ImGui::TableHeadersRow();
            for (int row = 0; row < 30; ++row)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Selectable(Str30f("%d", row).c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
                ImGui::TableNextColumn();
                ImGui::Text("Example text");
            }
            ImGui::EndTable();
        }
        if (ImGui::BeginTable("table2", 30, ImGuiTableFlags_ScrollX | table_extra_flags, ImVec2(300, 300)))
        {
            ImGui::TableSetupScrollFreeze(1, 0);
            for (int row = 0; row < 30; ++row)
            {
                ImGui::TableNextRow();
                for (int col = 0; col < 30; col++)
                {
                    ImGui::TableNextColumn();
                    ImGui::Button(Str30f("%d,%d", row, col).c_str());
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;

        auto check_is_fully_visible = [ctx](const ImGuiTestRef& ref)
        {
            ImGuiTestItemInfo item_info = ctx->ItemInfo(ref);
            IM_CHECK_EQ(item_info.RectClipped.Min, item_info.RectFull.Min);
            IM_CHECK_EQ(item_info.RectClipped.Max, item_info.RectFull.Max);
        };

        {
            ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("//Test Window/table1"));
            IM_CHECK(table != NULL);
            ctx->SetRef(table->ID);
            ctx->ScrollToTop(table->InnerWindow->ID);

            ImGuiTestRef header_a = TableGetHeaderID(table, "AAA");
            ctx->ItemClick(header_a, ImGuiMouseButton_Left);
            IM_CHECK_EQ(g.NavId, TableGetHeaderID(table, "AAA"));
            ctx->KeyPress(ImGuiKey_DownArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0"));
            ctx->KeyPress(ImGuiKey_DownArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("1"));
            ctx->KeyPress(ImGuiKey_DownArrow, 25);
            IM_CHECK_EQ(g.NavId, ctx->GetID("26"));
            check_is_fully_visible("26");
            check_is_fully_visible(header_a);
            ctx->KeyPress(ImGuiKey_UpArrow, 20);
            IM_CHECK_EQ(g.NavId, ctx->GetID("6"));
#if IMGUI_VERSION_NUM >= 18915
            check_is_fully_visible("6");
#endif
            check_is_fully_visible(header_a);
            ctx->KeyPress(ImGuiKey_UpArrow, 5);
            IM_CHECK_EQ(g.NavId, ctx->GetID("1"));
#if IMGUI_VERSION_NUM >= 18915
            check_is_fully_visible("1");
#endif
            check_is_fully_visible(header_a);
            ctx->KeyPress(ImGuiKey_UpArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0"));
#if IMGUI_VERSION_NUM >= 18915
            check_is_fully_visible("0");
#endif
            check_is_fully_visible(header_a);
        }
        {
            ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("//Test Window/table2"));
            IM_CHECK(table != NULL);
            ctx->SetRef(table->ID);
            ctx->ScrollToTop(table->InnerWindow->ID);

            ctx->ItemClick("0,0");
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,0"));
            ctx->KeyPress(ImGuiKey_RightArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,1"));
            ctx->KeyPress(ImGuiKey_RightArrow);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,2"));
            ctx->KeyPress(ImGuiKey_RightArrow, 25);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,27"));
            check_is_fully_visible("0,27");
            check_is_fully_visible("0,0");
            ctx->KeyPress(ImGuiKey_LeftArrow, 20);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,7"));
#if IMGUI_VERSION_NUM >= 18915
            check_is_fully_visible("0,7");
#endif
            check_is_fully_visible("0,0");
            ctx->KeyPress(ImGuiKey_LeftArrow, 5);
            IM_CHECK_EQ(g.NavId, ctx->GetID("0,2"));
#if IMGUI_VERSION_NUM >= 18915
            check_is_fully_visible("0,2");
#endif
            check_is_fully_visible("0,0");
        }
    };

    // ## Test whether freezing works with swapped columns.
    t = IM_REGISTER_TEST(e, "table", "table_freezing_column_swap");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(250.0f, 100.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));

        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_Reorderable | ImGuiTableFlags_ScrollX))
        {
            ImGui::TableSetupScrollFreeze(ctx->GenericVars.Int1, 0);
            ImGui::TableSetupColumn("1");
            ImGui::TableSetupColumn("2");
            ImGui::TableSetupColumn("3");
            ImGui::TableSetupColumn("4");
            ImGui::TableHeadersRow();
            HelperTableSubmitCellsText(4, 5);
            ImGui::EndTable();
        }
        ImGui::End();

    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));

        // Ensure default order is set initially.
        IM_CHECK(table->IsDefaultDisplayOrder);
        for (int i = 0; i < table->ColumnsCount; i++)
            IM_CHECK_EQ(table->DisplayOrderToIndex[i], i);

        // FIXME-TESTS: Column reorder fails in fast mode. Technically it doesn't affect this test since they are next to each others.
        ctx->ItemDragAndDrop(TableGetHeaderID(table, "1"), TableGetHeaderID(table, "2"));
        ctx->ItemDragAndDrop(TableGetHeaderID(table, "3"), TableGetHeaderID(table, "4"));
        IM_CHECK_EQ(table->DisplayOrderToIndex[0], 1);
        IM_CHECK_EQ(table->DisplayOrderToIndex[1], 0);
        IM_CHECK_EQ(table->DisplayOrderToIndex[2], 3);
        IM_CHECK_EQ(table->DisplayOrderToIndex[3], 2);
        ctx->GenericVars.Int1 = 2;
        ctx->Yield();
        IM_CHECK_EQ(table->DisplayOrderToIndex[0], 1);
        IM_CHECK_EQ(table->DisplayOrderToIndex[1], 0);
        IM_CHECK_EQ(table->DisplayOrderToIndex[2], 3);
        IM_CHECK_EQ(table->DisplayOrderToIndex[3], 2);
    };

    // ## Test window with _AlwaysAutoResize getting resized to size of a table it contains.
    t = IM_REGISTER_TEST(e, "table", "table_auto_resize");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::BeginTable("table1", 2, ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Button("test", ImVec2(50, 0));
            ImGui::TableNextColumn();
            ImGui::Button("test", ImVec2(101, 0));
            ImGuiTable* table = g.CurrentTable;
            ImGuiWindow* window = g.CurrentWindow;
            if (!ctx->IsFirstGuiFrame())
            {
                IM_CHECK_EQ(table->Columns[0].WidthAuto, 50.0f);
                IM_CHECK_EQ(table->Columns[0].WidthRequest, 50.0f);
                IM_CHECK_LE(table->Columns[0].WidthGiven, 50.0f);
                IM_CHECK_EQ(table->Columns[1].WidthAuto, 101.0f);
                IM_CHECK_EQ(table->Columns[1].WidthRequest, 101.0f);
                IM_CHECK_LE(table->Columns[1].WidthGiven, 101.0f);
            }
            if (ctx->FrameCount == 1)
            {
                IM_CHECK_EQ(window->ContentSize.x, table->ColumnsAutoFitWidth);
                ctx->Finish();
            }
            ImGui::EndTable();
        }
        ImGui::End();
    };

    // ## Test window with _AlwaysAutoResize getting resized to size of a table it contains.
    t = IM_REGISTER_TEST(e, "table", "table_reported_size");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        if (ctx->IsFirstGuiFrame())
            vars.Count = 5;

        //ImGui::SetNextWindowSize(ImVec2(20, 20), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | vars.WindowFlags);
        ImGui::CheckboxFlags("ImGuiWindowFlags_AlwaysAutoResize", &vars.WindowFlags, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetNextItemWidth(200.0f);
        ImGui::SliderInt("vars.Step", &vars.Step, 0, 3);
        ImGui::SetNextItemWidth(200.0f);
        ImGui::SliderInt("Columns", &vars.Count, 1, 10);
        ImGui::Text("Button width: %s", (vars.Step & 1) ? "100.0f" : "-FLT_MAX");
        ImGui::Text("Column width: %s", (vars.Step & 2) ? "Default" : "WidthFixed=100");
        ImGui::Text("table->OuterRect.GetWidth(): %.2f", vars.Width);
        ImGui::Text("window->ContentSize.x: %.2f", g.CurrentWindow->ContentSize.x);
        const int col_row_count = vars.Count;
        if (ImGui::BeginTable("table1", col_row_count))
        {
            if (vars.Step == 0 || vars.Step == 1)
            {
                for (int i = 0; i < col_row_count; i++)
                    ImGui::TableSetupColumn(Str16f("Col%d", i).c_str(), ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableHeadersRow();
                //HelperTableSubmitCellsButtonFill(col_row_count, col_row_count);
            }
            if (vars.Step == 2 || vars.Step == 3)
            {
                // No column width + window auto-fit + auto-fill button creates a feedback loop
                for (int i = 0; i < col_row_count; i++)
                    ImGui::TableSetupColumn(Str16f("Col%d", i).c_str());
                ImGui::TableHeadersRow();
                //HelperTableSubmitCellsButtonFill(col_row_count, col_row_count);
            }
            for (int row = 0; row < col_row_count; row++)
            {
                ImGui::TableNextRow();
                for (int column = 0; column < col_row_count; column++)
                {
                    ImGui::TableSetColumnIndex(column);
                    float w = ImGui::GetContentRegionAvail().x;
                    if (vars.Step == 0 || vars.Step == 2)
                        ImGui::Button(Str16f("%.0f %d,%d", w, row, column).c_str(), ImVec2(-FLT_MIN, 0.0f));
                    else
                        ImGui::Button(Str16f("%.0f %d,%d", w, row, column).c_str(), ImVec2(100.0f, 0.0f));
                }
            }

            vars.Width = g.CurrentTable->OuterRect.GetWidth();
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTestGenericVars& vars = ctx->GenericVars;

        ctx->SetRef("Test Window");
        ImGuiWindow* window = ctx->GetWindowByRef("");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));

        for (int step = 0; step < 4; step++)
        {
            ctx->LogDebug("Step %d", step);
            vars.Count = 5;
            vars.Step = step;
            vars.WindowFlags = ImGuiWindowFlags_None;
            ctx->Yield();
            ctx->Yield();

            // FIXME-TESTS: If width is too small we get clipped and things fail
            ctx->WindowResize("", ImVec2(20.0f * vars.Count, 50));

            for (int column_step = 0; column_step < 3; column_step++)
            {
                ctx->LogDebug("Step %d with %d columns", step, vars.Count);
                vars.WindowFlags = ImGuiWindowFlags_AlwaysAutoResize;
                ctx->Yield();

                // Ensure window size is big enough to contain entire table.
                IM_CHECK(window->Rect().Contains(window->ContentRegionRect));
                IM_CHECK_EQ(window->ContentRegionRect.Min.x, table->OuterRect.Min.x);
                IM_CHECK_EQ(window->ContentRegionRect.Max.x, table->OuterRect.Max.x);
                IM_CHECK_EQ(window->ContentRegionRect.Max.y, table->OuterRect.Max.y);

                float expected_table_width = vars.Count * 100.0f + (vars.Count - 1) * (g.Style.CellPadding.x * 2.0f);
                float expected_table_height = vars.Count * (ImGui::GetFrameHeight() + g.Style.CellPadding.y * 2.0f) + (ImGui::GetTextLineHeight() + g.Style.CellPadding.y * 2.0f);
                IM_CHECK_EQ(table->OuterRect.GetHeight(), expected_table_height);

                if (step == 2)
                {
                    // No columns width specs + auto-filling button = table will keep its width
                    IM_CHECK_LT(table->OuterRect.GetWidth(), expected_table_width);
                    IM_CHECK_LT(window->ContentSizeIdeal.x, expected_table_width);
                }
                else
                {
                    IM_CHECK_EQ(table->OuterRect.GetWidth(), expected_table_width);
                    IM_CHECK_EQ(window->ContentSizeIdeal.x, expected_table_width);
                }

                vars.Count++;
                ctx->Yield(); // Changing column count = recreate table. The whole thing will look glitchy at Step==3
                ctx->Yield();
            }
        }
    };

    // ## Test that resizable column report their current size
    t = IM_REGISTER_TEST(e, "table", "table_reported_size_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));
        if (ImGui::BeginTable("table1", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("", 0, 50.0f);
            ImGui::TableSetupColumn("", 0, 100.0f);
            ImGui::TableSetupColumn("", 0, 50.0f);
            ImGui::TableNextColumn();
            ImGui::Text("Hello");
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(255, 0, 0, 50), 0);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(0, 255, 0, 50), 1);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(0, 0, 255, 50), 2);
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));

        // FIXME: Expose nicer helpers?
        const float w_extra = (table->OuterPaddingX * 2.0f) + (table->CellSpacingX1 + table->CellSpacingX2) * (table->ColumnsEnabledCount - 1) + (table->CellPaddingX * 2.0f) * table->ColumnsEnabledCount;
        ctx->TableResizeColumn("table1", 1, 100.0f);
        IM_CHECK_EQ(table->OuterWindow->ContentSize.x, 50.0f + 100.0f + 50.0f + w_extra);

        ctx->TableResizeColumn("table1", 1, 80.0f);
        IM_CHECK_EQ(table->OuterWindow->ContentSize.x, 50.0f + 80.0f + 50.0f + w_extra);
    };

    // ## Test reported size
 #if IMGUI_VERSION_NUM >= 17913
    t = IM_REGISTER_TEST(e, "table", "table_reported_size_outer");
    t->SetVarsDataType<TableTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableTestingVars>();
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));

        if (ctx->IsFirstGuiFrame())
        {
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            vars.ItemSize = ImVec2(100.0f, 200.0f);
            vars.TableFlags |= ImGuiTableFlags_SizingFixedFit;
        }

        if (vars.WindowSize.x != 0.0f && vars.WindowSize.y != 0.0f)
            ImGui::SetNextWindowSize(vars.WindowSize, ctx->IsGuiFuncOnly() ? ImGuiCond_Appearing : ImGuiCond_Always);

        const ImU32 COL_CURSOR_MAX_POS = IM_COL32(255, 0, 255, 200);
        const ImU32 COL_IDEAL_MAX_POS = IM_COL32(0, 255, 0, 200);
        const ImU32 COL_ITEM_RECT = IM_COL32(255, 255, 0, 255);
        const ImU32 COL_ROW_BG = IM_COL32(0, 255, 0, 20);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Test Window", NULL, vars.WindowFlags | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
        ImGui::PopStyleVar();
        vars.OutTableLayoutSize = ImVec2(0.0f, 0.0f);
        vars.OutTableIsItemHovered = false;
        if (ImGui::BeginTable("table1", 1, vars.TableFlags, vars.OuterSize))
        {
            //ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);

            ImGui::TableNextColumn();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, COL_ROW_BG);
            ImGui::Button(Str30f("-- %dx%d --", (int)vars.ItemSize.x, (int)vars.ItemSize.y).c_str(), vars.ItemSize);
            ImGui::EndTable();

            HelperDrawAndFillBounds(&vars);
        }
        ImGui::End();
        ImGui::PopStyleVar(2);

        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("//Test Window/table1"));
        ImGui::Begin("Options");

        //if (ctx->FrameCount < 3) { ImGui::LogToTTY(); ImGui::LogText("[%05d]\n", ctx->FrameCount); }

        ImGui::CheckboxFlags("ImGuiTableFlags_Borders", &vars.TableFlags, ImGuiTableFlags_Borders);
        ImGui::CheckboxFlags("ImGuiTableFlags_Resizable", &vars.TableFlags, ImGuiTableFlags_Resizable);
        ImGui::CheckboxFlags("ImGuiTableFlags_NoHostExtendY", &vars.TableFlags, ImGuiTableFlags_NoHostExtendY);
        ImGui::CheckboxFlags("ImGuiTableFlags_ScrollX", &vars.TableFlags, ImGuiTableFlags_ScrollX);
        ImGui::CheckboxFlags("ImGuiTableFlags_ScrollY", &vars.TableFlags, ImGuiTableFlags_ScrollY);
        ImGui::CheckboxFlags("ImGuiTableFlags_SizingFixedFit", &vars.TableFlags, ImGuiTableFlags_SizingFixedFit);
        ImGui::CheckboxFlags("ImGuiTableFlags_NoKeepColumnsVisible", &vars.TableFlags, ImGuiTableFlags_NoKeepColumnsVisible);
        ImGui::CheckboxFlags("ImGuiWindowFlags_HorizontalScrollbar", &vars.WindowFlags, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::CheckboxFlags("ImGuiWindowFlags_AlwaysAutoResize", &vars.WindowFlags, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::DragFloat2("WindowSize", &vars.WindowSize.x, 1.0f, 0.0f, FLT_MAX);
        ImGui::DragFloat2("ItemSize", &vars.ItemSize.x, 1.0f, 0.0f, FLT_MAX);
        ImGui::DragFloat2("TableOuterSize", &vars.OuterSize.x, 1.0f, -FLT_MAX, FLT_MAX);
        ImGui::Text("Out: TableLayoutSize: (%.1f, %.1f)", vars.OutTableLayoutSize.x, vars.OutTableLayoutSize.y);
        if (table->InnerWindow != table->OuterWindow)
        {
            ImGui::Text("InnerWindow->ContentSize.x = %.1f", table->InnerWindow->ContentSize.x);
            ImGui::Text("InnerWindow->ContentSize.y = %.1f", table->InnerWindow->ContentSize.y);
            ImGui::Text("InnerWindow->ContentSizeIdeal.x = %.1f", table->InnerWindow->ContentSizeIdeal.x);
            ImGui::Text("InnerWindow->ContentSizeIdeal.y = %.1f", table->InnerWindow->ContentSizeIdeal.y);
            ImGui::Text("InnerWindow->Size.x = %.1f", table->InnerWindow->Size.x);
            ImGui::Text("InnerWindow->Size.y = %.1f", table->InnerWindow->Size.y);
        }
        ImGui::Text("OuterWindow->ContentSize.x = %.1f", table->OuterWindow->ContentSize.x);
        ImGui::Text("OuterWindow->ContentSize.y = %.1f", table->OuterWindow->ContentSize.y);
        ImGui::Text("OuterWindow->ContentSizeIdeal.x = %.1f", table->OuterWindow->ContentSizeIdeal.x);
        ImGui::Text("OuterWindow->ContentSizeIdeal.y = %.1f", table->OuterWindow->ContentSizeIdeal.y);
        ImGui::Text("OuterWindow->Size.x = %.1f", table->OuterWindow->Size.x);
        ImGui::Text("OuterWindow->Size.y = %.1f", table->OuterWindow->Size.y);
        ImGui::Text("IsItemHovered() = %d", vars.OutTableIsItemHovered);

        //if (ctx->FrameCount < 3) { ImGui::LogFinish(); }

        ImGui::Separator();
        ImGui::Text("Legend, after EndTable():");
        ImGui::Checkbox("Show bounds", &vars.DebugShowBounds);
        ImGui::ColorButton("ItemRect", ImColor(COL_ITEM_RECT), ImGuiColorEditFlags_NoTooltip); ImGui::SameLine(0.f, style.ItemInnerSpacing.x); ImGui::Text("ItemRect");
        ImGui::ColorButton("CursorMaxPos", ImColor(COL_CURSOR_MAX_POS), ImGuiColorEditFlags_NoTooltip); ImGui::SameLine(0.f, style.ItemInnerSpacing.x); ImGui::Text("CursorMaxPos");
        ImGui::ColorButton("IdealMaxPos", ImColor(COL_IDEAL_MAX_POS), ImGuiColorEditFlags_NoTooltip); ImGui::SameLine(0.f, style.ItemInnerSpacing.x); ImGui::Text("IdealMaxPos");

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableTestingVars>();

        ctx->SetRef("Test Window");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));

        // [1] Widths, Non-Scrolling
        if (1)
        {
            ctx->LogInfo("[1] Widths, Non-Scrolling");
            vars.ItemSize = ImVec2(200.0f, 100.0f);

            // Use horizontal borders only to not interfere with widths

            // Special case: ImGuiTableFlags_NoHostExtendX (was outer_size.x == 0.0f) + no scroll + no stretch = MinFitX
            vars.TableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            vars.OuterSize = ImVec2(0.0f, 0.0f);
            ctx->Yield(3);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.x, 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal.x, 200.0f);
            vars.WindowSize = ImVec2(50.0f, 300.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.x, 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal.x, 200.0f);

            vars.TableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_SizingFixedFit;
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            vars.OuterSize = ImVec2(-FLT_MIN, 0.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.x, 200.0f); // We want 200.0f not 300.0f (outer_size.x <= 0.0f -> report best-fit width)
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal.x, 200.0f);
            vars.WindowSize = ImVec2(50.0f, 300.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.x, 50.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal.x, 200.0f);

            vars.TableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_SizingFixedFit;
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            vars.OuterSize = ImVec2(-30.0f, 0.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.x, 200.0f); // We want 200.0f not 300.0f (outer_size.x <= 0.0f -> report best-fit width)
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal.x, 200.0f + 30.0f);
            vars.WindowSize = ImVec2(50.0f, 300.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.x, 50.0f - 30.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal.x, 200.0f + 30.0f);

            vars.TableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_SizingFixedFit;
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            vars.OuterSize = ImVec2(100.0f, 0.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.x, 100.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal.x, 100.0f);

            vars.TableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_SizingFixedFit;
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            vars.OuterSize = ImVec2(300.0f, 0.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.x, 300.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal.x, 300.0f);
            vars.WindowSize = ImVec2(100.0f, 300.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.x, 300.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal.x, 300.0f);
        }

        // [2] Height, Non-Scrolling
        // FIXME-TESTS: those tests were written earlier, differently than the Width tests.... consider making them the same (perhaps with a loop on Axis..)
        if (1)
        {
            ctx->LogInfo("[2] Heights, Non-scrolling");

            // outer_size -FLT_MIN, button 200, window 300 -> layout 300, ideal 200
            vars.TableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders;
            vars.ItemSize = ImVec2(100.0f, 200.0f);
            vars.OuterSize = ImVec2(0.0f, -FLT_MIN);
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(vars.OutTableLayoutSize.y, 300.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.y, 200.0f);
            //IM_SUSPEND_TESTFUNC();

            // outer_size -FLT_MIN, button 200, window 100 -> layout 200, ideal 200
            vars.TableFlags &= ~ImGuiTableFlags_NoHostExtendY;
            vars.OuterSize = ImVec2(0.0f, -FLT_MIN);
            vars.WindowSize = ImVec2(300.0f, 100.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(vars.OutTableLayoutSize.y, 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.y, 200.0f);

            // w/ ImGuiTableFlags_NoHostExtendY -> layout 1, ideal 200
            vars.TableFlags |= ImGuiTableFlags_NoHostExtendY;
            vars.OuterSize = ImVec2(0.0f, -FLT_MIN);
            vars.WindowSize = ImVec2(300.0f, 100.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(vars.OutTableLayoutSize.y, 100.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.y, 100.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal.y, 200.0f);

            // outer_size 0, button 200, window 300 -> layout 200, ideal 200
            vars.TableFlags &= ~ImGuiTableFlags_NoHostExtendY;
            vars.OuterSize = ImVec2(0.0f, 0.0f);
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(vars.OutTableLayoutSize.y, 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.y, 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ScrollbarY, false);

            // outer_size 0, button 200, window 100 -> layout 200, ideal 200 (scroll)
            vars.OuterSize = ImVec2(0.0f, 0.0f);
            vars.WindowSize = ImVec2(300.0f, 100.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(vars.OutTableLayoutSize.y, 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.y, 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ScrollbarY, true);

            // outer_size 100, button 200, window 300 -> layout 200, ideal 200
            vars.OuterSize = ImVec2(0.0f, 100.0f);
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(vars.OutTableLayoutSize.y, 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.y, 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ScrollbarY, false);

            // outer_size 100, button 200, window 300, NoHostExtend -> layout 100, ideal 200
            vars.TableFlags |= ImGuiTableFlags_NoHostExtendY;
            vars.OuterSize = ImVec2(0.0f, 100.0f);
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(vars.OutTableLayoutSize.y, 100.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.y, 100.0f);
            IM_CHECK_EQ(table->OuterWindow->ScrollbarY, false);

            // outer_size 300, button 200, window 100 -> layout 300, ideal 300 (scroll)
            vars.TableFlags |= ImGuiTableFlags_NoHostExtendY;
            vars.OuterSize = ImVec2(0.0f, 300.0f);
            vars.WindowSize = ImVec2(300.0f, 100.0f);
            ctx->Yield(2);
            IM_CHECK_EQ(vars.OutTableLayoutSize.y, 300.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize.y, 300.0f);
            IM_CHECK_EQ(table->OuterWindow->ScrollbarY, true);
        }

        // Widths, Height + Scrolling
        for (ImGuiAxis axis = ImGuiAxis_X; axis <= ImGuiAxis_Y; axis = (ImGuiAxis)(axis + 1))
        {
            if (axis == ImGuiAxis_X)
                ctx->LogInfo("[3] Width + Scrolling");
            else
                ctx->LogInfo("[4] Height + Scrolling");

            vars.ItemSize[axis] = 200.0f;
            vars.ItemSize[axis ^ 1] = 100.0f;

            vars.TableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_ScrollX;
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            vars.OuterSize[axis] = 0.0f; // right/bottom align
            vars.OuterSize[axis ^ 1] = 220.0f;
            ctx->Yield(3);
            IM_CHECK_EQ(table->OuterRect.GetSize()[axis], 300.0f);
            IM_CHECK_EQ(table->InnerWindow->ContentSize[axis], 200.0f);
            IM_CHECK_EQ(table->InnerWindow->ContentSizeIdeal[axis], 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize[axis], 200.0f); // We want 200.0f not 300.0f (outer_size.x <= 0.0f -> report best-fit width)
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal[axis], 200.0f);
            vars.WindowSize[axis] = 100.0f;
            ctx->Yield(2);
            IM_CHECK_EQ(table->InnerWindow->ContentSize[axis], 200.0f);
            IM_CHECK_EQ(table->InnerWindow->ContentSizeIdeal[axis], 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize[axis], 100.0f);
            IM_CHECK_EQ(axis == ImGuiAxis_X ? table->OuterWindow->ScrollbarX : table->OuterWindow->ScrollbarY, false);
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal[axis], 200.0f);
            //IM_SUSPEND_TESTFUNC();

            vars.TableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_ScrollX;
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            vars.OuterSize[axis] = -FLT_MIN; // right/bottom align
            vars.OuterSize[axis ^ 1] = 0.0f;
            ctx->Yield(2);
            IM_CHECK_EQ(table->InnerWindow->ContentSize[axis], 200.0f);
            IM_CHECK_EQ(table->InnerWindow->ContentSizeIdeal[axis], 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize[axis], 200.0f); // We want 200.0f not 300.0f (outer_size.x <= 0.0f -> report best-fit width)
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal[axis], 200.0f);
            vars.WindowSize[axis] = 100.0f;
            ctx->Yield(2);
            IM_CHECK_EQ(table->InnerWindow->ContentSize[axis], 200.0f);
            IM_CHECK_EQ(table->InnerWindow->ContentSizeIdeal[axis], 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize[axis], 100.0f);
            IM_CHECK_EQ(axis == ImGuiAxis_X ? table->OuterWindow->ScrollbarX : table->OuterWindow->ScrollbarY, false);
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal[axis], 200.0f);
            //IM_SUSPEND_TESTFUNC();

            vars.TableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_ScrollX;
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            vars.OuterSize[axis] = 100.0f;
            vars.OuterSize[axis ^ 1] = 0.0f;
            ctx->Yield(); // small item-size previously submitted, columns[0] is min-width, reported to inner window
            ctx->Yield(); // inner-window update to small contents width, columns[0] resize to fit, reported to inner window
            ctx->Yield(); // inner-window update to correct contents width
            IM_CHECK_EQ(table->InnerWindow->ContentSize[axis], 200.0f);
            IM_CHECK_EQ(table->InnerWindow->ContentSizeIdeal[axis], 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize[axis], 100.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSizeIdeal[axis], 100.0f);
            //IM_SUSPEND_TESTFUNC();

            vars.TableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_ScrollX;
            vars.WindowSize = ImVec2(300.0f, 300.0f);
            vars.OuterSize[axis] = 300.0f;
            vars.OuterSize[axis ^ 1] = 0.0f;
            ctx->Yield(2);
            IM_CHECK_EQ(table->InnerWindow->ContentSize[axis], 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize[axis], 300.0f);
            vars.WindowSize[axis] = 100.0f;
            vars.WindowSize[axis ^ 1] = 300.0f;
            ctx->Yield(2);
            IM_CHECK_EQ(table->InnerWindow->ContentSize[axis], 200.0f);
            IM_CHECK_EQ(table->OuterWindow->ContentSize[axis], 300.0f);
        }

#if IMGUI_VERSION_NUM >= 19074
        // #7651 "Table with ImGuiTableFlags_ScrollY does not reserve horizontal space for vertical scrollbar"
        // Puzzling that this wasn't exercised properly in other tests.
        vars.TableFlags = ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_ScrollY;
        vars.WindowFlags = ImGuiWindowFlags_AlwaysAutoResize;
        vars.WindowSize = ImVec2(0.0f, 0.0f);
        //vars.ColumnsCount = 1; // Unused, hardcoded
        vars.OuterSize = ImVec2(0.0f, 100.0f);
        vars.ItemSize = ImVec2(200.0f, 500.0f);
        ctx->Yield(2);
        ctx->Yield(2);
        IM_CHECK_EQ(table->OuterWindow->Size.x, 200.0f + table->InnerWindow->ScrollbarSizes.x);
        IM_CHECK_EQ(table->OuterWindow->ContentSize.x, 200.0f + table->InnerWindow->ScrollbarSizes.x);
        ctx->Yield();
#endif
    };
#endif

    // ## Test that scrolling table doesn't clip itself when outer window is trying measure size (#6510)
    // Test success only fully valid on first run (would need to clear table + child data)
#if IMGUI_VERSION_NUM >= 18992
    t = IM_REGISTER_TEST(e, "table", "table_reported_size_outer_clipped");
    t->Flags |= ImGuiTestFlags_NoGuiWarmUp;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(-1.0f, -1.0f), ImGuiCond_Appearing);
        if (ImGui::Begin("Test Window", nullptr, ImGuiWindowFlags_NoSavedSettings))
        {
            // Not strictly required but add a TabBar in the mix as it is typically a widget that might incur a delay.
            if (ImGui::BeginTabBar("Tabs"))
            {
                if (ImGui::BeginTabItem("Tab1"))
                {
                    if (ImGui::BeginTable("Table1", 4, ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Resizable))
                    {
                        for (int n = 0; n < 4; n++)
                            ImGui::TableSetupColumn(Str16f("Column%d", n).c_str());
                        ImGui::TableHeadersRow();
                        for (int n = 0; n < 4 * 5; n++)
                            if (ImGui::TableNextColumn())
                                ImGui::Text("Test Item %d", n);
                        ImGui::EndTable();
                    }
                    ImGuiWindow* window = ImGui::GetCurrentWindow();
                    ctx->LogDebug("ContentSize.x = %f", window->ContentSize.x);
                    ctx->LogDebug("IdealMaxOff.x = %f", window->DC.IdealMaxPos.x - window->Pos.x);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        if (ctx->IsFirstGuiFrame())
        {
            TableDiscardInstanceAndSettings(ctx->GetID("//Test Window/Tabs/Tab1/Table1"));
            ImGui::ClearWindowSettings("Test Window");
        }

        ctx->Yield(3);
        ctx->SetRef("Test Window");

        ImGuiWindow* window = ctx->GetWindowByRef("");
        IM_CHECK(window != NULL);
        ImVec2 size = window->Size;
        IM_CHECK_GT(size.x, ImGui::CalcTextSize("Column0Column1Column2Column3").x);

        ctx->WindowResize("", ImVec2(-1.0f, -1.0f)); // Auto-resize
        IM_CHECK_EQ(size, window->Size);             // Check that size hasn't changed.
    };
#endif

    // ## Test NavLayer in frozen cells
    t = IM_REGISTER_TEST(e, "table", "table_nav_layer");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::BeginTable("table1", 6, ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY, ImVec2(400, ImGui::GetTextLineHeightWithSpacing() * 5)))
        {
            ImGui::TableSetupScrollFreeze(2, 2);
            HelperTableSubmitCellsCustom(ctx, 6, 20,
                [](ImGuiTestContext* ctx, int column, int line)
                {
                    ImGuiWindow* window = ImGui::GetCurrentWindow();
                    ImGuiNavLayer layer = window->DC.NavLayerCurrent;
#if IMGUI_VERSION_NUM >= 18915
                    if (line < 2 && window->Scroll.y > 0.0f)
#else
                    if ((column < 2 && window->Scroll.x > 0.0f) || (line < 2 && window->Scroll.y > 0.0f))
#endif
                        IM_CHECK_NO_RET(layer == 1);
                    else
                        IM_CHECK_NO_RET(layer == 0);
                    ImGui::Text("%d ............", layer);
                }
            );
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));

        ctx->SetRef(table->InnerWindow);
        ctx->ScrollToX("", 0.0f);
        ctx->ScrollToY("", 0.0f);
        ctx->Yield(2);
        ctx->ScrollToX("", table->InnerWindow->ScrollMax.x);
        ctx->ScrollToY("", table->InnerWindow->ScrollMax.y);
        ctx->Yield(2);
    };

    // ## Test rendering into hidden/clipped cells.
    t = IM_REGISTER_TEST(e, "table", "table_hidden_output");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(200.0f, 100.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);

        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));    // Ensure there is no horizontal scroll cached from manual run.

        if (ImGui::BeginTable("table1", 30, ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX))
        {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            for (int line = 0; line < 40; line++)
            {
                ImGui::TableNextRow();
                for (int column = 0; column < 30; column++)
                {
                    bool column_visible = ImGui::TableSetColumnIndex(column);

                    if (line == 0 && column == 29 && ctx->FrameCount == 2)
                    {
                        IM_CHECK(!column_visible);  // Last column scrolled out of view.
                        //IM_CHECK_EQ_NO_RET(window->SkipItems, true);     // FIXME-TABLE: This should be set to true, still working things out.
                        IM_CHECK_NO_RET(window->ClipRect.GetWidth() == 0);
                    }

                    if (!column_visible)
                    {
                        // FIXME-TABLE: Actually test for this.
                        window->DrawList->AddCircle(ImGui::GetCursorScreenPos(), 100.0f, IM_COL32_WHITE);
                        continue;
                    }

                    ImGui::TextUnformatted("eek");
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) { ctx->Yield(2); };

    // ## Test changing column count of existing table.
    t = IM_REGISTER_TEST(e, "table", "table_varying_columns_count");
    t->SetVarsDataType<TableTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableTestingVars>();
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | vars.WindowFlags);
        vars.ColumnsCount = ImMax(vars.ColumnsCount, 1);
        ImGui::SliderInt("Count", &vars.ColumnsCount, 1, 30);
        ImGui::CheckboxFlags("ImGuiWindowFlags_AlwaysAutoResize", &vars.WindowFlags, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::CheckboxFlags("ImGuiTableFlags_Resizable", &vars.TableFlags, ImGuiTableFlags_Resizable);
        if (ImGui::BeginTable("table1", vars.ColumnsCount, vars.TableFlags))
        {
            HelperTableSubmitCellsButtonFix(vars.ColumnsCount, 7);
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TableTestingVars>();
        vars.WindowFlags = ImGuiWindowFlags_AlwaysAutoResize;
        ctx->Yield();
        ctx->SetRef("Test Window");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));
        const int column_count[] = { 10, 15, 13, 18, 12, 20, 19 };
        for (int i = 0; i < IM_ARRAYSIZE(column_count); i++)
        {
            vars.ColumnsCount = column_count[i];
#if IMGUI_VERSION_NUM >= 18204
            int old_count = table->ColumnsCount;
            float w0 = old_count >= 2 ? table->Columns[0].WidthGiven : -1.0f;
            float w1 = old_count >= 2 ? table->Columns[1].WidthGiven : -1.0f;
#endif
            ctx->Yield();
            IM_CHECK_EQ(table->ColumnsCount, column_count[i]);
#if IMGUI_VERSION_NUM >= 18204
            if (old_count >= 2)
            {
                IM_CHECK_EQ(w0, table->Columns[0].WidthGiven);
                IM_CHECK_EQ(w1, table->Columns[1].WidthGiven);
            }
            ctx->Yield();
            if (old_count >= 2)
            {
                IM_CHECK_EQ(w0, table->Columns[0].WidthGiven);
                IM_CHECK_EQ(w1, table->Columns[1].WidthGiven);
            }
#endif
        }
    };

    // ## Miscellaneous
    t = IM_REGISTER_TEST(e, "table", "table_cov_misc");
    t->SetVarsDataType<TableTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TableTestingVars& vars = ctx->GetVars<TableTestingVars>();

        ImGui::SetNextWindowSize(ImVec2(250.0f, 100.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));

        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_ScrollX | ImGuiTableFlags_Hideable | ImGuiTableFlags_Resizable, ImVec2(0, 0), 300.0f))
        {
            if (ctx->IsFirstGuiFrame())
                vars.ColumnFlags[0] = ImGuiTableColumnFlags_WidthFixed;

            ImGui::TableSetupColumn("One", vars.ColumnFlags[0], (vars.ColumnFlags[0] & ImGuiTableColumnFlags_WidthFixed) ? 100.0f : 0, 0);
            ImGui::TableSetupColumn("Two", vars.ColumnFlags[1]);
            ImGui::TableSetupColumn("Three", vars.ColumnFlags[2]);
            ImGui::TableSetupColumn(NULL, vars.ColumnFlags[3]);
            ImGui::TableHeadersRow();
            HelperTableSubmitCellsText(4, 5);
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TableTestingVars& vars = ctx->GetVars<TableTestingVars>();

        ctx->SetRef("Test window 1");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));
        ImGuiTableColumn* col0 = &table->Columns[0];
        IM_CHECK_EQ(col0->WidthRequest, 100.0f);
        IM_CHECK_EQ(col0->WidthGiven, 100.0f);

        // Test column naming.
        IM_CHECK_STR_EQ(ImGui::TableGetColumnName(table, 2), "Three");
        IM_CHECK_STR_EQ(ImGui::TableGetColumnName(table, 3), "");

        // Test inner_width parameter of BeginTable().
        IM_CHECK(table->InnerWidth == 300.0f);
        IM_CHECK(table->InnerWidth == table->InnerWindow->ContentSize.x);   // Only true when inner window is used because of ImGuiTableFlags_ScrollX

        // Test clearing of resizable flag when no columns are resizable.
        IM_CHECK((table->Flags & ImGuiTableFlags_Resizable) != 0);
        for (int i = 0; i < table->Columns.size(); i++)
            vars.ColumnFlags[i] |= ImGuiTableColumnFlags_NoResize;
        ctx->Yield();
        IM_CHECK((table->Flags & ImGuiTableFlags_Resizable) == 0);

        // Test column hiding.
        ctx->TableSetColumnEnabled("table1", "One", false);
        //ctx->ItemClick(TableGetHeaderID(table, "One"), ImGuiMouseButton_Right);
        //ctx->SetRef("//$FOCUSED");
        //ctx->ItemUncheck("One");
        IM_CHECK(col0->IsEnabled == false);
        ctx->TableSetColumnEnabled("table1", "One", true);
        //ctx->ItemCheck("One");
        IM_CHECK(col0->IsEnabled == true);
    };

    // ## Test column reordering and resetting to default order.
    // ## Test ImGuiTableFlags_ContextMenuInBody
    t = IM_REGISTER_TEST(e, "table", "table_reorder");
    t->SetVarsDataType<TableTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(250.0f, 100.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));

        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_Reorderable | ImGuiTableFlags_ContextMenuInBody))
        {
            ImGui::TableSetupColumn("One");
            ImGui::TableSetupColumn("Two");
            ImGui::TableSetupColumn("Three");
            ImGui::TableSetupColumn("Four");
            ImGui::TableHeadersRow();
            HelperTableSubmitCellsText(4, 4);
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test window 1");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));

        // Ensure default order is set initially.
        IM_CHECK(table->IsDefaultDisplayOrder);
        for (int i = 0; i < table->ColumnsCount; i++)
            IM_CHECK_EQ(table->DisplayOrderToIndex[i], i);

        // Swap two columns.
        ctx->ItemDragAndDrop(TableGetHeaderID(table, "Two"), TableGetHeaderID(table, "Three"));

        // Verify new order.
        IM_CHECK(!table->IsDefaultDisplayOrder);
        IM_CHECK_EQ(table->DisplayOrderToIndex[0], 0);
        IM_CHECK_EQ(table->DisplayOrderToIndex[1], 2);
        IM_CHECK_EQ(table->DisplayOrderToIndex[2], 1);
        IM_CHECK_EQ(table->DisplayOrderToIndex[3], 3);

        // Reset order using the ImGuiTableFlags_ContextMenuInBody location
        ctx->MouseMoveToPos(table->InnerClipRect.GetCenter());
        ctx->MouseClick(ImGuiMouseButton_Right);
        ctx->SetRef("//$FOCUSED");
#if IMGUI_VERSION_NUM >= 18903
        ctx->ItemClick("###ResetOrder");
#else
        ctx->ItemClick("Reset order");
#endif

        // Verify default order.
        IM_CHECK(table->IsDefaultDisplayOrder);
        for (int i = 0; i < table->ColumnsCount; i++)
            IM_CHECK_EQ(table->DisplayOrderToIndex[i], i);
    };

    // ## Test whether widgets in a custom table header can be clicked.
    t = IM_REGISTER_TEST(e, "table", "table_custom_header");
    t->SetVarsDataType<TableTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(250.0f, 100.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));

        const int columns_count = 1;
        if (ImGui::BeginTable("table1", columns_count))
        {
            ImGui::TableSetupColumn("One");
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            ImGui::TableSetColumnIndex(0);
            ImGui::TableHeader(ImGui::TableGetColumnName(0));
            bool ret = ImGui::Checkbox("##checkall", &vars.Bool1);
            vars.Status.QueryInc(ret);
            ImGui::TableNextRow();
            HelperTableSubmitCellsButtonFix(columns_count, 1);
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ctx->SetRef("Test window 1");
        IM_CHECK(vars.Status.RetValue == 0);
        IM_CHECK(vars.Status.Hovered == 0);
        IM_CHECK(vars.Status.Clicked == 0);
        ctx->ItemClick(ctx->GetID("##checkall", ctx->GetID("table1")));
        IM_CHECK(vars.Status.RetValue > 0);
        IM_CHECK(vars.Status.Hovered > 0);
        IM_CHECK(vars.Status.Clicked > 0);
    };

    // ## Test keeping columns visible after column resize.
    t = IM_REGISTER_TEST(e, "table", "table_scroll_on_resize");
    t->SetVarsDataType<TableTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(250.0f, 100.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));

        if (ImGui::BeginTable("table1", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX))
        {
            ImGui::TableSetupColumn("One");
            ImGui::TableSetupColumn("Two");
            ImGui::TableSetupColumn("Three");
            ImGui::TableHeadersRow();
            ImGui::TableNextRow();
            for (int column_n = 0; column_n < ImGui::TableGetColumnCount(); column_n++)
            {
                ImGui::TableSetColumnIndex(column_n);
                ImGui::Button(Str16f("%d", column_n).c_str());
            };
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test window 1");
        ImGuiWindow* window = ctx->GetWindowByRef("");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));
        ImGuiID resize_id = ImGui::TableGetColumnResizeID(table, 0);

        // Left to right, verify that column is still fully visible at the end of resize
        ctx->ItemDragWithDelta(resize_id, ImVec2(window->InnerRect.GetWidth() * 2.0f, 0.0f));
        ImGuiTestItemInfo item_info = ctx->ItemInfo("table1/0");
        IM_CHECK(item_info.ID != 0);
        IM_CHECK(window->ClipRect.Min.x <= table->Columns[0].ClipRect.Min.x && window->ClipRect.Max.x >= table->Columns[0].ClipRect.Max.x);

        // Right to left, verify that column is still fully visible at the end of resize
        ctx->ItemDragWithDelta(resize_id, ImVec2(-window->InnerRect.GetWidth() * 2.0f, 0.0f));
        item_info = ctx->ItemInfo("table1/0");
        IM_CHECK(item_info.ID != 0);
        IM_CHECK(window->ClipRect.Min.x <= table->Columns[0].ClipRect.Min.x && window->ClipRect.Max.x >= table->Columns[0].ClipRect.Max.x);
    };

    // ## Test nested/recursing tables. Also effectively stress nested child windows.
    t = IM_REGISTER_TEST(e, "table", "table_nested");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        if (vars.Int1 == 0)
            vars.Int1 = 16;
        ImGui::SliderInt("Levels", &vars.Int1, 1, 30);
        int table_beginned_into = 0;
        for (int n = 0; n < vars.Int1; n++)
        {
            int column_count = 3;
            ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame;
            if (n & 1)
                flags |= ImGuiTableFlags_ScrollX;
            else if (n & 2)
                flags |= ImGuiTableFlags_ScrollY;
            ImVec2 outer_size(800.f - n * 16.0f, 800.f - n * 16.0f);
            if (ImGui::BeginTable(Str30f("table %d", n).c_str(), column_count, flags, outer_size))
            {
                table_beginned_into++;
                ImGui::TableSetupColumn(Str30f("head %d", n).c_str());
                ImGui::TableSetupColumn(NULL, ImGuiTableColumnFlags_WidthFixed, 20.0f);
                ImGui::TableSetupColumn(NULL, ImGuiTableColumnFlags_WidthFixed, 20.0f);
                ImGui::TableHeadersRow();
                if (n + 1 < vars.Int1)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                }
            }
            else
            {
                break;
            }
        }
        for (int n = 0; n < table_beginned_into; n++)
            ImGui::EndTable();
        ImGui::End();
    };

    // ## Test LastItemId and LastItemStatusFlags being unset in hidden columns.
    t = IM_REGISTER_TEST(e, "table", "table_hidden_columns");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(250.0f, 100.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));

        if (ImGui::BeginTable("table1", 2, ImGuiTableFlags_Hideable))
        {
            ImGuiContext& g = *ctx->UiContext;
            ImGui::TableSetupColumn("One");
            ImGui::TableSetupColumn("Two", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableHeadersRow();
            for (int column_n = 0; column_n < 2; column_n++)
            {
                ImGui::TableNextColumn();
                IM_CHECK_EQ(g.LastItemData.ID, 0u);
                IM_CHECK_EQ(g.LastItemData.StatusFlags, 0);

                ImGui::Button(Str16f("%d", column_n).c_str());
                if (column_n == 0)
                {
                    // Visible column
                    IM_CHECK_EQ(g.LastItemData.ID, ImGui::GetID("0"));
                    if (ImGui::IsItemHovered())
                        IM_CHECK(g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredRect);
                }

                if (column_n == 1)
                {
                    // Hidden column
                    IM_CHECK_EQ(g.LastItemData.ID, 0u);
                    IM_CHECK_EQ(g.LastItemData.StatusFlags, 0);
                }
            };
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test window 1");
        ctx->MouseMove("table1/0");     // Ensure LastItemStatusFlags has _HoveredRect flag.
        ctx->Yield(2);                  // Do one more frame so tests in GuiFunc can run.
    };

#if IMGUI_VERSION_NUM >= 18983
    // ## Test SameLine() between columns
    t = IM_REGISTER_TEST(e, "table", "table_sameline_between_columns");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Hello");

        if (ImGui::BeginTable("columns1", 3, ImGuiTableFlags_Borders))
        {
            ImGui::TableNextColumn();

            // Test that SameLine(0, 0) doesn't alter position (even in first column)
            ImVec2 p1 = ImGui::GetCursorScreenPos();
            ImGui::SameLine(0.0f, 0.0f);
            IM_CHECK_EQ(p1.x, ImGui::GetCursorScreenPos().x);
            IM_CHECK_EQ(p1.y, ImGui::GetCursorScreenPos().y);
            ImGui::Dummy({ 32, 32 });

            ImGui::TableNextColumn();
            IM_CHECK_EQ(ImGui::GetCursorScreenPos().y, p1.y); // Verify we start same height as before
            ImGui::Text("Test");
            IM_CHECK_EQ(ImGui::GetCursorScreenPos().y, p1.y + ImGui::GetTextLineHeightWithSpacing()); // Line height not automatically shared

            // Test that SameLine() pulls line pos and height from before.
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
#if IMGUI_VERSION_NUM >= 18985
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImGui::Dummy({ 4.0f, 4.0f });
#endif
            p1 = ImGui::GetCursorScreenPos();
            ImGui::Dummy({ 32, 32 });

            ImGui::TableNextColumn();
#if IMGUI_VERSION_NUM >= 18985
            IM_CHECK_EQ(ImGui::GetCursorScreenPos().y, p0.y);
#else
            IM_CHECK_EQ(ImGui::GetCursorScreenPos().y, p1.y);
#endif
            ImGui::SameLine(0.0f, 0.0f);
            IM_CHECK_EQ(ImGui::GetCursorScreenPos().y, p1.y);
            ImGui::Text("Test");
            IM_CHECK_EQ(ImGui::GetCursorScreenPos().y, p1.y + 32 + ImGui::GetStyle().ItemSpacing.y); // Line height shared

            ImGui::EndTable();
        }
        ImGui::End();
    };
#endif

#if IMGUI_VERSION_NUM >= 18805
    // ## Test SameLine() before a row change
    t = IM_REGISTER_TEST(e, "table", "table_sameline_before_nextrow");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Hello");

        if (ImGui::BeginTable("columns1", 3))
        {
            {
                ImGui::TableNextColumn();
                ImGui::CollapsingHeader("AAAA");
                ImGui::SameLine();
                ImGui::Text("*1");
                ImGui::TableNextColumn();
                ImGui::Text("BBBB");
                ImGui::TableNextColumn();
                ImGui::Text("CCCC\nTwoLines\nThreeLines");
            }

            {
                ImGui::TableNextColumn();
                ImGui::CollapsingHeader("AAAA2");
                ImVec2 p1 = ImGui::GetItemRectMin();
                ImGui::SameLine();
                ImGui::Text("*2");
                ImVec2 p2 = ImGui::GetItemRectMin();
                IM_CHECK_EQ(p1.y, p2.y);
                ImGui::TableNextColumn();
                ImGui::Text("BBBB2");
                ImGui::TableNextColumn();
                ImGui::Text("CCCC2\nTwoLines\nThreeLines");
                ImGui::SameLine(); // <---- Note the extraneous SameLine here
            }

            {
                ImGui::TableNextColumn();
                ImGui::CollapsingHeader("AAAA3");
                ImVec2 p1 = ImGui::GetItemRectMin();
                ImGui::SameLine();
                ImGui::Text("*3");
                ImVec2 p2 = ImGui::GetItemRectMin();
                IM_CHECK_EQ(p1.y, p2.y); // On 2022/07/18 we fixed previous row SameLine() leaking into this
                ImGui::TableNextColumn();
                ImGui::Text("BBBB3");
                ImGui::TableNextColumn();
                ImGui::Text("CCCC3\nTwoLines\nThreeLines");
            }
            ImGui::EndTable();
        }
        ImGui::End();
        ImGui::PopStyleVar();
    };
#endif

#if IMGUI_VERSION_NUM >= 18831
    // ## Test status of ImGuiTableColumnFlags_IsHovered
    t = IM_REGISTER_TEST(e, "table", "table_hovered_column");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Hello");

#if !IMGUI_BROKEN_TESTS
        ImGui::Dummy(ImVec2(500, 0.0f)); // Enlarge window so all columns are visible: TestEngine doesn't know how to resize column yet
#endif

        if (ImGui::BeginTable("table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchSame))
        {
            for (int row = 0; row < 5; row++)
            {
                ImGui::TableNextRow();
                ImGui::PushID(row);

                ImGui::TableSetColumnIndex(0);
                ImGui::Selectable("##selectable", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::Button("Col0");
                if (ImGui::TableGetColumnFlags() & ImGuiTableColumnFlags_IsHovered)
                {
                    ImGui::SameLine();
                    ImGui::Text("Hovered");
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::Button("Col1");
                ImGui::SameLine();
                ImGui::Text("%d", vars.IntArray[row]);
                if (ImGui::TableGetColumnFlags() & ImGuiTableColumnFlags_IsHovered)
                {
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("<", ImGuiDir_Left))
                        vars.IntArray[row]--;
                    ImGui::SameLine();
                    if (ImGui::ArrowButton(">", ImGuiDir_Right))
                        vars.IntArray[row]++;
                }

                ImGui::TableSetColumnIndex(2);
                if (ImGui::TableGetColumnFlags() & ImGuiTableColumnFlags_IsHovered)
                    ImGui::Text("Hovered");

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ctx->SetRef("Test Window");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table"));

        // FIXME-TESTS: tests could be on actual on public ImGui::TableGetHoveredColumn()
        ctx->MouseMove("table/$$1/Col0");
        IM_CHECK(table->HoveredColumnBody == 0);

        ctx->MouseMove("table/$$1/Col1");
        IM_CHECK(table->HoveredColumnBody == 1);
        ctx->MouseMove("table/$$1/<");
        IM_CHECK(table->HoveredColumnBody == 1);
        IM_CHECK(vars.IntArray[1] == 0);
        ctx->ItemClick("table/$$1/<");
        IM_CHECK(table->HoveredColumnBody == 1);
        IM_CHECK(vars.IntArray[1] == -1);
        ctx->ItemClick("table/$$1/>");
        ctx->ItemClick("table/$$1/>");
        IM_CHECK(table->HoveredColumnBody == 1);
        IM_CHECK(vars.IntArray[1] == 1);
    };
#endif

#if IMGUI_VERSION_NUM >= 19041
    // ## Basic test for TableGetHoveredRow() (#7350)
    t = IM_REGISTER_TEST(e, "table", "table_hovered_row");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ImGui::SetNextWindowSize(ImVec2(0.0f, ImGui::GetFrameHeightWithSpacing() * 10)); // Make window smaller than line count to ensure scrolling
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTable("table1", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV, ImVec2(0.0f, -ImGui::GetTextLineHeightWithSpacing())))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            for (int n = 0; n < 3; n++)
                ImGui::TableSetupColumn(Str30f("Column%d", n).c_str());
            ImGui::TableHeadersRow();
            HelperTableSubmitCellsButtonFix(3, 30);
            vars.Int1 = ImGui::TableGetHoveredRow();
            vars.Int2 = ImGui::TableGetHoveredColumn();
            ImGui::EndTable();
        }
        ImGui::Text("Hovered row %d, column %d", vars.Int1, vars.Int2);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ctx->SetRef("Test Window");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));
        ctx->MouseMove(TableGetHeaderID(table, "Column0")); // table1/$$0/Column0 as TableHeadersRow() does a PushID(int)
        IM_CHECK_EQ(vars.Int1, 0);
        ctx->MouseMove(TableGetHeaderID(table, "Column2")); // table1/$$1/Column2 as "
        IM_CHECK_EQ(vars.Int1, 0);
        ctx->MouseMove("table1/0,0");
        IM_CHECK_EQ(vars.Int1, 1);
        ctx->MouseMove("table1/1,0");
        IM_CHECK_EQ(vars.Int1, 2);
        ctx->ScrollToBottom(table->InnerWindow->ID);
        ctx->MouseMove(TableGetHeaderID(table, "Column0"));
        IM_CHECK_EQ(vars.Int1, 0); // At this point your frozen row is overlapping some other row
    };
#endif
}

//-------------------------------------------------------------------------
// Tests: Columns
//-------------------------------------------------------------------------

void RegisterTests_Columns(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Test number of draw calls used by columns
    t = IM_REGISTER_TEST(e, "columns", "columns_draw_calls");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(50, 0), ImVec2(FLT_MAX, FLT_MAX));
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
        IM_CHECK_EQ(draw_list->CmdBuffer.Size, cmd_count + 0);

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

        // Test: Separators in columns don't consume a draw call
        cmd_count = draw_list->CmdBuffer.Size;
        ImGui::BeginColumns("columns3", 3);
        ImGui::Text("AAAA"); ImGui::NextColumn();
        ImGui::Text("BBBB"); ImGui::NextColumn();
        ImGui::Text("CCCC"); ImGui::NextColumn();
        ImGui::Separator();
        ImGui::Text("1111"); ImGui::NextColumn();
        ImGui::Text("2222"); ImGui::NextColumn();
        ImGui::Text("3333"); ImGui::NextColumn();
        ImGui::EndColumns();
        ImGui::Text("Hello");
        IM_CHECK(draw_list->CmdBuffer.Size == cmd_count || draw_list->CmdBuffer.Size == cmd_count + 3 + 1);

        // Test: Selectables in columns don't consume a draw call
        cmd_count = draw_list->CmdBuffer.Size;
        ImGui::BeginColumns("columns3", 3);
        ImGui::Selectable("AAAA", true, ImGuiSelectableFlags_SpanAllColumns); ImGui::NextColumn();
        ImGui::Text("BBBB"); ImGui::NextColumn();
        ImGui::Text("CCCC"); ImGui::NextColumn();
        ImGui::Selectable("1111", true, ImGuiSelectableFlags_SpanAllColumns); ImGui::NextColumn();
        ImGui::Text("2222"); ImGui::NextColumn();
        ImGui::Text("3333"); ImGui::NextColumn();
        ImGui::EndColumns();
        ImGui::Text("Hello");
        IM_CHECK(draw_list->CmdBuffer.Size == cmd_count || draw_list->CmdBuffer.Size == cmd_count + 3 + 1);

        ImGui::End();
    };

    // ## Test behavior of some Column functions without Columns/BeginColumns.
    t = IM_REGISTER_TEST(e, "columns", "columns_functions_without_columns");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        IM_CHECK_EQ(ImGui::GetColumnsCount(), 1);
        IM_CHECK_EQ(ImGui::GetColumnOffset(), 0.0f);
        IM_CHECK_EQ(ImGui::GetColumnWidth(), ImGui::GetContentRegionAvail().x);
        IM_CHECK_EQ(ImGui::GetColumnIndex(), 0);
        ImGui::End();
    };

    // ## Test column reordering and resetting to default order.
    t = IM_REGISTER_TEST(e, "columns", "columns_cov_legacy_columns");
    struct ColumnsTestingVars { ImGuiOldColumnFlags Flags = 0; int Count = 0; ImGuiID ColumnsID; };
    t->SetVarsDataType<ColumnsTestingVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<ColumnsTestingVars>();

        ImGui::SetNextWindowSize(ImVec2(300.0f, 60.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test window", NULL, ImGuiWindowFlags_NoSavedSettings);
        vars.ColumnsID = ImGui::GetColumnsID("Legacy Columns", 5);
        ImGui::BeginColumns("Legacy Columns", 5, vars.Flags);

        // Flex column offset/width functions.
        vars.Count++;
        if (vars.Count == 1)
        {
            for (int i = 0; i < 5; i++)
                ImGui::SetColumnWidth(i, 50.0f);
        }
        else if (vars.Count == 3)
        {
            for (int i = 0; i < 5; i++)
                IM_CHECK_EQ(ImGui::GetColumnWidth(0), 50.0f);
        }

        for (int i = 0; i < 5; i++)
        {
            ImGui::Button(Str16f("Button %d", i).c_str());
            ImGui::NextColumn();
        }

        ImGui::EndColumns();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<ColumnsTestingVars>();

        ctx->SetRef("Test window");
        ImGuiWindow* window = ctx->GetWindowByRef("");
        ImGuiOldColumns* columns = ImGui::FindOrCreateColumns(window, vars.ColumnsID);

        vars.Flags = ImGuiOldColumnFlags_None;
        vars.Count = -1;
        ctx->Yield(5);        // Run tests in GuiFunc.
        float w0 = columns->Columns[0].ClipRect.GetWidth();
        float w1 = columns->Columns[1].ClipRect.GetWidth();
        ctx->ItemDragWithDelta(columns->ID + 1, ImVec2(10.0f, 0.0f)); // Resizing
        IM_CHECK_EQ(columns->Columns[0].ClipRect.GetWidth(), w0 + 10.0f);
        IM_CHECK_EQ(columns->Columns[1].ClipRect.GetWidth(), w1);

        vars.Flags = ImGuiOldColumnFlags_NoPreserveWidths;
        vars.Count = -1;
        ctx->Yield(5);        // Run tests in GuiFunc.
        w0 = columns->Columns[0].ClipRect.GetWidth();
        w1 = columns->Columns[1].ClipRect.GetWidth();
        ctx->ItemDragWithDelta(columns->ID + 1, ImVec2(10.0f, 0.0f)); // Resizing
        IM_CHECK_EQ(columns->Columns[0].ClipRect.GetWidth(), w0 + 10.0f);
        IM_CHECK_EQ(columns->Columns[1].ClipRect.GetWidth(), w1 - 10.0f);
    };

#if IMGUI_VERSION_NUM >= 18805
    // ## Test SameLine() before a row change
    t = IM_REGISTER_TEST(e, "columns", "columns_sameline_before_nextrow");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Hello");

        ImGui::BeginColumns("columns1", 3);

        {
            ImGui::CollapsingHeader("AAAA");
            ImGui::SameLine();
            ImGui::Text("*1");
            ImGui::NextColumn();
            ImGui::Text("BBBB"); ImGui::NextColumn();
            ImGui::Text("CCCC\nTwoLines\nThreeLines");
        }
        ImGui::NextColumn();

        {
            ImGui::CollapsingHeader("AAAA2");
            ImVec2 p1 = ImGui::GetItemRectMin();
            ImGui::SameLine();
            ImGui::Text("*2");
            ImVec2 p2 = ImGui::GetItemRectMin();
            IM_CHECK_EQ(p1.y, p2.y);
            ImGui::NextColumn();
            ImGui::Text("BBBB2"); ImGui::NextColumn();
            ImGui::Text("CCCC2\nTwoLines\nThreeLines");
            ImGui::SameLine(); // <---- Note the extraneous SameLine here
        }
        ImGui::NextColumn();

        // Note: Y Position will look wrong here: this was always the case with Columns:
        // - they never used CursorMaxPos.y, only the current CursorPos.y value during cell change, so this is not a new bug.

        {
            ImGui::CollapsingHeader("AAAA3");
            ImVec2 p1 = ImGui::GetItemRectMin();
            ImGui::SameLine();
            ImGui::Text("*3");
            ImVec2 p2 = ImGui::GetItemRectMin();
            IM_CHECK_EQ(p1.y, p2.y); // On 2022/07/18 we fixed previous row SameLine() leaking into this
            ImGui::NextColumn();
            ImGui::Text("BBBB3"); ImGui::NextColumn();
            ImGui::Text("CCCC3\nTwoLines\nThreeLines");
        }
        ImGui::NextColumn();

        ImGui::EndColumns();
        ImGui::End();
        ImGui::PopStyleVar();
    };
#endif
}
