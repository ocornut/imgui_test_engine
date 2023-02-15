// dear imgui
// (tests: performances/benchmarks)

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
// Tests: Performances/Benchmarks
//-------------------------------------------------------------------------

#if IMGUI_VERSION_NUM >= 17703
#define IMGUI_HAS_TEXLINES
#endif

void RegisterTests_Perf(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    auto PerfCaptureFunc = [](ImGuiTestContext* ctx)
    {
        //ctx->KeyDown(ImGuiMod_Shift);
        ctx->PerfCapture();
        //ctx->KeyUp(ImGuiMod_Shift);
    };

    // ## Measure the cost all demo contents
    t = IM_REGISTER_TEST(e, "perf", "perf_demo_all");
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->PerfCalcRef();

        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemOpenAll("");
        ctx->MenuCheckAll("Examples");
        ctx->MenuCheckAll("Tools");

        IM_CHECK_SILENT(ctx->UiContext->IO.DisplaySize.x > 820);
        IM_CHECK_SILENT(ctx->UiContext->IO.DisplaySize.y > 820);
        if (ctx->IsError())
            return;

        // FIXME-TESTS: Backup full layout
        ImVec2 pos = ImGui::GetMainViewport()->Pos + ImVec2(20, 20);
        for (ImGuiWindow* window : ctx->UiContext->Windows)
        {
            window->Pos = pos;
            window->SizeFull = ImVec2(800, 800);
        }
        ctx->PerfCapture();

        ctx->SetRef("Dear ImGui Demo");
        ctx->ItemCloseAll("");
        ctx->MenuUncheckAll("Examples");
        ctx->MenuUncheckAll("Tools");
    };

    // ## Measure the drawing cost of various ImDrawList primitives
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
        DrawPrimFunc_TriangleStroke,
        DrawPrimFunc_TriangleStrokeThick,
        DrawPrimFunc_LongStroke,
        DrawPrimFunc_LongStrokeThick,
        DrawPrimFunc_LongJaggedStroke,
        DrawPrimFunc_LongJaggedStrokeThick,
		DrawPrimFunc_Line,
		DrawPrimFunc_LineAA,
#ifdef IMGUI_HAS_TEXLINES
        DrawPrimFunc_LineAANoTex,
#endif
        DrawPrimFunc_LineThick,
		DrawPrimFunc_LineThickAA,
#ifdef IMGUI_HAS_TEXLINES
        DrawPrimFunc_LineThickAANoTex
#endif
    };

    auto DrawPrimFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 200 * ctx->PerfStressAmount;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        int segments = 0;
        ImGui::Button("##CircleFilled", ImVec2(120, 120));
        ImVec2 bounds_min = ImGui::GetItemRectMin();
        ImVec2 bounds_size = ImGui::GetItemRectSize();
        ImVec2 center = bounds_min + bounds_size * 0.5f;
        float r = (float)(int)(ImMin(bounds_size.x, bounds_size.y) * 0.8f * 0.5f);
        float rounding = 8.0f;
        ImU32 col = IM_COL32(255, 255, 0, 255);
		ImDrawListFlags old_flags = draw_list->Flags; // Save old flags as some of these tests manipulate them
        if (ctx->IsFirstTestFrame())
            ctx->LogDebug("Drawing %d primitives...", loop_count);
        switch (ctx->Test->ArgVariant)
        {
        case DrawPrimFunc_RectStroke:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r,r), center + ImVec2(r,r), col, 0.0f, 0, 1.0f);
            break;
        case DrawPrimFunc_RectStrokeThick:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r, r), center + ImVec2(r, r), col, 0.0f, 0, 4.0f);
            break;
        case DrawPrimFunc_RectFilled:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRectFilled(center - ImVec2(r, r), center + ImVec2(r, r), col, 0.0f);
            break;
        case DrawPrimFunc_RectRoundedStroke:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r, r), center + ImVec2(r, r), col, rounding, 0, 1.0f);
            break;
        case DrawPrimFunc_RectRoundedStrokeThick:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddRect(center - ImVec2(r, r), center + ImVec2(r, r), col, rounding, 0, 4.0f);
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
        case DrawPrimFunc_TriangleStroke:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddNgon(center, r, col, 3, 1.0f);
            break;
        case DrawPrimFunc_TriangleStrokeThick:
            for (int n = 0; n < loop_count; n++)
                draw_list->AddNgon(center, r, col, 3, 4.0f);
            break;
        case DrawPrimFunc_LongStroke:
            draw_list->AddNgon(center, r, col, 10 * loop_count, 1.0f);
            break;
        case DrawPrimFunc_LongStrokeThick:
            draw_list->AddNgon(center, r, col, 10 * loop_count, 4.0f);
            break;
        case DrawPrimFunc_LongJaggedStroke:
            for (float n = 0; n < 10 * loop_count; n += 2.51327412287f)
                draw_list->PathLineTo(center + ImVec2(r * sinf(n), r * cosf(n)));
            draw_list->PathStroke(col, false, 1.0);
            break;
        case DrawPrimFunc_LongJaggedStrokeThick:
            for (float n = 0; n < 10 * loop_count; n += 2.51327412287f)
                draw_list->PathLineTo(center + ImVec2(r * sinf(n), r * cosf(n)));
            draw_list->PathStroke(col, false, 4.0);
            break;
		case DrawPrimFunc_Line:
			draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;
			for (int n = 0; n < loop_count; n++)
				draw_list->AddLine(center - ImVec2(r, r), center + ImVec2(r, r), col, 1.0f);
			break;
		case DrawPrimFunc_LineAA:
            draw_list->Flags |= ImDrawListFlags_AntiAliasedLines;
			for (int n = 0; n < loop_count; n++)
				draw_list->AddLine(center - ImVec2(r, r), center + ImVec2(r, r), col, 1.0f);
			break;
#ifdef IMGUI_HAS_TEXLINES
		case DrawPrimFunc_LineAANoTex:
			draw_list->Flags |= ImDrawListFlags_AntiAliasedLines;
			draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLinesUseTex;
			for (int n = 0; n < loop_count; n++)
				draw_list->AddLine(center - ImVec2(r, r), center + ImVec2(r, r), col, 1.0f);
			break;
#endif
		case DrawPrimFunc_LineThick:
			draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;
			for (int n = 0; n < loop_count; n++)
				draw_list->AddLine(center - ImVec2(r, r), center + ImVec2(r, r), col, 4.0f);
			break;
		case DrawPrimFunc_LineThickAA:
			draw_list->Flags |= ImDrawListFlags_AntiAliasedLines;
			for (int n = 0; n < loop_count; n++)
				draw_list->AddLine(center - ImVec2(r, r), center + ImVec2(r, r), col, 4.0f);
			break;
#ifdef IMGUI_HAS_TEXLINES
        case DrawPrimFunc_LineThickAANoTex:
			draw_list->Flags |= ImDrawListFlags_AntiAliasedLines;
			draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLinesUseTex;
			for (int n = 0; n < loop_count; n++)
				draw_list->AddLine(center - ImVec2(r, r), center + ImVec2(r, r), col, 4.0f);
			break;
#endif
        default:
            IM_ASSERT(0);
        }
		draw_list->Flags = old_flags; // Restore flags
        ImGui::End();
    };

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_rect_stroke");
    t->ArgVariant = DrawPrimFunc_RectStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_rect_stroke_thick");
    t->ArgVariant = DrawPrimFunc_RectStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_rect_filled");
    t->ArgVariant = DrawPrimFunc_RectFilled;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_rect_rounded_stroke");
    t->ArgVariant = DrawPrimFunc_RectRoundedStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_rect_rounded_stroke_thick");
    t->ArgVariant = DrawPrimFunc_RectRoundedStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_rect_rounded_filled");
    t->ArgVariant = DrawPrimFunc_RectRoundedFilled;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_circle_stroke");
    t->ArgVariant = DrawPrimFunc_CircleStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_circle_stroke_thick");
    t->ArgVariant = DrawPrimFunc_CircleStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_circle_filled");
    t->ArgVariant = DrawPrimFunc_CircleFilled;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_triangle_stroke");
    t->ArgVariant = DrawPrimFunc_TriangleStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_triangle_stroke_thick");
    t->ArgVariant = DrawPrimFunc_TriangleStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_long_stroke");
    t->ArgVariant = DrawPrimFunc_LongStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_long_stroke_thick");
    t->ArgVariant = DrawPrimFunc_LongStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_long_jagged_stroke");
    t->ArgVariant = DrawPrimFunc_LongJaggedStroke;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_long_jagged_stroke_thick");
    t->ArgVariant = DrawPrimFunc_LongJaggedStrokeThick;
    t->GuiFunc = DrawPrimFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_line");
	t->ArgVariant = DrawPrimFunc_Line;
	t->GuiFunc = DrawPrimFunc;
	t->TestFunc = PerfCaptureFunc;

	t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_line_antialiased");
	t->ArgVariant = DrawPrimFunc_LineAA;
	t->GuiFunc = DrawPrimFunc;
	t->TestFunc = PerfCaptureFunc;

#ifdef IMGUI_HAS_TEXLINES
	t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_line_antialiased_no_tex");
	t->ArgVariant = DrawPrimFunc_LineAANoTex;
	t->GuiFunc = DrawPrimFunc;
	t->TestFunc = PerfCaptureFunc;
#endif

	t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_line_thick");
	t->ArgVariant = DrawPrimFunc_LineThick;
	t->GuiFunc = DrawPrimFunc;
	t->TestFunc = PerfCaptureFunc;

	t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_line_thick_antialiased");
	t->ArgVariant = DrawPrimFunc_LineThickAA;
	t->GuiFunc = DrawPrimFunc;
	t->TestFunc = PerfCaptureFunc;

#ifdef IMGUI_HAS_TEXLINES
	t = IM_REGISTER_TEST(e, "perf", "perf_draw_prim_line_thick_antialiased_no_tex");
	t->ArgVariant = DrawPrimFunc_LineThickAANoTex;
	t->GuiFunc = DrawPrimFunc;
	t->TestFunc = PerfCaptureFunc;
#endif

    // ## Measure the cost of ImDrawListSplitter split/merge functions
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
            ctx->LogDebug("%d primitives over %d channels", loop_count, split_count);
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
            ctx->LogDebug("Vertices: %d", draw_list->VtxBuffer.Size);

        ImGui::End();
    };

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_split_1");
    t->ArgVariant = 1;
    t->GuiFunc = DrawSplittedFunc;
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_draw_split_10");
    t->ArgVariant = 10;
    t->GuiFunc = DrawSplittedFunc;
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of simple Button() calls
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_button");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
            ImGui::Button("Hello, world");
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of simple Button() calls + BeginDisabled()/EndDisabled()
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_button_disabled");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::BeginDisabled();
            ImGui::Button("Hello, world");
            ImGui::EndDisabled();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of simple Checkbox() calls
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_checkbox");
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

    // ## Measure the cost of dumb column-like setup using SameLine()
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_rows_1a");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 50 * 2 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::TextUnformatted("Cell 1");
            ImGui::SameLine(100);
            ImGui::TextUnformatted("Cell 2");
            ImGui::SameLine(200);
            ImGui::TextUnformatted("Cell 3");
        }
        ImGui::Columns(1);
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    t = IM_REGISTER_TEST(e, "perf", "perf_stress_rows_1b");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 50 * 2 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::Text("Cell 1");
            ImGui::SameLine(100);
            ImGui::Text("Cell 2");
            ImGui::SameLine(200);
            ImGui::Text("Cell 3");
        }
        ImGui::Columns(1);
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of NextColumn(): one column set, many rows
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_columns_1");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Columns(3, "Columns", true);
        int loop_count = 50 * 2 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::Text("Cell 1,%d", n);
            ImGui::NextColumn();
            ImGui::TextUnformatted("Cell 2");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Cell 3");
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of Columns(): many columns sets, few rows
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_columns_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 50 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            ImGui::Columns(3, "Columns", true);
            for (int row = 0; row < 2; row++)
            {
                ImGui::Text("Cell 1,%d", n);
                ImGui::NextColumn();
                ImGui::TextUnformatted("Cell 2");
                ImGui::NextColumn();
                ImGui::TextUnformatted("Cell 3");
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of Columns() + Selectable() spanning all columns.
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_columns_3_selectable");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Columns(3, "Columns", true);
        int loop_count = 50 * 2 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::Selectable("Hello", true, ImGuiSelectableFlags_SpanAllColumns);
            ImGui::NextColumn();
            ImGui::TextUnformatted("Cell 2");
            ImGui::NextColumn();
            ImGui::TextUnformatted("Cell 3");
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // Shared function to test basic table performances.
    struct TablePerfFuncVars { int TablesCount = 1; int RowsCount = 50; int ColumnsCount = 3; };
    auto TablePerfFunc = [](ImGuiTestContext* ctx)
    {
        auto& vars = ctx->GetVars<TablePerfFuncVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 800));
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

        const int tables_count = vars.TablesCount;
        const int columns_count = vars.ColumnsCount;
        const int rows_count = vars.RowsCount;
        if (ctx->IsFirstGuiFrame())
            ctx->LogDebug("%d tables x %d columns x %d rows", tables_count, columns_count, rows_count);
        for (int n = 0; n < tables_count; n++)
        {
            ImGui::PushID(n);
            if (ImGui::BeginTable("table1", columns_count, ImGuiTableFlags_BordersV))
            {
                for (int row = 0; row < rows_count; row++)
                {
                    for (int col = 0; col < columns_count; col++)
                    {
                        ImGui::TableNextColumn();
                        if (col == 0)
                            ImGui::Text("Cell 0,%d", n);
                        else if (col == 1)
                            ImGui::TextUnformatted("Cell 1");
                        else if (col == 2)
                            ImGui::TextUnformatted("Cell 2");
                        // No output on further columns to not interfere with performances more than necessary
                    }
                }
                ImGui::EndTable();
            }
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::End();
    };

    // ## Measure the cost of TableNextCell(), TableNextRow(): one table, few columns, many rows
    t = IM_REGISTER_TEST(e, "perf", "perf_tables_basic_tables_lo_cols_lo_rows_hi");
    t->SetVarsDataType<TablePerfFuncVars>([](ImGuiTestContext* ctx, auto& vars)
        {
            vars.TablesCount = 1;
            vars.ColumnsCount = 3;
            vars.RowsCount = 100 * ctx->PerfStressAmount;
        });
    t->GuiFunc = TablePerfFunc;
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of BeginTable(): many tables, few columns, few rows
    t = IM_REGISTER_TEST(e, "perf", "perf_tables_basic_tables_hi_cols_lo_rows_lo");
    t->SetVarsDataType<TablePerfFuncVars>([](ImGuiTestContext* ctx, auto& vars)
        {
            vars.TablesCount = 30 * ctx->PerfStressAmount;
            vars.ColumnsCount = 3;
            vars.RowsCount = 3;
        });
    t->GuiFunc = TablePerfFunc;
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of BeginTable(): many tables, many columns, few rows
    t = IM_REGISTER_TEST(e, "perf", "perf_tables_basic_tables_hi_cols_hi_rows_lo");
    t->SetVarsDataType<TablePerfFuncVars>([](ImGuiTestContext* ctx, auto& vars)
        {
            vars.TablesCount = 30 * ctx->PerfStressAmount;
            vars.ColumnsCount = 32;
            vars.RowsCount = 3;
        });
    t->GuiFunc = TablePerfFunc;
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of BeginTable(): one table, many columns, many rows
    t = IM_REGISTER_TEST(e, "perf", "perf_tables_basic_tables_lo_cols_hi_rows_hi");
    t->SetVarsDataType<TablePerfFuncVars>([](ImGuiTestContext* ctx, auto& vars)
        {
            vars.TablesCount = 1;
            vars.ColumnsCount = 32;
            vars.RowsCount = 30 * ctx->PerfStressAmount;
        });
    t->GuiFunc = TablePerfFunc;
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of simple ColorEdit4() calls (multi-component, group based widgets are quite heavy)
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_coloredit4");
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

    // ## Measure the cost of simple InputText() calls
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_input_text");
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

    // ## Measure the cost of simple InputTextMultiline() calls
    // (this is creating a child window for every non-clipped widget, so doesn't scale very well)
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_input_text_multiline");
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

    // ## Measure the cost of our ImHashXXX functions
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_hash");
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

    // ## Measure the cost of simple Listbox() calls
    // (this is creating a child window for every non-clipped widget, so doesn't scale very well)
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_list_box");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::PushID(n);
            if (ImGui::BeginListBox("ListBox", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 2)))
            {
                ImGui::MenuItem("Hello");
                ImGui::MenuItem("World");
                ImGui::EndListBox();
            }
            ImGui::PopID();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // ## Measure the cost of simple SliderFloat() calls
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_slider");
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

    // ## Measure the cost of simple SliderFloat2() calls
    // This at a glance by compared to SliderFloat() test shows us the overhead of group-based multi-component widgets
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_slider2");
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

    // ## Measure the cost of TextUnformatted() calls with relatively short text
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_text_unformatted_1");
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

    // ## Measure the cost of TextUnformatted() calls with long text
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_text_unformatted_2");
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

    // ## Measure the cost/overhead of creating too many windows
    t = IM_REGISTER_TEST(e, "perf", "perf_stress_window");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImVec2 pos = ImGui::GetMainViewport()->Pos + ImVec2(20, 20);
        int loop_count = 200 * ctx->PerfStressAmount;
        for (int n = 0; n < loop_count; n++)
        {
            ImGui::SetNextWindowPos(pos);
            ImGui::Begin(Str16f("Window_%05d", n + 1).c_str(), NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextUnformatted("Opening many windows!");
            ImGui::End();
        }
    };
    t->TestFunc = PerfCaptureFunc;

	// ## Circle segment count comparisons
	t = IM_REGISTER_TEST(e, "perf", "perf_circle_segment_counts");
	t->GuiFunc = [](ImGuiTestContext* ctx)
	{
		const int num_cols = 3; // Number of columns to draw
		const int num_rows = 3; // Number of rows to draw
		const float max_radius = 400.0f; // Maximum allowed radius

		static ImVec2 content_size(32.0f, 32.0f); // Size of window content on last frame

        // ImGui::SetNextWindowSize(ImVec2((num_cols + 0.5f) * item_spacing.x, (num_rows * item_spacing.y) + 128.0f));
		if (ImGui::Begin("perf_circle_segment_counts", NULL, ImGuiWindowFlags_HorizontalScrollbar))
		{
			// Control area
			static float line_width = 1.0f;
			static float radius = 32.0f;
            static int segment_count_manual = 32;
            static bool no_aa = false;
            static bool overdraw = false;
            static ImVec4 color_bg = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
            static ImVec4 color_fg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImGui::SliderFloat("Line width", &line_width, 1.0f, 10.0f);
            ImGui::SliderFloat("Radius", &radius, 1.0f, max_radius);
			ImGui::SliderInt("Segments", &segment_count_manual, 1, 512);
			ImGui::DragFloat("Circle Max Error", &ImGui::GetStyle().CircleTessellationMaxError, 0.005f, 0.1f, 5.0f, "%.2f");
			ImGui::Checkbox("No anti-aliasing", &no_aa);
			ImGui::SameLine();
			ImGui::Checkbox("Overdraw", &overdraw);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Draws each primitive repeatedly so that stray low-alpha pixels are easier to spot");
			ImGui::SameLine();
			const char* fill_modes[] = { "Stroke", "Fill", "Stroke+Fill", "Fill+Stroke" };
			static int fill_mode = 0;
			ImGui::SetNextItemWidth(128.0f);
			ImGui::Combo("Fill mode", &fill_mode, fill_modes, IM_ARRAYSIZE(fill_modes));

            ImGui::ColorEdit4("Color BG", &color_bg.x);
            ImGui::ColorEdit4("Color FG", &color_fg.x);

			// Display area
			ImGui::SetNextWindowContentSize(content_size);
			ImGui::BeginChild("Display", ImGui::GetContentRegionAvail(), false, ImGuiWindowFlags_HorizontalScrollbar);
			ImDrawList* draw_list = ImGui::GetWindowDrawList();

			// Set up the grid layout
			const float spacing = ImMax(96.0f, (radius * 2.0f) + line_width + 8.0f);
			ImVec2 item_spacing(spacing, spacing); // Spacing between rows/columns
			ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

			// Draw the first <n> radius/segment size pairs in a quasi-logarithmic down the side
			for (int pair_rad = 1, step = 1; pair_rad <= 512; pair_rad += step)
			{
                int segment_count = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC((float)pair_rad, draw_list->_Data->CircleSegmentMaxError);
				ImGui::TextColored((pair_rad == (int)radius) ? color_bg : color_fg, "Rad %d = %d segs", pair_rad, segment_count);
				if ((pair_rad >= 16) && ((pair_rad & (pair_rad - 1)) == 0))
					step *= 2;
			}

			// Calculate the worst-case width for the size pairs
			float max_pair_width = ImGui::CalcTextSize("Rad 0000 = 0000 segs").x;

			const ImVec2 text_standoff(max_pair_width + 64.0f, 16.0f); // How much space to leave for the size list and labels
			ImVec2 base_pos(cursor_pos.x + (item_spacing.x * 0.5f) + text_standoff.x, cursor_pos.y + text_standoff.y);

			// Update content size for next frame
			content_size = ImVec2(cursor_pos.x + (item_spacing.x * num_cols) + text_standoff.x, ImMax(cursor_pos.y + (item_spacing.y * num_rows) + text_standoff.y, ImGui::GetCursorPosY()));

			// Save old flags
			ImDrawListFlags backup_draw_list_flags = draw_list->Flags;
			if (no_aa)
				draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;

			// Get the suggested segment count for this radius
			const int segment_count_suggested = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, draw_list->_Data->CircleSegmentMaxError);
            const int segment_count_by_column[3] = { segment_count_suggested, segment_count_manual, 512 };
            const char* desc_by_column[3] = { "auto", "manual", "max" };

			// Draw row/column labels
			for (int i = 0; i < num_cols; i++)
			{
                Str64f name("%s\n%d segs", desc_by_column[i], segment_count_by_column[i]);
                draw_list->AddText(
                    ImVec2(cursor_pos.x + max_pair_width + 8.0f, cursor_pos.y + text_standoff.y + ((i + 0.5f) * item_spacing.y)),
                    ImGui::GetColorU32(color_fg), name.c_str());
                draw_list->AddText(
                    ImVec2(cursor_pos.x + text_standoff.x + ((i + 0.2f) * item_spacing.x), cursor_pos.y),
                    ImGui::GetColorU32(color_bg), name.c_str());
			}

			// Draw circles
			for (int y = 0; y < num_rows; y++)
			{
				for (int x = 0; x < num_cols; x++)
				{
					ImVec2 center = ImVec2(base_pos.x + (item_spacing.x * x), base_pos.y + (item_spacing.y * (y + 0.5f)));
					for (int pass = 0; pass < 2; pass++)
					{
                        const int type_index = (pass == 0) ? x : y;
                        const ImU32 color = ImColor((pass == 0) ? color_bg : color_fg);
                        const int num_segment_count = segment_count_by_column[type_index];
                        const int num_shapes_to_draw = overdraw ? 20 : 1;

                        // We fill either if fill mode was selected, or in stroke+fill mode for the first pass (so we can see what varying segment count fill+stroke looks like)
                        const bool fill = (fill_mode == 1) || (fill_mode == 2 && pass == 1) || (fill_mode == 3 && pass == 0);
						for (int i = 0; i < num_shapes_to_draw; i++)
						{
							if (fill)
								draw_list->AddCircleFilled(center, radius, color, num_segment_count);
							else
								draw_list->AddCircle(center, radius, color, num_segment_count, line_width);
						}
					}
				}
			}

			// Restore draw list flags
			draw_list->Flags = backup_draw_list_flags;
			ImGui::EndChild();
        }
        ImGui::End();
    };
    t->TestFunc = PerfCaptureFunc;

    // ## Draw various AA/non-AA lines (not really a perf test, more a correctness one)
	t = IM_REGISTER_TEST(e, "perf", "perf_misc_lines");
	t->GuiFunc = [](ImGuiTestContext* ctx)
	{
		const int num_cols = 16; // Number of columns (line rotations) to draw
		const int num_rows = 3; // Number of rows (line variants) to draw
		const float line_len = 64.0f; // Length of line to draw
		const ImVec2 line_spacing(80.0f, 96.0f); // Spacing between lines

		ImGui::SetNextWindowSize(ImVec2((num_cols + 0.5f) * line_spacing.x, (num_rows * 2 * line_spacing.y) + 128.0f), ImGuiCond_Once);
		if (ImGui::Begin("perf_misc_lines", NULL, ImGuiWindowFlags_NoSavedSettings))
		{
			ImDrawList* draw_list = ImGui::GetWindowDrawList();

			static float base_rot = 0.0f;
			ImGui::SliderFloat("Base rotation", &base_rot, 0.0f, 360.0f);
			static float line_width = 1.0f;
			ImGui::SliderFloat("Line width", &line_width, 1.0f, 10.0f);

            ImGui::Text("Press SHIFT to toggle textured/non-textured rows");
            bool tex_toggle = ImGui::GetIO().KeyShift;

            // Rotating lines
			ImVec2 cursor_pos = ImGui::GetCursorPos();
            ImVec2 cursor_screen_pos = ImGui::GetCursorScreenPos();
            ImVec2 base_pos(cursor_screen_pos.x + (line_spacing.x * 0.5f), cursor_screen_pos.y);

#ifndef IMGUI_HAS_TEXLINES
            const ImDrawListFlags ImDrawListFlags_AntiAliasedLinesUseTex = 0;
#endif

			for (int i = 0; i < num_rows; i++)
			{
                int row_idx = i;

                if (tex_toggle)
                {
                    if (i == 1)
                        row_idx = 2;
                    else if (i == 2)
                        row_idx = 1;
                }

				const char* name = "";
				switch (row_idx)
				{
				case 0: name = "No AA";  draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines; break;
				case 1: name = "AA no texturing"; draw_list->Flags |= ImDrawListFlags_AntiAliasedLines; draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLinesUseTex; break;
				case 2: name = "AA w/ texturing"; draw_list->Flags |= ImDrawListFlags_AntiAliasedLines; draw_list->Flags |= ImDrawListFlags_AntiAliasedLinesUseTex; break;
				}

				int initial_vtx_count = draw_list->VtxBuffer.Size;
				int initial_idx_count = draw_list->IdxBuffer.Size;

				for (int j = 0; j < num_cols; j++)
				{
					float r = (base_rot * IM_PI / 180.0f) + ((j * IM_PI * 0.5f) / (num_cols - 1));

					ImVec2 center = ImVec2(base_pos.x + (line_spacing.x * j), base_pos.y + (line_spacing.y * (i + 0.5f)));
					ImVec2 start = ImVec2(center.x + (ImSin(r) * line_len * 0.5f), center.y + (ImCos(r) * line_len * 0.5f));
					ImVec2 end = ImVec2(center.x - (ImSin(r) * line_len * 0.5f), center.y - (ImCos(r) * line_len * 0.5f));

					draw_list->AddLine(start, end, IM_COL32(255, 255, 255, 255), line_width);
				}

				ImGui::SetCursorPosY(cursor_pos.y + (i * line_spacing.y));
				ImGui::Text("%s - %d vertices, %d indices", name, draw_list->VtxBuffer.Size - initial_vtx_count, draw_list->IdxBuffer.Size - initial_idx_count);
			}

			ImGui::SetCursorPosY(cursor_pos.y + (num_rows * line_spacing.y));

            // Squares
            cursor_pos = ImGui::GetCursorPos();
            cursor_screen_pos = ImGui::GetCursorScreenPos();
            base_pos = ImVec2(cursor_screen_pos.x + (line_spacing.x * 0.5f), cursor_screen_pos.y);

            for (int i = 0; i < num_rows; i++)
            {
                int row_idx = i;

                if (tex_toggle)
                {
                    if (i == 1)
                        row_idx = 2;
                    else if (i == 2)
                        row_idx = 1;
                }

                const char* name = "";
                switch (row_idx)
                {
                case 0: name = "No AA";  draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines; break;
                case 1: name = "AA no texturing"; draw_list->Flags |= ImDrawListFlags_AntiAliasedLines; draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLinesUseTex; break;
                case 2: name = "AA w/ texturing"; draw_list->Flags |= ImDrawListFlags_AntiAliasedLines; draw_list->Flags |= ImDrawListFlags_AntiAliasedLinesUseTex; break;
                }

                int initial_vtx_count = draw_list->VtxBuffer.Size;
                int initial_idx_count = draw_list->IdxBuffer.Size;

                for (int j = 0; j < num_cols; j++)
                {
                    float cell_line_width = line_width + ((j * 4.0f) / (num_cols - 1));

                    ImVec2 center = ImVec2(base_pos.x + (line_spacing.x * j), base_pos.y + (line_spacing.y * (i + 0.5f)));
                    ImVec2 top_left = ImVec2(center.x - (line_len * 0.5f), center.y - (line_len * 0.5f));
                    ImVec2 bottom_right = ImVec2(center.x + (line_len * 0.5f), center.y + (line_len * 0.5f));

                    draw_list->AddRect(top_left, bottom_right, IM_COL32(255, 255, 255, 255), 0.0f, 0, cell_line_width);

                    ImGui::SetCursorPos(ImVec2(cursor_pos.x + ((j + 0.5f) * line_spacing.x) - 16.0f, cursor_pos.y + ((i + 0.5f) * line_spacing.y) - (ImGui::GetTextLineHeight() * 0.5f)));
                    ImGui::Text("%.2f", cell_line_width);
                }

                ImGui::SetCursorPosY(cursor_pos.y + (i * line_spacing.y));
                ImGui::Text("%s - %d vertices, %d indices", name, draw_list->VtxBuffer.Size - initial_vtx_count, draw_list->IdxBuffer.Size - initial_idx_count);
            }
		}
		ImGui::End();
	};
	t->TestFunc = PerfCaptureFunc;

    // ## "perf_draw_text_XXXX" tests: measure performance of drawlist text rendering
    {
        struct PerfDrawTextVars
        {
            ImVector<char>  str;
            int             line_count;
            int             line_length;
            ImVec2          text_size;
        };
        enum PerfTestTextFlags : int
        {
            PerfTestTextFlags_TextShort             = 1 << 0,
            PerfTestTextFlags_TextLong              = 1 << 1,
            PerfTestTextFlags_TextWayTooLong        = 1 << 2,
            PerfTestTextFlags_NoWrapWidth           = 1 << 3,
            PerfTestTextFlags_WithWrapWidth         = 1 << 4,
            PerfTestTextFlags_NoCpuFineClipRect     = 1 << 5,
            PerfTestTextFlags_WithCpuFineClipRect   = 1 << 6,
        };
        auto gui_func_measure_text_rendering_perf = [](ImGuiTestContext* ctx)
        {
            auto& vars = ctx->GetVars<PerfDrawTextVars>();
            ImGui::SetNextWindowSize(ImVec2(300, 120), ImGuiCond_Always);
            ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings);

            ImGuiWindow* window = ImGui::GetCurrentWindow();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            const int test_variant = ctx->Test->ArgVariant;
            float wrap_width = 0.0f;

            auto& str = vars.str;
            int& line_count = vars.line_count;
            int& line_length = vars.line_length;
            ImVec2& text_size = vars.text_size;
            ImVec4* cpu_fine_clip_rect = NULL;
            ImVec2 window_padding = ImGui::GetCursorScreenPos() - window->Pos;

            if (test_variant & PerfTestTextFlags_WithWrapWidth)
                wrap_width = 250.0f;

            if (test_variant & PerfTestTextFlags_WithCpuFineClipRect)
                cpu_fine_clip_rect = &ctx->GenericVars.Color1; // :)

            // Set up test string.
            if (str.empty())
            {
                if (test_variant & PerfTestTextFlags_TextLong)
                {
                    line_count = 6;
                    line_length = 2000;
                }
                else if (test_variant & PerfTestTextFlags_TextWayTooLong)
                {
                    line_count = 1;
                    line_length = 10000;
                }
                else
                {
                    line_count = 400;
                    line_length = 30;
                }

                // Support variable stress.
                line_count *= ctx->PerfStressAmount;

                // Create a test string.
                str.resize(line_length * line_count);
                for (int i = 0; i < str.Size; i++)
                    str.Data[i] = '0' + (char)(((2166136261 ^ i) * 16777619) & 0x7F) % ('z' - '0');
                for (int i = 14; i < str.Size; i += 15)
                    str.Data[i] = ' ';      // Spaces for word wrap.
                str.back() = 0;             // Null-terminate

                // Measure text size and cache result.
                text_size = ImGui::CalcTextSize(str.begin(), str.begin() + line_length, false, wrap_width);

                // Set up a cpu fine clip rect which should be about half of rect rendered text would occupy.
                if (test_variant & PerfTestTextFlags_WithCpuFineClipRect)
                {
                    cpu_fine_clip_rect->x = window->Pos.x + window_padding.x;
                    cpu_fine_clip_rect->y = window->Pos.y + window_padding.y;
                    cpu_fine_clip_rect->z = window->Pos.x + window_padding.x + text_size.x * 0.5f;
                    cpu_fine_clip_rect->w = window->Pos.y + window_padding.y + text_size.y * line_count * 0.5f;
                }
            }

            // Push artificially large clip rect to prevent coarse clipping of text on CPU side.
            ImVec2 text_pos = window->Pos + window_padding;
            draw_list->PushClipRect(text_pos, text_pos + ImVec2(text_size.x * 1.0f, text_size.y * line_count));
            for (int i = 0, end = line_count; i < end; i++)
                draw_list->AddText(NULL, 0.0f, text_pos + ImVec2(0.0f, text_size.y * (float)i),
                    IM_COL32_WHITE, str.begin() + (i * line_length), str.begin() + ((i + 1) * line_length), wrap_width, cpu_fine_clip_rect);
            draw_list->PopClipRect();

            ImGui::End();
        };
        const char* base_name = "perf_draw_text";
        const char* text_suffixes[] = { "_short", "_long", "_too_long" };
        const char* wrap_suffixes[] = { "", "_wrapped" };
        const char* clip_suffixes[] = { "", "_clipped" };
        for (int i = 0; i < IM_ARRAYSIZE(text_suffixes); i++)
        {
            for (int j = 0; j < IM_ARRAYSIZE(wrap_suffixes); j++)
            {
                for (int k = 0; k < IM_ARRAYSIZE(clip_suffixes); k++)
                {
                    Str64f test_name("%s%s%s%s", base_name, text_suffixes[i], wrap_suffixes[j], clip_suffixes[k]);
                    t = IM_REGISTER_TEST(e, "perf", "");
                    t->SetOwnedName(test_name.c_str());
                    t->SetVarsDataType<PerfDrawTextVars>();
                    t->ArgVariant = (PerfTestTextFlags_TextShort << i) | (PerfTestTextFlags_NoWrapWidth << j) | (PerfTestTextFlags_NoCpuFineClipRect << k);
                    t->GuiFunc = gui_func_measure_text_rendering_perf;
                    t->TestFunc = PerfCaptureFunc;
                }
            }
        }
    }
}

