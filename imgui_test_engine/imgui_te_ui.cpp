#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_te_ui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "imgui_te_internal.h"
#include "imgui_te_perftool.h"
#include "shared/imgui_utils.h"
#include "thirdparty/Str/Str.h"

//-------------------------------------------------------------------------
// TEST ENGINE: USER INTERFACE
//-------------------------------------------------------------------------
// - DrawTestLog() [internal]
// - GetVerboseLevelName() [internal]
// - ShowTestGroup() [internal]
// - ImGuiTestEngine_ShowTestWindows()
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
        e_io.SrcFileOpenFunc(buf.c_str(), line_no, e_io.SrcFileOpenUserData);
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

static bool ShowTestGroupFilterTest(ImGuiTestEngine* e, ImGuiTestGroup group, ImGuiTextFilter* filter, ImGuiTest* test)
{
    if (test->Group != group)
        return false;
    // FIXME: We cannot combine filters when used with "-remove" need to rework filtering system
    if (!filter->PassFilter(test->Name))// && !filter->PassFilter(test->Category))
        return false;
    if (e->UiFilterFailingOnly && test->Status == ImGuiTestStatus_Success)
        return false;
    return true;
}

static void ShowTestGroup(ImGuiTestEngine* e, ImGuiTestGroup group, ImGuiTextFilter* filter)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();
    ImGuiTestEngineIO& e_io = ImGuiTestEngine_GetIO(e);

    //ImGui::Text("TESTS (%d)", engine->TestsAll.Size);
    if (ImGui::Button("Run"))
    {
        for (int n = 0; n < e->TestsAll.Size; n++)
        {
            ImGuiTest* test = e->TestsAll[n];
            if (!ShowTestGroupFilterTest(e, group, filter, test))
                continue;
            ImGuiTestEngine_QueueTest(e, test, ImGuiTestRunFlags_None);
        }
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.0f);
    if (ImGui::BeginCombo("##filterbystatus", e->UiFilterFailingOnly ? "Not OK" : "All"))
    {
        if (ImGui::Selectable("All", e->UiFilterFailingOnly == false))
            e->UiFilterFailingOnly = false;
        if (ImGui::Selectable("Not OK", e->UiFilterFailingOnly == true))
            e->UiFilterFailingOnly = true;
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    const char* perflog_label = "Perf Tool";
    float filter_width = ImGui::GetWindowContentRegionMax().x - ImGui::GetCursorPos().x;
    if (group == ImGuiTestGroup_Perfs)
        filter_width -= style.ItemSpacing.x + style.FramePadding.x * 2 + ImGui::CalcTextSize(perflog_label).x;
    filter->Draw("##filter", ImMax(20.0f, filter_width));
    if (group == ImGuiTestGroup_Perfs)
    {
        ImGui::SameLine();
        if (ImGui::Button(perflog_label))
        {
            e->UiPerfToolOpen = true;
            ImGui::FocusWindow(ImGui::FindWindowByName("Dear ImGui Perf Tool"));
        }
    }

    if (ImGui::BeginTable("Tests", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Group");
        ImGui::TableSetupColumn("Test", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4) * e_io.DpiScale);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 0) * e_io.DpiScale);
        //ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(100, 10) * e_io.DpiScale);
        for (int n = 0; n < e->TestsAll.Size; n++)
        {
            ImGuiTest* test = e->TestsAll[n];
            if (!ShowTestGroupFilterTest(e, group, filter, test))
                continue;

            ImGuiTestContext* test_context = (e->TestContext && e->TestContext->Test == test) ? e->TestContext : NULL;

            ImGui::TableNextRow();
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
            case ImGuiTestStatus_Suspended:
                if (test_context && (test_context->RunFlags & ImGuiTestRunFlags_GuiFuncOnly))
                    status_color = ImVec4(0.8f, 0.0f, 0.8f, 1.0f);
                else
                    status_color = ImVec4(0.8f, 0.4f, 0.1f, 1.0f);
                break;
            default:
                status_color = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
                break;
            }

            ImGui::TableNextColumn();
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::ColorButton("status", status_color, ImGuiColorEditFlags_NoTooltip);
            ImGui::SameLine();
            if (test->Status == ImGuiTestStatus_Running || test->Status == ImGuiTestStatus_Suspended)
                ImGui::RenderText(p + style.FramePadding + ImVec2(0, 0), &"|\0/\0-\0\\"[(((ImGui::GetFrameCount() / 5) & 3) << 1)], NULL);

            bool queue_test = false;
            bool queue_gui_func_toggle = false;
            bool select_test = false;

            if (test->Status == ImGuiTestStatus_Suspended)
            {
                // Resume IM_DEBUG_HALT_TESTFUNC
                if (ImGui::Button("Con###Run"))
                    test->Status = ImGuiTestStatus_Running;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("CTRL+Space to continue.");
                if (ImGui::IsKeyPressedMap(ImGuiKey_Space) && io.KeyCtrl)
                    test->Status = ImGuiTestStatus_Running;
            }
            else
            {
                if (ImGui::Button("Run###Run"))
                   queue_test = select_test = true;
            }

            ImGui::TableNextColumn();
            if (ImGui::Selectable(test->Category, test == e->UiSelectedTest, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_SelectOnNav))
                select_test = true;

            // Double-click to run test, CTRL+Double-click to run GUI function
            const bool is_running_gui_func = (test_context && (test_context->RunFlags & ImGuiTestRunFlags_GuiFuncOnly));
            const bool has_gui_func = (test->GuiFunc != NULL);
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                if (ImGui::GetIO().KeyCtrl)
                    queue_gui_func_toggle = true;
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
                if (ImGui::MenuItem("Run GUI func", "Ctrl+DblClick", is_running_gui_func, has_gui_func))
                    queue_gui_func_toggle = true;

                ImGui::Separator();

                const bool open_source_available = (test->SourceFile != NULL) && (e->IO.SrcFileOpenFunc != NULL);
                if (open_source_available)
                {
                    Str128f buf("Open source (%s:%d)", test->SourceFileShort, test->SourceLine);
                    if (ImGui::MenuItem(buf.c_str()))
                        e->IO.SrcFileOpenFunc(test->SourceFile, test->SourceLine, e->IO.SrcFileOpenUserData);
                    if (ImGui::MenuItem("View source..."))
                        view_source = true;
                }
                else
                {
                    ImGui::MenuItem("Open source", NULL, false, false);
                    ImGui::MenuItem("View source", NULL, false, false);
                }

                if (group == ImGuiTestGroup_Perfs && ImGui::MenuItem("View perflog"))
                {
                    e->PerfTool->ViewOnly(test->Name);
                    e->UiPerfToolOpen = true;
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

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(test->Name);

            // Process selection
            if (select_test)
                e->UiSelectedTest = test;

            // Process queuing
            if (queue_gui_func_toggle && is_running_gui_func)
                ImGuiTestEngine_AbortTest(e);
            else if (queue_gui_func_toggle && !e->IO.RunningTests)
                ImGuiTestEngine_QueueTest(e, test, ImGuiTestRunFlags_ManualRun | ImGuiTestRunFlags_GuiFuncOnly);
            if (queue_test && !e->IO.RunningTests)
                ImGuiTestEngine_QueueTest(e, test, ImGuiTestRunFlags_ManualRun);

            ImGui::PopID();
        }
        ImGui::Spacing();
        ImGui::PopStyleVar(2);
        ImGui::EndTable();
    }
}

static void ImGuiTestEngine_ShowLogAndTools(ImGuiTestEngine* engine)
{
    ImGuiContext& g = *GImGui;
    if (!ImGui::BeginTabBar("##tools"))
        return;

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

    // Misc
    if (ImGui::BeginTabItem("MISC"))
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("TestEngine: HookItems: %d, HookPushId: %d, InfoTasks: %d", g.TestEngineHookItems, g.DebugHookIdInfo != 0, engine->InfoTasks.Size);
        ImGui::Separator();

        ImGui::Checkbox("Slow down whole app", &engine->ToolSlowDown);
        ImGui::SameLine(); ImGui::SetNextItemWidth(70 * engine->IO.DpiScale);
        ImGui::SliderInt("##ms", &engine->ToolSlowDownMs, 0, 400, "%d ms");
        ImGui::Checkbox("Capture when requested by API", &engine->IO.ConfigCaptureEnabled); HelpTooltip("Enable or disable screen capture.");
        ImGui::Checkbox("Capture screen on error", &engine->IO.ConfigCaptureOnError); HelpTooltip("Capture a screenshot on test failure.");

        ImGui::CheckboxFlags("io.ConfigFlags: NavEnableKeyboard", &io.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard);
        ImGui::CheckboxFlags("io.ConfigFlags: NavEnableGamepad", &io.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad);
#ifdef IMGUI_HAS_DOCK
        ImGui::Checkbox("io.ConfigDockingAlwaysTabBar", &io.ConfigDockingAlwaysTabBar);
#endif
        if (ImGui::Button("Reboot UI context"))
            engine->ToolDebugRebootUiContext = true;

        ImGui::DragFloat("DpiScale", &engine->IO.DpiScale, 0.005f, 0.0f, 0.0f, "%.2f");
        const ImGuiInputTextCallback filter_callback = [](ImGuiInputTextCallbackData* data) { return (data->EventChar == ',' || data->EventChar == ';') ? 1 : 0; };
        ImGui::InputText("Branch/Annotation", engine->IO.GitBranchName, IM_ARRAYSIZE(engine->IO.GitBranchName), ImGuiInputTextFlags_CallbackCharFilter, filter_callback, NULL);

        // Perfs
        // FIXME-TESTS: Need to be visualizing the samples/spikes.
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

static void ImGuiTestEngine_ShowTestTool(ImGuiTestEngine* engine, bool* p_open)
{
    if (engine->UiFocus)
    {
        ImGui::SetNextWindowFocus();
        engine->UiFocus = false;
    }
    if (!ImGui::Begin("Dear ImGui Test Engine", p_open))// , ImGuiWindowFlags_MenuBar);
    {
        ImGui::End();
        return;
    }

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
    if (ImGui::SmallButton(" TOOLS "))
        ImGui::OpenPopup("Tools");
    ImGui::SameLine();
    if (ImGui::BeginPopup("Tools"))
    {
        if (ImGui::Checkbox("Metrics", &engine->UiMetricsOpen)) ImGui::CloseCurrentPopup(); // FIXME: duplicate with Demo one... use macros to activate in demo?
        if (ImGui::Checkbox("Stack Tool", &engine->UiStackToolOpen)) ImGui::CloseCurrentPopup();
        if (ImGui::Checkbox("Capture Tool", &engine->UiCaptureToolOpen)) ImGui::CloseCurrentPopup();
        if (ImGui::Checkbox("Perf Tool", &engine->UiPerfToolOpen)) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::Checkbox("Fast", &engine->IO.ConfigRunFast); HelpTooltip("Run tests as fast as possible (no vsync, no delay, teleport mouse, etc.).");
    ImGui::SameLine();
    ImGui::BeginDisabled();
    ImGui::Checkbox("Blind", &engine->IO.ConfigRunBlind);
    ImGui::EndDisabled();
    HelpTooltip("<UNSUPPORTED>\nRun tests in a blind ui context.");
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
    ImGui::DragInt("PerfStress", &engine->IO.PerfStressAmount, 0.1f, 1, 20); HelpTooltip("Increase workload of performance tests (higher means longer run)."); // FIXME: Move?
    ImGui::PopStyleVar();
    ImGui::Separator();

    // SPLITTER
    // FIXME-OPT: A better splitter API supporting arbitrary number of splits would be useful.
    float list_height = 0.0f;
    float& log_height = engine->UiLogHeight;
    ImGui::Splitter("splitter", &list_height, &log_height, ImGuiAxis_Y, +1);

    // TESTS
    ImGui::BeginChild("List", ImVec2(0, list_height), false, ImGuiWindowFlags_NoScrollbar);
    if (ImGui::BeginTabBar("##Tests", ImGuiTabBarFlags_NoTooltip))  // Add _NoPushId flag in TabBar?
    {
        if (ImGui::BeginTabItem("TESTS", NULL, ImGuiTabItemFlags_NoPushId))
        {
            ShowTestGroup(engine, ImGuiTestGroup_Tests, &engine->UiFilterTests);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("PERFS", NULL, ImGuiTabItemFlags_NoPushId))
        {
            ShowTestGroup(engine, ImGuiTestGroup_Perfs, &engine->UiFilterPerfs);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();
    engine->UiSelectAndScrollToTest = NULL;

    // LOG & TOOLS
    ImGui::BeginChild("Log", ImVec2(0, log_height));
    ImGuiTestEngine_ShowLogAndTools(engine);
    ImGui::EndChild();

    ImGui::End();
}

void    ImGuiTestEngine_ShowTestWindows(ImGuiTestEngine* e, bool* p_open)
{
    // Test Tool
    ImGuiTestEngine_ShowTestTool(e, p_open);

    // Stack Tool
    if (e->UiStackToolOpen)
        ImGui::ShowStackToolWindow(&e->UiStackToolOpen);

    // Capture Tool
    ImGuiCaptureTool& capture_tool = e->CaptureTool;
    capture_tool.Context.ScreenCaptureFunc = e->IO.ScreenCaptureFunc;
    capture_tool.Context.ScreenCaptureUserData = e->IO.ScreenCaptureUserData;
    if (e->UiCaptureToolOpen)
        capture_tool.ShowCaptureToolWindow(&e->UiCaptureToolOpen);

    // Perf tool
    if (e->UiPerfToolOpen)
    {
        if (ImGui::Begin("Dear ImGui Perf Tool", &e->UiPerfToolOpen))
        {
            if (ImGui::IsWindowAppearing() && e->PerfTool->Empty())
                e->PerfTool->LoadCSV();
            e->PerfTool->ShowUI(e);
        }
        ImGui::End();
    }

    // Metrics window
    // FIXME
    if (e->UiMetricsOpen)
        ImGui::ShowMetricsWindow(&e->UiMetricsOpen);
}
