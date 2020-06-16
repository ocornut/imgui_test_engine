// dear imgui
// (tests: tables, columns)

#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_tests.h"
#include "shared/imgui_utils.h"
#include "test_engine/imgui_te_core.h"      // IM_REGISTER_TEST()
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

static void HelperTableSubmitCells(int count_w, int count_h)
{
    for (int line = 0; line < count_h; line++)
    {
        ImGui::TableNextRow();
        for (int column = 0; column < count_w; column++)
        {
            if (!ImGui::TableSetColumnIndex(column))
                continue;
            Str16f label("%d,%d", line, column);
            //ImGui::TextUnformatted(label.c_str());
            ImGui::Button(label.c_str(), ImVec2(-FLT_MIN, 0.0f));
        }
    }
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
    ImGui::TableAutoHeaders();
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

    t = IM_REGISTER_TEST(e, "table", "table_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::BeginTable("##table0", 4))
        {
            ImGui::TableSetupColumn("One", ImGuiTableColumnFlags_WidthFixed, 100.0f, 0);
            ImGui::TableSetupColumn("Two");
            ImGui::TableSetupColumn("Three");
            ImGui::TableSetupColumn("Four");
            HelperTableSubmitCells(4, 5);
            ImGuiTable* table = ctx->UiContext->CurrentTable;
            IM_CHECK_EQ(table->Columns[0].WidthRequest, 100.0f);
            ImGui::EndTable();
        }
        ImGui::End();
    };

    // ## Table: measure draw calls count
    // FIXME-TESTS: Resize window width to e.g. ideal size first, then resize down
    // Important: HelperTableSubmitCells uses Button() with -1 width which will CPU clip text, so we don't have interference from the contents here.
    t = IM_REGISTER_TEST(e, "table", "table_2_draw_calls");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImGui::Text("Text before");
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("##table1", 4, ImGuiTableFlags_NoClipX | ImGuiTableFlags_Borders, ImVec2(400, 0)))
            {
                HelperTableSubmitCells(4, 5);
                ImGui::EndTable();
            }
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("##table2", 4, ImGuiTableFlags_Borders, ImVec2(400, 0)))
            {
                HelperTableSubmitCells(4, 4);
                ImGui::EndTable();
            }
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("##table3", 4, ImGuiTableFlags_Borders, ImVec2(400, 0)))
            {
                ImGui::TableSetupColumn("One");
                ImGui::TableSetupColumn("TwoTwo");
                ImGui::TableSetupColumn("ThreeThreeThree");
                ImGui::TableSetupColumn("FourFourFourFour");
                ImGui::TableAutoHeaders();
                HelperTableSubmitCells(4, 4);
                ImGui::EndTable();
            }
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        {
            int cmd_size_before = draw_list->CmdBuffer.Size;
            if (ImGui::BeginTable("##table4", 3, ImGuiTableFlags_Borders))
            {
                ImGui::TableSetupColumn("One");
                ImGui::TableSetupColumn("TwoTwo");
                ImGui::TableSetupColumn("ThreeThreeThree");
                ImGui::TableAutoHeaders();
                HelperTableSubmitCells(3, 4);
                ImGui::EndTable();
            }
            ImGui::Text("Some text");
            int cmd_size_after = draw_list->CmdBuffer.Size;
            IM_CHECK_EQ(cmd_size_before, cmd_size_after);
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        // Test with/without clipping
        ctx->WindowResize("Test window 1", ImVec2(500, 600));
        ctx->WindowResize("Test window 1", ImVec2(10, 600));
    };

    // ## Table: measure equal width
    t = IM_REGISTER_TEST(e, "table", "table_3_width");
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
            if (ImGui::BeginTable("##table", tc.ColumnCount, tc.Flags, ImVec2(0, 0)))
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

    // ## Test behavior of some Table functions without BeginTable
    t = IM_REGISTER_TEST(e, "table", "table_4_functions_without_table");
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
    t = IM_REGISTER_TEST(e, "table", "table_5_resizing_behaviors");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowPos(ctx->GetMainViewportPos() + ImVec2(20, 5), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(400.0f, 0.0f), ImGuiCond_Once);
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
        ImVector<float> initial_col_size;

        ctx->WindowRef("Test window 1");
        table = ImGui::FindTableByID(ctx->GetID("table1"));    // Columns: FFF, do not span entire width of the table
        IM_CHECK(table->ColumnsTotalWidth + 1 < table->InnerWindow->ContentRegionRect.GetWidth());
        initial_col_size.resize(table->ColumnsCount);
        for (int column_n = 0; column_n >= 0; column_n = table->Columns[column_n].NextVisibleColumn)
        {
            const ImGuiTableColumn* col_curr = &table->Columns[column_n];
            const ImGuiTableColumn* col_prev = col_curr->PrevVisibleColumn >= 0 ? &table->Columns[col_curr->PrevVisibleColumn] : NULL;
            const ImGuiTableColumn* col_next = col_curr->NextVisibleColumn >= 0 ? &table->Columns[col_curr->NextVisibleColumn] : NULL;
            const float width_curr = col_curr->WidthGiven;
            const float width_prev = col_prev ? col_prev->WidthGiven : 0;
            const float width_next = col_next ? col_next->WidthGiven : 0;
            const float width_total = table->ColumnsTotalWidth;
            initial_col_size[column_n] = col_curr->WidthGiven;              // Save initial column size for next test

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
                IM_CHECK(table->Columns[column_n].WidthGiven < initial_col_size[column_n]);

            // Fit right-most column
            int column_n = table->RightMostVisibleColumn;
            const ImGuiTableColumn* col_curr = &table->Columns[column_n];

            // Fit column. ID calculation from BeginTableEx() and TableAutoHeaders()
            ImGuiID instance_id = (ImGuiID)(table->InstanceCurrent * table->ColumnsCount + column_n);   // pushed by TableAutoHeaders()
            ImGuiID table_id = ctx->GetIDByInt((int)(table->ID + (ImGuiID)table->InstanceCurrent));     // pushed by BeginTable()
            ImGuiID column_label_id = ctx->GetID("F3", ctx->GetIDByInt((int)instance_id, table_id));
            ctx->ItemClick(column_label_id, ImGuiMouseButton_Right);
            ctx->WindowRef(g.NavWindow);
            ctx->ItemClick("Size column to fit");
            IM_CHECK(col_curr->WidthGiven == initial_col_size[column_n]);  // Column restored original size

            // Ensure columns other than right-most one were not affected
            for (column_n = 0; column_n >= 0 && column_n < table->RightMostVisibleColumn; column_n = table->Columns[column_n].NextVisibleColumn)
                IM_CHECK(table->Columns[column_n].WidthGiven < initial_col_size[column_n]);

            // Test fitting rest of the columns
            ctx->ItemClick(column_label_id, ImGuiMouseButton_Right);
            ctx->WindowRef(g.NavWindow);
            ctx->ItemClick("Size all columns to fit");

            // Ensure all columns fit to contents
            for (column_n = 0; column_n >= 0; column_n = table->Columns[column_n].NextVisibleColumn)
                IM_CHECK(table->Columns[column_n].WidthGiven == initial_col_size[column_n]);

            ctx->WindowRef("Test window 1");                    // Restore previous ref
        }

        table = ImGui::FindTableByID(ctx->GetID("table2"));     // Columns: FFW, do span entire width of the table
        IM_CHECK(table->ColumnsTotalWidth + 1 == table->InnerWindow->ContentRegionRect.GetWidth());

        // Iterate visible columns and check existence of resize handles
        for (int column_n = 0; column_n >= 0; column_n = table->Columns[column_n].NextVisibleColumn)
        {
            ImGuiID handle_id = ImGui::TableGetColumnResizeID(table, column_n);
            if (column_n == table->RightMostVisibleColumn)
                IM_CHECK(ctx->ItemInfo(handle_id, ImGuiTestOpFlags_NoError) == NULL); // W
            else
                IM_CHECK(ctx->ItemInfo(handle_id, ImGuiTestOpFlags_NoError) != NULL); // FF
        }
        IM_CHECK(table->ColumnsTotalWidth + 1 == table->InnerWindow->ContentRegionRect.GetWidth());
    };

    // ## Test Visible flag
    t = IM_REGISTER_TEST(e, "table", "table_6_clip");
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
    t = IM_REGISTER_TEST(e, "table", "table_7_empty");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::BeginTable("Table", 3);
        ImGui::EndTable();
        ImGui::End();
    };

    // ## Test default sort order assignment
    t = IM_REGISTER_TEST(e, "table", "table_8_default_sort_order");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::BeginTable("Table", 4, ImGuiTableFlags_MultiSortable);
        ImGui::TableSetupColumn("0", ImGuiTableColumnFlags_None);
        ImGui::TableSetupColumn("1", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("1", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortAscending);
        ImGui::TableSetupColumn("2", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending);
        const ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
        ImGui::TableAutoHeaders();
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
    t = IM_REGISTER_TEST(e, "table", "table_9_max_columns");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);
        //ImDrawList* cmd = ImGui::GetWindowDrawList();
        ImGui::BeginTable("Table", 64);
        for (int n = 0; n < 64; n++)
            ImGui::TableSetupColumn("Header");
        ImGui::TableAutoHeaders();
        for (int i = 0; i < 10; i++)
        {
            ImGui::TableNextRow();
            for (int n = 0; n < 64; n++)
            {
                ImGui::Text("Data");
                ImGui::TableNextCell();
            }
        }
        ImGui::EndTable();
        ImGui::End();
    };

    // ## Test rendering two tables with same ID.
    t = IM_REGISTER_TEST(e, "table", "table_10_multi_instance");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_Appearing);
        ImGui::Begin("Test window 1", NULL, ImGuiWindowFlags_NoSavedSettings);

        const int col_count = 3;
        for (int i = 0; i < 2; i++)
        {
            ImGui::BeginTable("Table", col_count, ImGuiTableFlags_NoSavedSettings|ImGuiTableFlags_Resizable|ImGuiTableFlags_Borders);
            for (int c = 0; c < col_count; c++)
                ImGui::TableSetupColumn("Header", c ? ImGuiTableColumnFlags_WidthFixed : ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableAutoHeaders();
            for (int r = 0; r < 10; r++)
            {
                ImGui::TableNextRow();
                for (int c = 0; c < col_count; c++)
                {
                    // Second table contains larger data, attempting to confuse column sync.
                    ImGui::Text(i ? "Long Data" : "Data");
                    ImGui::TableNextCell();
                }
            }

            if (ctx->IsFirstFrame() || ctx->GenericVars.Bool1)
            {
                // Perform actual test.
                ImGuiContext& g = *ctx->UiContext;
                ImGuiTable* table = g.CurrentTable;
                float column_widths[col_count];

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
        ctx->WindowRef("Test window 1");
        ImGuiTable* table = ImGui::FindTableByID(ctx->GetID("Table"));

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
                if (ImGui::BeginTable(Str16f("Table%d", i).c_str(), 2))
                {
                    ImGui::TableSetupColumn("Header1");
                    ImGui::TableSetupColumn("Header2");
                    ImGui::TableAutoHeaders();
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
        ctx->WindowRef("Bug Report");
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

        if (ImGui::BeginTable("Table", column_count, ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Sortable))
        {
            ImGui::TableSetupColumn("Col1");
            ImGui::TableSetupColumn("Col2");
            ImGui::TableSetupColumn("Col3", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Col4", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableAutoHeaders();
            if (vars.call_get_sort_specs) // Test against TableGetSortSpecs() having side effects
                ImGui::TableGetSortSpecs();
            HelperTableSubmitCells(column_count, 3);
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        TableSettingsVars& vars = ctx->GetUserData<TableSettingsVars>();

        ctx->WindowRef("Table Settings");
        ImGuiID table_id = ctx->GetID("Table");
        ImGuiTable* table = ImGui::FindTableByID(table_id);

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

                IM_CHECK_EQ_NO_RET(table->Columns[2].WidthGiven, 50.0f);
                IM_CHECK_EQ_NO_RET(table->Columns[3].WidthStretchWeight, 1.0f);
            };
            auto check_modified_settings = [&](ImGuiTable* table)
            {
                IM_CHECK_EQ_NO_RET(table->Columns[0].SortOrder, 0);
                IM_CHECK_EQ_NO_RET(table->Columns[0].SortDirection, col0_sorted_desc ? ImGuiSortDirection_Descending : ImGuiSortDirection_Ascending);
                IM_CHECK_EQ_NO_RET(table->Columns[1].IsVisible, !col1_hidden);
                IM_CHECK_EQ_NO_RET(table->Columns[0].DisplayOrder, col0_reordered ? (col1_hidden ? 2 : 1) : 0);
                IM_CHECK_EQ_NO_RET(table->Columns[1].DisplayOrder, col0_reordered ? 0 : 1);
                IM_CHECK_EQ_NO_RET(table->Columns[2].DisplayOrder, col0_reordered ? (col1_hidden ? 1 : 2) : 2);
                IM_CHECK_EQ_NO_RET(table->Columns[2].WidthGiven, (col2_resized ? 60.0f : 50.0f));
                IM_CHECK_EQ_NO_RET(table->Columns[3].WidthStretchWeight, col3_resized ? 0.2f : 1.0f);
            };

            // Discard previous table state.
            table = ImGui::FindTableByID(table_id);
            if (table)
                ctx->TableDiscard(table);
            ctx->Yield(); // Rebuild a table on next frame

            ctx->LogInfo("Permutation %d: col0_sorted_desc:%d col1_hidden:%d col0_reordered:%d col2_resized:%d col3_resized:%d", mask,
                col0_sorted_desc, col1_hidden, col0_reordered, col2_resized, col3_resized);

            // Check initial settings
            ctx->LogInfo("1/4 Check initial settings");
            table = ImGui::FindTableByID(table_id);
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
                table->Columns[3].WidthStretchWeight = 0.2f;
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
            ctx->TableDiscard(table);
            ctx->Yield();
            table = ImGui::FindTableByID(table_id);

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
        table = ImGui::FindTableByID(table_id);
        if (table)
            ctx->TableDiscard(table);
    };

    // ## Test table sorting behaviors.
    t = IM_REGISTER_TEST(e, "table", "table_sorting");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        int& table_flags = ctx->GenericVars.Int1;
        ImGui::SetNextWindowSize(ImVec2(600, 80), ImGuiCond_Appearing); // FIXME-TESTS: Why?
        ImGui::Begin("Test window", NULL, ImGuiWindowFlags_NoSavedSettings);

        if (ctx->IsFirstFrame())
        {
            table_flags = ImGuiTableFlags_Sortable | ImGuiTableFlags_MultiSortable;
            if (ImGuiTable* table = ImGui::FindTableByID(ImGui::GetID("Table")))
                ctx->TableDiscard(table);
        }

        if (ImGui::BeginTable("Table", 6, table_flags))
        {
            ImGui::TableSetupColumn("Default");
            ImGui::TableSetupColumn("PreferSortAscending", ImGuiTableColumnFlags_PreferSortAscending);
            ImGui::TableSetupColumn("PreferSortDescending", ImGuiTableColumnFlags_PreferSortDescending);
            ImGui::TableSetupColumn("NoSort", ImGuiTableColumnFlags_NoSort);
            ImGui::TableSetupColumn("NoSortAscending", ImGuiTableColumnFlags_NoSortAscending);
            ImGui::TableSetupColumn("NoSortDescending", ImGuiTableColumnFlags_NoSortDescending);
            ImGui::TableAutoHeaders();
            HelperTableSubmitCells(6, 1);
            ImGui::EndTable();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->WindowRef("Test window");
        ImGuiTable* table = ImGui::FindTableByID(ctx->GetID("Table"));
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

            int column_n = table->Columns.index_from_ptr(column);
            ImGuiID column_header_id = ctx->GetID(label, ctx->GetIDByInt(column_n, ctx->GetIDByInt((int)table->ID))); // FIXME-TESTS: Add helpers
            if (click_mod != ImGuiKeyModFlags_None)
                ctx->KeyDownMap(ImGuiKey_COUNT, click_mod);
            ctx->ItemClick(column_header_id, ImGuiMouseButton_Left);
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
        IM_CHECK(sort_specs->SpecsCount == 1);
        IM_CHECK(sort_specs->ColumnsMask == 0x01);
        IM_CHECK(sort_specs->Specs[0].ColumnIndex == 0);
        IM_CHECK(sort_specs->Specs[0].SortOrder == 0);
        IM_CHECK(sort_specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending);

        IM_CHECK_EQ(click_column_and_get_sort("Default"), ImGuiSortDirection_Descending);   // Sorted implicitly by calling TableGetSortSpecs().
        IM_CHECK_EQ(click_column_and_get_sort("Default"), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(click_column_and_get_sort("PreferSortAscending"), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(click_column_and_get_sort("PreferSortAscending"), ImGuiSortDirection_Descending);

        // Not holding shift does not perform multi-sort.
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs != NULL);
        IM_CHECK(sort_specs->SpecsCount == 1);

        // Holding shift includes all sortable columns in multi-sort.
        IM_CHECK_EQ(click_column_and_get_sort("PreferSortDescending", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Descending);
        IM_CHECK_EQ(click_column_and_get_sort("PreferSortDescending", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Ascending);
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs && sort_specs->SpecsCount == 2);

        IM_CHECK_EQ(click_column_and_get_sort("NoSort", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(click_column_and_get_sort("NoSort", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Ascending);
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs&& sort_specs->SpecsCount == 2);

        IM_CHECK_EQ(click_column_and_get_sort("NoSortAscending", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Descending);
        IM_CHECK_EQ(click_column_and_get_sort("NoSortAscending", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Descending);
        IM_CHECK_EQ(click_column_and_get_sort("NoSortDescending", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Ascending);
        IM_CHECK_EQ(click_column_and_get_sort("NoSortDescending", ImGuiKeyModFlags_Shift), ImGuiSortDirection_Ascending);
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs&& sort_specs->SpecsCount == 4);

        // Disable multi-sort and ensure there is only one sorted column left.
        table_flags = ImGuiTableFlags_Sortable;
        ctx->Yield();
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs != NULL);
        IM_CHECK(sort_specs->SpecsCount == 1);

        // Disable sorting completely. Sort spec should not be returned.
        table_flags = ImGuiTableFlags_None;
        ctx->Yield();
        sort_specs = table_get_sort_specs(ctx, table);
        IM_CHECK(sort_specs == NULL);
    };

    // ## Test freezing of table rows and columns.
    t = IM_REGISTER_TEST(e, "table", "table_freezing");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Appearing);
        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings);
        memset(ctx->GenericVars.BoolArray, 0, sizeof(ctx->GenericVars.BoolArray));
        const int column_count = 15;
        ImGuiTableFlags flags = ImGuiTableFlags_Scroll | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ctx->GenericVars.Int1;
        if (ImGui::BeginTable("Table", column_count, flags))
        {
            for (int i = 0; i < column_count; i++)
                ImGui::TableSetupColumn(Str16f("Col%d", i).c_str());
            ImGui::TableAutoHeaders();

            for (int line = 0; line < 15; line++)
            {
                ImGui::TableNextRow();
                for (int column = 0; column < column_count; column++)
                {
                    if (!ImGui::TableSetColumnIndex(column))
                        continue;
                    ImGui::Text(Str16f("%d,%d", line, column).c_str(), ImVec2(40.0f, 20.0f));
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
        ctx->WindowRef("Test Window");
        ImGuiTable* table = ImGui::FindTableByID(ctx->GetID("Table"));
        ctx->WindowRef(table->InnerWindow);

        // Reset scroll, if any.
        ctx->ScrollToX(0.0f);
        ctx->ScrollToY(0.0f);
        ctx->Yield();

        // No initial freezing.
        IM_CHECK(table->FreezeColumnsRequest == 0);
        IM_CHECK(table->FreezeColumnsCount == 0);
        IM_CHECK(table->FreezeRowsRequest == 0);
        IM_CHECK(table->FreezeRowsCount == 0);

        // First five columns and rows are visible at the start.
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

        // Test row freezing.
        for (int freeze_count = 1; freeze_count <= 3; freeze_count++)
        {
            ctx->GenericVars.Int1 = ImGuiTableFlags_ScrollFreezeTopRow * freeze_count;
            IM_ASSERT((ctx->GenericVars.Int1 & ~ImGuiTableFlags_ScrollFreezeRowsMask_) == 0); // Make sure we don't overflow
            ctx->Yield();
            IM_CHECK(table->FreezeRowsRequest == freeze_count);
            IM_CHECK(table->FreezeRowsCount == freeze_count);

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
            ctx->GenericVars.Int1 = ImGuiTableFlags_ScrollFreezeLeftColumn * freeze_count;
            IM_ASSERT((ctx->GenericVars.Int1 & ~ImGuiTableFlags_ScrollFreezeColumnsMask_) == 0); // Make sure we don't overflow
            ctx->Yield();
            IM_CHECK(table->FreezeColumnsRequest == freeze_count);
            IM_CHECK(table->FreezeColumnsCount == freeze_count);

            // Test whether frozen columns are visible.
            for (int column_n = 0; column_n < 4; column_n++)
                IM_CHECK_EQ(table->Columns[column_n].IsClipped, !(column_n < freeze_count));
        }
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

