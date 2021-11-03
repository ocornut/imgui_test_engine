#pragma once

#include "imgui.h"

#define IMGUI_PERFLOG_FILENAME  "output/imgui_perflog.csv"

struct ImGuiPerfLogColumnInfo;
struct ImGuiTestEngine;

// [Internal] Perf log entry. Changes to this struct should be reflected in ImGuiTestContext::PerfCapture() and ImGuiTestEngine_Start().
struct ImGuiPerflogEntry
{
    ImU64                       Timestamp = 0;                  // Title of a particular batch of perflog entries.
    const char*                 Category = NULL;                // Name of category perf test is in.
    const char*                 TestName = NULL;                // Name of perf test.
    double                      DtDeltaMs = 0.0;                // Result of perf test.
    double                      DtDeltaMsMin = +FLT_MAX;        // May be used by perflog.
    double                      DtDeltaMsMax = -FLT_MAX;        // May be used by perflog.
    int                         NumSamples = 1;                 // Number aggregated samples.
    int                         PerfStressAmount = 0;           //
    const char*                 GitBranchName = NULL;           // Build information.
    const char*                 BuildType = NULL;               //
    const char*                 Cpu = NULL;                     //
    const char*                 OS = NULL;                      //
    const char*                 Compiler = NULL;                //
    const char*                 Date = NULL;                    // Date of this entry or min date of combined entries.
    //const char*               DateMax = NULL;                 // Max date of combined entries, or NULL.
    double                      VsBaseline = 0.0;               // Percent difference vs baseline.
    bool                        DataOwner = false;              // Owns lifetime of pointers when set to true.
    int                         LabelIndex = 0;                 // Index of TestName in ImGuiPerfLog::_LabelsVisible.

    ~ImGuiPerflogEntry();
    void TakeDataOwnership();
};

// [Internal] Perf log batch.
struct ImGuiPerflogBatch
{
    ImU64                       BatchID = 0;                    // Timestamp of the batch, or unique ID of the build in combined mode.
    int                         NumSamples = 0;                 // A number of unique batches aggregated.
    int                         BranchIndex = 0;                // For per-branch color mapping.
    ImVector<ImGuiPerflogEntry> Entries;                        // Aggregated perf test entries. Order follows ImGuiPerfLog::_LabelsVisible order.
    ~ImGuiPerflogBatch()        { Entries.clear_destruct(); }
};

enum ImGuiPerflogDisplayType_
{
    ImGuiPerflogDisplayType_Simple,                             // Each run will be displayed individually.
    ImGuiPerflogDisplayType_PerBranchColors,                    // Use one bar color per branch.
    ImGuiPerflogDisplayType_CombineByBuildInfo,                 // Entries with same build information will be averaged.
};
typedef int ImGuiPerflogDisplayType;

struct ImGuiPerfLog
{
    ImVector<ImGuiPerflogEntry> _SrcData;                       // Raw entries from CSV file (with string pointer into CSV data).
    ImVector<const char*>       _Labels;
    ImVector<const char*>       _LabelsVisible;                 // ImPlot requires a pointer of all labels beforehand. Always contains a dummy "" entry at the end!
    ImVector<ImGuiPerflogBatch> _Batches;
    ImGuiStorage                _LabelBarCounts;                // Number bars each label will render.
    int                         _NumVisibleBuilds = 0;          // Cached number of visible builds.
    int                         _NumUniqueBuilds = 0;           // Cached number of unique builds.
    ImGuiPerflogDisplayType     _DisplayType = ImGuiPerflogDisplayType_CombineByBuildInfo;
    int                         _BaselineBatchIndex = 0;        // Index of baseline build.
    ImU64                       _BaselineTimestamp = 0;
    ImU64                       _BaselineBuildId = 0;
    char                        _Filter[128];                   // Context menu filtering substring.
    char                        _FilterDateFrom[11] = {};
    char                        _FilterDateTo[11] = {};
    float                       _InfoTableHeight = 180.0f;
    int                         _AlignStress = 0;               // Alignment values for build info components, so they look aligned in the legend.
    int                         _AlignType = 0;
    int                         _AlignOs = 0;
    int                         _AlignCompiler = 0;
    int                         _AlignBranch = 0;
    int                         _AlignSamples = 0;
    bool                        _InfoTableSortDirty = false;
    ImVector<int>               _InfoTableSort;                 // _InfoTableSort[_LabelsVisible.Size * _Batches.Size]. Contains sorted batch indices for each label.
    const ImGuiTableSortSpecs*  _InfoTableSortSpecs = NULL;     // Current table sort specs.
    int                         _InfoTableNowSortingLabelIdx = 0;// Label index that is being sorted now.
    ImGuiStorage                _TempSet;                       // Used as a set
    int                         _TableHoveredTest = -1;         // Index within _VisibleLabelPointers array.
    int                         _TableHoveredBatch = -1;
    int                         _PlotHoverTest = -1;
    int                         _PlotHoverBatch = -1;
    bool                        _PlotHoverTestLabel = false;
    bool                        _ReportGenerating = false;
    ImGuiStorage                _Visibility;

    ImGuiPerfLog();
    ~ImGuiPerfLog();

    void        Clear();
    bool        LoadCSV(const char* filename = NULL);
    void        AddEntry(ImGuiPerflogEntry* entry);

    void        ShowUI(ImGuiTestEngine* engine);
    void        ViewOnly(const char* perf_name);
    void        ViewOnly(const char** perf_names);
    ImGuiPerflogEntry* GetEntryByBatchIdx(int idx, const char* perf_name = NULL);
    bool        SaveReport(const char* file_name, const char* image_file = NULL);

    void        _Rebuild();
    bool        _IsVisibleBuild(ImGuiPerflogBatch* batch);
    bool        _IsVisibleBuild(ImGuiPerflogEntry* batch);
    bool        _IsVisibleTest(ImGuiPerflogEntry* entry);
    void        _CalculateLegendAlignment();
    void        _ShowEntriesPlot();
    void        _ShowEntriesTable();
    void        _SetBaseline(int batch_index);
};

void    ImGuiTestEngine_PerflogAppendToCSV(ImGuiPerfLog* perf_log, ImGuiPerflogEntry* entry, const char* filename = NULL);
