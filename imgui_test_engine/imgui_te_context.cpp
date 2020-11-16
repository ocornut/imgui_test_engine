// dear imgui
// (test engine, context for a running test + end user automation API)

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_te_context.h"
#include "imgui.h"
#include "imgui_te_engine.h"
#include "imgui_te_internal.h"
#include "imgui_te_perftool.h"
#include "imgui_te_util.h"
#include "shared/imgui_utils.h"
#include "thirdparty/Str/Str.h"

#ifndef _WIN32
#include <unistd.h>     // usleep
#endif

//-------------------------------------------------------------------------
// [SECTION] ImGuiTestRefDesc
//-------------------------------------------------------------------------

ImGuiTestRefDesc::ImGuiTestRefDesc(const ImGuiTestRef& ref, const ImGuiTestItemInfo* item)
{
    if (ref.Path)
        ImFormatString(Buf, IM_ARRAYSIZE(Buf), "'%s' > %08X", ref.Path, ref.ID);
    else
        ImFormatString(Buf, IM_ARRAYSIZE(Buf), "%08X > '%s'", ref.ID, item ? item->DebugLabel : "NULL");
}

//-------------------------------------------------------------------------
// [SECTION] ImGuiTestContextDepthScope
//-------------------------------------------------------------------------

// Helper to increment/decrement the function depth (so our log entry can be padded accordingly)
#define IM_TOKENCONCAT_INTERNAL(x, y)                   x ## y
#define IM_TOKENCONCAT(x, y)                            IM_TOKENCONCAT_INTERNAL(x, y)
#define IMGUI_TEST_CONTEXT_REGISTER_DEPTH(_THIS)        ImGuiTestContextDepthScope IM_TOKENCONCAT(depth_register, __LINE__)(_THIS)

struct ImGuiTestContextDepthScope
{
    ImGuiTestContext* TestContext;
    ImGuiTestContextDepthScope(ImGuiTestContext* ctx) { TestContext = ctx; TestContext->ActionDepth++; }
    ~ImGuiTestContextDepthScope() { TestContext->ActionDepth--; }
};

//-------------------------------------------------------------------------
// [SECTION] Enum names helpers
//-------------------------------------------------------------------------

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

inline const char* GetActionVerb(ImGuiTestAction action)
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

//-------------------------------------------------------------------------
// [SECTION] ImGuiTestContext
// This is the interface that most tests will interact with.
//-------------------------------------------------------------------------

void    ImGuiTestContext::LogEx(ImGuiTestVerboseLevel level, ImGuiTestLogFlags flags, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogExV(level, flags, fmt, args);
    va_end(args);
}

void    ImGuiTestContext::LogExV(ImGuiTestVerboseLevel level, ImGuiTestLogFlags flags, const char* fmt, va_list args)
{
    ImGuiTestContext* ctx = this;
    ImGuiTest* test = ctx->Test;

    IM_ASSERT(level > ImGuiTestVerboseLevel_Silent && level < ImGuiTestVerboseLevel_COUNT);

    if (level == ImGuiTestVerboseLevel_Debug && ctx->ActionDepth > 1)
        level = ImGuiTestVerboseLevel_Trace;

    // Log all messages that we may want to print in future.
    if (EngineIO->ConfigVerboseLevelOnError < level)
        return;

    ImGuiTestLog* log = &test->TestLog;
    const int prev_size = log->Buffer.size();

    //const char verbose_level_char = ImGuiTestEngine_GetVerboseLevelName(level)[0];
    //if (flags & ImGuiTestLogFlags_NoHeader)
    //    log->Buffer.appendf("[%c] ", verbose_level_char);
    //else
    //    log->Buffer.appendf("[%c] [%04d] ", verbose_level_char, ctx->FrameCount);
    if ((flags & ImGuiTestLogFlags_NoHeader) == 0)
        log->Buffer.appendf("[%04d] ", ctx->FrameCount);

    if (level >= ImGuiTestVerboseLevel_Debug)
        log->Buffer.appendf("-- %*s", ImMax(0, (ctx->ActionDepth - 1) * 2), "");
    log->Buffer.appendfv(fmt, args);
    log->Buffer.append("\n");

    log->UpdateLineOffsets(EngineIO, level, log->Buffer.begin() + prev_size);
    LogToTTY(level, log->Buffer.c_str() + prev_size);
    LogToDebugger(level, log->Buffer.c_str() + prev_size);
}

void    ImGuiTestContext::LogDebug(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogExV(ImGuiTestVerboseLevel_Debug, ImGuiTestLogFlags_None, fmt, args);
    va_end(args);
}

void ImGuiTestContext::LogInfo(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogExV(ImGuiTestVerboseLevel_Info, ImGuiTestLogFlags_None, fmt, args);
    va_end(args);
}

void ImGuiTestContext::LogWarning(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogExV(ImGuiTestVerboseLevel_Warning, ImGuiTestLogFlags_None, fmt, args);
    va_end(args);
}

void ImGuiTestContext::LogError(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogExV(ImGuiTestVerboseLevel_Error, ImGuiTestLogFlags_None, fmt, args);
    va_end(args);
}

void    ImGuiTestContext::LogToTTY(ImGuiTestVerboseLevel level, const char* message)
{
    IM_ASSERT(level > ImGuiTestVerboseLevel_Silent && level < ImGuiTestVerboseLevel_COUNT);

    if (!EngineIO->ConfigLogToTTY)
        return;

    ImGuiTestContext* ctx = this;
    ImGuiTest* test = ctx->Test;
    ImGuiTestLog* log = &test->TestLog;

    if (test->Status == ImGuiTestStatus_Error)
    {
        // Current test failed.
        if (!log->CachedLinesPrintedToTTY)
        {
            // Print current message and all previous logged messages.
            log->CachedLinesPrintedToTTY = true;
            for (int i = 0; i < log->LineInfoError.Size; i++)
            {
                ImGuiTestLogLineInfo& line_info = log->LineInfoError[i];
                char* line_beg = log->Buffer.Buf.Data + line_info.LineOffset;
                char* line_end = strchr(line_beg, '\n');
                char line_end_bkp = *(line_end + 1);
                *(line_end + 1) = 0;                            // Terminate line temporarily to avoid extra copying.
                LogToTTY(line_info.Level, line_beg);
                *(line_end + 1) = line_end_bkp;                 // Restore new line after printing.
            }
            return;                                             // This process included current line as well.
        }
        // Otherwise print only current message. If we are executing here log level already is within range of
        // ConfigVerboseLevelOnError setting.
    }
    else if (EngineIO->ConfigVerboseLevel < level)
    {
        // Skip printing messages of lower level than configured.
        return;
    }

    switch (level)
    {
    case ImGuiTestVerboseLevel_Warning:
        ImOsConsoleSetTextColor(ImOsConsoleStream_StandardOutput, ImOsConsoleTextColor_BrightYellow);
        break;
    case ImGuiTestVerboseLevel_Error:
        ImOsConsoleSetTextColor(ImOsConsoleStream_StandardOutput, ImOsConsoleTextColor_BrightRed);
        break;
    default:
        ImOsConsoleSetTextColor(ImOsConsoleStream_StandardOutput, ImOsConsoleTextColor_White);
        break;
    }
    fprintf(stdout, "%s", message);
    ImOsConsoleSetTextColor(ImOsConsoleStream_StandardOutput, ImOsConsoleTextColor_White);
    fflush(stdout);
}

void        ImGuiTestContext::LogToDebugger(ImGuiTestVerboseLevel level, const char* message)
{
    IM_ASSERT(level > ImGuiTestVerboseLevel_Silent && level < ImGuiTestVerboseLevel_COUNT);

    if (!EngineIO->ConfigLogToDebugger)
        return;

    if (EngineIO->ConfigVerboseLevel < level)
        return;

    switch (level)
    {
    default:
        break;
    case ImGuiTestVerboseLevel_Error:
        ImOsOutputDebugString("[error] ");
        break;
    case ImGuiTestVerboseLevel_Warning:
        ImOsOutputDebugString("[warn.] ");
        break;
    case ImGuiTestVerboseLevel_Info:
        ImOsOutputDebugString("[info ] ");
        break;
    case ImGuiTestVerboseLevel_Debug:
        ImOsOutputDebugString("[debug] ");
        break;
    case ImGuiTestVerboseLevel_Trace:
        ImOsOutputDebugString("[trace] ");
        break;
    }

    ImOsOutputDebugString(message);
}

void    ImGuiTestContext::LogBasicUiState()
{
    ImGuiID item_hovered_id = UiContext->HoveredIdPreviousFrame;
    ImGuiID item_active_id = UiContext->ActiveId;
    ImGuiTestItemInfo* item_hovered_info = item_hovered_id ? ImGuiTestEngine_FindItemInfo(Engine, item_hovered_id, "") : NULL;
    ImGuiTestItemInfo* item_active_info = item_active_id ? ImGuiTestEngine_FindItemInfo(Engine, item_active_id, "") : NULL;
    LogDebug("Hovered: 0x%08X (\"%s\"), Active:  0x%08X(\"%s\")",
        item_hovered_id, item_hovered_info ? item_hovered_info->DebugLabel : "",
        item_active_id, item_active_info ? item_active_info->DebugLabel : "");
}

void    ImGuiTestContext::Finish()
{
    if (RunFlags & ImGuiTestRunFlags_GuiFuncOnly)
        return;
    ImGuiTest* test = Test;
    if (test->Status == ImGuiTestStatus_Running)
        test->Status = ImGuiTestStatus_Success;
}

static void LogWarningFunc(void* user_data, const char* fmt, ...)
{
    ImGuiTestContext* ctx = (ImGuiTestContext*)user_data;
    va_list args;
    va_start(args, fmt);
    ctx->LogExV(ImGuiTestVerboseLevel_Warning, ImGuiTestLogFlags_None, fmt, args);
    va_end(args);
}

static void LogNotAsWarningFunc(void* user_data, const char* fmt, ...)
{
    ImGuiTestContext* ctx = (ImGuiTestContext*)user_data;
    va_list args;
    va_start(args, fmt);
    ctx->LogExV(ImGuiTestVerboseLevel_Debug, ImGuiTestLogFlags_None, fmt, args);
    va_end(args);
}

void    ImGuiTestContext::RecoverFromUiContextErrors()
{
    IM_ASSERT(Test != NULL);

    // If we are _already_ in a test error state, recovering is normal so we'll hide the log.
    const bool verbose = (Test->Status != ImGuiTestStatus_Error) || (EngineIO->ConfigVerboseLevel >= ImGuiTestVerboseLevel_Debug);
    if (verbose && (Test->Flags & ImGuiTestFlags_NoRecoverWarnings) == 0)
        ImGui::ErrorCheckEndFrameRecover(LogWarningFunc, this);
    else
        ImGui::ErrorCheckEndFrameRecover(LogNotAsWarningFunc, this);
}

void    ImGuiTestContext::Yield(int count)
{
    IM_ASSERT(count > 0);
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

    if (EngineIO->ConfigRunFast)
    {
        //ImGuiTestEngine_AddExtraTime(Engine, time); // We could add time, for now we have no use for it...
        ImGuiTestEngine_Yield(Engine);
    }
    else
    {
        while (time > 0.0f && !Abort)
        {
            ImGuiTestEngine_Yield(Engine);
            time -= UiContext->IO.DeltaTime;
        }
    }
}

// Return true to request aborting TestFunc
bool    ImGuiTestContext::DebugHaltTestFunc(const char* file, int line)
{
    if (IsError())
        return false;

    file = ImPathFindFilename(file);
    LogError("DebugHaltTestFunc at %s:%d", file, line);

    // Save relevant state.
    // FIXME-TESTS: Saving/restoring window z-order could be desirable.
    ImVec2 mouse_pos = Inputs->MousePosValue;
    ImGuiTestRunFlags run_flags = RunFlags;

    RunFlags |= ImGuiTestRunFlags_GuiFuncOnly;
    Test->Status = ImGuiTestStatus_Suspended;
    while (Test->Status == ImGuiTestStatus_Suspended && !Abort)
        Yield();
    Test->Status = ImGuiTestStatus_Running;

    // Restore relevant state.
    RunFlags = run_flags;
    Inputs->MousePosValue = mouse_pos;

    // Terminate TestFunc on abort, continue otherwise.
    return Abort;
}

// Sleep for a given clock time from the point of view of the imgui context, without affecting wall clock time of the running application.
void    ImGuiTestContext::SleepNoSkip(float time, float frame_time_step)
{
    if (IsError())
        return;

    while (time > 0.0f && !Abort)
    {
        ImGuiTestEngine_SetDeltaTime(Engine, frame_time_step);
        ImGuiTestEngine_Yield(Engine);
        time -= UiContext->IO.DeltaTime;
    }
}

void    ImGuiTestContext::SleepShort()
{
    Sleep(0.3f);
}

void ImGuiTestContext::SetInputMode(ImGuiInputSource input_mode)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("SetInputMode %d", input_mode);

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

void ImGuiTestContext::SetRef(ImGuiWindow* window)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("WindowRef '%s' %08X", window->Name, window->ID);

    // We grab the ID directly and avoid ImHashDecoratedPath so "/" in window names are not ignored.
    size_t len = strlen(window->Name);
    IM_ASSERT(len < IM_ARRAYSIZE(RefStr) - 1);
    strcpy(RefStr, window->Name);
    RefID = window->ID;

    // Automatically uncollapse by default
    if (!(OpFlags & ImGuiTestOpFlags_NoAutoUncollapse))
        WindowCollapse(window, false);
}

// FIXME-TESTS: May be to focus window when docked? Otherwise locate request won't even see an item?
void ImGuiTestContext::SetRef(ImGuiTestRef ref)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("WindowRef '%s' %08X", ref.Path ? ref.Path : "NULL", ref.ID);

    if (ref.Path)
    {
        size_t len = strlen(ref.Path);
        IM_ASSERT(len < IM_ARRAYSIZE(RefStr) - 1);

        strcpy(RefStr, ref.Path);
        RefID = GetID(ref.Path, ImGuiTestRef());
    }
    else
    {
        RefStr[0] = 0;
        RefID = ref.ID;
    }

    // Automatically uncollapse by default
    if (!(OpFlags & ImGuiTestOpFlags_NoAutoUncollapse))
        if (ImGuiWindow* window = GetWindowByRef(""))
            WindowCollapse(window, false);
}

ImGuiTestRef ImGuiTestContext::GetRef()
{
    return RefID;
}

// Turn ref into a root ref unless ref is empty
ImGuiWindow* ImGuiTestContext::GetWindowByRef(ImGuiTestRef ref)
{
    ImGuiID window_id = ref.IsEmpty() ? GetID(ref) : GetID(ref, "/");
    ImGuiWindow* window = ImGui::FindWindowByID(window_id);
    return window;
}

ImGuiID ImGuiTestContext::GetID(ImGuiTestRef ref)
{
    if (ref.ID)
        return ref.ID;

    return GetID(ref, RefID);
}

ImGuiID ImGuiTestContext::GetID(ImGuiTestRef ref, ImGuiTestRef seed_ref)
{
    ImGuiContext& g = *UiContext;

    if (ref.ID)
        return ref.ID; // FIXME: What if seed_ref != 0

    const char* FOCUSED_PREFIX = "/$FOCUSED";
    const size_t FOCUSED_PREFIX_LEN = 9;

    const char* path = ref.Path ? ref.Path : "";
    if (strncmp(path, FOCUSED_PREFIX, FOCUSED_PREFIX_LEN) == 0)
        if (path[FOCUSED_PREFIX_LEN] == '/' || path[FOCUSED_PREFIX_LEN] == 0)
        {
            path += FOCUSED_PREFIX_LEN;
            if (path[0] == '/')
                path++;
            if (g.NavWindow)
                seed_ref = g.NavWindow->ID;
            else
                LogError("\"/$FOCUSED\" was used with no focused window!");
        }

    return ImHashDecoratedPath(path, NULL, seed_ref.Path ? GetID(seed_ref) : seed_ref.ID);
}

ImGuiID ImGuiTestContext::GetIDByInt(int n)
{
    return ImHashData(&n, sizeof(n), GetID(RefID));
}

ImGuiID ImGuiTestContext::GetIDByInt(int n, ImGuiTestRef seed_ref)
{
    return ImHashData(&n, sizeof(n), GetID(seed_ref));
}

ImGuiID ImGuiTestContext::GetIDByPtr(void* p)
{
    return ImHashData(&p, sizeof(p), GetID(RefID));
}

ImGuiID ImGuiTestContext::GetIDByPtr(void* p, ImGuiTestRef seed_ref)
{
    return ImHashData(&p, sizeof(p), GetID(seed_ref));
}

ImGuiID ImGuiTestContext::GetChildWindowID(const char* child_name)
{
    // Use SetRef() before calling a function that depends on current ref.
    IM_ASSERT(RefID != 0);
    return GetChildWindowID(RefID, child_name);
}

ImGuiID ImGuiTestContext::GetChildWindowID(ImGuiID child_id)
{
    // Use SetRef() before calling a function that depends on current ref.
    IM_ASSERT(RefID != 0);
    return GetChildWindowID(RefID, child_id);
}

// Helper function for GetChildWindowID()
// FIXME-TESTS: Badly named.
static bool GetWindowInformation(ImGuiTestContext* ctx, ImGuiTestRef window_ref, Str* out_name, ImGuiID* out_id)
{
    IM_ASSERT(out_name != NULL || out_id != NULL);
    if (window_ref.Path)
    {
        Str256 tmp;
        if (out_name == NULL)
            out_name = &tmp;

        // Assume window_ref follows ImGuiTestRef conventions (unescaped slash resets ID counter).
        const char* name = window_ref.Path;
        if (*name == '/')   // Skip initial /, indicating we arent using RefID as ID base.
            name++;

        // Ensure there are no unescaped slashes remaining, since this is just a window name, not a hashable path.
        // Valid: "Window", "/Window", "/Window\\/Foo".
        // Invalid: "/Window/Foo", "Window/Foo".
        for (int i = 1; i < name[i]; i++)
            if (name[i] == '/' && name[i - 1] != '\\')
            {
                IM_ASSERT(0);
                return false;
            }

        out_name->set(name);
        ImStrReplace(out_name, "\\/", "/"); // Unescape slashes.

        if (out_id)
            *out_id = ImHashStr(name);
    }
    else
    {
        // Parent window specified by ID, it must exist.
        ImGuiWindow* window = ctx->GetWindowByRef(window_ref);
        for (int retries = 2; window == NULL && retries > 0; retries--)
        {
            ctx->Yield();
            window = ctx->GetWindowByRef(window_ref);
        }
        if (window == NULL)
            return false;
        if (out_name)
            out_name->set(window->Name);
        if (out_id)
            *out_id = window->ID;
    }
    return true;
}

// Mimic logic of BeginChildEx(), ASSUMING child is output in root of parent
ImGuiID ImGuiTestContext::GetChildWindowID(ImGuiTestRef parent_ref, const char* child_name)
{
    IM_ASSERT(!parent_ref.IsEmpty());
    IM_ASSERT(child_name != NULL);

    ImGuiID parent_id = 0;
    Str256 parent_name;
    if (!GetWindowInformation(this, parent_ref, &parent_name, &parent_id))
    {
        LogError("GetChildWindowID: parent window should exist, when specifying by ID(0x%08X).", parent_ref.ID);
        return 0;
    }

    ImGuiID child_item_id = GetID(child_name, parent_id);
    ImStrReplace(&parent_name, "/", "\\/");
    if (const char* last_slash = strrchr(child_name, '/'))
    {
        child_name = last_slash + 1;
        IM_ASSERT(*child_name != 0);    // child_name should not end with a slash.
    }
    return GetID(Str128f("/%s\\/%s_%08X", parent_name.c_str(), child_name, child_item_id).c_str());
}

ImGuiID ImGuiTestContext::GetChildWindowID(ImGuiTestRef parent_ref, ImGuiID child_id)
{
    IM_ASSERT(!parent_ref.IsEmpty());
    IM_ASSERT(child_id != 0);

    Str256 parent_name;
    if (!GetWindowInformation(this, parent_ref, &parent_name, NULL))
    {
        LogError("GetChildWindowID: parent window should exist, when specifying by ID(0x%08X).", parent_ref.ID);
        return 0;
    }

    ImStrReplace(&parent_name, "/", "\\/");
    return GetID(Str128f("/%s\\/%08X", parent_name.c_str(), child_id).c_str());
}

ImVec2 ImGuiTestContext::GetMainMonitorWorkPos()
{
#ifdef IMGUI_HAS_VIEWPORT
    if (UiContext->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        const ImGuiPlatformMonitor* monitor = ImGui::GetViewportPlatformMonitor(ImGui::GetMainViewport());
        return monitor->WorkPos;
    }
#endif
    return ImGui::GetMainViewport()->WorkPos;
}

ImVec2 ImGuiTestContext::GetMainMonitorWorkSize()
{
#ifdef IMGUI_HAS_VIEWPORT
    if (UiContext->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        const ImGuiPlatformMonitor* monitor = ImGui::GetViewportPlatformMonitor(ImGui::GetMainViewport());
        return monitor->WorkSize;
    }
#endif
    return ImGui::GetMainViewport()->WorkSize;
}

static bool ImGuiTestContext_CanCapture(ImGuiTestContext* ctx)
{
    ImGuiTestEngineIO* io = ctx->EngineIO;
    return io->ConfigCaptureEnabled;
}

void ImGuiTestContext::CaptureInitArgs(ImGuiCaptureArgs* args, int flags)
{
    args->InFlags = (ImGuiCaptureFlags)flags;
    ImFormatString(args->InOutputFileTemplate, IM_ARRAYSIZE(args->InOutputFileTemplate), "output/captures/%s_%04d.png", Test->Name, CaptureCounter);
    CaptureCounter++;
}

bool ImGuiTestContext::CaptureAddWindow(ImGuiCaptureArgs* args, ImGuiTestRef ref)
{
    ImGuiWindow* window = GetWindowByRef(ref);
    IM_CHECK_SILENT_RETV(window != NULL, false);
    args->InCaptureWindows.push_back(window);
    return true;
}

bool ImGuiTestContext::CaptureScreenshotEx(ImGuiCaptureArgs* args)
{
    if (IsError())
        return false;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("CaptureScreenshot()");
#ifndef IMGUI_TEST_ENGINE_DISABLE_CAPTURE
    if (!ImGuiTestContext_CanCapture(this))
        args->InFlags |= ImGuiCaptureFlags_NoSave;
    bool ret = ImGuiTestEngine_CaptureScreenshot(Engine, args);
    if (ImGuiTestContext_CanCapture(this))
        LogInfo("Saved '%s' (%d*%d pixels)", args->OutSavedFileName, (int)args->OutImageSize.x, (int)args->OutImageSize.y);
    else
        LogWarning("Skipped saving '%s' (%d*%d pixels) (enable in 'Misc->Options')", args->OutSavedFileName, (int)args->OutImageSize.x, (int)args->OutImageSize.y);
    return ret;
#else
    IM_UNUSED(args);
    LogWarning("Skipped capturing screenshot: capture disabled by IMGUI_TEST_ENGINE_DISABLE_CAPTURE.");
    return false;
#endif
}

// FIXME-TESTS: Add ImGuiCaptureFlags_NoHideOtherWindows
void ImGuiTestContext::CaptureScreenshotWindow(ImGuiTestRef ref, int capture_flags)
{
    ImGuiCaptureArgs args;
    CaptureInitArgs(&args, capture_flags);
    CaptureAddWindow(&args, ref);
    CaptureScreenshotEx(&args);
}

bool ImGuiTestContext::CaptureBeginGif(ImGuiCaptureArgs* args)
{
    if (IsError())
        return false;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogInfo("CaptureBeginGif()");
    IM_CHECK_RETV(args != NULL, false);
#ifndef IMGUI_TEST_ENGINE_DISABLE_CAPTURE
    if (!ImGuiTestContext_CanCapture(this))
        args->InFlags |= ImGuiCaptureFlags_NoSave;
    return ImGuiTestEngine_CaptureBeginGif(Engine, args);
#else
    IM_UNUSED(args);
    LogWarning("Skipped recording GIF: capture disabled by IMGUI_TEST_ENGINE_DISABLE_CAPTURE.");
    return false;
#endif
}

bool ImGuiTestContext::CaptureEndGif(ImGuiCaptureArgs* args)
{
    IM_CHECK_RETV(args != NULL, false);
    bool ret = Engine->CaptureContext.IsCapturingGif() && ImGuiTestEngine_CaptureEndGif(Engine, args);
    if (ret)
    {
        // In-progress capture was canceled by user. Delete incomplete file.
        if (IsError())
        {
            //ImFileDelete(args->OutSavedFileName);
            return false;
        }
        if (ImGuiTestContext_CanCapture(this))
            LogDebug("Saved '%s' (%d*%d pixels)", args->OutSavedFileName, (int)args->OutImageSize.x, (int)args->OutImageSize.y);
        else
            LogWarning("Skipped saving '%s' (%d*%d pixels) (enable in 'Misc->Options')", args->OutSavedFileName, (int)args->OutImageSize.x, (int)args->OutImageSize.y);
    }
    return ret;
}

// Supported values for ImGuiTestOpFlags:
// - ImGuiTestOpFlags_NoError
ImGuiTestItemInfo* ImGuiTestContext::ItemInfo(ImGuiTestRef ref, ImGuiTestOpFlags flags)
{
    if (IsError())
        return NULL;

    ImGuiID full_id = 0;

    // Wildcard matching
    // FIXME-TESTS: Need to verify that this is not inhibited by a \, so \**/ should not pass, but \\**/ should :)
    // We could add a simple helpers that would iterate the strings, handling inhibitors, and let you check if a given characters is inhibited or not.
    const char* wildcard_prefix_start = NULL;
    const char* wildcard_prefix_end = NULL;
    const char* wildcard_suffix_start = NULL;
    if (ref.Path)
        if (const char* p = strstr(ref.Path, "**/"))
        {
            wildcard_prefix_start = ref.Path;
            wildcard_prefix_end = p;
            wildcard_suffix_start = wildcard_prefix_end + 3;
        }

    if (wildcard_prefix_start)
    {
        // Wildcard matching
        ImGuiTestFindByLabelTask* task = &Engine->FindByLabelTask;
        if (wildcard_prefix_start < wildcard_prefix_end)
            task->InPrefixId = ImHashDecoratedPath(wildcard_prefix_start, wildcard_prefix_end, RefID);
        else
            task->InPrefixId = RefID;
        task->OutItemId = 0;

        // Advance pointer to point it to the last label
        task->InSuffix = task->InSuffixLastItem = wildcard_suffix_start;
        for (const char* c = task->InSuffix; *c; c++)
            if (*c == '/')
                task->InSuffixLastItem = c + 1;

        // Count number of labels
        task->InSuffixDepth = 0;
        for (const char* c = wildcard_suffix_start; *c; c++)
            if (*c == '/')
                task->InSuffixDepth++;

        LogDebug("Wildcard matching..");
        int retries = 0;
        while (retries < 2 && task->OutItemId == 0)
        {
            ImGuiTestEngine_Yield(Engine);
            retries++;
        }

        // Wildcard matching requires item to be visible, because clipped items are unaware of their labels. Try panning through entire window, searching for target item.
        // FIXME-TESTS: Scrollbar position restoration may be desirable, however it interferes with using found item.
        if (task->OutItemId == 0)
        {
            ImGuiTestItemInfo* base_item = ItemInfo(task->InPrefixId, ImGuiTestOpFlags_NoError);
            ImGuiWindow* window = base_item ? base_item->Window : GetWindowByRef(task->InPrefixId);
            if (window)
            {
                ImVec2 rect_size = window->InnerRect.GetSize();
                for (float scroll_x = 0.0f; task->OutItemId == 0; scroll_x += rect_size.x)
                {
                    for (float scroll_y = 0.0f; task->OutItemId == 0; scroll_y += rect_size.y)
                    {
                        window->Scroll.x = scroll_x;
                        window->Scroll.y = scroll_y;

                        retries = 0;
                        while (retries < 2 && task->OutItemId == 0)
                        {
                            ImGuiTestEngine_Yield(Engine);
                            retries++;
                        }
                        if (window->Scroll.y >= window->ScrollMax.y)
                            break;
                    }
                    if (window->Scroll.x >= window->ScrollMax.x)
                        break;
                }
            }
        }
        full_id = task->OutItemId;

        // FIXME: InFilterItemStatusFlags is not clear here intentionally, because it is set in ItemAction() and reused in later calls to ItemInfo() to resolve ambiguities.
        task->InPrefixId = 0;
        task->InSuffix = task->InSuffixLastItem = NULL;
        task->InSuffixDepth = 0;
        task->OutItemId = 0;    // -V1048   // Variable 'OutItemId' was assigned the same value. False-positive, because value of OutItemId could be modified from other thread during ImGuiTestEngine_Yield() call.
    }
    else
    {
        // Normal matching
        full_id = GetID(ref);
    }

    // If ui_ctx->TestEngineHooksEnabled is not already on (first ItemItem task in a while) we'll probably need an extra frame to warmup
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestItemInfo* item = NULL;
    int retries = 0;
    while (full_id && retries < 2)
    {
        item = ImGuiTestEngine_FindItemInfo(Engine, full_id, ref.Path);
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
            IM_ERRORF_NOHDR("Unable to locate item: '%s/%s' (0x%08X)", RefStr, ref.Path, full_id);
        else
            IM_ERRORF_NOHDR("Unable to locate item: 0x%08X", ref.ID);
    }

    return NULL;
}

void    ImGuiTestContext::ScrollToTop()
{
    if (IsError())
        return;

    ImGuiWindow* window = GetWindowByRef("");
    IM_CHECK_SILENT(window != NULL);
    if (window->Scroll.y == 0.0f)
        return;
    ScrollToY(0.0f);
    Yield();
}

void    ImGuiTestContext::ScrollToBottom()
{
    if (IsError())
        return;

    ImGuiWindow* window = GetWindowByRef("");
    IM_CHECK_SILENT(window != NULL);
    if (window->Scroll.y == window->ScrollMax.y)
        return;
    ScrollToY(window->ScrollMax.y);
    Yield();
}

bool    ImGuiTestContext::ScrollErrorCheck(ImGuiAxis axis, float expected, float actual, int* remaining_attempts)
{
    if (IsError())
        return false;

    float THRESHOLD = 1.0f;
    if (ImFabs(actual - expected) <= THRESHOLD)
        return true;

    (*remaining_attempts)--;
    if (*remaining_attempts > 0)
    {
        LogInfo("Failed to set Scroll%c. Requested %.2f, got %.2f. Will try again.", 'X' + axis, expected, actual);
        return true;
    }
    else
    {
        IM_ERRORF("Failed to set Scroll%c. Requested %.2f, got %.2f. Aborting.", 'X' + axis, expected, actual);
        return false;
    }
}

// FIXME-TESTS: Mostly the same code as ScrollbarEx()
static ImVec2 GetWindowScrollbarMousePositionForScroll(ImGuiWindow* window, ImGuiAxis axis, float scroll_v)
{
    ImGuiContext& g = *GImGui;
    ImRect bb = ImGui::GetWindowScrollbarRect(window, axis);

    // From Scrollbar():
    //float* scroll_v = &window->Scroll[axis];
    const float size_avail_v = window->InnerRect.Max[axis] - window->InnerRect.Min[axis];
    const float size_contents_v = window->ContentSize[axis] + window->WindowPadding[axis] * 2.0f;

    // From ScrollbarEx() onward:

    // V denote the main, longer axis of the scrollbar (= height for a vertical scrollbar)
    const float scrollbar_size_v = bb.Max[axis] - bb.Min[axis];

    // Calculate the height of our grabbable box. It generally represent the amount visible (vs the total scrollable amount)
    // But we maintain a minimum size in pixel to allow for the user to still aim inside.
    const float win_size_v = ImMax(ImMax(size_contents_v, size_avail_v), 1.0f);
    const float grab_h_pixels = ImClamp(scrollbar_size_v * (size_avail_v / win_size_v), g.Style.GrabMinSize, scrollbar_size_v);

    const float scroll_max = ImMax(1.0f, size_contents_v - size_avail_v);
    const float scroll_ratio = ImSaturate(scroll_v / scroll_max);
    const float grab_v = scroll_ratio * (scrollbar_size_v - grab_h_pixels); // Grab position

    ImVec2 position;
    position[axis] = bb.Min[axis] + grab_v + grab_h_pixels * 0.5f;
    position[axis ^ 1] = bb.GetCenter()[axis ^ 1];

    return position;
}

void    ImGuiTestContext::ScrollTo(ImGuiWindow* window, ImGuiAxis axis, float scroll_target)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    // Early out
    const float scroll_target_clamp = ImClamp(scroll_target, 0.0f, window->ScrollMax[axis]);
    if (ImFabs(window->Scroll[axis] - scroll_target_clamp) < 1.0f)
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    const char axis_c = (char)('X' + axis);
    LogDebug("ScrollTo %c %.1f/%.1f", axis_c, scroll_target, window->ScrollMax[axis]);

    WindowBringToFront(window);
    //Yield();

    // Try to use Scrollbar if available
    const ImGuiTestItemInfo* scrollbar_item = ItemInfo(ImGui::GetWindowScrollbarID(window, axis), ImGuiTestOpFlags_NoError);
    if (scrollbar_item != NULL && !EngineIO->ConfigRunFast)
    {
        const ImRect scrollbar_rect = ImGui::GetWindowScrollbarRect(window, axis);
        const float scrollbar_size_v = scrollbar_rect.Max[axis] - scrollbar_rect.Min[axis];
        const float window_resize_grip_size = IM_FLOOR(ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f));

        // In case of a very small window, directly use SetScrollX/Y function to prevent resizing it
        // FIXME-TESTS: GetWindowScrollbarMousePositionForScroll doesn't return the exact value when scrollbar grip is too small
        if (scrollbar_size_v >= window_resize_grip_size)
        {
            const float scroll_src = window->Scroll[axis];
            ImVec2 scrollbar_src_pos = GetWindowScrollbarMousePositionForScroll(window, axis, scroll_src);
            scrollbar_src_pos[axis] = ImMin(scrollbar_src_pos[axis], scrollbar_rect.Min[axis] + scrollbar_size_v - window_resize_grip_size);
            MouseMoveToPos(scrollbar_src_pos);
            MouseDown(0);
            SleepShort();

            ImVec2 scrollbar_dst_pos = GetWindowScrollbarMousePositionForScroll(window, axis, scroll_target_clamp);
            MouseMoveToPos(scrollbar_dst_pos);
            MouseUp(0);
            SleepShort();

            // Verify that things worked
            const float scroll_result = window->Scroll[axis];
            if (ImFabs(scroll_result - scroll_target_clamp) < 1.0f)
                return;

            // FIXME-TESTS: Investigate
            LogWarning("Failed to set Scroll%c. Requested %.2f, got %.2f.", 'X' + axis, scroll_target_clamp, scroll_result);
        }
    }

    // Fallback: manual slow scroll
    // FIXME-TESTS: Consider using mouse wheel
    int remaining_failures = 3;
    while (!Abort)
    {
        if (ImFabs(window->Scroll[axis] - scroll_target_clamp) < 1.0f)
            break;

        const float scroll_speed = EngineIO->ConfigRunFast ? FLT_MAX : ImFloor(EngineIO->ScrollSpeed * g.IO.DeltaTime + 0.99f);
        const float scroll_next = ImLinearSweep(window->Scroll[axis], scroll_target, scroll_speed);
        if (axis == ImGuiAxis_X)
            ImGui::SetScrollX(window, scroll_next);
        else
            ImGui::SetScrollY(window, scroll_next);

        // Error handling to avoid getting stuck in this function.
        Yield();
        if (!ScrollErrorCheck(axis, scroll_next, window->Scroll[axis], &remaining_failures))
            break;
    }

    // Need another frame for the result->Rect to stabilize
    Yield();
}

// FIXME-TESTS: scroll_ratio_y unsupported
void    ImGuiTestContext::ScrollToItemY(ImGuiTestRef ref, float scroll_ratio_y)
{
    IM_UNUSED(scroll_ratio_y);

    if (IsError())
        return;

    // If the item is not currently visible, scroll to get it in the center of our window
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestItemInfo* item = ItemInfo(ref);
    ImGuiTestRefDesc desc(ref, item);
    LogDebug("ScrollToItemY %s", desc.c_str());

    if (item == NULL)
        return;

    // Ensure window size and ScrollMax are up-to-date
    Yield();

    ImGuiWindow* window = item->Window;
    float item_curr_y = ImFloor(item->RectFull.GetCenter().y);
    float item_target_y = ImFloor(window->InnerClipRect.GetCenter().y);
    float scroll_delta_y = item_target_y - item_curr_y;
    float scroll_target_y = ImClamp(window->Scroll.y - scroll_delta_y, 0.0f, window->ScrollMax.y);

    ScrollTo(window, ImGuiAxis_Y, scroll_target_y);
}

void   ImGuiTestContext::ScrollToItemX(ImGuiTestRef ref)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    // If the item is not currently visible, scroll to get it in the center of our window
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestItemInfo* item = ItemInfo(ref);
    ImGuiTestRefDesc desc(ref, item);
    LogDebug("ScrollToItemX %s", desc.c_str());
    if (item == NULL)
        return;

    // Ensure window size and ScrollMax are up-to-date
    Yield();

    // TabBar are a special case because they have no scrollbar and rely on ScrollButton "<" and ">"
    // FIXME-TESTS: Consider moving to its own function.
    if (ImGuiTabBar* tab_bar = g.TabBars.GetByKey(item->ParentID))
    {
        ScrollToTabItem(tab_bar, item->ID);
    }
    else
    {
        ImGuiWindow* window = item->Window;
        float item_curr_x = ImFloor(item->RectFull.GetCenter().x);
        float item_target_x = ImFloor(window->InnerClipRect.GetCenter().x);
        float scroll_delta_x = item_target_x - item_curr_x;
        float scroll_target_x = ImClamp(window->Scroll.x - scroll_delta_x, 0.0f, window->ScrollMax.x);

        ScrollTo(window, ImGuiAxis_X, scroll_target_x);
    }
}

void    ImGuiTestContext::ScrollToTabItem(ImGuiTabBar* tab_bar, ImGuiID tab_id)
{
    if (IsError())
        return;

    // Cancel if "##v", because it's outside the tab_bar rect, and will be considered as "not visible" even if it is!
    //if (GetID("##v") == item->ID)
    //    return;

    IM_CHECK_SILENT(tab_bar != NULL);
    const ImGuiTabItem* selected_tab_item = ImGui::TabBarFindTabByID(tab_bar, tab_bar->SelectedTabId);
    const ImGuiTabItem* target_tab_item = ImGui::TabBarFindTabByID(tab_bar, tab_id);
    if (target_tab_item == NULL)
        return;

    int selected_tab_index = tab_bar->Tabs.index_from_ptr(selected_tab_item);
    int target_tab_index = tab_bar->Tabs.index_from_ptr(target_tab_item);

    ImGuiTestRef backup_ref = GetRef();
    SetRef(tab_bar->ID);

    if (selected_tab_index > target_tab_index)
    {
        MouseMove("##<");
        for (int i = 0; i < selected_tab_index - target_tab_index; ++i)
            MouseClick(0);
    }
    else
    {
        MouseMove("##>");
        for (int i = 0; i < target_tab_index - selected_tab_index; ++i)
            MouseClick(0);
    }

    // Skip the scroll animation
    if (EngineIO->ConfigRunFast)
    {
        tab_bar->ScrollingAnim = tab_bar->ScrollingTarget;
        Yield();
    }

    SetRef(backup_ref);
}

// Verify that ScrollMax is stable regardless of scrolling position
// - This can break when the layout of clipped items doesn't match layout of unclipped items
// - This can break with non-rounded calls to ItemSize(), namely when the starting position is negative (above visible area)
//   We should ideally be more tolerant of non-rounded sizes passed by the users.
// - One of the net visible effect of an unstable ScrollMax is that the End key would put you at a spot that's not exactly the lowest spot,
//   and so a second press to End would you move again by a few pixels.
// FIXME-TESTS: Make this an iterative, smooth scroll.
void    ImGuiTestContext::ScrollVerifyScrollMax(ImGuiWindow* window)
{
    ImGui::SetScrollY(window, 0.0f);
    Yield();
    float scroll_max_0 = window->ScrollMax.y;
    ImGui::SetScrollY(window, window->ScrollMax.y);
    Yield();
    float scroll_max_1 = window->ScrollMax.y;
    IM_CHECK_EQ(scroll_max_0, scroll_max_1);
}

void    ImGuiTestContext::NavMoveTo(ImGuiTestRef ref)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiContext& g = *UiContext;
    ImGuiTestItemInfo* item = ItemInfo(ref);
    ImGuiTestRefDesc desc(ref, item);
    LogDebug("NavMove to %s", desc.c_str());

    if (item == NULL)
        return;
    item->RefCount++;

    // Focus window before scrolling/moving so things are nicely visible
    WindowBringToFront(item->Window);

    // Teleport
    // FIXME-NAV: We should have a nav request feature that does this,
    // except it'll have to queue the request to find rect, then set scrolling, which would incur a 2 frame delay :/
    // FIXME-TESTS-NOT_SAME_AS_END_USER
    IM_ASSERT(g.NavMoveSubmitted == false);
    ImRect rect_rel = item->RectFull;
    rect_rel.Translate(ImVec2(-item->Window->Pos.x, -item->Window->Pos.y));
    ImGui::SetNavID(item->ID, (ImGuiNavLayer)item->NavLayer, 0, rect_rel);
    g.NavDisableHighlight = false;
    g.NavDisableMouseHover = g.NavMousePosDirty = true;
    ImGui::ScrollToBringRectIntoView(item->Window, item->RectFull);
    while (g.NavMoveSubmitted)
        Yield();
    Yield();

    if (!Abort)
    {
        if (g.NavId != item->ID)
            IM_ERRORF_NOHDR("Unable to set NavId to %s", desc.c_str());
    }

    item->RefCount--;
}

void    ImGuiTestContext::NavKeyPress(ImGuiNavInput input)
{
    NavKeyDown(input);
    NavKeyUp(input);
}

void    ImGuiTestContext::NavKeyDown(ImGuiNavInput input)
{
    IM_ASSERT(input >= 0 && input < ImGuiNavInput_COUNT);
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("NavKeyDown %d", (int)input);

    Engine->Inputs.Queue.push_back(ImGuiTestInput::FromNav(input, ImGuiKeyState_Down));
    Yield();
}

void    ImGuiTestContext::NavKeyUp(ImGuiNavInput input)
{
    IM_ASSERT(input >= 0 && input < ImGuiNavInput_COUNT);
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("NavKeyUp %d", (int)input);

    Engine->Inputs.Queue.push_back(ImGuiTestInput::FromNav(input, ImGuiKeyState_Up));
    Yield();
    Yield(); // For nav code to react e.g. run a query
}

void    ImGuiTestContext::NavActivate()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("NavActivate");

    Yield(); // ?
    NavKeyPress(ImGuiNavInput_Activate);
}

void    ImGuiTestContext::NavInput()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("NavInput");
    NavKeyPress(ImGuiNavInput_Input);
}

void    ImGuiTestContext::NavEnableForWindow()
{
    // FIXME-TESTS: this shouldn't be required, currently used as a kludge (see e.g #2048)
    KeyModPress(ImGuiKeyModFlags_Alt);
    KeyModPress(ImGuiKeyModFlags_Alt);
}

// Supported values for ImGuiTestOpFlags:
// - ImGuiTestOpFlags_MoveToEdgeL
// - ImGuiTestOpFlags_MoveToEdgeR
// - ImGuiTestOpFlags_MoveToEdgeU
// - ImGuiTestOpFlags_MoveToEdgeD
static ImVec2 GetMouseAimingPos(ImGuiTestItemInfo* item, ImGuiTestOpFlags flags)
{
    ImRect r = item->RectClipped;
    ImVec2 pos;
    if (flags & ImGuiTestOpFlags_MoveToEdgeL)
        pos.x = (r.Min.x + 1.0f);
    else if (flags & ImGuiTestOpFlags_MoveToEdgeR)
        pos.x = (r.Max.x - 1.0f);
    else
        pos.x = (r.Min.x + r.Max.x) * 0.5f;
    if (flags & ImGuiTestOpFlags_MoveToEdgeU)
        pos.y = (r.Min.y + 1.0f);
    else if (flags & ImGuiTestOpFlags_MoveToEdgeD)
        pos.y = (r.Max.y - 1.0f);
    else
        pos.y = (r.Min.y + r.Max.y) * 0.5f;
    return pos;
}

// Supported values for ImGuiTestOpFlags:
// - ImGuiTestOpFlags_NoFocusWindow
// - ImGuiTestOpFlags_NoCheckHoveredId
// - ImGuiTestOpFlags_IsSecondAttempt [used when recursively calling ourself)
// FIXME-TESTS: This is too eagerly trying to scroll everything even if already visible.
// FIXME: Maybe ImGuiTestOpFlags_NoCheckHoveredId could be automatic if we detect that another item is active as intended?
void    ImGuiTestContext::MouseMove(ImGuiTestRef ref, ImGuiTestOpFlags flags)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiContext& g = *UiContext;
    ImGuiTestItemInfo* item = ItemInfo(ref);
    ImGuiTestRefDesc desc(ref, item);
    LogDebug("MouseMove to %s", desc.c_str());

    if (item == NULL)
        return;

    if (!item->Window->WasActive)
    {
        LogError("Window '%s' is not active!", item->Window->Name);
        return;
    }

    item->RefCount++;

    // Focus window before scrolling/moving so things are nicely visible
    // FIXME-TESTS-NOT_SAME_AS_END_USER: This has too many side effect, could we do without?
    if (!(flags & ImGuiTestOpFlags_NoFocusWindow))
        WindowBringToFront(item->Window);

    // Another active test (in the case focus change has a side effect BUT also implicitly we have yield an extra frame)
    if (!item->Window->WasActive)
    {
        LogError("Window '%s' is not active (after focusing)", item->Window->Name);
        return;
    }

    // Scroll to make item visible
    ImGuiWindow* window = item->Window;
    ImRect window_inner_r_padded = window->InnerClipRect;
    window_inner_r_padded.Expand(ImVec2(-g.WindowsHoverPadding.x, -g.WindowsHoverPadding.y));
    if (item->NavLayer == ImGuiNavLayer_Main)
    {
        bool contains_y = (item->RectClipped.Min.y >= window_inner_r_padded.Min.y && item->RectClipped.Max.y <= window_inner_r_padded.Max.y);
        bool contains_x = (item->RectClipped.Min.x >= window_inner_r_padded.Min.x && item->RectClipped.Max.x <= window_inner_r_padded.Max.x);
        if (!contains_y)
            ScrollToItemY(ref);
        if (!contains_x)
            ScrollToItemX(ref);
    }

    // FIXME-TESTS-NOT_SAME_AS_END_USER
    ImVec2 pos = item->RectFull.GetCenter();
    WindowTeleportToMakePosVisibleInViewport(window, pos);

    // Move toward an actually visible point
    pos = GetMouseAimingPos(item, flags);
    MouseMoveToPos(pos);

    // Focus again in case something made us lost focus (which could happen on a simple hover)
    if (!(flags & ImGuiTestOpFlags_NoFocusWindow))
    {
        //LogDebug("Window '%s' not focused anymore, focusing again", window->Name);
        WindowBringToFront(window);

        // 2021-12-13: straying away from that kind of stuff. We'd better error or handle hover without requiring focus.
#if 0
        // Some windows are refusing to get focused.... e.g. MainMenuBar will relinguish focus... need to hide other windows.
#ifdef IMGUI_HAS_DOCK
        if (window->RootWindowDockTree != g.Windows.back()->RootWindowDockTree)
#else
        if (window->RootWindow != g.Windows.back()->RootWindow)
#endif
        {
            LogDebug("Window '%s' not focused anymore: hiding other windows", window->Name);
            ImGuiWindow* ignore_list[] = { window, NULL };
            ForeignWindowsHideOverPos(pos, ignore_list);
        }
#endif
    }

    // Check hovering target
    if (!Abort && !(flags & ImGuiTestOpFlags_NoCheckHoveredId))
    {
        const ImGuiID hovered_id = g.HoveredIdPreviousFrame;
        if (hovered_id != item->ID)
        {
            if (!(window->Flags & ImGuiWindowFlags_NoResize) && !(flags & ImGuiTestOpFlags_IsSecondAttempt))
            {
                bool is_resize_corner = false;
#if IMGUI_VERSION_NUM < 18203
                for (int n = 0; n < 2; n++)
                    is_resize_corner |= (hovered_id == ImGui::GetWindowResizeID(window, n));
#else
                for (int n = 0; n < 2; n++)
                    is_resize_corner |= (hovered_id == ImGui::GetWindowResizeCornerID(window, n));
#endif
                if (is_resize_corner)
                {
                    LogDebug("Obstructed by ResizeGrip, trying to resize window and trying again..");
                    float extra_size = window->CalcFontSize() * 3.0f;
                    WindowResize(window->ID, window->Size + ImVec2(extra_size, extra_size));
                    MouseMove(ref, flags | ImGuiTestOpFlags_IsSecondAttempt);
                    item->RefCount--;
                    return;
                }
            }

            IM_ERRORF_NOHDR(
                "Unable to Hover %s:\n"
                "- Expected item %08X in window '%s', targeted position: (%.1f,%.1f)'\n"
                "- Hovered id was %08X in '%s'.",
                desc.c_str(),
                item->ID, item->Window ? item->Window->Name : "<NULL>", pos.x, pos.y,
                hovered_id, g.HoveredWindow ? g.HoveredWindow->Name : "");
        }
    }

    item->RefCount--;
}

// Make the point at 'pos' (generally expected to be within window's boundaries) visible in the viewport,
// so it can be later focused then clicked.
bool    ImGuiTestContext::WindowTeleportToMakePosVisibleInViewport(ImGuiWindow* window, ImVec2 pos)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return false;

#ifdef IMGUI_HAS_DOCK
    // This is particularly useful for docked windows, as we have to move root dockspace window instead of docket window
    // itself. As a side effect this also adds support for child windows.
    window = window->RootWindowDockTree;
#endif

    ImRect visible_r;
    visible_r.Min = ImGui::GetMainViewport()->Pos;
    visible_r.Max = visible_r.Min + ImGui::GetMainViewport()->Size;
    if (!visible_r.Contains(pos))
    {
        // Fallback move window directly to make our item reachable with the mouse.
        // FIXME-TESTS-NOT_SAME_AS_END_USER
        float pad = g.FontSize;
        ImVec2 delta;
        delta.x = (pos.x < visible_r.Min.x) ? (visible_r.Min.x - pos.x + pad) : (pos.x > visible_r.Max.x) ? (visible_r.Max.x - pos.x - pad) : 0.0f;
        delta.y = (pos.y < visible_r.Min.y) ? (visible_r.Min.y - pos.y + pad) : (pos.y > visible_r.Max.y) ? (visible_r.Max.y - pos.y - pad) : 0.0f;
        ImGui::SetWindowPos(window, window->Pos + delta, ImGuiCond_Always);
        LogDebug("WindowMoveBypass %s delta (%.1f,%.1f)", window->Name, delta.x, delta.y);
        Yield();
        return true;
    }
    return false;
}

// ignore_list is a NULL-terminated list of pointers
// FIXME-TESTS-NOT_SAME_AS_END_USER: Aim to get rid of this.
void ImGuiTestContext::ForeignWindowsHideOverPos(ImVec2 pos, ImGuiWindow** ignore_list)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("ForeignWindowsHideOverPos (%.0f,%.0f)", pos.x, pos.y);
    IM_CHECK_SILENT(ignore_list != NULL); // It makes little sense to call this function with an empty list.
    IM_CHECK_SILENT(ignore_list[0] != NULL);
    //auto& ctx = this;  IM_DEBUG_HALT_TESTFUNC();

    bool hidden_windows = false;
    for (int i = 0; i < g.Windows.Size; i++)
    {
        ImGuiWindow* other_window = g.Windows[i];
        if (other_window->RootWindow == other_window && other_window->WasActive)
        {
            ImRect r = other_window->Rect();
            r.Expand(g.WindowsHoverPadding);
            if (r.Contains(pos))
            {
                for (int j = 0; ignore_list[j]; j++)
#ifdef IMGUI_HAS_DOCK
                    if (ignore_list[j]->RootWindowDockTree == other_window->RootWindowDockTree)
#else
                    if (ignore_list[j] == other_window)
#endif
                    {
                        other_window = NULL;
                        break;
                    }
                if (other_window)
                {
                    ForeignWindowsToHide.push_back(other_window);
                    hidden_windows = true;
                }
            }
        }
    }
    if (hidden_windows)
        Yield();
}

void ImGuiTestContext::ForeignWindowsUnhideAll()
{
    ForeignWindowsToHide.clear();
    Yield();
}

void	ImGuiTestContext::MouseMoveToPos(ImVec2 target)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MouseMoveToPos from (%.0f,%.0f) to (%.0f,%.0f)", Inputs->MousePosValue.x, Inputs->MousePosValue.y, target.x, target.y);

    // Enforce a mouse move if we are already at destination, to enforce g.NavDisableMouseHover gets cleared.
    if (g.NavDisableMouseHover && ImLengthSqr(Inputs->MousePosValue - target) < 1.0f)
    {
        Inputs->MousePosValue = target + ImVec2(1.0f, 0.0f);
        ImGuiTestEngine_Yield(Engine);
    }

    if (EngineIO->ConfigRunFast)
    {
        Inputs->MousePosValue = target;
        ImGuiTestEngine_Yield(Engine);
        ImGuiTestEngine_Yield(Engine);
        return;
    }

    // Simulate slower movements. We use a slightly curved movement to make the movement look less robotic.

    // Calculate some basic parameters
    const ImVec2 start_pos = Inputs->MousePosValue;
    const ImVec2 delta = target - start_pos;
    const float length2 = ImLengthSqr(delta);
    const float length = (length2 > 0.0001f) ? ImSqrt(length2) : 1.0f;
    const float inv_length = 1.0f / length;

    // Calculate a vector perpendicular to the motion delta
    const ImVec2 perp = ImVec2(delta.y, -delta.x) * inv_length;

    // Calculate how much wobble we want, clamped to max out when the delta is 100 pixels (shorter movements get less wobble)
    const float position_offset_magnitude = ImClamp(length, 1.0f, 100.0f) * EngineIO->MouseWobble;

    // Wobble positions, using a sine wave based on position as a cheap way to get a deterministic offset
    ImVec2 intermediate_pos_a = start_pos + (delta * 0.3f);
    ImVec2 intermediate_pos_b = start_pos + (delta * 0.6f);
    intermediate_pos_a += perp * ImSin(intermediate_pos_a.y * 0.1f) * position_offset_magnitude;
    intermediate_pos_b += perp * ImCos(intermediate_pos_b.y * 0.1f) * position_offset_magnitude;

    // We manipulate Inputs->MousePosValue without reading back from g.IO.MousePos because the later is rounded.
    // To handle high framerate it is easier to bypass this rounding.
    float current_dist = 0.0f; // Our current distance along the line (in pixels)
    while (true)
    {
        float move_speed = EngineIO->MouseSpeed * g.IO.DeltaTime;

        //if (g.IO.KeyShift)
        //    move_speed *= 0.1f;

        current_dist += move_speed; // Move along the line

        // Calculate a parametric position on the direct line that we will use for the curve
        float t = current_dist * inv_length;
        t = ImClamp(t, 0.0f, 1.0f);
        t = 1.0f - ((ImCos(t * IM_PI) + 1.0f) * 0.5f); // Generate a smooth curve with acceleration/deceleration

        //ImGui::GetOverlayDrawList()->AddCircle(target, 10.0f, IM_COL32(255, 255, 0, 255));

        if (t >= 1.0f)
        {
            Inputs->MousePosValue = target;
            ImGuiTestEngine_Yield(Engine);
            ImGuiTestEngine_Yield(Engine);
            return;
        }
        else
        {
            // Use a bezier curve through the wobble points
            Inputs->MousePosValue = ImBezierCubicCalc(start_pos, intermediate_pos_a, intermediate_pos_b, target, t);
            //ImGui::GetOverlayDrawList()->AddBezierCurve(start_pos, intermediate_pos_a, intermediate_pos_b, target, IM_COL32(255,0,0,255), 1.0f);
            ImGuiTestEngine_Yield(Engine);
        }
    }
}

// This always teleport the mouse regardless of fast/slow mode. Useful e.g. to set initial mouse position for a GIF recording.
void	ImGuiTestContext::MouseTeleportToPos(ImVec2 target)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MouseTeleportToPos from (%.0f,%.0f) to (%.0f,%.0f)", Inputs->MousePosValue.x, Inputs->MousePosValue.y, target.x, target.y);

    Inputs->MousePosValue = target;
    ImGuiTestEngine_Yield(Engine);
    ImGuiTestEngine_Yield(Engine);
}

void    ImGuiTestContext::MouseDown(ImGuiMouseButton button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MouseDown %d", button);

    UiContext->IO.MouseClickedTime[button] = -FLT_MAX; // Prevent accidental double-click from happening ever
    Inputs->MouseButtonsValue = (1 << button);
    Yield();
}

void    ImGuiTestContext::MouseUp(ImGuiMouseButton button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MouseUp %d", button);

    Inputs->MouseButtonsValue &= ~(1 << button);
    Yield();
}

// TODO: click time argument (seconds and/or frames)
void    ImGuiTestContext::MouseClick(ImGuiMouseButton button)
{
    if (IsError())
        return;
    MouseClickMulti(button, 1);
}

// TODO: click time argument (seconds and/or frames)
void    ImGuiTestContext::MouseClickMulti(ImGuiMouseButton button, int count)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    if (count > 1)
        LogDebug("MouseClickMulti %d x%d", button, count);
    else
        LogDebug("MouseClick %d", button);

    // Make sure mouse buttons are released
    IM_ASSERT(count >= 1);
    IM_ASSERT(Inputs->MouseButtonsValue == 0);
    Yield();

    // Press
    UiContext->IO.MouseClickedTime[button] = -FLT_MAX; // Prevent accidental double-click from happening ever

    for (int n = 0; n < count; n++)
    {
        Inputs->MouseButtonsValue = (1 << button);
        Yield();
        Inputs->MouseButtonsValue = 0;
        Yield();
    }

    // Now NewFrame() has seen the mouse release.
    // Let the imgui frame finish, now e.g. Button() function will return true. Start a new frame.
    Yield();
}

// TODO: click time argument (seconds and/or frames)
void    ImGuiTestContext::MouseDoubleClick(ImGuiMouseButton button)
{
    MouseClickMulti(button, 2);
}

void    ImGuiTestContext::MouseLiftDragThreshold(ImGuiMouseButton button)
{
    if (IsError())
        return;

    ImGuiContext& g = *UiContext;
    g.IO.MouseDragMaxDistanceAbs[button] = ImVec2(g.IO.MouseDragThreshold, g.IO.MouseDragThreshold);
    g.IO.MouseDragMaxDistanceSqr[button] = (g.IO.MouseDragThreshold * g.IO.MouseDragThreshold) + (g.IO.MouseDragThreshold * g.IO.MouseDragThreshold);
}

static bool IsPosOnVoid(ImGuiContext& g, const ImVec2& pos)
{
    for (ImGuiWindow* window : g.Windows)
#ifdef IMGUI_HAS_DOCK
        if (window->RootWindowDockTree == window && window->WasActive)
#else
        if (window->RootWindow == window && window->WasActive)
#endif
        {
            ImRect r = window->Rect();
            r.Expand(g.WindowsHoverPadding);
            if (r.Contains(pos))
                return false;
        }
    return true;
}

// Sample viewport for an easy location with nothing on it.
// FIXME-OPT: If ever any problematic:
// - (1) could iterate g.WindowsFocusOrder[] once we make the switch of it only containing root windows
// - (2) increase steps iteratively
// - (3) remember last answer and tries it first.
// - (4) shortcut negative if a window covers the whole viewport
bool    ImGuiTestContext::FindExistingVoidPosOnViewport(ImGuiViewport* viewport, ImVec2* out)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return false;

    for (int yn = 0; yn < 20; yn++)
        for (int xn = 0; xn < 20; xn++)
        {
            ImVec2 pos = viewport->Pos + viewport->Size * ImVec2(xn / 20.0f, yn / 20.0f);
            if (!IsPosOnVoid(g, pos))
                continue;
            *out = pos;
            return true;
        }
    return false;
}

ImVec2   ImGuiTestContext::GetPosOnVoid()
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return ImVec2();

    ImVec2 void_pos;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    bool found_existing_void_pos = FindExistingVoidPosOnViewport(viewport, &void_pos);
    if (found_existing_void_pos)
        return void_pos;

    // Move windows away
    void_pos = viewport->Pos + ImVec2(1, 1);
    ImVec2 window_min_pos = void_pos + g.WindowsHoverPadding + ImVec2(1.0f, 1.0f);
    for (ImGuiWindow* window : g.Windows)
#ifdef IMGUI_HAS_DOCK
        if (window->RootWindowDockTree == window && window->WasActive)
#else
        if (window->RootWindow == window && window->WasActive)
#endif
            if (window->Rect().Contains(window_min_pos))
                WindowMove(window->Name, window_min_pos);

    return void_pos;
}

ImVec2  ImGuiTestContext::GetWindowTitlebarPoint(ImGuiTestRef window_ref)
{
    // FIXME-TESTS: Need to find a -visible- click point. drag_pos may end up being outside of main viewport.
    if (IsError())
        return ImVec2();

    ImGuiWindow* window = GetWindowByRef(window_ref);
    if (window == NULL)
    {
        IM_ERRORF_NOHDR("Unable to locate ref window: '%s'", window_ref.Path);
        return ImVec2();
    }

    ImVec2 drag_pos;
    for (int n = 0; n < 2; n++)
    {
#ifdef IMGUI_HAS_DOCK
        if (window->DockNode != NULL && window->DockNode->TabBar != NULL)
        {
            ImGuiTabBar* tab_bar = window->DockNode->TabBar;
            ImGuiTabItem* tab = ImGui::TabBarFindTabByID(tab_bar, window->ID);
            IM_ASSERT(tab != NULL);
            drag_pos = tab_bar->BarRect.Min + ImVec2(tab->Offset + tab->Width * 0.5f, tab_bar->BarRect.GetHeight() * 0.5f);
        }
        else
#endif
        {
            const float h = window->TitleBarHeight();
            drag_pos = ImFloor(window->Pos + ImVec2(window->Size.x, h) * 0.5f);
        }

        // If we didn't have to teleport it means we can reach the position already
        if (!WindowTeleportToMakePosVisibleInViewport(window, drag_pos))
            break;
    }
    return drag_pos;
}

void    ImGuiTestContext::MouseClickOnVoid(int mouse_button)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MouseClickOnVoid %d", mouse_button);

    // Click position which should now be empty space.
    MouseMoveToPos(GetPosOnVoid());
    IM_CHECK(g.HoveredWindow == NULL);
    MouseClick(mouse_button);
}

void    ImGuiTestContext::MouseDragWithDelta(ImVec2 delta, ImGuiMouseButton button)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MouseDragWithDelta %d (%.1f, %.1f)", button, delta.x, delta.y);

    MouseDown(button);
    MouseMoveToPos(g.IO.MousePos + delta);
    MouseUp(button);
}

void    ImGuiTestContext::MouseWheel(float vertical, float horizontal)
{
    MouseWheel(ImVec2(horizontal, vertical));
}

// Important: always call MouseWheel() with an understand that holding Shift will swap axises.
// - On Windows/Linux, this swap is done in ImGui::NewFrame()
// - On OSX, this swap is generally done by the backends
// - In simulated test engine, always assume Windows/Linux behavior as we will swap in ImGuiTestEngine_ApplyInputToImGuiContext()
void    ImGuiTestContext::MouseWheel(ImVec2 delta)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);

    LogDebug("MouseWheel(%d, %d)", (int)delta.x, (int)delta.y);

    float td = 0.0f;
    const float scroll_speed = 15.0f; // Units per second.
    while (delta.x != 0.0f || delta.y != 0.0f)
    {
        ImVec2 scroll;
        if (EngineIO->ConfigRunFast)
        {
            scroll = delta;
        }
        else
        {
            td += UiContext->IO.DeltaTime;
            scroll = ImFloor(delta * ImVec2(td, td) * scroll_speed);
        }

        if (scroll.x != 0.0f || scroll.y != 0.0f)
        {
            scroll = ImClamp(scroll, ImVec2(ImMin(delta.x, 0.0f), ImMin(delta.y, 0.0f)), ImVec2(ImMax(delta.x, 0.0f), ImMax(delta.y, 0.0f)));
            Inputs->MouseWheel = scroll;
            delta -= scroll;
            td = 0;
        }
        Yield();
    }
}

void    ImGuiTestContext::KeyDownMap(ImGuiKey key, int mod_flags)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    char mod_flags_str[32];
    GetImGuiKeyModsPrefixStr(mod_flags, mod_flags_str, IM_ARRAYSIZE(mod_flags_str));
    LogDebug("KeyDownMap(%s%s)", mod_flags_str, (key != ImGuiKey_COUNT) ? GetImGuiKeyName(key) : "");
    Inputs->Queue.push_back(ImGuiTestInput::FromKey(key, ImGuiKeyState_Down, mod_flags));
    Yield();
    Yield();
}

void    ImGuiTestContext::KeyUpMap(ImGuiKey key, int mod_flags)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    char mod_flags_str[32];
    GetImGuiKeyModsPrefixStr(mod_flags, mod_flags_str, IM_ARRAYSIZE(mod_flags_str));
    LogDebug("KeyUpMap(%s%s)", mod_flags_str, (key != ImGuiKey_COUNT) ? GetImGuiKeyName(key) : "");
    Inputs->Queue.push_back(ImGuiTestInput::FromKey(key, ImGuiKeyState_Up, mod_flags));
    Yield();
    Yield();
}

void    ImGuiTestContext::KeyPressMap(ImGuiKey key, int mod_flags, int count)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    char mod_flags_str[32];
    GetImGuiKeyModsPrefixStr(mod_flags, mod_flags_str, IM_ARRAYSIZE(mod_flags_str));
    LogDebug("KeyPressMap(%s%s, %d)", mod_flags_str, (key != ImGuiKey_COUNT) ? GetImGuiKeyName(key) : "", count);
    while (count > 0)
    {
        count--;
        Inputs->Queue.push_back(ImGuiTestInput::FromKey(key, ImGuiKeyState_Down, mod_flags));
        Yield();
        Inputs->Queue.push_back(ImGuiTestInput::FromKey(key, ImGuiKeyState_Up, mod_flags));
        Yield();

        // Give a frame for items to react
        Yield();
    }
}

void    ImGuiTestContext::KeyHoldMap(ImGuiKey key, int mod_flags, float time)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    char mod_flags_str[32];
    GetImGuiKeyModsPrefixStr(mod_flags, mod_flags_str, IM_ARRAYSIZE(mod_flags_str));
    LogDebug("KeyHoldMap(%s%s, %.2f sec)", mod_flags_str, (key != ImGuiKey_COUNT) ? GetImGuiKeyName(key) : "", time);

    Inputs->Queue.push_back(ImGuiTestInput::FromKey(key, ImGuiKeyState_Down, mod_flags));
    SleepNoSkip(time, 1 / 100.0f);
    Inputs->Queue.push_back(ImGuiTestInput::FromKey(key, ImGuiKeyState_Up, mod_flags));
    Yield(); // Give a frame for items to react
}

void    ImGuiTestContext::KeyChars(const char* chars)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("KeyChars('%s')", chars);
    while (*chars)
    {
        unsigned int c = 0;
        int bytes_count = ImTextCharFromUtf8(&c, chars, NULL);
        chars += bytes_count;
        if (c > 0 && c <= 0xFFFF)
            Inputs->Queue.push_back(ImGuiTestInput::FromChar((ImWchar)c));

        if (!EngineIO->ConfigRunFast)
            Sleep(1.0f / EngineIO->TypingSpeed);
    }
    Yield();
}

void    ImGuiTestContext::KeyCharsAppend(const char* chars)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("KeyCharsAppend('%s')", chars);
    KeyPressMap(ImGuiKey_End);
    KeyChars(chars);
}

void    ImGuiTestContext::KeyCharsAppendEnter(const char* chars)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("KeyCharsAppendEnter('%s')", chars);
    KeyPressMap(ImGuiKey_End);
    KeyChars(chars);
    KeyPressMap(ImGuiKey_Enter);
}

void    ImGuiTestContext::KeyCharsReplace(const char* chars)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("KeyCharsReplace('%s')", chars);
    KeyPressMap(ImGuiKey_A, ImGuiKeyModFlags_Shortcut);
    if (chars[0])
        KeyChars(chars);
    else
        KeyPressMap(ImGuiKey_Delete);
}

void    ImGuiTestContext::KeyCharsReplaceEnter(const char* chars)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("KeyCharsReplaceEnter('%s')", chars);
    KeyPressMap(ImGuiKey_A, ImGuiKeyModFlags_Shortcut);
    if (chars[0])
        KeyChars(chars);
    else
        KeyPressMap(ImGuiKey_Delete);
    KeyPressMap(ImGuiKey_Enter);
}

// Supported values for ImGuiTestOpFlags:
// - ImGuiTestOpFlags_NoError
bool    ImGuiTestContext::WindowBringToFront(ImGuiWindow* window, ImGuiTestOpFlags flags)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return false;

    if (window == NULL)
    {
        ImGuiID window_id = GetID("");
        window = ImGui::FindWindowByID(window_id);
        IM_ASSERT(window != NULL);
    }

    if (window != g.NavWindow)
    {
        IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
        LogDebug("BringWindowToFront->FocusWindow('%s')", window->Name);
        ImGui::FocusWindow(window); // FIXME-TESTS-NOT_SAME_AS_END_USER
        Yield();
        Yield();
        //IM_CHECK(g.NavWindow == window);
    }
    else if (window->RootWindow != g.Windows.back()->RootWindow)
    {
        IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
        LogDebug("BringWindowToDisplayFront('%s') (window.back=%s)", window->Name, g.Windows.back()->Name);
        ImGui::BringWindowToDisplayFront(window); // FIXME-TESTS-NOT_SAME_AS_END_USER
        Yield();
        Yield();
    }

    // We cannot guarantee this will work 100%
    // Because merely hovering an item may e.g. open a window or change focus.
    // In particular this can be the case with MenuItem. So trying to Open a MenuItem may lead to its child opening while hovering,
    // causing this function to seemingly fail (even if the end goal was reached).
    bool ret = (window == g.NavWindow);
    if (!ret && !(flags & ImGuiTestOpFlags_NoError))
        LogDebug("-- Expected focused window '%s', but '%s' got focus back.", window->Name, g.NavWindow ? g.NavWindow->Name : "<NULL>");

    return ret;
}

void    ImGuiTestContext::GatherItems(ImGuiTestItemList* out_list, ImGuiTestRef parent, int depth)
{
    IM_ASSERT(out_list != NULL);
    IM_ASSERT(depth > 0 || depth == -1);
    IM_ASSERT(GatherTask->InParentID == 0);
    IM_ASSERT(GatherTask->LastItemInfo == NULL);

    if (IsError())
        return;

    // Register gather tasks
    if (depth == -1)
        depth = 99;
    if (parent.ID == 0)
        parent.ID = GetID(parent);
    GatherTask->InParentID = parent.ID;
    GatherTask->InDepth = depth;
    GatherTask->OutList = out_list;

    // Keep running while gathering
    const int begin_gather_size = out_list->GetSize();
    while (true)
    {
        const int begin_gather_size_for_frame = out_list->GetSize();
        Yield();
        const int end_gather_size_for_frame = out_list->GetSize();
        if (begin_gather_size_for_frame == end_gather_size_for_frame)
            break;
    }
    const int end_gather_size = out_list->GetSize();

    // FIXME-TESTS: To support filter we'd need to process the list here,
    // Because ImGuiTestItemList is a pool (ImVector + map ID->index) we'll need to filter, rewrite, rebuild map

    ImGuiTestItemInfo* parent_item = ItemInfo(parent, ImGuiTestOpFlags_NoError);
    LogDebug("GatherItems from %s, %d deep: found %d items.", ImGuiTestRefDesc(parent, parent_item).c_str(), depth, end_gather_size - begin_gather_size);

    GatherTask->InParentID = 0;
    GatherTask->InDepth = 0;
    GatherTask->OutList = NULL;
    GatherTask->LastItemInfo = NULL;
}

void    ImGuiTestContext::ItemAction(ImGuiTestAction action, ImGuiTestRef ref, void* action_arg, ImGuiTestOpFlags flags)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);

    // [DEBUG] Breakpoint
    //if (ref.ID == 0x0d4af068)
    //    printf("");

    // FIXME-TESTS: Fix that stuff
    const bool is_wildcard = ref.Path != NULL && strstr(ref.Path, "**/") != 0;
    if (is_wildcard)
    {
        // This is a fragile way to avoid some ambiguities, we're relying on expected action to further filter by status flags.
        // These flags are not cleared by ItemInfo() because ItemAction() may call ItemInfo() again to get same item and thus it
        // needs these flags to remain in place.
        if (action == ImGuiTestAction_Check || action == ImGuiTestAction_Uncheck)
            Engine->FindByLabelTask.InFilterItemStatusFlags = ImGuiItemStatusFlags_Checkable;
        else if (action == ImGuiTestAction_Open || action == ImGuiTestAction_Close)
            Engine->FindByLabelTask.InFilterItemStatusFlags = ImGuiItemStatusFlags_Openable;
    }

    ImGuiTestItemInfo* item = ItemInfo(ref);
    ImGuiTestRefDesc desc(ref, item);
    if (item == NULL)
        return;

    LogDebug("Item%s %s%s", GetActionName(action), desc.c_str(), (InputMode == ImGuiInputSource_Mouse) ? "" : " (w/ Nav)");

    // Automatically uncollapse by default
    if (item->Window && !(OpFlags & ImGuiTestOpFlags_NoAutoUncollapse))
        WindowCollapse(item->Window, false);

    if (action == ImGuiTestAction_Hover)
    {
        MouseMove(ref, flags);
    }
    if (action == ImGuiTestAction_Click || action == ImGuiTestAction_DoubleClick)
    {
        if (InputMode == ImGuiInputSource_Mouse)
        {
            const int mouse_button = (int)(intptr_t)action_arg;
            IM_ASSERT(mouse_button >= 0 && mouse_button < ImGuiMouseButton_COUNT);
            MouseMove(ref, flags);
            if (!EngineIO->ConfigRunFast)
                Sleep(0.05f);
            if (action == ImGuiTestAction_DoubleClick)
                MouseDoubleClick(mouse_button);
            else
                MouseClick(mouse_button);
        }
        else
        {
            action = ImGuiTestAction_NavActivate;
        }
    }

    if (action == ImGuiTestAction_NavActivate)
    {
        IM_ASSERT(action_arg == NULL); // Unused
        NavMoveTo(ref);
        NavActivate();
        if (action == ImGuiTestAction_DoubleClick)
            IM_ASSERT(0);
    }
    else if (action == ImGuiTestAction_Input)
    {
        IM_ASSERT(action_arg == NULL); // Unused
        if (InputMode == ImGuiInputSource_Mouse)
        {
            MouseMove(ref, flags);
            KeyModDown(ImGuiKeyModFlags_Ctrl);
            MouseClick(0);
            KeyModUp(ImGuiKeyModFlags_Ctrl);
        }
        else
        {
            NavMoveTo(ref);
            NavInput();
        }
    }
    else if (action == ImGuiTestAction_Open)
    {
        IM_ASSERT(action_arg == NULL); // Unused
        if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) == 0)
        {
            item->RefCount++;
            MouseMove(ref, flags);

            // Some item may open just by hovering, give them that chance
            if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) == 0)
            {
                MouseClick(0);
                if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) == 0)
                {
                    MouseDoubleClick(0); // Attempt a double-click // FIXME-TESTS: let's not start doing those fuzzy things..
                    if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) == 0)
                        IM_ERRORF_NOHDR("Unable to Open item: %s", ImGuiTestRefDesc(ref, item).c_str());
                }
            }
            item->RefCount--;
            //Yield();
        }
    }
    else if (action == ImGuiTestAction_Close)
    {
        IM_ASSERT(action_arg == NULL); // Unused
        if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) != 0)
        {
            item->RefCount++;
            ItemClick(ref, 0, flags);
            if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) != 0)
            {
                ItemDoubleClick(ref, flags); // Attempt a double-click
                // FIXME-TESTS: let's not start doing those fuzzy things.. widget should give direction of how to close/open... e.g. do you we close a TabItem?
                if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) != 0)
                    IM_ERRORF_NOHDR("Unable to Close item: %s", ImGuiTestRefDesc(ref, item).c_str());
            }
            item->RefCount--;
            Yield();
        }
    }
    else if (action == ImGuiTestAction_Check)
    {
        IM_ASSERT(action_arg == NULL); // Unused
        if ((item->StatusFlags & ImGuiItemStatusFlags_Checkable) && !(item->StatusFlags & ImGuiItemStatusFlags_Checked))
        {
            ItemClick(ref, 0, flags);
        }
        ItemVerifyCheckedIfAlive(ref, true); // We can't just IM_ASSERT(ItemIsChecked()) because the item may disappear and never update its StatusFlags any more!
    }
    else if (action == ImGuiTestAction_Uncheck)
    {
        IM_ASSERT(action_arg == NULL); // Unused
        if ((item->StatusFlags & ImGuiItemStatusFlags_Checkable) && (item->StatusFlags & ImGuiItemStatusFlags_Checked))
        {
            ItemClick(ref, 0, flags);
        }
        ItemVerifyCheckedIfAlive(ref, false); // We can't just IM_ASSERT(ItemIsChecked()) because the item may disappear and never update its StatusFlags any more!
    }

    //if (is_wildcard)
        Engine->FindByLabelTask.InFilterItemStatusFlags = ImGuiItemStatusFlags_None;
}

void    ImGuiTestContext::ItemActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent, const ImGuiTestActionFilter* filter)
{
    int max_depth = filter ? filter->MaxDepth : -1;
    if (max_depth == -1)
        max_depth = 99;
    int max_passes = filter ? filter->MaxPasses : -1;
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
            for (auto& item : items)
                if ((item.StatusFlags & ImGuiItemStatusFlags_Openable) && (item.StatusFlags & ImGuiItemStatusFlags_Opened))
                    highest_depth = ImMax(highest_depth, item.Depth);

        const int actioned_total_at_beginning_of_pass = actioned_total;

        // Process top-to-bottom in most cases
        int scan_start = 0;
        int scan_end = items.GetSize();
        int scan_dir = +1;
        if (action == ImGuiTestAction_Close)
        {
            // Close bottom-to-top because
            // 1) it is more likely to handle same-depth parent/child relationship better (e.g. CollapsingHeader)
            // 2) it gives a nicer sense of symmetry with the corresponding open operation.
            scan_start = items.GetSize() - 1;
            scan_end = -1;
            scan_dir = -1;
        }

        int processed_count_per_depth[8];
        memset(processed_count_per_depth, 0, sizeof(processed_count_per_depth));

        for (int n = scan_start; n != scan_end; n += scan_dir)
        {
            if (IsError())
                break;

            const ImGuiTestItemInfo& item = *items[n];

            if (filter && filter->RequireAllStatusFlags != 0)
                if ((item.StatusFlags & filter->RequireAllStatusFlags) != filter->RequireAllStatusFlags)
                    continue;

            if (filter && filter->RequireAnyStatusFlags != 0)
                if ((item.StatusFlags & filter->RequireAnyStatusFlags) != 0)
                    continue;

            if (filter && filter->MaxItemCountPerDepth != NULL)
            {
                if (item.Depth < IM_ARRAYSIZE(processed_count_per_depth))
                {
                    if (processed_count_per_depth[item.Depth] >= filter->MaxItemCountPerDepth[item.Depth])
                        continue;
                    processed_count_per_depth[item.Depth]++;
                }
            }

            switch (action)
            {
            case ImGuiTestAction_Hover:
            case ImGuiTestAction_Click:
                ItemAction(action, item.ID);
                actioned_total++;
                break;
            case ImGuiTestAction_Check:
                if ((item.StatusFlags & ImGuiItemStatusFlags_Checkable) && !(item.StatusFlags & ImGuiItemStatusFlags_Checked))
                {
                    ItemAction(action, item.ID);
                    actioned_total++;
                }
                break;
            case ImGuiTestAction_Uncheck:
                if ((item.StatusFlags & ImGuiItemStatusFlags_Checkable) && (item.StatusFlags & ImGuiItemStatusFlags_Checked))
                {
                    ItemAction(action, item.ID);
                    actioned_total++;
                }
                break;
            case ImGuiTestAction_Open:
                if ((item.StatusFlags & ImGuiItemStatusFlags_Openable) && !(item.StatusFlags & ImGuiItemStatusFlags_Opened))
                {
                    ItemAction(action, item.ID);
                    actioned_total++;
                }
                break;
            case ImGuiTestAction_Close:
                if (item.Depth == highest_depth && (item.StatusFlags & ImGuiItemStatusFlags_Openable) && (item.StatusFlags & ImGuiItemStatusFlags_Opened))
                {
                    ItemClose(item.ID);
                    actioned_total++;
                }
                break;
            default:
                IM_ASSERT(0);
            }
        }

        if (IsError())
            break;

        if (action == ImGuiTestAction_Hover)
            break;
        if (actioned_total_at_beginning_of_pass == actioned_total)
            break;
    }
    LogDebug("%s %d items in total!", GetActionVerb(action), actioned_total);
}

void    ImGuiTestContext::ItemOpenAll(ImGuiTestRef ref_parent, int max_depth, int max_passes)
{
    ImGuiTestActionFilter filter;
    filter.MaxDepth = max_depth;
    filter.MaxPasses = max_passes;
    ItemActionAll(ImGuiTestAction_Open, ref_parent, &filter);
}

void    ImGuiTestContext::ItemCloseAll(ImGuiTestRef ref_parent, int max_depth, int max_passes)
{
    ImGuiTestActionFilter filter;
    filter.MaxDepth = max_depth;
    filter.MaxPasses = max_passes;
    ItemActionAll(ImGuiTestAction_Close, ref_parent, &filter);
}

void    ImGuiTestContext::ItemInputValue(ImGuiTestRef ref, int value)
{
    char buf[32];
    ImFormatString(buf, IM_ARRAYSIZE(buf), "%d", value);
    ItemInput(ref);
    KeyCharsReplaceEnter(buf);
}
void    ImGuiTestContext::ItemInputValue(ImGuiTestRef ref, float value)
{
    char buf[32];
    ImFormatString(buf, IM_ARRAYSIZE(buf), "%f", value);
    ItemInput(ref);
    KeyCharsReplaceEnter(buf);
}

void    ImGuiTestContext::ItemInputValue(ImGuiTestRef ref, const char* value)
{
    ItemInput(ref);
    KeyCharsReplaceEnter(value);
}

void    ImGuiTestContext::ItemHold(ImGuiTestRef ref, float time)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("ItemHold '%s' %08X", ref.Path ? ref.Path : "NULL", ref.ID);

    MouseMove(ref);

    Yield();
    Inputs->MouseButtonsValue = (1 << 0);
    Sleep(time);
    Inputs->MouseButtonsValue = 0;
    Yield();
}

void    ImGuiTestContext::ItemHoldForFrames(ImGuiTestRef ref, int frames)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("ItemHoldForFrames '%s' %08X", ref.Path ? ref.Path : "NULL", ref.ID);

    MouseMove(ref);
    Yield();
    Inputs->MouseButtonsValue = (1 << 0);
    Yield(frames);
    Inputs->MouseButtonsValue = 0;
    Yield();
}

// Used to test opening containers (TreeNode, Tabs) while dragging a payload
void    ImGuiTestContext::ItemDragOverAndHold(ImGuiTestRef ref_src, ImGuiTestRef ref_dst)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestItemInfo* item_src = ItemInfo(ref_src);
    ImGuiTestItemInfo* item_dst = ItemInfo(ref_dst);
    ImGuiTestRefDesc desc_src(ref_src, item_src);
    ImGuiTestRefDesc desc_dst(ref_dst, item_dst);
    LogDebug("ItemDragOverAndHold %s to %s", desc_src.c_str(), desc_dst.c_str());

    MouseMove(ref_src, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepShort();
    MouseDown(0);

    // Enforce lifting drag threshold even if both item are exactly at the same location.
    MouseLiftDragThreshold();

    MouseMove(ref_dst, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepNoSkip(1.0f, 1.0f / 10.0f);
    MouseUp(0);
}

void    ImGuiTestContext::ItemDragAndDrop(ImGuiTestRef ref_src, ImGuiTestRef ref_dst, ImGuiMouseButton button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestItemInfo* item_src = ItemInfo(ref_src);
    ImGuiTestItemInfo* item_dst = ItemInfo(ref_dst);
    ImGuiTestRefDesc desc_src(ref_src, item_src);
    ImGuiTestRefDesc desc_dst(ref_dst, item_dst);
    LogDebug("ItemDragAndDrop %s to %s", desc_src.c_str(), desc_dst.c_str());

    // Try to keep destination window above other windows. MouseMove() operation will avoid focusing destination window
    // as that may steal ActiveID and break operation.
    // FIXME-TESTS: This does not handle a case where source and destination windows overlap.
    WindowBringToFront(item_dst->Window);

    MouseMove(ref_src, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepShort();
    MouseDown(button);

    // Enforce lifting drag threshold even if both item are exactly at the same location.
    MouseLiftDragThreshold();

    MouseMove(ref_dst, ImGuiTestOpFlags_NoCheckHoveredId | ImGuiTestOpFlags_NoFocusWindow);
    SleepShort();
    MouseUp(button);
}

void    ImGuiTestContext::ItemDragWithDelta(ImGuiTestRef ref_src, ImVec2 pos_delta)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestItemInfo* item_src = ItemInfo(ref_src);
    ImGuiTestRefDesc desc_src(ref_src, item_src);
    LogDebug("ItemDragWithDelta %s to (%f, %f)", desc_src.c_str(), pos_delta.x, pos_delta.y);

    MouseMove(ref_src, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepShort();
    MouseDown(0);

    MouseMoveToPos(UiContext->IO.MousePos + pos_delta);
    SleepShort();
    MouseUp(0);
}

void    ImGuiTestContext::ItemVerifyCheckedIfAlive(ImGuiTestRef ref, bool checked)
{
    Yield();
    ImGuiTestItemInfo* item = ItemInfo(ref, ImGuiTestOpFlags_NoError);
    if (item
     && (item->TimestampMain + 1 >= ImGuiTestEngine_GetFrameCount(Engine))
     && (item->TimestampStatus == item->TimestampMain)
     && (((item->StatusFlags & ImGuiItemStatusFlags_Checked) != 0) != checked))
    {
        IM_CHECK(((item->StatusFlags & ImGuiItemStatusFlags_Checked) != 0) == checked);
    }
}

// FIXME-TESTS: Could this be handled by ItemClose()?
void    ImGuiTestContext::TabClose(ImGuiTestRef ref)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("TabClose '%s' %08X", ref.Path ? ref.Path : "NULL", ref.ID);

    // Move into first, then click close button as it appears
    MouseMove(ref);
    ItemClick(GetID("#CLOSE", ref));
}

bool    ImGuiTestContext::TabBarCompareOrder(ImGuiTabBar* tab_bar, const char** tab_order)
{
    if (IsError())
        return false;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("TabBarCompareOrder");

    // Display
    char buf[256];
    char* buf_end = buf + IM_ARRAYSIZE(buf);

    char* p = buf;
    for (int i = 0; i < tab_bar->Tabs.Size; i++)
        p += ImFormatString(p, buf_end - p, "%s\"%s\"", i ? ", " : " ", tab_bar->GetTabName(&tab_bar->Tabs[i]));
    LogDebug("  Current  {%s }", buf);

    p = buf;
    for (int i = 0; tab_order[i] != NULL; i++)
        p += ImFormatString(p, buf_end - p, "%s\"%s\"", i ? ", " : " ", tab_order[i]);
    LogDebug("  Expected {%s }", buf);

    // Compare
    for (int i = 0; tab_order[i] != NULL; i++)
    {
        if (i >= tab_bar->Tabs.Size)
            return false;
        const char* current = tab_bar->GetTabName(&tab_bar->Tabs[i]);
        const char* expected = tab_order[i];
        if (strcmp(current, expected) != 0)
            return false;
    }
    return true;
}


void    ImGuiTestContext::MenuAction(ImGuiTestAction action, ImGuiTestRef ref)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MenuAction '%s' %08X", ref.Path ? ref.Path : "NULL", ref.ID);

    IM_ASSERT(ref.Path != NULL);

    int depth = 0;
    const char* path = ref.Path;
    const char* path_end = path + strlen(path);
    ImGuiWindow* current_window = NULL;

    if (*path == '/')
    {
        const char* end = strstr(path + 1, "/");
        IM_CHECK_SILENT(end != NULL); // Menu interaction without any menus specified in ref.
        Str30 window_name;
        window_name.append(path, end);
        current_window = GetWindowByRef(GetID(window_name.c_str()));
        path = end + 1;
    }
    else if (RefID)
    {
        current_window = GetWindowByRef(RefID);
    }
    IM_CHECK_SILENT(current_window != NULL);  // A ref window must always be set

    Str128 buf;
    while (path < path_end && !IsError())
    {
        const char* p = ImStrchrRangeWithEscaping(path, path_end, '/');
        if (p == NULL)
            p = path_end;

        const bool is_target_item = (p == path_end);
        if (current_window->Flags & ImGuiWindowFlags_MenuBar)
            buf.setf("/%s/##menubar/%.*s", current_window->Name, (int)(p - path), path);    // Click menu in menu bar
        else
            buf.setf("/%s/%.*s", current_window->Name, (int)(p - path), path);              // Click sub menu in its own window

#if IMGUI_VERSION_NUM < 18520
        if (depth == 0 && (current_window->Flags & ImGuiWindowFlags_Popup))
            depth++;
#endif

        ImGuiTestItemInfo* item = ItemInfo(buf.c_str());
        IM_CHECK_SILENT(item != NULL);
        if (!(item->StatusFlags & ImGuiItemStatusFlags_Opened)) // Open menus can be ignored completely.
        {
            // We cannot move diagonally to a menu item because depending on the angle and other items we cross on our path we could close our target menu.
            // First move horizontally into the menu, then vertically!
            if (depth > 0)
            {
                IM_CHECK_SILENT(item != NULL);
                item->RefCount++;
                if (depth > 1 && (Inputs->MousePosValue.x <= item->RectFull.Min.x || Inputs->MousePosValue.x >= item->RectFull.Max.x))
                    MouseMoveToPos(ImVec2(item->RectFull.GetCenter().x, Inputs->MousePosValue.y));
                if (depth > 0 && (Inputs->MousePosValue.y <= item->RectFull.Min.y || Inputs->MousePosValue.y >= item->RectFull.Max.y))
                    MouseMoveToPos(ImVec2(Inputs->MousePosValue.x, item->RectFull.GetCenter().y));
                item->RefCount--;
            }

            if (is_target_item)
            {
                // Final item
                ItemAction(action, buf.c_str());
                break;
            }
            else
            {
                // Then aim at the menu item. Menus may be navigated by holding mouse button down by hovering a menu.
                ItemAction(Inputs->MouseButtonsValue ? ImGuiTestAction_Hover : ImGuiTestAction_Click, buf.c_str());
            }
        }
        current_window = GetWindowByRef(Str16f("##Menu_%02d", depth).c_str());
        IM_CHECK_SILENT(current_window != NULL);

        path = p + 1;
        depth++;
    }
}

void    ImGuiTestContext::MenuActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent)
{
    ImGuiTestItemList items;
    MenuAction(ImGuiTestAction_Open, ref_parent);
    GatherItems(&items, "/$FOCUSED", 1);
    for (auto item : items)
    {
        MenuAction(ImGuiTestAction_Open, ref_parent); // We assume that every interaction will close the menu again
        ItemAction(action, item.ID);
    }
}

static bool IsWindowACombo(ImGuiWindow* window)
{
    if ((window->Flags & ImGuiWindowFlags_Popup) == 0)
        return false;
    if (strncmp(window->Name, "##Combo_", strlen("##Combo_")) != 0)
        return false;
    return true;
}

void    ImGuiTestContext::ComboClick(ImGuiTestRef ref)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("ComboClick '%s' %08X", ref.Path ? ref.Path : "NULL", ref.ID);

    IM_ASSERT(ref.Path != NULL);

    const char* path = ref.Path;
    const char* path_end = path + strlen(path);

    const char* p = ImStrchrRangeWithEscaping(path, path_end, '/');
    Str128f combo_popup_buf = Str128f("%.*s", (int)(p-path), path);
    ItemClick(combo_popup_buf.c_str());

    ImGuiWindow* popup = GetWindowByRef("/$FOCUSED");
    IM_CHECK_SILENT(popup && IsWindowACombo(popup));

    Str128f combo_item_buf = Str128f("/%s/**/%s", popup->Name, p + 1);
    ItemClick(combo_item_buf.c_str());
}

void    ImGuiTestContext::ComboClickAll(ImGuiTestRef ref_parent)
{
    ItemClick(ref_parent);

    ImGuiWindow* popup = GetWindowByRef("/$FOCUSED");
    IM_CHECK_SILENT(popup && IsWindowACombo(popup));

    ImGuiTestItemList items;
    GatherItems(&items, "/$FOCUSED");
    for (auto item : items)
    {
        ItemClick(ref_parent); // We assume that every interaction will close the combo again
        ItemClick(item.ID);
    }
}

static ImGuiTableColumn* HelperTableFindColumnByName(ImGuiTable* table, const char* name)
{
    for (int i = 0; i < table->Columns.size(); i++)
        if (strcmp(ImGui::TableGetColumnName(table, i), name) == 0)
            return &table->Columns[i];
    return NULL;
}

void ImGuiTestContext::TableOpenContextMenu(ImGuiTestRef ref, int column_n)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("TableOpenContextMenu '%s' %08X", ref.Path ? ref.Path : "NULL", ref.ID);

    ImGuiTable* table = ImGui::TableFindByID(GetID(ref));
    IM_CHECK_SILENT(table != NULL);

    if (column_n == -1)
        column_n = table->RightMostEnabledColumn;
    ItemClick(TableGetHeaderID(table, column_n), ImGuiMouseButton_Right);
    Yield();
}

ImGuiSortDirection ImGuiTestContext::TableClickHeader(ImGuiTestRef ref, const char* label, ImGuiKeyModFlags keys_mod)
{
    ImGuiTable* table = ImGui::TableFindByID(GetID(ref));
    IM_CHECK_SILENT_RETV(table != NULL, ImGuiSortDirection_None);

    ImGuiTableColumn* column = HelperTableFindColumnByName(table, label);
    IM_CHECK_SILENT_RETV(column != NULL, ImGuiSortDirection_None);

    if (keys_mod != ImGuiKeyModFlags_None)
        KeyModDown(keys_mod);

    ItemClick(TableGetHeaderID(table, label), ImGuiMouseButton_Left);

    if (keys_mod != ImGuiKeyModFlags_None)
        KeyModUp(keys_mod);
    return (ImGuiSortDirection_)column->SortDirection;
}

void ImGuiTestContext::TableSetColumnEnabled(ImGuiTestRef ref, const char* label, bool enabled)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("TableSetColumnEnabled '%s' %08X = %d", ref.Path ? ref.Path : "NULL", ref.ID, enabled);

    TableOpenContextMenu(ref);

    ImGuiTestRef backup_ref = GetRef();
    SetRef("/$FOCUSED");
    if (enabled)
        ItemCheck(label);
    else
        ItemUncheck(label);
    PopupCloseOne();
    SetRef(backup_ref);
}

void ImGuiTestContext::TableResizeColumn(ImGuiTestRef ref, int column_n, float width)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("TableResizeColumn '%s' %08X column %d width %.2f", ref.Path ? ref.Path : "NULL", ref.ID, column_n, width);

    ImGuiTable* table = ImGui::TableFindByID(GetID(ref));
    IM_CHECK_SILENT(table != NULL);

    ImGuiID resize_id = ImGui::TableGetColumnResizeID(table, column_n);
    float old_width = table->Columns[column_n].WidthGiven;
    ItemDragWithDelta(resize_id, ImVec2(width - old_width, 0));

    IM_CHECK_EQ(table->Columns[column_n].WidthRequest, width);
}

const ImGuiTableSortSpecs* ImGuiTestContext::TableGetSortSpecs(ImGuiTestRef ref)
{
    ImGuiTable* table = ImGui::TableFindByID(GetID(ref));
    IM_CHECK_SILENT_RETV(table != NULL, NULL);

    ImGuiContext& g = *UiContext;
    ImSwap(table, g.CurrentTable);
    const ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
    ImSwap(table, g.CurrentTable);
    return sort_specs;
}

void    ImGuiTestContext::WindowClose(ImGuiTestRef ref)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("WindowClose");
    ItemClick(GetID("#CLOSE", ref));
}

void    ImGuiTestContext::WindowCollapse(ImGuiWindow* window, bool collapsed)
{
    if (IsError())
        return;
    if (window == NULL)
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    if (window->Collapsed != collapsed)
    {
        LogDebug("WindowCollapse %d", collapsed);
        ImGuiTestOpFlags backup_op_flags = OpFlags;
        OpFlags |= ImGuiTestOpFlags_NoAutoUncollapse;
        ItemClick(GetID("#COLLAPSE", window->ID));
        OpFlags = backup_op_flags;
        Yield();
        IM_CHECK(window->Collapsed == collapsed);
    }
}

// FIXME-TESTS: Ideally we would aim toward a clickable spot in the window.
void    ImGuiTestContext::WindowFocus(ImGuiTestRef ref)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestRefDesc desc(ref, NULL);
    LogDebug("FocusWindow('%s')", desc.c_str());

    ImGuiID window_id = GetID(ref);
    ImGuiWindow* window = ImGui::FindWindowByID(window_id);
    IM_CHECK_SILENT(window != NULL);
    if (window)
    {
        ImGui::FocusWindow(window); // FIXME-TESTS-NOT_SAME_AS_END_USER: Click or tab?
        Yield();
    }
}

// Supported values for ImGuiTestOpFlags:
// - ImGuiTestOpFlags_NoFocusWindow
void    ImGuiTestContext::WindowMove(ImGuiTestRef ref, ImVec2 input_pos, ImVec2 pivot, ImGuiTestOpFlags flags)
{
    if (IsError())
        return;

    ImGuiWindow* window = GetWindowByRef(ref);
    IM_CHECK_SILENT(window != NULL);

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("WindowMove %s (%.1f,%.1f) ", window->Name, input_pos.x, input_pos.y);
    ImVec2 target_pos = ImFloor(input_pos - pivot * window->Size);
    if (ImLengthSqr(target_pos - window->Pos) < 0.001f)
    {
        //MouseMoveToPos(window->Pos); //??
        return;
    }

    if ((flags & ImGuiTestOpFlags_NoFocusWindow) == 0)
        WindowBringToFront(window);
    WindowCollapse(window, false);

    MouseMoveToPos(GetWindowTitlebarPoint(ref));
    //IM_CHECK_SILENT(UiContext->HoveredWindow == window);
    MouseDown(0);

    // Disable docking
#ifdef IMGUI_HAS_DOCK
    if (UiContext->IO.ConfigDockingWithShift)
        KeyModUp(ImGuiKeyModFlags_Shift);
    else
        KeyModDown(ImGuiKeyModFlags_Shift);
#endif

    ImVec2 delta = target_pos - window->Pos;
    MouseMoveToPos(Inputs->MousePosValue + delta);
    Yield();

    MouseUp();
#ifdef IMGUI_HAS_DOCK
    KeyModUp(ImGuiKeyModFlags_Shift);
#endif
}

void    ImGuiTestContext::WindowResize(ImGuiTestRef ref, ImVec2 size)
{
    if (IsError())
        return;

    ImGuiWindow* window = GetWindowByRef(ref);
    IM_CHECK_SILENT(window != NULL);
    size = ImFloor(size);

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("WindowResize %s (%.1f,%.1f)", window->Name, size.x, size.y);
    if (ImLengthSqr(size - window->Size) < 0.001f)
        return;

    WindowBringToFront(window);
    WindowCollapse(window, false);

#if IMGUI_VERSION_NUM < 18203
    ImGuiID id = ImGui::GetWindowResizeID(window, 0);
#else
    ImGuiID id = ImGui::GetWindowResizeCornerID(window, 0);
#endif
    MouseMove(id);
    MouseDown(0);

    ImVec2 delta = size - window->Size;
    MouseMoveToPos(Inputs->MousePosValue + delta);
    Yield(); // At this point we don't guarantee the final size!

    MouseUp();
}

void    ImGuiTestContext::PopupCloseOne()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("PopupCloseOne");
    ImGuiContext& g = *UiContext;
    if (g.OpenPopupStack.Size > 0)
        ImGui::ClosePopupToLevel(g.OpenPopupStack.Size - 1, true);    // FIXME-TESTS-NOT_SAME_AS_END_USER
    Yield();
}

void    ImGuiTestContext::PopupCloseAll()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("PopupCloseAll");
    ImGuiContext& g = *UiContext;
    if (g.OpenPopupStack.Size > 0)
        ImGui::ClosePopupToLevel(0, true);    // FIXME-TESTS-NOT_SAME_AS_END_USER
    Yield();
}

#ifdef IMGUI_HAS_DOCK
// Note: unlike DockBuilder functions, for _nodes_ this require the node to be visible.
// Supported values for ImGuiTestOpFlags:
// - ImGuiTestOpFlags_NoFocusWindow
// FIXME-TESTS: USING ImGuiTestOpFlags_NoFocusWindow leads to increase of ForeignWindowsHideOverPos(), best to avoid
void    ImGuiTestContext::DockInto(ImGuiTestRef src_id, ImGuiTestRef dst_id, ImGuiDir split_dir, bool split_outer, ImGuiTestOpFlags flags)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);

    ImGuiWindow* window_src = GetWindowByRef(src_id);
    ImGuiWindow* window_dst = GetWindowByRef(dst_id);
    ImGuiDockNode* node_src = ImGui::DockBuilderGetNode(src_id.ID);
    ImGuiDockNode* node_dst = ImGui::DockBuilderGetNode(dst_id.ID);
    IM_CHECK_SILENT((window_src != NULL) != (node_src != NULL)); // Src must be either a window either a node
    IM_CHECK_SILENT((window_dst != NULL) != (node_dst != NULL)); // Dst must be either a window either a node

    if (node_src)
        window_src = node_src->HostWindow;
    if (node_dst)
        window_dst = node_dst->HostWindow;

    Str128f log("DockWindowInto() Src: %s '%s' (0x%08X), Dst: %s '%s' (0x%08X)",
        node_src ? "node" : "window", node_src ? "" : window_src->Name, node_src ? node_src->ID : window_src->ID,
        node_dst ? "node" : "window", node_dst ? "" : window_dst->Name, node_dst ? node_dst->ID : window_dst->ID);
    LogDebug("%s", log.c_str());

    IM_CHECK_SILENT(window_src != NULL);
    IM_CHECK_SILENT(window_dst != NULL);
    IM_CHECK_SILENT(window_src->WasActive);
    IM_CHECK_SILENT(window_dst->WasActive);

    // Avoid focusing if we don't need it (this facilitate avoiding focus flashing when recording animated gifs)
    if (!(flags & ImGuiTestOpFlags_NoFocusWindow))
    {
        if (g.Windows[g.Windows.Size - 2] != window_dst)
            WindowFocus(window_dst->ID);
        if (g.Windows[g.Windows.Size - 1] != window_src)
            WindowFocus(window_src->ID);
    }

    // Aim at title bar or tab or node grab
    ImGuiTestRef ref_src;
    if (node_src)
        ref_src = ImGui::DockNodeGetWindowMenuButtonId(node_src); // Whole node grab
    else
        ref_src = (window_src->DockIsActive ? window_src->ID : window_src->MoveId); // FIXME-TESTS FIXME-DOCKING: Identify tab
    MouseMove(ref_src, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepShort();

    // Locate target
    ImVec2 drop_pos;
    bool drop_is_valid = ImGui::DockContextCalcDropPosForDocking(window_dst, node_dst, window_src, split_dir, split_outer, &drop_pos);
    IM_CHECK_SILENT(drop_is_valid);
    if (!drop_is_valid)
        return;

    // Ensure we can reach target
    WindowTeleportToMakePosVisibleInViewport(window_dst, drop_pos);
    ImGuiWindow* friend_windows[] = { window_src, window_dst, NULL };
    ForeignWindowsHideOverPos(drop_pos, friend_windows);

    // Drag
    drop_is_valid = ImGui::DockContextCalcDropPosForDocking(window_dst, node_dst, window_src, split_dir, split_outer, &drop_pos);
    IM_CHECK_SILENT(drop_is_valid);
    MouseDown(0);
    if (g.IO.ConfigDockingWithShift)
        KeyModDown(ImGuiKeyModFlags_Shift);
    MouseLiftDragThreshold();
    MouseMoveToPos(drop_pos);
    IM_CHECK_SILENT(g.MovingWindow == window_src);
#ifdef IMGUI_HAS_DOCK
    Yield();    // Docking to dockspace over viewport fails in fast mode without this.
    IM_CHECK_SILENT(g.HoveredWindowUnderMovingWindow && g.HoveredWindowUnderMovingWindow->RootWindowDockTree == window_dst->RootWindowDockTree);
#else
    IM_CHECK_SILENT(g.HoveredWindowUnderMovingWindow && g.HoveredWindowUnderMovingWindow->RootWindow == window_dst);
#endif

    MouseUp(0);
    if (g.IO.ConfigDockingWithShift)
        KeyModUp(ImGuiKeyModFlags_Shift);
    ForeignWindowsUnhideAll();
    Yield();
    Yield();
}

void    ImGuiTestContext::DockClear(const char* window_name, ...)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("DockClear");

    va_list args;
    va_start(args, window_name);
    while (window_name != NULL)
    {
        ImGui::DockBuilderDockWindow(window_name, 0);
        window_name = va_arg(args, const char*);
    }
    va_end(args);

    if (ActiveFunc == ImGuiTestActiveFunc_TestFunc)
        Yield(2); // Give time to rebuild dock in case io.ConfigDockingAlwaysTabBar is set
}

bool    ImGuiTestContext::WindowIsUndockedOrStandalone(ImGuiWindow* window)
{
    if (window->DockNode == NULL)
        return true;
    return DockIdIsUndockedOrStandalone(window->DockId);
}

bool    ImGuiTestContext::DockIdIsUndockedOrStandalone(ImGuiID dock_id)
{
    if (dock_id == 0)
        return true;
    if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_id))
        if (node->IsFloatingNode() && node->IsLeafNode() && node->Windows.Size == 1)
            return true;
    return false;
}

void    ImGuiTestContext::DockNodeHideTabBar(ImGuiDockNode* node, bool hidden)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("DockNodeHideTabBar %d", hidden);

    // FIXME-TEST: Alter WindowRef
    ImGuiTestRef backup_ref = GetRef();
    if (hidden)
    {
        SetRef(node->HostWindow);
        ItemClick(ImGui::DockNodeGetWindowMenuButtonId(node));
        ItemClick(Str64f("/##Popup_%08x/Hide tab bar", GetID("#WindowMenu", node->ID)).c_str());
        IM_CHECK_SILENT(node->IsHiddenTabBar());

    }
    else
    {
        IM_CHECK_SILENT(node->VisibleWindow != NULL);
        SetRef(node->VisibleWindow);
        ItemClick("#UNHIDE", 0, ImGuiTestOpFlags_MoveToEdgeD | ImGuiTestOpFlags_MoveToEdgeR);
        IM_CHECK_SILENT(!node->IsHiddenTabBar());
    }
    SetRef(backup_ref);
}

void    ImGuiTestContext::UndockNode(ImGuiID dock_id)
{
    ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_id);
    if (node == NULL)
        return;
    if (node->IsFloatingNode())
        return;
    if (node->Windows.empty())
        return;

    const float h = node->Windows[0]->TitleBarHeight();
    if (!UiContext->IO.ConfigDockingWithShift)
        KeyModDown(ImGuiKeyModFlags_Shift); // Disable docking
    ItemDragWithDelta(ImGui::DockNodeGetWindowMenuButtonId(node), ImVec2(h, h) * -2);
    if (!UiContext->IO.ConfigDockingWithShift)
        KeyModUp(ImGuiKeyModFlags_Shift);
    MouseUp();
}

void    ImGuiTestContext::UndockWindow(const char* window_name)
{
    IM_ASSERT(window_name != NULL);

    ImGuiWindow* window = GetWindowByRef(window_name);
    if (!window->DockIsActive)
        return;

    const float h = window->TitleBarHeight();
    if (!UiContext->IO.ConfigDockingWithShift)
        KeyModDown(ImGuiKeyModFlags_Shift);
    ItemDragWithDelta(window_name, ImVec2(h, h) * -2);
    if (!UiContext->IO.ConfigDockingWithShift)
        KeyModUp(ImGuiKeyModFlags_Shift);
    Yield();
}

#endif // #ifdef IMGUI_HAS_DOCK

//-------------------------------------------------------------------------
// ImGuiTestContext - Performance Tools
//-------------------------------------------------------------------------

// Calculate the reference DeltaTime, averaged over 500 frames, with GuiFunc disabled.
void    ImGuiTestContext::PerfCalcRef()
{
    LogDebug("Measuring ref dt...");
    SetGuiFuncEnabled(false);
    for (int n = 0; n < 500 && !Abort; n++)
        Yield();
    PerfRefDt = ImGuiTestEngine_GetPerfDeltaTime500Average(Engine);
    SetGuiFuncEnabled(true);
}

void    ImGuiTestContext::PerfCapture(const char* category, const char* test_name, const char* csv_file)
{
    // Calculate reference average DeltaTime if it wasn't explicitly called by TestFunc
    if (PerfRefDt < 0.0)
        PerfCalcRef();
    IM_ASSERT(PerfRefDt >= 0.0);

    // Yield for the average to stabilize
    LogDebug("Measuring gui dt...");
    for (int n = 0; n < 500 && !Abort; n++)
        Yield();
    if (Abort)
        return;

    double dt_curr = ImGuiTestEngine_GetPerfDeltaTime500Average(Engine);
    double dt_ref_ms = PerfRefDt * 1000;
    double dt_delta_ms = (dt_curr - PerfRefDt) * 1000;

    const ImBuildInfo& build_info = ImBuildGetCompilationInfo();

    // Display results
    // FIXME-TESTS: Would be nice if we could submit a custom marker (e.g. branch/feature name)
    LogInfo("[PERF] Conditions: Stress x%d, %s, %s, %s, %s, %s",
        PerfStressAmount, build_info.Type, build_info.Cpu, build_info.OS, build_info.Compiler, build_info.Date);
    LogInfo("[PERF] Result: %+6.3f ms (from ref %+6.3f)", dt_delta_ms, dt_ref_ms);

    ImGuiPerfToolEntry entry;
    entry.Timestamp = BatchStartTime;
    entry.Category = category ? category : Test->Category;
    entry.TestName = test_name ? test_name : Test->Name;
    entry.DtDeltaMs = dt_delta_ms;
    entry.PerfStressAmount = PerfStressAmount;
    entry.GitBranchName = EngineIO->GitBranchName;
    entry.BuildType = build_info.Type;
    entry.Cpu = build_info.Cpu;
    entry.OS = build_info.OS;
    entry.Compiler = build_info.Compiler;
    entry.Date = build_info.Date;
    ImGuiTestEngine_PerfToolAppendToCSV(Engine->PerfTool, &entry, csv_file);

    // Disable the "Success" message
    RunFlags |= ImGuiTestRunFlags_NoSuccessMsg;
}

//-------------------------------------------------------------------------
