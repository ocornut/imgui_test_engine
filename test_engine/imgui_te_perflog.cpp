#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_te_perflog.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_te_util.h"
#include "libs/Str/Str.h"
#include "libs/implot/implot.h"
#include "libs/implot/implot_internal.h"
#include "shared/imgui_utils.h"

// For tests
#include "imgui_te_engine.h"
#include "imgui_te_context.h"

//-------------------------------------------------------------------------
// ImGuiPerflogEntry
//-------------------------------------------------------------------------

void ImGuiPerflogEntry::TakeDataOwnership()
{
    if (DataOwner)
        return;
    DataOwner = true;
    Category = ImStrdup(Category);
    TestName = ImStrdup(TestName);
    GitBranchName = ImStrdup(GitBranchName);
    BuildType = ImStrdup(BuildType);
    Cpu = ImStrdup(Cpu);
    OS = ImStrdup(OS);
    Compiler = ImStrdup(Compiler);
    Date = ImStrdup(Date);
}

ImGuiPerflogEntry::~ImGuiPerflogEntry()
{
    if (!DataOwner)
        return;
    IM_FREE((void*)Category);
    IM_FREE((void*)TestName);
    IM_FREE((void*)GitBranchName);
    IM_FREE((void*)BuildType);
    IM_FREE((void*)Cpu);
    IM_FREE((void*)OS);
    IM_FREE((void*)Compiler);
    IM_FREE((void*)Date);
}

//-------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------

typedef ImGuiID(*HashEntryFn)(ImGuiPerflogEntry* entry);
typedef void(*FormatEntryLabelFn)(ImGuiPerfLog* perflog, Str* result, ImGuiPerflogEntry* entry);

struct ImGuiPerfLogColumnInfo
{
    const char*     Title;
    int             Offset;
    ImGuiDataType   Type;
    bool            ShowAlways;

    template<typename T>
    T GetValue(const ImGuiPerflogEntry* entry) const { return *(T*)((const char*)entry + Offset); }
};

// Update _ShowEntriesTable() and SaveReport() when adding new entries.
static const ImGuiPerfLogColumnInfo PerfLogColumnInfo[] =
{
    { /* 00 */ "Test Name",   IM_OFFSETOF(ImGuiPerflogEntry, TestName),         ImGuiDataType_COUNT,  true  },
    { /* 01 */ "Branch",      IM_OFFSETOF(ImGuiPerflogEntry, GitBranchName),    ImGuiDataType_COUNT,  true  },
    { /* 02 */ "Compiler",    IM_OFFSETOF(ImGuiPerflogEntry, Compiler),         ImGuiDataType_COUNT,  true  },
    { /* 03 */ "OS",          IM_OFFSETOF(ImGuiPerflogEntry, OS),               ImGuiDataType_COUNT,  true  },
    { /* 04 */ "CPU",         IM_OFFSETOF(ImGuiPerflogEntry, Cpu),              ImGuiDataType_COUNT,  true  },
    { /* 05 */ "Build",       IM_OFFSETOF(ImGuiPerflogEntry, BuildType),        ImGuiDataType_COUNT,  true  },
    { /* 06 */ "Stress",      IM_OFFSETOF(ImGuiPerflogEntry, PerfStressAmount), ImGuiDataType_S32,    true  },
    { /* 07 */ "Avg ms",      IM_OFFSETOF(ImGuiPerflogEntry, DtDeltaMs),        ImGuiDataType_Double, true  },
    { /* 08 */ "Min ms",      IM_OFFSETOF(ImGuiPerflogEntry, DtDeltaMsMin),     ImGuiDataType_Double, false },
    { /* 09 */ "Max ms",      IM_OFFSETOF(ImGuiPerflogEntry, DtDeltaMsMax),     ImGuiDataType_Double, false },
    { /* 10 */ "Samples",     IM_OFFSETOF(ImGuiPerflogEntry, NumSamples),       ImGuiDataType_S32,    false },
    { /* 11 */ "VS Baseline", IM_OFFSETOF(ImGuiPerflogEntry, VsBaseline),       ImGuiDataType_Float,  true  },
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
        const ImGuiPerfLogColumnInfo& col_info = PerfLogColumnInfo[specs->ColumnIndex];
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

static float FormatVsBaseline(ImGuiPerflogEntry* entry, ImGuiPerflogEntry* baseline_entry, Str30f& out_label)
{
    if (baseline_entry == NULL)
    {
        out_label.appendf("--");
        return FLT_MAX;
    }

    if (entry == baseline_entry)
    {
        out_label.append("baseline");
        return FLT_MAX;
    }

    double percent_vs_first = 100.0 / baseline_entry->DtDeltaMs * entry->DtDeltaMs;
    double dt_change = -(100.0 - percent_vs_first);
    if (ImAbs(dt_change) > 0.001f)
        out_label.appendf("%+.2lf%% (%s)", dt_change, dt_change < 0.0f ? "faster" : "slower");
    else
        out_label.appendf("==");
    return (float)dt_change;
}

static ImGuiID PerflogHashTestName(ImGuiPerflogEntry* entry)
{
    return ImHashStr(entry->TestName);
}

static void PerflogFormatTestName(ImGuiPerfLog* perflog, Str* result, ImGuiPerflogEntry* entry)
{
    IM_UNUSED(perflog);
    result->set_ref(entry->TestName);
}

#ifdef IMGUI_TEST_ENGINE_ENABLE_IMPLOT
static void PerflogFormatBuildInfo(ImGuiPerfLog* perflog, Str* result, ImGuiPerflogEntry* entry)
{
    Str64f legend_format("x%%-%dd %%-%ds %%-%ds %%-%ds %%s %%-%ds %%s%%s%%s%%s(%%-%dd sample%%s)%%s",
        perflog->_AlignStress, perflog->_AlignType, perflog->_AlignOs, perflog->_AlignCompiler, perflog->_AlignBranch,
        perflog->_AlignSamples);
    result->appendf(legend_format.c_str(), entry->PerfStressAmount, entry->BuildType, entry->Cpu, entry->OS,
        entry->Compiler, entry->GitBranchName, entry->Date,
#if 0
        // Show min-max dates.
        perflog->_CombineByBuildInfo ? " - " : "", entry->DateMax ? entry->DateMax : "",
#else
        "", "",
#endif
        *entry->Date ? " " : "", entry->NumSamples,
        entry->NumSamples > 1 ? "s" : "", entry->NumSamples > 1 || perflog->_AlignSamples == 1 ? "" : " ");
}
#endif

static int PerfLogCountVisibleBuilds(ImGuiPerfLog* perflog, const char* perf_test_name = NULL)
{
    int num_visible_builds = 0;
    for (int batch_idx = 0; batch_idx < perflog->_Legend.Size; batch_idx++)
    {
        ImGuiPerflogEntry* entry;
        if (perf_test_name == NULL)
            entry = perflog->_Legend.Data[batch_idx];
        else
            entry = perflog->GetEntryByBatchIdx(batch_idx, perf_test_name);
        if (entry && perflog->_IsVisibleBuild(entry))
            num_visible_builds++;
    }
    return num_visible_builds;
}

static void LogUpdateVisibleLabels(ImGuiPerfLog* perflog)
{
    perflog->_VisibleLabelPointers.resize(0);
    for (ImGuiPerflogEntry* entry : perflog->_Labels)
        if (perflog->_IsVisibleTest(entry))
            for (int batch_idx = 0; batch_idx < perflog->_Legend.Size; batch_idx++)
                if (perflog->GetEntryByBatchIdx(batch_idx, entry->TestName) != NULL && perflog->_IsVisibleBuild(perflog->_Legend.Data[batch_idx]))
                {
                    perflog->_VisibleLabelPointers.push_front(entry->TestName);
                    break;
                }
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

static ImGuiID GetBuildID(ImGuiPerflogEntry* entry)
{
    ImGuiID build_id = ImHashStr(entry->BuildType);
    build_id = ImHashStr(entry->OS, 0, build_id);
    build_id = ImHashStr(entry->Cpu, 0, build_id);
    build_id = ImHashStr(entry->Compiler, 0, build_id);
    build_id = ImHashStr(entry->GitBranchName, 0, build_id);
    build_id = ImHashStr(entry->TestName, 0, build_id);
    return build_id;
}

static int CompareEntryName(const ImGuiPerflogEntry& val, const void* data)
{
    return strcmp(val.TestName, (const char*)data);
}

static bool Date(const char* label, char* date, int date_len, bool valid)
{
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("YYYY-MM-DD").x);
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
    float           BarHeight;
    int             BatchIndex;
};

#ifdef IMGUI_TEST_ENGINE_ENABLE_IMPLOT

static int PerflogGetGetMaxVisibleBuildsPerTest(ImGuiPerfLog* perf)
{
    int max_visible = 0;
    for (int i = 0; i < perf->_VisibleLabelPointers.Size; i++)
        max_visible = ImMax(PerfLogCountVisibleBuilds(perf, perf->_VisibleLabelPointers.Data[i]), max_visible);
    return max_visible;
}

// Convert global batch index to visible batch index for a particular test.
static int PerfLogBatchToVisibleIdx(ImGuiPerfLog* perf, int batch_idx, const char* perf_test_name)
{
    IM_ASSERT(batch_idx < perf->_Legend.Size);
    int visible_idx = 0;
    for (int i = 0; i < perf->_Legend.Size; i++)
    {
        if (i == batch_idx)
            return visible_idx;
        if (perf->GetEntryByBatchIdx(i, perf_test_name) != NULL && perf->_IsVisibleBuild(perf->_Legend.Data[i]))
            visible_idx++;
    }
    IM_ASSERT(0);
    return 0;
}

// Return Y in plot coordinates for a particular bar.
static float PerflogGetBarY(ImGuiPerfLog* perf, int batch_idx, int perf_idx, float bar_height)
{
    IM_ASSERT(batch_idx < perf->_Legend.Size);
    IM_ASSERT(perf_idx < perf->_VisibleLabelPointers.Size);
    const char* perf_test_name = perf->_VisibleLabelPointers.Data[perf_idx];
    if (!*perf_test_name)
        return 0;
    int num_visible_builds = PerfLogCountVisibleBuilds(perf, perf_test_name);
    batch_idx = PerfLogBatchToVisibleIdx(perf, batch_idx, perf_test_name);
    IM_ASSERT(num_visible_builds > 0);
    float shift;
    shift = (float)(num_visible_builds - 1) * bar_height;
    if (num_visible_builds > 0 && (num_visible_builds % 2) == 0)
        shift -= bar_height * 0.5f;
    return (float)perf_idx - bar_height * (float)batch_idx + shift;
}

static ImPlotPoint PerflogGetPlotPoint(void* data, int idx)
{
    GetPlotPointData* d = (GetPlotPointData*)data;
    ImGuiPerfLog* perf = d->Perf;
    const char* perf_test_name = perf->_VisibleLabelPointers.Data[idx];
    ImGuiPerflogEntry* entry = perf->GetEntryByBatchIdx(d->BatchIndex, perf_test_name);
    ImPlotPoint pt;
    pt.x = entry ? entry->DtDeltaMs : 0.0;
    pt.y = PerflogGetBarY(perf, d->BatchIndex, idx, d->BarHeight);
    return pt;
}
#endif

static void RenderFilterInput(ImGuiPerfLog* perf, const char* hint, float width = -FLT_MIN)
{
    if (ImGui::IsWindowAppearing())
        strcpy(perf->_Filter, "");
    ImGui::SetNextItemWidth(width);
    ImGui::InputTextWithHint("##filter", hint, perf->_Filter, IM_ARRAYSIZE(perf->_Filter));
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
}

static bool RenderMultiSelectFilter(ImGuiPerfLog* perf, const char* filter_hint, ImVector<ImGuiPerflogEntry*>* entries, HashEntryFn hash, FormatEntryLabelFn format)
{
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiIO& io = ImGui::GetIO();
    Str256 buf;
    ImGuiStorage& visibility = perf->_Settings.Visibility;
    bool modified = false;
    RenderFilterInput(perf, filter_hint, -ImGui::CalcTextSize("?").x - g.Style.ItemSpacing.x);
    ImGui::SameLine();
    ImGui::TextDisabled("?");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Hold CTRL to invert other items.\nHold SHIFT to close popup instantly.");

    // Keep popup open for multiple actions if SHIFT is pressed.
    if (!io.KeyShift)
        ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);

    if (ImGui::MenuItem("Show All"))
    {
        for (ImGuiPerflogEntry* entry : *entries)
        {
            buf.clear();
            format(perf, &buf, entry);
            if (strstr(buf.c_str(), perf->_Filter) != NULL)
                visibility.SetBool(hash(entry), true);
        }
    }

    if (ImGui::MenuItem("Hide All"))
    {
        for (ImGuiPerflogEntry* entry : *entries)
        {
            buf.clear();
            format(perf, &buf, entry);
            if (strstr(buf.c_str(), perf->_Filter) != NULL)
                visibility.SetBool(hash(entry), false);
        }
    }

    int filtered_entries = 0;
    for (ImGuiPerflogEntry* entry : *entries)
    {
        buf.clear();
        format(perf, &buf, entry);
        if (strstr(buf.c_str(), perf->_Filter) == NULL)   // Filter out entries not matching a filter query
            continue;

        if (filtered_entries == 0)
            ImGui::Separator();

        ImGuiID build_id = hash(entry);
        bool visible = visibility.GetBool(build_id, true);
        if (ImGui::MenuItem(buf.c_str(), NULL, &visible))
        {
            modified = true;
            if (io.KeyCtrl)
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

    if (!io.KeyShift)
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
    perflog->_FilteredData.resize(0);
    perflog->_BaselineBatchIndex = -1;  // Index will be re-discovered from _Settings.BaselineTimestamp in _Rebuild().
}

static void PerflogSettingsHandler_WriteAll(ImGuiContext*, ImGuiSettingsHandler* ini_handler, ImGuiTextBuffer* buf)
{
    ImGuiPerfLog* perflog = (ImGuiPerfLog*)ini_handler->UserData;
    if (perflog->_FilteredData.empty())
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
#ifdef IMGUI_TEST_ENGINE_ENABLE_IMPLOT
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
#endif

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

    Clear();
}

ImGuiPerfLog::~ImGuiPerfLog()
{
    _SrcData.clear_destruct();
    _FilteredData.clear_destruct();
}

void ImGuiPerfLog::AddEntry(ImGuiPerflogEntry* entry)
{
    if (strcmp(_FilterDateFrom, entry->Date) > 0)
        ImStrncpy(_FilterDateFrom, entry->Date, IM_ARRAYSIZE(_FilterDateFrom));
    if (strcmp(_FilterDateTo, entry->Date) < 0)
        ImStrncpy(_FilterDateTo, entry->Date, IM_ARRAYSIZE(_FilterDateTo));

    _SrcData.push_back(*entry);
    _SrcData.back().TakeDataOwnership();
    _FilteredData.resize(0);
}

void ImGuiPerfLog::_Rebuild()
{
    if (_SrcData.empty())
        return;

    ImGuiStorage& temp_set = _TempSet;
    temp_set.Data.resize(0);
    _FilteredData.resize(0);
    _InfoTableSort.resize(0);
    _InfoTableSortDirty = true;

    // FIXME: What if entries have a varying timestep?
    if (_CombineByBuildInfo)
    {
        // Combine similar runs by build config.
        ImVector<int> counts;
        for (ImGuiPerflogEntry& entry : _SrcData)
        {
            if (_FilterDateFrom[0] && strcmp(entry.Date, _FilterDateFrom) < 0)
                continue;
            if (_FilterDateTo[0] && strcmp(entry.Date, _FilterDateTo) > 0)
                continue;

            ImGuiID build_id = GetBuildID(&entry);
            int i = temp_set.GetInt(build_id, -1);
            if (i < 0)
            {
                _FilteredData.push_back(entry);
                _FilteredData.back().DtDeltaMs = 0.0;
                counts.push_back(0);
                i = _FilteredData.Size - 1;
                temp_set.SetInt(build_id, i);
                IM_ASSERT(_FilteredData.Size == counts.Size);
            }

            ImGuiPerflogEntry& new_entry = _FilteredData[i];
#if 0
            // Find min-max dates.
            if (new_entry.DateMax == NULL || strcmp(entry.Date, new_entry.DateMax) > 0)
                new_entry.DateMax = entry.Date;
            if (strcmp(entry.Date, new_entry.Date) < 0)
                new_entry.Date = entry.Date;
#else
            new_entry.Date = "";
#endif
            new_entry.DtDeltaMs += entry.DtDeltaMs;
            new_entry.DtDeltaMsMin = ImMin(new_entry.DtDeltaMsMin, entry.DtDeltaMs);
            new_entry.DtDeltaMsMax = ImMax(new_entry.DtDeltaMsMax, entry.DtDeltaMs);
            counts[i]++;
        }

        // Average data
        for (int i = 0; i < _FilteredData.Size; i++)
        {
            _FilteredData[i].NumSamples = counts[i];
            _FilteredData[i].DtDeltaMs /= counts[i];
        }
    }
    else
    {
        // Copy to a new buffer that we are going to modify.
        for (ImGuiPerflogEntry& entry : _SrcData)
        {
            if (_FilterDateFrom[0] && strcmp(entry.Date, _FilterDateFrom) < 0)
                continue;
            if (_FilterDateTo[0] && strcmp(entry.Date, _FilterDateTo) > 0)
                continue;
            _FilteredData.push_back(entry);
        }

        // Calculate number of matching build samples for display, when ImPlot collapses these builds into a single legend entry.
        ImGuiStorage& counts = _TempSet;
        counts.Data.resize(0);
        for (ImGuiPerflogEntry& entry : _FilteredData)
        {
            ImGuiID build_id = GetBuildID(&entry);
            counts.SetInt(build_id, counts.GetInt(build_id, 0) + 1);
        }

        for (ImGuiPerflogEntry& entry : _FilteredData)
            entry.NumSamples = counts.GetInt(GetBuildID(&entry), 0);
    }

    if (_FilteredData.empty())
        return;

    // Sort entries by timestamp and test name, so all tests are grouped by batch and have a consistent entry order.
    ImQsort(_FilteredData.Data, _FilteredData.Size, sizeof(ImGuiPerflogEntry), &PerflogComparerByTimestampAndTestName);

    // Index data for a convenient access.
    int branch_index_last = 0;
    _Legend.resize(0);
    _Labels.resize(0);
    ImU64 last_timestamp = UINT64_MAX;
    int batch_size = 0;
    temp_set.Data.resize(0);
    for (int i = 0; i < _FilteredData.Size; i++)
    {
        ImGuiPerflogEntry* entry = &_FilteredData[i];
        entry->DataOwner = false;
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
    _Labels.clear();
    _Legend.clear();
    _Settings.Clear();
    _FilteredData.clear();
    for (ImGuiPerflogEntry& entry : _SrcData)
        entry.~ImGuiPerflogEntry();
    _SrcData.clear();

    ImStrncpy(_FilterDateFrom, "9999-99-99", IM_ARRAYSIZE(_FilterDateFrom));
    ImStrncpy(_FilterDateTo, "0000-00-00", IM_ARRAYSIZE(_FilterDateFrom));
}

bool ImGuiPerfLog::LoadCSV(const char* filename)
{
    if (filename == NULL)
        filename = IMGUI_PERFLOG_FILENAME;

    ImGuiCSVParser csv(11);
    if (!csv.Load(filename))
        return false;

    // Read perf test entries from CSV
    Clear();
    for (int row = 0; row < csv.Rows; row++)
    {
        ImGuiPerflogEntry entry;
        int col = 0;
        sscanf(csv.GetCell(row, col++), "%llu", &entry.Timestamp);
        entry.Category = csv.GetCell(row, col++);
        entry.TestName = csv.GetCell(row, col++);
        sscanf(csv.GetCell(row, col++), "%lf", &entry.DtDeltaMs);
        sscanf(csv.GetCell(row, col++), "x%d", &entry.PerfStressAmount);
        entry.GitBranchName = csv.GetCell(row, col++);
        entry.BuildType = csv.GetCell(row, col++);
        entry.Cpu = csv.GetCell(row, col++);
        entry.OS = csv.GetCell(row, col++);
        entry.Compiler = csv.GetCell(row, col++);
        entry.Date = csv.GetCell(row, col++);
        AddEntry(&entry);
    }

    return true;
}

// This is declared as a standalone function in order to run without a Perflog instance
void ImGuiTestEngine_PerflogAppendToCSV(ImGuiPerfLog* perf_log, ImGuiPerflogEntry* entry, const char* filename)
{
    if (filename == NULL)
        filename = IMGUI_PERFLOG_FILENAME;

    // Appends to .csv
    FILE* f = fopen(filename, "a+b");
    if (f == NULL)
    {
        fprintf(stderr, "Unable to open '%s', perflog entry was not saved.\n", filename);
        return;
    }
    fprintf(f, "%llu,%s,%s,%.3f,x%d,%s,%s,%s,%s,%s,%s\n", entry->Timestamp, entry->Category, entry->TestName,
        entry->DtDeltaMs, entry->PerfStressAmount, entry->GitBranchName, entry->BuildType, entry->Cpu, entry->OS,
        entry->Compiler, entry->Date);
    fflush(f);
    fclose(f);

    // Register to runtime perf tool if any
    if (perf_log != NULL)
        perf_log->AddEntry(entry);
}

void ImGuiPerfLog::ShowUI()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // -----------------------------------------------------------------------------------------------------------------
    // Render utility buttons
    // -----------------------------------------------------------------------------------------------------------------

    // Date filter
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Date:");
    ImGui::SameLine();

    bool dirty = _FilteredData.empty();
    bool date_changed = Date("##date-from", _FilterDateFrom, IM_ARRAYSIZE(_FilterDateFrom), (strcmp(_FilterDateFrom, _FilterDateTo) <= 0 || !*_FilterDateTo));
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("Date From Menu");
    ImGui::SameLine(0, 0.0f);
    ImGui::TextUnformatted("..");
    ImGui::SameLine(0, 0.0f);
    date_changed |= Date("##date-to", _FilterDateTo, IM_ARRAYSIZE(_FilterDateTo), (strcmp(_FilterDateFrom, _FilterDateTo) <= 0 || !*_FilterDateFrom));
    if (date_changed)
    {
        dirty = (!_FilterDateFrom[0] || IsDateValid(_FilterDateFrom)) && (!_FilterDateTo[0] || IsDateValid(_FilterDateTo));
        if (_FilterDateFrom[0] && _FilterDateTo[0])
            dirty &= strcmp(_FilterDateFrom, _FilterDateTo) <= 0;
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("Date To Menu");
    ImGui::SameLine();

    for (int i = 0; i < 2; i++)
    {
        if (ImGui::BeginPopup(i == 0 ? "Date From Menu" : "Date To Menu"))
        {
            char* date = i == 0 ? _FilterDateFrom : _FilterDateTo;
            int date_size = i == 0 ? IM_ARRAYSIZE(_FilterDateFrom) : IM_ARRAYSIZE(_FilterDateTo);
            if (i == 0 && ImGui::MenuItem("Set Min"))
            {
                for (ImGuiPerflogEntry& entry : _SrcData)
                    if (strcmp(date, entry.Date) > 0)
                    {
                        ImStrncpy(date, entry.Date, date_size);
                        dirty = true;
                    }
            }
            if (ImGui::MenuItem("Set Max"))
            {
                for (ImGuiPerflogEntry& entry : _SrcData)
                    if (strcmp(date, entry.Date) < 0)
                    {
                        ImStrncpy(date, entry.Date, date_size);
                        dirty = true;
                    }
            }
            if (ImGui::MenuItem("Set Today"))
            {
                time_t now = time(NULL);
                struct tm* tm = localtime(&now);
                ImFormatString(date, date_size, "%d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
                dirty = true;
            }
            ImGui::EndPopup();
        }
    }

    if (ImGui::Button(Str16f("Filter builds (%d/%d)###Filter builds", _NumVisibleBuilds, _Legend.Size).c_str()))
        ImGui::OpenPopup("Filter builds");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Hide or show individual builds.");
    ImGui::SameLine();
    if (ImGui::Button(Str16f("Filter tests (%d/%d)###Filter tests", _VisibleLabelPointers.Size, _Labels.Size).c_str()))
        ImGui::OpenPopup("Filter perfs");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Hide or show individual tests.");
    ImGui::SameLine();
    dirty |= ImGui::Checkbox("Combine by build info", &_CombineByBuildInfo);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Combine multiple runs with same build info into one averaged build entry.");
    ImGui::SameLine();
    ImGui::Checkbox("Per branch colors", &_PerBranchColors);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Use one color per branch. Disables baseline comparisons!");
    ImGui::SameLine();

    // Align help button to the right.
    float help_pos = ImGui::GetWindowContentRegionMax().x - style.FramePadding.x * 2 - ImGui::CalcTextSize("Help").x;
    if (help_pos > ImGui::GetCursorPosX())
        ImGui::SetCursorPosX(help_pos);
    if (ImGui::Button("Help"))
        ImGui::OpenPopup("Help");

    // FIXME-PERFLOG: Move more of this to its more suitable location.
    if (ImGui::BeginPopup("Help"))
    {
        ImGui::BulletText("To change baseline build, double-click desired build in the legend.");
        ImGui::BulletText("Extra information is displayed when hovering bars of a particular perf test and holding SHIFT.");
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

            // Find columns with nothing checked.
            bool checked_any[] = { false, false, false, false, false };
            for (ImGuiPerflogEntry* entry : _Legend)
            {
                const char* properties[] = { entry->GitBranchName, entry->BuildType, entry->Cpu, entry->OS, entry->Compiler };
                for (int i = 0; i < IM_ARRAYSIZE(properties); i++)
                {
                    ImGuiID hash = ImHashStr(properties[i]);
                    checked_any[i] |= _Settings.Visibility.GetBool(hash, true);
                }
            }

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
                    if (!checked_any[i])
                    {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImColor(1.0f, 0.0f, 0.0f, 0.2f));
                        if (ImGui::TableGetColumnFlags() & ImGuiTableColumnFlags_IsHovered)
                            ImGui::SetTooltip("Check at least one item in each column to see any data.");
                    }
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Filter perfs"))
    {
        RenderMultiSelectFilter(this, "Filter by perf test", &_Labels, PerflogHashTestName, PerflogFormatTestName);
        if (ImGui::IsKeyPressedMap(ImGuiKey_Escape))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (dirty)
        _Rebuild();
    _NumVisibleBuilds = PerfLogCountVisibleBuilds(this);
    LogUpdateVisibleLabels(this);

    // Rendering a plot of empty dataset is not possible.
    if (_FilteredData.empty() || _VisibleLabelPointers.empty() || _NumVisibleBuilds == 0)
    {
        ImGui::TextUnformatted("No data is available. Run some perf tests or adjust filter settings.");
        return;
    }

#ifdef IMGUI_TEST_ENGINE_ENABLE_IMPLOT
    // Splitter between two following child windows is rendered first.
    float plot_height = 0.0f;
    float& table_height = _InfoTableHeight;
    ImGui::Splitter("splitter", &plot_height, &table_height, ImGuiAxis_Y, +1);

    // Double-click to move splitter to bottom
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        table_height = 0;
        plot_height = ImGui::GetContentRegionAvail().y - style.ItemSpacing.y;
        ImGui::ClearActiveID();
    }

    // Render entries plot
    if (ImGui::BeginChild(ImGui::GetID("plot"), ImVec2(0, plot_height)))
        _ShowEntriesPlot();
    ImGui::EndChild();

    // Render entries tables
    if (table_height > 0.0f)
    {
        if (ImGui::BeginChild(ImGui::GetID("info-table"), ImVec2(0, table_height)))
            _ShowEntriesTable();
        ImGui::EndChild();
    }
#else
    _ShowEntriesTable();
#endif
}

void ImGuiPerfLog::_ShowEntriesPlot()
{
#ifdef IMGUI_TEST_ENGINE_ENABLE_IMPLOT
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    Str256 label;
    Str256 display_label;

    // A workaround for ImPlot requiring at least two labels.
    if (_VisibleLabelPointers.Size < 2)
        _VisibleLabelPointers.push_back("");

    ImPlot::SetNextPlotTicksY(0, _VisibleLabelPointers.Size - 1, _VisibleLabelPointers.Size, _VisibleLabelPointers.Data);
    if (ImPlot::GetCurrentContext()->Plots.GetByKey(ImGui::GetID("Perflog")) == NULL)
        ImPlot::FitNextPlotAxes();   // Fit plot when appearing.
    if (!ImPlot::BeginPlot("Perflog", NULL, NULL, ImVec2(-1, -1), ImPlotFlags_NoTitle, ImPlotAxisFlags_NoTickLabels))
        return;

    static bool plot_initialized = false;
    if (!plot_initialized)
    {
        plot_initialized = true;
        ImPlotPlot& plot = *ImPlot::GetCurrentPlot();
        plot.LegendLocation = ImPlotLocation_NorthEast;
    }

    // Plot bars
    const int max_visibler_builds = PerflogGetGetMaxVisibleBuildsPerTest(this);
    const float occupy_h = 0.8f;
    const float h = occupy_h / (float)max_visibler_builds;

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
        if (!_CombineByBuildInfo && !_PerBranchColors)  // Use per-run hash when each run has unique color.
            label_id = ImHashData(&entry->Timestamp, sizeof(entry->Timestamp));
        else
            label_id = ImHashStr(label.c_str());        // Otherwise using label hash allows them to collapse in the legend.
        display_label.appendf("%s###%08X", _BaselineBatchIndex == batch_index ? " *" : "", label_id);

        GetPlotPointData data = {};
        data.BarHeight = h;
        data.Perf = this;
        data.BatchIndex = batch_index;
        ImPlot::SetNextFillStyle(ImPlot::GetColormapColor(_PerBranchColors ? entry->BranchIndex : batch_index));
        ImPlot::PlotBarsHG(display_label.c_str(), &PerflogGetPlotPoint, &data, _VisibleLabelPointers.Size, h);

        // Set baseline.
        if (ImPlot::IsLegendEntryHovered(display_label.c_str()))
        {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                _BaselineBatchIndex = batch_index;
        }
        bar_index++;
    }

    // Highlight background behind bar when hovering test entry in info table.
    ImPlotContext& gp = *GImPlot;
    ImPlotPlot& plot = *gp.CurrentPlot;
    ImRect test_bars_rect;
    _PlotHoverTest = -1;
    _PlotHoverBatch = -1;
    _PlotHoverTestLabel = false;
    for (int i = 0; i < _VisibleLabelPointers.Size; i++)
    {
        ImRect label_rect = ImPlotGetYTickRect(i);
        ImRect label_rect_hover = label_rect;
        label_rect_hover.Min.x = plot.CanvasRect.Min.x;
        ImRect bars_rect_combined(+FLT_MAX, +FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (ImGuiPerflogEntry*& entry : _Legend)
        {
            int batch_index = _Legend.index_from_ptr(&entry);
            if (!_IsVisibleBuild(entry))
                continue;

            float pad_min = batch_index == 0 ? h * 0.25f : 0;
            float pad_max = batch_index == _Legend.Size - 1 ? h * 0.25f : 0;
            float bar_min = PerflogGetBarY(this, batch_index, i, h);
            test_bars_rect.Min.x = plot.PlotRect.Min.x;
            test_bars_rect.Max.x = plot.PlotRect.Max.x;
            test_bars_rect.Min.y = ImPlot::PlotToPixels(0, bar_min + h * 0.5f + pad_min).y;
            test_bars_rect.Max.y = ImPlot::PlotToPixels(0, bar_min - h * 0.5f - pad_max).y;
            bars_rect_combined.Add(test_bars_rect);

            // Expand label_rect on vertical axis to make it envelop height of plotted bars.
            label_rect_hover.Min.y = ImMin(label_rect_hover.Min.y, test_bars_rect.Min.y);
            label_rect_hover.Max.y = ImMax(label_rect_hover.Max.y, test_bars_rect.Max.y);

            // Mouse is hovering label or bars of a perf test - highlight them in info table.
            if (_PlotHoverTest < 0 && test_bars_rect.Min.y <= io.MousePos.y && io.MousePos.y < test_bars_rect.Max.y && io.MousePos.x > label_rect_hover.Max.x)
            {
                // _VisibleLabelPointers is inverted to make perf test order match info table order. Revert it back.
                _PlotHoverTest = _VisibleLabelPointers.Size - i - 1;
                _PlotHoverBatch = batch_index;

                ImPlot::GetPlotDrawList()->AddRectFilled(
                    test_bars_rect.Min, test_bars_rect.Max, ImColor(style.Colors[ImGuiCol_TableRowBgAlt]));
            }

            // Mouse is hovering a row in info table - highlight relevant bars on the plot.
            if (_TableHoveredBatch == batch_index && _TableHoveredTest == i)
                ImPlot::GetPlotDrawList()->AddRectFilled(
                    test_bars_rect.Min, test_bars_rect.Max, ImColor(style.Colors[ImGuiCol_TableRowBgAlt]));
        }

        // Mouse hovers perf label.
        if (label_rect_hover.Contains(io.MousePos))
        {
            // Render underline signaling it is clickable. Clicks are handled when rendering info table.
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImPlot::GetPlotDrawList()->AddLine(ImFloor(label_rect.GetBL()), ImFloor(label_rect.GetBR()),
                                               ImColor(style.Colors[ImGuiCol_Text]));

            // Highlight bars belonging to hovered label.
            ImPlot::GetPlotDrawList()->AddRectFilled(
                bars_rect_combined.Min, bars_rect_combined.Max, ImColor(style.Colors[ImGuiCol_TableRowBgAlt]));
            _PlotHoverTestLabel = true;
        }
    }

    if (io.KeyShift && _PlotHoverTest >= 0)
    {
        // Info tooltip with delta times of each batch for a hovered test.
        const char* test_name = _Labels.Data[_PlotHoverTest]->TestName;
        ImGui::BeginTooltip();
        float w = ImGui::CalcTextSize(test_name).x;
        float total_w = ImGui::GetContentRegionAvail().x;
        if (total_w > w)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (total_w - w) * 0.5f);
        ImGui::TextUnformatted(test_name);

        for (int i = 0; i < _Legend.Size; i++)
        {
            if (ImGuiPerflogEntry* hovered_entry = GetEntryByBatchIdx(i, test_name))
                ImGui::Text("%s %.3fms", label.c_str(), hovered_entry->DtDeltaMs);
            else
                ImGui::Text("%s --", label.c_str());
        }
        ImGui::EndTooltip();
    }

    ImPlot::EndPlot();
#else
    ImGui::TextUnformatted("Not enabled because ImPlot is not available (IMGUI_TEST_ENGINE_ENABLE_IMPLOT is not defined).");
#endif
}

void ImGuiPerfLog::_ShowEntriesTable()
{
    if (!ImGui::BeginTable("PerfInfo", IM_ARRAYSIZE(PerfLogColumnInfo), ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_SortTristate | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY))
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    // Test name column is not sorted because we do sorting only within perf runs of a particular tests,
    // so as far as sorting function is concerned all items in first column are identical.
    for (int i = 0; i < IM_ARRAYSIZE(PerfLogColumnInfo); i++)
    {
        ImGuiTableColumnFlags column_flags = ImGuiTableColumnFlags_None;
        if (i == 0)
            column_flags |= ImGuiTableColumnFlags_NoSort;
        if (!PerfLogColumnInfo[i].ShowAlways && !_CombineByBuildInfo)
            column_flags |= ImGuiTableColumnFlags_Disabled;
        ImGui::TableSetupColumn(PerfLogColumnInfo[i].Title, column_flags);
    }
    ImGui::TableSetupScrollFreeze(0, 1);

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
                _InfoTableSortSpecs = sorts_specs;
                PerfLogInstance = this;
                ImQsort(&_InfoTableSort[0], (size_t)_InfoTableSort.Size, sizeof(_InfoTableSort[0]), CompareWithSortSpecs);
                _InfoTableSortSpecs = NULL;
                PerfLogInstance = NULL;
            }
        }

    ImGui::TableHeadersRow();

    _TableHoveredTest = -1;
    _TableHoveredBatch = -1;
    for (int label_index = 0; label_index < _Labels.Size; label_index++)
    {
        const char* test_name = _Labels.Data[label_index]->TestName;
        for (int batch_index = 0; batch_index < _Legend.Size; batch_index++)
        {
            ImGuiPerflogEntry* entry = GetEntryByBatchIdx(_InfoTableSort[batch_index], test_name);
            if (entry == NULL || !_IsVisibleBuild(entry) || !_IsVisibleTest(entry))
                continue;

            ImGui::PushID(entry);
            ImGui::TableNextRow();
            if (label_index & 1)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_TableRowBgAlt, 0.5f));
            else
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_TableRowBg, 0.5f));

            if (_PlotHoverTest == label_index)
            {
                // Highlight a row that corresponds to hovered bar, or all rows that correspond to hovered perf test label.
                if (_PlotHoverBatch == batch_index || _PlotHoverTestLabel)
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImColor(style.Colors[ImGuiCol_TableRowBgAlt]));
                if (_PlotHoverTestLabel && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    ImGui::SetScrollHereY(0);
                    _PlotHoverTestLabel = false;
                }
            }

            ImGuiPerflogEntry* baseline_entry = GetEntryByBatchIdx(_BaselineBatchIndex, test_name);

            // Build info
            if (ImGui::TableNextColumn())
            {
                // ImGuiSelectableFlags_Disabled + changing ImGuiCol_TextDisabled color prevents selectable from overriding table highlight behavior.
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, style.Colors[ImGuiCol_Text]);
                ImGui::Selectable(entry->TestName, false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_Disabled);
                ImGui::PopStyleColor();

                if (ImGui::BeginPopupContextItem())
                {
                    if (entry == baseline_entry)
                        ImGui::BeginDisabled();
                    if (ImGui::MenuItem("Set as baseline"))
                        _BaselineBatchIndex = batch_index;
                    if (entry == baseline_entry)
                        ImGui::EndDisabled();
                    ImGui::EndPopup();
                }
            }
            if (ImGui::TableNextColumn())
                ImGui::TextUnformatted(entry->GitBranchName);
            if (ImGui::TableNextColumn())
                ImGui::TextUnformatted(entry->Compiler);
            if (ImGui::TableNextColumn())
                ImGui::TextUnformatted(entry->OS);
            if (ImGui::TableNextColumn())
                ImGui::TextUnformatted(entry->Cpu);
            if (ImGui::TableNextColumn())
                ImGui::TextUnformatted(entry->BuildType);
            if (ImGui::TableNextColumn())
                ImGui::Text("x%d", entry->PerfStressAmount);

            // Avg ms
            if (ImGui::TableNextColumn())
                ImGui::Text("%.3lf", entry->DtDeltaMs);

            // Min ms
            if (ImGui::TableNextColumn())
                ImGui::Text("%.3lf", entry->DtDeltaMsMin);

            // Max ms
            if (ImGui::TableNextColumn())
                ImGui::Text("%.3lf", entry->DtDeltaMsMax);

                // Num samples
            if (ImGui::TableNextColumn())
                ImGui::Text("%d", entry->NumSamples);

            // VS Baseline
            if (ImGui::TableNextColumn())
            {
                Str30f label("");
                ImGuiPerflogEntry* baseline_entry = GetEntryByBatchIdx(_BaselineBatchIndex, test_name);
                float dt_change = FormatVsBaseline(entry, baseline_entry, label);
                ImGui::TextUnformatted(label.c_str());
                if (dt_change != entry->VsBaseline)
                {
                    entry->VsBaseline = dt_change;
                    _InfoTableSortDirty = true;             // Force re-sorting.
                }
            }

            // FIXME: Aim to remove that direct access to table.... could use a selectable on first column?
            ImGuiTable* table = ImGui::GetCurrentTable();
            if (table->InnerClipRect.Contains(io.MousePos))
                if (table->RowPosY1 < io.MousePos.y && io.MousePos.y < table->RowPosY2 - 1) // FIXME: RowPosY1/RowPosY2 may overlap between adjacent rows. Compensate for that.
                {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Header));
                    for (int i = 0; i < _VisibleLabelPointers.Size && _TableHoveredTest == -1; i++)
                        if (strcmp(_VisibleLabelPointers.Data[i], test_name) == 0)
                            _TableHoveredTest = i;
                    _TableHoveredBatch = batch_index;
                }

            ImGui::PopID();
        }
    }
    ImGui::EndTable();
}

void ImGuiPerfLog::ViewOnly(const char** perf_names)
{
    // Data would not be built if we tried to view perflog of a particular test without first opening perflog via button. We need data to be built to hide perf tests.
    if (_FilteredData.empty())
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
    if (perf_name == NULL || _FilteredData.empty())
        return first;
    ImGuiPerflogEntry* last = &_FilteredData.back();    // Entries themselves are stored in _Data vector.

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
    // Estimate paddings for legend format so it looks nice and aligned
    // FIXME: Rely on font being monospace. May need to recalculate every frame on a per-need basis based on font?
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

bool ImGuiPerfLog::SaveReport(const char* file_name, const char* image_file)
{
    FILE* fp = fopen(file_name, "w+");
    if (fp == NULL)
        return false;

    fprintf(fp, "<!doctype html>\n"
                "<html>\n"
                "<head>\n"
                "  <meta charset=\"utf-8\"/>\n"
                "  <title>Dear ImGui perf report</title>\n"
                "</head>\n"
                "<body>\n"
                "  <pre id=\"content\">\n");

    // Embed performance chart.
    fprintf(fp, "## Dear ImGui perf report\n\n");

    if (image_file != NULL)
    {
        FILE* fp_img = fopen(image_file, "rb");
        if (fp_img != NULL)
        {
            ImVector<char> image_buffer;
            ImVector<char> base64_buffer;
            fseek(fp_img, 0, SEEK_END);
            image_buffer.resize((int)ftell(fp_img));
            base64_buffer.resize(((image_buffer.Size / 3) + 1) * 4 + 1);
            rewind(fp_img);
            fread(image_buffer.Data, 1, image_buffer.Size, fp_img);
            fclose(fp_img);
            int len = ImBase64Encode((unsigned char*)image_buffer.Data, base64_buffer.Data, image_buffer.Size);
            base64_buffer.Data[len] = 0;
            fprintf(fp, "![](data:image/png;base64,%s)\n\n", base64_buffer.Data);
        }
    }

    // Print info table.
    for (const auto& column_info : PerfLogColumnInfo)
        if (column_info.ShowAlways || _CombineByBuildInfo)
            fprintf(fp, "| %s ", column_info.Title);
    fprintf(fp, "|\n");
    for (const auto& column_info : PerfLogColumnInfo)
        if (column_info.ShowAlways || _CombineByBuildInfo)
            fprintf(fp, "| -- ");
    fprintf(fp, "|\n");

    for (int label_index = 0; label_index < _Labels.Size; label_index++)
    {
        const char* test_name = _Labels.Data[label_index]->TestName;
        for (int batch_index = 0; batch_index < _Legend.Size; batch_index++)
        {
            ImGuiPerflogEntry* entry = GetEntryByBatchIdx(_InfoTableSort[batch_index], test_name);
            if (entry == NULL || !_IsVisibleBuild(entry))
                continue;

            ImGuiPerflogEntry* baseline_entry = GetEntryByBatchIdx(_BaselineBatchIndex, test_name);
            for (int i = 0; i < IM_ARRAYSIZE(PerfLogColumnInfo); i++)
            {
                Str30f label("");
                const ImGuiPerfLogColumnInfo& column_info = PerfLogColumnInfo[i];
                if (column_info.ShowAlways || _CombineByBuildInfo)
                {
                    switch (i)
                    {
                    case 0:  fprintf(fp, "| %s ", entry->TestName);             break;
                    case 1:  fprintf(fp, "| %s ", entry->GitBranchName);        break;
                    case 2:  fprintf(fp, "| %s ", entry->Compiler);             break;
                    case 3:  fprintf(fp, "| %s ", entry->OS);                   break;
                    case 4:  fprintf(fp, "| %s ", entry->Cpu);                  break;
                    case 5:  fprintf(fp, "| %s ", entry->BuildType);            break;
                    case 6:  fprintf(fp, "| x%d ", entry->PerfStressAmount);    break;
                    case 7:  fprintf(fp, "| %.2f ", entry->DtDeltaMs);          break;
                    case 8:  fprintf(fp, "| %.2f ", entry->DtDeltaMsMin);       break;
                    case 9:  fprintf(fp, "| %.2f ", entry->DtDeltaMsMax);       break;
                    case 10: fprintf(fp, "| %d ", entry->NumSamples);           break;
                    case 11: FormatVsBaseline(entry, baseline_entry, label); fprintf(fp, "| %s ", label.c_str()); break;
                    default: IM_ASSERT(0); break;
                    }
                }
            }
            fprintf(fp, "|\n");
        }
    }

    fprintf(fp, "</pre>\n"
                "  <script src=\"https://cdn.jsdelivr.net/npm/marked/marked.min.js\"></script>\n"
                "  <script>\n"
                "    var content = document.getElementById('content');\n"
                "    content.innerHTML = marked(content.innerText);\n"
                "  </script>\n"
                "</body>\n"
                "</html>\n");

    fclose(fp);
    return true;
}

static bool SetPerfLogWindowOpen(ImGuiTestContext* ctx, bool is_open)
{
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ctx->WindowFocus("/Dear ImGui Test Engine");
    ctx->ItemClick("/Dear ImGui Test Engine/ TOOLS ");
    Str30f perf_tool_checkbox("/%s/Perf Tool", g.NavWindow->Name);
    if (ImGuiTestItemInfo* checkbox_info = ctx->ItemInfo(perf_tool_checkbox.c_str()))
    {
        bool is_checked = (checkbox_info->StatusFlags & ImGuiItemStatusFlags_Checked) != 0;
        if (is_checked != is_open)
            ctx->ItemClick(perf_tool_checkbox.c_str());
        return is_checked;
    }
    return false;
}

void RegisterTests_PerfLog(ImGuiTestEngine* e)
{
    ImGuiTest* t = NULL;

    // ## Flex perf tool code.
    t = IM_REGISTER_TEST(e, "misc", "misc_cov_perf_tool");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        IM_UNUSED(ctx);
        ImGui::Begin("Test Func", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        int loop_count = 1000;
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
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiPerfLog* perflog = ImGuiTestEngine_GetPerfTool(ctx->Engine);
        const char* temp_perf_csv = "output/misc_cov_perf_tool.csv";

        Str16f min_date_bkp = perflog->_FilterDateFrom;
        Str16f max_date_bkp = perflog->_FilterDateTo;

        // Execute few perf tests, serialize them to temporary csv file.
        ctx->PerfCapture("perf", "misc_cov_perf_tool_1", temp_perf_csv);
        ctx->PerfCapture("perf", "misc_cov_perf_tool_2", temp_perf_csv);

        // Load perf data from csv file and open perf tool.
        perflog->Clear();
        perflog->LoadCSV(temp_perf_csv);
        bool perf_was_open = SetPerfLogWindowOpen(ctx, true);
        ctx->Yield();

        ctx->SetRef("Dear ImGui Perf Tool");
        ctx->WindowMove("", ImVec2(50, 50));
        ctx->WindowResize("", ImVec2(1400, 900));
#ifdef IMGUI_TEST_ENGINE_ENABLE_IMPLOT
        ImGuiWindow* plot_child = ctx->GetWindowByRef(ctx->GetChildWindowID(ctx->GetChildWindowID(ctx->GetID("plot")), "Perflog"));
        IM_CHECK_NO_RET(plot_child != NULL);

        // Move legend to right side.
        ctx->MouseMoveToPos(plot_child->Rect().GetCenter());
        ctx->MouseDoubleClick(ImGuiMouseButton_Left);               // Auto-size plots while at it
        ctx->MouseClick(ImGuiMouseButton_Right);
        ctx->MenuClick("Settings/Legend/##NE");

        // Click some stuff for more coverage.
        ctx->MouseMoveToPos(plot_child->Rect().GetCenter());
        ctx->KeyPressMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
#endif
        ctx->ItemClick("##date-from", ImGuiMouseButton_Right);
        ctx->ItemClick(ctx->GetID("Set Min", g.NavWindow->ID));
        ctx->ItemClick("##date-to", ImGuiMouseButton_Right);
        ctx->ItemClick(ctx->GetID("Set Max", g.NavWindow->ID));
        ctx->ItemClick("###Filter builds");
        ctx->ItemClick("###Filter tests");
        ctx->ItemClick("Combine by build info");                    // Toggle twice to leave state unchanged
        ctx->ItemClick("Combine by build info");
        ctx->ItemClick("Per branch colors");
        ctx->ItemClick("Per branch colors");

        // Restore original state.
        perflog->Clear();                                           // Clear test data and load original data
        ImFileDelete(temp_perf_csv);
        perflog->LoadCSV();
        ctx->Yield();
#ifdef IMGUI_TEST_ENGINE_ENABLE_IMPLOT
        ctx->MouseMoveToPos(plot_child->Rect().GetCenter());
        ctx->MouseDoubleClick(ImGuiMouseButton_Left);               // Fit plot to original data
#endif
        ImStrncpy(perflog->_FilterDateFrom, min_date_bkp.c_str(), IM_ARRAYSIZE(perflog->_FilterDateFrom));
        ImStrncpy(perflog->_FilterDateTo, max_date_bkp.c_str(), IM_ARRAYSIZE(perflog->_FilterDateTo));
        SetPerfLogWindowOpen(ctx, perf_was_open);                   // Restore window visibility
    };
}
