// dear imgui
// (test engine)

#pragma once

#include "stdio.h"              // FILE
#include "imgui_internal.h"     // ImPool<>, ImGuiItemStatusFlags, ImFormatString
#include "imgui_te_util.h"

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
typedef int ImGuiTestCheckFlags;    // See ImGuiTestCheckFlags_
typedef int ImGuiTestLogFlags;      // See ImGuiTestLogFlags_
typedef int ImGuiTestOpFlags;       // See ImGuiTestOpFlags_
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

enum ImGuiTestCheckFlags_
{
    ImGuiTestCheckFlags_None            = 0,
    ImGuiTestCheckFlags_SilentSuccess   = 1 << 0
};

enum ImGuiTestLogFlags_
{
    ImGuiTestLogFlags_None              = 0,
    ImGuiTestLogFlags_NoHeader          = 1 << 0,   // Do not display framecount and depth padding
    ImGuiTestLogFlags_Verbose           = 1 << 1
};

enum ImGuiTestOpFlags_
{
    ImGuiTestOpFlags_None               = 0,
    ImGuiTestOpFlags_Verbose            = 1 << 0,
    ImGuiTestOpFlags_NoCheckHoveredId   = 1 << 1,
    ImGuiTestOpFlags_NoError            = 1 << 2,   // Don't abort/error e.g. if the item cannot be found
    ImGuiTestOpFlags_NoFocusWindow      = 1 << 3
};

enum ImGuiTestRunFlags_
{
    ImGuiTestRunFlags_None              = 0,
    ImGuiTestRunFlags_NoGuiFunc         = 1 << 0,
    ImGuiTestRunFlags_NoTestFunc        = 1 << 1,
    ImGuiTestRunFlags_NoSuccessMsg      = 1 << 2,
    ImGuiTestRunFlags_NoStopOnError     = 1 << 3,
    ImGuiTestRunFlags_NoBreakOnError    = 1 << 4,
    ImGuiTestRunFlags_CommandLine       = 1 << 5
};

enum ImGuiTestInputType
{
    ImGuiTestInputType_None,
    ImGuiTestInputType_Key,
    ImGuiTestInputType_Nav,
    ImGuiTestInputType_Char
};

struct ImGuiTestRef
{
    ImGuiID                 ID;
    const char*             Path;

    ImGuiTestRef() { ID = 0; Path = NULL; }
    ImGuiTestRef(ImGuiID id) { ID = id; Path = NULL; }
    ImGuiTestRef(const char* p) { ID = 0; Path = p; }
};

struct ImGuiTestInput
{
    ImGuiTestInputType      Type;
    ImGuiKey                Key = ImGuiKey_COUNT;
    ImGuiKeyModFlags        KeyMods = ImGuiKeyModFlags_None;
    ImGuiNavInput           NavInput = ImGuiNavInput_COUNT;
    ImWchar                 Char = 0;
    ImGuiKeyState           State = ImGuiKeyState_Unknown;;

    static ImGuiTestInput   FromKey(ImGuiKey v, ImGuiKeyState state, ImGuiKeyModFlags mods = ImGuiKeyModFlags_None)
    {
        ImGuiTestInput inp;
        inp.Type = ImGuiTestInputType_Key;
        inp.Key = v;
        inp.KeyMods = mods;
        inp.State = state;
        return inp;
    }
    static ImGuiTestInput   FromNav(ImGuiNavInput v, ImGuiKeyState state)
    {
        ImGuiTestInput inp;
        inp.Type = ImGuiTestInputType_Nav;
        inp.NavInput = v;
        inp.State = state;
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

struct ImGuiTestInputs
{
    ImGuiIO                     SimulatedIO;
    int                         ApplyingSimulatedIO = 0;
    ImVec2                      MousePosValue;             // Own non-rounded copy of MousePos in order facilitate simulating mouse movement very slow speed and high-framerate
    ImVec2                      HostLastMousePos;
    int                         MouseButtonsValue = 0x00;  // FIXME-TESTS: Use simulated_io.MouseDown[] ?
    ImGuiKeyModFlags            KeyMods = 0x00;            // FIXME-TESTS: Use simulated_io.KeyXXX ?
    ImVector<ImGuiTestInput>    Queue;
};

//-------------------------------------------------------------------------
// Internal function
//-------------------------------------------------------------------------

ImGuiTestItemInfo*   ImGuiTestEngine_ItemLocate(ImGuiTestEngine* engine, ImGuiID id, const char* debug_id);
void                 ImGuiTestEngine_PushInput(ImGuiTestEngine* engine, const ImGuiTestInput& input);
void                 ImGuiTestEngine_Yield(ImGuiTestEngine* engine);
int                  ImGuiTestEngine_GetFrameCount(ImGuiTestEngine* engine);
double               ImGuiTestEngine_GetPerfDeltaTime500Average(ImGuiTestEngine* engine);
FILE*                ImGuiTestEngine_GetPerfPersistentLogCsv(ImGuiTestEngine* engine);

//-------------------------------------------------------------------------
// Hooks for Core Library
//-------------------------------------------------------------------------

void    ImGuiTestEngineHook_PreNewFrame(ImGuiContext* ctx);
void    ImGuiTestEngineHook_PostNewFrame(ImGuiContext* ctx);
void    ImGuiTestEngineHook_ItemAdd(ImGuiContext* ctx, const ImRect& bb, ImGuiID id);
void    ImGuiTestEngineHook_ItemInfo(ImGuiContext* ctx, ImGuiID id, const char* label, ImGuiItemStatusFlags flags);
void    ImGuiTestEngineHook_Log(ImGuiContext* ctx, const char* fmt, ...);
void    ImGuiTestEngineHook_AssertFunc(const char* expr, const char* file, const char* function, int line);

//-------------------------------------------------------------------------
// Hooks for Tests
//-------------------------------------------------------------------------

// We embed every macro in a do {} while(0) statement as a trick to allow using them as regular single statement, e.g. if (XXX) IM_CHECK(A); else IM_CHECK(B)
// We leave the assert call (which will trigger a debugger break) outside of the check function to step out faster.
#define IM_CHECK_NO_RET(_EXPR)      do { if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, ImGuiTestCheckFlags_None, (bool)(_EXPR), #_EXPR))          { IM_ASSERT(_EXPR); } } while (0)
#define IM_CHECK(_EXPR)             do { if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, ImGuiTestCheckFlags_None, (bool)(_EXPR), #_EXPR))          { IM_ASSERT(_EXPR); } if (!(bool)(_EXPR)) return; } while (0)
#define IM_CHECK_SILENT(_EXPR)      do { if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, ImGuiTestCheckFlags_SilentSuccess, (bool)(_EXPR), #_EXPR)) { IM_ASSERT(0); } if (!(bool)(_EXPR)) return; } while (0)
#define IM_CHECK_RETV(_EXPR, _RETV) do { if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, ImGuiTestCheckFlags_None, (bool)(_EXPR), #_EXPR))          { IM_ASSERT(_EXPR); } if (!(bool)(_EXPR)) return _RETV; } while (0)
#define IM_ERRORF(_FMT,...)         do { if (ImGuiTestEngineHook_Error(__FILE__, __func__, __LINE__, ImGuiTestCheckFlags_None, _FMT, __VA_ARGS__))              { IM_ASSERT(0); } } while (0)
#define IM_ERRORF_NOHDR(_FMT,...)   do { if (ImGuiTestEngineHook_Error(NULL, NULL, 0, ImGuiTestCheckFlags_None, _FMT, __VA_ARGS__))                             { IM_ASSERT(0); } } while (0)

template<typename T> void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, T value)         { /*static_assert(false, "Append function not defined");*/ }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, const char* value)  { buf.appendf("%s", value); }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, int value)          { buf.appendf("%d", value); }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, short value)        { buf.appendf("%hd", value); }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, unsigned int value) { buf.appendf("%u", value); }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, float value)        { buf.appendf("%f", value); }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, ImVec2 value)       { buf.appendf("(%f, %f)", value.x, value.y); }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, const void* value)  { buf.appendf("%p", value); }

// Those macros allow us to print out the values of both lhs and rhs expressions involved in a check.
#define IM_CHECK_OP(_LHS, _RHS, _OP)                                        \
    do                                                                      \
    {                                                                       \
        auto __lhs = _LHS;  /* Cache in variables to avoid side effects */  \
        auto __rhs = _RHS;                                                  \
        bool __res = __lhs _OP __rhs;                                       \
        ImGuiTextBuffer value_expr_buf;                                     \
        if (!__res)                                                         \
        {                                                                   \
            ImGuiTestEngineUtil_AppendStrValue(value_expr_buf, __lhs);      \
            value_expr_buf.append(" " #_OP " ");                            \
            ImGuiTestEngineUtil_AppendStrValue(value_expr_buf, __rhs);      \
        }                                                                   \
        if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, ImGuiTestCheckFlags_None, __res, #_LHS " " #_OP " " #_RHS, value_expr_buf.c_str())) \
            IM_ASSERT(__res);                                               \
        if (!__res)                                                         \
            return;                                                         \
    } while (0)
#define IM_CHECK_EQ(_LHS, _RHS)         IM_CHECK_OP(_LHS, _RHS, ==)     // Equal
#define IM_CHECK_NE(_LHS, _RHS)         IM_CHECK_OP(_LHS, _RHS, !=)     // Not Equal
#define IM_CHECK_LT(_LHS, _RHS)         IM_CHECK_OP(_LHS, _RHS, <)      // Less Than
#define IM_CHECK_LE(_LHS, _RHS)         IM_CHECK_OP(_LHS, _RHS, <=)     // Less or Equal
#define IM_CHECK_GT(_LHS, _RHS)         IM_CHECK_OP(_LHS, _RHS, >)      // Greater Than
#define IM_CHECK_GE(_LHS, _RHS)         IM_CHECK_OP(_LHS, _RHS, >=)     // Greater or Equal

#define IM_CHECK_STR_EQ(_LHS, _RHS)                                         \
    do                                                                      \
    {                                                                       \
        bool __res = strcmp(_LHS, _RHS) == 0;                               \
        ImGuiTextBuffer value_expr_buf;                                     \
        if (!__res)                                                         \
        {                                                                   \
            value_expr_buf.appendf("\"%s\" == \"%s\"", _LHS, _RHS);         \
        }                                                                   \
        if (ImGuiTestEngineHook_Check(__FILE__, __func__, __LINE__, ImGuiTestCheckFlags_None, __res, #_LHS " == " #_RHS, value_expr_buf.c_str())) \
            IM_ASSERT(__res);                                               \
        if (!__res)                                                         \
            return;                                                         \
    } while (0)

//#define IM_ASSERT(_EXPR)      (void)( (!!(_EXPR)) || (ImGuiTestEngineHook_Check(false, #_EXPR, __FILE__, __func__, __LINE__), 0) )

bool    ImGuiTestEngineHook_Check(const char* file, const char* func, int line, ImGuiTestCheckFlags flags, bool result, const char* expr, char const* value_expr = NULL);
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
void                ImGuiTestEngine_QueueTests(ImGuiTestEngine* engine, const char* filter = NULL, ImGuiTestRunFlags run_flags = 0);
void                ImGuiTestEngine_QueueTest(ImGuiTestEngine* engine, ImGuiTest* test, ImGuiTestRunFlags run_flags);
bool                ImGuiTestEngine_IsRunningTests(ImGuiTestEngine* engine);
bool                ImGuiTestEngine_IsRunningTest(ImGuiTestEngine* engine, ImGuiTest* test);
void                ImGuiTestEngine_CalcSourceLineEnds(ImGuiTestEngine* engine);
void                ImGuiTestEngine_PrintResultSummary(ImGuiTestEngine* engine);
void                ImGuiTestEngine_GetResult(ImGuiTestEngine* engine, int& count_tested, int& success_count);

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
    ImRect                      RectFull = ImRect();
    ImRect                      RectClipped = ImRect();
    ImGuiItemStatusFlags        StatusFlags = 0;
    char                        DebugLabel[32] = {};        // Shortened label for debugging purpose

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

// Gather items in given parent scope.
struct ImGuiTestGatherTask
{
    ImGuiID                 ParentID = 0;
    int                     Depth = 0;
    ImGuiTestItemList*      OutList = NULL;
    ImGuiTestItemInfo*      LastItemInfo = NULL;
};

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

//-------------------------------------------------------------------------
// ImGuiTest
//-------------------------------------------------------------------------

typedef void    (*ImGuiTestRunFunc)(ImGuiTestContext* ctx);
typedef void    (*ImGuiTestGuiFunc)(ImGuiTestContext* ctx);
typedef void    (*ImGuiTestTestFunc)(ImGuiTestContext* ctx);

// Wraps a placement new of a given type (where 'buffer' is the allocated memory)
typedef void    (*ImGuiTestUserDataConstructor)(void* buffer);
typedef void    (*ImGuiTestUserDataDestructor)(void* ptr);

struct ImGuiTest
{
    const char*                     Category;           // Literal, not owned
    const char*                     Name;               // Literal, not owned
    const char*                     SourceFile;         // __FILE__
    const char*                     SourceFileShort;    // Pointer within SourceFile, skips filename.
    int                             SourceLine;         // __LINE__
    int                             SourceLineEnd;      //
    int                             ArgVariant;
    size_t                          UserDataSize;
    ImGuiTestUserDataConstructor    UserDataConstructor;
    ImGuiTestUserDataDestructor     UserDataDestructor;
    ImGuiTestStatus                 Status;
    ImGuiTestFlags                  Flags;
    ImGuiTestRunFunc                RootFunc;           // NULL function is ok
    ImGuiTestGuiFunc                GuiFunc;            // GUI functions can be reused
    ImGuiTestTestFunc               TestFunc;
    ImGuiTextBuffer                 TestLog;
    bool                            TestLogScrollToBottom;

    ImGuiTest()
    {
        Category = NULL;
        Name = NULL;
        SourceFile = SourceFileShort = NULL;
        SourceLine = SourceLineEnd = 0;
        ArgVariant = 0;
        UserDataSize = 0;
        UserDataConstructor = NULL;
        UserDataDestructor = NULL;
        Status = ImGuiTestStatus_Unknown;
        Flags = ImGuiTestFlags_None;
        RootFunc = NULL;
        GuiFunc = NULL;
        TestFunc = NULL;
        TestLogScrollToBottom = false;
    }

    template <typename T>
    void SetUserDataType()
    {
        UserDataSize = sizeof(T);
        UserDataConstructor = [](void* buffer) { IM_PLACEMENT_NEW(buffer) T; };
        UserDataDestructor = [](void* ptr) { IM_UNUSED(ptr); reinterpret_cast<T*>(ptr)->~T(); };
    }
};
