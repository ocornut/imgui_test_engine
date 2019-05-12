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

typedef int ImGuiTestFlags;         // See ImGuiTestFlags_
typedef int ImGuiTestOpFlags;       // See ImGuiTestOpFlags_
typedef int ImGuiTestCheckFlags;    // See ImGuiTestCheckFlags_
typedef int ImGuiTestRunFlags;      // See ImGuiTestRunFlags_

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
    ImGuiTestFlags_NoAutoFinish = 1 << 1        // By default, tests with no test func end on Frame 0 (after the warm up). Setting this require test to call ctx->Finish().
};

enum ImGuiTestOpFlags_
{
    ImGuiTestOpFlags_None               = 0,
    ImGuiTestOpFlags_Verbose            = 1 << 0,
    ImGuiTestOpFlags_NoCheckHoveredId   = 1 << 1,
    ImGuiTestOpFlags_NoError            = 1 << 2,   // Don't abort/error e.g. if the item cannot be found
    ImGuiTestOpFlags_NoFocusWindow      = 1 << 3
};

enum ImGuiTestCheckFlags_
{
    ImGuiTestCheckFlags_None            = 0,
    ImGuiTestCheckFlags_SilentSuccess   = 1 << 0
};

enum ImGuiTestRunFlags_
{
    ImGuiTestRunFlags_None              = 0,
    ImGuiTestRunFlags_NoGuiFunc         = 1 << 0,
    ImGuiTestRunFlags_NoTestFunc        = 1 << 1,
    ImGuiTestRunFlags_NoSuccessMsg      = 1 << 2,
    ImGuiTestRunFlags_NoStopOnError     = 1 << 3,
    ImGuiTestRunFlags_NoBreakOnError    = 1 << 4,
};

//-------------------------------------------------------------------------
// Hooks for Core Library
//-------------------------------------------------------------------------

void    ImGuiTestEngineHook_PreNewFrame();
void    ImGuiTestEngineHook_PostNewFrame();
void    ImGuiTestEngineHook_ItemAdd(ImGuiID id, const ImRect& bb);
void    ImGuiTestEngineHook_ItemInfo(ImGuiID id, const char* label, int flags);
void    ImGuiTestEngineHook_AssertFunc(const char* expr, const char* file, const char* function, int line);

//-------------------------------------------------------------------------
// Hooks for Tests
//-------------------------------------------------------------------------

// We embed every macro in a do {} while(0) statement as a trick to allow using them as regular single statement, e.g. if (XXX) IM_CHECK(A); else IM_CHECK(B)
// We leave the assert call (which will trigger a debugger break) outside of the check function to step out faster.
#define IM_CHECK_NO_RET(_EXPR)      do { if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, ImGuiTestCheckFlags_None, (bool)(_EXPR), #_EXPR))  { IM_ASSERT(_EXPR); } } while (0)
#define IM_CHECK(_EXPR)             do { if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, ImGuiTestCheckFlags_None, (bool)(_EXPR), #_EXPR))  { IM_ASSERT(_EXPR); } if (!(bool)(_EXPR)) return; } while (0)
#define IM_CHECK_SILENT(_EXPR)      do { if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, ImGuiTestCheckFlags_SilentSuccess, (bool)(_EXPR), #_EXPR))  { IM_ASSERT(0); } if (!(bool)(_EXPR)) return; } while (0)
#define IM_CHECK_RETV(_EXPR, _RETV) do { if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, ImGuiTestCheckFlags_None, (bool)(_EXPR), #_EXPR))  { IM_ASSERT(_EXPR); } if (!(bool)(_EXPR)) return _RETV; } while (0)
#define IM_ERRORF(_FMT,...)         do { if (ImGuiTestEngineHook_Error(__FILE__, __func__, __LINE__, ImGuiTestCheckFlags_None, _FMT, __VA_ARGS__))      { IM_ASSERT(0); } } while (0)
#define IM_ERRORF_NOHDR(_FMT,...)   do { if (ImGuiTestEngineHook_Error(NULL, NULL, 0, ImGuiTestCheckFlags_None, _FMT, __VA_ARGS__))                     { IM_ASSERT(0); } } while (0)
//#define IM_ASSERT(_EXPR)      (void)( (!!(_EXPR)) || (ImGuiTestEngineHook_Check(false, #_EXPR, __FILE__, __func__, __LINE__), 0) )

bool    ImGuiTestEngineHook_Check(const char* file, const char* func, int line, ImGuiTestCheckFlags flags, bool result, const char* expr);
bool    ImGuiTestEngineHook_Error(const char* file, const char* func, int line, ImGuiTestCheckFlags flags, const char* fmt, ...);

//-------------------------------------------------------------------------
// ImGuiTestEngine
//-------------------------------------------------------------------------

// Functions
ImGuiTestEngine*    ImGuiTestEngine_CreateContext(ImGuiContext* imgui_context);
void                ImGuiTestEngine_ShutdownContext(ImGuiTestEngine* engine);
ImGuiTestEngineIO&  ImGuiTestEngine_GetIO(ImGuiTestEngine* engine);
void                ImGuiTestEngine_Abort(ImGuiTestEngine* engine);
void                ImGuiTestEngine_ShowTestWindow(ImGuiTestEngine* engine, bool* p_open);
ImGuiTest*          ImGuiTestEngine_RegisterTest(ImGuiTestEngine* engine, const char* category, const char* name, const char* src_file = NULL, int src_line = 0);
void                ImGuiTestEngine_QueueTests(ImGuiTestEngine* engine, const char* filter = NULL);
void                ImGuiTestEngine_QueueTest(ImGuiTestEngine* engine, ImGuiTest* test, ImGuiTestRunFlags run_flags);
bool                ImGuiTestEngine_IsRunningTests(ImGuiTestEngine* engine);
bool                ImGuiTestEngine_IsRunningTest(ImGuiTestEngine* engine, ImGuiTest* test);
void                ImGuiTestEngine_CalcSourceLineEnds(ImGuiTestEngine* engine);
void                ImGuiTestEngine_PrintResultSummary(ImGuiTestEngine* engine);

// IO structure
typedef bool (*ImGuiTestEngineNewFrameFunc)(ImGuiTestEngine*, void* user_data);
typedef bool (*ImGuiTestEngineEndFrameFunc)(ImGuiTestEngine*, void* user_data);
typedef void (*ImGuiTestEngineFileOpenerFunc)(const char* filename, int line, void* user_data);

struct ImGuiTestEngineIO
{
    ImGuiTestEngineEndFrameFunc EndFrameFunc = NULL;
    ImGuiTestEngineNewFrameFunc NewFrameFunc = NULL;
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
    int                     SourceLineEnd;      //
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
        SourceLine = SourceLineEnd = 0;
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
    ImGuiTestAction_Input,
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

struct ImGuiTestGenericStatus
{
    int     Ret;
    int     Hovered;
    int     Active;
    int     Focused;
    int     Clicked;
    int     Visible;
    int     Activated;
    int     Deactivated;
    int     DeactivatedAfterEdit;

    ImGuiTestGenericStatus()    { Clear(); }
    void Clear()                { memset(this, 0, sizeof(*this)); }
    void QuerySet(bool ret_val = false) { Clear(); QueryInc(ret_val); }
    void QueryInc(bool ret_val = false)
    { 
        Ret += ret_val;
        Hovered += ImGui::IsItemHovered();
        Active += ImGui::IsItemActive();;
        Focused += ImGui::IsItemFocused();
        Clicked += ImGui::IsItemClicked();
        Visible += ImGui::IsItemVisible();
        Activated += ImGui::IsItemActivated();
        Deactivated += ImGui::IsItemDeactivated();
        DeactivatedAfterEdit += ImGui::IsItemDeactivatedAfterEdit();
    }
};

// Generic structure with varied data. This is useful for tests to quickly share data between the GUI functions and the Test function.
// This is however totally optional. Using a RunFunc it is possible to store custom data on the stack and read from it as UserData.
struct ImGuiTestGenericVars
{
    int             Int1;
    int             Int2;
    int             IntArray[10];
    float           Float1;
    float           Float2;
    float           FloatArray[10];
    bool            Bool1;
    bool            Bool2;
    bool            BoolArray[10];
    ImVec4          Vec4;
    ImVec4          Vec4Array[10];
    ImGuiID         Id;
    ImGuiID         IdArray[10];
    char            Str1[256];
    char            Str2[256];
    ImVector<char>  StrLarge;
    void*           Ptr1;
    void*           Ptr2;
    void*           PtrArray[10];
    ImGuiID         DockId;
    ImGuiTestGenericStatus  Status;

    ImGuiTestGenericVars()  { Clear(); }
    void Clear()            { StrLarge.clear(); memset(this, 0, sizeof(*this)); }
    void ClearInts()        { Int1 = Int2 = 0; memset(IntArray, 0, sizeof(IntArray)); }
    void ClearBools()       { Bool2 = Bool2 = false; memset(BoolArray, 0, sizeof(BoolArray)); }
};

enum ImGuiTestActiveFunc
{
    ImGuiTestActiveFunc_None,
    ImGuiTestActiveFunc_GuiFunc,
    ImGuiTestActiveFunc_TestFunc
};

struct ImGuiTestContext
{
    ImGuiTest*              Test = NULL;
    ImGuiTestEngine*        Engine = NULL;
    ImGuiTestEngineIO*      EngineIO = NULL;
    ImGuiContext*           UiContext = NULL;
    ImGuiTestRunFlags       RunFlags = ImGuiTestRunFlags_None;
    ImGuiTestGenericVars    GenericVars;
    ImGuiTestActiveFunc     ActiveFunc = ImGuiTestActiveFunc_None;
    void*                   UserData = NULL;
    int                     UserCounter = 0;
    int                     FrameCount = 0;             // Test frame count (restarts from zero every time)
    int                     FirstFrameCount = 0;        // This is generally -2 or 0 depending on whether we have warm up enabled
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

    void        RunCurrentTest(void* user_data);
    void        Log(const char* fmt, ...) IM_FMTARGS(1);
    void        LogVerbose(const char* fmt, ...) IM_FMTARGS(1);
    void        LogDebug();
    void        Finish();
    bool        IsError() const             { return Test->Status == ImGuiTestStatus_Error || Abort; }
    bool        IsFirstFrame() const        { return FrameCount == FirstFrameCount; }
    void        SetGuiFuncEnabled(bool v)   { if (v) RunFlags &= ~ImGuiTestRunFlags_NoGuiFunc; else RunFlags |= ImGuiTestRunFlags_NoGuiFunc; }
    void        RecoverFromUiContextErrors();

    void        Yield();
    void        YieldFrames(int count);
    void        YieldUntil(int frame_count);
    void        Sleep(float time);
    void        SleepDebugNoSkip(float time);
    void        SleepShort();

    void        SetInputMode(ImGuiInputSource input_mode);

    void        SetRef(ImGuiTestRef ref);
    ImGuiWindow*GetWindowByRef(ImGuiTestRef ref);
    ImGuiID     GetID(ImGuiTestRef ref);
    ImGuiID     GetID(ImGuiTestRef seed_ref, ImGuiTestRef ref);
    ImGuiTestRef GetFocusWindowRef();
    ImVec2      GetMainViewportPos();

    void        MouseMove(ImGuiTestRef ref, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    void        MouseMoveToPos(ImVec2 pos);
    void        MouseMoveToPosInsideWindow(ImVec2* pos, ImGuiWindow* window);
    void        MouseClick(int button = 0);
    void        MouseDoubleClick(int button = 0);
    void        MouseDown(int button = 0);
    void        MouseUp(int button = 0);
    void        MouseLiftDragThreshold(int button = 0);
    
    void        KeyDownMap(ImGuiKey key, int mod_flags = 0);
    void        KeyUpMap(ImGuiKey key, int mod_flags = 0);
    void        KeyPressMap(ImGuiKey key, int mod_flags = 0, int count = 1);
    void        KeyChars(const char* chars);
    void        KeyCharsAppend(const char* chars);
    void        KeyCharsAppendEnter(const char* chars);

    void        NavMove(ImGuiTestRef ref);
    void        NavActivate();
    void        NavInput();

    void        FocusWindow(ImGuiTestRef ref);
    bool        BringWindowToFront(ImGuiWindow* window, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    bool        BringWindowToFrontFromItem(ImGuiTestRef ref);

    void        ScrollToY(ImGuiTestRef ref, float scroll_ratio_y = 0.5f);

    void        GatherItems(ImGuiTestItemList* out_list, ImGuiTestRef parent, int depth = -1);

    void        ItemAction(ImGuiTestAction action, ImGuiTestRef ref);
    void        ItemClick(ImGuiTestRef ref)         { ItemAction(ImGuiTestAction_Click, ref); }
    void        ItemDoubleClick(ImGuiTestRef ref)   { ItemAction(ImGuiTestAction_DoubleClick, ref); }
    void        ItemCheck(ImGuiTestRef ref)         { ItemAction(ImGuiTestAction_Check, ref); }
    void        ItemUncheck(ImGuiTestRef ref)       { ItemAction(ImGuiTestAction_Uncheck, ref); }
    void        ItemOpen(ImGuiTestRef ref)          { ItemAction(ImGuiTestAction_Open, ref); }
    void        ItemClose(ImGuiTestRef ref)         { ItemAction(ImGuiTestAction_Close, ref); }
    void        ItemInput(ImGuiTestRef ref)         { ItemAction(ImGuiTestAction_Input, ref); }

    void        ItemActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent, int depth = -1, int passes = -1);
    void        ItemOpenAll(ImGuiTestRef ref_parent, int depth = -1, int passes = -1)    { ItemActionAll(ImGuiTestAction_Open, ref_parent, depth, passes); }
    void        ItemCloseAll(ImGuiTestRef ref_parent, int depth = -1, int passes = -1)   { ItemActionAll(ImGuiTestAction_Close, ref_parent, depth, passes); }

    void        ItemHold(ImGuiTestRef ref, float time);
    void        ItemHoldForFrames(ImGuiTestRef ref, int frames);
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

    // FIXME-TESTS: Refactor this horrible mess... perhaps all functions should have a ImGuiTestRef defaulting to empty?
    void        WindowClose();
    void        WindowSetCollapsed(ImGuiTestRef ref, bool collapsed);
    void        WindowMove(ImGuiTestRef ref, ImVec2 pos, ImVec2 pivot = ImVec2(0.0f, 0.0f));
    void        WindowResize(ImGuiTestRef ref, ImVec2 sz);
    void        WindowMoveToMakePosVisible(ImGuiWindow* window, ImVec2 pos);
    void        PopupClose();

#ifdef IMGUI_HAS_DOCK
    void        DockWindowInto(const char* window_src, const char* window_dst, ImGuiDir split_dir = ImGuiDir_None);
    void        DockMultiClear(const char* window_name, ...);
    void        DockMultiSet(ImGuiID dock_id, const char* window_name, ...);
    ImGuiID     DockMultiSetupBasic(ImGuiID dock_id, const char* window_name, ...);
    bool        DockIdIsUndockedOrStandalone(ImGuiID dock_id);
#endif

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
