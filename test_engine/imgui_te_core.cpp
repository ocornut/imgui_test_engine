// dear imgui
// (test engine, core)

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifdef _MSC_VER
#pragma warning (disable: 4100)     // unreferenced formal parameter
#endif

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_te_core.h"
#include "imgui_te_util.h"
#include "imgui_te_context.h"
#include <Str/Str.h>

#define IMGUI_DEBUG_TEST_ENGINE     1

/*

Index of this file:

// [SECTION] TODO
// [SECTION] DATA STRUCTURES
// [SECTION] TEST ENGINE: FORWARD DECLARATIONS
// [SECTION] TEST ENGINE: FUNCTIONS
// [SECTION] HOOKS FOR CORE LIBRARY
// [SECTION] HOOKS FOR TESTS
// [SECTION] USER INTERFACE
// [SECTION] SETTINGS

*/

//-------------------------------------------------------------------------
// [SECTION] TODO
//-------------------------------------------------------------------------

// GOAL: Code coverage.
// GOAL: Custom testing.
// GOAL: Take screenshots for web/docs.
// GOAL: Reliable performance measurement (w/ deterministic setup)
// GOAL: Full blind version with no graphical context.

// FIXME-TESTS: IM_CHECK macro that may print both values.
// FIXME-TESTS: threaded test func instead of intrusive yield
// FIXME-TESTS: UI to setup breakpoint (e.g. GUI func on frame X, beginning of Test func or at certain Yield/Sleep spot)
// FIXME-TESTS: Locate within stack that uses windows/<pointer>/name -> ItemInfo hook
// FIXME-TESTS: Be able to run blind within GUI
// FIXME-TESTS: Be able to interactively run GUI function without Test function
// FIXME-TESTS: Provide variants of same test (e.g. run same tests with a bunch of styles/flags, incl. configuration flags)
// FIXME-TESTS: Automate clicking/opening stuff based on gathering id?
// FIXME-TESTS: Mouse actions on ImGuiNavLayer_Menu layer
// FIXME-TESTS: Fail to open a double-click tree node
// FIXME-TESTS: Possible ID resolving variables e.g. "$REF/Main menu bar" / "$NAV/Main menu bar" / "$TOP/Main menu bar"

//-------------------------------------------------------------------------
// [SECTION] DATA STRUCTURES
//-------------------------------------------------------------------------

static ImGuiTestEngine* GImGuiHookingEngine = NULL;

// [Internal] Locate item position/window/state given ID.
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
    int                         CallDepth = 0;
    ImVector<ImGuiTestLocateTask*>  LocateTasks;
    ImGuiTestGatherTask         GatherTask;
    void*                       UserDataBuffer = NULL;
    size_t                      UserDataBufferSize = 0;

    // Inputs
    ImGuiTestInputs             Inputs;

    // UI support
    bool                        Abort = false;
    bool                        UiFocus = false;
    ImGuiTest*                  UiSelectAndScrollToTest = NULL;
    ImGuiTest*                  UiSelectedTest = NULL;
    ImGuiTextFilter             UiTestFilter;
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
        if (UiContextBlind != NULL)
            ImGui::DestroyContext(UiContextBlind);
    }
};

//-------------------------------------------------------------------------
// [SECTION] TEST ENGINE: FORWARD DECLARATIONS
//-------------------------------------------------------------------------

// Private functions
static void ImGuiTestEngine_ClearInput(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ApplyInputToImGuiContext(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ProcessTestQueue(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ClearTests(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ClearLocateTasks(ImGuiTestEngine* engine);
static void ImGuiTestEngine_PreNewFrame(ImGuiTestEngine* engine, ImGuiContext* ctx);
static void ImGuiTestEngine_PostNewFrame(ImGuiTestEngine* engine, ImGuiContext* ctx);
static void ImGuiTestEngine_RunTest(ImGuiTestEngine* engine, ImGuiTestContext* ctx, void* user_data);

// Settings
static void* ImGuiTestEngine_SettingsReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
static void  ImGuiTestEngine_SettingsReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line);
static void  ImGuiTestEngine_SettingsWriteAll(ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);

//-------------------------------------------------------------------------
// [SECTION] TEST ENGINE: FUNCTIONS
//-------------------------------------------------------------------------
// Public
// - ImGuiTestEngine_CreateContext()
// - ImGuiTestEngine_ShutdownContext()
// - ImGuiTestEngine_GetIO()
// - ImGuiTestEngine_Abort()
// - ImGuiTestEngine_QueueAllTests()
//-------------------------------------------------------------------------
// - ImGuiTestEngine_FindLocateTask()
// - ImGuiTestEngine_ItemLocate()
// - ImGuiTestEngine_ClearTests()
// - ImGuiTestEngine_ClearLocateTasks()
// - ImGuiTestEngine_ApplyInputToImGuiContext()
// - ImGuiTestEngine_PreNewFrame()
// - ImGuiTestEngine_PostNewFrame()
// - ImGuiTestEngine_Yield()
// - ImGuiTestEngine_ProcessTestQueue()
// - ImGuiTestEngine_QueueTest()
// - ImGuiTestEngine_RunTest()
//-------------------------------------------------------------------------

// Create test context and attach to imgui context
ImGuiTestEngine*    ImGuiTestEngine_CreateContext(ImGuiContext* imgui_context)
{
    IM_ASSERT(imgui_context != NULL);
    ImGuiTestEngine* engine = IM_NEW(ImGuiTestEngine)();
    engine->UiContextVisible = imgui_context;
    engine->UiContextBlind = NULL;
    engine->UiContextTarget = engine->UiContextVisible;
    engine->UiContextActive = NULL;

    // Setup hook
    if (GImGuiHookingEngine == NULL)
        GImGuiHookingEngine = engine;

    // Add .ini handle for ImGuiWindow type
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "TestEngine";
    ini_handler.TypeHash = ImHashStr("TestEngine");
    ini_handler.ReadOpenFn = ImGuiTestEngine_SettingsReadOpen;
    ini_handler.ReadLineFn = ImGuiTestEngine_SettingsReadLine;
    ini_handler.WriteAllFn = ImGuiTestEngine_SettingsWriteAll;
    imgui_context->SettingsHandlers.push_back(ini_handler);

    return engine;
}

void    ImGuiTestEngine_ShutdownContext(ImGuiTestEngine* engine)
{
    engine->UiContextVisible = engine->UiContextBlind = engine->UiContextTarget = engine->UiContextActive = NULL;

    IM_FREE(engine->UserDataBuffer);
    engine->UserDataBuffer = NULL;
    engine->UserDataBufferSize = 0;

    IM_ASSERT(engine->CallDepth == 0);
    ImGuiTestEngine_ClearTests(engine);
    ImGuiTestEngine_ClearLocateTasks(engine);
    IM_DELETE(engine);

    // Release hook
    if (GImGuiHookingEngine == engine)
        GImGuiHookingEngine = NULL;
}

ImGuiTestEngineIO&  ImGuiTestEngine_GetIO(ImGuiTestEngine* engine)
{
    return engine->IO;
}

void    ImGuiTestEngine_Abort(ImGuiTestEngine* engine)
{
    engine->Abort = true;
    if (ImGuiTestContext* test_context = engine->TestContext)
        test_context->Abort = true;
}

// FIXME-OPT
ImGuiTestLocateTask* ImGuiTestEngine_FindLocateTask(ImGuiTestEngine* engine, ImGuiID id)
{
    for (int task_n = 0; task_n < engine->LocateTasks.Size; task_n++)
    {
        ImGuiTestLocateTask* task = engine->LocateTasks[task_n];
        if (task->ID == id)
            return task;
    }
    return NULL;
}

// Request information about one item.
// Will push a request for the test engine to process.
// Will return NULL when results are not ready (or not available).
ImGuiTestItemInfo* ImGuiTestEngine_ItemLocate(ImGuiTestEngine* engine, ImGuiID id, const char* debug_id)
{
    IM_ASSERT(id != 0);

    if (ImGuiTestLocateTask* task = ImGuiTestEngine_FindLocateTask(engine, id))
    {
        if (task->Result.TimestampMain + 2 >= engine->FrameCount)
        {
            task->FrameCount = engine->FrameCount; // Renew task
            return &task->Result;
        }
        return NULL;
    }

    // Create task
    ImGuiTestLocateTask* task = IM_NEW(ImGuiTestLocateTask)();
    task->ID = id;
    task->FrameCount = engine->FrameCount;
#if IMGUI_DEBUG_TEST_ENGINE
    if (debug_id)
    {
        size_t debug_id_sz = strlen(debug_id);
        if (debug_id_sz < IM_ARRAYSIZE(task->DebugName) - 1)
        {
            memcpy(task->DebugName, debug_id, debug_id_sz + 1);
        }
        else
        {
            size_t header_sz = (size_t)(IM_ARRAYSIZE(task->DebugName) * 0.30f);
            size_t footer_sz = IM_ARRAYSIZE(task->DebugName) - 2 - header_sz;
            IM_ASSERT(header_sz > 0 && footer_sz > 0);
            ImFormatString(task->DebugName, IM_ARRAYSIZE(task->DebugName), "%.*s..%.*s", (int)header_sz, debug_id, (int)footer_sz, debug_id + debug_id_sz - footer_sz);
        }
    }
#endif
    engine->LocateTasks.push_back(task);

    return NULL;
}

static void ImGuiTestEngine_ClearTests(ImGuiTestEngine* engine)
{
    for (int n = 0; n < engine->TestsAll.Size; n++)
        IM_DELETE(engine->TestsAll[n]);
    engine->TestsAll.clear();
    engine->TestsQueue.clear();
}

static void ImGuiTestEngine_ClearLocateTasks(ImGuiTestEngine* engine)
{
    for (int n = 0; n < engine->LocateTasks.Size; n++)
        IM_DELETE(engine->LocateTasks[n]);
    engine->LocateTasks.clear();
}

void ImGuiTestEngine_ClearInput(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->UiContextTarget != NULL);
    engine->Inputs.MouseButtonsValue = 0;
    engine->Inputs.KeyMods = ImGuiKeyModFlags_None;
    engine->Inputs.Queue.clear();

    ImGuiIO& simulated_io = engine->Inputs.SimulatedIO;
    simulated_io.KeyCtrl = simulated_io.KeyShift = simulated_io.KeyAlt = simulated_io.KeySuper = false;
    memset(simulated_io.MouseDown, 0, sizeof(simulated_io.MouseDown));
    memset(simulated_io.KeysDown, 0, sizeof(simulated_io.KeysDown));
    memset(simulated_io.NavInputs, 0, sizeof(simulated_io.NavInputs));
    simulated_io.ClearInputCharacters();
    ImGuiTestEngine_ApplyInputToImGuiContext(engine);
}

void ImGuiTestEngine_PushInput(ImGuiTestEngine* engine, const ImGuiTestInput& input)
{
    engine->Inputs.Queue.push_back(input);
}

void ImGuiTestEngine_ApplyInputToImGuiContext(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->UiContextTarget != NULL);
    ImGuiContext& g = *engine->UiContextTarget;

    ImGuiIO& main_io = g.IO;
    ImGuiIO& simulated_io = engine->Inputs.SimulatedIO;

    main_io.MouseDrawCursor = true;

    const bool want_simulated_inputs = engine->UiContextActive != NULL && ImGuiTestEngine_IsRunningTests(engine) && !(engine->TestContext->RunFlags & ImGuiTestRunFlags_NoTestFunc);
    if (want_simulated_inputs)
    {
        IM_ASSERT(engine->TestContext != NULL);

        // Clear host IO queues (because we can't easily just memcpy the vectors)
        if (engine->Inputs.ApplyingSimulatedIO == 0)
            simulated_io.MousePos = main_io.MousePos;

        engine->Inputs.ApplyingSimulatedIO = 2;
        main_io.ClearInputCharacters();

        // Process input requests/queues
        if (engine->Inputs.Queue.Size > 0)
        {
            for (int n = 0; n < engine->Inputs.Queue.Size; n++)
            {
                const ImGuiTestInput& input = engine->Inputs.Queue[n];
                switch (input.Type)
                {
                case ImGuiTestInputType_Key:
                {
                    if (input.State == ImGuiKeyState_Down)
                        engine->Inputs.KeyMods |= input.KeyMods;
                    else
                        engine->Inputs.KeyMods &= ~input.KeyMods;

                    if (input.Key != ImGuiKey_COUNT)
                    {
                        int idx = main_io.KeyMap[input.Key];
                        if (idx >= 0 && idx < IM_ARRAYSIZE(simulated_io.KeysDown))
                            simulated_io.KeysDown[idx] = (input.State == ImGuiKeyState_Down);
                    }
                    break;
                }
                case ImGuiTestInputType_Nav:
                {
                    IM_ASSERT(input.NavInput >= 0 && input.NavInput < ImGuiNavInput_COUNT);
                    simulated_io.NavInputs[input.NavInput] = (input.State == ImGuiKeyState_Down);
                    break;
                }
                case ImGuiTestInputType_Char:
                {
                    IM_ASSERT(input.Char != 0);
                    main_io.AddInputCharacter(input.Char);
                    break;
                }
                case ImGuiTestInputType_None:
                default:
                    break;
                }
            }
            engine->Inputs.Queue.resize(0);
        }

        // Apply mouse position
        simulated_io.MousePos = engine->Inputs.MousePosValue;
        //main_io.WantSetMousePos = true;
        for (int n = 0; n < IM_ARRAYSIZE(simulated_io.MouseDown); n++)
            simulated_io.MouseDown[n] = (engine->Inputs.MouseButtonsValue & (1 << n)) != 0;

        // Apply keyboard mods
        simulated_io.KeyCtrl  = (engine->Inputs.KeyMods & ImGuiKeyModFlags_Ctrl) != 0;
        simulated_io.KeyAlt   = (engine->Inputs.KeyMods & ImGuiKeyModFlags_Alt) != 0;
        simulated_io.KeyShift = (engine->Inputs.KeyMods & ImGuiKeyModFlags_Shift) != 0;
        simulated_io.KeySuper = (engine->Inputs.KeyMods & ImGuiKeyModFlags_Super) != 0;

        // Apply to real IO
        #define COPY_FIELD(_FIELD)  { memcpy(&main_io._FIELD, &simulated_io._FIELD, sizeof(simulated_io._FIELD)); }
        COPY_FIELD(MousePos);
        COPY_FIELD(MouseDown);
        COPY_FIELD(MouseWheel);
        COPY_FIELD(MouseWheelH);
        COPY_FIELD(KeyCtrl);
        COPY_FIELD(KeyShift);
        COPY_FIELD(KeyAlt);
        COPY_FIELD(KeySuper);
        COPY_FIELD(KeysDown);
        COPY_FIELD(NavInputs);
        #undef COPY_FIELD

        // FIXME-TESTS: This is a bit of a mess, ideally we should be able to swap/copy/isolate IO without all that fuss..
        memset(simulated_io.NavInputs, 0, sizeof(simulated_io.NavInputs));
        simulated_io.ClearInputCharacters();
    }
    else
    {
        engine->Inputs.Queue.resize(0);
    }
}

// FIXME: Trying to abort a running GUI test won't kill the app immediately.
static void ImGuiTestEngine_UpdateWatchdog(ImGuiTestEngine* engine, ImGuiContext* ctx, double t0, double t1)
{
    ImGuiTestContext* test_ctx = engine->TestContext;

    if (!engine->IO.ConfigRunFast || ImOsIsDebuggerPresent())
        return;

    if (test_ctx->RunFlags & ImGuiTestRunFlags_ManualRun)
        return;

    const float timer_warn      = engine->IO.ConfigRunWithGui ? 30.0f : 15.0f;
    const float timer_kill_test = engine->IO.ConfigRunWithGui ? 60.0f : 30.0f;
    const float timer_kill_app  = engine->IO.ConfigRunWithGui ? FLT_MAX : 35.0f;

    // Emit a warning and then fail the test after a given time.
    if (t0 < timer_warn && t1 >= timer_warn)
    {
        test_ctx->LogWarning("[Watchdog] Running time for '%s' is >%.f seconds, may be excessive.", test_ctx->Test->Name, timer_warn);
    }
    if (t0 < timer_kill_test && t1 >= timer_kill_test)
    {
        test_ctx->LogError("[Watchdog] Running time for '%s' is >%.f seconds, aborting.", test_ctx->Test->Name, timer_kill_test);
        IM_CHECK(false);
    }

    // Final safety watchdog in case the TestFunc is calling Yield() but never returning.
    // Note that we are not catching infinite loop cases where the TestFunc may be running but not yielding..
    if (t0 < timer_kill_app + 5.0f && t1 >= timer_kill_app + 5.0f)
    {
        test_ctx->LogError("[Watchdog] Emergency process exit as the test didn't return.");
        exit(1);
    }
}

static void ImGuiTestEngine_PreNewFrame(ImGuiTestEngine* engine, ImGuiContext* ctx)
{
    if (engine->UiContextTarget != ctx)
        return;
    IM_ASSERT(ctx == GImGui);
    ImGuiContext& g = *ctx;

    // Inject extra time into the imgui context
    if (engine->OverrideDeltaTime >= 0.0f)
    {
        ctx->IO.DeltaTime = engine->OverrideDeltaTime;
        engine->OverrideDeltaTime = -1.0f;
    }

    // NewFrame() will increase this so we are +1 ahead at the time of calling this
    engine->FrameCount = g.FrameCount + 1;
    if (ImGuiTestContext* test_ctx = engine->TestContext)
    {
        double t0 = test_ctx->RunningTime;
        double t1 = t0 + ctx->IO.DeltaTime;
        test_ctx->FrameCount++;
        test_ctx->RunningTime = t1;
        ImGuiTestEngine_UpdateWatchdog(engine, ctx, t0, t1);
    }

    engine->PerfDeltaTime100.AddSample(g.IO.DeltaTime);
    engine->PerfDeltaTime500.AddSample(g.IO.DeltaTime);
    engine->PerfDeltaTime1000.AddSample(g.IO.DeltaTime);
    engine->PerfDeltaTime2000.AddSample(g.IO.DeltaTime);

    if (ImGuiTestEngine_IsRunningTests(engine) && !engine->Abort)
    {
        // Abort testing by holding ESC
        // When running GuiFunc only main_io == simulated_io so we held for a hold.
        ImGuiIO& main_io = g.IO;
        //ImGuiIO& simulated_io = engine->Inputs.SimulatedIO;
        const int key_idx_escape = g.IO.KeyMap[ImGuiKey_Escape];
        if (key_idx_escape != -1 && main_io.KeysDownDuration[key_idx_escape] > 0.5f)// && !simulated_io.KeysDown[key_idx_escape])
        {
            if (engine->TestContext)
                engine->TestContext->LogWarning("KO: User aborted (pressed ESC)");
            ImGuiTestEngine_Abort(engine);
        }
    }

    ImGuiTestEngine_ApplyInputToImGuiContext(engine);
}

static void ImGuiTestEngine_PostNewFrame(ImGuiTestEngine* engine, ImGuiContext* ctx)
{
    if (engine->UiContextTarget != ctx)
        return;
    IM_ASSERT(ctx == GImGui);

    // Restore host inputs
    const bool want_simulated_inputs = engine->UiContextActive != NULL && ImGuiTestEngine_IsRunningTests(engine) && !(engine->TestContext->RunFlags & ImGuiTestRunFlags_NoTestFunc);
    if (!want_simulated_inputs)
    {
        ImGuiIO& main_io = ctx->IO;
        //IM_ASSERT(engine->UiContextActive == NULL);
        if (engine->Inputs.ApplyingSimulatedIO > 0)
        {
            // Restore
            engine->Inputs.ApplyingSimulatedIO--;
            main_io.MousePos = engine->Inputs.HostLastMousePos;
            //main_io.WantSetMousePos = true;
        }
        else
        {
            // Backup
            if (ImGui::IsMousePosValid(&main_io.MousePos))
                engine->Inputs.MousePosValue = engine->Inputs.HostLastMousePos = main_io.MousePos;
        }
    }

    // Garbage collect unused tasks
    const int LOCATION_TASK_ELAPSE_FRAMES = 20;
    for (int task_n = 0; task_n < engine->LocateTasks.Size; task_n++)
    {
        ImGuiTestLocateTask* task = engine->LocateTasks[task_n];
        if (task->FrameCount < engine->FrameCount - LOCATION_TASK_ELAPSE_FRAMES && task->Result.RefCount == 0)
        {
            IM_DELETE(task);
            engine->LocateTasks.erase(engine->LocateTasks.Data + task_n);
            task_n--;
        }
    }

    // Slow down whole app
    if (engine->ToolSlowDown)
        ImSleepInMilliseconds(engine->ToolSlowDownMs);

    // Process on-going queues
    if (engine->CallDepth == 0)
        ImGuiTestEngine_ProcessTestQueue(engine);
}

// Yield control back from the TestFunc to the main update + GuiFunc, for one frame.
void ImGuiTestEngine_Yield(ImGuiTestEngine* engine)
{
    ImGuiTestContext* ctx = engine->TestContext;
    ImGuiContext& g = *ctx->UiContext;

    if (g.WithinFrameScope)
        engine->IO.EndFrameFunc(engine, engine->IO.UserData);

    engine->IO.NewFrameFunc(engine, engine->IO.UserData);
    IM_ASSERT(g.IO.DeltaTime > 0.0f);

    if (!g.WithinFrameScope)
        return;

    if (ctx)
    {
        // Can only yield in the test func!
        IM_ASSERT(ctx->ActiveFunc == ImGuiTestActiveFunc_TestFunc);
    }

    if (ctx && ctx->Test->GuiFunc)
    {
        // Call user GUI function
        if (!(ctx->RunFlags & ImGuiTestRunFlags_NoGuiFunc))
        {
            ImGuiTestActiveFunc backup_active_func = ctx->ActiveFunc;
            ctx->ActiveFunc = ImGuiTestActiveFunc_GuiFunc;
            engine->TestContext->Test->GuiFunc(engine->TestContext);
            ctx->ActiveFunc = backup_active_func;
        }

        // Safety net
        //if (ctx->Test->Status == ImGuiTestStatus_Error)
        ctx->RecoverFromUiContextErrors();
    }
}

void ImGuiTestEngine_SetDeltaTime(ImGuiTestEngine* engine, float delta_time)
{
    IM_ASSERT(delta_time >= 0.0f);
    engine->OverrideDeltaTime = delta_time;
}

int ImGuiTestEngine_GetFrameCount(ImGuiTestEngine* engine)
{
    return engine->FrameCount;
}

double ImGuiTestEngine_GetPerfDeltaTime500Average(ImGuiTestEngine* engine)
{
    return engine->PerfDeltaTime500.GetAverage();
}

const char* ImGuiTestEngine_GetVerboseLevelName(ImGuiTestVerboseLevel v)
{
    static const char* names[ImGuiTestVerboseLevel_COUNT] = { "Silent", "Error", "Warning", "Info", "Debug", "Trace" };
    IM_STATIC_ASSERT(IM_ARRAYSIZE(names) == ImGuiTestVerboseLevel_COUNT);
    if (v >= 0 && v < IM_ARRAYSIZE(names))
        return names[v];
    return "N/A";
}

bool ImGuiTestEngine_CaptureScreenshot(ImGuiTestEngine* engine, ImGuiCaptureArgs* args)
{
    ImGuiCaptureContext& ct = engine->CaptureTool.Context;
    if (ct.ScreenCaptureFunc == NULL)
    {
        IM_ASSERT(0);
        return false;
    }

    // Graphics API must render a window so it can be captured
    const bool backup_fast = engine->IO.ConfigRunFast;
    engine->IO.ConfigRunFast = false;

    while (ct.CaptureScreenshot(args))
        ImGuiTestEngine_Yield(engine);

    engine->IO.ConfigRunFast = backup_fast;
    return true;
}

static void ImGuiTestEngine_ProcessTestQueue(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->CallDepth == 0);
    engine->CallDepth++;

    // Avoid tracking scrolling in UI when running a single test
    const bool track_scrolling = (engine->TestsQueue.Size > 1) || (engine->TestsQueue.Size == 1 && (engine->TestsQueue[0].RunFlags & ImGuiTestRunFlags_CommandLine));

    int ran_tests = 0;
    engine->IO.RunningTests = true;
    for (int n = 0; n < engine->TestsQueue.Size; n++)
    {
        ImGuiTestRunTask* run_task = &engine->TestsQueue[n];
        ImGuiTest* test = run_task->Test;
        IM_ASSERT(test->Status == ImGuiTestStatus_Queued);

        if (engine->Abort)
        {
            test->Status = ImGuiTestStatus_Unknown;
            continue;
        }

        // FIXME-TESTS: Blind mode not supported
        IM_ASSERT(engine->UiContextTarget != NULL);
        IM_ASSERT(engine->UiContextActive == NULL);
        engine->UiContextActive = engine->UiContextTarget;
        engine->UiSelectedTest = test;
        test->Status = ImGuiTestStatus_Running;

        ImGuiTestContext ctx;
        ctx.Test = test;
        ctx.Engine = engine;
        ctx.EngineIO = &engine->IO;
        ctx.Inputs = &engine->Inputs;
        ctx.GatherTask = &engine->GatherTask;
        ctx.UserData = NULL;
        ctx.UiContext = engine->UiContextActive;
        ctx.PerfStressAmount = engine->IO.PerfStressAmount;
        ctx.RunFlags = run_task->RunFlags;
#ifdef IMGUI_HAS_DOCK
        ctx.HasDock = true;
#else
        ctx.HasDock = false;
#endif
        ImFormatString(ctx.CaptureArgs.OutImageFileTemplate, IM_ARRAYSIZE(ctx.CaptureArgs.OutImageFileTemplate), "captures/%s_%%04d.png", test->Name);
        engine->TestContext = &ctx;
        if (track_scrolling)
            engine->UiSelectAndScrollToTest = test;

        ctx.LogEx(ImGuiTestVerboseLevel_Info, ImGuiTestLogFlags_NoHeader, "----------------------------------------------------------------------");
        ctx.LogInfo("Test: '%s' '%s'..", test->Category, test->Name);
        if (test->UserDataConstructor != NULL)
        {
            if ((engine->UserDataBuffer == NULL) || (engine->UserDataBufferSize < test->UserDataSize))
            {
                IM_FREE(engine->UserDataBuffer);
                engine->UserDataBufferSize = test->UserDataSize;
                engine->UserDataBuffer = IM_ALLOC(engine->UserDataBufferSize);
            }

            test->UserDataConstructor(engine->UserDataBuffer);
            ImGuiTestEngine_RunTest(engine, &ctx, engine->UserDataBuffer);
            test->UserDataDestructor(engine->UserDataBuffer);
        }
        else
        {
            ImGuiTestEngine_RunTest(engine, &ctx, NULL);
        }
        ran_tests++;

        IM_ASSERT(engine->TestContext == &ctx);
        engine->TestContext = NULL;

        IM_ASSERT(engine->UiContextActive == engine->UiContextTarget);
        engine->UiContextActive = NULL;

        // Auto select the first error test
        //if (test->Status == ImGuiTestStatus_Error)
        //    if (engine->UiSelectedTest == NULL || engine->UiSelectedTest->Status != ImGuiTestStatus_Error)
        //        engine->UiSelectedTest = test;
    }
    engine->IO.RunningTests = false;

    engine->Abort = false;
    engine->CallDepth--;
    engine->TestsQueue.clear();

    //ImGuiContext& g = *engine->UiTestContext;
    //if (g.OpenPopupStack.empty())   // Don't refocus Test Engine UI if popups are opened: this is so we can see remaining popups when implementing tests.
    if (ran_tests && engine->IO.ConfigTakeFocusBackAfterTests)
        engine->UiFocus = true;
}

bool ImGuiTestEngine_IsRunningTests(ImGuiTestEngine* engine)
{
    return engine->TestsQueue.Size > 0;
}

bool ImGuiTestEngine_IsRunningTest(ImGuiTestEngine* engine, ImGuiTest* test)
{
    for (ImGuiTestRunTask& t : engine->TestsQueue)
        if (t.Test == test)
            return true;
    return false;
}

void ImGuiTestEngine_QueueTest(ImGuiTestEngine* engine, ImGuiTest* test, ImGuiTestRunFlags run_flags)
{
    if (ImGuiTestEngine_IsRunningTest(engine, test))
        return;

    // Detect lack of signal from imgui context, most likely not compiled with IMGUI_ENABLE_TEST_ENGINE=1
    if (engine->FrameCount < engine->UiContextTarget->FrameCount - 2)
    {
        ImGuiTestEngine_Abort(engine);
        IM_ASSERT(0 && "Not receiving signal from core library. Did you call ImGuiTestEngine_CreateContext() with the correct context? Did you compile imgui/ with IMGUI_ENABLE_TEST_ENGINE=1?");
        test->Status = ImGuiTestStatus_Error;
        return;
    }

    test->Status = ImGuiTestStatus_Queued;

    ImGuiTestRunTask run_task;
    run_task.Test = test;
    run_task.RunFlags = run_flags;
    engine->TestsQueue.push_back(run_task);
}

ImGuiTest* ImGuiTestEngine_RegisterTest(ImGuiTestEngine* engine, const char* category, const char* name, const char* src_file, int src_line)
{
    ImGuiTestGroup group = ImGuiTestGroup_Tests;
    if (strcmp(category, "perf") == 0)
        group = ImGuiTestGroup_Perf;

    ImGuiTest* t = IM_NEW(ImGuiTest)();
    t->Group = group;
    t->Category = category;
    t->Name = name;
    t->SourceFile = t->SourceFileShort = src_file;
    t->SourceLine = t->SourceLineEnd = src_line;
    engine->TestsAll.push_back(t);

    // Find filename only out of the fully qualified source path
    if (src_file)
        t->SourceFileShort = ImPathFindFilename(t->SourceFileShort);

    return t;
}

void ImGuiTestEngine_QueueTests(ImGuiTestEngine* engine, ImGuiTestGroup group, const char* filter_str, ImGuiTestRunFlags run_flags)
{
    IM_ASSERT(group >= 0 && group < ImGuiTestGroup_COUNT);
    ImGuiTextFilter filter;
    if (filter_str != NULL)
    {
        IM_ASSERT(strlen(filter_str) + 1 < IM_ARRAYSIZE(filter.InputBuf));
        ImFormatString(filter.InputBuf, IM_ARRAYSIZE(filter.InputBuf), "%s", filter_str);
        filter.Build();
    }
    for (int n = 0; n < engine->TestsAll.Size; n++)
    {
        ImGuiTest* test = engine->TestsAll[n];
        if (test->Group != group)
            continue;
        if (!filter.PassFilter(test->Name))
            continue;
        ImGuiTestEngine_QueueTest(engine, test, run_flags);
    }
}

void ImGuiTestEngine_CalcSourceLineEnds(ImGuiTestEngine* engine)
{
    if (engine->TestsAll.empty())
        return;

    ImVector<int> line_starts;
    line_starts.reserve(engine->TestsAll.Size);
    for (int n = 0; n < engine->TestsAll.Size; n++)
        line_starts.push_back(engine->TestsAll[n]->SourceLine);
    ImQsort(line_starts.Data, (size_t)line_starts.Size, sizeof(int), [](const void* lhs, const void* rhs) { return (*(const int*)lhs) - *(const int*)rhs; });

    for (int n = 0; n < engine->TestsAll.Size; n++)
    {
        ImGuiTest* test = engine->TestsAll[n];
        for (int m = 0; m < line_starts.Size - 1; m++) // FIXME-OPT
            if (line_starts[m] == test->SourceLine)
                test->SourceLineEnd = ImMax(test->SourceLine, line_starts[m + 1]);
    }
}

void ImGuiTestEngine_PrintResultSummary(ImGuiTestEngine* engine)
{
    int count_tested = 0;
    int count_success = 0;
    ImGuiTestEngine_GetResult(engine, count_tested, count_success);
    printf("\nTests Result: %s\n(%d/%d tests passed)\n", (count_success == count_tested) ? "OK" : "KO", count_success, count_tested);
}

void ImGuiTestEngine_GetResult(ImGuiTestEngine* engine, int& count_tested, int& count_success)
{
    count_tested = 0;
    count_success = 0;
    for (int n = 0; n < engine->TestsAll.Size; n++)
    {
        ImGuiTest* test = engine->TestsAll[n];
        if (test->Status == ImGuiTestStatus_Unknown)
            continue;
        IM_ASSERT(test->Status != ImGuiTestStatus_Queued);
        IM_ASSERT(test->Status != ImGuiTestStatus_Running);
        count_tested++;
        if (test->Status == ImGuiTestStatus_Success)
            count_success++;
    }
}

static void ImGuiTestEngine_RunTest(ImGuiTestEngine* engine, ImGuiTestContext* ctx, void* user_data)
{
    // Clear ImGui inputs to avoid key/mouse leaks from one test to another
    ImGuiTestEngine_ClearInput(engine);

    ImGuiTest* test = ctx->Test;
    ctx->UserData = user_data;
    ctx->FrameCount = 0;
    ctx->WindowRef("");
    ctx->SetInputMode(ImGuiInputSource_Mouse);
    ctx->Clipboard.clear();
    ctx->GenericVars.Clear();
    test->TestLog.Clear();

    // Setup buffered clipboard
    typedef const char* (*ImGuiGetClipboardTextFn)(void* user_data);
    typedef void        (*ImGuiSetClipboardTextFn)(void* user_data, const char* text);
    ImGuiGetClipboardTextFn backup_get_clipboard_text_fn = ctx->UiContext->IO.GetClipboardTextFn;
    ImGuiSetClipboardTextFn backup_set_clipboard_text_fn = ctx->UiContext->IO.SetClipboardTextFn;
    void*                   backup_clipboard_user_data   = ctx->UiContext->IO.ClipboardUserData;
    ctx->UiContext->IO.GetClipboardTextFn = [](void* user_data) -> const char*
    {
        ImGuiTestContext* ctx = (ImGuiTestContext*)user_data;
        return ctx->Clipboard.empty() ? "" : ctx->Clipboard.Data;
    };
    ctx->UiContext->IO.SetClipboardTextFn = [](void* user_data, const char* text)
    {
        ImGuiTestContext* ctx = (ImGuiTestContext*)user_data;
        ctx->Clipboard.resize((int)strlen(text) + 1);
        strcpy(ctx->Clipboard.Data, text);
    };
    ctx->UiContext->IO.ClipboardUserData = ctx;

    // Mark as currently running the TestFunc (this is the only time when we are allowed to yield)
    IM_ASSERT(ctx->ActiveFunc == ImGuiTestActiveFunc_None);
    ImGuiTestActiveFunc backup_active_func = ctx->ActiveFunc;
    ctx->ActiveFunc = ImGuiTestActiveFunc_TestFunc;

    // Warm up GUI
    // - We need one mandatory frame running GuiFunc before running TestFunc
    // - We add a second frame, to avoid running tests while e.g. windows are typically appearing for the first time, hidden,
    // measuring their initial size. Most tests are going to be more meaningful with this stabilized base.
    if (!(test->Flags & ImGuiTestFlags_NoWarmUp))
    {
        ctx->FrameCount -= 2;
        ctx->Yield();
        ctx->Yield();
    }
    ctx->FirstFrameCount = ctx->FrameCount;

    // Call user test function (optional)
    if (ctx->RunFlags & ImGuiTestRunFlags_NoTestFunc)
    {
        // No test function
        while (!engine->Abort && test->Status == ImGuiTestStatus_Running)
            ctx->Yield();
    }
    else
    {
        if (test->TestFunc)
        {
            // Test function
            test->TestFunc(ctx);
        }
        else
        {
            // No test function
            if (test->Flags & ImGuiTestFlags_NoAutoFinish)
                while (!engine->Abort && test->Status == ImGuiTestStatus_Running)
                    ctx->Yield();
        }

        // Recover missing End*/Pop* calls.
        ctx->RecoverFromUiContextErrors();

        if (!engine->IO.ConfigRunFast)
            ctx->SleepShort();

        while (engine->IO.ConfigKeepGuiFunc && !engine->Abort)
        {
            ctx->RunFlags |= ImGuiTestRunFlags_NoTestFunc;
            ctx->Yield();
        }
    }

    // Process and display result/status
    if (test->Status == ImGuiTestStatus_Running)
        test->Status = ImGuiTestStatus_Success;

    if (engine->Abort && test->Status != ImGuiTestStatus_Error)
        test->Status = ImGuiTestStatus_Unknown;

    if (test->Status == ImGuiTestStatus_Success)
    {
        if ((ctx->RunFlags & ImGuiTestRunFlags_NoSuccessMsg) == 0)
            ctx->LogInfo("Success.");
    }
    else if (engine->Abort)
        ctx->LogWarning("Aborted.");
    else if (test->Status == ImGuiTestStatus_Error)
        ctx->LogError("%s test failed.", test->Name);
    else
        ctx->LogWarning("Unknown status.");

    // Additional yields to avoid consecutive tests who may share identifiers from missing their window/item activation.
    ctx->RunFlags |= ImGuiTestRunFlags_NoGuiFunc;
    ctx->Yield();
    ctx->Yield();

    // Restore active func
    ctx->ActiveFunc = backup_active_func;

    // Restore backend clipboard functions
    ctx->UiContext->IO.GetClipboardTextFn = backup_get_clipboard_text_fn;
    ctx->UiContext->IO.SetClipboardTextFn = backup_set_clipboard_text_fn;
    ctx->UiContext->IO.ClipboardUserData = backup_clipboard_user_data;
}

//-------------------------------------------------------------------------
// [SECTION] HOOKS FOR CORE LIBRARY
//-------------------------------------------------------------------------
// - ImGuiTestEngineHook_PreNewFrame()
// - ImGuiTestEngineHook_PostNewFrame()
// - ImGuiTestEngineHook_ItemAdd()
// - ImGuiTestEngineHook_ItemInfo()
//-------------------------------------------------------------------------

void ImGuiTestEngineHook_PreNewFrame(ImGuiContext* ctx)
{
    if (ImGuiTestEngine* engine = GImGuiHookingEngine)
        ImGuiTestEngine_PreNewFrame(engine, ctx);
}

void ImGuiTestEngineHook_PostNewFrame(ImGuiContext* ctx)
{
    if (ImGuiTestEngine* engine = GImGuiHookingEngine)
        ImGuiTestEngine_PostNewFrame(engine, ctx);
}

void ImGuiTestEngineHook_ItemAdd(ImGuiContext* ctx, const ImRect& bb, ImGuiID id)
{
    ImGuiTestEngine* engine = GImGuiHookingEngine;
    if (engine == NULL || engine->UiContextActive != ctx)
        return;

    IM_ASSERT(id != 0);
    ImGuiContext& g = *ctx;
    ImGuiWindow* window = g.CurrentWindow;

    // FIXME-OPT: Early out if there are no active Locate/Gather tasks.

    // Locate Tasks
    if (ImGuiTestLocateTask* task = ImGuiTestEngine_FindLocateTask(engine, id))
    {
        ImGuiTestItemInfo* item = &task->Result;
        item->TimestampMain = g.FrameCount;
        item->ID = id;
        item->ParentID = window->IDStack.back();
        item->Window = window;
        item->RectFull = item->RectClipped = bb;
        item->RectClipped.ClipWithFull(window->ClipRect);      // This two step clipping is important, we want RectClipped to stays within RectFull
        item->RectClipped.ClipWithFull(item->RectFull);
        item->NavLayer = window->DC.NavLayerCurrent;
        item->Depth = 0;
        item->StatusFlags = (window->DC.LastItemId == id) ? window->DC.LastItemStatusFlags : ImGuiItemStatusFlags_None;
    }

    // Gather Task (only 1 can be active)
    if (engine->GatherTask.ParentID != 0 && window->DC.NavLayerCurrent == ImGuiNavLayer_Main) // FIXME: Layer filter?
    {
        const ImGuiID gather_parent_id = engine->GatherTask.ParentID;
        int depth = -1;
        if (gather_parent_id == window->IDStack.back())
        {
            depth = 0;
        }
        else
        {
            int max_depth = ImMin(window->IDStack.Size, engine->GatherTask.Depth);
            for (int n_depth = 1; n_depth < max_depth; n_depth++)
                if (window->IDStack[window->IDStack.Size - 1 - n_depth] == gather_parent_id)
                {
                    depth = n_depth;
                    break;
                }
        }
        if (depth != -1)
        {
            ImGuiTestItemInfo* item_info = engine->GatherTask.OutList->Pool.GetOrAddByKey(id);
            item_info->TimestampMain = engine->FrameCount;
            item_info->ID = id;
            item_info->ParentID = window->IDStack.back();
            item_info->Window = window;
            item_info->RectFull = item_info->RectClipped = bb;
            item_info->RectClipped.ClipWithFull(window->ClipRect);      // This two step clipping is important, we want RectClipped to stays within RectFull
            item_info->RectClipped.ClipWithFull(item_info->RectFull);
            item_info->NavLayer = window->DC.NavLayerCurrent;
            item_info->Depth = depth;
            engine->GatherTask.LastItemInfo = item_info;
        }
    }
}

// label is optional
void ImGuiTestEngineHook_ItemInfo(ImGuiContext* ctx, ImGuiID id, const char* label, ImGuiItemStatusFlags flags)
{
    ImGuiTestEngine* engine = GImGuiHookingEngine;
    if (engine == NULL || engine->UiContextActive != ctx)
        return;

    IM_ASSERT(id != 0);
    ImGuiContext& g = *ctx;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(window->DC.LastItemId == id || window->DC.LastItemId == 0);

    // Update Locate Task status flags
    if (ImGuiTestLocateTask* task = ImGuiTestEngine_FindLocateTask(engine, id))
    {
        ImGuiTestItemInfo* item = &task->Result;
        item->TimestampStatus = g.FrameCount;
        item->StatusFlags = flags;
        if (label)
            ImStrncpy(item->DebugLabel, label, IM_ARRAYSIZE(item->DebugLabel));
    }

    // Update Gather Task status flags
    if (engine->GatherTask.LastItemInfo && engine->GatherTask.LastItemInfo->ID == id)
    {
        ImGuiTestItemInfo* item = engine->GatherTask.LastItemInfo;
        item->TimestampStatus = g.FrameCount;
        item->StatusFlags = flags;
        if (label)
            ImStrncpy(item->DebugLabel, label, IM_ARRAYSIZE(item->DebugLabel));
    }
}

// Forward core/user-land text to test log
void ImGuiTestEngineHook_Log(ImGuiContext* ctx, const char* fmt, ...)
{
    ImGuiTestEngine* engine = GImGuiHookingEngine;
    if (engine == NULL || engine->UiContextActive != ctx)
        return;

    va_list args;
    va_start(args, fmt);
    engine->TestContext->LogExV(ImGuiTestVerboseLevel_Debug, ImGuiTestLogFlags_None, fmt, args);
    va_end(args);
}

void ImGuiTestEngineHook_AssertFunc(const char* expr, const char* file, const char* function, int line)
{
    if (ImGuiTestEngine* engine = GImGuiHookingEngine)
    {
        if (ImGuiTestContext* ctx = engine->TestContext)
        {
            ctx->LogError("Assert: '%s'", expr);
            ctx->LogWarning("In %s:%d, function %s()", file, line, function);
            if (ImGuiTest* test = ctx->Test)
                ctx->LogWarning("While running test: %s %s", test->Category, test->Name);
        }
    }

    // Consider using github.com/scottt/debugbreak
#ifdef _MSC_VER
    __debugbreak();
#else
    IM_ASSERT(0);
#endif
}

//-------------------------------------------------------------------------
// [SECTION] HOOKS FOR TESTS
//-------------------------------------------------------------------------
// - ImGuiTestEngineHook_Check()
// - ImGuiTestEngineHook_Error()
//-------------------------------------------------------------------------

// Return true to request a debugger break
bool ImGuiTestEngineHook_Check(const char* file, const char* func, int line, ImGuiTestCheckFlags flags, bool result, const char* expr, const char* value_expr)
{
    ImGuiTestEngine* engine = GImGuiHookingEngine;
    (void)func;

    // Removed absolute path from output so we have deterministic output (otherwise __FILE__ gives us machine depending output)
    const char* file_without_path = file ? (file + strlen(file)) : NULL;
    while (file_without_path > file && file_without_path[-1] != '/' && file_without_path[-1] != '\\')
        file_without_path--;

    if (ImGuiTestContext* ctx = engine->TestContext)
    {
        ImGuiTest* test = ctx->Test;
        //ctx->LogDebug("IM_CHECK(%s)", expr);
        if (!result)
        {
            if (!(ctx->RunFlags & ImGuiTestRunFlags_NoTestFunc))
                test->Status = ImGuiTestStatus_Error;

            bool display_value_expr = (value_expr != NULL) && (result == false);
            if (file)
            {
                if (display_value_expr)
                    ctx->LogError("KO %s:%d '%s' -> '%s'", file_without_path, line, expr, value_expr);
                else
                    ctx->LogError("KO %s:%d '%s'", file_without_path, line, expr);
            }
            else
            {
                if (display_value_expr)
                    ctx->LogError("KO '%s' -> '%s'", expr, value_expr);
                else
                    ctx->LogError("KO '%s'", expr);
            }
        }
        else if (!(flags & ImGuiTestCheckFlags_SilentSuccess))
        {
            if (file)
                ctx->LogInfo("OK %s:%d '%s'", file_without_path, line, expr);
            else
                ctx->LogInfo("OK '%s'", expr);
        }
    }
    else
    {
        ctx->LogError("Error: no active test!\n");
        IM_ASSERT(0);
    }

    if (result == false && engine->IO.ConfigStopOnError && !engine->Abort)
        engine->Abort = true;
        //ImGuiTestEngine_Abort(engine);

    if (result == false && engine->IO.ConfigBreakOnError && !engine->Abort)
        return true;

    return false;
}

bool ImGuiTestEngineHook_Error(const char* file, const char* func, int line, ImGuiTestCheckFlags flags, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Str256 buf;
    buf.setfv(fmt, args);
    bool ret = ImGuiTestEngineHook_Check(file, func, line, flags, false, buf.c_str());
    va_end(args);

    ImGuiTestEngine* engine = GImGuiHookingEngine;
    if (engine && engine->Abort)
        return false;
    return ret;
}

//-------------------------------------------------------------------------
// [SECTION] USER INTERFACE
//-------------------------------------------------------------------------
// - DrawTestLog() [internal]
// - GetVerboseLevelName() [internal]
// - ImGuiTestEngine_ShowTestWindow()
//-------------------------------------------------------------------------

// Look for " filename:number " in the string.
static bool ParseLineAndDrawFileOpenItem(ImGuiTestEngine* e, ImGuiTest* test, const char* line_start, const char* line_end)
{
    const char* separator = ImStrchrRange(line_start, line_end, ':');
    if (separator == NULL)
        return false;

    const char* filename_end = separator;
    const char* filename_start = separator - 1;
    while (filename_start > line_start && filename_start[-1] != ' ')
        filename_start--;

    int line_no = -1;
    sscanf(separator + 1, "%d ", &line_no);

    if (line_no == -1 || filename_start == filename_end)
        return false;

    Str256f buf("Open %.*s at line %d", (int)(filename_end - filename_start), filename_start, line_no);
    if (ImGui::MenuItem(buf.c_str()))
    {
        // FIXME-TESTS: Assume folder is same as folder of test->SourceFile!
        const char* src_file_path = test->SourceFile;
        const char* src_file_name = ImPathFindFilename(src_file_path);
        buf.setf("%.*s%.*s", (int)(src_file_name - src_file_path), src_file_path, (int)(filename_end - filename_start), filename_start);
        e->IO.SrcFileOpenFunc(buf.c_str(), line_no, e->IO.UserData);
    }

    return true;
}

static void DrawTestLog(ImGuiTestEngine* e, ImGuiTest* test, bool is_interactive)
{
    ImU32 error_col = IM_COL32(255, 150, 150, 255);
    ImU32 warning_col = IM_COL32(240, 240, 150, 255);
    ImU32 unimportant_col = IM_COL32(190, 190, 190, 255);

    // FIXME-OPT: Split TestLog by lines so we can clip it easily.
    ImGuiTestLog* log = &test->TestLog;

    const char* text = test->TestLog.Buffer.begin();
    const char* text_end = test->TestLog.Buffer.end();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 2.0f));
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

void    ImGuiTestEngine_ShowTestGroup(ImGuiTestEngine* engine, ImGuiTestGroup group)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiTextFilter* filter = &engine->UiTestFilter;

    //ImGui::Text("TESTS (%d)", engine->TestsAll.Size);
    if (ImGui::Button("Run All"))
        ImGuiTestEngine_QueueTests(engine, group, filter->InputBuf); // FIXME: Filter func differs

    ImGui::SameLine();
    filter->Draw("##filter", -1.0f);
    ImGui::Separator();

    if (ImGui::BeginChild("Tests", ImVec2(0, 0)))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 3));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 1));
        for (int n = 0; n < engine->TestsAll.Size; n++)
        {
            ImGuiTest* test = engine->TestsAll[n];
            if (test->Group != group)
                continue;
            if (!filter->PassFilter(test->Name) && !filter->PassFilter(test->Category))
                continue;

            ImGuiTestContext* test_context = (engine->TestContext && engine->TestContext->Test == test) ? engine->TestContext : NULL;

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
            if (ImGui::Selectable(buf.c_str(), test == engine->UiSelectedTest))
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

            if (engine->UiSelectAndScrollToTest == test)
                ImGui::SetScrollHereY();

            bool view_source = false;
            if (ImGui::BeginPopupContextItem())
            {
                select_test = true;

                if (ImGui::MenuItem("Run test"))
                    queue_test = true;

                bool is_running_gui_func = (test_context && (test_context->RunFlags & ImGuiTestRunFlags_NoTestFunc));
                if (ImGui::MenuItem("Run GUI func", NULL, is_running_gui_func))
                {
                    if (is_running_gui_func)
                    {
                        ImGuiTestEngine_Abort(engine);
                    }
                    else
                    {
                        queue_gui_func = true;
                    }
                }
                ImGui::Separator();

                const bool open_source_available = (test->SourceFile != NULL) && (engine->IO.SrcFileOpenFunc != NULL);
                if (open_source_available)
                {
                    buf.setf("Open source (%s:%d)", test->SourceFileShort, test->SourceLine);
                    if (ImGui::MenuItem(buf.c_str()))
                    {
                        engine->IO.SrcFileOpenFunc(test->SourceFile, test->SourceLine, engine->IO.UserData);
                    }
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
                engine->UiSelectedTest = test;

            // Process queuing
            if (engine->CallDepth == 0)
            {
                if (queue_test)
                    ImGuiTestEngine_QueueTest(engine, test, ImGuiTestRunFlags_ManualRun);
                else if (queue_gui_func)
                    ImGuiTestEngine_QueueTest(engine, test, ImGuiTestRunFlags_ManualRun | ImGuiTestRunFlags_NoTestFunc);
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
    ImGui::SetNextItemWidth(60);
    if (ImGui::DragInt("Verbose", (int*)&engine->IO.ConfigVerboseLevel, 0.1f, 0, ImGuiTestVerboseLevel_COUNT - 1, ImGuiTestEngine_GetVerboseLevelName(engine->IO.ConfigVerboseLevel)))
        engine->IO.ConfigVerboseLevelOnError = engine->IO.ConfigVerboseLevel;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(30);
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
            ImGuiTestEngine_ShowTestGroup(engine, ImGuiTestGroup_Tests);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("PERFS"))
        {
            ImGuiTestEngine_ShowTestGroup(engine, ImGuiTestGroup_Perf);
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
                DrawTestLog(engine, engine->UiSelectedTest, true);
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
            ImGui::Checkbox("Capture Tool", &engine->CaptureTool.Visible);
            ImGui::Checkbox("Slow down whole app", &engine->ToolSlowDown);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            ImGui::SliderInt("##ms", &engine->ToolSlowDownMs, 0, 400, "%d ms");

            ImGui::Separator();
            ImGui::Text("Configuration:");
            ImGui::CheckboxFlags("io.ConfigFlags: NavEnableKeyboard", (unsigned int *)&io.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard);
            ImGui::CheckboxFlags("io.ConfigFlags: NavEnableGamepad", (unsigned int *)&io.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad);
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

            const ImGuiInputTextCallback filter_callback = [](ImGuiInputTextCallbackData* data) { return (data->EventChar == ',' || data->EventChar == ';') ? 1 : 0; };
            ImGui::InputText("Branch/Annotation", engine->IO.PerfAnnotation, IM_ARRAYSIZE(engine->IO.PerfAnnotation), ImGuiInputTextFlags_CallbackCharFilter, filter_callback, NULL);

            double dt_ref = engine->PerfRefDeltaTime;
            ImGui::Text("[ref dt]    %6.3f ms", engine->PerfRefDeltaTime * 1000);
            ImGui::Text("[last 0001] %6.3f ms (%.1f FPS) ++ %6.3f ms",                           dt_1    * 1000.0, 1.0 / dt_1,    (dt_1 - dt_ref) * 1000);
            ImGui::Text("[last 0100] %6.3f ms (%.1f FPS) ++ %6.3f ms ~ converging in %.1f secs", dt_100  * 1000.0, 1.0 / dt_100,  (dt_1 - dt_ref) * 1000, 100.0  / fps_now);
            ImGui::Text("[last 1000] %6.3f ms (%.1f FPS) ++ %6.3f ms ~ converging in %.1f secs", dt_1000 * 1000.0, 1.0 / dt_1000, (dt_1 - dt_ref) * 1000, 1000.0 / fps_now);
            ImGui::Text("[last 2000] %6.3f ms (%.1f FPS) ++ %6.3f ms ~ converging in %.1f secs", dt_2000 * 1000.0, 1.0 / dt_2000, (dt_1 - dt_ref) * 1000, 2000.0 / fps_now);

            //ImGui::PlotLines("Last 100", &engine->PerfDeltaTime100.Samples.Data, engine->PerfDeltaTime100.Samples.Size, engine->PerfDeltaTime100.Idx, NULL, 0.0f, dt_1000 * 1.10f, ImVec2(0.0f, ImGui::GetFontSize()));
            ImVec2 plot_size(0.0f, ImGui::GetFrameHeight() * 3);
            ImMovingAverage<double>* ma = &engine->PerfDeltaTime500;
            ImGui::PlotLines("Last 500",
                [](void* data, int n) { ImMovingAverage<double>* ma = (ImMovingAverage<double>*)data; return (float)(ma->Samples[n] * 1000); },
                ma, ma->Samples.Size, 0*ma->Idx, NULL, 0.0f, (float)(ImMax(dt_100, dt_1000) * 1000.0 * 1.2f), plot_size);

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
// [SECTION] SETTINGS
//-------------------------------------------------------------------------
// FIXME: In our wildest dreams we could provide a imgui_club/ serialization helper that would be
// easy to use in both the ReadLine and WriteAll functions.
//-------------------------------------------------------------------------

static void*    ImGuiTestEngine_SettingsReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
    if (strcmp(name, "Data") != 0)
        return NULL;
    return (void*)1;
}

static void     ImGuiTestEngine_SettingsReadLine(ImGuiContext* imgui_ctx, ImGuiSettingsHandler*, void* entry, const char* line)
{
    ImGuiTestEngine* engine = GImGuiHookingEngine;
    IM_ASSERT(engine != NULL);
    IM_ASSERT(engine->UiContextTarget == imgui_ctx);

    int n = 0;
    if (sscanf(line, "Filter=%s", engine->UiTestFilter.InputBuf) == 1)  { engine->UiTestFilter.Build(); }   // FIXME
    else if (sscanf(line, "LogHeight=%f", &engine->UiLogHeight) == 1)   { }
    else if (sscanf(line, "CaptureTool=%d", &n) == 1)                   { engine->CaptureTool.Visible = (n != 0); }
}

static void     ImGuiTestEngine_SettingsWriteAll(ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    ImGuiTestEngine* engine = GImGuiHookingEngine;
    IM_ASSERT(engine != NULL);
    IM_ASSERT(engine->UiContextTarget == imgui_ctx);

    buf->appendf("[%s][Data]\n", handler->TypeName);
    buf->appendf("Filter=%s\n", engine->UiTestFilter.InputBuf);
    buf->appendf("LogHeight=%.0f\n", engine->UiLogHeight);
    buf->appendf("CaptureTool=%d\n", engine->CaptureTool.Visible);
    buf->appendf("\n");
}

//-------------------------------------------------------------------------
