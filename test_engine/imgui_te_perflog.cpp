#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_te_perflog.h"
#include "imgui_te_internal.h"
#include "libs/Str/Str.h"
#include "libs/implot/implot.h"
#include "libs/implot/implot_internal.h"

//-------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------

typedef ImGuiID(*HashEntryFn)(ImGuiPerflogEntry* entry);
typedef void(*FormatEntryLabelFn)(ImGuiPerfLog* perflog, Str256f* result, ImGuiPerflogEntry* entry);

struct ImGuiPerfLogColumnInfo
{
    const char*     Title;
    int             Offset;
    ImGuiDataType   Type;

    template<typename T>
    T GetValue(const ImGuiPerflogEntry* entry) const { return *(T*)((const char*)entry + Offset); }
};

static int IMGUI_CDECL PerflogComparerByTimestampAndTestName(const void* lhs, const void* rhs)
{
    const ImGuiPerflogEntry* a = (const ImGuiPerflogEntry*)lhs;
    const ImGuiPerflogEntry* b = (const ImGuiPerflogEntry*)rhs;

    // Sort batches by branch
    if (strcmp(a->GitBranchName, b->GitBranchName) < 0)
        return -1;
    if (strcmp(a->GitBranchName, b->GitBranchName) > 0)
        return +1;

    // Keep batches together
    if ((ImS64)b->Timestamp - (ImS64)a->Timestamp < 0)
        return -1;
    if ((ImS64)b->Timestamp - (ImS64)a->Timestamp > 0)
        return +1;

    // And sort entries within a batch by name
    return strcmp(a->TestName, b->TestName);
}

static ImGuiPerfLog* PerfLogInstance = NULL;
static int IMGUI_CDECL CompareWithSortSpecs(const void* lhs, const void* rhs)
{
    IM_ASSERT(PerfLogInstance != NULL);
    const ImGuiTableSortSpecs* sort_specs = PerfLogInstance->_InfoTableSortSpecs;

    for (int i = 0; i < sort_specs->SpecsCount; i++)
    {
        const ImGuiTableColumnSortSpecs* specs = &sort_specs->Specs[i];
        const ImGuiPerfLogColumnInfo& col_info = PerfLogInstance->_InfoTableSortColInfo[specs->ColumnIndex];
        const ImGuiPerflogEntry* a = PerfLogInstance->_Legend[*(int*) lhs];
        const ImGuiPerflogEntry* b = PerfLogInstance->_Legend[*(int*) rhs];
        if (specs->SortDirection == ImGuiSortDirection_Ascending)
            ImSwap(a, b);

        int result = 0;
        switch (col_info.Type)
        {
        case ImGuiDataType_S32:
            result = col_info.GetValue<int>(a) < col_info.GetValue<int>(b) ? +1 : -1;
            break;
        case ImGuiDataType_Float:
            result = col_info.GetValue<float>(a) < col_info.GetValue<float>(b) ? +1 : -1;
            break;
        case ImGuiDataType_Double:
            result = col_info.GetValue<double>(a) < col_info.GetValue<double>(b) ? +1 : -1;
            break;
        case ImGuiDataType_COUNT:
            result = strcmp(col_info.GetValue<const char*>(a), col_info.GetValue<const char*>(b));
            break;
        default:
            IM_ASSERT(false);
        }
        if (result != 0)
            return result;
    }
    return 0;
}

// Dates are in format "YYYY-MM-DD"
static bool IsDateValid(const char* date)
{
    if (date[4] != '-' || date[7] != '-')
        return false;
    for (int i = 0; i < 10; i++)
    {
        if (i == 4 || i == 7)
            continue;
        if (date[i] < '0' || date[i] > '9')
            return false;
    }
    return true;
}

static ImGuiID PerflogHashTestName(ImGuiPerflogEntry* entry)
{
    return ImHashStr(entry->TestName);
}

static void PerflogFormatTestName(ImGuiPerfLog* perflog, Str256f* result, ImGuiPerflogEntry* entry)
{
    IM_UNUSED(perflog);
    result->set_ref(entry->TestName);
}

static void PerflogFormatBuildInfo(ImGuiPerfLog* perflog, Str256f* result, ImGuiPerflogEntry* entry)
{
    Str64f legend_format("x%%-%dd %%-%ds %%s %%-%ds %%-%ds %%s %%-%ds", perflog->_AlignStress, perflog->_AlignType, perflog->_AlignOs, perflog->_AlignCompiler, perflog->_AlignBranch);
    result->appendf(legend_format.c_str(), entry->PerfStressAmount, entry->BuildType, entry->Cpu, entry->OS, entry->Compiler, entry->Date, entry->GitBranchName);
}

// Copied from implot_demo.cpp and modified.
template <typename T>
int BinarySearch(const T* arr, int l, int r, const void* data, int (*compare)(const T&, const void*))
{
    if (r < l)
        return -1;
    int mid = l + (r - l) / 2;
    int cmp = compare(arr[mid], data);
    if (cmp == 0)
        return mid;
    if (cmp > 0)
        return BinarySearch(arr, l, mid - 1, data, compare);
    return BinarySearch(arr, mid + 1, r, data, compare);
}

static int CompareEntryName(const ImGuiPerflogEntry& val, const void* data)
{
    return strcmp(val.TestName, (const char*)data);
}

static bool Date(const char* label, char* date, int date_len, bool valid)
{
    ImGui::SetNextItemWidth(80.0f);
    bool date_valid = *date == 0 || (IsDateValid(date) && valid/*strcmp(_FilterDateFrom, _FilterDateTo) <= 0*/);
    if (!date_valid)
    {
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 0, 0, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
    }
    bool date_changed = ImGui::InputTextWithHint(label, "YYYY-MM-DD", date, date_len);
    if (!date_valid)
    {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }
    return date_changed;
}

struct GetPlotPointData
{
    ImGuiPerfLog*   Perf;
    double          Shift;
    int             BatchIndex;
};

static ImPlotPoint GetPlotPoint(void* data, int idx)
{
    GetPlotPointData* d = (GetPlotPointData*)data;
    ImGuiPerfLog* perf = d->Perf;
    ImGuiPerflogEntry* entry = perf->GetEntryByBatchIdx(d->BatchIndex, perf->_VisibleLabelPointers.Data[idx]);
    ImPlotPoint pt;
    pt.x = entry ? entry->DtDeltaMs : 0.0;
    pt.y = idx + d->Shift;
    return pt;
}

static void RenderFilterInput(ImGuiPerfLog* perf, const char* hint)
{
    if (ImGui::IsWindowAppearing())
        perf->_Filter.clear();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##filter", hint, &perf->_Filter);
    ImGui::SetKeyboardFocusHere();
}

static bool RenderMultiSelectFilter(ImGuiPerfLog* perf, const char* filter_hint, ImVector<ImGuiPerflogEntry*>* entries, HashEntryFn hash, FormatEntryLabelFn format)
{
    ImGuiContext& g = *GImGui;
    Str256f buf("");
    ImGuiStorage& visibility = perf->_Settings.Visibility;
    bool modified = false;
    RenderFilterInput(perf, filter_hint);

    // Keep popup open for multiple actions if SHIFT is pressed.
    if (!g.IO.KeyShift)
        ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);

    if (ImGui::MenuItem("Show All"))
    {
        for (ImGuiPerflogEntry* entry : *entries)
        {
            buf.clear();
            format(perf, &buf, entry);
            if (strstr(buf.c_str(), perf->_Filter.c_str()) != NULL)
                visibility.SetBool(hash(entry), true);
        }
    }

    if (ImGui::MenuItem("Hide All"))
    {
        for (ImGuiPerflogEntry* entry : *entries)
        {
            buf.clear();
            format(perf, &buf, entry);
            if (strstr(buf.c_str(), perf->_Filter.c_str()) != NULL)
                visibility.SetBool(hash(entry), false);
        }
    }

    int filtered_entries = 0;
    for (ImGuiPerflogEntry* entry : *entries)
    {
        buf.clear();
        format(perf, &buf, entry);
        if (strstr(buf.c_str(), perf->_Filter.c_str()) == NULL)   // Filter out entries not matching a filter query
            continue;

        if (filtered_entries == 0)
            ImGui::Separator();

        ImGuiID build_id = hash(entry);
        bool visible = visibility.GetBool(build_id, true);
        if (ImGui::MenuItem(buf.c_str(), NULL, &visible))
        {
            modified = true;
            if (g.IO.KeyCtrl)
            {
                for (ImGuiPerflogEntry* entry2 : *entries)
                {
                    ImGuiID build_id2 = hash(entry2);
                    visibility.SetBool(build_id2, !visibility.GetBool(build_id2, true));
                }
            }
            else
            {
                visibility.SetBool(build_id, !visibility.GetBool(build_id, true));
            }
        }
        filtered_entries++;
    }

    if (!g.IO.KeyShift)
        ImGui::PopItemFlag();

    return modified;
}

static void PerflogSettingsHandler_ClearAll(ImGuiContext*, ImGuiSettingsHandler* ini_handler)
{
    ImGuiPerfLog* perflog = (ImGuiPerfLog*)ini_handler->UserData;
    perflog->_Settings.Clear();
}

static void* PerflogSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char*)
{
    return (void*)1;
}

static void PerflogSettingsHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler* ini_handler, void*, const char* line)
{
    ImGuiPerfLog* perflog = (ImGuiPerfLog*)ini_handler->UserData;
    char buf[128];
    int visible, combine, branch_colors;
    /**/ if (sscanf(line, "DateFrom=%10s", perflog->_FilterDateFrom))               { }
    else if (sscanf(line, "DateTo=%10s", perflog->_FilterDateTo))                   { }
    else if (sscanf(line, "CombineByBuildInfo=%d", &combine))                       { perflog->_CombineByBuildInfo = !!combine; }
    else if (sscanf(line, "PerBranchColors=%d", &branch_colors))                    { perflog->_PerBranchColors = !!branch_colors; }
    else if (sscanf(line, "Baseline=%llu", &perflog->_Settings.BaselineTimestamp))  { }
    else if (sscanf(line, "TestVisibility=%[^,]=%d", buf, &visible))                { perflog->_Settings.Visibility.SetBool(ImHashStr(buf), !!visible); }
    else if (sscanf(line, "BuildVisibility=%[^,]=%d", buf, &visible))               { perflog->_Settings.Visibility.SetBool(ImHashStr(buf), !!visible); }
}

static void PerflogSettingsHandler_ApplyAll(ImGuiContext*, ImGuiSettingsHandler* ini_handler)
{
    ImGuiPerfLog* perflog = (ImGuiPerfLog*)ini_handler->UserData;
    perflog->_Dirty = true;
    perflog->_BaselineBatchIndex = -1;  // Index will be re-discovered from _Settings.BaselineTimestamp in _Rebuild().
}

static void PerflogSettingsHandler_WriteAll(ImGuiContext*, ImGuiSettingsHandler* ini_handler, ImGuiTextBuffer* buf)
{
    ImGuiPerfLog* perflog = (ImGuiPerfLog*)ini_handler->UserData;
    if (perflog->_Data.empty())
        return;
    buf->appendf("[%s][Data]\n", ini_handler->TypeName);
    buf->appendf("DateFrom=%s\n", perflog->_FilterDateFrom);
    buf->appendf("DateTo=%s\n", perflog->_FilterDateTo);
    buf->appendf("CombineByBuildInfo=%d\n", perflog->_CombineByBuildInfo);
    buf->appendf("PerBranchColors=%d\n", perflog->_PerBranchColors);
    if (perflog->_BaselineBatchIndex >= 0)
        buf->appendf("Baseline=%llu\n", perflog->_Legend[perflog->_BaselineBatchIndex]->Timestamp);
    for (ImGuiPerflogEntry* entry : perflog->_Labels)
        buf->appendf("TestVisibility=%s,%d\n", entry->TestName, perflog->_Settings.Visibility.GetBool(PerflogHashTestName(entry), true));

    ImGuiStorage& temp_set = perflog->_TempSet;
    temp_set.Data.clear();
    for (ImGuiPerflogEntry* entry : perflog->_Legend)
    {
        const char* properties[] = { entry->GitBranchName, entry->BuildType, entry->Cpu, entry->OS, entry->Compiler };
        for (int i = 0; i < IM_ARRAYSIZE(properties); i++)
        {
            ImGuiID hash = ImHashStr(properties[i]);
            if (!temp_set.GetBool(hash))
            {
                temp_set.SetBool(hash, true);
                buf->appendf("BuildVisibility=%s,%d\n", properties[i], perflog->_Settings.Visibility.GetBool(hash, true));
            }
        }
    }
    buf->append("\n");
}

// Copied from ImPlot::BeginPlot().
static ImRect ImPlotGetYTickRect(int t, int y = 0)
{
    ImPlotContext& gp = *GImPlot;
    ImPlotPlot& plot = *gp.CurrentPlot;
    ImRect result(1.0f, 1.0f, -1.0f, -1.0f);
    if (plot.YAxis[y].Present && !ImHasFlag(plot.YAxis[y].Flags, ImPlotAxisFlags_NoTickLabels))
    {
        const float x_start = gp.YAxisReference[y] + (y == 0 ? (-gp.Style.LabelPadding.x - gp.YTicks[y].Ticks[t].LabelSize.x) : gp.Style.LabelPadding.x);
        ImPlotTick *yt = &gp.YTicks[y].Ticks[t];
        if (yt->ShowLabel && yt->PixelPos >= plot.PlotRect.Min.y - 1 && yt->PixelPos <= plot.PlotRect.Max.y + 1)
        {
            result.Min = ImVec2(x_start, yt->PixelPos - 0.5f * yt->LabelSize.y);
            result.Max = result.Min + ImGui::CalcTextSize(gp.YTicks[y].GetText(t));
        }
    }
    return result;
}

ImGuiPerfLog::ImGuiPerfLog()
{
    ImGuiContext& g = *GImGui;
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "Perflog";
    ini_handler.TypeHash = ImHashStr("Perflog");
    ini_handler.ClearAllFn = PerflogSettingsHandler_ClearAll;
    ini_handler.ReadOpenFn = PerflogSettingsHandler_ReadOpen;
    ini_handler.ReadLineFn = PerflogSettingsHandler_ReadLine;
    ini_handler.ApplyAllFn = PerflogSettingsHandler_ApplyAll;
    ini_handler.WriteAllFn = PerflogSettingsHandler_WriteAll;
    ini_handler.UserData = this;
    g.SettingsHandlers.push_back(ini_handler);
}

bool ImGuiPerfLog::Load(const char* file_name)
{
    if (!_CSVParser.Load(file_name))
        return false;

    if (_CSVParser.Columns != 11)
    {
        IM_ASSERT(0 && "Invalid number of columns.");
        return false;
    }

    ImStrncpy(_FilterDateFrom, "9999-99-99", IM_ARRAYSIZE(_FilterDateFrom));
    ImStrncpy(_FilterDateTo, "0000-00-00", IM_ARRAYSIZE(_FilterDateFrom));

    // Read perf test entries from CSV
    for (int row = 0; row < _CSVParser.Rows; row++)
    {
        ImGuiPerflogEntry entry;
        int col = 0;
        sscanf(_CSVParser.GetCell(row, col++), "%llu", &entry.Timestamp);
        entry.Category = _CSVParser.GetCell(row, col++);
        entry.TestName = _CSVParser.GetCell(row, col++);
        sscanf(_CSVParser.GetCell(row, col++), "%lf", &entry.DtDeltaMs);
        sscanf(_CSVParser.GetCell(row, col++), "x%d", &entry.PerfStressAmount);
        entry.GitBranchName = _CSVParser.GetCell(row, col++);
        entry.BuildType = _CSVParser.GetCell(row, col++);
        entry.Cpu = _CSVParser.GetCell(row, col++);
        entry.OS = _CSVParser.GetCell(row, col++);
        entry.Compiler = _CSVParser.GetCell(row, col++);
        entry.Date = _CSVParser.GetCell(row, col++);
        _CSVData.push_back(entry);

        if (strcmp(_FilterDateFrom, entry.Date) > 0)
            ImStrncpy(_FilterDateFrom, entry.Date, IM_ARRAYSIZE(_FilterDateFrom));
        if (strcmp(_FilterDateTo, entry.Date) < 0)
            ImStrncpy(_FilterDateTo, entry.Date, IM_ARRAYSIZE(_FilterDateTo));
    }

    _Labels.resize(0);
    _Legend.resize(0);

    return true;
}

bool ImGuiPerfLog::Save(const char* file_name)
{
    // Log to .csv
    FILE* f = fopen(file_name, "wb");
    if (f == NULL)
        return false;

    for (ImGuiPerflogEntry& entry : _CSVData)
        fprintf(f, "%llu,%s,%s,%.3f,x%d,%s,%s,%s,%s,%s,%s\n", entry.Timestamp, entry.Category, entry.TestName,
            entry.DtDeltaMs, entry.PerfStressAmount, entry.GitBranchName, entry.BuildType, entry.Cpu, entry.OS,
            entry.Compiler, entry.Date);
    fflush(f);
    fclose(f);
    return true;
}

void ImGuiPerfLog::AddEntry(ImGuiPerflogEntry* entry)
{
    _CSVData.push_back(*entry);
    _Dirty = true;
}

void ImGuiPerfLog::_Rebuild()
{
    if (_CSVData.empty())
        return;

    ImGuiStorage& temp_set = _TempSet;
    temp_set.Data.resize(0);
    _Dirty = false;
    _Data.resize(0);
    _InfoTableSort.resize(0);
    _InfoTableSortDirty = true;

    // FIXME: What if entries have a varying timestep?
    if (_CombineByBuildInfo)
    {
        // Combine similar runs by build config.
        ImVector<int> counts;
        for (ImGuiPerflogEntry& entry : _CSVData)
        {
            if (_FilterDateFrom[0] && strcmp(entry.Date, _FilterDateFrom) < 0)
                continue;
            if (_FilterDateTo[0] && strcmp(entry.Date, _FilterDateTo) > 0)
                continue;

            ImGuiID build_id = ImHashStr(entry.BuildType);
            build_id = ImHashStr(entry.OS, 0, build_id);
            build_id = ImHashStr(entry.Cpu, 0, build_id);
            build_id = ImHashStr(entry.Compiler, 0, build_id);
            build_id = ImHashStr(entry.GitBranchName, 0, build_id);
            build_id = ImHashStr(entry.TestName, 0, build_id);

            int i = temp_set.GetInt(build_id, -1);
            if (i < 0)
            {
                _Data.push_back(entry);
                _Data.back().DtDeltaMs = 0.0;
                counts.push_back(0);
                i = _Data.Size - 1;
                temp_set.SetInt(build_id, i);
                IM_ASSERT(_Data.Size == counts.Size);
            }

            ImGuiPerflogEntry& new_entry = _Data[i];
            new_entry.Date = "";
            new_entry.DtDeltaMs += entry.DtDeltaMs;
            new_entry.DtDeltaMsMin = ImMin(new_entry.DtDeltaMsMin, entry.DtDeltaMs);
            new_entry.DtDeltaMsMax = ImMax(new_entry.DtDeltaMsMax, entry.DtDeltaMs);
            counts[i]++;
        }

        // Average data
        for (int i = 0; i < _Data.Size; i++)
        {
            _Data[i].NumSamples = counts[i];
            _Data[i].DtDeltaMs /= counts[i];
        }
    }
    else
    {
        // Copy to a new buffer that we are going to modify.
        for (ImGuiPerflogEntry& entry : _CSVData)
        {
            if (_FilterDateFrom[0] && strcmp(entry.Date, _FilterDateFrom) < 0)
                continue;
            if (_FilterDateTo[0] && strcmp(entry.Date, _FilterDateTo) > 0)
                continue;
            _Data.push_back(entry);
        }
    }

    if (_Data.empty())
        return;

    // Sort entries by timestamp and test name, so all tests are grouped by batch and have a consistent entry order.
    ImQsort(_Data.Data, _Data.Size, sizeof(ImGuiPerflogEntry), &PerflogComparerByTimestampAndTestName);

    // Index data for a convenient access.
    int branch_index_last = 0;
    _Legend.resize(0);
    _Labels.resize(0);
    ImU64 last_timestamp = UINT64_MAX;
    int batch_size = 0;
    temp_set.Data.resize(0);
    for (int i = 0; i < _Data.Size; i++)
    {
        ImGuiPerflogEntry* entry = &_Data[i];
        if (entry->Timestamp != last_timestamp)
        {
            _Legend.push_back(entry);
            last_timestamp = entry->Timestamp;
            batch_size = 0;
        }
        batch_size++;
        ImGuiID name_id = ImHashStr(entry->TestName);
        if (temp_set.GetInt(name_id, -1) < 0)
        {
            temp_set.SetInt(name_id, _Labels.Size);
            _Labels.push_back(entry);
        }

        // Index branches. Unique branch index is used for per-branch colors.
        ImGuiID branch_hash = ImHashStr(entry->GitBranchName);
        int unique_branch_index = temp_set.GetInt(branch_hash, -1);
        if (unique_branch_index < 0)
        {
            unique_branch_index = branch_index_last++;
            temp_set.SetInt(branch_hash, unique_branch_index);
        }
        entry->BranchIndex = unique_branch_index;
    }
    _BaselineBatchIndex = ImClamp(_BaselineBatchIndex, 0, _Legend.Size - 1);
    _CalculateLegendAlignment();
}

void ImGuiPerfLog::Clear()
{
    _CSVData.clear();
    _Data.clear();
    _Labels.clear();
    _Legend.clear();
    _Settings.Clear();
}

void ImGuiPerfLog::ShowUI(ImGuiTestEngine*)
{
#ifdef IMGUI_TEST_ENGINE_ENABLE_IMPLOT
    ImGuiContext& g = *GImGui;
    Str256f label("");
    Str256f display_label("");

    if (_Dirty)
        _Rebuild();

    // -----------------------------------------------------------------------------------------------------------------
    // Render utility buttons
    // -----------------------------------------------------------------------------------------------------------------
    // Date filter
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Date:");
    ImGui::SameLine();

    bool date_changed = Date("##date-from", _FilterDateFrom, IM_ARRAYSIZE(_FilterDateFrom), (strcmp(_FilterDateFrom, _FilterDateTo) <= 0 || !*_FilterDateTo));
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("Date From Menu");
    ImGui::SameLine(0, 0.0f);
    ImGui::TextUnformatted("..");
    ImGui::SameLine(0, 0.0f);
    date_changed |= Date("##date-to", _FilterDateTo, IM_ARRAYSIZE(_FilterDateTo), (strcmp(_FilterDateFrom, _FilterDateTo) <= 0 || !*_FilterDateFrom));
    if (date_changed)
    {
        _Dirty = (!_FilterDateFrom[0] || IsDateValid(_FilterDateFrom)) && (!_FilterDateTo[0] || IsDateValid(_FilterDateTo));
        if (_FilterDateFrom[0] && _FilterDateTo[0])
            _Dirty &= strcmp(_FilterDateFrom, _FilterDateTo) <= 0;
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("Date To Menu");
    ImGui::SameLine();

    // FIXME-PERFLOG: Share "Set Today" code and expose Set Min/Set Max for both (as "Max" for both can be useful)
    if (ImGui::BeginPopup("Date From Menu"))
    {
        if (ImGui::MenuItem("Set Min"))
        {
            for (ImGuiPerflogEntry& entry : _CSVData)
                if (strcmp(_FilterDateFrom, entry.Date) > 0)
                {
                    ImStrncpy(_FilterDateFrom, entry.Date, IM_ARRAYSIZE(_FilterDateFrom));
                    _Dirty = true;
                }
        }
        if (ImGui::MenuItem("Set Today"))
        {
            time_t now = time(NULL);
            struct tm* tm = localtime(&now);
            ImFormatString(_FilterDateFrom, IM_ARRAYSIZE(_FilterDateFrom), "%d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
            _Dirty = true;
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Date To Menu"))
    {
        if (ImGui::MenuItem("Set Max"))
        {
            for (ImGuiPerflogEntry& entry : _CSVData)
                if (strcmp(_FilterDateTo, entry.Date) < 0)
                {
                    ImStrncpy(_FilterDateTo, entry.Date, IM_ARRAYSIZE(_FilterDateTo));
                    _Dirty = true;
                }
        }
        if (ImGui::MenuItem("Set Today"))
        {
            time_t now = time(NULL);
            struct tm* tm = localtime(&now);
            ImFormatString(_FilterDateTo, IM_ARRAYSIZE(_FilterDateTo), "%d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
            _Dirty = true;
        }
        ImGui::EndPopup();
    }

    // Total visible builds.
    int num_visible_builds = 0;
    for (ImGuiPerflogEntry* entry : _Legend)
        if (_IsVisibleBuild(entry))
            num_visible_builds++;

    // Gather pointers of visible labels. ImPlot requires such array for rendering.
    _VisibleLabelPointers.resize(0);
    for (ImGuiPerflogEntry* entry : _Labels)
        if (_IsVisibleTest(entry))
            _VisibleLabelPointers.push_back(entry->TestName);

    if (ImGui::Button(Str16f("Filter builds (%d/%d)", num_visible_builds, _Legend.Size).c_str()))
        ImGui::OpenPopup("Filter builds");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Hide or show individual builds.");
    ImGui::SameLine();
    if (ImGui::Button(Str16f("Filter tests (%d/%d)", _VisibleLabelPointers.Size, _Labels.Size).c_str()))
        ImGui::OpenPopup("Filter perfs");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Hide or show individual tests.");
    ImGui::SameLine();
    _Dirty |= ImGui::Checkbox("Combine by build info", &_CombineByBuildInfo);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Combine multiple runs with same build info into one averaged build entry.");
    ImGui::SameLine();
    ImGui::Checkbox("Per Branch colors", &_PerBranchColors);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Use one color per branch. Disables baseline comparisons!");
    ImGui::SameLine();

    if (ImGui::Button("Delete Data"))
        ImGui::OpenPopup("Delete Data");
    ImGui::SameLine();

    // Align help button to the right.
    float help_pos = ImGui::GetWindowContentRegionMax().x - g.Style.FramePadding.x * 2 - ImGui::CalcTextSize("Help").x;
    if (help_pos > ImGui::GetCursorPosX())
        ImGui::SetCursorPosX(help_pos);
    if (ImGui::Button("Help"))
        ImGui::OpenPopup("Help");

    if (ImGui::BeginPopup("Delete Data"))
    {
        ImGui::TextUnformatted("Permanently remove all perflog data?");
        ImGui::TextUnformatted("This action _CAN NOT_ be undone!");
        if (ImGui::Button("Yes, I am sure"))
        {
            ImGui::CloseCurrentPopup();
            Clear();
        }
        ImGui::EndPopup();

        if (_CSVData.empty())
            return;
    }

    // FIXME-PERFLOG: Move more of this to its more suitable location.
    if (ImGui::BeginPopup("Help"))
    {
        ImGui::BulletText("Data may be filtered by enabling or disabling individual builds or perf tests.");
        ImGui::BulletText("Hold CTRL when toggling build info or perf test visibility in order to invert visibility or other items.");
        ImGui::BulletText("Hold SHIFT when toggling build info or perf test visibility in order to close popup instantly.");
        ImGui::BulletText("Data of different runs may be combined together based on build information and averaged.");
        ImGui::BulletText("To change baseline build, double-click desired build in the legend.");
        ImGui::BulletText("Plot displays perf test delta time of each build (bars) per perf test (left).");
        ImGui::BulletText("Extra information is displayed when hovering bars of a particular perf test and holding SHIFT.");
        ImGui::BulletText("This information tooltip displays performance change compared to baseline build.");
        ImGui::BulletText("Extra information tooltip will display min/max delta values as well as number of samples.");
        ImGui::BulletText("Double-click plot to fit plot into available area.");
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Filter builds"))
    {
        ImGuiStorage& temp_set = _TempSet;
        temp_set.Data.resize(0);

        static const char* columns[] = { "Branch", "Build", "CPU", "OS", "Compiler" };
        bool show_all = ImGui::Button("Show All");
        ImGui::SameLine();
        bool hide_all = ImGui::Button("Hide All");
        if (ImGui::BeginTable("PerfInfo", IM_ARRAYSIZE(columns), ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
        {
            for (int i = 0; i < IM_ARRAYSIZE(columns); i++)
                ImGui::TableSetupColumn(columns[i]);
            ImGui::TableHeadersRow();

            bool visible = true;
            for (ImGuiPerflogEntry* entry : _Legend)
            {
                bool new_row = true;
                ImGuiID hash;
                const char* properties[] = { entry->GitBranchName, entry->BuildType, entry->Cpu, entry->OS, entry->Compiler };
                for (int i = 0; i < IM_ARRAYSIZE(properties); i++)
                {
                    hash = ImHashStr(properties[i]);
                    if (temp_set.GetBool(hash))
                        continue;
                    temp_set.SetBool(hash, true);

                    if (new_row)
                        ImGui::TableNextRow();
                    new_row = false;

                    ImGui::TableSetColumnIndex(i);
                    visible = _Settings.Visibility.GetBool(hash, true) || show_all;
                    if (hide_all)
                        visible = false;
                    if (ImGui::Checkbox(properties[i], &visible) || show_all || hide_all)
                        _CalculateLegendAlignment();
                    _Settings.Visibility.SetBool(hash, visible);
                }
            }
            ImGui::EndTable();
        }
        _ClosePopupMaybe();
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Filter perfs"))
    {
        RenderMultiSelectFilter(this, "Filter by perf test", &_Labels, PerflogHashTestName, PerflogFormatTestName);
        _ClosePopupMaybe();
        ImGui::EndPopup();
    }

    // Rendering a plot of empty dataset is not possible.
    if (_Data.empty() || _VisibleLabelPointers.empty() || num_visible_builds == 0)
    {
        ImGui::TextUnformatted("No data is available. Run some perf tests or adjust filter settings.");
        return;
    }

    // Splitter between two following child windows is rendered first.
    float plot_height;
    ImGui::Splitter("splitter", &plot_height, &_InfoTableHeight, ImGuiAxis_Y, +1);

    // -----------------------------------------------------------------------------------------------------------------
    // Render a plot
    // -----------------------------------------------------------------------------------------------------------------
    int scroll_to_test = -1;
    if (ImGui::BeginChild(ImGui::GetID("plot"), ImVec2(0, plot_height)))
    {
        // A workaround for ImPlot requiring at least two labels.
        if (_VisibleLabelPointers.Size < 2)
            _VisibleLabelPointers.push_back("");

        ImPlot::SetNextPlotTicksY(0, _VisibleLabelPointers.Size - 1, _VisibleLabelPointers.Size, _VisibleLabelPointers.Data);
        if (ImPlot::GetCurrentContext()->Plots.GetByKey(ImGui::GetID("Perflog")) == NULL)
            ImPlot::FitNextPlotAxes();   // Fit plot when appearing.
        if (!ImPlot::BeginPlot("Perflog", "Delta milliseconds", "Test", ImVec2(-1, -1)))
            return;

        // Clickable test names
        bool test_name_hovered = false;
        for (int t = 0; t < _VisibleLabelPointers.Size; t++)
        {
            ImRect label_rect = ImPlotGetYTickRect(t);
            if (label_rect.IsInverted())
                continue;

            if (!test_name_hovered && label_rect.Contains(g.IO.MousePos))
            {
                test_name_hovered = true;   // Ensure only one test name is hovered when they are overlapping due to zoom level.
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                g.CurrentWindow->DrawList->AddLine(ImFloor(label_rect.GetBL()), ImFloor(label_rect.GetBR()), ImColor(g.Style.Colors[ImGuiCol_Text]));
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    scroll_to_test = t;
            }
        }

        // Plot bars
        const float occupy_h = 0.8f;
        const float h = occupy_h / num_visible_builds;
        const bool combine_by_build_info = _CombineByBuildInfo && !_Dirty;

        int bar_index = 0;
        for (ImGuiPerflogEntry*& entry : _Legend)
        {
            int batch_index = _Legend.index_from_ptr(&entry);
            if (!_IsVisibleBuild(entry))
                continue;

            // Plot bars.
            label.clear();
            display_label.clear();
            PerflogFormatBuildInfo(this, &label, entry);
            display_label.append(label.c_str());
            ImGuiID label_id;
            if (combine_by_build_info)
                display_label.appendf(Str16f(" (%%-%dd samples)", _AlignSamples).c_str(), entry->NumSamples);
            if (!combine_by_build_info && !_PerBranchColors)  // Use per-run hash when each run has unique color.
                label_id = ImHashData(&entry->Timestamp, sizeof(entry->Timestamp));
            else
                label_id = ImHashStr(label.c_str());        // Otherwise using label hash allows them to collapse in the legend.
            display_label.appendf("%s###%08X", !_PerBranchColors && _BaselineBatchIndex == batch_index ? " *" : "", label_id);
            float shift = (float)(num_visible_builds - 1) * 0.5f * h;

            // Highlight background behind bar when hovering test entry in info table.
            if (_TableHoveredTest >= 0)
            {
                ImPlotContext& gp = *GImPlot;
                ImPlotPlot& plot = *gp.CurrentPlot;
                ImRect test_bars_rect;
                float bar_min = -h * _TableHoveredBatch + shift;
                test_bars_rect.Min.x = plot.PlotRect.Min.x;
                test_bars_rect.Max.x = plot.PlotRect.Max.x;
                test_bars_rect.Min.y = ImPlot::PlotToPixels(0, (float)_TableHoveredTest + bar_min - h * 0.5f).y;
                test_bars_rect.Max.y = ImPlot::PlotToPixels(0, (float)_TableHoveredTest + bar_min + h * 0.5f).y;
                ImPlot::GetPlotDrawList()->AddRectFilled(test_bars_rect.Min, test_bars_rect.Max, ImColor(g.Style.Colors[ImGuiCol_TableRowBgAlt]));
            }

            GetPlotPointData data;
            data.Shift = -h * bar_index + shift;
            data.Perf = this;
            data.BatchIndex = batch_index;
            ImPlot::SetNextFillStyle(ImPlot::GetColormapColor(_PerBranchColors ? entry->BranchIndex : batch_index));
            ImPlot::PlotBarsHG(display_label.c_str(), &GetPlotPoint, &data, _VisibleLabelPointers.Size, h);

            // Set baseline.
            if (ImPlot::IsLegendEntryHovered(display_label.c_str()))
            {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !_PerBranchColors)
                {
                    _BaselineBatchIndex = batch_index;
                    _Dirty = true;
                }
            }
            else if (g.IO.KeyShift)
            {
                // Info tooltip with delta times of each batch for a hovered test.
                int test_index = (int)IM_ROUND(ImPlot::GetPlotMousePos().y);
                if (0 <= test_index && test_index < _VisibleLabelPointers.Size)
                {
                    const char* test_name = _VisibleLabelPointers.Data[test_index];
                    ImGuiPerflogEntry* hovered_entry = GetEntryByBatchIdx(batch_index, test_name);

                    ImGui::BeginTooltip();
                    if (bar_index == 0)
                    {
                        float w = ImGui::CalcTextSize(test_name).x;
                        float total_w = ImGui::GetContentRegionAvail().x;
                        if (total_w > w)
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (total_w - w) * 0.5f);
                        ImGui::TextUnformatted(test_name);
                    }
                    if (hovered_entry)
                        ImGui::Text("%s %.3fms", label.c_str(), hovered_entry->DtDeltaMs);
                    else
                        ImGui::Text("%s --", label.c_str());
                    ImGui::EndTooltip();
                }
            }

            bar_index++;
        }
        ImPlot::EndPlot();
    }
    ImGui::EndChild();

    // -----------------------------------------------------------------------------------------------------------------
    // Render perf test info table
    // -----------------------------------------------------------------------------------------------------------------
    if (ImGui::BeginChild(ImGui::GetID("info-table"), ImVec2(0, _InfoTableHeight)))
    {
        static const ImGuiPerfLogColumnInfo columns_combined[] = {
            { /* 00 */ "Test Name",   IM_OFFSETOF(ImGuiPerflogEntry, TestName),         ImGuiDataType_COUNT  },
            { /* 01 */ "Branch",      IM_OFFSETOF(ImGuiPerflogEntry, GitBranchName),    ImGuiDataType_COUNT  },
            { /* 02 */ "Compiler",    IM_OFFSETOF(ImGuiPerflogEntry, Compiler),         ImGuiDataType_COUNT  },
            { /* 03 */ "OS",          IM_OFFSETOF(ImGuiPerflogEntry, OS),               ImGuiDataType_COUNT  },
            { /* 04 */ "CPU",         IM_OFFSETOF(ImGuiPerflogEntry, Cpu),              ImGuiDataType_COUNT  },
            { /* 05 */ "Build",       IM_OFFSETOF(ImGuiPerflogEntry, BuildType),        ImGuiDataType_COUNT  },
            { /* 06 */ "Stress",      IM_OFFSETOF(ImGuiPerflogEntry, PerfStressAmount), ImGuiDataType_S32    },
            { /* 07 */ "Avg ms",      IM_OFFSETOF(ImGuiPerflogEntry, DtDeltaMs),        ImGuiDataType_Double },
            { /* 08 */ "Min ms",      IM_OFFSETOF(ImGuiPerflogEntry, DtDeltaMsMin),     ImGuiDataType_Double },
            { /* 09 */ "Max ms",      IM_OFFSETOF(ImGuiPerflogEntry, DtDeltaMsMax),     ImGuiDataType_Double },
            { /* 10 */ "Num samples", IM_OFFSETOF(ImGuiPerflogEntry, NumSamples),       ImGuiDataType_S32    },
            { /* 11 */ "VS Baseline", IM_OFFSETOF(ImGuiPerflogEntry, VsBaseline),       ImGuiDataType_Float  },
        };
        // Same as above, except we skip "Min ms", "Max ms" and "Num samples".
        static const ImGuiPerfLogColumnInfo columns_separate[] =
        {
            columns_combined[0], columns_combined[1], columns_combined[2], columns_combined[3], columns_combined[4],
            columns_combined[5], columns_combined[6], columns_combined[7], columns_combined[11],
        };
        const bool combine_by_build_info = _CombineByBuildInfo && !_Dirty;
        const ImGuiPerfLogColumnInfo* columns = combine_by_build_info ? columns_combined : columns_separate;
        int columns_num = combine_by_build_info ? IM_ARRAYSIZE(columns_combined) : IM_ARRAYSIZE(columns_separate);
        if (!ImGui::BeginTable("PerfInfo", columns_num, ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_SortTristate | ImGuiTableFlags_SizingFixedFit))
            return;

        // Test name column is not sorted because we do sorting only within perf runs of a particular tests, so as far
        // as sorting function is concerned all items in first column are identical.
        for (int i = 0; i < columns_num; i++)
            ImGui::TableSetupColumn(columns[i].Title, i ? 0 : ImGuiTableColumnFlags_NoSort);

        if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
            if (sorts_specs->SpecsDirty || _InfoTableSortDirty)
            {
                // Fill sort table with unsorted indices.
                sorts_specs->SpecsDirty = _InfoTableSortDirty = false;
                _InfoTableSort.resize(0);
                for (int i = 0; i < _Legend.Size; i++)
                    _InfoTableSort.push_back(i);
                if (sorts_specs->SpecsCount > 0 && _InfoTableSort.Size > 1)
                {
                    _InfoTableSortColInfo = columns;
                    _InfoTableSortSpecs = sorts_specs;
                    PerfLogInstance = this;
                    ImQsort(&_InfoTableSort[0], (size_t)_InfoTableSort.Size, sizeof(_InfoTableSort[0]), CompareWithSortSpecs);
                    _InfoTableSortColInfo = NULL;
                    _InfoTableSortSpecs = NULL;
                    PerfLogInstance = NULL;
                }
            }

        ImGuiTable* table = g.CurrentTable;
        _TableHoveredTest = -1;
        _TableHoveredBatch = -1;
        for (int label_index = 0; label_index < _Labels.Size; label_index++)
        {
            if (scroll_to_test == label_index)
            {
                ImGuiWindow* window = g.CurrentWindow;
                ImGui::SetScrollFromPosY(window->DC.CursorPos.y - window->Pos.y, 0.0f);
            }
            ImGui::TableHeadersRow();
            for (int batch_index = 0; batch_index < _Legend.Size; batch_index++)
            {
                ImGuiPerflogEntry* entry = GetEntryByBatchIdx(_InfoTableSort[batch_index], _Labels[label_index]->TestName);
                if (entry == NULL || !_IsVisibleBuild(entry))
                    continue;

                ImGui::TableNextRow();

                // Build info
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(entry->TestName);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(entry->GitBranchName);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(entry->Compiler);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(entry->OS);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(entry->Cpu);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(entry->BuildType);
                ImGui::TableNextColumn();
                ImGui::Text("x%d", entry->PerfStressAmount);

                // Avg ms
                ImGui::TableNextColumn();
                ImGui::Text("%.3lfms", entry->DtDeltaMs);

                if (combine_by_build_info)
                {
                    // Min ms
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3lfms", entry->DtDeltaMsMin);

                    // Max ms
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3lfms", entry->DtDeltaMsMax);

                    // Num samples
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", entry->NumSamples);
                }

                // VS Baseline
                ImGui::TableNextColumn();
                ImGuiPerflogEntry* baseline_entry = _PerBranchColors ? NULL : GetEntryByBatchIdx(_BaselineBatchIndex, _Labels[label_index]->TestName);
                if (baseline_entry == NULL)
                {
                    ImGui::TextUnformatted("--");
                }
                else if (entry == baseline_entry)
                {
                    ImGui::TextUnformatted("baseline");
                }
                else
                {
                    double percent_vs_first = 100.0 / baseline_entry->DtDeltaMs * entry->DtDeltaMs;
                    double dt_change = -(100.0 - percent_vs_first);
                    if (ImAbs(dt_change) > 0.001f)
                        ImGui::Text("%+.2lf%% (%s)", dt_change, dt_change < 0.0f ? "faster" : "slower");
                    else
                        ImGui::TextUnformatted("==");
                    if (dt_change != entry->VsBaseline)
                    {
                        entry->VsBaseline = dt_change;
                        _InfoTableSortDirty = true;             // Force re-sorting.
                    }
                }

                if (table->InnerClipRect.Contains(g.IO.MousePos))
                    if (table->RowPosY1 < g.IO.MousePos.y && g.IO.MousePos.y < table->RowPosY2 - 1) // FIXME-OPT: RowPosY1/RowPosY2 may overlap between adjacent rows. Compensate for that.
                    {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImColor(g.Style.Colors[ImGuiCol_TableRowBgAlt]));
                        _TableHoveredTest = label_index;
                        _TableHoveredBatch = batch_index;
                    }
            }
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
#else
    IM_ASSERT(0 && "Perflog UI is not enabled, because ImPlot is not available (IMGUI_TEST_ENGINE_ENABLE_IMPLOT is not defined).");
#endif
}

void ImGuiPerfLog::ViewOnly(const char** perf_names)
{
    // Data would not be built if we tried to view perflog of a particular test without first opening perflog via button. We need data to be built to hide perf tests.
    if (_Data.empty())
        _Rebuild();

    // Hide other perf tests.
    for (ImGuiPerflogEntry* entry : _Labels)
    {
        bool visible = false;
        for (const char** p_name = perf_names; !visible && *p_name; p_name++)
            visible |= strcmp(entry->TestName, *p_name) == 0;
        _Settings.Visibility.SetBool(PerflogHashTestName(entry), visible);
    }
}

void ImGuiPerfLog::ViewOnly(const char* perf_name)
{
    const char* names[] = { perf_name, NULL };
    ViewOnly(names);
}

ImGuiPerflogEntry* ImGuiPerfLog::GetEntryByBatchIdx(int idx, const char* perf_name)
{
    IM_ASSERT(idx < _Legend.Size);
    ImGuiPerflogEntry* first = _Legend[idx];    // _Legend contains pointers to first entry in the batch.
    ImGuiPerflogEntry* last = &_Data.back();    // Entries themselves are stored in _Data vector.
    if (perf_name == NULL)
        return first;

    // Find a specific perf test.
    int batch_size = 0;
    for (ImGuiPerflogEntry* entry = first; entry <= last && entry->Timestamp == first->Timestamp; entry++, batch_size++);
    int test_idx = BinarySearch(first, 0, batch_size - 1, perf_name, CompareEntryName);
    if (test_idx < 0)
        return nullptr;
    return &first[test_idx];
}

bool ImGuiPerfLog::_IsVisibleBuild(ImGuiPerflogEntry* entry)
{
    return _Settings.Visibility.GetBool(ImHashStr(entry->GitBranchName), true) &&
        _Settings.Visibility.GetBool(ImHashStr(entry->Compiler), true) &&
        _Settings.Visibility.GetBool(ImHashStr(entry->Cpu), true) &&
        _Settings.Visibility.GetBool(ImHashStr(entry->OS), true) &&
        _Settings.Visibility.GetBool(ImHashStr(entry->BuildType), true);
}

bool ImGuiPerfLog::_IsVisibleTest(ImGuiPerflogEntry* entry)
{
    return _Settings.Visibility.GetBool(PerflogHashTestName(entry), true);
}

void ImGuiPerfLog::_CalculateLegendAlignment()
{
    // Estimate paddings for legend format so it looks nice and aligned.
    _AlignStress = _AlignType = _AlignOs = _AlignCompiler = _AlignBranch = _AlignSamples = 0;
    for (ImGuiPerflogEntry* entry : _Legend)
    {
        if (!_IsVisibleBuild(entry))
            continue;
        _AlignStress = ImMax(_AlignStress, (int)ceil(log10(entry->PerfStressAmount)));
        _AlignType = ImMax(_AlignType, (int)strlen(entry->BuildType));
        _AlignOs = ImMax(_AlignOs, (int)strlen(entry->OS));
        _AlignCompiler = ImMax(_AlignCompiler, (int)strlen(entry->Compiler));
        _AlignBranch = ImMax(_AlignBranch, (int)strlen(entry->GitBranchName));
        _AlignSamples = ImMax(_AlignSamples, (int)Str16f("%d", entry->NumSamples).length());
    }
}

void ImGuiPerfLog::_ClosePopupMaybe()
{
    ImGuiContext& g = *GImGui;
    if (ImGui::IsKeyPressedMap(ImGuiKey_Escape) || (ImGui::IsMouseClicked(0) && g.HoveredWindow != g.CurrentWindow))
        ImGui::CloseCurrentPopup();
}
