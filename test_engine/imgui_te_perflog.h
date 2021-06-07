#pragma once

#include <Str/Str.h>
#include "imgui.h"

struct ImGuiCSVParser;
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
    //const char*                 DateMax = NULL;               // Max date of combined entries, or NULL.
    double                      VsBaseline = 0.0;               // Percent difference vs baseline.
    int                         BranchIndex = 0;                // Unique linear branch index.
};

struct ImGuiPerfLogColumnInfo;

struct ImGuiPerfLog
{
    ImGuiCSVParser*             _CSVParser;                     // CSV parser.
    ImVector<ImGuiPerflogEntry> _CSVData;                       // Raw entries from CSV file (with string pointer into CSV data).
    ImVector<ImGuiPerflogEntry> _Data;                          // Data used to render plots. This is not necessarily same as Entries. Sorted by Timestamp and TestName.
    ImVector<ImGuiPerflogEntry*> _Labels;                       // A list of labels (left of the plot).
    ImVector<ImGuiPerflogEntry*> _Legend;                       // _Legend[idx] = first batch element.
    ImVector<const char*>       _VisibleLabelPointers;          // ImPlot requires a pointer of all labels beforehand.
    bool                        _CombineByBuildInfo = true;     // Entries with same build information will be averaged.
    bool                        _PerBranchColors = true;        // Use one bar color per branch.
    int                         _BaselineBatchIndex = 0;        // Index of baseline build.
    Str64                       _Filter;                        // Context menu filtering substring.
    char                        _FilterDateFrom[11] = {};
    char                        _FilterDateTo[11] = {};
    float                       _InfoTableHeight = 180.0f;
    bool                        _Dirty = true;                  // Flag indicating that data rebuild is necessary. It is only set when new perf data is added by running tests or "Combine by build info" option is toggled.
    int                         _AlignStress = 0;               // Alignment values for build info components, so they look aligned in the legend.
    int                         _AlignType = 0;
    int                         _AlignOs = 0;
    int                         _AlignCompiler = 0;
    int                         _AlignBranch = 0;
    int                         _AlignSamples = 0;
    bool                        _InfoTableSortDirty = false;
    ImVector<int>               _InfoTableSort;                 // _InfoTableSort[_Legend.Size]. Contains indices into _Legend vector.
    const ImGuiPerfLogColumnInfo*_InfoTableSortColInfo = NULL;  // Current Table column information.
    const ImGuiTableSortSpecs*  _InfoTableSortSpecs = NULL;     // Current table sort specs.
    ImGuiStorage                _TempSet;                       // Used as a set
    int                         _TableHoveredTest = -1;
    int                         _TableHoveredBatch = -1;

    struct
    {
        ImGuiStorage            Visibility;
        ImU64                   BaselineTimestamp;

        void    Clear()         { Visibility.Clear(); BaselineTimestamp = (ImU64)-1; }
    } _Settings;

    ImGuiPerfLog();
    ~ImGuiPerfLog();
    bool        Load(const char* file_name);
    bool        Save(const char* file_name);
    void        AddEntry(ImGuiPerflogEntry* entry);
    void        Clear();
    void        ShowUI(ImGuiTestEngine* engine);
    void        ViewOnly(const char* perf_name);
    void        ViewOnly(const char** perf_names);
    ImGuiPerflogEntry* GetEntryByBatchIdx(int idx, const char* perf_name=NULL);

    void        _Rebuild();
    bool        _IsVisibleBuild(ImGuiPerflogEntry* entry);
    bool        _IsVisibleTest(ImGuiPerflogEntry* entry);
    void        _CalculateLegendAlignment();
    void        _ClosePopupMaybe();
};
