// dear imgui
// (test engine, core)

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_te_core.h"
#include "imgui_te_util.h"
#include "imgui_te_context.h"
#include "imgui_te_internal.h"
#include "shared/imgui_utils.h"
#include "libs/Str/Str.h"

/*

Index of this file:

// [SECTION] TODO
// [SECTION] FORWARD DECLARATIONS
// [SECTION] DATA STRUCTURES
// [SECTION] TEST ENGINE FUNCTIONS
// [SECTION] HOOKS FOR CORE LIBRARY
// [SECTION] HOOKS FOR TESTS
// [SECTION] USER INTERFACE
// [SECTION] SETTINGS
// [SECTION] ImGuiTest

*/

//-------------------------------------------------------------------------
// [SECTION] TODO
//-------------------------------------------------------------------------

// GOAL: Code coverage.
// GOAL: Custom testing.
// GOAL: Take screenshots for web/docs.
// GOAL: Reliable performance measurement (w/ deterministic setup)
// GOAL: Full blind version with no graphical context.

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
// [SECTION] DATA
//-------------------------------------------------------------------------

static ImGuiTestEngine* GImGuiHookingEngine = NULL;

//-------------------------------------------------------------------------
// [SECTION] FORWARD DECLARATIONS
//-------------------------------------------------------------------------

// Private functions
static void ImGuiTestEngine_StartCalcSourceLineEnds(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ClearInput(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ApplyInputToImGuiContext(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ProcessTestQueue(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ClearTests(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ClearLocateTasks(ImGuiTestEngine* engine);
static void ImGuiTestEngine_PreNewFrame(ImGuiTestEngine* engine, ImGuiContext* ui_ctx);
static void ImGuiTestEngine_PostNewFrame(ImGuiTestEngine* engine, ImGuiContext* ui_ctx);
static void ImGuiTestEngine_RunGuiFunc(ImGuiTestEngine* engine);
static void ImGuiTestEngine_RunTest(ImGuiTestEngine* engine, ImGuiTestContext* ctx);
static void ImGuiTestEngine_TestQueueCoroutineMain(void* engine_opaque);

// Settings
static void* ImGuiTestEngine_SettingsReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
static void  ImGuiTestEngine_SettingsReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line);
static void  ImGuiTestEngine_SettingsWriteAll(ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);

//-------------------------------------------------------------------------
// [SECTION] TEST ENGINE FUNCTIONS
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

void    ImGuiTestEngine_CoroutineStopRequest(ImGuiTestEngine* engine)
{
    if (engine->TestQueueCoroutine != NULL)
        engine->TestQueueCoroutineShouldExit = true;
}

static void    ImGuiTestEngine_CoroutineStopAndJoin(ImGuiTestEngine* engine)
{
    if (engine->TestQueueCoroutine != NULL)
    {
        // Run until the coroutine exits
        engine->TestQueueCoroutineShouldExit = true;
        while (true)
        {
            if (!engine->IO.CoroutineFuncs->RunFunc(engine->TestQueueCoroutine))
                break;
        }
        engine->IO.CoroutineFuncs->DestroyFunc(engine->TestQueueCoroutine);
        engine->TestQueueCoroutine = NULL;
    }
}

void    ImGuiTestEngine_ShutdownContext(ImGuiTestEngine* engine)
{
    // Shutdown coroutine
    ImGuiTestEngine_CoroutineStopAndJoin(engine);

    engine->UiContextVisible = engine->UiContextBlind = engine->UiContextTarget = engine->UiContextActive = NULL;

    IM_FREE(engine->UserDataBuffer);
    engine->UserDataBuffer = NULL;
    engine->UserDataBufferSize = 0;

    ImGuiTestEngine_ClearTests(engine);
    ImGuiTestEngine_ClearLocateTasks(engine);
    IM_DELETE(engine);

    // Release hook
    if (GImGuiHookingEngine == engine)
        GImGuiHookingEngine = NULL;
}

void    ImGuiTestEngine_Start(ImGuiTestEngine* engine)
{
    IM_ASSERT(!engine->Started);

    ImGuiTestEngine_StartCalcSourceLineEnds(engine);

    // Create our coroutine
    // (we include the word "Main" in the name to facilitate filtering for both this thread and the "Main Thread" in debuggers)
    if (!engine->TestQueueCoroutine)
        engine->TestQueueCoroutine = engine->IO.CoroutineFuncs->CreateFunc(ImGuiTestEngine_TestQueueCoroutineMain, "Main Dear ImGui Test Thread", engine);

    engine->Started = true;
}

void    ImGuiTestEngine_Stop(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->Started);

    ImGuiTestEngine_CoroutineStopAndJoin(engine);
    engine->Started = false;
}

void    ImGuiTestEngine_PostRender(ImGuiTestEngine* engine)
{
    // Capture a screenshot from main thread while coroutine waits
    if (engine->CurrentCaptureArgs != NULL)
    {
        engine->CaptureContext.ScreenCaptureFunc = engine->IO.ScreenCaptureFunc;
        if (!engine->CaptureContext.CaptureUpdate(engine->CurrentCaptureArgs))
        {
            ImStrncpy(engine->CaptureTool.LastSaveFileName, engine->CurrentCaptureArgs->OutSavedFileName, IM_ARRAYSIZE(engine->CaptureTool.LastSaveFileName));
            engine->CurrentCaptureArgs = NULL;
        }
    }
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
#ifdef IMGUI_TEST_ENGINE_DEBUG
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
    simulated_io.KeyMods = ImGuiKeyModFlags_None;
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

static bool ImGuiTestEngine_UseSimulatedInputs(ImGuiTestEngine* engine)
{
    if (engine->UiContextActive)
        if (ImGuiTestEngine_IsRunningTests(engine))
            if (!(engine->TestContext->RunFlags & ImGuiTestRunFlags_NoTestFunc))
                return true;
    return false;
}

void ImGuiTestEngine_ApplyInputToImGuiContext(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->UiContextTarget != NULL);
    ImGuiContext& g = *engine->UiContextTarget;

    ImGuiIO& main_io = g.IO;
    ImGuiIO& simulated_io = engine->Inputs.SimulatedIO;

    main_io.MouseDrawCursor = true;

    const bool use_simulated_inputs = ImGuiTestEngine_UseSimulatedInputs(engine);
    if (use_simulated_inputs)
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
        simulated_io.KeyMods  = (engine->Inputs.KeyMods);

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
        COPY_FIELD(KeyMods);
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
static void ImGuiTestEngine_UpdateWatchdog(ImGuiTestEngine* engine, ImGuiContext* ui_ctx, double t0, double t1)
{
    IM_UNUSED(ui_ctx);
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

static void ImGuiTestEngine_PreNewFrame(ImGuiTestEngine* engine, ImGuiContext* ui_ctx)
{
    if (engine->UiContextTarget != ui_ctx)
        return;
    IM_ASSERT(ui_ctx == GImGui);
    ImGuiContext& g = *ui_ctx;

    // Inject extra time into the imgui context
    if (engine->OverrideDeltaTime >= 0.0f)
    {
        ui_ctx->IO.DeltaTime = engine->OverrideDeltaTime;
        engine->OverrideDeltaTime = -1.0f;
    }

    // NewFrame() will increase this so we are +1 ahead at the time of calling this
    engine->FrameCount = g.FrameCount + 1;
    if (ImGuiTestContext* test_ctx = engine->TestContext)
    {
        double t0 = test_ctx->RunningTime;
        double t1 = t0 + ui_ctx->IO.DeltaTime;
        test_ctx->FrameCount++;
        test_ctx->RunningTime = t1;
        ImGuiTestEngine_UpdateWatchdog(engine, ui_ctx, t0, t1);
    }

    engine->PerfDeltaTime100.AddSample(g.IO.DeltaTime);
    engine->PerfDeltaTime500.AddSample(g.IO.DeltaTime);
    engine->PerfDeltaTime1000.AddSample(g.IO.DeltaTime);
    engine->PerfDeltaTime2000.AddSample(g.IO.DeltaTime);

    if (ImGuiTestEngine_IsRunningTests(engine) && !engine->Abort)
    {
        // Abort testing by holding ESC
        // When running GuiFunc only main_io == simulated_io we test for a long hold.
        ImGuiIO& main_io = g.IO;
        ImGuiIO& simulated_io = engine->Inputs.SimulatedIO;
        const int key_idx_escape = g.IO.KeyMap[ImGuiKey_Escape];
        const bool use_simulated_inputs = ImGuiTestEngine_UseSimulatedInputs(engine);

        bool abort = false;
        if (use_simulated_inputs)
            abort = (key_idx_escape != -1 && main_io.KeysDown[key_idx_escape] && !simulated_io.KeysDown[key_idx_escape]);
        else
            abort = (key_idx_escape != -1 && main_io.KeysDownDuration[key_idx_escape] > 0.5f);
        if (abort)
        {
            if (engine->TestContext)
                engine->TestContext->LogWarning("KO: User aborted (pressed ESC)");
            ImGuiTestEngine_Abort(engine);
        }
    }

    ImGuiTestEngine_ApplyInputToImGuiContext(engine);
}

static void ImGuiTestEngine_PostNewFrame(ImGuiTestEngine* engine, ImGuiContext* ui_ctx)
{
    if (engine->UiContextTarget != ui_ctx)
        return;
    IM_ASSERT(ui_ctx == GImGui);

    // Restore host inputs
    const bool want_simulated_inputs = engine->UiContextActive != NULL && ImGuiTestEngine_IsRunningTests(engine) && !(engine->TestContext->RunFlags & ImGuiTestRunFlags_NoTestFunc);
    if (!want_simulated_inputs)
    {
        ImGuiIO& main_io = ui_ctx->IO;
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
        ImThreadSleepInMilliseconds(engine->ToolSlowDownMs);

    // Call user GUI function
    ImGuiTestEngine_RunGuiFunc(engine);

    // Process on-going queues in a coroutine
    // Run the test coroutine. This will resume the test queue from either the last point the test called YieldFromCoroutine(),
    // or the loop in ImGuiTestEngine_TestQueueCoroutineMain that does so if no test is running.
    // If you want to breakpoint the point execution continues in the test code, breakpoint the exit condition in YieldFromCoroutine()
    engine->IO.CoroutineFuncs->RunFunc(engine->TestQueueCoroutine);

    // Update output flags
    engine->IO.RenderWantMaxSpeed = (engine->IO.RunningTests && engine->IO.ConfigRunFast) || engine->IO.ConfigNoThrottle;
}

static void ImGuiTestEngine_RunGuiFunc(ImGuiTestEngine* engine)
{
    ImGuiTestContext* ctx = engine->TestContext;
    if (ctx && ctx->Test->GuiFunc)
    {
        ctx->Test->GuiFuncLastFrame = ctx->UiContext->FrameCount;
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

// Main function for the test coroutine
static void ImGuiTestEngine_TestQueueCoroutineMain(void* engine_opaque)
{
    ImGuiTestEngine* engine = (ImGuiTestEngine*)engine_opaque;
    while (!engine->TestQueueCoroutineShouldExit)
    {
        ImGuiTestEngine_ProcessTestQueue(engine);
        engine->IO.CoroutineFuncs->YieldFunc();
    }
}

// Yield control back from the TestFunc to the main update + GuiFunc, for one frame.
void ImGuiTestEngine_Yield(ImGuiTestEngine* engine)
{
    ImGuiTestContext* ctx = engine->TestContext;

    // Can only yield in the test func!
    if (ctx)
        IM_ASSERT(ctx->ActiveFunc == ImGuiTestActiveFunc_TestFunc);

    engine->IO.CoroutineFuncs->YieldFunc();
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
    if (engine->IO.ScreenCaptureFunc == NULL)
    {
        IM_ASSERT(0);
        return false;
    }

    // Graphics API must render a window so it can be captured
    // FIXME: This should work without this, as long as Present vs Vsync are separated (we need a Present, we don't need Vsync)
    const bool backup_fast = engine->IO.ConfigRunFast;
    engine->IO.ConfigRunFast = false;

    // Because we rely on window->ContentSize for stitching, let 1 extra frame elapse to make sure any
    // windows which contents have changed in the last frame get a correct window->ContentSize value.
    ImGuiTestEngine_Yield(engine);

    engine->CurrentCaptureArgs = args;
    while (engine->CurrentCaptureArgs != NULL)
        ImGuiTestEngine_Yield(engine);

    engine->IO.ConfigRunFast = backup_fast;
    return true;
}

bool ImGuiTestEngine_BeginCaptureAnimation(ImGuiTestEngine* engine, ImGuiCaptureArgs* args)
{
    if (engine->IO.ScreenCaptureFunc == NULL)
    {
        IM_ASSERT(0);
        return false;
    }

    engine->RunFastBackupValue = engine->IO.ConfigRunFast;
    engine->IO.ConfigRunFast = false;
    engine->CurrentCaptureArgs = args;
    engine->CaptureContext.BeginGifCapture(args);
    return true;
}

bool ImGuiTestEngine_EndCaptureAnimation(ImGuiTestEngine* engine, ImGuiCaptureArgs* args)
{
    engine->CaptureContext.EndGifCapture(args);
    while (engine->CaptureContext._GifWriter != NULL)   // Wait until last frame is captured and gif is saved.
        ImGuiTestEngine_Yield(engine);
    engine->IO.ConfigRunFast = engine->RunFastBackupValue;
    engine->CurrentCaptureArgs = NULL;
    return true;
};

static void ImGuiTestEngine_ProcessTestQueue(ImGuiTestEngine* engine)
{
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
        engine->TestContext = &ctx;
        if (track_scrolling)
            engine->UiSelectAndScrollToTest = test;

        ctx.LogEx(ImGuiTestVerboseLevel_Info, ImGuiTestLogFlags_NoHeader, "----------------------------------------------------------------------");

        // Test name is not displayed in UI due to a happy accident - logged test name is cleared in
        // ImGuiTestEngine_RunTest(). This is a behavior we want.
        ctx.LogWarning("Test: '%s' '%s'..", test->Category, test->Name);
        if (test->UserDataConstructor != NULL)
        {
            if ((engine->UserDataBuffer == NULL) || (engine->UserDataBufferSize < test->UserDataSize))
            {
                IM_FREE(engine->UserDataBuffer);
                engine->UserDataBufferSize = test->UserDataSize;
                engine->UserDataBuffer = IM_ALLOC(engine->UserDataBufferSize);
            }

            // Run test with a custom data type in the stack
            ctx.UserData = engine->UserDataBuffer;
            test->UserDataConstructor(engine->UserDataBuffer);
            ImGuiTestEngine_RunTest(engine, &ctx);
            test->UserDataDestructor(engine->UserDataBuffer);
        }
        else
        {
            // Run test
            ImGuiTestEngine_RunTest(engine, &ctx);
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

static bool ImGuiTestEngine_IsRunningTest(ImGuiTestEngine* engine, ImGuiTest* test)
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
        group = ImGuiTestGroup_Perfs;

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

static void ImGuiTestEngine_StartCalcSourceLineEnds(ImGuiTestEngine* engine)
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

static void ImGuiTestEngine_RunTest(ImGuiTestEngine* engine, ImGuiTestContext* ctx)
{
    // Clear ImGui inputs to avoid key/mouse leaks from one test to another
    ImGuiTestEngine_ClearInput(engine);

    ImGuiTest* test = ctx->Test;
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
        // Sanity check
        if (test->GuiFunc)
            IM_ASSERT(test->GuiFuncLastFrame == ctx->UiContext->FrameCount);

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

    // Restore back-end clipboard functions
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

void ImGuiTestEngineHook_PreNewFrame(ImGuiContext* ui_ctx)
{
    if (ImGuiTestEngine* engine = GImGuiHookingEngine)
        ImGuiTestEngine_PreNewFrame(engine, ui_ctx);
}

void ImGuiTestEngineHook_PostNewFrame(ImGuiContext* ui_ctx)
{
    if (ImGuiTestEngine* engine = GImGuiHookingEngine)
        ImGuiTestEngine_PostNewFrame(engine, ui_ctx);
}

void ImGuiTestEngineHook_ItemAdd(ImGuiContext* ui_ctx, const ImRect& bb, ImGuiID id)
{
    ImGuiTestEngine* engine = GImGuiHookingEngine;
    if (engine == NULL || engine->UiContextActive != ui_ctx)
        return;

    IM_ASSERT(id != 0);
    ImGuiContext& g = *ui_ctx;
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
void ImGuiTestEngineHook_ItemInfo(ImGuiContext* ui_ctx, ImGuiID id, const char* label, ImGuiItemStatusFlags flags)
{
    ImGuiTestEngine* engine = GImGuiHookingEngine;
    if (engine == NULL || engine->UiContextActive != ui_ctx)
        return;

    IM_ASSERT(id != 0);
    ImGuiContext& g = *ui_ctx;
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
void ImGuiTestEngineHook_Log(ImGuiContext* ui_ctx, const char* fmt, ...)
{
    ImGuiTestEngine* engine = GImGuiHookingEngine;
    if (engine == NULL || engine->UiContextActive != ui_ctx)
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
#if __GNUC__
    // __builtin_trap() is not part of IM_DEBUG_BREAK() because GCC optimizes away everything that comes after.
    __builtin_trap();
#else
    IM_DEBUG_BREAK();
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
    const char* file_without_path = file ? ImPathFindFilename(file) : "";

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
    IM_UNUSED(entry);
    ImGuiTestEngine* engine = GImGuiHookingEngine;
    IM_ASSERT(engine != NULL);
    IM_ASSERT(engine->UiContextTarget == imgui_ctx);

    int n = 0;
    /**/ if (sscanf(line, "FilterTests=%s", engine->UiFilterTests.InputBuf) == 1)   { engine->UiFilterTests.Build(); }
    else if (sscanf(line, "FilterPerfs=%s", engine->UiFilterPerfs.InputBuf) == 1)   { engine->UiFilterPerfs.Build(); }
    else if (sscanf(line, "LogHeight=%f", &engine->UiLogHeight) == 1)               { }
    else if (sscanf(line, "CaptureTool=%d", &n) == 1)                               { engine->CaptureTool.Visible = (n != 0); }
}

static void     ImGuiTestEngine_SettingsWriteAll(ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    ImGuiTestEngine* engine = GImGuiHookingEngine;
    IM_ASSERT(engine != NULL);
    IM_ASSERT(engine->UiContextTarget == imgui_ctx);

    buf->appendf("[%s][Data]\n", handler->TypeName);
    buf->appendf("FilterTests=%s\n", engine->UiFilterTests.InputBuf);
    buf->appendf("FilterPerfs=%s\n", engine->UiFilterPerfs.InputBuf);
    buf->appendf("LogHeight=%.0f\n", engine->UiLogHeight);
    buf->appendf("CaptureTool=%d\n", engine->CaptureTool.Visible);
    buf->appendf("\n");
}

//-------------------------------------------------------------------------
// [SECTION] ImGuiTest
//-------------------------------------------------------------------------

ImGuiTest::~ImGuiTest()
{
    if (NameOwned)
        ImGui::MemFree((char*)Name);
}

void ImGuiTest::SetOwnedName(const char* name)
{
    IM_ASSERT(!NameOwned);
    NameOwned = true;
    Name = ImStrdup(name);
}

//-------------------------------------------------------------------------
