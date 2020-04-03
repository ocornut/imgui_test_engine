#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_te_core.h"
#include "imgui_te_context.h"
#include "imgui_te_internal.h"
#include "shared/imgui_utils.h"
#include "libs/Str/Str.h"

//-------------------------------------------------------------------------
// TEST ENGINE: USER INTERFACE
//-------------------------------------------------------------------------
// - DrawTestLog() [internal]
// - GetVerboseLevelName() [internal]
// - ImGuiTestEngine_ShowTestGroup() [Internal]
// - ImGuiTestEngine_ShowTestWindow()
//-------------------------------------------------------------------------

// Look for " filename:number " in the string and add menu option to open source.
static bool ParseLineAndDrawFileOpenItemForSourceFile(ImGuiTestEngine* e, ImGuiTest* test, const char* line_start, const char* line_end)
{
    const char* separator = ImStrchrRange(line_start, line_end, ':');
    if (separator == NULL)
        return false;

    const char* path_end = separator;
    const char* path_begin = separator - 1;
    while (path_begin > line_start&& path_begin[-1] != ' ')
        path_begin--;
    if (path_begin == path_end)
        return false;

    int line_no = -1;
    sscanf(separator + 1, "%d ", &line_no);
    if (line_no == -1)
        return false;

    Str256f buf("Open '%.*s' at line %d", (int)(path_end - path_begin), path_begin, line_no);
    if (ImGui::MenuItem(buf.c_str()))
    {
        // FIXME-TESTS: Assume folder is same as folder of test->SourceFile!
        const char* src_path = test->SourceFile;
        const char* src_name = ImPathFindFilename(src_path);
        buf.setf("%.*s%.*s", (int)(src_name - src_path), src_path, (int)(path_end - path_begin), path_begin);

        ImGuiTestEngineIO& e_io = ImGuiTestEngine_GetIO(e);
        e_io.SrcFileOpenFunc(buf.c_str(), line_no, e_io.UserData);
    }

    return true;
}

// Look for "[ ,"]filename.png" in the string and add menu option to open image.
static bool ParseLineAndDrawFileOpenItemForImageFile(ImGuiTestEngine* e, ImGuiTest* test, const char* line_start, const char* line_end, const char* file_ext)
{
    IM_UNUSED(e);
    IM_UNUSED(test);

    const char* extension = ImStristr(line_start, line_end, file_ext, NULL);
    if (extension == NULL)
        return false;

    const char* path_end = extension + strlen(file_ext);
    const char* path_begin = extension - 1;
    while (path_begin > line_start && path_begin[-1] != ' ' && path_begin[-1] != '\'' && path_begin[-1] != '\"')
        path_begin--;
    if (path_begin == path_end)
        return false;

    Str256 buf;

    // Open file
    buf.setf("Open file: %.*s", (int)(path_end - path_begin), path_begin);
    if (ImGui::MenuItem(buf.c_str()))
    {
        buf.setf("%.*s", (int)(path_end - path_begin), path_begin);
        ImOsOpenInShell(buf.c_str());
    }

    // Open folder
    const char* folder_begin = path_begin;
    const char* folder_end = ImPathFindFilename(path_begin, path_end);
    buf.setf("Open folder: %.*s", (int)(folder_end - folder_begin), path_begin);
    if (ImGui::MenuItem(buf.c_str()))
    {
        buf.setf("%.*s", (int)(folder_end - folder_begin), folder_begin);
        ImOsOpenInShell(buf.c_str());
    }

    return true;
}

static bool ParseLineAndDrawFileOpenItem(ImGuiTestEngine* e, ImGuiTest* test, const char* line_start, const char* line_end)
{
    if (ParseLineAndDrawFileOpenItemForSourceFile(e, test, line_start, line_end))
        return true;
    if (ParseLineAndDrawFileOpenItemForImageFile(e, test, line_start, line_end, ".png"))
        return true;
    if (ParseLineAndDrawFileOpenItemForImageFile(e, test, line_start, line_end, ".gif"))
        return true;
    return false;
}

static void DrawTestLog(ImGuiTestEngine* e, ImGuiTest* test)
{
    ImU32 error_col = IM_COL32(255, 150, 150, 255);
    ImU32 warning_col = IM_COL32(240, 240, 150, 255);
    ImU32 unimportant_col = IM_COL32(190, 190, 190, 255);

    ImGuiTestLog* log = &test->TestLog;
    ImGuiTestEngineIO& e_io = ImGuiTestEngine_GetIO(e);

    const char* text = test->TestLog.Buffer.begin();
    const char* text_end = test->TestLog.Buffer.end();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 2.0f) * e_io.DpiScale);
    ImVector<ImGuiTestLogLineInfo>& line_info_vector = test->Status == ImGuiTestStatus_Error ? log->LineInfoError : log->LineInfo;
    ImGuiListClipper clipper;
    clipper.Begin(line_info_vector.Size);
    while (clipper.Step())
    {
        for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
        {
            ImGuiTestLogLineInfo& line_info = line_info_vector[line_no];
            const char* line_start = text + line_info.LineOffset;
            const char* line_end = strchr(line_start, '\n');
            if (line_end == NULL)
                line_end = text_end;

            switch (line_info.Level)
            {
            case ImGuiTestVerboseLevel_Error:
                ImGui::PushStyleColor(ImGuiCol_Text, error_col);
                break;
            case ImGuiTestVerboseLevel_Warning:
                ImGui::PushStyleColor(ImGuiCol_Text, warning_col);
                break;
            case ImGuiTestVerboseLevel_Debug:
            case ImGuiTestVerboseLevel_Trace:
                ImGui::PushStyleColor(ImGuiCol_Text, unimportant_col);
                break;
            default:
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32_WHITE);
                break;
            }
            ImGui::TextUnformatted(line_start, line_end);
            ImGui::PopStyleColor();

            ImGui::PushID(line_no);
            if (ImGui::BeginPopupContextItem("Context", 1))
            {
                if (!ParseLineAndDrawFileOpenItem(e, test, line_start, line_end))
                    ImGui::MenuItem("No options", NULL, false, false);
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
    }
    ImGui::PopStyleVar();
}

static void HelpTooltip(const char* desc)
{
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", desc);
}

static void ShowTestGroup(ImGuiTestEngine* e, ImGuiTestGroup group, ImGuiTextFilter* filter)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiTestEngineIO& e_io = ImGuiTestEngine_GetIO(e);

    //ImGui::Text("TESTS (%d)", engine->TestsAll.Size);
    if (ImGui::Button("Run All"))
        ImGuiTestEngine_QueueTests(e, group, filter->InputBuf); // FIXME: Filter func differs

    ImGui::SameLine();
    filter->Draw("##filter", -1.0f);
    ImGui::Separator();

    if (ImGui::BeginChild("Tests", ImVec2(0, 0)))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 3) * e_io.DpiScale);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 1) * e_io.DpiScale);
        for (int n = 0; n < e->TestsAll.Size; n++)
        {
            ImGuiTest* test = e->TestsAll[n];
            if (test->Group != group)
                continue;
            if (!filter->PassFilter(test->Name) && !filter->PassFilter(test->Category))
                continue;

            ImGuiTestContext* test_context = (e->TestContext && e->TestContext->Test == test) ? e->TestContext : NULL;

            ImGui::PushID(n);

            ImVec4 status_color;
            switch (test->Status)
            {
            case ImGuiTestStatus_Error:
                status_color = ImVec4(0.9f, 0.1f, 0.1f, 1.0f);
                break;
            case ImGuiTestStatus_Success:
                status_color = ImVec4(0.1f, 0.9f, 0.1f, 1.0f);
                break;
            case ImGuiTestStatus_Queued:
            case ImGuiTestStatus_Running:
                if (test_context && (test_context->RunFlags & ImGuiTestRunFlags_NoTestFunc))
                    status_color = ImVec4(0.8f, 0.0f, 0.8f, 1.0f);
                else
                    status_color = ImVec4(0.8f, 0.4f, 0.1f, 1.0f);
                break;
            default:
                status_color = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
                break;
            }

            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::ColorButton("status", status_color, ImGuiColorEditFlags_NoTooltip);
            ImGui::SameLine();
            if (test->Status == ImGuiTestStatus_Running)
                ImGui::RenderText(p + style.FramePadding + ImVec2(0, 0), &"|\0/\0-\0\\"[(((ImGui::GetFrameCount() / 5) & 3) << 1)], NULL);

            bool queue_test = false;
            bool queue_gui_func = false;
            bool select_test = false;

            if (ImGui::Button("Run"))
                queue_test = select_test = true;
            ImGui::SameLine();

            Str128f buf("%-*s - %s", 10, test->Category, test->Name);
            if (ImGui::Selectable(buf.c_str(), test == e->UiSelectedTest))
                select_test = true;

            // Double-click to run test, CTRL+Double-click to run GUI function
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                if (ImGui::GetIO().KeyCtrl)
                    queue_gui_func = true;
                else
                    queue_test = true;
            }

            /*if (ImGui::IsItemHovered() && test->TestLog.size() > 0)
            {
            ImGui::BeginTooltip();
            DrawTestLog(engine, test, false);
            ImGui::EndTooltip();
            }*/

            if (e->UiSelectAndScrollToTest == test)
                ImGui::SetScrollHereY();

            bool view_source = false;
            if (ImGui::BeginPopupContextItem())
            {
                select_test = true;

                if (ImGui::MenuItem("Run test"))
                    queue_test = true;

                bool is_running_gui_func = (test_context && (test_context->RunFlags & ImGuiTestRunFlags_NoTestFunc));
                if (ImGui::MenuItem("Run GUI func", "Ctrl+Click", is_running_gui_func))
                {
                    if (is_running_gui_func)
                        ImGuiTestEngine_Abort(e);
                    else
                        queue_gui_func = true;
                }
                ImGui::Separator();

                const bool open_source_available = (test->SourceFile != NULL) && (e->IO.SrcFileOpenFunc != NULL);
                if (open_source_available)
                {
                    buf.setf("Open source (%s:%d)", test->SourceFileShort, test->SourceLine);
                    if (ImGui::MenuItem(buf.c_str()))
                        e->IO.SrcFileOpenFunc(test->SourceFile, test->SourceLine, e->IO.UserData);
                    if (ImGui::MenuItem("View source..."))
                        view_source = true;
                }
                else
                {
                    ImGui::MenuItem("Open source", NULL, false, false);
                    ImGui::MenuItem("View source", NULL, false, false);
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Copy name", NULL, false))
                    ImGui::SetClipboardText(test->Name);

                if (ImGui::MenuItem("Copy log", NULL, false, !test->TestLog.Buffer.empty()))
                    ImGui::SetClipboardText(test->TestLog.Buffer.c_str());

                if (ImGui::MenuItem("Clear log", NULL, false, !test->TestLog.Buffer.empty()))
                    test->TestLog.Clear();

                ImGui::EndPopup();
            }

            // Process source popup
            static ImGuiTextBuffer source_blurb;
            static int goto_line = -1;
            if (view_source)
            {
                source_blurb.clear();
                size_t file_size = 0;
                char* file_data = (char*)ImFileLoadToMemory(test->SourceFile, "rb", &file_size);
                if (file_data)
                    source_blurb.append(file_data, file_data + file_size);
                else
                    source_blurb.append("<Error loading sources>");
                goto_line = (test->SourceLine + test->SourceLineEnd) / 2;
                ImGui::OpenPopup("Source");
            }
            if (ImGui::BeginPopup("Source"))
            {
                // FIXME: Local vs screen pos too messy :(
                const ImVec2 start_pos = ImGui::GetCursorStartPos();
                const float line_height = ImGui::GetTextLineHeight();
                if (goto_line != -1)
                    ImGui::SetScrollFromPosY(start_pos.y + (goto_line - 1) * line_height, 0.5f);
                goto_line = -1;

                ImRect r(0.0f, test->SourceLine * line_height, ImGui::GetWindowWidth(), (test->SourceLine + 1) * line_height); // SourceLineEnd is too flaky
                ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos() + start_pos + r.Min, ImGui::GetWindowPos() + start_pos + r.Max, IM_COL32(80, 80, 150, 150));

                ImGui::TextUnformatted(source_blurb.c_str(), source_blurb.end());
                ImGui::EndPopup();
            }

            // Process selection
            if (select_test)
                e->UiSelectedTest = test;

            // Process queuing
            if (!e->IO.RunningTests)
            {
                if (queue_test)
                    ImGuiTestEngine_QueueTest(e, test, ImGuiTestRunFlags_ManualRun);
                else if (queue_gui_func)
                    ImGuiTestEngine_QueueTest(e, test, ImGuiTestRunFlags_ManualRun | ImGuiTestRunFlags_NoTestFunc);
            }

            ImGui::PopID();
        }
        ImGui::Spacing();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
}

void    ImGuiTestEngine_ShowTestWindow(ImGuiTestEngine* engine, bool* p_open)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;

    if (engine->UiFocus)
    {
        ImGui::SetNextWindowFocus();
        engine->UiFocus = false;
    }
    ImGui::Begin("Dear ImGui Test Engine", p_open);// , ImGuiWindowFlags_MenuBar);

#if 0
    if (0 && ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Options"))
        {
            const bool busy = ImGuiTestEngine_IsRunningTests(engine);
            ImGui::MenuItem("Run fast", NULL, &engine->IO.ConfigRunFast, !busy);
            ImGui::MenuItem("Run in blind context", NULL, &engine->IO.ConfigRunBlind);
            ImGui::MenuItem("Debug break on error", NULL, &engine->IO.ConfigBreakOnError);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
#endif

    // Options
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::Checkbox("Fast", &engine->IO.ConfigRunFast); HelpTooltip("Run tests as fast as possible (no vsync, no delay, teleport mouse, etc.).");
    ImGui::SameLine();
    ImGui::Checkbox("Blind", &engine->IO.ConfigRunBlind); HelpTooltip("<UNSUPPORTED>\nRun tests in a blind ui context.");
    ImGui::SameLine();
    ImGui::Checkbox("Stop", &engine->IO.ConfigStopOnError); HelpTooltip("Stop running tests when hitting an error.");
    ImGui::SameLine();
    ImGui::Checkbox("DbgBrk", &engine->IO.ConfigBreakOnError); HelpTooltip("Break in debugger when hitting an error.");
    ImGui::SameLine();
    ImGui::Checkbox("KeepGUI", &engine->IO.ConfigKeepGuiFunc); HelpTooltip("Keep GUI function running after test function is finished.");
    ImGui::SameLine();
    ImGui::Checkbox("Refocus", &engine->IO.ConfigTakeFocusBackAfterTests); HelpTooltip("Set focus back to Test window after running tests.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60 * engine->IO.DpiScale);
    if (ImGui::DragInt("Verbose", (int*)&engine->IO.ConfigVerboseLevel, 0.1f, 0, ImGuiTestVerboseLevel_COUNT - 1, ImGuiTestEngine_GetVerboseLevelName(engine->IO.ConfigVerboseLevel)))
        engine->IO.ConfigVerboseLevelOnError = engine->IO.ConfigVerboseLevel;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(30 * engine->IO.DpiScale);
    ImGui::DragInt("Perf Stress Amount", &engine->IO.PerfStressAmount, 0.1f, 1, 20); HelpTooltip("Increase workload of performance tests (higher means longer run).");
    ImGui::PopStyleVar();
    ImGui::Separator();

    // FIXME-DOGFOODING: It would be about time that we made it easier to create splitters, current low-level splitter behavior is not easy to use properly.
    // FIXME-SCROLL: When resizing either we'd like to keep scroll focus on something (e.g. last clicked item for list, bottom for log)
    // See https://github.com/ocornut/imgui/issues/319
    const float avail_y = ImGui::GetContentRegionAvail().y - style.ItemSpacing.y;
    const float min_size_0 = ImGui::GetFrameHeight() * 1.2f;
    const float min_size_1 = ImGui::GetFrameHeight() * 1;
    float log_height = engine->UiLogHeight;
    float list_height = ImMax(avail_y - engine->UiLogHeight, min_size_0);
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        float y = ImGui::GetCursorScreenPos().y + list_height;
        ImRect splitter_bb = ImRect(window->WorkRect.Min.x, y - 1, window->WorkRect.Max.x, y + 1);
        ImGui::SplitterBehavior(splitter_bb, ImGui::GetID("splitter"), ImGuiAxis_Y, &list_height, &log_height, min_size_0, min_size_1, 3.0f);
        engine->UiLogHeight = log_height;
        //ImGui::DebugDrawItemRect();
    }

    // TESTS
    ImGui::BeginChild("List", ImVec2(0, list_height), false, ImGuiWindowFlags_NoScrollbar);
    if (ImGui::BeginTabBar("##Tests", ImGuiTabBarFlags_NoTooltip))
    {
        if (ImGui::BeginTabItem("TESTS"))
        {
            ShowTestGroup(engine, ImGuiTestGroup_Tests, &engine->UiFilterTests);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("PERFS"))
        {
            ShowTestGroup(engine, ImGuiTestGroup_Perfs, &engine->UiFilterPerfs);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::EndChild();
    engine->UiSelectAndScrollToTest = NULL;

    // LOG
    ImGui::BeginChild("Log", ImVec2(0, log_height));
    if (ImGui::BeginTabBar("##tools"))
    {
        if (ImGui::BeginTabItem("LOG"))
        {
            if (engine->UiSelectedTest)
                ImGui::Text("Log for %s: %s", engine->UiSelectedTest->Category, engine->UiSelectedTest->Name);
            else
                ImGui::Text("N/A");
            if (ImGui::SmallButton("Clear"))
                if (engine->UiSelectedTest)
                    engine->UiSelectedTest->TestLog.Clear();
            ImGui::SameLine();
            if (ImGui::SmallButton("Copy to clipboard"))
                if (engine->UiSelectedTest)
                    ImGui::SetClipboardText(engine->UiSelectedTest->TestLog.Buffer.c_str());
            ImGui::Separator();

            // Quick status
            ImGuiContext* ui_context = engine->UiContextActive ? engine->UiContextActive : engine->UiContextVisible;
            ImGuiID item_hovered_id = ui_context->HoveredIdPreviousFrame;
            ImGuiID item_active_id = ui_context->ActiveId;
            ImGuiTestItemInfo* item_hovered_info = item_hovered_id ? ImGuiTestEngine_ItemLocate(engine, item_hovered_id, "") : NULL;
            ImGuiTestItemInfo* item_active_info = item_active_id ? ImGuiTestEngine_ItemLocate(engine, item_active_id, "") : NULL;
            ImGui::Text("Hovered: 0x%08X (\"%s\") @ (%.1f,%.1f)", item_hovered_id, item_hovered_info ? item_hovered_info->DebugLabel : "", ui_context->IO.MousePos.x, ui_context->IO.MousePos.y);
            ImGui::Text("Active:  0x%08X (\"%s\")", item_active_id, item_active_info ? item_active_info->DebugLabel : "");

            ImGui::Separator();
            ImGui::BeginChild("Log");
            if (engine->UiSelectedTest)
            {
                DrawTestLog(engine, engine->UiSelectedTest);
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    ImGui::SetScrollHereY();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // Tools
        if (ImGui::BeginTabItem("MISC TOOLS"))
        {
            ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::Separator();

            ImGui::Text("Tools:");
            ImGui::DragFloat("DpiScale", &engine->IO.DpiScale, 0.005f, 0.0f, 0.0f, "%.2f");
            ImGui::Checkbox("Capture Tool", &engine->CaptureTool.Visible);
            ImGui::Checkbox("Slow down whole app", &engine->ToolSlowDown);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70 * engine->IO.DpiScale);
            ImGui::SliderInt("##ms", &engine->ToolSlowDownMs, 0, 400, "%d ms");

            ImGui::Separator();
            ImGui::Text("Configuration:");
            const ImGuiInputTextCallback filter_callback = [](ImGuiInputTextCallbackData* data) { return (data->EventChar == ',' || data->EventChar == ';') ? 1 : 0; };
            ImGui::InputText("Branch/Annotation", engine->IO.GitBranchName, IM_ARRAYSIZE(engine->IO.GitBranchName), ImGuiInputTextFlags_CallbackCharFilter, filter_callback, NULL);
            ImGui::CheckboxFlags("io.ConfigFlags: NavEnableKeyboard", (unsigned int*)&io.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard);
            ImGui::CheckboxFlags("io.ConfigFlags: NavEnableGamepad", (unsigned int*)&io.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad);
#ifdef IMGUI_HAS_DOCK
            ImGui::Checkbox("io.ConfigDockingAlwaysTabBar", &io.ConfigDockingAlwaysTabBar);
#endif
            ImGui::EndTabItem();
        }

        // FIXME-TESTS: Need to be visualizing the samples/spikes.
        if (ImGui::BeginTabItem("PERFS TOOLS"))
        {
            double dt_1 = 1.0 / ImGui::GetIO().Framerate;
            double fps_now = 1.0 / dt_1;
            double dt_100 = engine->PerfDeltaTime100.GetAverage();
            double dt_1000 = engine->PerfDeltaTime1000.GetAverage();
            double dt_2000 = engine->PerfDeltaTime2000.GetAverage();

            //if (engine->PerfRefDeltaTime <= 0.0 && engine->PerfRefDeltaTime.IsFull())
            //    engine->PerfRefDeltaTime = dt_2000;

            ImGui::Checkbox("Unthrolled", &engine->IO.ConfigNoThrottle);
            ImGui::SameLine();
            if (ImGui::Button("Pick ref dt"))
                engine->PerfRefDeltaTime = dt_2000;

            double dt_ref = engine->PerfRefDeltaTime;
            ImGui::Text("[ref dt]    %6.3f ms", engine->PerfRefDeltaTime * 1000);
            ImGui::Text("[last 0001] %6.3f ms (%.1f FPS) ++ %6.3f ms", dt_1 * 1000.0, 1.0 / dt_1, (dt_1 - dt_ref) * 1000);
            ImGui::Text("[last 0100] %6.3f ms (%.1f FPS) ++ %6.3f ms ~ converging in %.1f secs", dt_100 * 1000.0, 1.0 / dt_100, (dt_1 - dt_ref) * 1000, 100.0 / fps_now);
            ImGui::Text("[last 1000] %6.3f ms (%.1f FPS) ++ %6.3f ms ~ converging in %.1f secs", dt_1000 * 1000.0, 1.0 / dt_1000, (dt_1 - dt_ref) * 1000, 1000.0 / fps_now);
            ImGui::Text("[last 2000] %6.3f ms (%.1f FPS) ++ %6.3f ms ~ converging in %.1f secs", dt_2000 * 1000.0, 1.0 / dt_2000, (dt_1 - dt_ref) * 1000, 2000.0 / fps_now);

            //ImGui::PlotLines("Last 100", &engine->PerfDeltaTime100.Samples.Data, engine->PerfDeltaTime100.Samples.Size, engine->PerfDeltaTime100.Idx, NULL, 0.0f, dt_1000 * 1.10f, ImVec2(0.0f, ImGui::GetFontSize()));
            ImVec2 plot_size(0.0f, ImGui::GetFrameHeight() * 3);
            ImMovingAverage<double>* ma = &engine->PerfDeltaTime500;
            ImGui::PlotLines("Last 500",
                [](void* data, int n) { ImMovingAverage<double>* ma = (ImMovingAverage<double>*)data; return (float)(ma->Samples[n] * 1000); },
                ma, ma->Samples.Size, 0 * ma->Idx, NULL, 0.0f, (float)(ImMax(dt_100, dt_1000) * 1000.0 * 1.2f), plot_size);

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    ImGui::End();

    // Capture Tool
    ImGuiCaptureTool& capture_tool = engine->CaptureTool;
    capture_tool.Context.ScreenCaptureFunc = engine->IO.ScreenCaptureFunc;
    if (capture_tool.Visible)
        capture_tool.ShowCaptureToolWindow(&capture_tool.Visible);
}

//-------------------------------------------------------------------------
