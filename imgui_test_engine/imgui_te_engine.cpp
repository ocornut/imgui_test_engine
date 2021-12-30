// dear imgui
// (test engine, core)

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_te_engine.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_te_util.h"
#include "imgui_te_context.h"
#include "imgui_te_internal.h"
#include "imgui_te_perftool.h"
#include "shared/imgui_utils.h"
#include "thirdparty/Str/Str.h"

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

// GOAL: Custom testing.
// GOAL: Take screenshots and video for web/docs or testing purpose.
// GOAL: Improve code coverage.
// GOAL: Reliable performance measurement (w/ deterministic setup)
// GOAL: Run blind/tty with no graphical context.

// Note: GuiFunc can't run code that yields. There is an assert for that.
// Note: GuiFunc in tests generally can't use ImGuiCond_Once or ImGuiCond_FirstUseEver reliably.

// FIXME-TESTS: Make it possible to run ad hoc tests without registering/adding to a global list.
// FIXME-TESTS: UI to setup breakpoint (e.g. GUI func on frame X, beginning of Test func or at certain Yield/Sleep spot)
// FIXME-TESTS: Be able to run blind within GUI application (second second)
// FIXME-TESTS: Be able to run in own contexts to avoid side-effects
// FIXME-TESTS: Randomize test order in shared context ~ kind of fuzzing (need to be able to repro order)
// FIXME-TESTS: Mouse actions on ImGuiNavLayer_Menu layer
// FIXME-TESTS: Failing to open a double-click tree node


//-------------------------------------------------------------------------
// [SECTION] DATA
//-------------------------------------------------------------------------

static ImGuiTestEngine* GImGuiTestEngine = NULL;

//-------------------------------------------------------------------------
// [SECTION] FORWARD DECLARATIONS
//-------------------------------------------------------------------------

// Private functions
static void ImGuiTestEngine_CoroutineStopAndJoin(ImGuiTestEngine* engine);
static void ImGuiTestEngine_StartCalcSourceLineEnds(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ClearInput(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ApplyInputToImGuiContext(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ProcessTestQueue(ImGuiTestEngine* engine);
static void ImGuiTestEngine_ClearTests(ImGuiTestEngine* engine);
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
// - ImGuiTestEngine_BindImGuiContext()
// - ImGuiTestEngine_UnbindImGuiContext()
// - ImGuiTestEngine_GetIO()
// - ImGuiTestEngine_Abort()
// - ImGuiTestEngine_QueueAllTests()
//-------------------------------------------------------------------------
// - ImGuiTestEngine_FindItemInfo()
// - ImGuiTestEngine_ClearTests()
// - ImGuiTestEngine_ApplyInputToImGuiContext()
// - ImGuiTestEngine_PreNewFrame()
// - ImGuiTestEngine_PostNewFrame()
// - ImGuiTestEngine_Yield()
// - ImGuiTestEngine_ProcessTestQueue()
// - ImGuiTestEngine_QueueTest()
// - ImGuiTestEngine_RunTest()
//-------------------------------------------------------------------------

ImGuiTestEngine::ImGuiTestEngine()
{
    PerfRefDeltaTime = 0.0f;
    PerfDeltaTime100.Init(100);
    PerfDeltaTime500.Init(500);
    PerfDeltaTime1000.Init(1000);
    PerfDeltaTime2000.Init(2000);
    PerfTool = IM_NEW(ImGuiPerfTool);

    // Initialize std::thread based coroutine implementation if requested
#ifdef IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL
    IO.CoroutineFuncs = Coroutine_ImplStdThread_GetInterface();
#endif
}

ImGuiTestEngine::~ImGuiTestEngine()
{
    IM_ASSERT(TestQueueCoroutine == NULL);
    if (UiContextBlind != NULL)
        ImGui::DestroyContext(UiContextBlind);
    IM_DELETE(PerfTool);
}

static void ImGuiTestEngine_UnbindImGuiContext(ImGuiTestEngine* engine, ImGuiContext* imgui_context);

static void ImGuiTestEngine_BindImGuiContext(ImGuiTestEngine* engine, ImGuiContext* ui_ctx)
{
    IM_ASSERT(engine->UiContextTarget == NULL);

    engine->UiContextVisible = ui_ctx;
    engine->UiContextBlind = NULL;
    engine->UiContextTarget = engine->UiContextVisible;
    engine->UiContextActive = NULL;

    // Add .ini handle for ImGuiWindow type
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "TestEngine";
    ini_handler.TypeHash = ImHashStr("TestEngine");
    ini_handler.ReadOpenFn = ImGuiTestEngine_SettingsReadOpen;
    ini_handler.ReadLineFn = ImGuiTestEngine_SettingsReadLine;
    ini_handler.WriteAllFn = ImGuiTestEngine_SettingsWriteAll;
    ui_ctx->SettingsHandlers.push_back(ini_handler);

    // Install generic context hooks facility
    ImGuiContextHook hook;
    hook.Type = ImGuiContextHookType_Shutdown;
    hook.Callback = [](ImGuiContext* ui_ctx, ImGuiContextHook* hook) { ImGuiTestEngine_UnbindImGuiContext((ImGuiTestEngine*)hook->UserData, ui_ctx); };
    hook.UserData = (void*)engine;
    ImGui::AddContextHook(ui_ctx, &hook);

    hook.Type = ImGuiContextHookType_NewFramePre;
    hook.Callback = [](ImGuiContext* ui_ctx, ImGuiContextHook* hook) { ImGuiTestEngine_PreNewFrame((ImGuiTestEngine*)hook->UserData, ui_ctx); };
    hook.UserData = (void*)engine;
    ImGui::AddContextHook(ui_ctx, &hook);

    hook.Type = ImGuiContextHookType_NewFramePost;
    hook.Callback = [](ImGuiContext* ui_ctx, ImGuiContextHook* hook) { ImGuiTestEngine_PostNewFrame((ImGuiTestEngine*)hook->UserData, ui_ctx); };
    hook.UserData = (void*)engine;
    ImGui::AddContextHook(ui_ctx, &hook);

    // Install custom test engine hook data
    if (GImGuiTestEngine == NULL)
        GImGuiTestEngine = engine;
    IM_ASSERT(ui_ctx->TestEngine == NULL);
    ui_ctx->TestEngine = engine;
}

static void    ImGuiTestEngine_UnbindImGuiContext(ImGuiTestEngine* engine, ImGuiContext* ui_ctx)
{
    IM_ASSERT(engine->UiContextTarget == ui_ctx);

    // FIXME: Could use ImGui::RemoveContextHook() if we stored our hook ids
    for (int hook_n = 0; hook_n < ui_ctx->Hooks.Size; hook_n++)
        if (ui_ctx->Hooks[hook_n].UserData == engine)
            ImGui::RemoveContextHook(ui_ctx, ui_ctx->Hooks[hook_n].HookId);

    ImGuiTestEngine_CoroutineStopAndJoin(engine);

#if IMGUI_VERSION_NUM >= 17701
    IM_ASSERT(ui_ctx->TestEngine == engine);
    ui_ctx->TestEngine = NULL;

    // Remove .ini handler
    IM_ASSERT(GImGui == ui_ctx);
    if (ImGuiSettingsHandler* ini_handler = ImGui::FindSettingsHandler("TestEngine"))
        ui_ctx->SettingsHandlers.erase(ui_ctx->SettingsHandlers.Data + ui_ctx->SettingsHandlers.index_from_ptr(ini_handler));
#endif

    // Remove hook
    if (GImGuiTestEngine == engine)
        GImGuiTestEngine = NULL;
    engine->UiContextVisible = engine->UiContextBlind = engine->UiContextTarget = engine->UiContextActive = NULL;
}

// Create test context and attach to imgui context
ImGuiTestEngine*    ImGuiTestEngine_CreateContext(ImGuiContext* ui_ctx)
{
    IM_ASSERT(ui_ctx != NULL);
    ImGuiTestEngine* engine = IM_NEW(ImGuiTestEngine)();
    ImGuiTestEngine_BindImGuiContext(engine, ui_ctx);
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
    if (engine->UiContextTarget != NULL)
        ImGuiTestEngine_UnbindImGuiContext(engine, engine->UiContextTarget);

    IM_FREE(engine->UserDataBuffer);
    engine->UserDataBuffer = NULL;
    engine->UserDataBufferSize = 0;

    ImGuiTestEngine_ClearTests(engine);

    for (int n = 0; n < engine->InfoTasks.Size; n++)
        IM_DELETE(engine->InfoTasks[n]);
    engine->InfoTasks.clear();

    IM_DELETE(engine);

    // Release hook
    if (GImGuiTestEngine == engine)
        GImGuiTestEngine = NULL;
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

// [EXPERIMENTAL] Destroy and recreate ImGui context
// This potentially allow us to test issues related to handling new windows, restoring settings etc.
// This also gets us once inch closer to more dynamic management of context (e.g. jail tests in their own context)
// FIXME: This is currently called by ImGuiTestEngine_PreNewFrame() in hook but may end up needing to be called
// by main application loop in order to facilitate letting app know of the new pointers. For now none of our backends
// preserve the pointer so may be fine.
void    ImGuiTestEngine_RebootUiContext(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->Started);
    ImGuiContext* ctx = engine->UiContextTarget;
    ImGuiTestEngine_Stop(engine);
    ImGuiTestEngine_UnbindImGuiContext(engine, ctx);

    // Backup
    bool backup_atlas_owned_by_context = ctx->FontAtlasOwnedByContext;
    ImFontAtlas* backup_atlas = ctx->IO.Fonts;
    ImGuiIO backup_io = ctx->IO;
#ifdef IMGUI_HAS_VIEWPORT
    // FIXME: Break with multi-viewports as we don't preserve user windowing data properly.
    // Backend tend to store e.g. HWND data in viewport 0.
    if (ctx->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        IM_ASSERT(0);
    //ImGuiViewport backup_viewport0 = *(ImGuiViewport*)ctx->Viewports[0];
    //ImGuiPlatformIO backup_platform_io = ctx->PlatformIO;
    //ImGui::DestroyPlatformWindows();
#endif

    // Recreate
    ctx->FontAtlasOwnedByContext = false;
#if 1
    ImGui::DestroyContext();
    ImGui::CreateContext(backup_atlas);
#else
    // Preserve same context pointer, which is probably misleading and not even necessary.
    ImGui::Shutdown(ctx);
    ctx->~ImGuiContext();
    IM_PLACEMENT_NEW(ctx) ImGuiContext(backup_atlas);
    ImGui::Initialize(ctx);
#endif

    // Restore
    ctx->FontAtlasOwnedByContext = backup_atlas_owned_by_context;
    ctx->IO = backup_io;
#ifdef IMGUI_HAS_VIEWPORT
    //backup_platform_io.Viewports.swap(ctx->PlatformIO.Viewports);
    //ctx->PlatformIO = backup_platform_io;
    //ctx->Viewports[0]->RendererUserData = backup_viewport0.RendererUserData;
    //ctx->Viewports[0]->PlatformUserData = backup_viewport0.PlatformUserData;
    //ctx->Viewports[0]->PlatformHandle = backup_viewport0.PlatformHandle;
    //ctx->Viewports[0]->PlatformHandleRaw = backup_viewport0.PlatformHandleRaw;
    //memset(&backup_viewport0, 0, sizeof(backup_viewport0));
#endif

    ImGuiTestEngine_BindImGuiContext(engine, ctx);
    ImGuiTestEngine_Start(engine);
}

void    ImGuiTestEngine_PostSwap(ImGuiTestEngine* engine)
{
    if (engine->IO.ConfigFixedDeltaTime != 0.0f)
        ImGuiTestEngine_SetDeltaTime(engine, engine->IO.ConfigFixedDeltaTime);

    // Capture a screenshot from main thread while coroutine waits
    if (engine->CurrentCaptureArgs != NULL)
    {
        engine->CaptureContext.ScreenCaptureFunc = engine->IO.ScreenCaptureFunc;
        engine->CaptureContext.ScreenCaptureUserData = engine->IO.ScreenCaptureUserData;
        ImGuiCaptureStatus status = engine->CaptureContext.CaptureUpdate(engine->CurrentCaptureArgs);
        if (status != ImGuiCaptureStatus_InProgress)
        {
            if (status == ImGuiCaptureStatus_Done)
                ImStrncpy(engine->CaptureTool.LastOutputFileName, engine->CurrentCaptureArgs->OutSavedFileName, IM_ARRAYSIZE(engine->CaptureTool.LastOutputFileName));
            engine->CurrentCaptureArgs = NULL;
        }
    }
}

ImGuiTestEngineIO&  ImGuiTestEngine_GetIO(ImGuiTestEngine* engine)
{
    return engine->IO;
}

void    ImGuiTestEngine_AbortTest(ImGuiTestEngine* engine)
{
    engine->Abort = true;
    if (ImGuiTestContext* test_context = engine->TestContext)
        test_context->Abort = true;
}

bool    ImGuiTestEngine_TryAbortEngine(ImGuiTestEngine* engine)
{
    ImGuiTestEngine_AbortTest(engine);
    ImGuiTestEngine_CoroutineStopRequest(engine);
    if (!ImGuiTestEngine_IsRunningTests(engine))
        return true;
    return false; // Still running coroutine
}

// FIXME-OPT
static ImGuiTestInfoTask* ImGuiTestEngine_FindInfoTask(ImGuiTestEngine* engine, ImGuiID id)
{
    for (int task_n = 0; task_n < engine->InfoTasks.Size; task_n++)
    {
        ImGuiTestInfoTask* task = engine->InfoTasks[task_n];
        if (task->ID == id)
            return task;
    }
    return NULL;
}

// Request information about one item.
// Will push a request for the test engine to process.
// Will return NULL when results are not ready (or not available).
ImGuiTestItemInfo* ImGuiTestEngine_FindItemInfo(ImGuiTestEngine* engine, ImGuiID id, const char* debug_id)
{
    IM_ASSERT(id != 0);

    if (ImGuiTestInfoTask* task = ImGuiTestEngine_FindInfoTask(engine, id))
    {
        if (task->Result.TimestampMain + 2 >= engine->FrameCount)
        {
            task->FrameCount = engine->FrameCount; // Renew task
            return &task->Result;
        }
        return NULL;
    }

    // Create task
    ImGuiTestInfoTask* task = IM_NEW(ImGuiTestInfoTask)();
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
#else
    IM_UNUSED(debug_id);
#endif
    engine->InfoTasks.push_back(task);

    return NULL;
}

static void ImGuiTestEngine_ClearTests(ImGuiTestEngine* engine)
{
    for (int n = 0; n < engine->TestsAll.Size; n++)
        IM_DELETE(engine->TestsAll[n]);
    engine->TestsAll.clear();
    engine->TestsQueue.clear();
}

void ImGuiTestEngine_ClearInput(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->UiContextTarget != NULL);
    engine->Inputs.MouseButtonsValue = 0;
    engine->Inputs.KeyMods = ImGuiKeyModFlags_None;
    engine->Inputs.Queue.clear();
    engine->Inputs.MouseWheel = ImVec2(0, 0);

    ImGuiIO& simulated_io = engine->Inputs.SimulatedIO;
    simulated_io.KeyCtrl = simulated_io.KeyShift = simulated_io.KeyAlt = simulated_io.KeySuper = false;
    simulated_io.KeyMods = ImGuiKeyModFlags_None;
#if IMGUI_VERSION_NUM >= 18314
    simulated_io.KeyModsPrev = ImGuiKeyModFlags_None;
#endif
    memset(simulated_io.MouseDown, 0, sizeof(simulated_io.MouseDown));
    memset(simulated_io.KeysDown, 0, sizeof(simulated_io.KeysDown));
    memset(simulated_io.NavInputs, 0, sizeof(simulated_io.NavInputs));
    simulated_io.ClearInputCharacters();
    ImGuiTestEngine_ApplyInputToImGuiContext(engine);
}

static bool ImGuiTestEngine_UseSimulatedInputs(ImGuiTestEngine* engine)
{
    if (engine->UiContextActive)
        if (ImGuiTestEngine_IsRunningTests(engine))
            if (!(engine->TestContext->RunFlags & ImGuiTestRunFlags_GuiFuncOnly))
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
                    simulated_io.NavInputs[input.NavInput] = (input.State == ImGuiKeyState_Down) ? 1.0f : 0.0f;
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
        //simulated_io.KeyMods  = (engine->Inputs.KeyMods);

        // Mouse wheel
        // [OSX] Simulate OSX behavior of automatically swapping mouse wheel axis when SHIFT is held.
        // This is working in conjonction with the fact that ImGuiTestContext::MouseWheel() assume Windows-style behavior.
        simulated_io.MouseWheelH = engine->Inputs.MouseWheel.x;
        simulated_io.MouseWheel = engine->Inputs.MouseWheel.y;
        if (simulated_io.ConfigMacOSXBehaviors && simulated_io.KeyShift)
            ImSwap(simulated_io.MouseWheelH, simulated_io.MouseWheel);
        engine->Inputs.MouseWheel = ImVec2(0, 0);

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
        //COPY_FIELD(KeyMods);
        COPY_FIELD(KeysDown);
        COPY_FIELD(NavInputs);
#ifdef IMGUI_HAS_VIEWPORT
        COPY_FIELD(MouseHoveredViewport);
#endif
        #undef COPY_FIELD

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

    if (engine->ToolDebugRebootUiContext)
    {
        ImGuiTestEngine_RebootUiContext(engine);
        ui_ctx = engine->UiContextTarget;
        engine->ToolDebugRebootUiContext = false;
    }

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
            abort = (key_idx_escape != -1 && main_io.KeysDownDuration[key_idx_escape] > 0.30f);
        if (abort)
        {
            if (engine->TestContext)
                engine->TestContext->LogWarning("KO: User aborted (pressed ESC)");
            ImGuiTestEngine_AbortTest(engine);
        }
    }

    ImGuiTestEngine_ApplyInputToImGuiContext(engine);
    ImGuiTestEngine_UpdateHooks(engine);
}

static void ImGuiTestEngine_PostNewFrame(ImGuiTestEngine* engine, ImGuiContext* ui_ctx)
{
    if (engine->UiContextTarget != ui_ctx)
        return;
    IM_ASSERT(ui_ctx == GImGui);

    engine->CaptureContext.PostNewFrame();
    engine->CaptureTool.Context.PostNewFrame();

    // Restore host inputs
    const bool want_simulated_inputs = engine->UiContextActive != NULL && ImGuiTestEngine_IsRunningTests(engine) && !(engine->TestContext->RunFlags & ImGuiTestRunFlags_GuiFuncOnly);
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
    for (int task_n = 0; task_n < engine->InfoTasks.Size; task_n++)
    {
        ImGuiTestInfoTask* task = engine->InfoTasks[task_n];
        if (task->FrameCount < engine->FrameCount - LOCATION_TASK_ELAPSE_FRAMES && task->Result.RefCount == 0)
        {
            IM_DELETE(task);
            engine->InfoTasks.erase(engine->InfoTasks.Data + task_n);
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

    // Update hooks and output flags
    ImGuiTestEngine_UpdateHooks(engine);

    // Disable vsync
    engine->IO.RenderWantMaxSpeed = engine->IO.ConfigNoThrottle;
    if (engine->IO.ConfigRunFast && engine->IO.RunningTests && engine->TestContext && (engine->TestContext->RunFlags & ImGuiTestRunFlags_GuiFuncOnly) == 0)
        engine->IO.RenderWantMaxSpeed = true;
}

static void ImGuiTestEngine_RunGuiFunc(ImGuiTestEngine* engine)
{
    ImGuiTestContext* ctx = engine->TestContext;
    if (ctx && ctx->Test->GuiFunc)
    {
        ctx->Test->GuiFuncLastFrame = ctx->UiContext->FrameCount;
        if (!(ctx->RunFlags & ImGuiTestRunFlags_GuiFuncDisable))
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
    if (ctx)
        ctx->FirstGuiFrame = false;
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

static void ImGuiTestEngine_DisableWindowInputs(ImGuiWindow* window)
{
    window->DisableInputsFrames = 1;
    for (ImGuiWindow* child_window : window->DC.ChildWindows)
        ImGuiTestEngine_DisableWindowInputs(child_window);
}

// Yield control back from the TestFunc to the main update + GuiFunc, for one frame.
void ImGuiTestEngine_Yield(ImGuiTestEngine* engine)
{
    ImGuiTestContext* ctx = engine->TestContext;

    // Can only yield in the test func!
    if (ctx)
    {
        IM_ASSERT(ctx->ActiveFunc == ImGuiTestActiveFunc_TestFunc && "Can only yield inside TestFunc()!");
        for (ImGuiWindow* window : ctx->ForeignWindowsToHide)
        {
            window->HiddenFramesForRenderOnly = 2;          // Hide root window
            ImGuiTestEngine_DisableWindowInputs(window);    // Disable inputs for root window and all it's children recursively
        }
    }

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

    IM_ASSERT(engine->CurrentCaptureArgs == NULL && "Nested captures are not supported.");

    // Graphics API must render a window so it can be captured
    // FIXME: This should work without this, as long as Present vs Vsync are separated (we need a Present, we don't need Vsync)
    const bool backup_fast = engine->IO.ConfigRunFast;
    engine->IO.ConfigRunFast = false;

    const int frame_count = engine->FrameCount;

    // Because we rely on window->ContentSize for stitching, let 1 extra frame elapse to make sure any
    // windows which contents have changed in the last frame get a correct window->ContentSize value.
    // FIXME: Can remove this yield if not stitching
    if ((args->InFlags & ImGuiCaptureFlags_Instant) == 0)
        ImGuiTestEngine_Yield(engine);

    // This will yield until ImGui::Render() -> ImGuiTestEngine_PostSwap() -> ImGuiCaptureContext::CaptureUpdate() return false.
    engine->CurrentCaptureArgs = args;
    while (engine->CurrentCaptureArgs != NULL)
        ImGuiTestEngine_Yield(engine);

    // Verify that the ImGuiCaptureFlags_Instant flag got honored
    if (args->InFlags & ImGuiCaptureFlags_Instant)
        IM_ASSERT(frame_count + 1== engine->FrameCount);

    engine->IO.ConfigRunFast = backup_fast;
    return true;
}

bool ImGuiTestEngine_CaptureBeginGif(ImGuiTestEngine* engine, ImGuiCaptureArgs* args)
{
    if (engine->IO.ScreenCaptureFunc == NULL)
    {
        IM_ASSERT(0);
        return false;
    }

    IM_ASSERT(engine->CurrentCaptureArgs == NULL && "Nested captures are not supported.");

    engine->BackupConfigRunFast = engine->IO.ConfigRunFast;
    engine->BackupConfigNoThrottle = engine->IO.ConfigNoThrottle;
    if (engine->IO.ConfigRunFast)
    {
        engine->IO.ConfigRunFast = false;
        engine->IO.ConfigNoThrottle = true;
        engine->IO.ConfigFixedDeltaTime = 1.0f / 60.0f;
    }
    engine->CurrentCaptureArgs = args;
    engine->CaptureContext.BeginGifCapture(args);
    return true;
}

bool ImGuiTestEngine_CaptureEndGif(ImGuiTestEngine* engine, ImGuiCaptureArgs* args)
{
    IM_UNUSED(args);
    IM_ASSERT(engine->CurrentCaptureArgs != NULL && "No capture is in progress.");
    IM_ASSERT(engine->CaptureContext.IsCapturingGif() && "No gif capture is in progress.");

    engine->CaptureContext.EndGifCapture();
    while (engine->CaptureContext._GifWriter != NULL)   // Wait until last frame is captured and gif is saved.
        ImGuiTestEngine_Yield(engine);
    engine->IO.ConfigRunFast = engine->BackupConfigRunFast;
    engine->IO.ConfigNoThrottle = engine->BackupConfigNoThrottle;
    engine->IO.ConfigFixedDeltaTime = 0;
    engine->CurrentCaptureArgs = NULL;
    return true;
}

static void ImGuiTestEngine_ProcessTestQueue(ImGuiTestEngine* engine)
{
    // Avoid tracking scrolling in UI when running a single test
    const bool track_scrolling = (engine->TestsQueue.Size > 1) || (engine->TestsQueue.Size == 1 && (engine->TestsQueue[0].RunFlags & ImGuiTestRunFlags_CommandLine));
    ImGuiIO& io = ImGui::GetIO();
    const char* settings_ini_backup = io.IniFilename;
    io.IniFilename = NULL;

    ImU64 batch_start_time = ImTimeGetInMicroseconds();
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
        ctx.UserVars = NULL;
        ctx.UiContext = engine->UiContextActive;
        ctx.PerfStressAmount = engine->IO.PerfStressAmount;
        ctx.RunFlags = run_task->RunFlags;
        ctx.BatchStartTime = batch_start_time;
#ifdef IMGUI_HAS_DOCK
        ctx.HasDock = true;
#else
        ctx.HasDock = false;
#endif
        engine->TestContext = &ctx;
        ImGuiTestEngine_UpdateHooks(engine);
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
            ctx.UserVars = engine->UserDataBuffer;
            test->UserDataConstructor(engine->UserDataBuffer);
            if (test->UserDataPostConstructor != NULL && test->UserDataPostConstructorFn != NULL)
                test->UserDataPostConstructor(engine->UserDataBuffer, test->UserDataPostConstructorFn);
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
        IM_ASSERT(engine->UiContextActive == engine->UiContextTarget);
        engine->TestContext = NULL;
        engine->UiContextActive = NULL;
        ImGuiTestEngine_UpdateHooks(engine);

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
    io.IniFilename = settings_ini_backup;
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
        ImGuiTestEngine_AbortTest(engine);
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

ImGuiPerfTool* ImGuiTestEngine_GetPerfTool(ImGuiTestEngine* engine)
{
    return engine->PerfTool;
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

    if (count_success < count_tested)
    {
        printf("\nFailing tests:\n");
        for (ImGuiTest* test : engine->TestsAll)
            if (test->Status == ImGuiTestStatus_Error)
                printf("- %s\n", test->Name);
    }

    ImOsConsoleSetTextColor(ImOsConsoleStream_StandardOutput, (count_success == count_tested) ? ImOsConsoleTextColor_BrightGreen : ImOsConsoleTextColor_BrightRed);
    printf("\nTests Result: %s\n", (count_success == count_tested) ? "OK" : "KO");
    printf("(%d/%d tests passed)\n", count_success, count_tested);
    ImOsConsoleSetTextColor(ImOsConsoleStream_StandardOutput, ImOsConsoleTextColor_White);
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

void ImGuiTestEngine_UpdateHooks(ImGuiTestEngine* engine)
{
    ImGuiContext* ui_ctx = engine->UiContextTarget;
    IM_ASSERT(ui_ctx->TestEngine == engine);
    bool want_hooking = false;

    //if (engine->TestContext != NULL)
    //    want_hooking = true;

    if (engine->InfoTasks.Size > 0)
        want_hooking = true;
    if (engine->FindByLabelTask.InSuffix != NULL)
        want_hooking = true;
    if (engine->GatherTask.InParentID != 0)
        want_hooking = true;

    // Update test engine specific hooks
    ui_ctx->TestEngineHookItems = want_hooking;
}

static void ImGuiTestEngine_RunTest(ImGuiTestEngine* engine, ImGuiTestContext* ctx)
{
    // Clear ImGui inputs to avoid key/mouse leaks from one test to another
    ImGuiTestEngine_ClearInput(engine);

    ImGuiTest* test = ctx->Test;
    ctx->FrameCount = 0;
    ctx->ErrorCounter = 0;
    ctx->SetRef("");
    ctx->SetInputMode(ImGuiInputSource_Mouse);
    ctx->UiContext->NavInputSource = ImGuiInputSource_Keyboard;
    ctx->Clipboard.clear();
    ctx->GenericVars.Clear();
    test->TestLog.Clear();

    // Back entire IO and style. Allows tests modifying them and not caring about restoring state.
    ImGuiIO backup_io = ctx->UiContext->IO;
    ImGuiStyle backup_style = ctx->UiContext->Style;
    memset(backup_io.MouseDown, 0, sizeof(backup_io.MouseDown));
    memset(backup_io.KeysDown, 0, sizeof(backup_io.KeysDown));

#ifdef IMGUI_HAS_VIEWPORT
    // Backends that support _HasMouseHoveredViewport are unable to satisfy it's requirements when tests run,
    // because io.MousePos does not match position of real mouse cursor and hovered viewport is not necessarily one
    // that simulated mouse cursor hovers. In order to satisfy this condition we have to do a window search much
    // like FindHoveredWindow() (imgui.cpp) does. However given purpose of this flag, it is not important for test
    // suite, therefore it must be disabled when tests run. Flag is restored in ImGuiTestEngine_ProcessTestQueue()
    // when test queue finishes.
    ctx->UiContext->IO.BackendFlags &= ~ImGuiBackendFlags_HasMouseHoveredViewport;
#endif

    // Setup buffered clipboard
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
    ctx->FirstGuiFrame = (test->GuiFunc != NULL) ? true : false;

    // Warm up GUI
    // - We need one mandatory frame running GuiFunc before running TestFunc
    // - We add a second frame, to avoid running tests while e.g. windows are typically appearing for the first time, hidden,
    // measuring their initial size. Most tests are going to be more meaningful with this stabilized base.
    if (!(test->Flags & ImGuiTestFlags_NoWarmUp))
    {
        ctx->FrameCount -= 2;
        ctx->Yield();
        if (test->Status == ImGuiTestStatus_Running) // To allow GuIFunc calling Finish() in first frame
            ctx->Yield();
    }
    ctx->FirstTestFrameCount = ctx->FrameCount;

    // Call user test function (optional)
    if (ctx->RunFlags & ImGuiTestRunFlags_GuiFuncOnly)
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

        // Capture failure screenshot.
        if (ctx->IsError() && engine->IO.ConfigCaptureOnError)
        {
            // FIXME-VIEWPORT: Tested windows may be in their own viewport. This only captures everything in main viewport. Capture tool may be extended to capture viewport windows as well. This would leave out OS windows which may be a cause of failure.
            ImGuiCaptureArgs args;
            args.InCaptureRect.Min = ImGui::GetMainViewport()->Pos;
            args.InCaptureRect.Max = args.InCaptureRect.Min + ImGui::GetMainViewport()->Size;
            ctx->CaptureInitArgs(&args, ImGuiCaptureFlags_Instant);
            ImFormatString(args.InOutputFileTemplate, IM_ARRAYSIZE(args.InOutputFileTemplate), "output/failures/%s_%04d.png", ctx->Test->Name, ctx->ErrorCounter);
            if (ImGuiTestEngine_CaptureScreenshot(engine, &args))
                ctx->LogDebug("Saved '%s' (%d*%d pixels)", args.OutSavedFileName, (int)args.OutImageSize.x, (int)args.OutImageSize.y);
        }

        // Recover missing End*/Pop* calls.
        ctx->RecoverFromUiContextErrors();

        if (!engine->IO.ConfigRunFast)
            ctx->SleepShort();

        // Stop in GuiFunc mode
        if (engine->IO.ConfigKeepGuiFunc && ctx->IsError())
        {
            // Position mouse cursor
            ctx->UiContext->IO.WantSetMousePos = true;
            ctx->UiContext->IO.MousePos = engine->Inputs.MousePosValue;

            // Restore backend clipboard functions
            ctx->UiContext->IO.GetClipboardTextFn = backup_io.GetClipboardTextFn;
            ctx->UiContext->IO.SetClipboardTextFn = backup_io.SetClipboardTextFn;
            ctx->UiContext->IO.ClipboardUserData = backup_io.ClipboardUserData;

            // Unhide foreign windows (may be useful sometimes to inspect GuiFunc state... sometimes not)
            //ctx->ForeignWindowsUnhideAll();
        }

        // Keep GuiFunc spinning
        // FIXME-TESTS: after an error, this is not visible in the UI because status is not _Running anymore...
        while (engine->IO.ConfigKeepGuiFunc && !engine->Abort)
        {
            ctx->RunFlags |= ImGuiTestRunFlags_GuiFuncOnly;
            ctx->Yield();
        }
    }

    IM_ASSERT(engine->CurrentCaptureArgs == NULL && "Active capture was not terminated in the test code.");

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
    ctx->SetGuiFuncEnabled(false);
    ctx->Yield();
    ctx->Yield();

    // Restore active func
    ctx->ActiveFunc = backup_active_func;

    // Restore backed up IO and style
    backup_io.MetricsActiveAllocations = ctx->UiContext->IO.MetricsActiveAllocations;
    ctx->UiContext->IO = backup_io;
    ctx->UiContext->Style = backup_style;
}

//-------------------------------------------------------------------------
// [SECTION] HOOKS FOR CORE LIBRARY
//-------------------------------------------------------------------------
// - ImGuiTestEngineHook_ItemAdd()
// - ImGuiTestEngineHook_ItemInfo()
// - ImGuiTestEngineHook_Log()
// - ImGuiTestEngineHook_AssertFunc()
//-------------------------------------------------------------------------

void ImGuiTestEngineHook_ItemAdd(ImGuiContext* ui_ctx, const ImRect& bb, ImGuiID id)
{
    ImGuiTestEngine* engine = (ImGuiTestEngine*)ui_ctx->TestEngine;

    IM_ASSERT(id != 0);
    ImGuiContext& g = *ui_ctx;
    ImGuiWindow* window = g.CurrentWindow;

    // FIXME-OPT: Early out if there are no active Info/Gather tasks.

    // Info Tasks
    if (ImGuiTestInfoTask* task = ImGuiTestEngine_FindInfoTask(engine, id))
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
        item->StatusFlags = (g.LastItemData.ID == id) ? g.LastItemData.StatusFlags : ImGuiItemStatusFlags_None;
    }

    // Gather Task (only 1 can be active)
    if (engine->GatherTask.InParentID != 0 && window->DC.NavLayerCurrent == ImGuiNavLayer_Main) // FIXME: Layer filter?
    {
        const ImGuiID gather_parent_id = engine->GatherTask.InParentID;
        const ImGuiID stack_top_id = window->IDStack.back();
        int depth = -1;
        if (gather_parent_id == stack_top_id)
        {
            depth = 0;
        }
        else
        {
            int max_depth = ImMin(window->IDStack.Size, engine->GatherTask.InDepth + ((id == stack_top_id) ? 1 : 0));
            for (int n_depth = 1; n_depth < max_depth; n_depth++)
                if (window->IDStack[window->IDStack.Size - 1 - n_depth] == gather_parent_id)
                {
                    depth = n_depth;
                    break;
                }
        }
        if (depth != -1)
        {
            ImGuiTestItemInfo* item = engine->GatherTask.OutList->Pool.GetOrAddByKey(id);
            item->TimestampMain = engine->FrameCount;
            item->ID = id;
            item->ParentID = window->IDStack.back();
            item->Window = window;
            item->RectFull = item->RectClipped = bb;
            item->RectClipped.ClipWithFull(window->ClipRect);      // This two step clipping is important, we want RectClipped to stays within RectFull
            item->RectClipped.ClipWithFull(item->RectFull);
            item->NavLayer = window->DC.NavLayerCurrent;
            item->Depth = depth;
            engine->GatherTask.LastItemInfo = item;
        }
    }
}

// label is optional
#ifdef IMGUI_HAS_IMSTR
void ImGuiTestEngineHook_ItemInfo(ImGuiContext* ui_ctx, ImGuiID id, ImStrv label, ImGuiItemStatusFlags flags)
#else
void ImGuiTestEngineHook_ItemInfo(ImGuiContext* ui_ctx, ImGuiID id, const char* label, ImGuiItemStatusFlags flags)
#endif
{
    ImGuiTestEngine* engine = (ImGuiTestEngine*)ui_ctx->TestEngine;

    IM_ASSERT(id != 0);
    ImGuiContext& g = *ui_ctx;
    //ImGuiWindow* window = g.CurrentWindow;
    //IM_ASSERT(window->DC.LastItemId == id || window->DC.LastItemId == 0); // Need _ItemAdd() to be submitted before _ItemInfo()

    // Update Info Task status flags
    if (ImGuiTestInfoTask* task = ImGuiTestEngine_FindInfoTask(engine, id))
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

    // Update Find by Label Task
    ImGuiTestFindByLabelTask* label_task = &engine->FindByLabelTask;
    if (label && label_task->InSuffixLastItem && label_task->OutItemId == 0 && ImStrcmp(label_task->InSuffixLastItem, label) == 0)
    {
        bool match = false; //(label_task->InBaseId == 0);
        ImGuiWindow* window = g.CurrentWindow;
        if (!match)
        {
            // FIXME-TESTS: Depth limit?
            for (ImGuiID* p_id_stack = window->IDStack.end() - 1; p_id_stack >= window->IDStack.begin(); p_id_stack--)
                if (*p_id_stack == label_task->InPrefixId)
                {
                    if (ImGuiItemStatusFlags filter_flags = label_task->InFilterItemStatusFlags)
                        if (!(filter_flags & flags))
                            continue;
                    match = true;
                    break;
                }
        }

        // We matched the "bar" of "**/foo/bar"
        if (match)
        {
            int id_stack_size = window->IDStack.Size;
            if (label_task->InSuffixDepth < id_stack_size)
            {
                ImGuiID base_id = window->IDStack.Data[id_stack_size - label_task->InSuffixDepth - 1]; // base_id correspond to the "**"
                ImGuiID find_id = ImHashDecoratedPath(label_task->InSuffix, NULL, base_id);            // essentially compare the whole "foo/bar" thing.
                if (id == find_id)
                    label_task->OutItemId = id;
            }
        }
    }
}

// Forward core/user-land text to test log
void ImGuiTestEngineHook_Log(ImGuiContext* ui_ctx, const char* fmt, ...)
{
    ImGuiTestEngine* engine = (ImGuiTestEngine*)ui_ctx->TestEngine;

    va_list args;
    va_start(args, fmt);
    engine->TestContext->LogExV(ImGuiTestVerboseLevel_Debug, ImGuiTestLogFlags_None, fmt, args);
    va_end(args);
}

void ImGuiTestEngine_Assert(const char* expr, const char* file, const char* function, int line)
{
    ImGuiTestEngine* engine = GImGuiTestEngine;
    if (ImGuiTestContext* ctx = engine->TestContext)
    {
        ctx->LogError("Assert: '%s'", expr);
        ctx->LogWarning("In %s:%d, function %s()", file, line, function);
        if (ImGuiTest* test = ctx->Test)
            ctx->LogWarning("While running test: %s %s", test->Category, test->Name);
    }

    // Consider using github.com/scottt/debugbreak
    // FIXME: This should probably not happen in a user app?
    // FIXME: In our test app ideally we should break without an extra layer in call-stack.
#if __GNUC__
    // __builtin_trap() is not part of IM_DEBUG_BREAK() because GCC optimizes away everything that comes after.
    __builtin_trap();
#else
    IM_DEBUG_BREAK();
#endif
}

const char* ImGuiTestEngine_FindItemDebugLabel(ImGuiContext* ui_ctx, ImGuiID id)
{
    IM_ASSERT(ui_ctx->TestEngine != NULL);
    if (id == 0)
        return NULL;
    if (ImGuiTestItemInfo* id_info = ImGuiTestEngine_FindItemInfo((ImGuiTestEngine*)ui_ctx->TestEngine, id, ""))
        return id_info->DebugLabel;
    return NULL;
}

//-------------------------------------------------------------------------
// [SECTION] HOOKS FOR TESTS
//-------------------------------------------------------------------------
// - ImGuiTestEngine_Check()
// - ImGuiTestEngine_Error()
//-------------------------------------------------------------------------

// Return true to request a debugger break
bool ImGuiTestEngine_Check(const char* file, const char* func, int line, ImGuiTestCheckFlags flags, bool result, const char* expr)
{
    ImGuiTestEngine* engine = GImGuiTestEngine;
    (void)func;

    // Removed absolute path from output so we have deterministic output (otherwise __FILE__ gives us machine depending output)
    const char* file_without_path = file ? ImPathFindFilename(file) : "";

    if (ImGuiTestContext* ctx = engine->TestContext)
    {
        ImGuiTest* test = ctx->Test;
        //ctx->LogDebug("IM_CHECK(%s)", expr);
        if (!result)
        {
            if (!(ctx->RunFlags & ImGuiTestRunFlags_GuiFuncOnly))
                test->Status = ImGuiTestStatus_Error;

            if (file)
            {
                ctx->LogError("KO %s:%d '%s'", file_without_path, line, expr);
            }
            else
            {
                ctx->LogError("KO '%s'", expr);
            }

            // In case test failed without finishing gif capture - finish it here. It has to happen here because capture
            // args struct is on test function stack and would be lost when test function returns.
            if (engine->CaptureContext.IsCapturingGif())
            {
                ImGuiCaptureArgs* args = engine->CurrentCaptureArgs;
                ImGuiTestEngine_CaptureEndGif(engine, args);
                //ImFileDelete(args->OutSavedFileName);
            }
            ctx->ErrorCounter++;
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
        IM_ASSERT(0 && "No active tests!");
    }

    if (result == false && engine->IO.ConfigStopOnError && !engine->Abort)
        engine->Abort = true;
        //ImGuiTestEngine_Abort(engine);

    if (result == false && engine->IO.ConfigBreakOnError && !engine->Abort)
        return true;

    return false;
}

bool ImGuiTestEngine_Error(const char* file, const char* func, int line, ImGuiTestCheckFlags flags, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Str256 buf;
    buf.setfv(fmt, args);
    bool ret = ImGuiTestEngine_Check(file, func, line, flags, false, buf.c_str());
    va_end(args);

    ImGuiTestEngine* engine = GImGuiTestEngine;
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

static void     ImGuiTestEngine_SettingsReadLine(ImGuiContext* ui_ctx, ImGuiSettingsHandler*, void* entry, const char* line)
{
    ImGuiTestEngine* engine = (ImGuiTestEngine*)ui_ctx->TestEngine;
    IM_ASSERT(engine != NULL);
    IM_ASSERT(engine->UiContextTarget == ui_ctx);
    IM_UNUSED(entry);

    int n = 0;
    /**/ if (sscanf(line, "FilterTests=%s", engine->UiFilterTests.InputBuf) == 1)   { engine->UiFilterTests.Build(); }
    else if (sscanf(line, "FilterPerfs=%s", engine->UiFilterPerfs.InputBuf) == 1)   { engine->UiFilterPerfs.Build(); }
    else if (sscanf(line, "LogHeight=%f", &engine->UiLogHeight) == 1)               { }
    else if (sscanf(line, "CaptureTool=%d", &n) == 1)                               { engine->UiCaptureToolOpen = (n != 0); }
    else if (sscanf(line, "PerfTool=%d", &n) == 1)                                  { engine->UiPerfToolOpen = (n != 0); }
    else if (sscanf(line, "StackTool=%d", &n) == 1)                                 { engine->UiStackToolOpen = (n != 0); }
    else if (sscanf(line, "CaptureEnabled=%d", &n) == 1)                            { engine->IO.ConfigCaptureEnabled = (n != 0); }
    else if (sscanf(line, "CaptureOnError=%d", &n) == 1)                            { engine->IO.ConfigCaptureOnError = (n != 0); }
}

static void     ImGuiTestEngine_SettingsWriteAll(ImGuiContext* ui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    ImGuiTestEngine* engine = (ImGuiTestEngine*)ui_ctx->TestEngine;
    IM_ASSERT(engine != NULL);
    IM_ASSERT(engine->UiContextTarget == ui_ctx);

    buf->appendf("[%s][Data]\n", handler->TypeName);
    buf->appendf("FilterTests=%s\n", engine->UiFilterTests.InputBuf);
    buf->appendf("FilterPerfs=%s\n", engine->UiFilterPerfs.InputBuf);
    buf->appendf("LogHeight=%.0f\n", engine->UiLogHeight);
    buf->appendf("CaptureTool=%d\n", engine->UiCaptureToolOpen);
    buf->appendf("PerfTool=%d\n", engine->UiPerfToolOpen);
    buf->appendf("StackTool=%d\n", engine->UiStackToolOpen);
    buf->appendf("CaptureEnabled=%d\n", engine->IO.ConfigCaptureEnabled);
    buf->appendf("CaptureOnError=%d\n", engine->IO.ConfigCaptureOnError);
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
