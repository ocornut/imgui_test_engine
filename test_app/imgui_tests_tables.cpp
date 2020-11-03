// dear imgui
// (tests: tables, columns)

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

//-------------------------------------------------------------------------
// Tests: Tables
//-------------------------------------------------------------------------

#ifdef IMGUI_HAS_TABLE

static ImGuiTableColumn* HelperTableFindColumnByName(ImGuiTable* table, const char* name)
{
    for (int i = 0; i < table->Columns.size(); i++)
        if (strcmp(ImGui::TableGetColumnName(table, i), name) == 0)
            return &table->Columns[i];
    return NULL;
}

static void HelperTableSubmitCellsCustom(ImGuiTestContext* ctx, int count_w, int count_h, void(*cell_cb)(ImGuiTestContext* ctx, int column, int line))
{
    IM_ASSERT(cell_cb != NULL);
    for (int line = 0; line < count_h; line++)
    {
        ImGui::TableNextRow();
        for (int column = 0; column < count_w; column++)
        {
            if (!ImGui::TableSetColumnIndex(column)) // FIXME-TABLE
                continue;
            cell_cb(ctx, column, line);
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
    table_flags |= ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Borders | ImGuiTableFlags_NoSavedSettings;

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
        else if (policy == 'a' || policy == 'A') { column_flags |= ImGuiTableColumnFlags_WidthAlwaysAutoResize; }
        else IM_ASSERT(0);
        ImGui::TableSetupColumn(Str16f("%c%d", policy, column + 1).c_str(), column_flags);
    }
    ImFont* font = FindFontByName("Roboto-Medium.ttf, 16px");
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
            if (policy == 'F') { column_desc = "Fixed"; }
            if (policy == 'W') { column_desc = "Stretch"; }
            if (policy == 'A') { column_desc = "Auto"; }
            ImGui::Text("%s %d,%d", column_desc, row, column);
        }
    }
    ImGui::EndTable();
}
#endif // #ifdef IMGUI_HAS_TABLE

void RegisterTests_Table(ImGuiTestEngine* e)
{
#ifdef IMGUI_HAS_TABLE
    ImGuiTest* t = NULL;

    // ## Table: measure draw calls count
    // FIXME-TESTS: Resize window width to e.g. ideal size first, then resize down
    // Important: HelperTableSubmitCells uses Button() with -1 width which will CPU clip text, so we don't have interference from the contents here.
    t = IM_REGISTER_TEST(e, "table", "table_draw_calls");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(3, 3));
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Checkbox("Enable checks", &vars.Bool1);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImGui::Text("Text before");
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_NoClip | ImGuiTableFlags_Borders, ImVec2(400, 0)))
            {
                HelperTableSubmitCellsButtonFill(4, 5);
                ImGui::EndTable();
            }
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            if (vars.Bool1)
                IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("table2", 4, ImGuiTableFlags_Borders, ImVec2(400, 0)))
            {
                HelperTableSubmitCellsButtonFill(4, 4);
                ImGui::EndTable();
            }
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            if (vars.Bool1)
                IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("table3", 4, ImGuiTableFlags_Borders, ImVec2(400, 0)))
            {
                ImGui::TableSetupColumn("One");
                ImGui::TableSetupColumn("TwoTwo");
                ImGui::TableSetupColumn("ThreeThreeThree");
                ImGui::TableSetupColumn("FourFourFourFour");
                ImGui::TableHeadersRow();
                HelperTableSubmitCellsButtonFill(4, 4);
                ImGui::EndTable();
            }
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            if (vars.Bool1)
                IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("table4", 3, ImGuiTableFlags_Borders))
            {
                ImGui::TableSetupColumn("One");
                ImGui::TableSetupColumn("TwoTwo");
                ImGui::TableSetupColumn("ThreeThreeThree");
                //ImGui::TableHeadersRow();
                HelperTableSubmitCellsButtonFill(3, 4);
                ImGui::EndTable();
            }
            ImGui::Text("Some text");;
            int cmd_size_after = draw_list->CmdBuffer.Size;
            if (vars.Bool1)
                IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Test with/without clipping
        auto& vars = ctx->GenericVars;
        vars.Bool1 = false;
        ctx->WindowResize("Test window 1", ImVec2(500, 600));
        vars.Bool1 = true;
        ctx->Yield();
        ctx->Yield();
        vars.Bool1 = false;
        ctx->WindowResize("Test window 1", ImVec2(10, 600));
        vars.Bool1 = true;
        ctx->Yield();
    };

    // ## Table: test effect of specifying a width in TableSetupColumn()
    t = IM_REGISTER_TEST(e, "table", "table_width_explicit");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));
        if (ImGui::BeginTable("table1", 4))
        {
            ImGui::TableSetupColumn("One", ImGuiTableColumnFlags_WidthFixed, 100.0f, 0);
            ImGui::TableSetupColumn("Two");
            ImGui::TableSetupColumn("Three");
            ImGui::TableSetupColumn("Four");
            HelperTableSubmitCellsButtonFill(4, 5);
            ImGuiTable* table = ctx->UiContext->CurrentTable;
            IM_CHECK_EQ(table->Columns[0].WidthRequest, 100.0f);
            ImGui::EndTable();
        }
        ImGui::End();
    };

    // ## Table: measure equal width
    t = IM_REGISTER_TEST(e, "table", "table_width_distrib");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        struct TestCase
        {
            int             ColumnCount;
            ImGuiTableFlags Flags;
        };

        TestCase test_cases[] =
        {
            { 2, ImGuiTableFlags_None },
            { 3, ImGuiTableFlags_None },
            { 9, ImGuiTableFlags_None },
            { 2, ImGuiTableFlags_BordersOuter },
            { 3, ImGuiTableFlags_BordersOuter },
            { 9, ImGuiTableFlags_BordersOuter },
            { 2, ImGuiTableFlags_BordersV },
            { 3, ImGuiTableFlags_BordersV },
            { 9, ImGuiTableFlags_BordersV },
            { 2, ImGuiTableFlags_Borders },
            { 3, ImGuiTableFlags_Borders },
            { 9, ImGuiTableFlags_Borders },
        };

        ImGui::Text("(width variance should be <= 1.0f)");
        for (int test_case_n = 0; test_case_n < IM_ARRAYSIZE(test_cases); test_case_n++)
        {
            const TestCase& tc = test_cases[test_case_n];
            ImGui::PushID(test_case_n);

            ImGui::Spacing();
            ImGui::Spacing();
            if (ImGui::BeginTable("table1", tc.ColumnCount, tc.Flags, ImVec2(0, 0)))
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
                    ImGui::Text("Width %.2f", w);
                }
                float w_variance = max_w - min_w;
                IM_CHECK_LE_NO_RET(w_variance, 1.0f);
                ImGui::EndTable();

                if (w_variance > 1.0f)
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
                ImGui::Text("#%02d: Variance %.2f (min %.2f max %.2f)", test_case_n, w_variance, min_w, max_w);
                if (w_variance > 1.0f)
                    ImGui::PopStyleColor();
            }
            ImGui::PopID();
        }

        ImGui::End();
    };

    // ## Test Padding
    t = IM_REGISTER_TEST(e, "table", "table_padding");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GenericVars;

        const float cell_padding = g.Style.CellPadding.x;
        const float border_size = 1.0f;

        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetNextItemWidth(30.0f);
        ImGui::SliderInt("Step", &vars.Step, 0, 3);

        ImGuiTableFlags table_flags = ImGuiTableFlags_SizingPolicyFixedX;
        if (vars.Step == 0)
        {
            vars.Width = 50.0f + 100.0f + cell_padding;
        }
        if (vars.Step == 1)
        {
            table_flags |= ImGuiTableFlags_BordersInnerV;
            vars.Width = 50.0f + 100.0f + cell_padding * 2.0f + border_size;
        }
        if (vars.Step == 2)
        {
            table_flags |= ImGuiTableFlags_BordersOuterV;
            vars.Width = 50.0f + 100.0f + cell_padding * 3.0f + border_size * 2.0f;
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
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GenericVars;
        ctx->SetRef("Test window 1");
        ImGuiWindow* window = ctx->GetWindowByRef("");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));
        for (int step = 0; step < 4; step++)
        {
            ctx->LogDebug("Step %d", step);
            vars.Step = step;
            ctx->Yield();
            ctx->Yield();
            IM_CHECK_EQ(table->Columns[0].ContentMaxXUnfrozen - table->Columns[0].ContentMinX, 50.0f);
            IM_CHECK_EQ(table->Columns[1].ContentMaxXUnfrozen - table->Columns[1].ContentMinX, 100.0f);
            IM_CHECK_EQ(table->ColumnsAutoFitWidth, vars.Width);
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
        IM_CHECK_EQ(ImGui::TableGetColumnIsVisible(0), false);
        IM_CHECK_EQ(ImGui::TableGetColumnIsSorted(0), false);
        IM_CHECK_EQ(ImGui::TableGetColumnName(), (const char*)NULL);
        ImGui::End();
    };

    // ## Resizing test-bed (not an actual automated test)
    t = IM_REGISTER_TEST(e, "table", "table_resizing_behaviors");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowPos(ctx->GetMainViewportPos() + ImVec2(20, 5), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(400.0f, 0.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        ImGui::BulletText("OK: Resize from F1| or F2|");    // ok: alter ->WidthRequested of Fixed column. Subsequent columns will be offset.
        ImGui::BulletText("OK: Resize from F3|");           // ok: alter ->WidthRequested of Fixed column. If active, ScrollX extent can be altered.
        HelperTableWithResizingPolicies("table1", 0, "FFF");
        ImGui::Spacing();

        ImGui::BulletText("OK: Resize from F1| or F2|");    // ok: alter ->WidthRequested of Fixed column. If active, ScrollX extent can be altered, but it doesn't make much sense as the Weighted column will always be minimal size.
        ImGui::BulletText("OK: Resize from W3| (off)");     // ok: no-op (disabled by Rule A)
        HelperTableWithResizingPolicies("table2", 0, "FFW");
        ImGui::Spacing();

        ImGui::BulletText("KO: Resize from W1| or W2|");    // FIXME: not implemented
        ImGui::BulletText("OK: Resize from W3| (off)");     // ok: no-op (disabled by Rule A)
        HelperTableWithResizingPolicies("table3", 0, "WWWw");
        ImGui::Spacing();

        // Need F2w + F3w to be stable to avoid moving W1
        // lock F2L
        // move F2R
        // move F3L
        // lock F3R
        ImGui::BulletText("OK: Resize from W1| (fwd)");     // ok: forward to resizing |F2. F3 will not move.
        ImGui::BulletText("KO: Resize from F2| or F3|");    // FIXME should resize F2, F3 and not have effect on W1 (Weighted columns are _before_ the Fixed column).
        ImGui::BulletText("OK: Resize from F4| (off)");     // ok: no-op (disabled by Rule A)
        HelperTableWithResizingPolicies("table4", 0, "WFFF");
        ImGui::Spacing();

        ImGui::BulletText("OK: Resize from W1| (fwd)");     // ok: forward to resizing |F2
        ImGui::BulletText("OK: Resize from F2| (off)");     // ok: no-op (disabled by Rule A)
        HelperTableWithResizingPolicies("table5", 0, "WF");
        ImGui::Spacing();

        ImGui::BulletText("KO: Resize from W1|");           // FIXME
        ImGui::BulletText("KO: Resize from W2|");           // FIXME
        HelperTableWithResizingPolicies("table6", 0, "WWF");
        ImGui::Spacing();

        ImGui::BulletText("KO: Resize from W1|");           // FIXME
        ImGui::BulletText("KO: Resize from F2|");           // FIXME
        HelperTableWithResizingPolicies("table7", 0, "WFW");
        ImGui::Spacing();

        ImGui::BulletText("OK: Resize from W2| (fwd)");     // ok: forward
        HelperTableWithResizingPolicies("table8", 0, "FWF");
        ImGui::Spacing();

        ImGui::BulletText("OK: Resize from ");
        HelperTableWithResizingPolicies("table9", 0, "WWFWW");
        ImGui::Spacing();

        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiTable* table = NULL;
        ImVector<float> initial_column_width;

        ctx->SetRef("Test window 1");
        table = ImGui::TableFindByID(ctx->GetID("table1"));    // Columns: FFF, do not span entire width of the table
        IM_CHECK_LT(table->ColumnsTotalWidth + 1.0f, table->InnerWindow->ContentRegionRect.GetWidth());
        initial_column_width.resize(table->ColumnsCount);
        for (int column_n = 0; column_n >= 0; column_n = table->Columns[column_n].NextVisibleColumn)
        {
            const ImGuiTableColumn* col_curr = &table->Columns[column_n];
            const ImGuiTableColumn* col_prev = col_curr->PrevVisibleColumn >= 0 ? &table->Columns[col_curr->PrevVisibleColumn] : NULL;
            const ImGuiTableColumn* col_next = col_curr->NextVisibleColumn >= 0 ? &table->Columns[col_curr->NextVisibleColumn] : NULL;
            const float width_curr = col_curr->WidthGiven;
            const float width_prev = col_prev ? col_prev->WidthGiven : 0;
            const float width_next = col_next ? col_next->WidthGiven : 0;
            const float width_total = table->ColumnsTotalWidth;
            initial_column_width[column_n] = col_curr->WidthGiven;              // Save initial column size for next test

            // Resize a column
            // FIXME-TESTS: higher level helpers.
            const float move_by = -30.0f;
            ImGuiID handle_id = ImGui::TableGetColumnResizeID(table, column_n);
            ctx->ItemDragWithDelta(handle_id, ImVec2(move_by, 0));

            IM_CHECK(!col_prev || col_prev->WidthGiven == width_prev);      // Previous column width does not change
            IM_CHECK(col_curr->WidthGiven == width_curr + move_by);         // Current column expands
            IM_CHECK(!col_next || col_next->WidthGiven == width_next);      // Next column width does not change
            IM_CHECK(table->ColumnsTotalWidth == width_total + move_by);    // Empty space after last column shrinks
        }
        IM_CHECK(table->ColumnsTotalWidth + 1 < table->InnerWindow->ContentRegionRect.GetWidth());  // All columns span entire width of the table

        // Test column fitting
        {
            // Ensure columns are smaller than their contents due to previous tests on table1
            for (int column_n = 0; column_n >= 0; column_n = table->Columns[column_n].NextVisibleColumn)
                IM_CHECK(table->Columns[column_n].WidthGiven < initial_column_width[column_n]);

            // Fit right-most column
            int column_n = table->RightMostVisibleColumn;
            const ImGuiTableColumn* col_curr = &table->Columns[column_n];

            // Fit column.
            ctx->SetRef("Test window 1");
            ctx->ItemClick(TableGetHeaderID(table, "F3"), ImGuiMouseButton_Right);
            ctx->SetRef(g.NavWindow);
            ctx->ItemClick("Size column to fit");
            IM_CHECK(col_curr->WidthGiven == initial_column_width[column_n]);  // Column restored original size

            // Ensure columns other than right-most one were not affected
            for (column_n = 0; column_n >= 0 && column_n < table->RightMostVisibleColumn; column_n = table->Columns[column_n].NextVisibleColumn)
                IM_CHECK(table->Columns[column_n].WidthGiven < initial_column_width[column_n]);

            // Test fitting rest of the columns
            ctx->SetRef("Test window 1");
            ctx->ItemClick(TableGetHeaderID(table, "F3"), ImGuiMouseButton_Right);
            ctx->SetRef(g.NavWindow);
            ctx->ItemClick("Size all columns to fit");

            // Ensure all columns fit to contents
            for (column_n = 0; column_n >= 0; column_n = table->Columns[column_n].NextVisibleColumn)
                IM_CHECK(table->Columns[column_n].WidthGiven == initial_column_width[column_n]);
        }

        ctx->SetRef("Test window 1");
        table = ImGui::TableFindByID(ctx->GetID("table2"));     // Columns: FFW, do span entire width of the table
        IM_CHECK(table->ColumnsTotalWidth == table->InnerWindow->ContentRegionRect.GetWidth());

        // Iterate visible columns and check existence of resize handles
        for (int column_n = 0; column_n >= 0; column_n = table->Columns[column_n].NextVisibleColumn)
        {
            ImGuiID handle_id = ImGui::TableGetColumnResizeID(table, column_n);
            if (column_n == table->RightMostVisibleColumn)
                IM_CHECK(ctx->ItemInfo(handle_id, ImGuiTestOpFlags_NoError) == NULL); // W
            else
                IM_CHECK(ctx->ItemInfo(handle_id, ImGuiTestOpFlags_NoError) != NULL); // FF
        }
        IM_CHECK(table->ColumnsTotalWidth == table->InnerWindow->ContentRegionRect.GetWidth());
    };

    // ## Test Visible flag
    t = IM_REGISTER_TEST(e, "table", "table_clip");
    t->Flags |= ImGuiTestFlags_NoAutoFinish;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_ScrollX | ImGuiTableFlags_Borders, ImVec2(200, 200)))
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

    // ## Test that BeginTable/EndTable with no contents doesn't fail
    t = IM_REGISTER_TEST(e, "table", "table_empty");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::BeginTable("table1", 3);
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
        ImGui::BeginTable("table1", 4, ImGuiTableFlags_MultiSortable);
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

    // ## Test using the maximum of 64 columns (#3058)
    t = IM_REGISTER_TEST(e, "table", "table_max_columns");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        //ImDrawList* cmd = ImGui::GetWindowDrawList();
        ImGui::BeginTable("table1", 64);
        for (int n = 0; n < 64; n++)
            ImGui::TableSetupColumn("Header");
        ImGui::TableHeadersRow();
        for (int i = 0; i < 10; i++)
        {
            ImGui::TableNextRow();
            for (int n = 0; n < 64; n++)
            {
                ImGui::TableNextColumn();
                ImGui::Text("Data");
            }
        }
        ImGui::EndTable();
        ImGui::End();
    };

    // ## Test rendering two tables with same ID.
    t = IM_REGISTER_TEST(e, "table", "table_multi_instances");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        const int col_count = 3;
        float column_widths[col_count] = {};
        for (int i = 0; i < 2; i++)
        {
            ImGui::BeginTable("table1", col_count, ImGuiTableFlags_NoSavedSettings|ImGuiTableFlags_Resizable|ImGuiTableFlags_Borders);
            for (int c = 0; c < col_count; c++)
                ImGui::TableSetupColumn("Header", c ? ImGuiTableColumnFlags_WidthFixed : ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            for (int r = 0; r < 10; r++)
            {
                ImGui::TableNextRow();
                for (int c = 0; c < col_count; c++)
                {
                    // Second table contains larger data, attempting to confuse column sync.
                    ImGui::TableNextColumn();
                    ImGui::Text(i ? "Long Data" : "Data");
                }
            }

            if (ctx->IsFirstTestFrame() || ctx->GenericVars.Bool1)
            {
                // Perform actual test.
                ImGuiContext& g = *ctx->UiContext;
                ImGuiTable* table = g.CurrentTable;

                if (i == 0)
                {
                    // Save column widths of table during first iteration.
                    for (int c = 0; c < col_count; c++)
                        column_widths[c] = table->Columns[c].WidthGiven;
                }
                else
                {
                    // Verify column widths match during second iteration.
                    for (int c = 0; c < col_count; c++)
                        IM_CHECK(column_widths[c] == table->Columns[c].WidthGiven);
                }
            }

            ImGui::EndTable();
        }

        ctx->GenericVars.Bool1 = false;
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test window 1");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));

        for (int instance_no = 0; instance_no < 2; instance_no++)
        {
            // Resize a column in the second table. It is not important whether we increase or reduce column size.
            // Changing direction ensures resize happens around the first third of the table and does not stick to
            // either side of the table across multiple test runs.
            float direction = (table->ColumnsTotalWidth * 0.3f) < table->Columns[0].WidthGiven ? -1.0f : 1.0f;
            float length = 30.0f + 10.0f * instance_no; // Different length for different table instances
            ctx->ItemDragWithDelta(ImGui::TableGetColumnResizeID(table, 0, instance_no), ImVec2(length * direction, 0.0f));
            ctx->GenericVars.Bool1 = true;      // Retest again
            ctx->Yield();                       // Render one more frame to retest column widths
        }
    };

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
        ctx->SleepShort();
        const float tooltip_width = ctx->GenericVars.Float1;
        for (int n = 0; n < 3; n++)
            IM_CHECK_EQ(ctx->GenericVars.Float1, tooltip_width);
    };

    // ## Test saving and loading table settings.
    t = IM_REGISTER_TEST(e, "table", "table_settings");
    struct TableSettingsVars { ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoSavedSettings; bool call_get_sort_specs = false; };
    t->SetUserDataType<TableSettingsVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        TableSettingsVars& vars = ctx->GetUserData<TableSettingsVars>();

        const int column_count = 4;
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Table Settings", NULL, vars.window_flags);

        if (ImGui::BeginTable("table1", column_count, ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable))
        {
            ImGui::TableSetupColumn("Col1");
            ImGui::TableSetupColumn("Col2");
            ImGui::TableSetupColumn("Col3", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Col4", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            if (vars.call_get_sort_specs) // Test against TableGetSortSpecs() having side effects
                ImGui::TableGetSortSpecs();
            HelperTableSubmitCellsButtonFill(column_count, 3);
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TableSettingsVars& vars = ctx->GetUserData<TableSettingsVars>();

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

                IM_CHECK_EQ_NO_RET(table->Columns[0].IsVisible, true);
                IM_CHECK_EQ_NO_RET(table->Columns[1].IsVisible, true);
                IM_CHECK_EQ_NO_RET(table->Columns[2].IsVisible, true);
                IM_CHECK_EQ_NO_RET(table->Columns[3].IsVisible, true);

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
                IM_CHECK_EQ_NO_RET(table->Columns[1].IsVisible, !col1_hidden);
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
                // FIXME-TESTS: Later we should try to simulate inputs at user level
                table->Columns[1].IsVisible = table->Columns[1].IsVisibleNextFrame = false;
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
            }

            if (col3_resized)
            {
                // FIXME-TESTS: Later we should try to simulate inputs at user level
                table->Columns[3].StretchWeight = 0.2f;
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

    // ## Test table sorting behaviors.
    t = IM_REGISTER_TEST(e, "table", "table_sorting");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        int& table_flags = ctx->GenericVars.Int1;
        ImGui::SetNextWindowSize(ImVec2(600, 80), ImGuiCond_Appearing); // FIXME-TESTS: Why?
        ImGui::Begin("Test window", NULL, ImGuiWindowFlags_NoSavedSettings);

        if (ctx->IsFirstTestFrame())
        {
            table_flags = ImGuiTableFlags_Sortable | ImGuiTableFlags_MultiSortable;
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));
        }

        if (ImGui::BeginTable("table1", 6, table_flags))
        {
            ImGui::TableSetupColumn("Default", g.CurrentTable->Columns[0].FlagsIn);
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
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));
        const ImGuiTableSortSpecs* sort_specs = NULL;
        int& table_flags = ctx->GenericVars.Int1;

        // Clicks a column header, optionally holding a specified modifier key. Returns SortDirection of clicked column.
        auto click_column_and_get_sort = [&](const char* label, ImGuiKeyModFlags click_mod = ImGuiKeyModFlags_None) -> ImGuiSortDirection
        {
            IM_ASSERT(ctx != NULL);
            IM_ASSERT(table != NULL);
            IM_ASSERT(label != NULL);

            ImGuiTableColumn* column = HelperTableFindColumnByName(table, label);
            IM_CHECK_RETV(column != NULL, ImGuiSortDirection_None);

            if (click_mod != ImGuiKeyModFlags_None)
                ctx->KeyDownMap(ImGuiKey_COUNT, click_mod);
            ctx->ItemClick(TableGetHeaderID(table, label), ImGuiMouseButton_Left);
            if (click_mod != ImGuiKeyModFlags_None)
                ctx->KeyUpMap(ImGuiKey_COUNT, click_mod);
            return column->SortDirection;
        };

        // Calls ImGui::TableGetSortSpecs() and returns it's result.
        auto table_get_sort_specs = [](ImGuiTestContext* ctx, ImGuiTable* table)
        {
            ImGuiContext& g = *ctx->UiContext;
            ImSwap(table, g.CurrentTable);
            const ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
            ImSwap(table, g.CurrentTable);
            return sort_specs;
        };

        // Table has no default sorting flags. Check for implicit default sorting.
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs != NULL);
        IM_CHECK_EQ(sort_specs->SpecsCount, 1);
        IM_CHECK_EQ(sort_specs->ColumnsMask, 0x01u);
        IM_CHECK_EQ(sort_specs->Specs[0].ColumnIndex, 0);
        IM_CHECK_EQ(sort_specs->Specs[0].SortOrder, 0);
        IM_CHECK_EQ(sort_specs->Specs[0].SortDirection, ImGuiSortDirection_Ascending);

        IM_CHECK_EQ(click_column_and_get_sort("Default"), ImGuiSortDirection_Descending);   // Sorted implicitly by calling TableGetSortSpecs().
        IM_CHECK_EQ(click_column_and_get_sort("Default"), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(click_column_and_get_sort("PreferSortAscending"), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(click_column_and_get_sort("PreferSortAscending"), ImGuiSortDirection_Descending);

        // Not holding shift does not perform multi-sort.
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs != NULL);
        IM_CHECK_EQ(sort_specs->SpecsCount, 1);

        // Holding shift includes all sortable columns in multi-sort.
        IM_CHECK_EQ(click_column_and_get_sort("PreferSortDescending", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Descending);
        IM_CHECK_EQ(click_column_and_get_sort("PreferSortDescending", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Ascending);
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 2);

        IM_CHECK_EQ(click_column_and_get_sort("NoSort", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(click_column_and_get_sort("NoSort", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Ascending);
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 2);

        IM_CHECK_EQ(click_column_and_get_sort("NoSortAscending", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Descending);
        IM_CHECK_EQ(click_column_and_get_sort("NoSortAscending", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Descending);
        IM_CHECK_EQ(click_column_and_get_sort("NoSortDescending", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(click_column_and_get_sort("NoSortDescending", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Ascending);
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 4);

        // Disable multi-sort and ensure there is only one sorted column left.
        table_flags = ImGuiTableFlags_Sortable;
        ctx->Yield();
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs != NULL);
        IM_CHECK_EQ(sort_specs->SpecsCount, 1);

        // Disable sorting completely. Sort spec should not be returned.
        table_flags = ImGuiTableFlags_None;
        ctx->Yield();
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs == NULL);

        // Test updating sorting direction on column flag change.
        ImGuiTableColumn* col = &table->Columns[0];
        table_flags = ImGuiTableFlags_Sortable;
        col->FlagsIn = ImGuiTableColumnFlags_NoSortAscending | ImGuiTableColumnFlags_NoSortDescending;
        ctx->Yield();
        IM_CHECK((col->Flags & ImGuiTableColumnFlags_NoSort) != 0);

        col->SortDirection = ImGuiSortDirection_Ascending;
        col->FlagsIn = ImGuiTableColumnFlags_NoSortAscending;
        ctx->Yield();
        IM_CHECK_EQ(col->SortDirection, ImGuiSortDirection_Descending);

        col->SortDirection = ImGuiSortDirection_Descending;
        col->FlagsIn = ImGuiTableColumnFlags_NoSortDescending;
        ctx->Yield();
        IM_CHECK_EQ(col->SortDirection, ImGuiSortDirection_Ascending);
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
        ctx->ScrollToX(0.0f);
        ctx->ScrollToY(0.0f);
        ctx->SetRef(table->InnerWindow);
        ctx->ScrollToX(0.0f);
        ctx->ScrollToY(0.0f);
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
            IM_CHECK_EQ(table->Columns[i].IsClipped, false);
            IM_CHECK_EQ(ctx->GenericVars.BoolArray[i], true);
        }

        // Scroll to the bottom-right of the table.
        ctx->ScrollToX(table->InnerWindow->ScrollMax.x);
        ctx->ScrollToY(table->InnerWindow->ScrollMax.y);
        ctx->Yield();

        // First five columns and rows are no longer visible
        for (int i = 0; i < 5; i++)
        {
            IM_CHECK_EQ(table->Columns[i].IsClipped, true);
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
                IM_CHECK_EQ(table->Columns[column_n].IsClipped, !(column_n < freeze_count));
        }
    };

    // ## Test window with _AlwaysAutoResize getting resized to size of a table it contains.
    t = IM_REGISTER_TEST(e, "table", "table_auto_resize");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GenericVars;
        if (ctx->IsFirstGuiFrame())
            vars.Count = 5;

        //ImGui::SetNextWindowSize(ImVec2(20, 20), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | vars.WindowFlags);
        ImGui::CheckboxFlags("ImGuiWindowFlags_AlwaysAutoResize", (unsigned int*)&vars.WindowFlags, ImGuiWindowFlags_AlwaysAutoResize);
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
            if (vars.Step == 0)
            {
                for (int i = 0; i < col_row_count; i++)
                    ImGui::TableSetupColumn(Str16f("Col%d", i).c_str(), ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableHeadersRow();
                HelperTableSubmitCellsButtonFill(col_row_count, col_row_count);
            }
            if (vars.Step == 1)
            {
                for (int i = 0; i < col_row_count; i++)
                    ImGui::TableSetupColumn(Str16f("Col%d", i).c_str(), ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableHeadersRow();
                HelperTableSubmitCellsButtonFix(col_row_count, col_row_count);
            }
            if (vars.Step == 2)
            {
                // No column width + window auto-fit + auto-fill button creates a feedback loop
                for (int i = 0; i < col_row_count; i++)
                    ImGui::TableSetupColumn(Str16f("Col%d", i).c_str());
                ImGui::TableHeadersRow();
                HelperTableSubmitCellsButtonFill(col_row_count, col_row_count);
            }
            if (vars.Step == 3)
            {
                for (int i = 0; i < col_row_count; i++)
                    ImGui::TableSetupColumn(Str16f("Col%d", i).c_str());
                ImGui::TableHeadersRow();
                HelperTableSubmitCellsButtonFix(col_row_count, col_row_count);
            }
            vars.Width = g.CurrentTable->OuterRect.GetWidth();
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        auto& vars = ctx->GenericVars;

        ctx->SetRef("Test Window");
        ImGuiWindow* window = ctx->GetWindowByRef(ctx->RefID);
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

                float expected_table_width = vars.Count * 100.0f + (vars.Count - 1) * g.Style.CellPadding.x;
                if (step == 2)
                {
                    // No columns width specs + auto-filling button = table will keep its width
                    IM_CHECK_LT(table->OuterRect.GetWidth(), expected_table_width);
                    IM_CHECK_LT(window->ContentSize.x, expected_table_width);
                }
                else
                {
                    IM_CHECK_EQ(table->OuterRect.GetWidth(), expected_table_width);
                    IM_CHECK_EQ(window->ContentSize.x, expected_table_width);
                }

                vars.Count++;
                ctx->Yield(); // Changing column count = recreate table. The whole thing will look glitchy at Step==3
                ctx->Yield();
            }
        }
    };

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
                    if ((column < 2 && window->Scroll.x > 0.0f) || (line < 2 && window->Scroll.y > 0.0f))
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
        ctx->ScrollToX(0.0f);
        ctx->ScrollToY(0.0f);
        ctx->YieldFrames(2);
        ctx->ScrollToX(table->InnerWindow->ScrollMax.x);
        ctx->ScrollToY(table->InnerWindow->ScrollMax.y);
        ctx->YieldFrames(2);
    };

    // ## Miscellaneous
    t = IM_REGISTER_TEST(e, "table", "table_cov_misc");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGui::SetNextWindowSize(ImVec2(250.0f, 100.0f), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        if (ctx->IsFirstGuiFrame())
            TableDiscardInstanceAndSettings(ImGui::GetID("table1"));

        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_ScrollX | ImGuiTableFlags_Hideable | ImGuiTableFlags_Resizable, ImVec2(0, 0), 300.0f))
        {
            ImGuiTable* table = g.CurrentTable;
            if (ctx->IsFirstGuiFrame())
                table->Columns[0].FlagsIn = ImGuiTableColumnFlags_WidthFixed;

            ImGui::TableSetupColumn("One", table->Columns[0].FlagsIn, 100.0f, 0);
            ImGui::TableSetupColumn("Two", table->Columns[1].FlagsIn);
            ImGui::TableSetupColumn("Three", table->Columns[2].FlagsIn);
            ImGui::TableSetupColumn(NULL, table->Columns[3].FlagsIn);
            ImGui::TableHeadersRow();
            HelperTableSubmitCellsText(4, 5);
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ctx->SetRef("Test window 1");
        ImGuiTable* table = ImGui::TableFindByID(ctx->GetID("table1"));
        ImGuiTableColumn* col0 = &table->Columns[0];
        ImGuiTableColumn* col1 = &table->Columns[1];
        IM_CHECK_EQ(col0->WidthRequest, 100.0f);
        IM_CHECK_EQ(col0->WidthGiven, 100.0f);

        // Test column naming.
        IM_CHECK_STR_EQ(ImGui::TableGetColumnName(table, 2), "Three");
        IM_CHECK_STR_EQ(ImGui::TableGetColumnName(table, 3), "");

        // Test inner_width parameter of BeginTable().
        IM_CHECK(table->InnerWidth == 300.0f);
        IM_CHECK(table->InnerWidth == table->InnerWindow->ContentSize.x);   // Only true when inner window is used because of ImGuiTableFlags_ScrollX

        // Test column fitting.
        col0->WidthRequest = (col0->ContentMaxXHeadersIdeal - col0->ContentMinX) + 50.0f;
        ctx->Yield();
        IM_CHECK_GT(col0->WidthGiven, col0->ContentMaxXHeadersIdeal - col0->ContentMinX);
        ctx->MouseMoveToPos(ImVec2(col0->MaxX, table->InnerWindow->Pos.y));
        ctx->MouseDoubleClick();
        ctx->Yield();
        IM_CHECK_EQ(col0->WidthGiven, 100.0f);  // Resets to initial width because column os fixed.

        col1->WidthRequest = (col1->ContentMaxXHeadersIdeal - col1->ContentMinX) + 50.0f;
        ctx->Yield();
        IM_CHECK_GT(col1->WidthGiven, col1->ContentMaxXHeadersIdeal - col1->ContentMinX);
        ctx->MouseMoveToPos(ImVec2(col1->MaxX, table->InnerWindow->Pos.y));
        ctx->MouseDoubleClick();
        ctx->Yield();
        IM_CHECK_EQ(col1->WidthGiven, col1->ContentMaxXHeadersIdeal - col1->ContentMinX);  // Resets to ideal width because column is resizable.

        // Test clearing of resizable flag when no columns are resizable.
        IM_CHECK((table->Flags & ImGuiTableFlags_Resizable) != 0);
        for (int i = 0; i < table->Columns.size(); i++)
            table->Columns[i].FlagsIn |= ImGuiTableColumnFlags_NoResize;
        ctx->Yield();
        IM_CHECK((table->Flags & ImGuiTableFlags_Resizable) == 0);

        // Test column hiding.
        ctx->ItemClick(TableGetHeaderID(table, "One"), ImGuiMouseButton_Right);
        ctx->SetRef(g.NavWindow);
        ctx->ItemClick("One");
        IM_CHECK(col0->IsVisible == false);
        ctx->ItemClick("One");
        IM_CHECK(col0->IsVisible == true);
    };


#else // #ifdef IMGUI_HAS_TABLE
    IM_UNUSED(e);
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
}

