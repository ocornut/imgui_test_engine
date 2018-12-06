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
#include "imgui_test_engine.h"

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

// FIXME-TESTS: Be able to run blind
// FIXME-TESTS: Be able to interactively run GUI function without Test function
// FIXME-TESTS: Provide variants of same test (e.g. run same tests with a bunch of styles/flags)
// FIXME-TESTS: Automate clicking/opening stuff based on gathering id?
// FIXME-TESTS: Mouse actions on ImGuiNavLayer_Menu layer
// FIXME-TESTS: Fail to open a double-click tree node

//-------------------------------------------------------------------------
// [SECTION] DATA STRUCTURES
//-------------------------------------------------------------------------

static ImGuiTestEngine*     GImGuiTestEngine;

// Locate item position/window/state given ID.
struct ImGuiTestLocateTask
{
    ImGuiID                 ID = 0;
    int                     FrameCount = -1;        // Timestamp of request
    char                    DebugName[64] = "";
    ImGuiTestItemInfo       Result;
};

struct ImGuiTestEngine
{
    ImGuiTestEngineIO       IO;
    ImGuiContext*           UiVisibleContext = NULL;    // imgui context for visible/interactive needs
    ImGuiContext*           UiBlindContext = NULL;
    ImGuiContext*           UiTestContext = NULL;       // imgui context for testing == io.ConfigRunBling ? UiBlindContext : UiVisibleContext

    int                     FrameCount = 0;
    ImVector<ImGuiTest*>    TestsAll;
    ImVector<ImGuiTest*>    TestsQueue;
    ImGuiTestContext*       TestContext = NULL;
    int                     CallDepth = 0;
    ImVector<ImGuiTestLocateTask*>  LocateTasks;
    bool                    SetMousePos = false;
    bool                    SetMouseButtons = false;
    ImVec2                  SetMousePosValue;
    int                     SetMouseButtonsValue = 0x00;

    // UI support
    bool                    Abort = false;
    bool                    UiFocus = false;
    ImGuiTest*              UiSelectedTest = NULL;

    // Functions
    ImGuiTestEngine() { }
    ~ImGuiTestEngine()
    {
        if (UiBlindContext != NULL)
            ImGui::DestroyContext(UiBlindContext);
    }
};

//-------------------------------------------------------------------------
// [SECTION] TEST ENGINE: FORWARD DECLARATIONS
//-------------------------------------------------------------------------

// Private functions
static ImGuiTestItemInfo*   ImGuiTestEngine_ItemLocate(ImGuiTestEngine* engine, ImGuiID id, const char* debug_id);
static void                 ImGuiTestEngine_ApplyInputToImGuiContext(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_ProcessQueue(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_ClearTests(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_ClearLocateTasks(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_PreNewFrame(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_PostNewFrame(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_Yield(ImGuiTestEngine* engine);
static void                 ImGuiTestEngine_QueueTest(ImGuiTestEngine* engine, ImGuiTest* test);
static inline bool          ImGuiTestEngine_IsRunningTests(ImGuiTestEngine* engine) { return engine->TestsQueue.Size > 0; }

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
    if (GImGuiTestEngine == NULL)
        GImGuiTestEngine = engine;
    engine->UiVisibleContext = imgui_context;
    engine->UiBlindContext = NULL;
    engine->UiTestContext = imgui_context;
    return engine;
}

void    ImGuiTestEngine_ShutdownContext(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->CallDepth == 0);
    ImGuiTestEngine_ClearTests(engine);
    ImGuiTestEngine_ClearLocateTasks(engine);
    IM_DELETE(engine);

    if (GImGuiTestEngine == NULL)
        GImGuiTestEngine = NULL;
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

// Hash "hello/world" as if it was "helloworld"
// To hash a forward slash we need to use "hello\\/world"
//   IM_ASSERT(ImHashDecoratedPath("Hello/world")   == ImHash("Helloworld", 0));
//   IM_ASSERT(ImHashDecoratedPath("Hello\\/world") == ImHash("Hello/world", 0));
// Adapted from ImHash(). Not particularly fast!
static ImGuiID ImHashDecoratedPath(const char* str, ImGuiID seed)
{
    static ImU32 crc32_lut[256] = { 0 };
    if (!crc32_lut[1])
    {
        const ImU32 polynomial = 0xEDB88320;
        for (ImU32 i = 0; i < 256; i++)
        {
            ImU32 crc = i;
            for (ImU32 j = 0; j < 8; j++)
                crc = (crc >> 1) ^ (ImU32(-int(crc & 1)) & polynomial);
            crc32_lut[i] = crc;
        }
    }

    // Prefixing the string with / ignore the seed
    if (str[0] == '/')
        seed = 0;

    seed = ~seed;
    ImU32 crc = seed;

    // Zero-terminated string
    bool inhibit_one = false;
    const unsigned char* current = (const unsigned char*)str;
    while (unsigned char c = *current++)
    {
        if (c == '\\' && !inhibit_one)
        {
            inhibit_one = true;
            continue;
        }

        // Forward slashes are ignored unless prefixed with a backward slash
        if (c == '/' && !inhibit_one)
        {
            inhibit_one = false;
            continue;
        }

        // Reset the hash when encountering ### 
        if (c == '#' && current[0] == '#' && current[1] == '#')
            crc = seed;

        crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        inhibit_one = false;
    }
    return ~crc;
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
            return &task->Result;
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
    ImGuiContext& g = *engine->UiTestContext;
    g.IO.MouseDrawCursor = true;

    if (ImGuiTestEngine_IsRunningTests(engine))
    {
        //if (engine->SetMousePos)
        {
            g.IO.MousePos = engine->SetMousePosValue;
            g.IO.WantSetMousePos = true;
        }
        if (engine->SetMouseButtons)
        {
            for (int n = 0; n < IM_ARRAYSIZE(g.IO.MouseDown); n++)
                g.IO.MouseDown[n] = (engine->SetMouseButtonsValue & (1 << n)) != 0;
        }
        engine->SetMousePos = false;
        engine->SetMouseButtons = false;
    }
    else
    {
        engine->SetMousePosValue = g.IO.MousePos;
    }
}

static void ImGuiTestEngine_PreNewFrame(ImGuiTestEngine* engine)
{
    ImGuiContext& g = *engine->UiTestContext;
    IM_ASSERT(engine->UiTestContext == GImGui);

    engine->FrameCount = g.FrameCount + 1;  // NewFrame() will increase this.
    if (engine->TestContext)
        engine->TestContext->FrameCount++;

    if (ImGuiTestEngine_IsRunningTests(engine))
    {
        const int key_idx_escape = g.IO.KeyMap[ImGuiKey_Escape];
        if (key_idx_escape != -1 && g.IO.KeysDown[key_idx_escape])
            ImGuiTestEngine_Abort(engine);
    }

    ImGuiTestEngine_ApplyInputToImGuiContext(engine);
}

static void ImGuiTestEngine_PostNewFrame(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->UiTestContext == GImGui);

    // Garbage collect unused tasks
    for (int task_n = 0; task_n < engine->LocateTasks.Size; task_n++)
    {
        ImGuiTestLocateTask* task = engine->LocateTasks[task_n];
        if (task->FrameCount < engine->FrameCount - 20 && task->Result.RefCount == 0)
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
    engine->IO.EndFrameFunc(engine, engine->IO.UserData);
    engine->IO.NewFrameFunc(engine, engine->IO.UserData);

    // Call user GUI function
    if (engine->TestContext && engine->TestContext->Test->GuiFunc)
        engine->TestContext->Test->GuiFunc(engine->TestContext);
}

static void ImGuiTestEngine_ProcessQueue(ImGuiTestEngine* engine)
{
    IM_ASSERT(engine->CallDepth == 0);
    engine->CallDepth++;

    int ran_tests = 0;
    engine->IO.RunningTests = true;
    for (int n = 0; n < engine->TestsQueue.Size; n++)
    {
        ImGuiTest* test = engine->TestsQueue[n];
        IM_ASSERT(test->Status == ImGuiTestStatus_Queued);

        if (engine->Abort)
        {
            test->Status = ImGuiTestStatus_Unknown;
            continue;
        }

        test->Status = ImGuiTestStatus_Running;
        engine->UiSelectedTest = test;

        // If the user defines a RootFunc, they will call RunTest() themselves. This allows them to conveniently store data in the stack.
        ImGuiTestContext ctx;
        ctx.Test = test;
        ctx.Engine = engine;
        ctx.UserData = NULL;
        ctx.UiContext = engine->UiTestContext;
        engine->TestContext = &ctx;

        ctx.Log("----------------------------------------------------------------------\n");
        ctx.Log("Test: '%s' '%s'..\n", test->Category, test->Name);
        if (test->RootFunc)
            test->RootFunc(&ctx);
        else
            ctx.RunCurrentTest(NULL);
        ran_tests++;

        IM_ASSERT(test->Status != ImGuiTestStatus_Running);
        if (engine->Abort)
            test->Status = ImGuiTestStatus_Unknown;

        if (engine->Abort)
            ctx.Log("Aborted.\n");
        else if (test->Status == ImGuiTestStatus_Success)
            ctx.Log("Success.\n");
        else if (test->Status == ImGuiTestStatus_Error)
            ctx.Log("Error.\n");
        else
            ctx.Log("Unknown status.\n");

        IM_ASSERT(engine->TestContext == &ctx);
        engine->TestContext = NULL;

        // Auto select the first error test
        //if (test->Status == ImGuiTestStatus_Error)
        //    if (engine->UiSelectedTest == NULL || engine->UiSelectedTest->Status != ImGuiTestStatus_Error)
        //        engine->UiSelectedTest = test;
    }
    engine->IO.RunningTests = false;

    engine->Abort = false;
    engine->CallDepth--;
    engine->TestsQueue.clear();
    if (ran_tests)
        engine->UiFocus = true;
}

static void ImGuiTestEngine_QueueTest(ImGuiTestEngine* engine, ImGuiTest* test)
{
    if (engine->TestsQueue.contains(test))
        return;

    // Detect lack of signal from imgui context, most likely not compiled with IMGUI_ENABLE_TEST_ENGINE=1
    if (engine->FrameCount < engine->UiTestContext->FrameCount - 2)
    {
        ImGuiTestEngine_Abort(engine);
        IM_ASSERT(0 && "Not receiving signal from core library. Did you call ImGuiTestEngine_CreateContext() with the correct context? Did you compile imgui/ with IMGUI_ENABLE_TEST_ENGINE=1?");
        test->Status = ImGuiTestStatus_Error;
        return;
    }

    test->Status = ImGuiTestStatus_Queued;
    engine->TestsQueue.push_back(test);
}

void ImGuiTestEngine_QueueAllTests(ImGuiTestEngine* engine)
{
    for (int n = 0; n < engine->TestsAll.Size; n++)
        ImGuiTestEngine_QueueTest(engine, engine->TestsAll[n]);
}

static void ImGuiTestEngine_RunTest(ImGuiTestEngine* engine, ImGuiTestContext* ctx, void* user_data)
{
    ImGuiTest* test = ctx->Test;
    ctx->Engine = engine;
    ctx->UserData = user_data;
    ctx->FrameCount = 0;
    ctx->SetRef("");

    test->TestLog.clear();

    if (!(test->Flags & ImGuiTestFlags_NoWarmUp))
    {
        ctx->Yield();
        ctx->Yield();
    }

    // Call user test function
    test->TestFunc(ctx);

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

void ImGuiTestEngineHook_PreNewFrame()
{
    if (ImGuiTestEngine* engine = GImGuiTestEngine)
        if (engine->UiTestContext == GImGui)
            ImGuiTestEngine_PreNewFrame(engine);
}

void ImGuiTestEngineHook_PostNewFrame()
{
    if (ImGuiTestEngine* engine = GImGuiTestEngine)
        if (engine->UiTestContext == GImGui)
            ImGuiTestEngine_PostNewFrame(engine);
}

void ImGuiTestEngineHook_ItemAdd(ImGuiID id, const ImRect& bb)
{
    if (id == 0)
        return;
    ImGuiTestEngine* engine = GImGuiTestEngine;
    if (engine == NULL)
        return;

    ImGuiContext& g = *GImGui;
    if (&g != engine->UiTestContext)
        return;

    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(window->DC.LastItemId == id);

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
        item->StatusFlags = window->DC.LastItemStatusFlags;
    }
}

// label is optional
void ImGuiTestEngineHook_ItemInfo(ImGuiID id, const char* label, int flags)
{
    if (id == 0)
        return;
    ImGuiTestEngine* engine = GImGuiTestEngine;
    if (engine == NULL)
        return;

    ImGuiContext& g = *GImGui;
    if (&g != engine->UiTestContext)
        return;

    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(window->DC.LastItemId == id);

    // Update Locate Task status flags
    if (ImGuiTestLocateTask* task = ImGuiTestEngine_FindLocateTask(engine, id))
    {
        ImGuiTestItemInfo* item = &task->Result;
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
    ImGuiTestEngine* engine = GImGuiTestEngine;
    (void)func;

    // Removed absolute path from output so we have deterministic output (otherwise __FILE__ gives us machine depending output)
    const char* file_without_path = file ? (file + strlen(file)) : NULL;
    while (file_without_path > file && file_without_path[-1] != '/' && file_without_path[-1] != '\\')
        file_without_path--;

    if (result == false)
    {
        if (file)
            printf("KO: %s:%d  '%s'\n", file_without_path, line, expr);
        else
            printf("KO: %s\n", expr);
    }

    if (ImGuiTestContext* ctx = engine->TestContext)
    {
        ImGuiTest* test = ctx->Test;
        ctx->LogVerbose("IM_CHECK(%s)\n", expr);

        if (result)
        {
            if (file)
                test->TestLog.appendf("[%04d] OK %s:%d  '%s'\n", ctx->FrameCount, file_without_path, line, expr);
            else
                test->TestLog.appendf("[%04d] OK %s\n", ctx->FrameCount, expr);
        }
        else
        {
            test->Status = ImGuiTestStatus_Error;
            if (file)
                test->TestLog.appendf("[%04d] KO %s:%d  '%s'\n", ctx->FrameCount, file_without_path, line, expr);
            else
                test->TestLog.appendf("[%04d] KO %s\n", ctx->FrameCount, expr);
        }
    }
    else
    {
        printf("Error: no active test!\n");
        IM_ASSERT(0);
    }

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

    ImGuiTestEngine* engine = GImGuiTestEngine;
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
    float copy_button_width = ImGui::CalcTextSize("C").x + ImGui::GetStyle().FramePadding.x * 2.0f;

    const char* text = test->TestLog.begin();
    const char* text_end = test->TestLog.end();
    int line_no = 0;
    while (text < text_end)
    {
        const char* line_start = text;
        const char* line_end = ImStreolRange(line_start, text_end);
        const bool is_error = ImStristr(line_start, line_end, "] KO", NULL) != NULL; // FIXME-OPT
        const bool is_unimportant = ImStristr(line_start, line_end, "] --", NULL) != NULL; // FIXME-OPT

        if (is_interactive)
        {
            ImGui::PushID(line_no);
            if (ImGui::SmallButton("C"))
            {
                char buf[256];
                const char* copy_start = ImStrchrRange(line_start, line_end, '\'') + 1;
                const char* copy_end = ImStrchrRange(copy_start, line_end, '\'');
                ImFormatString(buf, IM_ARRAYSIZE(buf), "%.*s", copy_end - copy_start, copy_start);
                ImGui::SetClipboardText(buf);
            }
            ImGui::SameLine();
        }
        else
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + copy_button_width);
        }

        if (is_error)
            ImGui::PushStyleColor(ImGuiCol_Text, error_col);
        else if (is_unimportant)
            ImGui::PushStyleColor(ImGuiCol_Text, unimportant_col);
        ImGui::TextUnformatted(line_start, line_end);
        if (is_error || is_unimportant)
            ImGui::PopStyleColor();
        if (is_interactive)
            ImGui::PopID();

        text = line_end + 1;
        line_no++;
    }
    ImGui::PopStyleVar();
}

static const char* GetVerboseLevelName(ImGuiTestVerboseLevel v)
{
    switch (v)
    {
    case ImGuiTestVerboseLevel_Silent:  return "Silent";
    case ImGuiTestVerboseLevel_Normal:  return "Normal";
    case ImGuiTestVerboseLevel_Max:     return "Max";
    }
    return "N/A";
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

    const float log_height = ImGui::GetTextLineHeight() * 15.0f;
    const ImU32 col_highlight = IM_COL32(255, 255, 20, 255);

    // Options
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::Text("OPTIONS:");
    ImGui::SameLine();
    ImGui::Checkbox("Fast", &engine->IO.ConfigRunFast);
    ImGui::SameLine();
    ImGui::Checkbox("Blind", &engine->IO.ConfigRunBlind);
    ImGui::SameLine();
    ImGui::Checkbox("Break", &engine->IO.ConfigBreakOnError);
    ImGui::SameLine();
    ImGui::PushItemWidth(60);
    ImGui::DragInt("Verbose Level", (int*)&engine->IO.ConfigVerboseLevel, 0.1f, 0, ImGuiTestVerboseLevel_COUNT - 1, GetVerboseLevelName(engine->IO.ConfigVerboseLevel));
    //ImGui::Checkbox("Verbose", &engine->IO.ConfigLogVerbose);
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
    ImGui::Separator();

    // TESTS
    ImGui::Text("TESTS (%d)", engine->TestsAll.Size);
    static ImGuiTextFilter filter;
    if (ImGui::Button("Run All"))
        ImGuiTestEngine_QueueAllTests(engine);

    ImGui::SameLine();
    filter.Draw("", -1.0f);
    ImGui::Separator();
    ImGui::BeginChild("Tests", ImVec2(0, -log_height));

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 3));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 1));
    for (int n = 0; n < engine->TestsAll.Size; n++)
    {
        ImGuiTest* test = engine->TestsAll[n];
        if (!filter.PassFilter(test->Name) && !filter.PassFilter(test->Category))
            continue;

        ImGui::PushID(n);

        ImVec4 status_color;
        switch (test->Status)
        {
        case ImGuiTestStatus_Error:     status_color = ImVec4(0.9f, 0.1f, 0.1f, 1.0f); break;
        case ImGuiTestStatus_Success:   status_color = ImVec4(0.1f, 0.9f, 0.1f, 1.0f); break;
        case ImGuiTestStatus_Queued:
        case ImGuiTestStatus_Running:   status_color = ImVec4(0.8f, 0.4f, 0.1f, 1.0f); break;
        default:                        status_color = ImVec4(0.4f, 0.4f, 0.4f, 1.0f); break;
        }
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::ColorButton("status", status_color, ImGuiColorEditFlags_NoTooltip);
        ImGui::SameLine();
        if (test->Status == ImGuiTestStatus_Running)
            ImGui::RenderText(p + g.Style.FramePadding + ImVec2(0,0), "|\0/\0-\0\\" + (((g.FrameCount/5) & 3) << 1), NULL);

        bool queue_test = false;
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
            }
            else
            {
                ImGui::MenuItem("Open source", NULL, false, false);
            }

            if (ImGui::MenuItem("Copy log", NULL, false, !test->TestLog.empty()))
            {
                ImGui::SetClipboardText(test->TestLog.c_str());
            }

            if (ImGui::MenuItem("Run test"))
            {
                queue_test = true;
            }

            ImGui::EndPopup();
        }

        // Process selection
        if (select_test)
            engine->UiSelectedTest = test;

        // Process queuing
        if (queue_test && engine->CallDepth == 0)
            ImGuiTestEngine_QueueTest(engine, test);

        ImGui::PopID();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::EndChild();

    // LOG
    ImGui::Separator();
    if (engine->UiSelectedTest)
        ImGui::Text("LOG %s: %s", engine->UiSelectedTest->Category, engine->UiSelectedTest->Name);
    else
        ImGui::Text("LOG");
    ImGui::Separator();
    ImGui::BeginChild("Log");
    if (engine->UiSelectedTest)
    {
        DrawTestLog(engine, engine->UiSelectedTest, true);
        if (ImGuiTestEngine_IsRunningTests(engine))
            ImGui::SetScrollHereY();
    }
    ImGui::EndChild();

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
    t->SourceLine = src_line;
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
    if (Engine->IO.ConfigVerboseLevel == ImGuiTestVerboseLevel_Silent)
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

void    ImGuiTestContext::Yield()
{
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
        while (time > 0.0f)
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

void ImGuiTestContext::SetRef(ImGuiTestRef ref)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("SetRef '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

    size_t len = strlen(ref.Path);
    IM_ASSERT(len < IM_ARRAYSIZE(RefStr) - 1);

    strcpy(RefStr, ref.Path);
    RefID = ImHashDecoratedPath(ref.Path, 0);
}

ImGuiID ImGuiTestContext::GetID(ImGuiTestRef ref)
{
    if (ref.ID)
        return ref.ID;
    return ImHashDecoratedPath(ref.Path, RefID);
}

ImGuiTestItemInfo* ImGuiTestContext::ItemLocate(ImGuiTestRef ref, ImGuiLocateFlags flags)
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

    if (!(flags & ImGuiLocateFlags_NoError))
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

void    ImGuiTestContext::MouseMove(ImGuiTestRef ref)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MouseMove to '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

    ImGuiTestItemInfo* item = ItemLocate(ref);
    if (item == NULL)
        return;
    item->RefCount++;

    ImGuiWindow* window = item->Window;
    if (item->NavLayer == ImGuiNavLayer_Main && !window->InnerClipRect.Contains(item->Rect))
    {
        // If the item is not currently visible, scroll to get it in the center of our window
        IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
        LogVerbose("Scroll to '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

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

            float scroll_speed = Engine->IO.ConfigRunFast ? FLT_MAX : ImFloor(Engine->IO.ScrollSpeed * g.IO.DeltaTime);
            window->Scroll.y = ImLinearSweep(window->Scroll.y, scroll_target, scroll_speed);

            ImGuiTestEngine_Yield(Engine);
        }
        
        // Need another frame for the result->Rect to stabilize
        ImGuiTestEngine_Yield(Engine);
    }

    MouseMove(item->Rect.GetCenter());
    if (!Abort)
        IM_ASSERT(g.HoveredIdPreviousFrame == item->ID);

    item->RefCount--;
}

void	ImGuiTestContext::MouseMove(ImVec2 target)
{
    ImGuiContext& g = *UiContext;

    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MouseMove from (%.0f,%.0f) to (%.0f,%.0f)\n", Engine->SetMousePosValue.x, Engine->SetMousePosValue.y, target.x, target.y);

    // We manipulate Engine->SetMousePosValue without reading back from g.IO.MousePos because the later is rounded.
    // To handle high framerate it is easier to bypass this rounding.
    while (true)
    {
        ImVec2 delta = target - Engine->SetMousePosValue;
        float inv_length = ImInvLength(delta, FLT_MAX);
        float move_speed = Engine->IO.MouseSpeed * g.IO.DeltaTime;

        if (Engine->IO.ConfigRunFast || (inv_length >= 1.0f / move_speed))
        {
            Engine->SetMousePos = true;
            Engine->SetMousePosValue = target;
            ImGuiTestEngine_Yield(Engine);
            ImGuiTestEngine_Yield(Engine);
            return;
        }
        else
        {
            Engine->SetMousePos = true;
            Engine->SetMousePosValue = Engine->SetMousePosValue + delta * move_speed * inv_length;
            ImGuiTestEngine_Yield(Engine);
        }
    }
}

// TODO: click time argument (seconds and/or frames)
void    ImGuiTestContext::MouseClick(int button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MouseClick %d\n", button);

    Yield();
    Engine->SetMouseButtons = true;
    Engine->SetMouseButtonsValue = (1 << button);
    Yield();
    Engine->SetMouseButtons = true;
    Engine->SetMouseButtonsValue = 0;
    Yield();
    Yield(); // Give a frame for items to react
}

void    ImGuiTestContext::FocusWindowForItem(ImGuiTestRef ref)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    if (const ImGuiTestItemInfo* item = ItemLocate(ref))
        ImGui::FocusWindow(item->Window);
}

void    ImGuiTestContext::ItemAction(ImGuiTestAction action, ImGuiTestRef ref)
{
    if (IsError())
        return;

    if (action == ImGuiTestAction_Click)
    {
        IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
        LogVerbose("ItemClick '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

        FocusWindowForItem(ref);
        MouseMove(ref);
        if (!Engine->IO.ConfigRunFast)
            Sleep(0.05f);
        MouseClick(0);
        return;
    }

    if (action == ImGuiTestAction_Open)
    {
        IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
        LogVerbose("ItemOpen '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

        if (ImGuiTestItemInfo* item = ItemLocate(ref))
            if (item->Window->DC.StateStorage->GetInt(item->ID, 0) == 0)
            {
                item->RefCount++;
                ItemClick(ref);
                IM_ASSERT(item->Window->DC.StateStorage->GetInt(item->ID, 0) == 1);
                item->RefCount--;
            }
        return;
    }

    if (action == ImGuiTestAction_Close)
    {
        IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
        LogVerbose("ItemClose '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

        if (ImGuiTestItemInfo* item = ItemLocate(ref))
            if (item->Window->DC.StateStorage->GetInt(item->ID, 0) == 1)
            {
                item->RefCount++;
                ItemClick(ref);
                IM_ASSERT(item->Window->DC.StateStorage->GetInt(item->ID, 0) == 0);
                item->RefCount--;
            }
        return;
    }

    if (action == ImGuiTestAction_Check)
    {
        IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
        LogVerbose("ItemCheck '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

        if (!ItemIsChecked(ref))
            ItemClick(ref);
        ItemVerifyCheckedIfAlive(ref, true);     // We can't just IM_ASSERT(ItemIsChecked()) because the item may disappear and never update its StatusFlags any more!

        return;
    }

    if (action == ImGuiTestAction_Uncheck)
    {
        IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
        LogVerbose("ItemUncheck '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

        if (ItemIsChecked(ref))
            ItemClick(ref);
        ItemVerifyCheckedIfAlive(ref, false);     // We can't just IM_ASSERT(ItemIsChecked()) because the item may disappear and never update its StatusFlags any more!

        return;
    }

    IM_ASSERT(0);
}

void    ImGuiTestContext::ItemHold(ImGuiTestRef ref, float time)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("ItemHold '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

    FocusWindowForItem(ref);
    MouseMove(ref);

    Yield();
    Engine->SetMouseButtons = true;
    Engine->SetMouseButtonsValue = (1 << 0);
    Sleep(time);
    Engine->SetMouseButtons = true;
    Engine->SetMouseButtonsValue = 0;
    Yield();
}

bool    ImGuiTestContext::ItemIsChecked(ImGuiTestRef ref)
{
    if (IsError())
        return false;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("ItemIsChecked '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

    if (ImGuiTestItemInfo* item = ItemLocate(ref))
    {
        IM_ASSERT(item->TimestampStatus == item->TimestampMain && "Using ItemIsChecked on an item that doesn't call ImGuiTestEngineHook_ItemStatusFlags?");
        return (item->StatusFlags & ImGuiItemStatusFlags_Checked) ? true : false;
    }
    return false;
}

void    ImGuiTestContext::ItemVerifyCheckedIfAlive(ImGuiTestRef ref, bool checked)
{
    Yield();
    if (ImGuiTestItemInfo* item = ItemLocate(ref, ImGuiLocateFlags_NoError))
        if (item->TimestampMain + 1 >= Engine->FrameCount && item->TimestampStatus == item->TimestampMain)
            IM_ASSERT(((item->StatusFlags & ImGuiItemStatusFlags_Checked) != 0) == checked);
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
            IM_ASSERT(RefStr[0] != 0);
            ImFormatString(buf, IM_ARRAYSIZE(buf), "##menubar/%.*s", p - path, path);
            if (is_target_item)
                ItemAction(action, buf);
            else
                ItemAction(ImGuiTestAction_Click, buf);
        }
        else
        {
            // Click sub menu in its own window
            ImFormatString(buf, IM_ARRAYSIZE(buf), "/##Menu_%02d/%.*s", depth - 1, p - path, path);
            if (is_target_item)
                ItemAction(action, buf);
            else
                ItemAction(ImGuiTestAction_Click, buf);
        }
        path = p + 1;
        depth++;
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

void    ImGuiTestContext::PopupClose()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("PopupClose\n");
    ImGui::ClosePopupToLevel(0);    // FIXME
}

//-------------------------------------------------------------------------
