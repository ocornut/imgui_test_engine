// dear imgui
// (test engine, test context = end user automation API)

#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_te_engine.h"            // IM_CHECK*, various flags, enums
#include <stdint.h>                     // intptr_t

// Undo some of the damage done by <windows.h>
#ifdef Yield
#undef Yield
#endif

#define IM_TOKENCONCAT_INTERNAL(x, y)   x ## y
#define IM_TOKENCONCAT(x, y)            IM_TOKENCONCAT_INTERNAL(x, y)

//-------------------------------------------------------------------------
// External forward declaration
//-------------------------------------------------------------------------

struct ImGuiCaptureArgs;
struct ImGuiTest;
struct ImGuiTestEngine;
struct ImGuiTestEngineIO;
struct ImGuiTestItemInfo;
struct ImGuiTestInputs;
struct ImGuiTestGatherTask;

//-------------------------------------------------------------------------
// ImGuiTestRef
//-------------------------------------------------------------------------

// Weak reference to an Item/Window given an ID or ID path.
struct ImGuiTestRef
{
    ImGuiID         ID;
    const char*     Path;

    ImGuiTestRef()                  { ID = 0; Path = NULL; }
    ImGuiTestRef(ImGuiID id)        { ID = id; Path = NULL; }
    ImGuiTestRef(const char* p)     { ID = 0; Path = p; }
    bool IsEmpty() const            { return ID == 0 && (Path == NULL || Path[0] == 0); }
};

// Helper to output a string showing the Path, ID or Debug Label based on what is available (some items only have ID as we couldn't find/store a Path)
struct ImGuiTestRefDesc
{
    char            Buf[80];

    const char* c_str()             { return Buf; }
    ImGuiTestRefDesc(const ImGuiTestRef& ref, const ImGuiTestItemInfo* item = NULL);
};

//-------------------------------------------------------------------------
// ImGuiTestContext
// This is the interface that most tests will interact with.
//-------------------------------------------------------------------------

// Note: keep in sync with GetActionName()
enum ImGuiTestAction
{
    ImGuiTestAction_Unknown = 0,
    ImGuiTestAction_Hover,
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
    case ImGuiTestAction_Hover:         return "Hover";
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
    case ImGuiTestAction_Hover:         return "Hovered";
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

struct ImGuiTestActionFilter
{
    int                     MaxDepth;
    int                     MaxPasses;
    const int*              MaxItemCountPerDepth;
    ImGuiItemStatusFlags    RequireAllStatusFlags;
    ImGuiItemStatusFlags    RequireAnyStatusFlags;

    ImGuiTestActionFilter() { MaxDepth = -1; MaxPasses = -1; MaxItemCountPerDepth = NULL; RequireAllStatusFlags = RequireAnyStatusFlags = 0; }
};

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
    // Generic storage with a bit of semantic to make code look neater
    int                     Step;
    int                     Count;
    ImGuiID                 DockId;
    ImGuiWindowFlags        WindowFlags;
    ImGuiTableFlags         TableFlags;
    ImGuiOldColumnFlags     ColumnsFlags;
    ImGuiTestGenericStatus  Status;
    float                   Width;
    ImVec2                  Pos;
    ImVec2                  Size;
    ImVec2                  Pivot;

    // Generic storage
    int                     Int1;
    int                     Int2;
    int                     IntArray[10];
    float                   Float1;
    float                   Float2;
    float                   FloatArray[10];
    bool                    Bool1;
    bool                    Bool2;
    bool                    BoolArray[10];
    ImVec2                  Vec2;
    ImVec4                  Vec4;
    ImVec4                  Vec4Array[10];
    ImGuiID                 Id;
    ImGuiID                 IdArray[10];
    char                    Str1[256];
    char                    Str2[256];
    ImVector<char>          StrLarge;
    void*                   Ptr1;
    void*                   Ptr2;
    void*                   PtrArray[10];

    ImGuiTestGenericVars()  { Clear(); }
    void Clear()            { StrLarge.clear(); memset(this, 0, sizeof(*this)); }
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
    int                     FirstTestFrameCount = 0;                // First frame where TestFunc is running (after warm-up frame). This is generally -1 or 0 depending on whether we have warm up enabled
    double                  RunningTime = 0.0f;                     // Amount of wall clock time the Test has been running. Used by safety watchdog.
    int                     ActionDepth = 0;
    int                     CaptureCounter = 0;
    int                     ErrorCounter = 0;
    bool                    FirstGuiFrame = false;
    bool                    Abort = false;
    bool                    HasDock = false;                        // #ifdef IMGUI_HAS_DOCK

    // Commonly user exposed state for the ctx-> functions
    ImGuiTestGenericVars    GenericVars;
    char                    RefStr[256] = { 0 };                    // Reference window/path for ID construction
    ImGuiID                 RefID = 0;
    ImGuiInputSource        InputMode = ImGuiInputSource_Mouse;
    ImGuiTestOpFlags        OpFlags = ImGuiTestOpFlags_None;
    ImVector<char>          Clipboard;
    ImVector<ImGuiWindow*>  ForeignWindowsToHide;

    // Performance
    double                  PerfRefDt = -1.0;
    int                     PerfStressAmount = 0;                   // Convenience copy of engine->IO.PerfStressAmount

    // Main control
    void        Finish();
    bool        IsError() const             { return Test->Status == ImGuiTestStatus_Error || Abort; }
    bool        IsFirstGuiFrame() const     { return FirstGuiFrame; }
    bool        IsFirstTestFrame() const    { return FrameCount == FirstTestFrameCount; }   // First frame where TestFunc is running (after warm-up frame).
    void        SetGuiFuncEnabled(bool v)   { if (v) RunFlags &= ~ImGuiTestRunFlags_GuiFuncDisable; else RunFlags |= ImGuiTestRunFlags_GuiFuncDisable; }
    void        RecoverFromUiContextErrors();
    template <typename T> T& GetUserData()  { IM_ASSERT(UserData != NULL); return *(T*)(UserData); } // FIXME: Assert to compare sizes

    // Debug Control Flow
    bool        DebugHaltTestFunc(const char* file, int line);

    // Logging
    void        LogEx(ImGuiTestVerboseLevel level, ImGuiTestLogFlags flags, const char* fmt, ...) IM_FMTARGS(4);
    void        LogExV(ImGuiTestVerboseLevel level, ImGuiTestLogFlags flags, const char* fmt, va_list args) IM_FMTLIST(4);
    void        LogToTTY(ImGuiTestVerboseLevel level, const char* message);
    void        LogToDebugger(ImGuiTestVerboseLevel level, const char* message);
    void        LogDebug(const char* fmt, ...)      IM_FMTARGS(2);  // ImGuiTestVerboseLevel_Debug or ImGuiTestVerboseLevel_Trace depending on context depth
    void        LogInfo(const char* fmt, ...)       IM_FMTARGS(2);  // ImGuiTestVerboseLevel_Info
    void        LogWarning(const char* fmt, ...)    IM_FMTARGS(2);  // ImGuiTestVerboseLevel_Warning
    void        LogError(const char* fmt, ...)      IM_FMTARGS(2);  // ImGuiTestVerboseLevel_Error
    void        LogBasicUiState();

    // Yield, Timing
    void        Yield(int count = 1);
    void        YieldUntil(int frame_count);
    void        Sleep(float time);
    void        SleepNoSkip(float time, float frame_time_step);
    void        SleepShort();

    // Windows
    // FIXME-TESTS: Refactor this horrible mess... perhaps all functions should have a ImGuiTestRef defaulting to empty?
    void        SetRef(ImGuiTestRef ref);
    void        SetRef(ImGuiWindow* window); // Shortcut to SetRef(window->Name) which works for ChildWindow (see code)
    ImGuiTestRef GetRef();
    void        WindowClose(ImGuiTestRef ref);
    void        WindowCollapse(ImGuiWindow* window, bool collapsed);
    void        WindowFocus(ImGuiTestRef ref);
    void        WindowMove(ImGuiTestRef ref, ImVec2 pos, ImVec2 pivot = ImVec2(0.0f, 0.0f));
    void        WindowResize(ImGuiTestRef ref, ImVec2 sz);
    bool        WindowTeleportToMakePosVisibleInViewport(ImGuiWindow* window, ImVec2 pos_in_window);
    bool        WindowBringToFront(ImGuiWindow* window, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    void        PopupCloseOne();
    void        PopupCloseAll();
    ImGuiWindow* GetWindowByRef(ImGuiTestRef ref);
    ImGuiTestRef GetFocusWindowRef();

    void        ForeignWindowsHideOverPos(ImVec2 pos, ImGuiWindow** ignore_list);
    void        ForeignWindowsUnhideAll();

    // ID
    ImGuiID     GetID(ImGuiTestRef ref);
    ImGuiID     GetID(ImGuiTestRef ref, ImGuiTestRef seed_ref);
    ImGuiID     GetIDByInt(int n);
    ImGuiID     GetIDByInt(int n, ImGuiTestRef seed_ref);
    ImGuiID     GetIDByPtr(void* p);
    ImGuiID     GetIDByPtr(void* p, ImGuiTestRef seed_ref);

    // Misc
    ImVec2      GetMainViewportPos();
    ImVec2      GetMainViewportSize();

    // Screen captures
    // - Simple API
    void        CaptureScreenshotWindow(ImGuiTestRef ref, int capture_flags = 0);
    // - Advanced API
    void        CaptureInitArgs(ImGuiCaptureArgs* args, int capture_flags = 0);
    bool        CaptureAddWindow(ImGuiCaptureArgs* args, ImGuiTestRef ref);
    bool        CaptureScreenshotEx(ImGuiCaptureArgs* args);
    // - Animation capturing API
    bool        BeginCaptureGif(ImGuiCaptureArgs* args);
    bool        EndCaptureGif(ImGuiCaptureArgs* args);

    // Mouse inputs
    void        MouseMove(ImGuiTestRef ref, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    void        MouseMoveToPos(ImVec2 pos);
    void        MouseTeleportToPos(ImVec2 pos);
    void        MouseClick(ImGuiMouseButton button = 0);
    void        MouseDoubleClick(ImGuiMouseButton button = 0);
    void        MouseDown(ImGuiMouseButton button = 0);
    void        MouseUp(ImGuiMouseButton button = 0);
    void        MouseLiftDragThreshold(ImGuiMouseButton button = 0);
    void        MouseClickOnVoid(ImGuiMouseButton button = 0);
    void        MouseDragWithDelta(ImVec2 delta, ImGuiMouseButton button = 0);
    void        MouseWheel(float vertical, float horizontal=0.0f);
    void        MouseWheel(ImVec2 delta);
    bool        FindExistingVoidPosOnViewport(ImGuiViewport* viewport, ImVec2* out);

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
    void        NavKeyDown(ImGuiNavInput input);
    void        NavKeyUp(ImGuiNavInput input);
    void        NavMoveTo(ImGuiTestRef ref);
    void        NavActivate();  // Activate current selected item. Same as pressing [space].
    void        NavInput();     // Press ImGuiNavInput_Input (e.g. Triangle) to turn a widget into a text input

    // Scrolling
    void        ScrollTo(ImGuiWindow* window, ImGuiAxis axis, float scroll_v);
    void        ScrollToX(float scroll_x) { ScrollTo(GetWindowByRef(""), ImGuiAxis_X, scroll_x); }
    void        ScrollToY(float scroll_y) { ScrollTo(GetWindowByRef(""), ImGuiAxis_Y, scroll_y); }
    void        ScrollToTop();
    void        ScrollToBottom();
    void        ScrollToItemY(ImGuiTestRef ref, float scroll_ratio_y = 0.5f);
    void        ScrollToItemX(ImGuiTestRef ref);
    bool        ScrollErrorCheck(ImGuiAxis axis, float expected, float actual, int* remaining_attempts);
    void        ScrollVerifyScrollMax(ImGuiWindow* window);

    // Low-level queries
    ImGuiTestItemInfo*  ItemInfo(ImGuiTestRef ref, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    void                GatherItems(ImGuiTestItemList* out_list, ImGuiTestRef parent, int depth = -1);

    // Item/Widgets manipulation
    void        ItemAction(ImGuiTestAction action, ImGuiTestRef ref, void* action_arg = NULL, ImGuiTestOpFlags flags = 0);
    void        ItemClick(ImGuiTestRef ref, ImGuiMouseButton button = 0, ImGuiTestOpFlags flags = 0) { ItemAction(ImGuiTestAction_Click, ref, (void*)(intptr_t)button, flags); }
    void        ItemDoubleClick(ImGuiTestRef ref, ImGuiTestOpFlags flags = 0)           { ItemAction(ImGuiTestAction_DoubleClick, ref, NULL, flags); }
    void        ItemCheck(ImGuiTestRef ref, ImGuiTestOpFlags flags = 0)                 { ItemAction(ImGuiTestAction_Check, ref, NULL, flags); }
    void        ItemUncheck(ImGuiTestRef ref, ImGuiTestOpFlags flags = 0)               { ItemAction(ImGuiTestAction_Uncheck, ref, NULL, flags); }
    void        ItemOpen(ImGuiTestRef ref, ImGuiTestOpFlags flags = 0)                  { ItemAction(ImGuiTestAction_Open, ref, NULL, flags); }
    void        ItemClose(ImGuiTestRef ref, ImGuiTestOpFlags flags = 0)                 { ItemAction(ImGuiTestAction_Close, ref, NULL, flags); }
    void        ItemInput(ImGuiTestRef ref, ImGuiTestOpFlags flags = 0)                 { ItemAction(ImGuiTestAction_Input, ref, NULL, flags); }
    void        ItemNavActivate(ImGuiTestRef ref, ImGuiTestOpFlags flags = 0)           { ItemAction(ImGuiTestAction_NavActivate, ref, NULL, flags); }

    void        ItemActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent, const ImGuiTestActionFilter* filter = NULL);
    void        ItemOpenAll(ImGuiTestRef ref_parent, int depth = -1, int passes = -1);
    void        ItemCloseAll(ImGuiTestRef ref_parent, int depth = -1, int passes = -1);

    void        ItemHold(ImGuiTestRef ref, float time);
    void        ItemHoldForFrames(ImGuiTestRef ref, int frames);
    void        ItemDragOverAndHold(ImGuiTestRef ref_src, ImGuiTestRef ref_dst);
    void        ItemDragAndDrop(ImGuiTestRef ref_src, ImGuiTestRef ref_dst, ImGuiMouseButton button = 0);
    void        ItemDragWithDelta(ImGuiTestRef ref_src, ImVec2 pos_delta);
    void        ItemVerifyCheckedIfAlive(ImGuiTestRef ref, bool checked);

    // Tab Bars
    void        TabClose(ImGuiTestRef ref);

    // Menus
    void        MenuAction(ImGuiTestAction action, ImGuiTestRef ref);
    void        MenuActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent);
    void        MenuClick(ImGuiTestRef ref)                 { MenuAction(ImGuiTestAction_Click, ref); }
    void        MenuCheck(ImGuiTestRef ref)                 { MenuAction(ImGuiTestAction_Check, ref); }
    void        MenuUncheck(ImGuiTestRef ref)               { MenuAction(ImGuiTestAction_Uncheck, ref); }
    void        MenuCheckAll(ImGuiTestRef ref_parent)       { MenuActionAll(ImGuiTestAction_Check, ref_parent); }
    void        MenuUncheckAll(ImGuiTestRef ref_parent)     { MenuActionAll(ImGuiTestAction_Uncheck, ref_parent); }

    // Combo
    void        ComboClick(ImGuiTestRef ref);
    void        ComboClickAll(ImGuiTestRef ref);

    // Tables
    void                        TableOpenContextMenu(ImGuiTestRef ref, int column_n = -1);
    ImGuiSortDirection          TableClickHeader(ImGuiTestRef ref, const char* label, ImGuiKeyModFlags keys_mod = ImGuiKeyModFlags_None);
    void                        TableSetColumnEnabled(ImGuiTestRef ref, const char* label, bool enabled);
    void                        TableResizeColumn(ImGuiTestRef ref, int column_n, float width);
    const ImGuiTableSortSpecs*  TableGetSortSpecs(ImGuiTestRef ref);

    // Docking
#ifdef IMGUI_HAS_DOCK
    void        DockWindowInto(ImGuiTestRef window_src, ImGuiTestRef window_dst, ImGuiDir split_dir = ImGuiDir_None);
    void        DockWindowIntoEx(ImGuiTestRef window_src, ImGuiTestRef window_dst, ImGuiID node_id, ImGuiDir split_dir = ImGuiDir_None, bool is_outer_docking = false);
    void        DockWindowIntoEx(ImGuiTestRef window_src, ImGuiTestRef window_dst, ImGuiDockNode* node_dst, ImGuiDir split_dir = ImGuiDir_None, bool is_outer_docking = false);
    void        DockMultiClear(const char* window_name, ...);
    void        DockMultiSet(ImGuiID dock_id, const char* window_name, ...);
    ImGuiID     DockMultiSetupBasic(ImGuiID dock_id, const char* window_name, ...);
    bool        WindowIsUndockedOrStandalone(ImGuiWindow* window);
    bool        DockIdIsUndockedOrStandalone(ImGuiID dock_id);
    void        DockNodeHideTabBar(ImGuiDockNode* node, bool hidden);
    void        UndockNode(ImGuiID dock_id);
    void        UndockWindow(const char* window_name);
#endif

    // Performances
    void        PerfCalcRef();
    void        PerfCapture();
};

// Helper to increment/decrement the function depth (so our log entry can be padded accordingly)
#define IMGUI_TEST_CONTEXT_REGISTER_DEPTH(_THIS)        ImGuiTestContextDepthScope IM_TOKENCONCAT(depth_register, __LINE__)(_THIS)

struct ImGuiTestContextDepthScope
{
    ImGuiTestContext*       TestContext;
    ImGuiTestContextDepthScope(ImGuiTestContext* ctx)   { TestContext = ctx; TestContext->ActionDepth++; }
    ~ImGuiTestContextDepthScope()                       { TestContext->ActionDepth--; }
};
