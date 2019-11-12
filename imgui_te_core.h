// dear imgui
// (test engine, core)

#pragma once

#include <stdio.h>                  // FILE
#include "imgui_internal.h"         // ImPool<>, ImGuiItemStatusFlags, ImFormatString
#include "imgui_te_util.h"
#include "imgui_capture_tool.h"

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
    ImGuiTestVerboseLevel_Error     = 1,
    ImGuiTestVerboseLevel_Warning   = 2,
    ImGuiTestVerboseLevel_Info      = 3,
    ImGuiTestVerboseLevel_Debug     = 4,
    ImGuiTestVerboseLevel_Trace     = 5,
    ImGuiTestVerboseLevel_COUNT     = 6
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
    ImGuiTestLogFlags_NoHeader          = 1 << 0    // Do not display framecount and depth padding
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
    ImGuiTestRunFlags_ManualRun         = 1 << 5,
    ImGuiTestRunFlags_CommandLine       = 1 << 6
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

    ImGuiTestRef()              { ID = 0; Path = NULL; }
    ImGuiTestRef(ImGuiID id)    { ID = id; Path = NULL; }
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

ImGuiTestItemInfo*  ImGuiTestEngine_ItemLocate(ImGuiTestEngine* engine, ImGuiID id, const char* debug_id);
void                ImGuiTestEngine_PushInput(ImGuiTestEngine* engine, const ImGuiTestInput& input);
void                ImGuiTestEngine_Yield(ImGuiTestEngine* engine);
void                ImGuiTestEngine_SetDeltaTime(ImGuiTestEngine* engine, float delta_time);
int                 ImGuiTestEngine_GetFrameCount(ImGuiTestEngine* engine);
double              ImGuiTestEngine_GetPerfDeltaTime500Average(ImGuiTestEngine* engine);
FILE*               ImGuiTestEngine_GetPerfPersistentLogCsv(ImGuiTestEngine* engine);
const char*         ImGuiTestEngine_GetVerboseLevelName(ImGuiTestVerboseLevel v);
bool                ImGuiTestEngine_CaptureWindow(ImGuiTestEngine* engine, ImGuiWindow* window, const char* output_file, ImGuiCaptureToolFlags flags);

//-------------------------------------------------------------------------
// Hooks for Core Library
//-------------------------------------------------------------------------

void                ImGuiTestEngineHook_PreNewFrame(ImGuiContext* ctx);
void                ImGuiTestEngineHook_PostNewFrame(ImGuiContext* ctx);
void                ImGuiTestEngineHook_ItemAdd(ImGuiContext* ctx, const ImRect& bb, ImGuiID id);
void                ImGuiTestEngineHook_ItemInfo(ImGuiContext* ctx, ImGuiID id, const char* label, ImGuiItemStatusFlags flags);
void                ImGuiTestEngineHook_Log(ImGuiContext* ctx, const char* fmt, ...);
void                ImGuiTestEngineHook_AssertFunc(const char* expr, const char* file, const char* function, int line);

//-------------------------------------------------------------------------
// Macros for Tests
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
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, bool value)         { buf.append(value ? "true" : "false"); }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, int value)          { buf.appendf("%d", value); }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, short value)        { buf.appendf("%hd", value); }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, unsigned int value) { buf.appendf("%u", value); }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, float value)        { buf.appendf("%f", value); }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, ImVec2 value)       { buf.appendf("(%f, %f)", value.x, value.y); }
template<> inline void ImGuiTestEngineUtil_AppendStrValue(ImGuiTextBuffer& buf, const void* value)  { buf.appendf("%p", value); }

// Those macros allow us to print out the values of both lhs and rhs expressions involved in a check.
#define IM_CHECK_OP_NO_RET(_LHS, _RHS, _OP)                                 \
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
    } while (0)
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

#define IM_CHECK_EQ(_LHS, _RHS)         IM_CHECK_OP(_LHS, _RHS, ==)         // Equal
#define IM_CHECK_NE(_LHS, _RHS)         IM_CHECK_OP(_LHS, _RHS, !=)         // Not Equal
#define IM_CHECK_LT(_LHS, _RHS)         IM_CHECK_OP(_LHS, _RHS, <)          // Less Than
#define IM_CHECK_LE(_LHS, _RHS)         IM_CHECK_OP(_LHS, _RHS, <=)         // Less or Equal
#define IM_CHECK_GT(_LHS, _RHS)         IM_CHECK_OP(_LHS, _RHS, >)          // Greater Than
#define IM_CHECK_GE(_LHS, _RHS)         IM_CHECK_OP(_LHS, _RHS, >=)         // Greater or Equal

#define IM_CHECK_EQ_NO_RET(_LHS, _RHS)  IM_CHECK_OP_NO_RET(_LHS, _RHS, ==)  // Equal
#define IM_CHECK_NE_NO_RET(_LHS, _RHS)  IM_CHECK_OP_NO_RET(_LHS, _RHS, !=)  // Not Equal
#define IM_CHECK_LT_NO_RET(_LHS, _RHS)  IM_CHECK_OP_NO_RET(_LHS, _RHS, <)   // Less Than
#define IM_CHECK_LE_NO_RET(_LHS, _RHS)  IM_CHECK_OP_NO_RET(_LHS, _RHS, <=)  // Less or Equal
#define IM_CHECK_GT_NO_RET(_LHS, _RHS)  IM_CHECK_OP_NO_RET(_LHS, _RHS, >)   // Greater Than
#define IM_CHECK_GE_NO_RET(_LHS, _RHS)  IM_CHECK_OP_NO_RET(_LHS, _RHS, >=)  // Greater or Equal

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
// ImGuiTestEngine API
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
typedef void (*ImGuiTestEngineSrcFileOpenFunc)(const char* filename, int line, void* user_data);
typedef bool (*ImGuiTestEngineScreenCaptureFunc)(int x, int y, int w, int h, unsigned int* pixels, void* user_data);

struct ImGuiTestEngineIO
{
    ImGuiTestEngineEndFrameFunc         EndFrameFunc = NULL;
    ImGuiTestEngineNewFrameFunc         NewFrameFunc = NULL;
    ImGuiTestEngineSrcFileOpenFunc      SrcFileOpenFunc = NULL; // (Optional) To open source files
    ImGuiTestEngineScreenCaptureFunc    ScreenCaptureFunc = NULL;
    void*                               UserData = NULL;
                                
    // Inputs: Options          
    bool                        ConfigRunFast = true;       // Run tests as fast as possible (teleport mouse, skip delays, etc.)
    bool                        ConfigRunBlind = false;     // Run tests in a blind ImGuiContext separated from the visible context
    bool                        ConfigStopOnError = false;  // Stop queued tests on test error
    bool                        ConfigBreakOnError = false; // Break debugger on test error
    bool                        ConfigKeepTestGui = false;  // Keep test GUI running at the end of the test
    ImGuiTestVerboseLevel       ConfigVerboseLevel = ImGuiTestVerboseLevel_Warning;
    ImGuiTestVerboseLevel       ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Info;
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

// Result of an ItemLocate query
struct ImGuiTestItemInfo
{
    int                         RefCount : 8;               // User can increment this if they want to hold on the result pointer across frames, otherwise the task will be GC-ed.
    int                         NavLayer : 1;               // Nav layer of the item
    int                         Depth : 16;                 // Depth from requested parent id. 0 == ID is immediate child of requested parent id.
    int                         TimestampMain = -1;         // Timestamp of main result (all fields)
    int                         TimestampStatus = -1;       // Timestamp of StatusFlags
    ImGuiID                     ID = 0;                     // Item ID
    ImGuiID                     ParentID = 0;               // Item Parent ID (value at top of the ID stack)
    ImGuiWindow*                Window = NULL;              // Item Window
    ImRect                      RectFull = ImRect();        // Item Rectangle
    ImRect                      RectClipped = ImRect();     // Item Rectangle (clipped with window->ClipRect at time of item submission)
    ImGuiItemStatusFlags        StatusFlags = 0;            // Item Status flags (fully updated for some items only, compare TimestampStatus to FrameCount)
    char                        DebugLabel[32] = {};        // Shortened label for debugging purpose

    ImGuiTestItemInfo()
    {
        RefCount = 0;
        NavLayer = 0;
        Depth = 0;
    }
};

// Result of an ItemGather query
struct ImGuiTestItemList
{
    ImPool<ImGuiTestItemInfo>   Pool;
    int&                        Size;           // FIXME: THIS IS REF/POINTER to Pool.Buf.Size! This codebase is totally embracing evil C++!

    void                        Clear()                 { Pool.Clear(); }
    void                        Reserve(int capacity)   { Pool.Reserve(capacity); }
    //int                       GetSize() const         { return Pool.GetSize(); }
    const ImGuiTestItemInfo*    operator[] (size_t n)   { return Pool.GetByIndex((int)n); }
    const ImGuiTestItemInfo*    GetByIndex(int n)       { return Pool.GetByIndex(n); }
    const ImGuiTestItemInfo*    GetByID(ImGuiID id)     { return Pool.GetByKey(id); }

    ImGuiTestItemList() : Size(Pool.Buf.Size) {} // FIXME: THIS IS REF/POINTER to Pool.Buf.Size!
};

// Gather items in given parent scope.
struct ImGuiTestGatherTask
{
    ImGuiID                 ParentID = 0;
    int                     Depth = 0;
    ImGuiTestItemList*      OutList = NULL;
    ImGuiTestItemInfo*      LastItemInfo = NULL;
};

// Helper to output a string showing the Path, ID or Debug Label based on what is available (some items only have ID as we couldn't find/store a Path)
struct ImGuiTestRefDesc
{
    char Buf[40];

    const char* c_str()     { return Buf; }
    ImGuiTestRefDesc(const ImGuiTestRef& ref, const ImGuiTestItemInfo* item = NULL)
    {
        if (ref.Path)
            ImFormatString(Buf, IM_ARRAYSIZE(Buf), "'%s' > %08X", ref.Path, ref.ID);
        else
            ImFormatString(Buf, IM_ARRAYSIZE(Buf), "%08X > '%s'", ref.ID, item ? item->DebugLabel : "NULL");
    }
};

//-------------------------------------------------------------------------
// ImGuiTestLog
//-------------------------------------------------------------------------

struct ImGuiTestLogLineInfo
{
    ImGuiTestVerboseLevel Level;
    int                   LineOffset;
};

struct ImGuiTestLog
{
    ImGuiTextBuffer                Buffer;
    ImVector<ImGuiTestLogLineInfo> LineInfo;
    ImVector<ImGuiTestLogLineInfo> LineInfoError;
    bool                           CachedLinesPrintedToTTY;

    ImGuiTestLog()
    {
        CachedLinesPrintedToTTY = false;
    }

    void Clear()
    {
        Buffer.clear();
        LineInfo.clear();
        LineInfoError.clear();
        CachedLinesPrintedToTTY = false;
    }

    void UpdateLineOffsets(ImGuiTestEngineIO* engine_io, ImGuiTestVerboseLevel level, const char* start)
    {
        IM_ASSERT(Buffer.begin() <= start && start < Buffer.end());
        const char* p_begin = start;
        const char* p_end = Buffer.end();
        const char* p = p_begin;
        while (p < p_end)
        {
            const char* p_bol = p;
            const char* p_eol = strchr(p, '\n');

            bool last_empty_line = (p_bol + 1 == p_end);

            if (!last_empty_line)
            {
                int offset = (int)(p_bol - Buffer.c_str());
                if (engine_io->ConfigVerboseLevel >= level)
                    LineInfo.push_back({level, offset});
                if (engine_io->ConfigVerboseLevelOnError >= level)
                    LineInfoError.push_back({level, offset});
            }
            p = p_eol ? p_eol + 1 : NULL;
        }
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

// Storage for one test
struct ImGuiTest
{
    const char*                     Category;           // Literal, not owned
    const char*                     Name;               // Literal, not owned
    const char*                     SourceFile;         // __FILE__
    const char*                     SourceFileShort;    // Pointer within SourceFile, skips filename.
    int                             SourceLine;         // __LINE__
    int                             SourceLineEnd;      //
    int                             ArgVariant;         // User parameter, for use by GuiFunc/TestFunc. Generally we use it to run variations of a same test.
    size_t                          UserDataSize;       // When SetUserDataType() is used, we create an instance of user structure so we can be used by GuiFunc/TestFunc.
    ImGuiTestUserDataConstructor    UserDataConstructor;
    ImGuiTestUserDataDestructor     UserDataDestructor;
    ImGuiTestStatus                 Status;
    ImGuiTestFlags                  Flags;              // See ImGuiTestFlags_
    ImGuiTestGuiFunc                GuiFunc;            // GUI functions can be reused
    ImGuiTestTestFunc               TestFunc;           // Test function
    ImGuiTestLog                    TestLog;

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
        GuiFunc = NULL;
        TestFunc = NULL;
    }

    template <typename T>
    void SetUserDataType()
    {
        UserDataSize = sizeof(T);
        UserDataConstructor = [](void* buffer) { IM_PLACEMENT_NEW(buffer) T; };
        UserDataDestructor = [](void* ptr) { IM_UNUSED(ptr); reinterpret_cast<T*>(ptr)->~T(); };
    }
};
