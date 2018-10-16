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
    ImGuiTestFlags_NoWarmUp     = 1 << 0        // By default, we run the gui func twice before starting the test code
};

//-------------------------------------------------------------------------
// Hooks for Core Library
//-------------------------------------------------------------------------

void    ImGuiTestEngineHook_PreNewFrame();
void    ImGuiTestEngineHook_PostNewFrame();
void    ImGuiTestEngineHook_ItemAdd(const ImRect& bb, ImGuiID id, const ImRect* nav_bb_arg);

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

// IO structure
typedef bool (*ImGuiTestEngineNewFrameFunc)(ImGuiTestEngine*, void* user_data);

struct ImGuiTestEngineIO
{
    ImGuiTestEngineNewFrameFunc     EndFrameFunc        = NULL;
    ImGuiTestEngineNewFrameFunc     NewFrameFunc        = NULL;
	void*                           UserData            = NULL;

    // Options
    bool                            ConfigRunFast       = true;     // Run tests as fast as possible (teleport mouse, skip delays, etc.)
    bool                            ConfigRunBlind      = false;    // Run tests in a blind ImGuiContext separated from the visible context
    bool                            ConfigBreakOnError  = false;    // Break debugger on test error
    bool                            ConfigLogVerbose    = false;    // Verbose log
    float                           MouseSpeed          = 1000.0f;  // Mouse speed (pixel/second) when not running in fast mode
    float                           ScrollSpeed         = 1000.0f;  // Scroll speed (pixel/second) when not running in fast mode
};

//-------------------------------------------------------------------------
// ImGuiTest
//-------------------------------------------------------------------------

typedef void    (*ImGuiTestRunFunc)(ImGuiTestContext* ctx);
typedef void    (*ImGuiTestGuiFunc)(ImGuiTestContext* ctx);
typedef void    (*ImGuiTestTestFunc)(ImGuiTestContext* ctx);

struct ImGuiTest
{
    const char*             Category;       // Literal, not owned
    const char*             Name;           // Literal, not owned
    ImGuiTestStatus         Status;
    ImGuiTestFlags          Flags;
    ImGuiTestRunFunc        RootFunc;       // NULL function is ok
    ImGuiTestGuiFunc        GuiFunc;        // GUI functions can be reused
    ImGuiTestTestFunc       TestFunc;
    ImGuiTextBuffer         TestLog;

    ImGuiTest()
    {
        Category = NULL;
        Name = NULL;
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

    ImGuiTest*  RegisterTest(const char* category, const char* name);
    void        RunCurrentTest(void* user_data);
    void        Log(const char* fmt, ...) IM_FMTARGS(1);
    void        LogVerbose(const char* fmt, ...) IM_FMTARGS(1);
    bool        IsError() const { return Test->Status == ImGuiTestStatus_Error || Abort; }

    void        Yield();
    void        Sleep(float time);
    void        SleepShort();

    void        SetRef(const char* str_id);

    void        MouseMove(const char* str_id);
    void        MouseMove(ImVec2 pos);
    void        MouseClick(int button = 0);

    void        FocusWindowForItem(const char* str_id);

    void        ItemClick(const char* str_id);
    void        ItemHold(const char* str_id, float time);
    void        ItemOpen(const char* str_id);
    ImGuiTestLocateResult* ItemLocate(const char* str_id);

    void        MenuItemClick(const char* menu_path);

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
