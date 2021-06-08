#pragma once

#include "imgui_te_coroutine.h"
#include "imgui_te_engine.h"
#include "shared/imgui_capture_tool.h"  // ImGuiCaptureTool  // FIXME

//-------------------------------------------------------------------------
// FORWARD DECLARATIONS
//-------------------------------------------------------------------------

struct ImGuiPerfLog;
struct ImGuiPerflogEntry;

//-------------------------------------------------------------------------
// DATA STRUCTURES
//-------------------------------------------------------------------------

// Query item position/window/state given ID.
struct ImGuiTestInfoTask
{
    // Input
    ImGuiID                 ID = 0;
    int                     FrameCount = -1;        // Timestamp of request
    char                    DebugName[64] = "";     // Debug string representing the queried ID

    // Output
    ImGuiTestItemInfo       Result;
};

// Gather item list in given parent ID.
struct ImGuiTestGatherTask
{
    // Input
    ImGuiID                 InParentID = 0;
    int                     InDepth = 0;

    // Output/Temp
    ImGuiTestItemList*      OutList = NULL;
    ImGuiTestItemInfo*      LastItemInfo = NULL;
};

// Find item ID given a label and a parent id
struct ImGuiTestFindByLabelTask
{
    // Input
    ImGuiID                 InBaseId = 0;                   // A known base ID which appears before wildcard ID(s)
    const char*             InLabel = NULL;                 // A label string which appears on ID stack after unknown ID(s)
    ImGuiItemStatusFlags    InFilterItemStatusFlags = 0;    // Flags required for item to be returned

    // Output
    ImGuiID                 OutItemId = 0;                  // Result item ID
};

// Processed by test queue
struct ImGuiTestRunTask
{
    ImGuiTest*              Test = NULL;
    ImGuiTestRunFlags       RunFlags = ImGuiTestRunFlags_None;
};

struct ImGuiStackLevelInfo
{
    ImGuiID                 ID = 0;
    bool                    QueryStarted = false;
    bool                    QuerySuccess = false;       // Obtained infos from PushID() hook
    char                    Desc[59] = "";
};

struct ImGuiStackTool
{
    ImGuiID                 QueryStackId = 0;           // Stack id to query details for
    int                     QueryStep = -1;
    ImGuiStackLevelInfo*    QueryIdInfoOutput = NULL;   // Current stack level we're hooking PushID for
    int                     QueryIdInfoTimestamp = -1;
    ImVector<ImGuiStackLevelInfo> Results;

    void    ShowStackToolWindow(ImGuiTestEngine* engine, bool* p_open = NULL);
    void    UpdateQueries(ImGuiTestEngine* engine);
};

struct ImGuiTestInputs
{
    ImGuiIO                     SimulatedIO;
    int                         ApplyingSimulatedIO = 0;
    ImVec2                      MousePosValue;                  // Own non-rounded copy of MousePos in order facilitate simulating mouse movement very slow speed and high-framerate
    ImVec2                      MouseWheel;
    ImVec2                      HostLastMousePos;
    int                         MouseButtonsValue = 0x00;       // FIXME-TESTS: Use simulated_io.MouseDown[] ?
    ImGuiKeyModFlags            KeyMods = 0x00;                 // FIXME-TESTS: Use simulated_io.KeyXXX ?
    ImVector<ImGuiTestInput>    Queue;
};

// [Internal] Test Engine Context
struct ImGuiTestEngine
{
    ImGuiTestEngineIO           IO;
    ImGuiContext*               UiContextVisible = NULL;        // imgui context for visible/interactive needs
    ImGuiContext*               UiContextBlind = NULL;          // FIXME: Unsupported
    ImGuiContext*               UiContextTarget = NULL;         // imgui context for testing == io.ConfigRunBlind ? UiBlindContext : UiVisibleContext when running tests, otherwise NULL.
    ImGuiContext*               UiContextActive = NULL;         // imgui context for testing == UiContextTarget or NULL

    bool                        Started = false;
    int                         FrameCount = 0;
    float                       OverrideDeltaTime = -1.0f;      // Inject custom delta time into imgui context to simulate clock passing faster than wall clock time.
    ImVector<ImGuiTest*>        TestsAll;
    ImVector<ImGuiTestRunTask>  TestsQueue;
    ImGuiTestContext*           TestContext = NULL;
    ImVector<ImGuiTestInfoTask*>InfoTasks;
    ImGuiTestGatherTask         GatherTask;
    ImGuiTestFindByLabelTask    FindByLabelTask;
    void*                       UserDataBuffer = NULL;
    size_t                      UserDataBufferSize = 0;
    ImGuiTestCoroutineHandle    TestQueueCoroutine = NULL;      // Coroutine to run the test queue
    bool                        TestQueueCoroutineShouldExit = false; // Flag to indicate that we are shutting down and the test queue coroutine should stop

    // Inputs
    ImGuiTestInputs             Inputs;

    // UI support
    bool                        Abort = false;
    bool                        UiFocus = false;
    ImGuiTest*                  UiSelectAndScrollToTest = NULL;
    ImGuiTest*                  UiSelectedTest = NULL;
    ImGuiTextFilter             UiFilterTests;
    ImGuiTextFilter             UiFilterPerfs;
    bool                        UiFilterFailingOnly = false;
    bool                        UiMetricsOpen = false; // FIXME
    bool                        UiCaptureToolOpen = false;
    bool                        UiStackToolOpen = false;
    bool                        UiPerfToolOpen = false;
    float                       UiLogHeight = 150.0f;

    // Performance Monitor
    double                      PerfRefDeltaTime;
    ImMovingAverage<double>     PerfDeltaTime100;
    ImMovingAverage<double>     PerfDeltaTime500;
    ImMovingAverage<double>     PerfDeltaTime1000;
    ImMovingAverage<double>     PerfDeltaTime2000;
    ImGuiPerfLog*               PerfLog = NULL;

    // Tools
    bool                        ToolDebugRebootUiContext = false;   // Completely shutdown and recreate the dear imgui context in place
    bool                        ToolSlowDown = false;
    int                         ToolSlowDownMs = 100;
    ImGuiStackTool              StackTool;
    ImGuiCaptureTool            CaptureTool;
    ImGuiCaptureContext         CaptureContext;
    ImGuiCaptureArgs*           CurrentCaptureArgs = NULL;
    bool                        BackupConfigRunFast = false;
    bool                        BackupConfigNoThrottle = false;

    // Functions
    ImGuiTestEngine();
    ~ImGuiTestEngine();
};

//-------------------------------------------------------------------------
// FUNCTIONS
//-------------------------------------------------------------------------

ImGuiTestItemInfo*  ImGuiTestEngine_FindItemInfo(ImGuiTestEngine* engine, ImGuiID id, const char* debug_id);
void                ImGuiTestEngine_PushInput(ImGuiTestEngine* engine, const ImGuiTestInput& input);
void                ImGuiTestEngine_Yield(ImGuiTestEngine* engine);
void                ImGuiTestEngine_SetDeltaTime(ImGuiTestEngine* engine, float delta_time);
int                 ImGuiTestEngine_GetFrameCount(ImGuiTestEngine* engine);
double              ImGuiTestEngine_GetPerfDeltaTime500Average(ImGuiTestEngine* engine);
const char*         ImGuiTestEngine_GetVerboseLevelName(ImGuiTestVerboseLevel v);
bool                ImGuiTestEngine_CaptureScreenshot(ImGuiTestEngine* engine, ImGuiCaptureArgs* args);
bool                ImGuiTestEngine_BeginCaptureAnimation(ImGuiTestEngine* engine, ImGuiCaptureArgs* args);
bool                ImGuiTestEngine_EndCaptureAnimation(ImGuiTestEngine* engine, ImGuiCaptureArgs* args);
bool                ImGuiTestEngine_PerflogLoad(ImGuiTestEngine* engine);
void                ImGuiTestEngine_PerflogAppend(ImGuiTestEngine* engine, ImGuiPerflogEntry* entry);                   // Append to last perflog batch, or open a new batch if BatchTitle is different.

//-------------------------------------------------------------------------
