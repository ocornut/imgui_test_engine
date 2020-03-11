#pragma once

#include "shared/imgui_capture_tool.h"  // ImGuiCaptureTool  // FIXME

#define IMGUI_DEBUG_TEST_ENGINE     1

//-------------------------------------------------------------------------
// [SECTION] DATA STRUCTURES
//-------------------------------------------------------------------------

// Locate item position/window/state given ID.
struct ImGuiTestLocateTask
{
    ImGuiID                 ID = 0;
    int                     FrameCount = -1;        // Timestamp of request
    char                    DebugName[64] = "";
    ImGuiTestItemInfo       Result;
};

struct ImGuiTestRunTask
{
    ImGuiTest*                  Test = NULL;
    ImGuiTestRunFlags           RunFlags = ImGuiTestRunFlags_None;
};

// [Internal] Test Engine Context
struct ImGuiTestEngine
{
    ImGuiTestEngineIO           IO;
    ImGuiContext*               UiContextVisible = NULL;        // imgui context for visible/interactive needs
    ImGuiContext*               UiContextBlind = NULL;          // FIXME
    ImGuiContext*               UiContextTarget = NULL;         // imgui context for testing == io.ConfigRunBlind ? UiBlindContext : UiVisibleContext when running tests, otherwise NULL.
    ImGuiContext*               UiContextActive = NULL;         // imgui context for testing == UiContextTarget or NULL

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
    ImGuiCaptureTool            CaptureTool;
    bool                        ToolSlowDown = false;
    int                         ToolSlowDownMs = 100;
    ImGuiCaptureContext         CaptureContext;
    ImGuiCaptureArgs*           CurrentCaptureArgs = NULL;

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
