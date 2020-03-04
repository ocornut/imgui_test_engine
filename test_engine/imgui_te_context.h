// dear imgui
// (test engine, test context = end user automation API)

#pragma once

#include <stdint.h> // uintptr_t

#include "imgui.h"
#include "imgui_te_core.h"
#include "../shared/imgui_capture_tool.h"

// Undo some of the damage done by <windows.h>
#ifdef Yield
#undef Yield
#endif

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
    ImGuiTestAction_NavActivate,
    ImGuiTestAction_COUNT
};

inline const char* GetActionName(ImGuiTestAction action)
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
    case ImGuiTestAction_Input:         return "Input";
    case ImGuiTestAction_NavActivate:   return "NavActivate";
    case ImGuiTestAction_COUNT:
    default:                            return "N/A";
    }
}

inline const char*  GetActionVerb(ImGuiTestAction action)
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
    case ImGuiTestAction_Input:         return "Input";
    case ImGuiTestAction_NavActivate:   return "NavActivated";
    case ImGuiTestAction_COUNT:
    default:                            return "N/A";
    }
}

// Helper struct to store various query-able state of an item.
// This facilitate interactions between GuiFunc <> TestFunc, since those state are frequently used.
struct ImGuiTestGenericStatus
{
    int     Ret;
    int     Hovered;
    int     Active;
    int     Focused;
    int     Clicked;
    int     Visible;
    int     Edited;
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
        Edited += ImGui::IsItemEdited();
        Activated += ImGui::IsItemActivated();
        Deactivated += ImGui::IsItemDeactivated();
        DeactivatedAfterEdit += ImGui::IsItemDeactivatedAfterEdit();
    }
};

enum ImGuiTestActiveFunc
{
    ImGuiTestActiveFunc_None,
    ImGuiTestActiveFunc_GuiFunc,
    ImGuiTestActiveFunc_TestFunc
};

// Generic structure with varied data. This is useful for tests to quickly share data between the GUI functions and the Test function.
// This is however totally optional. Using SetUserDataType() it is possible to store custom data on the stack and read from it as UserData.
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
    ImVec2          Vec2;
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
    void ClearBools()       { Bool1 = Bool2 = false; memset(BoolArray, 0, sizeof(BoolArray)); }
};

struct ImGuiTestContext
{
    ImGuiTestEngine*        Engine = NULL;
    ImGuiTest*              Test = NULL;
    ImGuiTestEngineIO*      EngineIO = NULL;
    ImGuiContext*           UiContext = NULL;
    ImGuiTestInputs*        Inputs = NULL;
    ImGuiTestGatherTask*    GatherTask = NULL;
    ImGuiTestRunFlags       RunFlags = ImGuiTestRunFlags_None;
    ImGuiTestActiveFunc     ActiveFunc = ImGuiTestActiveFunc_None;  // None/GuiFunc/TestFunc
    void*                   UserData = NULL;
    int                     FrameCount = 0;                         // Test frame count (restarts from zero every time)
    int                     FirstFrameCount = 0;                    // First frame where Test is running. After warm-up. This is generally -2 or 0 depending on whether we have warm up enabled
    double                  RunningTime = 0.0f;                     // Amount of wall clock time the Test has been running. Used by safety watchdog.
    int                     ActionDepth = 0;
    bool                    Abort = false;
    bool                    HasDock = false;                        // #ifdef IMGUI_HAS_DOCK
    ImGuiCaptureArgs        CaptureArgs;

    // Commonly user exposed state for the ctx-> functions
    ImGuiTestGenericVars    GenericVars;
    char                    RefStr[256] = { 0 };                    // Reference window/path for ID construction
    ImGuiID                 RefID = 0;
    ImGuiInputSource        InputMode = ImGuiInputSource_Mouse;
    ImGuiTestOpFlags        OpFlags = ImGuiTestOpFlags_None;
    ImVector<char>          Clipboard;

    // Performance
    double                  PerfRefDt = -1.0;
    int                     PerfStressAmount = 0;                   // Convenience copy of engine->IO.PerfStressAmount

    // Main control
    void        Finish();
    bool        IsError() const             { return Test->Status == ImGuiTestStatus_Error || Abort; }
    bool        IsFirstFrame() const        { return FrameCount == FirstFrameCount; }
    void        SetGuiFuncEnabled(bool v)   { if (v) RunFlags &= ~ImGuiTestRunFlags_NoGuiFunc; else RunFlags |= ImGuiTestRunFlags_NoGuiFunc; }
    void        RecoverFromUiContextErrors();
    template <typename T> T& GetUserData()  { IM_ASSERT(UserData != NULL); return *(T*)(UserData); } // FIXME: Assert to compare sizes

    // Logging
    void        LogEx(ImGuiTestVerboseLevel level, ImGuiTestLogFlags flags, const char* fmt, ...) IM_FMTARGS(4);
    void        LogExV(ImGuiTestVerboseLevel level, ImGuiTestLogFlags flags, const char* fmt, va_list args) IM_FMTLIST(4);
    void        LogToTTY(ImGuiTestVerboseLevel level, const char* message);
    void        LogDebug(const char* fmt, ...)      IM_FMTARGS(2);  // ImGuiTestVerboseLevel_Debug or ImGuiTestVerboseLevel_Trace depending on context depth
    void        LogInfo(const char* fmt, ...)       IM_FMTARGS(2);  // ImGuiTestVerboseLevel_Info
    void        LogWarning(const char* fmt, ...)    IM_FMTARGS(2);  // ImGuiTestVerboseLevel_Warning
    void        LogError(const char* fmt, ...)      IM_FMTARGS(2);  // ImGuiTestVerboseLevel_Error
    void        LogDebugInfo();

    // Yield, Timing
    void        Yield();
    void        YieldFrames(int count);
    void        YieldUntil(int frame_count);
    void        Sleep(float time);
    void        SleepNoSkip(float time, float frame_time_step);
    void        SleepShort();

    // Windows
    // FIXME-TESTS: Refactor this horrible mess... perhaps all functions should have a ImGuiTestRef defaulting to empty?
    void        WindowRef(ImGuiTestRef ref);
    void        WindowClose(ImGuiTestRef ref);
    void        WindowCollapse(ImGuiWindow* window, bool collapsed);
    void        WindowAutoUncollapse(ImGuiWindow* window);
    void        WindowFocus(ImGuiTestRef ref);
    void        WindowMove(ImGuiTestRef ref, ImVec2 pos, ImVec2 pivot = ImVec2(0.0f, 0.0f));
    void        WindowResize(ImGuiTestRef ref, ImVec2 sz);
    void        WindowMoveToMakePosVisible(ImGuiWindow* window, ImVec2 pos);
    bool        WindowBringToFront(ImGuiWindow* window, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    void        PopupClose();
    ImGuiWindow* GetWindowByRef(ImGuiTestRef ref);
    ImGuiTestRef GetFocusWindowRef();

    // ID
    ImGuiID     GetID(ImGuiTestRef ref);
    ImGuiID     GetID(ImGuiTestRef ref, ImGuiTestRef seed_ref);
    ImGuiID     GetIDByInt(int n);
    ImGuiID     GetIDByInt(int n, ImGuiTestRef seed_ref);
    ImGuiID     GetIDByPtr(void* p);
    ImGuiID     GetIDByPtr(void* p, ImGuiTestRef seed_ref);

    // Misc
    ImVec2      GetMainViewportPos();
    bool        CaptureAddWindow(ImGuiTestRef ref);
    bool        CaptureScreenshot();

    // Mouse inputs
    void        MouseMove(ImGuiTestRef ref, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    void        MouseMoveToPos(ImVec2 pos);
    void        MouseClick(int button = 0);
    void        MouseDoubleClick(int button = 0);
    void        MouseDown(int button = 0);
    void        MouseUp(int button = 0);
    void        MouseLiftDragThreshold(int button = 0);

    // Keyboard inputs
    void        KeyDownMap(ImGuiKey key, int mod_flags = 0);
    void        KeyUpMap(ImGuiKey key, int mod_flags = 0);
    void        KeyPressMap(ImGuiKey key, int mod_flags = 0, int count = 1);
    void        KeyChars(const char* chars);
    void        KeyCharsAppend(const char* chars);
    void        KeyCharsAppendEnter(const char* chars);
    void        KeyCharsReplace(const char* chars);
    void        KeyCharsReplaceEnter(const char* chars);

    // Navigation inputs
    void        SetInputMode(ImGuiInputSource input_mode);
    void        NavKeyPress(ImGuiNavInput input);
    void        NavMoveTo(ImGuiTestRef ref);
    void        NavActivate();  // Activate current selected item. Same as pressing [space].
    void        NavInput();     // Press ImGuiNavInput_Input (e.g. Triangle) to turn a widget into a text input

    // Scrolling
    void        ScrollToY(ImGuiTestRef ref, float scroll_ratio_y = 0.5f);
    void        ScrollVerifyScrollMax(ImGuiWindow* window);

    // Low-level queries
    ImGuiTestItemInfo*  ItemLocate(ImGuiTestRef ref, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    void                GatherItems(ImGuiTestItemList* out_list, ImGuiTestRef parent, int depth = -1);

    // Item/Widgets manipulation
    void        ItemAction(ImGuiTestAction action, ImGuiTestRef ref, void* action_arg = NULL);
    void        ItemClick(ImGuiTestRef ref, int button = 0) { ItemAction(ImGuiTestAction_Click, ref, (void*)(intptr_t)button); }
    void        ItemDoubleClick(ImGuiTestRef ref)           { ItemAction(ImGuiTestAction_DoubleClick, ref); }
    void        ItemCheck(ImGuiTestRef ref)                 { ItemAction(ImGuiTestAction_Check, ref); }
    void        ItemUncheck(ImGuiTestRef ref)               { ItemAction(ImGuiTestAction_Uncheck, ref); }
    void        ItemOpen(ImGuiTestRef ref)                  { ItemAction(ImGuiTestAction_Open, ref); }
    void        ItemClose(ImGuiTestRef ref)                 { ItemAction(ImGuiTestAction_Close, ref); }
    void        ItemInput(ImGuiTestRef ref)                 { ItemAction(ImGuiTestAction_Input, ref); }
    void        ItemNavActivate(ImGuiTestRef ref)           { ItemAction(ImGuiTestAction_NavActivate, ref); }

    void        ItemActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent, int depth = -1, int passes = -1);
    void        ItemOpenAll(ImGuiTestRef ref_parent, int depth = -1, int passes = -1)    { ItemActionAll(ImGuiTestAction_Open, ref_parent, depth, passes); }
    void        ItemCloseAll(ImGuiTestRef ref_parent, int depth = -1, int passes = -1)   { ItemActionAll(ImGuiTestAction_Close, ref_parent, depth, passes); }

    void        ItemHold(ImGuiTestRef ref, float time);
    void        ItemHoldForFrames(ImGuiTestRef ref, int frames);
    void        ItemDragAndDrop(ImGuiTestRef ref_src, ImGuiTestRef ref_dst);
    void        ItemVerifyCheckedIfAlive(ImGuiTestRef ref, bool checked);

    // Menus
    void        MenuAction(ImGuiTestAction action, ImGuiTestRef ref);
    void        MenuActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent);
    void        MenuClick(ImGuiTestRef ref)                 { MenuAction(ImGuiTestAction_Click, ref); }
    void        MenuCheck(ImGuiTestRef ref)                 { MenuAction(ImGuiTestAction_Check, ref); }
    void        MenuUncheck(ImGuiTestRef ref)               { MenuAction(ImGuiTestAction_Uncheck, ref); }
    void        MenuCheckAll(ImGuiTestRef ref_parent)       { MenuActionAll(ImGuiTestAction_Check, ref_parent); }
    void        MenuUncheckAll(ImGuiTestRef ref_parent)     { MenuActionAll(ImGuiTestAction_Uncheck, ref_parent); }

    // Docking
#ifdef IMGUI_HAS_DOCK
    void        DockWindowInto(const char* window_src, const char* window_dst, ImGuiDir split_dir = ImGuiDir_None);
    void        DockMultiClear(const char* window_name, ...);
    void        DockMultiSet(ImGuiID dock_id, const char* window_name, ...);
    ImGuiID     DockMultiSetupBasic(ImGuiID dock_id, const char* window_name, ...);
    bool        DockIdIsUndockedOrStandalone(ImGuiID dock_id);
    void        UndockNode(ImGuiID dock_id);
#endif

    // Performances
    void        PerfCalcRef();
    void        PerfCapture();
};

// Helper to increment/decrement the function depth (so our log entry can be padded accordingly)
#define IM_TOKENPASTE(x, y)     x ## y
#define IM_TOKENPASTE2(x, y)    IM_TOKENPASTE(x, y)
#define IMGUI_TEST_CONTEXT_REGISTER_DEPTH(_THIS)        ImGuiTestContextDepthScope IM_TOKENPASTE2(depth_register, __LINE__)(_THIS)

struct ImGuiTestContextDepthScope
{
    ImGuiTestContext*       TestContext;
    ImGuiTestContextDepthScope(ImGuiTestContext* ctx)   { TestContext = ctx; TestContext->ActionDepth++; }
    ~ImGuiTestContextDepthScope()                       { TestContext->ActionDepth--; }
};
