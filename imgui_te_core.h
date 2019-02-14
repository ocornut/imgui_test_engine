// dear imgui
// (test engine)

#pragma once

#include "imgui_internal.h"     // ImPool<>, ImGuiItemStatusFlags

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
struct ImGuiTestItemInfo;
struct ImGuiTestItemList;
struct ImRect;

typedef int ImGuiTestFlags;     // See ImGuiTestFlags_
typedef int ImGuiTestOpFlags;   // See ImGuiTestOpFlags_
typedef int ImGuiTestRunFlags;  // See ImGuiTestRunFlags_

//-------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------

enum ImGuiTestVerboseLevel
{
    ImGuiTestVerboseLevel_Silent    = 0,
    ImGuiTestVerboseLevel_Min       = 1,
    ImGuiTestVerboseLevel_Normal    = 2,
    ImGuiTestVerboseLevel_Max       = 3,
    ImGuiTestVerboseLevel_COUNT     = 4
};

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
    ImGuiTestFlags_NoWarmUp     = 1 << 0,       // By default, we run the GUI func twice before starting the test code
};

enum ImGuiTestOpFlags_
{
    ImGuiTestOpFlags_None                   = 0,
    ImGuiTestOpFlags_Verbose                = 1 << 0,
    ImGuiTestOpFlags_NoCheckHoveredId       = 1 << 1,
    ImGuiTestOpFlags_NoError                = 1 << 2    // Don't abort/error e.g. if the item cannot be found
};

enum ImGuiTestRunFlags_
{
    ImGuiTestRunFlags_None          = 0,
    ImGuiTestRunFlags_NoGuiFunc     = 1 << 0,
    ImGuiTestRunFlags_NoTestFunc    = 1 << 1,
    ImGuiTestRunFlags_NoSuccessMsg  = 1 << 2,
};

//-------------------------------------------------------------------------
// Hooks for Core Library
//-------------------------------------------------------------------------

void    ImGuiTestEngineHook_PreNewFrame();
void    ImGuiTestEngineHook_PostNewFrame();
void    ImGuiTestEngineHook_ItemAdd(ImGuiID id, const ImRect& bb);
void    ImGuiTestEngineHook_ItemInfo(ImGuiID id, const char* label, int flags);

//-------------------------------------------------------------------------
// Hooks for Tests
//-------------------------------------------------------------------------

// We embed every maacro in a do {} while(0) statement as a trick to allow using them as regular single statement, e.g. if (XXX) IM_CHECK(A); else IM_CHECK(B)
#define IM_CHECK_NO_ABORT(_EXPR)    do { if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, (bool)(_EXPR), #_EXPR))     { IM_ASSERT(0); } } while (0)
#define IM_CHECK(_EXPR)             do { if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, (bool)(_EXPR), #_EXPR))     { IM_ASSERT(0); } if (!(bool)(_EXPR)) return; } while (0)
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
void                ImGuiTestEngine_QueueTests(ImGuiTestEngine* engine, const char* filter = NULL);
void                ImGuiTestEngine_QueueTest(ImGuiTestEngine* engine, ImGuiTest* test, ImGuiTestRunFlags run_flags);
bool                ImGuiTestEngine_IsRunningTests(ImGuiTestEngine* engine);
bool                ImGuiTestEngine_IsRunningTest(ImGuiTestEngine* engine, ImGuiTest* test);
void                ImGuiTestEngine_PrintResultSummary(ImGuiTestEngine* engine);

// IO structure
typedef bool (*ImGuiTestEngineNewFrameFunc)(ImGuiTestEngine*, void* user_data);
typedef bool (*ImGuiTestEngineEndFrameFunc)(ImGuiTestEngine*, void* user_data);
typedef void (*ImGuiTestEngineFileOpenerFunc)(const char* filename, int line, void* user_data);

struct ImGuiTestEngineIO
{
    ImGuiTestEngineNewFrameFunc EndFrameFunc = NULL;
    ImGuiTestEngineEndFrameFunc NewFrameFunc = NULL;
    ImGuiTestEngineFileOpenerFunc FileOpenerFunc = NULL;    // (Optional) To open source files
    void*                       UserData = NULL;
                                
    // Inputs: Options          
    bool                        ConfigRunFast = true;       // Run tests as fast as possible (teleport mouse, skip delays, etc.)
    bool                        ConfigRunBlind = false;     // Run tests in a blind ImGuiContext separated from the visible context
    bool                        ConfigStopOnError = false;  // Stop queued tests on test error
    bool                        ConfigBreakOnError = false; // Break debugger on test error
    bool                        ConfigKeepTestGui = false;  // Keep test GUI running at the end of the test
    ImGuiTestVerboseLevel       ConfigVerboseLevel = ImGuiTestVerboseLevel_Normal;
    bool                        ConfigLogToTTY = false;
    bool                        ConfigTakeFocusBackAfterTests = true;
    bool                        ConfigNoThrottle = false;   // Disable vsync for performance measurement
    float                       MouseSpeed = 1000.0f;       // Mouse speed (pixel/second) when not running in fast mode
    float                       ScrollSpeed = 1600.0f;      // Scroll speed (pixel/second) when not running in fast mode
    float                       TypingSpeed = 30.0f;        // Char input speed (characters/second) when not running in fast mode
    int                         PerfStressAmount = 1;       // Integer to scale the amount of items submitted in test

    // Outputs: State           
    bool                        RunningTests = false;
};

struct ImGuiTestItemInfo
{
    int                         RefCount : 8;               // User can increment this if they want to hold on the result pointer, otherwise the task will be GC-ed.
    int                         NavLayer : 1;
    int                         Depth : 16;                 // Depth from requested parent id. 0 == ID is immediate child of requested parent id.
    int                         TimestampMain = -1;         // Timestamp of main result
    int                         TimestampStatus = -1;       // Timestamp of StatusFlags
    ImGuiID                     ID = 0;
    ImGuiID                     ParentID = 0;
    ImGuiWindow*                Window = NULL;
    ImRect                      Rect = ImRect();
    ImGuiItemStatusFlags        StatusFlags = 0;
    char                        DebugLabel[32] = "";        // Shortened label for debugging purpose

    ImGuiTestItemInfo()
    {
        RefCount = 0;
        NavLayer = 0;
        Depth = 0;
    }
};

struct ImGuiTestItemList
{
    ImPool<ImGuiTestItemInfo>   Pool;
    int&                        Size;   // THIS IS REF/POINTER to Pool.Data.Size. This codebase is totally embracing evil C++!

    void                        Clear()                 { Pool.Clear(); }
    void                        Reserve(int capacity)   { Pool.Reserve(capacity); }
    //int                       GetSize() const         { return Pool.GetSize(); }
    const ImGuiTestItemInfo*    operator[] (size_t n)   { return Pool.GetByIndex((int)n); }
    const ImGuiTestItemInfo*    GetByIndex(int n)       { return Pool.GetByIndex(n); }
    const ImGuiTestItemInfo*    GetByID(ImGuiID id)     { return Pool.GetByKey(id); }

    ImGuiTestItemList() : Size(Pool.Data.Size) {}
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
    int                     ArgVariant;
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
        ArgVariant = 0;
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

// Note: keep in sync with GetActionName()
enum ImGuiTestAction
{
    ImGuiTestAction_Unknown = 0,
    ImGuiTestAction_Click,
    ImGuiTestAction_DoubleClick,
    ImGuiTestAction_Check,
    ImGuiTestAction_Uncheck,
    ImGuiTestAction_Open,
    ImGuiTestAction_Close,
    ImGuiTestAction_COUNT
};

struct ImGuiTestRef
{
    ImGuiID                 ID;
    const char*             Path;

    ImGuiTestRef()              { ID = 0; Path = NULL; }
    ImGuiTestRef(ImGuiID id)    { ID = id; Path = NULL; }
    ImGuiTestRef(const char* p) { ID = 0; Path = p; }
};

// Generic structure with varied data. This is useful for tests to quickly share data between the GUI functions and the Test function.
struct ImGuiTestGenericState
{
    int         Int1;
    int         Int2;
    int         IntArray[10];
    float       Float1;
    float       Float2;
    float       FloatArray[10];
    bool        Bool1;
    bool        Bool2;
    bool        BoolArray[10];
    ImVec4      Vec4;
    ImVec4      Vec4Array[10];
    ImGuiID     Id;
    ImGuiID     IdArray[10];
    char        Str256[256];
    ImVector<char>  StrLarge;
    void*       Ptr1;
    void*       Ptr2;
    void*       PtrArray[10];

    ImGuiTestGenericState() { memset(this, 0, sizeof(*this)); }
};

struct ImGuiTestContext
{
    ImGuiTest*              Test = NULL;
    ImGuiTestEngine*        Engine = NULL;
    ImGuiTestEngineIO*      EngineIO = NULL;
    ImGuiContext*           UiContext = NULL;
    ImGuiTestRunFlags       RunFlags = ImGuiTestRunFlags_None;
    ImGuiTestGenericState   GenericState;
    void*                   UserData = NULL;
    int                     UserCounter = 0;
    int                     FrameCount = 0;             // Test frame count (restarts from zero every time)
    int                     ActionDepth = 0;
    bool                    Abort = false;
    char                    RefStr[256] = { 0 };
    ImGuiID                 RefID = 0;
    ImGuiInputSource        InputMode = ImGuiInputSource_Mouse;

    double                  PerfRefDt = -1.0;
    int                     PerfStressAmount = 0;

    ImGuiTestContext()
    {
    }

    ImGuiTest*  RegisterTest(const char* category, const char* name, const char* src_file = NULL, int src_line = 0);
    void        RunCurrentTest(void* user_data);
    void        Log(const char* fmt, ...) IM_FMTARGS(1);
    void        LogVerbose(const char* fmt, ...) IM_FMTARGS(1);
    void        LogDebug();
    bool        IsError() const { return Test->Status == ImGuiTestStatus_Error || Abort; }
    void        SetGuiFuncEnabled(bool v) { if (v) RunFlags &= ~ImGuiTestRunFlags_NoGuiFunc; else RunFlags |= ImGuiTestRunFlags_NoGuiFunc; }

    void        Yield();
    void        YieldFrames(int count);
    void        YieldUntil(int frame_count);
    void        Sleep(float time);
    void        SleepShort();

    void        SetInputMode(ImGuiInputSource input_mode);

    void        SetRef(ImGuiTestRef ref);
    ImGuiWindow*GetRefWindow();
    ImGuiID     GetID(ImGuiTestRef ref);
    ImGuiTestRef GetFocusWindowRef();
    ImVec2      GetMainViewportPos();

    void        MouseMove(ImGuiTestRef ref, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    void        MouseMoveToPos(ImVec2 pos);
    void        MouseClick(int button = 0);
    void        MouseDown(int button = 0);
    void        MouseUp(int button = 0);
    
    void        KeyPressMap(ImGuiKey key, int mod_flags = 0, int count = 1);
    void        KeyChars(const char* chars);
    void        KeyCharsAppend(const char* chars);
    void        KeyCharsAppendEnter(const char* chars);

    void        NavMove(ImGuiTestRef ref);
    void        NavActivate();

    void        FocusWindow(ImGuiTestRef ref);
    void        BringWindowToFront(ImGuiWindow* window);
    void        BringWindowToFrontFromItem(ImGuiTestRef ref);

    void        ScrollToY(ImGuiTestRef ref, float scroll_ratio_y = 0.5f);

    void        GatherItems(ImGuiTestItemList* out_list, ImGuiTestRef parent, int depth = -1);

    void        ItemAction(ImGuiTestAction action, ImGuiTestRef ref);
    void        ItemClick(ImGuiTestRef ref)         { ItemAction(ImGuiTestAction_Click, ref); }
    void        ItemDoubleClick(ImGuiTestRef ref)   { ItemAction(ImGuiTestAction_DoubleClick, ref); }
    void        ItemCheck(ImGuiTestRef ref)         { ItemAction(ImGuiTestAction_Check, ref); }
    void        ItemUncheck(ImGuiTestRef ref)       { ItemAction(ImGuiTestAction_Uncheck, ref); }
    void        ItemOpen(ImGuiTestRef ref)          { ItemAction(ImGuiTestAction_Open, ref); }
    void        ItemClose(ImGuiTestRef ref)         { ItemAction(ImGuiTestAction_Close, ref); }

    void        ItemActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent, int depth = -1, int passes = -1);
    void        ItemOpenAll(ImGuiTestRef ref_parent, int depth = -1, int passes = -1)    { ItemActionAll(ImGuiTestAction_Open, ref_parent, depth, passes); }
    void        ItemCloseAll(ImGuiTestRef ref_parent, int depth = -1, int passes = -1)   { ItemActionAll(ImGuiTestAction_Close, ref_parent, depth, passes); }

    void        ItemHold(ImGuiTestRef ref, float time);
    void        ItemDragAndDrop(ImGuiTestRef ref_src, ImGuiTestRef ref_dst);
    ImGuiTestItemInfo* ItemLocate(ImGuiTestRef ref, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    void        ItemVerifyCheckedIfAlive(ImGuiTestRef ref, bool checked);

    void        MenuAction(ImGuiTestAction action, ImGuiTestRef ref);
    void        MenuClick(ImGuiTestRef ref)                 { MenuAction(ImGuiTestAction_Click, ref); }
    void        MenuCheck(ImGuiTestRef ref)                 { MenuAction(ImGuiTestAction_Check, ref); }
    void        MenuUncheck(ImGuiTestRef ref)               { MenuAction(ImGuiTestAction_Uncheck, ref); }

    void        MenuActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent);
    void        MenuCheckAll(ImGuiTestRef ref_parent)       { MenuActionAll(ImGuiTestAction_Check, ref_parent); }
    void        MenuUncheckAll(ImGuiTestRef ref_parent)     { MenuActionAll(ImGuiTestAction_Uncheck, ref_parent); }

    void        WindowClose();
    void        WindowSetCollapsed(bool collapsed);
    void        WindowMove(ImVec2 pos, ImVec2 pivot = ImVec2(0.0f, 0.0f));
    void        WindowResize(ImVec2 sz);
    void        PopupClose();

    void        PerfCalcRef();
    void        PerfCapture();
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
