#pragma once

#include "imgui_te_coroutine.h"
#include "shared/imgui_capture_tool.h"  // ImGuiCaptureTool  // FIXME

//-------------------------------------------------------------------------
// DATA STRUCTURES
//-------------------------------------------------------------------------

// Gather items in given parent scope.
struct ImGuiTestGatherTask
{
    ImGuiID                 ParentID = 0;
    int                     Depth = 0;
    ImGuiTestItemList*      OutList = NULL;
    ImGuiTestItemInfo*      LastItemInfo = NULL;
};

// Locate item position/window/state given ID.
struct ImGuiTestLocateTask
{
    ImGuiID                 ID = 0;
    int                     FrameCount = -1;        // Timestamp of request
    char                    DebugName[64] = "";     // Debug string representing the queried ID
    ImGuiTestItemInfo       Result;
};

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
    bool                    Visible = false;
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
    ImVector<ImGuiTestLocateTask*>  LocateTasks;
    ImGuiTestGatherTask         GatherTask;
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
    float                       UiLogHeight = 150.0f;

    // Performance Monitor
    double                      PerfRefDeltaTime;
    ImMovingAverage<double>     PerfDeltaTime100;
    ImMovingAverage<double>     PerfDeltaTime500;
    ImMovingAverage<double>     PerfDeltaTime1000;
    ImMovingAverage<double>     PerfDeltaTime2000;

    // Tools
    bool                        ToolSlowDown = false;
    int                         ToolSlowDownMs = 100;
    ImGuiStackTool              StackTool;
    ImGuiCaptureTool            CaptureTool;
    ImGuiCaptureContext         CaptureContext;
    ImGuiCaptureArgs*           CurrentCaptureArgs = NULL;
    bool                        BackupConfigRunFast = false;
    bool                        BackupConfigNoThrottle = false;

    // Functions
    ImGuiTestEngine()
    {
        PerfRefDeltaTime = 0.0f;
        PerfDeltaTime100.Init(100);
        PerfDeltaTime500.Init(500);
        PerfDeltaTime1000.Init(1000);
        PerfDeltaTime2000.Init(2000);
    }
    ~ImGuiTestEngine()
    {
        IM_ASSERT(TestQueueCoroutine == NULL);
        if (UiContextBlind != NULL)
            ImGui::DestroyContext(UiContextBlind);
    }
};

//-------------------------------------------------------------------------
// FUNCTIONS
//-------------------------------------------------------------------------

ImGuiTestItemInfo*  ImGuiTestEngine_ItemLocate(ImGuiTestEngine* engine, ImGuiID id, const char* debug_id);
void                ImGuiTestEngine_PushInput(ImGuiTestEngine* engine, const ImGuiTestInput& input);
void                ImGuiTestEngine_Yield(ImGuiTestEngine* engine);
void                ImGuiTestEngine_SetDeltaTime(ImGuiTestEngine* engine, float delta_time);
int                 ImGuiTestEngine_GetFrameCount(ImGuiTestEngine* engine);
double              ImGuiTestEngine_GetPerfDeltaTime500Average(ImGuiTestEngine* engine);
const char*         ImGuiTestEngine_GetVerboseLevelName(ImGuiTestVerboseLevel v);
bool                ImGuiTestEngine_CaptureScreenshot(ImGuiTestEngine* engine, ImGuiCaptureArgs* args);
bool                ImGuiTestEngine_BeginCaptureAnimation(ImGuiTestEngine* engine, ImGuiCaptureArgs* args);
bool                ImGuiTestEngine_EndCaptureAnimation(ImGuiTestEngine* engine, ImGuiCaptureArgs* args);

//-------------------------------------------------------------------------
