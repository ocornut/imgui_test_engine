// dear imgui
// (test engine)

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

#define IMGUI_DEBUG_TEST_ENGINE     1

/*

Index of this file:

// [SECTION] DATA STRUCTURES
// [SECTION] TEST ENGINE: FORWARD DECLARATIONS
// [SECTION] TEST ENGINE: FUNCTIONS
// [SECTION] HOOKS FOR CORE LIBRARY
// [SECTION] HOOKS FOR TESTS
// [SECTION] USER INTERFACE
// [SECTION] ImGuiTestContext

*/

//-------------------------------------------------------------------------
// TODO
//-------------------------------------------------------------------------

// GOAL: Code coverage.
// GOAL: Custom testing.
// GOAL: Take screenshots for web/docs.
// GOAL: Reliable performance measurement (w/ deterministic setup)
// GOAL: Full blind version with no graphical context.

// FIXME-TESTS: threaded test func instead of intrusive yield
// FIXME-TESTS: UI to setup breakpoint on frame X (gui func or at yield time for test func)
// FIXME-TESTS: Locate within stack that uses windows/<pointer>/name
// FIXME-TESTS: Be able to run blind within GUI
// FIXME-TESTS: Be able to interactively run GUI function without Test function
// FIXME-TESTS: Provide variants of same test (e.g. run same tests with a bunch of styles/flags)
// FIXME-TESTS: Automate clicking/opening stuff based on gathering id?
// FIXME-TESTS: Mouse actions on ImGuiNavLayer_Menu layer
// FIXME-TESTS: Fail to open a double-click tree node

//-------------------------------------------------------------------------
// [SECTION] DATA STRUCTURES
//-------------------------------------------------------------------------

static ImGuiTestEngine* GImGuiHookingEngine = NULL;

// Helper to output a string showing the Path, ID or Debug Label based on what is available.
struct ImGuiTestRefDesc
{
    char Buf[40];

    const char* c_str() { return Buf; }
    ImGuiTestRefDesc(const ImGuiTestRef& ref, const ImGuiTestItemInfo* item = NULL)
    {
        if (ref.Path)
            ImFormatString(Buf, IM_ARRAYSIZE(Buf), "'%s' > %08X", ref.Path, ref.ID);
        else
            ImFormatString(Buf, IM_ARRAYSIZE(Buf), "%08X > '%s'", ref.ID, item ? item->DebugLabel : "NULL");
    }
};

// [Internal] Locate item position/window/state given ID.
struct ImGuiTestLocateTask
{
    ImGuiID                 ID = 0;
    int                     FrameCount = -1;        // Timestamp of request
    char                    DebugName[64] = "";
    ImGuiTestItemInfo       Result;
};

// [Internal] Gather items in given parent scope.
struct ImGuiTestGatherTask
{
    ImGuiID                 ParentID = 0;
    int                     Depth = 0;
    ImGuiTestItemList*      OutList = NULL;
    ImGuiTestItemInfo*      LastItemInfo = NULL;
};

enum ImGuiTestInputType
{
    ImGuiTestInputType_None,
    ImGuiTestInputType_Key,
    ImGuiTestInputType_Nav,
    ImGuiTestInputType_Char
};

struct ImGuiTestInput
{
    ImGuiTestInputType      Type;
    ImGuiKey                Key = ImGuiKey_COUNT;
    ImGuiKeyModFlags        KeyMods = ImGuiKeyModFlags_None;
    ImGuiNavInput           NavInput = ImGuiNavInput_COUNT;
    ImWchar                 Char = 0;
    bool                    IsDown = true;

    static ImGuiTestInput   FromKey(ImGuiKey v, bool down, ImGuiKeyModFlags mods = ImGuiKeyModFlags_None) 
    { 
        ImGuiTestInput inp; 
        inp.Type = ImGuiTestInputType_Key;
        inp.Key = v; 
        inp.IsDown = down; 
        inp.KeyMods = mods; 
        return inp; 
    }
    static ImGuiTestInput   FromNav(ImGuiNavInput v, bool down) 
    { 
        ImGuiTestInput inp; 
        inp.Type = ImGuiTestInputType_Nav;
        inp.NavInput = v;
        inp.IsDown = down; 
        return inp;
    }
    static ImGuiTestInput   FromChar(ImWchar v)
    { 
        ImGuiTestInput inp; 
        inp.Type = ImGuiTestInputType_Char;
        inp.Char = v;
        return inp; 
    }
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
    ImVector<ImGuiTest*>        TestsAll;
    ImVector<ImGuiTestRunTask>  TestsQueue;
    ImGuiTestContext*           TestContext = NULL;
    int                         CallDepth = 0;
    ImVector<ImGuiTestLocateTask*>  LocateTasks;
    ImGuiTestGatherTask         GatherTask;

    // Inputs
    bool                        InputMousePosSet = false;
    bool                        InputMouseButtonsSet = false;
    ImVec2                      InputMousePosValue;
    int                         InputMouseButtonsValue = 0x00;
    ImVector<ImGuiTestInput>    InputQueue;

    // UI support
    bool                        Abort = false;
    bool                        UiFocus = false;
    ImGuiTest*                  UiSelectedTest = NULL;

    // Build Info
    const char*                 InfoBuildType = "";
    const char*                 InfoBuildCpu = "";
    const char*                 InfoBuildOS = "";
    const char*                 InfoBuildCompiler = "";
    char                        InfoBuildDate[32];
    const char*                 InfoBuildTime = "";

    // Performance Monitor
    FILE*                       PerfPersistentLogCsv;
    double                      PerfRefDeltaTime;
    ImMovingAverage<double>     PerfDeltaTime100;
    ImMovingAverage<double>     PerfDeltaTime500;
    ImMovingAverage<double>     PerfDeltaTime1000;
    ImMovingAverage<double>     PerfDeltaTime2000;

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

static const char*  GetActionName(ImGuiTestAction action)
{
    switch (action)
    {
    case ImGuiTestAction_Unknown:       return "Unknown";
    case ImGuiTestAction_Click:         return "Click";
    case ImGuiTestAction_DoubleClick:   return "DoubleClick";
    case ImGuiTestAction_Check:         return "Check";
    case ImGuiTestAction_Uncheck:       return "Uncheck";
    case ImGuiTestAction_Open:          return "Open";
    case ImGuiTestAction_Close:         return "Close";
    case ImGuiTestAction_COUNT:
    default:                            return "N/A";
    }
}

static const char*  GetActionVerb(ImGuiTestAction action)
{
    switch (action)
    {
    case ImGuiTestAction_Unknown:       return "Unknown";
    case ImGuiTestAction_Click:         return "Clicked";
    case ImGuiTestAction_DoubleClick:   return "DoubleClicked";
    case ImGuiTestAction_Check:         return "Checked";
    case ImGuiTestAction_Uncheck:       return "Unchecked";
    case ImGuiTestAction_Open:          return "Opened";
    case ImGuiTestAction_Close:         return "Closed";
    case ImGuiTestAction_COUNT:
    default:                            return "N/A";
    }
}

static const char* GetVerboseLevelName(ImGuiTestVerboseLevel v)
{
    switch (v)
    {
    case ImGuiTestVerboseLevel_Silent:  return "Silent";
    case ImGuiTestVerboseLevel_Min:     return "Minimum";
    case ImGuiTestVerboseLevel_Normal:  return "Normal";
    case ImGuiTestVerboseLevel_Max:     return "Max";
    }
    return "N/A";
}

//-------------------------------------------------------------------------
// [SECTION] TEST ENGINE: FORWARD DECLARATIONS
//-------------------------------------------------------------------------

// Private functions
static void                 ImGuiTestEngine_SetupBuildInfo(ImGuiTestEngine* engine);
static ImGuiTestItemInfo*   ImGuiTestEngine_ItemLocate(ImGuiTestEngine* engine, ImGuiID id, const char* debug_id);
static void                 ImGuiTestEngine_ApplyInputToImGuiContext(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_ProcessQueue(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_ClearTests(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_ClearLocateTasks(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_PreNewFrame(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_PostNewFrame(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_Yield(ImGuiTestEngine* engine);

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
// - ImHashDecoratedPath()
// - ImGuiTestEngine_FindLocateTask()
// - ImGuiTestEngine_ItemLocate()
// - ImGuiTestEngine_ClearTests()
// - ImGuiTestEngine_ClearLocateTasks()
// - ImGuiTestEngine_ApplyInputToImGuiContext()
// - ImGuiTestEngine_PreNewFrame()
// - ImGuiTestEngine_PostNewFrame()
// - ImGuiTestEngine_Yield()
// - ImGuiTestEngine_ProcessQueue()
// - ImGuiTestEngine_QueueTest()
// - ImGuiTestEngine_RunTest()
//-------------------------------------------------------------------------

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

    ImGuiTestEngine_SetupBuildInfo(engine);

    engine->PerfPersistentLogCsv = fopen("imgui_perflog.csv", "a+t");

    return engine;
}

// Those strings are used to output easily identifiable markers in compare logs. We only need to support what we use for testing.
// We can probably grab info in eaplatform.h/eacompiler.h etc. in EASTL
void ImGuiTestEngine_SetupBuildInfo(ImGuiTestEngine* engine)
{
    // Build Type
#if defined(DEBUG) || defined(_DEBUG)
    engine->InfoBuildType = "Debug";
#else
    engine->InfoBuildType = "Release";
#endif

    // CPU
#if defined(_M_X86) || defined(_M_IX86) || defined(__i386) || defined(__i386__) || defined(_X86_) || defined(_M_AMD64) || defined(_AMD64_) || defined(__x86_64__)
    engine->InfoBuildCpu = (sizeof(size_t) == 4) ? "X86" : "X64";
#else
#error
    engine->InfoBuildCpu = (sizeof(size_t) == 4) ? "Unknown32" : "Unknown64";
#endif

    // Platform/OS
#if defined(_WIN32)
    engine->InfoBuildOS = "Windows";
#elif defined(__linux) || defined(__linux__)
    engine->InfoBuildOS = "Linux";
#elif defined(__MACH__) || (defined(__MSL__)
    engine->InfoBuildOS = "OSX";
#elif defined(__ORBIS__)
    engine->InfoBuildOS = "PS4";
#elif defined(_DURANGO)
    engine->InfoBuildOS = "XboxOne";
#else
    engine->InfoBuildOS = "Unknown";
#endif

    // Compiler
#if defined(_MSC_VER)
    engine->InfoBuildCompiler = "MSVC";
#elif defined(__clang__)
    engine->InfoBuildCompiler = "Clang";
#elif defined(__GNUC__)
    engine->InfoBuildCompiler = "GCC";
#else
    engine->InfoBuildCompiler = "Unknown";
#endif

    // Date/Time
    ImParseDateFromCompilerIntoYMD(__DATE__, engine->InfoBuildDate, IM_ARRAYSIZE(engine->InfoBuildDate));
    engine->InfoBuildTime = __TIME__;
}

void    ImGuiTestEngine_ShutdownContext(ImGuiTestEngine* engine)
{
    if (engine->PerfPersistentLogCsv)
        fclose(engine->PerfPersistentLogCsv);

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
    if (engine->TestContext)
        engine->TestContext->Abort = true;
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

static ImGuiTestItemInfo* ImGuiTestEngine_ItemLocate(ImGuiTestEngine* engine, ImGuiID id, const char* debug_id)
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
            ImFormatString(task->DebugName, IM_ARRAYSIZE(task->DebugName), "%.*s..%.*s", header_sz, debug_id, footer_sz, debug_id + debug_id_sz - footer_sz);
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

static void ImGuiTestEngine_ApplyInputToImGuiContext(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->UiContextTarget != NULL);
    ImGuiContext& g = *engine->UiContextTarget;
    g.IO.MouseDrawCursor = true;

    if (engine->UiContextActive != NULL && ImGuiTestEngine_IsRunningTests(engine))
    {
        IM_ASSERT(engine->TestContext != NULL);
        if (engine->TestContext->RunFlags & ImGuiTestRunFlags_NoTestFunc)
        {
            engine->InputQueue.resize(0);
            return;
        }

        //if (engine->SetMousePos)
        {
            g.IO.MousePos = engine->InputMousePosValue;
            g.IO.WantSetMousePos = true;
        }
        if (engine->InputMouseButtonsSet)
        {
            for (int n = 0; n < IM_ARRAYSIZE(g.IO.MouseDown); n++)
                g.IO.MouseDown[n] = (engine->InputMouseButtonsValue & (1 << n)) != 0;
        }
        engine->InputMousePosSet = false;
        engine->InputMouseButtonsSet = false;

        if (engine->InputQueue.Size > 0)
        {
            for (int n = 0; n < engine->InputQueue.Size; n++)
            {
                const ImGuiTestInput& input = engine->InputQueue[n];
                switch (input.Type)
                {
                case ImGuiTestInputType_Key:
                {
                    IM_ASSERT(input.Key != ImGuiKey_COUNT);
                    g.IO.KeyCtrl  = input.IsDown && (input.KeyMods & ImGuiKeyModFlags_Ctrl) != 0;
                    g.IO.KeyAlt   = input.IsDown && (input.KeyMods & ImGuiKeyModFlags_Alt)  != 0;
                    g.IO.KeyShift = input.IsDown && (input.KeyMods & ImGuiKeyModFlags_Shift) != 0;
                    g.IO.KeySuper = input.IsDown && (input.KeyMods & ImGuiKeyModFlags_Super) != 0;
                    int idx = g.IO.KeyMap[input.Key];
                    if (idx >= 0 && idx < IM_ARRAYSIZE(g.IO.KeysDown))
                        g.IO.KeysDown[idx] = input.IsDown;
                    break;
                }
                case ImGuiTestInputType_Nav:
                {
                    IM_ASSERT(input.NavInput >= 0 && input.NavInput < ImGuiNavInput_COUNT);
                    g.IO.NavInputs[input.NavInput] = input.IsDown;
                    break;
                }
                case ImGuiTestInputType_Char:
                {
                    IM_ASSERT(input.Char != 0);
                    g.IO.AddInputCharacter(input.Char);
                    break;
                }
                }
            }
            engine->InputQueue.resize(0);
        }
    }
    else
    {
        IM_ASSERT(engine->UiContextActive == NULL);
        engine->InputMousePosValue = g.IO.MousePos;
    }
}

static void ImGuiTestEngine_PreNewFrame(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->UiContextTarget != NULL);
    IM_ASSERT(engine->UiContextTarget == GImGui);
    ImGuiContext& g = *engine->UiContextTarget;

    engine->FrameCount = g.FrameCount + 1;  // NewFrame() will increase this.
    if (engine->TestContext)
        engine->TestContext->FrameCount++;

    engine->PerfDeltaTime100.AddSample(g.IO.DeltaTime);
    engine->PerfDeltaTime500.AddSample(g.IO.DeltaTime);
    engine->PerfDeltaTime1000.AddSample(g.IO.DeltaTime);
    engine->PerfDeltaTime2000.AddSample(g.IO.DeltaTime);

    if (ImGuiTestEngine_IsRunningTests(engine) && !engine->Abort)
    {
        // Abort testing by holding ESC for 0.3 seconds.
        // This is so ESC injected by tests don't interfer when sharing UI context.
        const int key_idx_escape = g.IO.KeyMap[ImGuiKey_Escape];
        if (key_idx_escape != -1 && g.IO.KeysDown[key_idx_escape] && g.IO.KeysDownDuration[key_idx_escape] >= 0.30f)
        {
            if (engine->TestContext)
                engine->TestContext->Log("KO: User aborted (pressed ESC)\n");
            ImGuiTestEngine_Abort(engine);
        }
    }

    ImGuiTestEngine_ApplyInputToImGuiContext(engine);
}

static void ImGuiTestEngine_PostNewFrame(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->UiContextTarget == GImGui);

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

    // Process on-going queues
    if (engine->CallDepth == 0)
        ImGuiTestEngine_ProcessQueue(engine);
}

static void ImGuiTestEngine_Yield(ImGuiTestEngine* engine)
{
    ImGuiContext& g = *engine->TestContext->UiContext;

    if (g.FrameScopeActive)
        engine->IO.EndFrameFunc(engine, engine->IO.UserData);

    engine->IO.NewFrameFunc(engine, engine->IO.UserData);
    IM_ASSERT(g.IO.DeltaTime > 0.0f);

    if (!g.FrameScopeActive)
        return;

    if (engine->TestContext && engine->TestContext->Test->GuiFunc)
    {
        // Call user GUI function
        if (!(engine->TestContext->RunFlags & ImGuiTestRunFlags_NoGuiFunc))
            engine->TestContext->Test->GuiFunc(engine->TestContext);

        // Safety net
        if (engine->TestContext->Test->Status == ImGuiTestStatus_Error)
        {
            while (g.CurrentWindowStack.Size > 1) // FIXME-ERRORHANDLING
                ImGui::End();
        }
    }
}

static void ImGuiTestEngine_ProcessQueue(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->CallDepth == 0);
    engine->CallDepth++;

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

        // If the user defines a RootFunc, they will call RunTest() themselves. This allows them to conveniently store data in the stack.
        ImGuiTestContext ctx;
        ctx.Test = test;
        ctx.Engine = engine;
        ctx.UserData = NULL;
        ctx.UserCounter = 0;
        ctx.UiContext = engine->UiContextActive;
        ctx.PerfStressAmount = engine->IO.PerfStressAmount;
        ctx.RunFlags = run_task->RunFlags;
        engine->TestContext = &ctx;

        ctx.Log("----------------------------------------------------------------------\n");
        ctx.Log("Test: '%s' '%s'..\n", test->Category, test->Name);
        if (test->RootFunc)
        {
            test->RootFunc(&ctx);
        }
        else
        {
            ctx.RunCurrentTest(NULL);
        }
        ran_tests++;

        IM_ASSERT(test->Status != ImGuiTestStatus_Running);
        if (engine->Abort && test->Status != ImGuiTestStatus_Error)
            test->Status = ImGuiTestStatus_Unknown;

        if (test->Status == ImGuiTestStatus_Success)
        {
            if ((ctx.RunFlags & ImGuiTestRunFlags_NoSuccessMsg) == 0)
                ctx.Log("Success.\n");
        }
        else if (engine->Abort)
            ctx.Log("Aborted.\n");
        else if (test->Status == ImGuiTestStatus_Error)
            ctx.Log("Error.\n");
        else
            ctx.Log("Unknown status.\n");

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

void ImGuiTestEngine_QueueTests(ImGuiTestEngine* engine, const char* filter)
{
    if (filter != NULL && filter[0] == 0)
        filter = NULL;
    for (int n = 0; n < engine->TestsAll.Size; n++)
    {
        ImGuiTest* test = engine->TestsAll[n];
        if (filter != NULL)
            if (ImStristr(test->Name, NULL, filter, NULL) == NULL)
                continue;
        ImGuiTestEngine_QueueTest(engine, test, ImGuiTestRunFlags_None);
    }
}

void ImGuiTestEngine_CalcSourceLineEnds(ImGuiTestEngine* engine)
{
    if (engine->TestsAll.empty())
        return;

    ImVector<int> line_starts;
    line_starts.resize(engine->TestsAll.Size, 0);
    for (int n = 0; n < engine->TestsAll.Size; n++)
        line_starts.push_back(engine->TestsAll[n]->SourceLine);
    ImQsort(line_starts.Data, (size_t)line_starts.Size, sizeof(int), [](const void* lhs, const void* rhs) { return (*(const int*)lhs) - *(const int*)rhs; });

    for (int n = 0; n < engine->TestsAll.Size; n++)
    {
        ImGuiTest* test = engine->TestsAll[n];
        for (int m = 0; m < line_starts.Size; m++) // FIXME-OPT
            if (line_starts[m] == test->SourceLine)
                test->SourceLineEnd = ImMax(test->SourceLine, (m + 1 < line_starts.Size) ? line_starts[m + 1] - 1 : 99999);
    }
}

void ImGuiTestEngine_PrintResultSummary(ImGuiTestEngine* engine)
{
    int count_tested = 0;
    int count_success = 0;
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
    printf("\nTests Result: %s\n(%d/%d tests passed)\n", (count_success == count_tested) ? "OK" : "KO", count_success, count_tested);
}

static void ImGuiTestEngine_RunTest(ImGuiTestEngine* engine, ImGuiTestContext* ctx, void* user_data)
{
    ImGuiTest* test = ctx->Test;
    ctx->Engine = engine;
    ctx->EngineIO = &engine->IO;
    ctx->UserData = user_data;
    ctx->UserCounter = 0;
    ctx->FrameCount = 0;
    ctx->SetRef("");
    ctx->SetInputMode(ImGuiInputSource_Mouse);
    ctx->GenericState.clear();

    test->TestLog.clear();

    if (!(test->Flags & ImGuiTestFlags_NoWarmUp))
    {
        ctx->FrameCount -= 2;
        ctx->Yield();
        ctx->Yield();
    }

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
            test->TestFunc(ctx);
        }
        else
        {
            // No test function
            if (test->Flags & ImGuiTestFlags_NoAutoFinish)
                while (!engine->Abort && test->Status == ImGuiTestStatus_Running)
                    ctx->Yield();
        }

        if (!ctx->Engine->IO.ConfigRunFast)
            ctx->SleepShort();

        while (ctx->Engine->IO.ConfigKeepTestGui && !engine->Abort)
        {
            ctx->RunFlags |= ImGuiTestRunFlags_NoTestFunc;
            ctx->Yield();
        }
    }

    // Additional yield is currently _only_ so the Log gets scrolled to the bottom on the last frame of the log output.
    ctx->Yield();

    if (test->Status == ImGuiTestStatus_Running)
        test->Status = ImGuiTestStatus_Success;
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
        if (engine->UiContextTarget == ctx)
            ImGuiTestEngine_PreNewFrame(engine);
}

void ImGuiTestEngineHook_PostNewFrame(ImGuiContext* ctx)
{
    if (ImGuiTestEngine* engine = GImGuiHookingEngine)
        if (engine->UiContextTarget == ctx)
            ImGuiTestEngine_PostNewFrame(engine);
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
        item->Rect = bb;
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
            item_info->Rect = bb;
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

//-------------------------------------------------------------------------
// [SECTION] HOOKS FOR TESTS
//-------------------------------------------------------------------------
// - ImGuiTestEngineHook_Check()
// - ImGuiTestEngineHook_Error()
//-------------------------------------------------------------------------

// Return true to request a debugger break
bool ImGuiTestEngineHook_Check(const char* file, const char* func, int line, bool result, const char* expr)
{
    ImGuiTestEngine* engine = GImGuiHookingEngine;
    (void)func;

    // Removed absolute path from output so we have deterministic output (otherwise __FILE__ gives us machine depending output)
    const char* file_without_path = file ? (file + strlen(file)) : NULL;
    while (file_without_path > file && file_without_path[-1] != '/' && file_without_path[-1] != '\\')
        file_without_path--;

    if (result == false)
    {
        ImOsConsoleSetTextColor(ImOsConsoleTextColor_BrightRed);
        if (file)
            printf("KO: %s:%d  '%s'\n", file_without_path, line, expr);
        else
            printf("KO: %s\n", expr);
        ImOsConsoleSetTextColor(ImOsConsoleTextColor_White);
    }
    
    if (ImGuiTestContext* ctx = engine->TestContext)
    {
        ImGuiTest* test = ctx->Test;
        //ctx->LogVerbose("IM_CHECK(%s)\n", expr);

        if (result)
        {
            if (file)
                test->TestLog.appendf("[%04d] OK %s:%d  '%s'\n", ctx->FrameCount, file_without_path, line, expr);
            else
                test->TestLog.appendf("[%04d] OK  '%s'\n", ctx->FrameCount, expr);
        }
        else
        {
            test->Status = ImGuiTestStatus_Error;
            if (file)
                test->TestLog.appendf("[%04d] KO %s:%d  '%s'\n", ctx->FrameCount, file_without_path, line, expr);
            else
                test->TestLog.appendf("[%04d] KO  '%s'\n", ctx->FrameCount, expr);
        }
    }
    else
    {
        printf("Error: no active test!\n");
        IM_ASSERT(0);
    }

    if (result == false && engine->IO.ConfigStopOnError && !engine->Abort)
        engine->Abort = true;
        //ImGuiTestEngine_Abort(engine);

    if (result == false && engine->IO.ConfigBreakOnError && !engine->Abort)
        return true;

    return false;
}

bool ImGuiTestEngineHook_Error(const char* file, const char* func, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[256];
    ImFormatStringV(buf, IM_ARRAYSIZE(buf), fmt, args);
    bool ret = ImGuiTestEngineHook_Check(file, func, line, false, buf);
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

static void DrawTestLog(ImGuiTestEngine* e, ImGuiTest* test, bool is_interactive)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 2.0f));
    ImU32 error_col = IM_COL32(255, 150, 150, 255);
    ImU32 unimportant_col = IM_COL32(190, 190, 190, 255);

    const char* text = test->TestLog.begin();
    const char* text_end = test->TestLog.end();
    int line_no = 0;
    while (text < text_end)
    {
        const char* line_start = text;
        const char* line_end = ImStreolRange(line_start, text_end);
        const bool is_error = ImStristr(line_start, line_end, "] KO", NULL) != NULL; // FIXME-OPT
        const bool is_unimportant = ImStristr(line_start, line_end, "] --", NULL) != NULL; // FIXME-OPT

        if (is_error)
            ImGui::PushStyleColor(ImGuiCol_Text, error_col);
        else if (is_unimportant)
            ImGui::PushStyleColor(ImGuiCol_Text, unimportant_col);
        ImGui::TextUnformatted(line_start, line_end);
        if (is_error || is_unimportant)
            ImGui::PopStyleColor();

        text = line_end + 1;
        line_no++;
    }
    ImGui::PopStyleVar();
}

void    ImGuiTestEngine_ShowTestWindow(ImGuiTestEngine* engine, bool* p_open)
{
    ImGuiContext& g = *GImGui;

    if (engine->UiFocus)
    {
        ImGui::SetNextWindowFocus();
        engine->UiFocus = false;
    }
    ImGui::Begin("ImGui Test Engine", p_open);// , ImGuiWindowFlags_MenuBar);

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

    const float log_height = ImGui::GetTextLineHeight() * 20.0f;
    const ImU32 col_highlight = IM_COL32(255, 255, 20, 255);

    // Options
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    //ImGui::Text("OPTIONS:");
    //ImGui::SameLine();
    ImGui::Checkbox("Fast", &engine->IO.ConfigRunFast);
    ImGui::SameLine();
    ImGui::Checkbox("Blind", &engine->IO.ConfigRunBlind);
    ImGui::SameLine();
    ImGui::Checkbox("Stop", &engine->IO.ConfigStopOnError);
    ImGui::SameLine();
    ImGui::Checkbox("DbgBrk", &engine->IO.ConfigBreakOnError);
    ImGui::SameLine();
    ImGui::Checkbox("KeepGUI", &engine->IO.ConfigKeepTestGui);
    ImGui::SameLine();
    ImGui::Checkbox("Focus", &engine->IO.ConfigTakeFocusBackAfterTests);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Set focus back to Test window after running tests.");
    ImGui::SameLine();
    ImGui::PushItemWidth(60);
    ImGui::DragInt("Verbose", (int*)&engine->IO.ConfigVerboseLevel, 0.1f, 0, ImGuiTestVerboseLevel_COUNT - 1, GetVerboseLevelName(engine->IO.ConfigVerboseLevel));
    ImGui::PopItemWidth();
    //ImGui::Checkbox("Verbose", &engine->IO.ConfigLogVerbose);
    ImGui::SameLine();
    ImGui::PushItemWidth(30);
    ImGui::DragInt("Perf Stress Amount", &engine->IO.PerfStressAmount, 0.1f, 1, 20);
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
    ImGui::Separator();

    // TESTS
    if (ImGui::BeginTabBar("##blah", ImGuiTabBarFlags_NoTooltip))
    {
        char tab_label[32];
        ImFormatString(tab_label, IM_ARRAYSIZE(tab_label), "TESTS (%d)###TESTS", engine->TestsAll.Size);
        ImGui::BeginTabItem(tab_label);

        //ImGui::Text("TESTS (%d)", engine->TestsAll.Size);
        static ImGuiTextFilter filter;
        if (ImGui::Button("Run All"))
            ImGuiTestEngine_QueueTests(engine, filter.InputBuf); // FIXME: Filter func differs

        ImGui::SameLine();
        filter.Draw("##filter", -1.0f);
        ImGui::Separator();
        ImGui::BeginChild("Tests", ImVec2(0, -log_height));

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 3));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 1));
        for (int n = 0; n < engine->TestsAll.Size; n++)
        {
            ImGuiTest* test = engine->TestsAll[n];
            if (!filter.PassFilter(test->Name) && !filter.PassFilter(test->Category))
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
                ImGui::RenderText(p + g.Style.FramePadding + ImVec2(0, 0), "|\0/\0-\0\\" + (((g.FrameCount / 5) & 3) << 1), NULL);

            bool queue_test = false;
            bool queue_gui_func = false;
            bool select_test = false;

            if (ImGui::Button("Run"))
                queue_test = select_test = true;
            ImGui::SameLine();

            char buf[128];
            ImFormatString(buf, IM_ARRAYSIZE(buf), "%-*s - %s", 10, test->Category, test->Name);
            if (ImGui::Selectable(buf, test == engine->UiSelectedTest))
                select_test = true;

            /*if (ImGui::IsItemHovered() && test->TestLog.size() > 0)
            {
                ImGui::BeginTooltip();
                DrawTestLog(engine, test, false);
                ImGui::EndTooltip();
            }*/

            bool view_source = false;
            if (ImGui::BeginPopupContextItem())
            {
                select_test = true;

                const bool open_source_available = (test->SourceFile != NULL) && (engine->IO.FileOpenerFunc != NULL);
                if (open_source_available)
                {
                    ImFormatString(buf, IM_ARRAYSIZE(buf), "Open source (%s:%d)", test->SourceFileShort, test->SourceLine);
                    if (ImGui::MenuItem(buf))
                    {
                        engine->IO.FileOpenerFunc(test->SourceFile, test->SourceLine, engine->IO.UserData);
                    }
                    if (ImGui::MenuItem("View source..."))
                        view_source = true;
                }
                else
                {
                    ImGui::MenuItem("Open source", NULL, false, false);
                    ImGui::MenuItem("View source", NULL, false, false);
                }

                if (ImGui::MenuItem("Copy log", NULL, false, !test->TestLog.empty()))
                    ImGui::SetClipboardText(test->TestLog.c_str());

                if (ImGui::MenuItem("Clear log", NULL, false, !test->TestLog.empty()))
                    test->TestLog.clear();

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
                    ImGuiTestEngine_QueueTest(engine, test, ImGuiTestRunFlags_None);
                else if (queue_gui_func)
                    ImGuiTestEngine_QueueTest(engine, test, ImGuiTestRunFlags_NoTestFunc);
            }

            ImGui::PopID();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::EndTabItem();
        ImGui::EndTabBar();
    }

    // LOG
    ImGui::Separator();

    if (ImGui::BeginTabBar("##tools"))
    {
        if (ImGui::BeginTabItem("LOG"))
        {
            if (engine->UiSelectedTest)
                ImGui::Text("Log for %s: %s", engine->UiSelectedTest->Category, engine->UiSelectedTest->Name);
            else
                ImGui::Text("N/A");
            if (ImGui::SmallButton("Copy to clipboard"))
                if (engine->UiSelectedTest)
                    ImGui::SetClipboardText(engine->UiSelectedTest->TestLog.begin());
            ImGui::Separator();

            // Quick status
            ImGuiContext* ui_context = engine->UiContextActive ? engine->UiContextActive : engine->UiContextVisible;
            ImGuiID item_hovered_id = ui_context->HoveredIdPreviousFrame;
            ImGuiID item_active_id = ui_context->ActiveId;
            ImGuiTestItemInfo* item_hovered_info = item_hovered_id ? ImGuiTestEngine_ItemLocate(engine, item_hovered_id, "") : NULL;
            ImGuiTestItemInfo* item_active_info = item_active_id ? ImGuiTestEngine_ItemLocate(engine, item_active_id, "") : NULL;
            ImGui::Text("Hovered: 0x%08X (\"%s\")", item_hovered_id, item_hovered_info ? item_hovered_info->DebugLabel : "");
            ImGui::Text("Active:  0x%08X (\"%s\")", item_active_id, item_active_info ? item_active_info->DebugLabel : "");

            ImGui::Separator();
            ImGui::BeginChild("Log");
            if (engine->UiSelectedTest)
            {
                DrawTestLog(engine, engine->UiSelectedTest, true);
                if (ImGuiTestEngine_IsRunningTests(engine))
                    ImGui::SetScrollHereY();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // FIXME-TESTS: Need to be visualizing the samples/spikes.
        if (ImGui::BeginTabItem("PERF"))
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

    ImGui::End();
}

//-------------------------------------------------------------------------
// [SECTION] ImGuiTestContext
// This is the interface that most tests will interact with.
//-------------------------------------------------------------------------

ImGuiTest*  ImGuiTestContext::RegisterTest(const char* category, const char* name, const char* src_file, int src_line)
{
    ImGuiTest* t = IM_NEW(ImGuiTest)();
    t->Category = category;
    t->Name = name;
    t->SourceFile = t->SourceFileShort = src_file;
    t->SourceLine = t->SourceLineEnd = src_line;
    Engine->TestsAll.push_back(t);

    // Find filename only out of the fully qualified source path
    if (src_file)
    {
        // FIXME: Be reasonable and use a named helper.
        for (t->SourceFileShort = t->SourceFile + strlen(t->SourceFile); t->SourceFileShort > t->SourceFile; t->SourceFileShort--)
            if (t->SourceFileShort[-1] == '/' || t->SourceFileShort[-1] == '\\')
                break;
    }

    return t;
}

void	ImGuiTestContext::RunCurrentTest(void* user_data)
{
    ImGuiTestEngine_RunTest(Engine, this, user_data);
}

void    ImGuiTestContext::Log(const char* fmt, ...)
{
    ImGuiTestContext* ctx = Engine->TestContext;
    ImGuiTest* test = ctx->Test;

    const int prev_size = test->TestLog.size();

    va_list args;
    va_start(args, fmt);
    test->TestLog.appendf("[%04d] ", ctx->FrameCount);
    test->TestLog.appendfv(fmt, args);
    va_end(args);

    if (Engine->IO.ConfigLogToTTY)
        printf("%s", test->TestLog.c_str() + prev_size);
}

void    ImGuiTestContext::LogVerbose(const char* fmt, ...)
{
    if (Engine->IO.ConfigVerboseLevel <= ImGuiTestVerboseLevel_Min)
        return;

    ImGuiTestContext* ctx = Engine->TestContext;
    if (Engine->IO.ConfigVerboseLevel == ImGuiTestVerboseLevel_Normal && ctx->ActionDepth > 1)
        return;

    ImGuiTest* test = ctx->Test;

    const int prev_size = test->TestLog.size();

    va_list args;
    va_start(args, fmt);
    test->TestLog.appendf("[%04d] -- %*s", ctx->FrameCount, ImMax(0, (ctx->ActionDepth - 1) * 2), "");
    test->TestLog.appendfv(fmt, args);
    va_end(args);

    if (Engine->IO.ConfigLogToTTY)
        printf("%s", test->TestLog.c_str() + prev_size);
}

void    ImGuiTestContext::LogDebug()
{
    ImGuiID item_hovered_id = UiContext->HoveredIdPreviousFrame;
    ImGuiID item_active_id = UiContext->ActiveId;
    ImGuiTestItemInfo* item_hovered_info = item_hovered_id ? ImGuiTestEngine_ItemLocate(Engine, item_hovered_id, "") : NULL;
    ImGuiTestItemInfo* item_active_info = item_active_id ? ImGuiTestEngine_ItemLocate(Engine, item_active_id, "") : NULL;
    LogVerbose("Hovered: 0x%08X (\"%s\"), Active:  0x%08X(\"%s\")\n", 
        item_hovered_id, item_hovered_info ? item_hovered_info->DebugLabel : "",
        item_active_id, item_active_info ? item_active_info->DebugLabel : "");
}

void    ImGuiTestContext::Finish()
{
    if (RunFlags & ImGuiTestRunFlags_NoTestFunc)
        return;
    ImGuiTest* test = Test;
    if (test->Status == ImGuiTestStatus_Running)
        test->Status = ImGuiTestStatus_Success;
}

void    ImGuiTestContext::Yield()
{
    ImGuiTestEngine_Yield(Engine);
}

void    ImGuiTestContext::YieldFrames(int count = 0)
{
    while (count > 0)
    {
        ImGuiTestEngine_Yield(Engine);
        count--;
    }
}

void    ImGuiTestContext::YieldUntil(int frame_count)
{
    while (FrameCount < frame_count)
        ImGuiTestEngine_Yield(Engine);
}

void    ImGuiTestContext::Sleep(float time)
{
    if (IsError())
        return;

    if (Engine->IO.ConfigRunFast)
    {
        ImGuiTestEngine_Yield(Engine);
    }
    else
    {
        while (time > 0.0f && !Engine->Abort)
        {
            ImGuiTestEngine_Yield(Engine);
            time -= UiContext->IO.DeltaTime;
        }
    }
}

void    ImGuiTestContext::SleepShort()
{
    Sleep(0.2f);
}

void ImGuiTestContext::SetInputMode(ImGuiInputSource input_mode)
{
    IM_ASSERT(input_mode == ImGuiInputSource_Mouse || input_mode == ImGuiInputSource_Nav);
    InputMode = input_mode;

    if (InputMode == ImGuiInputSource_Nav)
    {
        UiContext->NavDisableHighlight = false;
        UiContext->NavDisableMouseHover = true;
    }
    else
    {
        UiContext->NavDisableHighlight = true;
        UiContext->NavDisableMouseHover = false;
    }
}

void ImGuiTestContext::SetRef(ImGuiTestRef ref)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("SetRef '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

    if (ref.Path)
    {
        size_t len = strlen(ref.Path);
        IM_ASSERT(len < IM_ARRAYSIZE(RefStr) - 1);

        strcpy(RefStr, ref.Path);
        RefID = ImHashDecoratedPath(ref.Path, 0);
    }
    else
    {
        RefStr[0] = 0;
        RefID = ref.ID;
    }
}

ImGuiWindow* ImGuiTestContext::GetRefWindow()
{
    ImGuiID window_id = GetID("");
    ImGuiWindow* window = ImGui::FindWindowByID(window_id);
    return window;
}

ImGuiID ImGuiTestContext::GetID(ImGuiTestRef ref)
{
    if (ref.ID)
        return ref.ID;
    return ImHashDecoratedPath(ref.Path, RefID);
}

ImGuiTestRef ImGuiTestContext::GetFocusWindowRef()
{
    ImGuiContext& g = *UiContext;
    return g.NavWindow ? g.NavWindow->ID : 0;
}

ImVec2 ImGuiTestContext::GetMainViewportPos()
{
#ifdef IMGUI_HAS_VIEWPORT
    return ImGui::GetMainViewport()->Pos;
#else
    return ImVec2(0, 0);
#endif
}

ImGuiTestItemInfo* ImGuiTestContext::ItemLocate(ImGuiTestRef ref, ImGuiTestOpFlags flags)
{
    if (IsError())
        return NULL;

    ImGuiID full_id;
    if (ref.ID)
        full_id = ref.ID;
    else
        full_id = ImHashDecoratedPath(ref.Path, RefID);

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestItemInfo* item = NULL;
    int retries = 0;
    while (retries < 2)
    {
        item = ImGuiTestEngine_ItemLocate(Engine, full_id, ref.Path);
        if (item)
            return item;
        ImGuiTestEngine_Yield(Engine);
        retries++;
    }

    if (!(flags & ImGuiTestOpFlags_NoError))
    {
        // Prefixing the string with / ignore the reference/current ID
        if (ref.Path && ref.Path[0] == '/' && RefStr[0] != 0)
            IM_ERRORF_NOHDR("Unable to locate item: '%s'", ref.Path);
        else if (ref.Path)
            IM_ERRORF_NOHDR("Unable to locate item: '%s/%s'", RefStr, ref.Path);
        else
            IM_ERRORF_NOHDR("Unable to locate item: 0x%08X", ref.ID);
    }

    return NULL;
}

// FIXME-TESTS: scroll_ratio_y unsupported
void    ImGuiTestContext::ScrollToY(ImGuiTestRef ref, float scroll_ratio_y)
{
    if (IsError())
        return;

    // If the item is not currently visible, scroll to get it in the center of our window
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiContext& g = *UiContext;
    ImGuiTestItemInfo* item = ItemLocate(ref);
    ImGuiTestRefDesc desc(ref, item);
    LogVerbose("ScrollToY %s\n", desc.c_str());

    ImGuiWindow* window = item->Window;

    //if (item->ID == 0xDFFBB0CE || item->ID == 0x87CBBA09)
    //    printf("[%03d] scrollmax %f\n", FrameCount, ImGui::GetWindowScrollMaxY(window));

    while (!Abort)
    {
        // result->Rect will be updated after each iteration.
        ImRect item_rect = item->Rect;
        float item_curr_y = ImFloor(item_rect.GetCenter().y);
        float item_target_y = ImFloor(window->InnerClipRect.GetCenter().y);
        float scroll_delta_y = item_target_y - item_curr_y;
        float scroll_target = ImClamp(window->Scroll.y - scroll_delta_y, 0.0f, ImGui::GetWindowScrollMaxY(window));

        if (ImFabs(window->Scroll.y - scroll_target) < 1.0f)
            break;

        float scroll_speed = Engine->IO.ConfigRunFast ? FLT_MAX : ImFloor(Engine->IO.ScrollSpeed * g.IO.DeltaTime + 0.99f);
        //printf("[%03d] window->Scroll.y %f + %f\n", FrameCount, window->Scroll.y, scroll_speed);
        window->Scroll.y = ImLinearSweep(window->Scroll.y, scroll_target, scroll_speed);

        Yield();

        BringWindowToFrontFromItem(ref);
    }

    // Need another frame for the result->Rect to stabilize
    Yield();
}

void    ImGuiTestContext::NavMove(ImGuiTestRef ref)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiContext& g = *UiContext;
    ImGuiTestItemInfo* item = ItemLocate(ref);
    ImGuiTestRefDesc desc(ref, item);
    LogVerbose("NavMove to %s\n", desc.c_str());

    if (item == NULL)
        return;
    item->RefCount++;

    // Focus window before scrolling/moving so things are nicely visible
    BringWindowToFrontFromItem(ref);

    ImGui::SetNavID(item->ID, item->NavLayer);
    Yield();

    if (!Abort)
    {
        if (g.NavId != item->ID)
            IM_ERRORF_NOHDR("Unable to set NavId to %s", desc.c_str());
    }

    item->RefCount--;
}

void    ImGuiTestContext::NavActivate()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("NavActivate\n");

    Yield();

    int frames_to_hold = 2; // FIXME-TESTS: <-- number of frames could be fuzzed here
#if 1
    // Feed gamepad nav inputs
    for (int n = 0; n < frames_to_hold; n++)
    {
        Engine->InputQueue.push_back(ImGuiTestInput::FromNav(ImGuiNavInput_Activate, true));
        Yield();
    }
    Yield();
#else
    // Feed keyboard keys
    Engine->InputQueue.push_back(ImGuiTestInput::FromKey(ImGuiKey_Space, true));
    for (int n = 0; n < frames_to_hold; n++)
        Yield();
    Engine->InputQueue.push_back(ImGuiTestInput::FromKey(ImGuiKey_Space, false));
    Yield();
    Yield();
#endif
}

void    ImGuiTestContext::NavInput()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("NavInput\n");

    int frames_to_hold = 2; // FIXME-TESTS: <-- number of frames could be fuzzed here
    for (int n = 0; n < frames_to_hold; n++)
    {
        Engine->InputQueue.push_back(ImGuiTestInput::FromNav(ImGuiNavInput_Input, true));
        Yield();
    }
    Yield();
}

void    ImGuiTestContext::MouseMove(ImGuiTestRef ref, ImGuiTestOpFlags flags)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiContext& g = *UiContext;
    ImGuiTestItemInfo* item = ItemLocate(ref);
    ImGuiTestRefDesc desc(ref, item);
    LogVerbose("MouseMove to %s\n", desc.c_str());

    if (item == NULL)
        return;
    item->RefCount++;

    // Focus window before scrolling/moving so things are nicely visible
    BringWindowToFrontFromItem(ref);

    ImGuiWindow* window = item->Window;
    if (item->NavLayer == ImGuiNavLayer_Main && !window->InnerClipRect.Contains(item->Rect))
        ScrollToY(ref);

    ImVec2 pos = item->Rect.GetCenter();
    ImRect visible_r(0.0f, 0.0f, g.IO.DisplaySize.x, g.IO.DisplaySize.y);   // FIXME: Viewport
    if (!visible_r.Contains(pos))
    {
        // Fallback move window directly to make our item reachable with the mouse.
        float pad = g.FontSize;
        ImVec2 delta;
        delta.x = (pos.x < visible_r.Min.x) ? (visible_r.Min.x - pos.x + pad) : (pos.x > visible_r.Max.x) ? (visible_r.Max.x - pos.x - pad) : 0.0f;
        delta.y = (pos.y < visible_r.Min.y) ? (visible_r.Min.y - pos.y + pad) : (pos.y > visible_r.Max.y) ? (visible_r.Max.y - pos.y - pad) : 0.0f;
        ImGui::SetWindowPos(window, window->Pos + delta, ImGuiCond_Always);
        LogVerbose("WindowMoveBypass %s delta (%.1f,%.1f)\n", window->Name, delta.x, delta.y);
        Yield();
        pos = item->Rect.GetCenter();
    }

    MouseMoveToPos(pos);

    // Focus again in case something made us lost focus (which could happen on a simple hover)
    BringWindowToFrontFromItem(ref);// , ImGuiTestOpFlags_Verbose);

    if (!Abort && !(flags & ImGuiTestOpFlags_NoCheckHoveredId))
    {
        if (g.HoveredIdPreviousFrame != item->ID)
            IM_ERRORF_NOHDR("Unable to Hover %s. Expected %08X, HoveredId was %08X.", desc.c_str(), item->ID, g.HoveredIdPreviousFrame);
    }

    item->RefCount--;
}

void	ImGuiTestContext::MouseMoveToPos(ImVec2 target)
{
    ImGuiContext& g = *UiContext;

    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MouseMove from (%.0f,%.0f) to (%.0f,%.0f)\n", Engine->InputMousePosValue.x, Engine->InputMousePosValue.y, target.x, target.y);

    // We manipulate Engine->SetMousePosValue without reading back from g.IO.MousePos because the later is rounded.
    // To handle high framerate it is easier to bypass this rounding.
    while (true)
    {
        ImVec2 delta = target - Engine->InputMousePosValue;
        float inv_length = ImInvLength(delta, FLT_MAX);
        float move_speed = Engine->IO.MouseSpeed * g.IO.DeltaTime;
        if (g.IO.KeyShift)
            move_speed *= 0.1f;

        //ImGui::GetOverlayDrawList()->AddCircle(target, 10.0f, IM_COL32(255, 255, 0, 255));

        if (Engine->IO.ConfigRunFast || (inv_length >= 1.0f / move_speed))
        {
            Engine->InputMousePosSet = true;
            Engine->InputMousePosValue = target;
            ImGuiTestEngine_Yield(Engine);
            ImGuiTestEngine_Yield(Engine);
            return;
        }
        else
        {
            Engine->InputMousePosSet = true;
            Engine->InputMousePosValue = Engine->InputMousePosValue + delta * move_speed * inv_length;
            ImGuiTestEngine_Yield(Engine);
        }
    }
}

void    ImGuiTestContext::MouseDown(int button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MouseDown %d\n", button);

    Engine->InputMouseButtonsSet = true;
    Engine->InputMouseButtonsValue = (1 << button);
    Yield();
}

void    ImGuiTestContext::MouseUp(int button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MouseUp %d\n", button);

    Engine->InputMouseButtonsSet = true;
    Engine->InputMouseButtonsValue &= ~(1 << button);
    Yield();
}

// TODO: click time argument (seconds and/or frames)
void    ImGuiTestContext::MouseClick(int button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MouseClick %d\n", button);

    Yield();
    Engine->InputMouseButtonsSet = true;
    Engine->InputMouseButtonsValue = (1 << button);
    Yield();
    Engine->InputMouseButtonsSet = true;
    Engine->InputMouseButtonsValue = 0;
    Yield();
    Yield(); // Give a frame for items to react
}

void    ImGuiTestContext::KeyPressMap(ImGuiKey key, int mod_flags, int count)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    char mod_flags_str[32];
    GetImGuiKeyModsPrefixStr(mod_flags, mod_flags_str, IM_ARRAYSIZE(mod_flags_str));
    LogVerbose("KeyPressMap(%s%s, %d)\n", mod_flags_str, GetImGuiKeyName(key), count);
    while (count > 0)
    {
        count--;
        Engine->InputQueue.push_back(ImGuiTestInput::FromKey(key, true, mod_flags));
        Yield();
        Engine->InputQueue.push_back(ImGuiTestInput::FromKey(key, false));
        Yield();

        // Give a frame for items to react
        Yield();
    }
}

void    ImGuiTestContext::KeyChars(const char* chars)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("KeyChars('%s')\n", chars);
    while (*chars)
    {
        unsigned int c = 0;
        int bytes_count = ImTextCharFromUtf8(&c, chars, NULL);
        chars += bytes_count;
        if (c > 0 && c <= 0xFFFF)
            Engine->InputQueue.push_back(ImGuiTestInput::FromChar((ImWchar)c));

        if (!Engine->IO.ConfigRunFast)
            Sleep(1.0f / Engine->IO.TypingSpeed);
    }
    Yield();
}

void    ImGuiTestContext::KeyCharsAppend(const char* chars)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("KeyCharsAppend('%s')\n", chars);
    KeyPressMap(ImGuiKey_End);
    KeyChars(chars);
}

void    ImGuiTestContext::KeyCharsAppendEnter(const char* chars)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("KeyCharsAppendValidate('%s')\n", chars);
    KeyPressMap(ImGuiKey_End);
    KeyChars(chars);
    KeyPressMap(ImGuiKey_Enter);
}

void    ImGuiTestContext::FocusWindow(ImGuiTestRef ref)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestRefDesc desc(ref, NULL);
    LogVerbose("FocusWindow('%s')\n", desc.c_str());

    ImGuiID window_id = GetID(ref);
    ImGuiWindow* window = ImGui::FindWindowByID(window_id);
    IM_CHECK(window != NULL);
    if (window)
    {
        ImGui::FocusWindow(window);
        Yield();
    }
}

void    ImGuiTestContext::BringWindowToFront(ImGuiWindow* window)
{
    ImGuiContext& g = *UiContext;

    if (window == NULL)
    {
        ImGuiID window_id = GetID("");
        window = ImGui::FindWindowByID(window_id);
        IM_ASSERT(window != NULL);
    }

    if (window != g.NavWindow)
    {
        IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
        LogVerbose("BringWindowToFront->FocusWindow('%s')\n", window->Name);
        ImGui::FocusWindow(window);
        Yield();
        Yield();
    }
    else if (window->RootWindow != g.Windows.back()->RootWindow)
    {
        IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
        LogVerbose("BringWindowToDisplayFront('%s') (window.back=%s)\n", window->Name, g.Windows.back()->Name);
        ImGui::BringWindowToDisplayFront(window);
        Yield();
        Yield();
    }
}

void    ImGuiTestContext::BringWindowToFrontFromItem(ImGuiTestRef ref)
{
    if (IsError())
        return;

    const ImGuiTestItemInfo* item = ItemLocate(ref);
    if (item == NULL)
        IM_CHECK(item != NULL);
    //LogVerbose("FocusWindowForItem %s\n", ImGuiTestRefDesc(ref, item).c_str());
    //LogVerbose("Lost focus on window '%s', getting again..\n", item->Window->Name);
    BringWindowToFront(item->Window);
}

void    ImGuiTestContext::GatherItems(ImGuiTestItemList* out_list, ImGuiTestRef parent, int depth)
{
    IM_ASSERT(out_list != NULL);
    IM_ASSERT(depth > 0 || depth == -1);
    IM_ASSERT(Engine->GatherTask.ParentID == 0);
    IM_ASSERT(Engine->GatherTask.LastItemInfo == NULL);

    if (IsError())
        return;

    // Register gather tasks
    if (depth == -1)
        depth = 99;
    if (parent.ID == 0)
        parent.ID = GetID(parent);
    Engine->GatherTask.ParentID = parent.ID;
    Engine->GatherTask.Depth = depth;
    Engine->GatherTask.OutList = out_list;

    // Keep running while gathering
    const int begin_gather_size = out_list->Size;
    while (true)
    {
        const int begin_gather_size_for_frame = out_list->Size;
        Yield();
        const int end_gather_size_for_frame = out_list->Size;
        if (begin_gather_size_for_frame == end_gather_size_for_frame)
            break;
    }
    const int end_gather_size = out_list->Size;

    ImGuiTestItemInfo* parent_item = ItemLocate(parent, ImGuiTestOpFlags_NoError);
    LogVerbose("GatherItems from %s, %d deep: found %d items.\n", ImGuiTestRefDesc(parent, parent_item).c_str(), depth, end_gather_size - begin_gather_size);

    Engine->GatherTask.ParentID = 0;
    Engine->GatherTask.Depth = 0;
    Engine->GatherTask.OutList = NULL;
    Engine->GatherTask.LastItemInfo = NULL;
}

void    ImGuiTestContext::ItemAction(ImGuiTestAction action, ImGuiTestRef ref)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);

    // [DEBUG] Breakpoint
    //if (ref.ID == 0x0d4af068)
    //    printf("");

    ImGuiTestItemInfo* item = ItemLocate(ref);
    ImGuiTestRefDesc desc(ref, item);
    if (item == NULL)
        return;

    LogVerbose("Item%s %s%s\n", GetActionName(action), desc.c_str(), (InputMode == ImGuiInputSource_Mouse) ? "" : " (w/ Nav)");

    if (action == ImGuiTestAction_Click || action == ImGuiTestAction_DoubleClick)
    {
        if (InputMode == ImGuiInputSource_Mouse)
        {
            MouseMove(ref);
            if (!Engine->IO.ConfigRunFast)
                Sleep(0.05f);
            MouseClick(0);
            if (action == ImGuiTestAction_DoubleClick)
                MouseClick(0);
        }
        else
        {
            NavMove(ref);
            NavActivate();
            if (action == ImGuiTestAction_DoubleClick)
                IM_ASSERT(0);
        }
        return;
    }

    if (action == ImGuiTestAction_Input)
    {
        if (InputMode == ImGuiInputSource_Mouse)
            MouseMove(ref);
        else
            NavMove(ref);
        if (!Engine->IO.ConfigRunFast)
            Sleep(0.05f);
        NavInput();
        return;
    }

    if (action == ImGuiTestAction_Open)
    {
        if (item && (item->StatusFlags & ImGuiItemStatusFlags_Opened) == 0)
        {
            item->RefCount++;
            ItemClick(ref);
            if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) == 0)
            {
                ItemDoubleClick(ref); // Attempt a double-click // FIXME-TESTS: let's not start doing those fuzzy things..
                if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) == 0)
                    IM_ERRORF_NOHDR("Unable to Open item: %s", ImGuiTestRefDesc(ref, item).c_str());
            }
            item->RefCount--;
            Yield();
        }
        return;
    }

    if (action == ImGuiTestAction_Close)
    {
        if (item && (item->StatusFlags & ImGuiItemStatusFlags_Opened) != 0)
        {
            item->RefCount++;
            ItemClick(ref);
            if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) != 0)
            {
                ItemDoubleClick(ref); // Attempt a double-click // FIXME-TESTS: let's not start doing those fuzzy things..
                if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) != 0)
                    IM_ERRORF_NOHDR("Unable to Close item: %s", ImGuiTestRefDesc(ref, item).c_str());
            }
            item->RefCount--;
            Yield();
        }
        return;
    }

    if (action == ImGuiTestAction_Check)
    {
        if ((item->StatusFlags & ImGuiItemStatusFlags_Checkable) && !(item->StatusFlags & ImGuiItemStatusFlags_Checked))
        {
            ItemClick(ref);
            Yield();
        }
        ItemVerifyCheckedIfAlive(ref, true);     // We can't just IM_ASSERT(ItemIsChecked()) because the item may disappear and never update its StatusFlags any more!

        return;
    }

    if (action == ImGuiTestAction_Uncheck)
    {
        if ((item->StatusFlags & ImGuiItemStatusFlags_Checkable) && (item->StatusFlags & ImGuiItemStatusFlags_Checked))
        {
            ItemClick(ref);
            Yield();
        }
        ItemVerifyCheckedIfAlive(ref, false);       // We can't just IM_ASSERT(ItemIsChecked()) because the item may disappear and never update its StatusFlags any more!

        return;
    }

    IM_ASSERT(0);
}

void    ImGuiTestContext::ItemActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent, int max_depth, int max_passes)
{
    if (max_depth == -1)
        max_depth = 99;
    if (max_passes == -1)
        max_passes = 99;
    IM_ASSERT(max_depth > 0 && max_passes > 0);

    int actioned_total = 0;
    for (int pass = 0; pass < max_passes; pass++)
    {
        ImGuiTestItemList items;
        GatherItems(&items, ref_parent, max_depth);

        // Find deep most items
        int highest_depth = -1;
        if (action == ImGuiTestAction_Close)
        {
            for (int n = 0; n < items.Size; n++)
            {
                const ImGuiTestItemInfo* info = items[n];
                if ((info->StatusFlags & ImGuiItemStatusFlags_Openable) && (info->StatusFlags & ImGuiItemStatusFlags_Opened))
                    highest_depth = ImMax(highest_depth, info->Depth);
            }
        }

        int actioned_total_at_beginning_of_pass = actioned_total;

        // Process top-to-bottom in most cases
        int scan_start = 0;
        int scan_end = items.Size;
        int scan_dir = +1;
        if (action == ImGuiTestAction_Close)
        {
            // Close bottom-to-top because 
            // 1) it is more likely to handle same-depth parent/child relationship better (e.g. CollapsingHeader)
            // 2) it gives a nicer sense of symmetry with the corresponding open operation.
            scan_start = items.Size - 1;
            scan_end = -1;
            scan_dir = -1;
        }

        for (int n = scan_start; n != scan_end; n += scan_dir)
        {
            if (IsError())
                break;

            const ImGuiTestItemInfo* info = items[n];
            switch (action)
            {
            case ImGuiTestAction_Click:
                ItemAction(action, info->ID);
                actioned_total++;
                break;
            case ImGuiTestAction_Check:
                if ((info->StatusFlags & ImGuiItemStatusFlags_Checkable) && !(info->StatusFlags & ImGuiItemStatusFlags_Checked))
                {
                    ItemAction(action, info->ID);
                    actioned_total++;
                }
                break;
            case ImGuiTestAction_Uncheck:
                if ((info->StatusFlags & ImGuiItemStatusFlags_Checkable) && (info->StatusFlags & ImGuiItemStatusFlags_Checked))
                {
                    ItemAction(action, info->ID);
                    actioned_total++;
                }
                break;
            case ImGuiTestAction_Open:
                if ((info->StatusFlags & ImGuiItemStatusFlags_Openable) && !(info->StatusFlags & ImGuiItemStatusFlags_Opened))
                {
                    ItemAction(action, info->ID);
                    actioned_total++;
                }
                break;
            case ImGuiTestAction_Close:
                if (info->Depth == highest_depth && (info->StatusFlags & ImGuiItemStatusFlags_Openable) && (info->StatusFlags & ImGuiItemStatusFlags_Opened))
                {
                    ItemClose(info->ID);
                    actioned_total++;
                }
                break;
            default:
                IM_ASSERT(0);
            }
        }

        if (IsError())
            break;

        if (actioned_total_at_beginning_of_pass == actioned_total)
            break;
    }
    LogVerbose("%s %d items in total!\n", GetActionVerb(action), actioned_total);
}

void    ImGuiTestContext::ItemHold(ImGuiTestRef ref, float time)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("ItemHold '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

    MouseMove(ref);

    Yield();
    Engine->InputMouseButtonsSet = true;
    Engine->InputMouseButtonsValue = (1 << 0);
    Sleep(time);
    Engine->InputMouseButtonsSet = true;
    Engine->InputMouseButtonsValue = 0;
    Yield();
}

void    ImGuiTestContext::ItemHoldForFrames(ImGuiTestRef ref, int frames)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("ItemHoldForFrames '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

    MouseMove(ref);
    Yield();
    Engine->InputMouseButtonsSet = true;
    Engine->InputMouseButtonsValue = (1 << 0);
    YieldFrames(frames);
    Engine->InputMouseButtonsSet = true;
    Engine->InputMouseButtonsValue = 0;
    Yield();
}

void    ImGuiTestContext::ItemDragAndDrop(ImGuiTestRef ref_src, ImGuiTestRef ref_dst)
{
    if (IsError())
        return;
    ImGuiContext& g = *UiContext;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestItemInfo* item_src = ItemLocate(ref_src);
    ImGuiTestItemInfo* item_dst = ItemLocate(ref_dst);
    ImGuiTestRefDesc desc_src(ref_src, item_src);
    ImGuiTestRefDesc desc_dst(ref_dst, item_dst);
    LogVerbose("ItemDragAndDrop %s to %s\n", desc_src.c_str(), desc_dst.c_str());

    MouseMove(ref_src, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepShort();
    MouseDown(0);

    // Enforce lifting drag threshold even if both item are exactly at the same location.
    g.IO.MouseDragMaxDistanceAbs[0] = ImVec2(g.IO.MouseDragThreshold, g.IO.MouseDragThreshold);
    g.IO.MouseDragMaxDistanceSqr[0] = (g.IO.MouseDragThreshold * g.IO.MouseDragThreshold) + (g.IO.MouseDragThreshold * g.IO.MouseDragThreshold);

    MouseMove(ref_dst, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepShort();
    MouseUp(0);
}

void    ImGuiTestContext::ItemVerifyCheckedIfAlive(ImGuiTestRef ref, bool checked)
{
    Yield();
    if (ImGuiTestItemInfo* item = ItemLocate(ref, ImGuiTestOpFlags_NoError))
        if (item->TimestampMain + 1 >= Engine->FrameCount && item->TimestampStatus == item->TimestampMain)
            if (((item->StatusFlags & ImGuiItemStatusFlags_Checked) != 0) != checked)
                IM_CHECK(((item->StatusFlags & ImGuiItemStatusFlags_Checked) != 0) == checked);
}

void    ImGuiTestContext::MenuAction(ImGuiTestAction action, ImGuiTestRef ref)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MenuAction '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

    IM_ASSERT(ref.Path != NULL);

    int depth = 0;
    const char* path = ref.Path;
    const char* path_end = path + strlen(path);
    char buf[128];
    while (path < path_end)
    {
        const char* p = ImStrchrRange(path, path_end, '/');
        if (p == NULL)
            p = path_end;
        const bool is_target_item = (p == path_end);
        if (depth == 0)
        {
            // Click menu in menu bar
            IM_ASSERT(RefStr[0] != 0); // Unsupported: window needs to be in Ref
            ImFormatString(buf, IM_ARRAYSIZE(buf), "##menubar/%.*s", p - path, path);
        }
        else
        {
            // Click sub menu in its own window
            ImFormatString(buf, IM_ARRAYSIZE(buf), "/##Menu_%02d/%.*s", depth - 1, p - path, path);
        }

        // We cannot move diagonally to a menu item because depending on the angle and other items we cross on our path we could close our target menu.
        // First move horizontal into the menu...
        if (depth > 0)
        {
            ImGuiTestItemInfo* item = ItemLocate(buf);
            item->RefCount++;
            if (depth > 1 && Engine->InputMousePosValue.x <= item->Rect.Min.x || Engine->InputMousePosValue.x >= item->Rect.Max.x)
                MouseMoveToPos(ImVec2(item->Rect.GetCenter().x, Engine->InputMousePosValue.y));
            if (depth > 0 && Engine->InputMousePosValue.y <= item->Rect.Min.y || Engine->InputMousePosValue.y >= item->Rect.Max.y)
                MouseMoveToPos(ImVec2(Engine->InputMousePosValue.x, item->Rect.GetCenter().y));
            item->RefCount--;
        }

        if (is_target_item)
        {
            // Final item
            ItemAction(action, buf);
        }
        else
        {
            // Then aim at the menu item
            ItemAction(ImGuiTestAction_Click, buf);
        }

        path = p + 1;
        depth++;
    }
}

void    ImGuiTestContext::MenuActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent)
{
    ImGuiTestItemList items;
    MenuAction(ImGuiTestAction_Open, ref_parent);
    GatherItems(&items, GetFocusWindowRef(), 1);
    for (int n = 0; n < items.Size; n++)
    {
        const ImGuiTestItemInfo* item = items[n];
        MenuAction(ImGuiTestAction_Open, ref_parent); // We assume that every interaction will close the menu again
        ItemAction(action, item->ID);
    }
}

void    ImGuiTestContext::WindowClose()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("WindowClose\n");
    ItemClick("#CLOSE");
}

void    ImGuiTestContext::WindowSetCollapsed(bool collapsed)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("WindowSetCollapsed(%d)\n", collapsed);
    ImGuiWindow* window = ImGui::FindWindowByID(RefID);
    if (window == NULL)
    {
        IM_ERRORF_NOHDR("Unable to find Ref window: %s / %08X", RefStr, RefID);
        return;
    }

    if (window->Collapsed != collapsed)
    {
        ItemClick("#COLLAPSE");
        IM_ASSERT(window->Collapsed == collapsed);
    }
}

void    ImGuiTestContext::WindowMove(ImVec2 pos, ImVec2 pivot)
{
    if (IsError())
        return;

    ImGuiWindow* window = GetRefWindow();
    pos -= pivot * window->Size;
    pos = ImFloor(pos);
    if (ImLengthSqr(pos - window->Pos) < 0.001f)
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    BringWindowToFront(window);
    WindowSetCollapsed(false);

    float h = ImGui::GetFrameHeight();

    // FIXME-TESTS: Need to find a -visible- click point
    MouseMoveToPos(window->Pos + ImVec2(h * 2.0f, h * 0.5f));
    MouseDown(0);

    ImVec2 delta = pos - window->Pos;
    MouseMoveToPos(Engine->InputMousePosValue + delta);
    Yield();

    MouseUp();
}

void    ImGuiTestContext::WindowResize(ImVec2 size)
{
    if (IsError())
        return;

    ImGuiWindow* window = GetRefWindow();
    size = ImFloor(size);
    if (ImLengthSqr(size - window->Size) < 0.001f)
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    BringWindowToFront(window);
    WindowSetCollapsed(false);

    void* zero_ptr = (void*)0;
    ImGuiID id = ImHashData(&zero_ptr, sizeof(void*), GetID("#RESIZE")); // FIXME-TESTS
    MouseMove(id);

    MouseDown(0);

    ImVec2 delta = size - window->Size;
    MouseMoveToPos(Engine->InputMousePosValue + delta);
    Yield();

    MouseUp();
}

void    ImGuiTestContext::PopupClose()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("PopupClose\n");
    ImGui::ClosePopupToLevel(0, true);    // FIXME
}

void    ImGuiTestContext::DockSetMulti(ImGuiID dock_id, const char* window_name, ...)
{
    va_list args;
    va_start(args, window_name);
    while (window_name != NULL)
    {
        ImGui::DockBuilderDockWindow(window_name, dock_id);
        window_name = va_arg(args, const char*);
    }
    va_end(args);
}

ImGuiID ImGuiTestContext::DockSetupBasicMulti(ImGuiID dock_id, const char* window_name, ...)
{
    va_list args;
    va_start(args, window_name);
    dock_id = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_None);
    ImGui::DockBuilderSetNodePos(dock_id, ImGui::GetMainViewport()->Pos + ImVec2(100, 100));
    ImGui::DockBuilderSetNodeSize(dock_id, ImVec2(200, 200));
    while (window_name != NULL)
    {
        ImGui::DockBuilderDockWindow(window_name, dock_id);
        window_name = va_arg(args, const char*);
    }
    return dock_id;
}

//-------------------------------------------------------------------------
// ImGuiTestContext - Performance Tools
//-------------------------------------------------------------------------

void    ImGuiTestContext::PerfCalcRef()
{
    LogVerbose("Measuring ref dt...\n");
    SetGuiFuncEnabled(false);
    for (int n = 0; n < 500 && !Abort; n++)
        Yield();
    PerfRefDt = Engine->PerfDeltaTime500.GetAverage();
    SetGuiFuncEnabled(true);
}

void    ImGuiTestContext::PerfCapture()
{
    ImGuiTestEngine* engine = Engine;

    if (PerfRefDt < 0.0)
        PerfCalcRef();
    IM_ASSERT(PerfRefDt >= 0.0);

    // Yield for the average to stabilize
    LogVerbose("Measuring gui dt...\n");
    for (int n = 0; n < 500 && !Abort; n++)
        Yield();
    if (Abort)
        return;

    double dt_curr = engine->PerfDeltaTime500.GetAverage();
    double dt_ref_ms = PerfRefDt * 1000;
    double dt_delta_ms = (dt_curr - PerfRefDt) * 1000;
    //Log("[PERF] Name: %s\n", Test->Name);
    Log("[PERF] Conditions: Stress x%d, %s, %s, %s, %s, %s\n", 
        PerfStressAmount, engine->InfoBuildType, engine->InfoBuildCpu, engine->InfoBuildOS, engine->InfoBuildCompiler, engine->InfoBuildDate);
    Log("[PERF] Result: %+6.3f ms (from ref %+6.3f)\n", dt_delta_ms, dt_ref_ms);

    // Log to .csv
    if (Engine->PerfPersistentLogCsv != NULL)
    {
        fprintf(Engine->PerfPersistentLogCsv,
            ",%s,%s,%.3f,,%d,%s,%s,%s,%s,%s\n",
            Test->Category, Test->Name, dt_delta_ms,
            PerfStressAmount, engine->InfoBuildType, engine->InfoBuildCpu, engine->InfoBuildOS, engine->InfoBuildCompiler, engine->InfoBuildDate);
        fflush(engine->PerfPersistentLogCsv);
    }

    // Disable the "Success" message
    RunFlags |= ImGuiTestRunFlags_NoSuccessMsg;
}

//-------------------------------------------------------------------------
