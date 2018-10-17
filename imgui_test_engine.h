// dear imgui
// (test engine)

#pragma once

// Undo some of the damage done by <windows.h>
#ifdef Yield
#undef Yield
#endif

//-------------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------------

struct ImGuiTest;
struct ImGuiTestContext;
struct ImGuiTestEngine;
struct ImGuiTestEngineIO;
struct ImGuiTestLocateResult;
struct ImRect;

typedef int     ImGuiTestFlags;
typedef int     ImGuiLocateFlags;

//-------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------

enum ImGuiTestStatus
{
    ImGuiTestStatus_Unknown     = -1,
    ImGuiTestStatus_Success     = 0,
    ImGuiTestStatus_Queued      = 1,
    ImGuiTestStatus_Running     = 2,
    ImGuiTestStatus_Error       = 3,
};

enum ImGuiTestFlags_
{
    ImGuiTestFlags_None         = 0,
    ImGuiTestFlags_NoWarmUp     = 1 << 0        // By default, we run the GUI func twice before starting the test code
};

enum ImGuiLocateFlags_
{
    ImGuiLocateFlags_None       = 0,
    ImGuiLocateFlags_NoError     = 1 << 0        // Don't abort/error if the item cannot be found
};

//-------------------------------------------------------------------------
// Hooks for Core Library
//-------------------------------------------------------------------------

void    ImGuiTestEngineHook_PreNewFrame();
void    ImGuiTestEngineHook_PostNewFrame();
void    ImGuiTestEngineHook_ItemAdd(ImGuiID id, const ImRect& bb);
void    ImGuiTestEngineHook_ItemStatusFlags(ImGuiID id, int flags);

//-------------------------------------------------------------------------
// Hooks for Tests
//-------------------------------------------------------------------------

// We embed every maacro in a do {} while(0) statement as a trick to allow using them as regular single statement, e.g. if (XXX) IM_CHECK(A); else IM_CHECK(B)
#define IM_CHECK(_EXPR)             do { if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, (bool)(_EXPR), #_EXPR))     { IM_ASSERT(0); } } while (0)
#define IM_ERRORF(_FMT,...)         do { if (ImGuiTestEngineHook_Error(__FILE__, __func__, __LINE__, _FMT, __VA_ARGS__))         { IM_ASSERT(0); } } while (0)
#define IM_ERRORF_NOHDR(_FMT,...)   do { if (ImGuiTestEngineHook_Error(NULL, NULL, 0, _FMT, __VA_ARGS__))                        { IM_ASSERT(0); } } while (0)
//#define IM_ASSERT(_EXPR)      (void)( (!!(_EXPR)) || (ImGuiTestEngineHook_Check(false, #_EXPR, __FILE__, __func__, __LINE__), 0) )

bool    ImGuiTestEngineHook_Check(const char* file, const char* func, int line, bool result, const char* expr);
bool    ImGuiTestEngineHook_Error(const char* file, const char* func, int line, const char* fmt, ...);

//-------------------------------------------------------------------------
// ImGuiTestEngine
//-------------------------------------------------------------------------

// Functions
ImGuiTestEngine*    ImGuiTestEngine_CreateContext(ImGuiContext* imgui_context);
void                ImGuiTestEngine_ShutdownContext(ImGuiTestEngine* engine);
ImGuiTestEngineIO&  ImGuiTestEngine_GetIO(ImGuiTestEngine* engine);
void                ImGuiTestEngine_Abort(ImGuiTestEngine* engine);
void                ImGuiTestEngine_ShowTestWindow(ImGuiTestEngine* engine, bool* p_open);
void                ImGuiTestEngine_QueueAllTests(ImGuiTestEngine* engine);

// IO structure
typedef bool (*ImGuiTestEngineNewFrameFunc)(ImGuiTestEngine*, void* user_data);
typedef bool (*ImGuiTestEngineEndFrameFunc)(ImGuiTestEngine*, void* user_data);
typedef void (*ImGuiTestEngineFileOpenerFunc)(const char* filename, int line, void* user_data);

struct ImGuiTestEngineIO
{
    ImGuiTestEngineNewFrameFunc     EndFrameFunc        = NULL;
    ImGuiTestEngineEndFrameFunc     NewFrameFunc        = NULL;
    ImGuiTestEngineFileOpenerFunc   FileOpenerFunc      = NULL;     // (Optional) To open source files
	void*                           UserData            = NULL;

    // Inputs: Options
    bool                            ConfigRunFast       = true;     // Run tests as fast as possible (teleport mouse, skip delays, etc.)
    bool                            ConfigRunBlind      = false;    // Run tests in a blind ImGuiContext separated from the visible context
    bool                            ConfigBreakOnError  = false;    // Break debugger on test error
    bool                            ConfigLogVerbose    = false;    // Verbose log
    bool                            ConfigLogToTTY      = false;
    float                           MouseSpeed          = 1000.0f;  // Mouse speed (pixel/second) when not running in fast mode
    float                           ScrollSpeed         = 1600.0f;  // Scroll speed (pixel/second) when not running in fast mode

    // Outputs: State
    bool                            RunningTests        = false;
};

//-------------------------------------------------------------------------
// ImGuiTest
//-------------------------------------------------------------------------

typedef void    (*ImGuiTestRunFunc)(ImGuiTestContext* ctx);
typedef void    (*ImGuiTestGuiFunc)(ImGuiTestContext* ctx);
typedef void    (*ImGuiTestTestFunc)(ImGuiTestContext* ctx);

struct ImGuiTest
{
    const char*             Category;           // Literal, not owned
    const char*             Name;               // Literal, not owned
    const char*             SourceFile;         // __FILE__
    const char*             SourceFileShort;    // Pointer within SourceFile, skips filename.
    int                     SourceLine;         // __LINE__
    ImGuiTestStatus         Status;
    ImGuiTestFlags          Flags;
    ImGuiTestRunFunc        RootFunc;           // NULL function is ok
    ImGuiTestGuiFunc        GuiFunc;            // GUI functions can be reused
    ImGuiTestTestFunc       TestFunc;
    ImGuiTextBuffer         TestLog;

    ImGuiTest()
    {
        Category = NULL;
        Name = NULL;
        SourceFile = SourceFileShort = NULL;
        SourceLine = 0;
        Status = ImGuiTestStatus_Unknown;
        Flags = ImGuiTestFlags_None;
        RootFunc = NULL;
        GuiFunc = NULL;
        TestFunc = NULL;
    }
};

//-------------------------------------------------------------------------
// ImGuiTestContext
// This is the interface that most tests will interact with.
//-------------------------------------------------------------------------

enum ImGuiTestAction
{
    ImGuiTestAction_Unknown = 0,
    ImGuiTestAction_Click,
    ImGuiTestAction_Check,
    ImGuiTestAction_Uncheck,
    ImGuiTestAction_Open,
    ImGuiTestAction_Close
};

struct ImGuiTestContext
{
    ImGuiTest*              Test;
    ImGuiTestEngine*        Engine;
    ImGuiContext*           UiContext;
    void*                   UserData;
    int                     FrameCount;     // Test frame count (restarts from zero every time)
    int                     ActionDepth;
    bool                    Abort;
    char                    RefStr[256];
    ImGuiID                 RefID;

    ImGuiTestContext()
    {
        Test = NULL;
        Engine = NULL;
        UserData = NULL;
        UiContext = NULL;
        FrameCount = 0;
        ActionDepth = 0;
        Abort = false;
        memset(RefStr, 0, sizeof(RefStr));
        RefID = 0;
    }

    ImGuiTest*  RegisterTest(const char* category, const char* name, const char* src_file = NULL, int src_line = 0);
    void        RunCurrentTest(void* user_data);
    void        Log(const char* fmt, ...) IM_FMTARGS(1);
    void        LogVerbose(const char* fmt, ...) IM_FMTARGS(1);
    bool        IsError() const { return Test->Status == ImGuiTestStatus_Error || Abort; }

    void        Yield();
    void        Sleep(float time);
    void        SleepShort();

    void        SetRef(const char* path);

    void        MouseMove(const char* path);
    void        MouseMove(ImVec2 pos);
    void        MouseClick(int button = 0);

    void        FocusWindowForItem(const char* path);

    void        ItemAction(ImGuiTestAction action, const char* path);
    void        ItemClick(const char* path)       { ItemAction(ImGuiTestAction_Click, path); }
    void        ItemOpen(const char* path)        { ItemAction(ImGuiTestAction_Open, path); }
    void        ItemClose(const char* path)       { ItemAction(ImGuiTestAction_Close, path); }

    void        ItemHold(const char* path, float time);
    ImGuiTestLocateResult* ItemLocate(const char* path, ImGuiLocateFlags flags = ImGuiLocateFlags_None);
    bool        ItemIsChecked(const char* path);
    void        ItemVerifyCheckedIfAlive(const char* path, bool checked);

    void        MenuAction(ImGuiTestAction action, const char* path);
    void        MenuClick(const char* path)     { MenuAction(ImGuiTestAction_Click, path); }
    void        MenuCheck(const char* path)     { MenuAction(ImGuiTestAction_Check, path); }
    void        MenuUncheck(const char* path)   { MenuAction(ImGuiTestAction_Uncheck, path); }

    void        PopupClose();
};

#define IM_TOKENPASTE(x, y)     x ## y
#define IM_TOKENPASTE2(x, y)    IM_TOKENPASTE(x, y)
#define IMGUI_TEST_CONTEXT_REGISTER_DEPTH(_THIS)            ImGuiTestContextDepthRegister IM_TOKENPASTE2(depth_register, __LINE__)(_THIS)

struct ImGuiTestContextDepthRegister
{
    ImGuiTestContext*       TestContext;
    ImGuiTestContextDepthRegister(ImGuiTestContext* ctx)    { TestContext = ctx; TestContext->ActionDepth++; }
    ~ImGuiTestContextDepthRegister()                        { TestContext->ActionDepth--; }
};
