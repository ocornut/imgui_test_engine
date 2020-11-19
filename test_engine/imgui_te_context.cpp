// dear imgui
// (test engine, test context = end user automation API)

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _WIN32
#include <unistd.h>     // usleep
#endif

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "imgui_te_internal.h"
#include "imgui_te_util.h"
#include "shared/imgui_utils.h"
#include "libs/Str/Str.h"

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
            for (int i = 0; i < log->LineInfo.Size; i++)
            {
                ImGuiTestLogLineInfo& line_info = log->LineInfo[i];
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
};

void    ImGuiTestContext::RecoverFromUiContextErrors()
{
    IM_ASSERT(Test != NULL);

    // If we are _already_ in a test error state, recovering is normal so we'll hide the log.
    const bool verbose = (Test->Status != ImGuiTestStatus_Error) || (EngineIO->ConfigVerboseLevel >= ImGuiTestVerboseLevel_Debug);
    if (verbose)
        ImGui::ErrorCheckEndFrameRecover(LogWarningFunc, this);
    else
        ImGui::ErrorCheckEndFrameRecover(NULL, NULL);
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
        RefID = ImHashDecoratedPath(ref.Path, NULL, 0);
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
    return ImHashDecoratedPath(ref.Path, NULL, RefID);
}

ImGuiID ImGuiTestContext::GetID(ImGuiTestRef ref, ImGuiTestRef seed_ref)
{
    if (ref.ID)
        return ref.ID; // FIXME: What if seed_ref != 0
    return ImHashDecoratedPath(ref.Path, NULL, GetID(seed_ref));
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

ImGuiTestRef ImGuiTestContext::GetFocusWindowRef()
{
    ImGuiContext& g = *UiContext;
    return g.NavWindow ? g.NavWindow->ID : 0;
}

ImVec2 ImGuiTestContext::GetMainViewportPos()
{
#ifdef IMGUI_HAS_VIEWPORT
    return ImGui::GetMainViewport()->Pos;
#else
    return ImVec2(0, 0);
#endif
}

ImVec2 ImGuiTestContext::GetMainViewportSize()
{
#ifdef IMGUI_HAS_VIEWPORT
    return ImGui::GetMainViewport()->Size;
#else
    return ImGui::GetIO().DisplaySize;
#endif
}

void ImGuiTestContext::CaptureInitArgs(ImGuiCaptureArgs* args, int flags)
{
    args->InFlags = (ImGuiCaptureFlags)flags;
    args->InPadding = 13.0f;
    ImFormatString(args->InOutputFileTemplate, IM_ARRAYSIZE(args->InOutputFileTemplate), "captures/%s_%04d.png", Test->Name, CaptureCounter);
    CaptureCounter++;
}

bool ImGuiTestContext::CaptureAddWindow(ImGuiCaptureArgs* args, ImGuiTestRef ref)
{
    ImGuiWindow* window = GetWindowByRef(ref);
    if (window == NULL)
        IM_CHECK_RETV(window != NULL, false);
    args->InCaptureWindows.push_back(window);
    return window != NULL;
}

bool ImGuiTestContext::CaptureScreenshot(ImGuiCaptureArgs* args)
{
    if (IsError())
        return false;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogInfo("CaptureScreenshot()");
    bool ret = ImGuiTestEngine_CaptureScreenshot(Engine, args);
    LogDebug("Saved '%s' (%d*%d pixels)", args->OutSavedFileName, (int)args->OutImageSize.x, (int)args->OutImageSize.y);
    return ret;
}

bool ImGuiTestContext::BeginCaptureGif(ImGuiCaptureArgs* args)
{
    if (IsError())
        return false;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogInfo("BeginCaptureGif()");
    return ImGuiTestEngine_BeginCaptureAnimation(Engine, args);
}

bool ImGuiTestContext::EndCaptureGif(ImGuiCaptureArgs* args)
{
    bool ret = Engine->CaptureContext.IsCapturingGif() && ImGuiTestEngine_EndCaptureAnimation(Engine, args);
    if (ret)
    {
        // In-progress capture was canceled by user. Delete incomplete file.
        if (IsError())
        {
            //ImFileDelete(args->OutSavedFileName);
            return false;
        }

        LogDebug("Saved '%s' (%d*%d pixels)", args->OutSavedFileName, (int)args->OutImageSize.x, (int)args->OutImageSize.y);
    }
    return ret;
}

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
            task->InBaseId = ImHashDecoratedPath(wildcard_prefix_start, wildcard_prefix_end, RefID);
        else
            task->InBaseId = RefID;
        task->InLabel = wildcard_suffix_start;
        task->OutItemId = 0;
        LogDebug("Wildcard matching..");

        int retries = 0;
        while (retries < 2 && task->OutItemId == 0)
        {
            ImGuiTestEngine_Yield(Engine);
            retries++;
        }
        full_id = task->OutItemId;

        // FIXME: InFilterItemStatusFlags is not clear here intentionally, because it is set in ItemAction() and reused in later calls to ItemInfo() to resolve ambiguities.
        task->InBaseId = 0;
        task->InLabel = NULL;
        task->OutItemId = 0;
    }
    else
    {
        // Normal matching
        if (ref.ID)
            full_id = ref.ID;
        else
            full_id = ImHashDecoratedPath(ref.Path, NULL, RefID);
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
    if (window)
        ScrollToY(0.0f);
    else
        LogError("ScrollToTop: failed to get window");
    Yield();
}

void    ImGuiTestContext::ScrollToBottom()
{
    if (IsError())
        return;
    ImGuiWindow* window = GetWindowByRef("");
    if (window)
        ScrollToY(window->ScrollMax.y);
    else
        LogError("ScrollToBottom: failed to get window");
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
        LogWarning("Failed to set Scroll%c. Requested %.2f, got %.2f. Will try again.", 'X' + axis, expected, actual);
        return true;
    }
    else
    {
        IM_ERRORF("Failed to set Scroll%c. Requested %.2f, got %.2f. Aborting.", 'X' + axis, expected, actual);
        return false;
    }
}

void    ImGuiTestContext::ScrollTo(ImGuiAxis axis, float scroll_target)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiWindow* window = GetWindowByRef("");
    const char axis_c = (char)('X' + axis);
    if (!window)
    {
        LogError("ScrollTo%c: failed to get window", axis_c);
        return;
    }

    ImGuiContext& g = *UiContext;
    LogDebug("ScrollTo%c %.1f/%.1f", axis_c, scroll_target, window->ScrollMax[axis]);
    WindowBringToFront(window);

    int remaining_failures = 3;
    while (!Abort)
    {
        if (ImFabs(window->Scroll[axis] - scroll_target) < 1.0f)
            break;

        float scroll_speed = EngineIO->ConfigRunFast ? FLT_MAX : ImFloor(EngineIO->ScrollSpeed * g.IO.DeltaTime + 0.99f);
        float scroll_next = ImLinearSweep(window->Scroll[axis], scroll_target, scroll_speed);
        if (axis == ImGuiAxis_X)
            ImGui::SetScrollX(window, scroll_next);
        else
            ImGui::SetScrollY(window, scroll_next);
        Yield();

        // Error handling to avoid getting stuck in this function.
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
    ImGuiContext& g = *UiContext;
    ImGuiTestItemInfo* item = ItemInfo(ref);
    ImGuiTestRefDesc desc(ref, item);
    LogDebug("ScrollToItemY %s", desc.c_str());

    if (item == NULL)
        return;
    ImGuiWindow* window = item->Window;

    int remaining_failures = 2;
    while (!Abort)
    {
        // result->Rect fields will be updated after each iteration.
        float item_curr_y = ImFloor(item->RectFull.GetCenter().y);
        float item_target_y = ImFloor(window->InnerClipRect.GetCenter().y);
        float scroll_delta_y = item_target_y - item_curr_y;
        float scroll_target_y = ImClamp(window->Scroll.y - scroll_delta_y, 0.0f, window->ScrollMax.y);
        if (ImFabs(window->Scroll.y - scroll_target_y) < 1.0f)
            break;

        // FIXME-TESTS: Scroll snap on edge can make this function loops forever.
        // [20191014: Repro is to resize e.g. widgets_checkbox_001 window to be small vertically]

        // FIXME-TESTS: There's a bug which can be repro by moving #RESIZE grips to Layer 0, making window small and trying to resize a dock node host.
        // Somehow SizeContents.y keeps increase and we never reach our desired (but faulty) scroll target.
        float scroll_speed = EngineIO->ConfigRunFast ? FLT_MAX : ImFloor(EngineIO->ScrollSpeed * g.IO.DeltaTime + 0.99f);
        float scroll_next_y = ImLinearSweep(window->Scroll.y, scroll_target_y, scroll_speed);
        //printf("[%03d] window->Scroll.y %f + %f\n", FrameCount, window->Scroll.y, scroll_speed);
        //window->Scroll.y = scroll_next_y;
        ImGui::SetScrollY(window, scroll_next_y);
        Yield();

        // Error handling to avoid getting stuck in this function.
        if (!ScrollErrorCheck(ImGuiAxis_Y, scroll_next_y, window->Scroll.y, &remaining_failures))
            break;

        WindowBringToFront(window);
    }

    // Need another frame for the result->Rect to stabilize
    Yield();
}

// Verify that ScrollMax is stable regardless of scrolling position
// - This can break when the layout of clipped items doesn't match layout of unclipped items
// - This can break with non-rounded calls to ItemSize(), namely when the starting position is negative (above visible area)
//   We should ideally be more tolerant of non-rounded sizes passed by the users.
// - One of the net visible effect of an unstable ScrollMax is that the End key would put you at a spot that's not exactly the lowest spot,
//   and so a second press to End would you move again by a few pixels.
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
    IM_ASSERT(g.NavMoveRequest == false);
    ImRect rect_rel = item->RectFull;
    rect_rel.Translate(ImVec2(-item->Window->Pos.x, -item->Window->Pos.y));
    ImGui::SetNavIDWithRectRel(item->ID, item->NavLayer, 0, rect_rel);
    ImGui::ScrollToBringRectIntoView(item->Window, item->RectFull);
    while (g.NavMoveRequest)
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
    IM_ASSERT(input >= 0 && input < ImGuiNavInput_COUNT);
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("NavInput %d", (int)input);

    ImGuiTestEngine_PushInput(Engine, ImGuiTestInput::FromNav(input, ImGuiKeyState_Down));
    Yield();
    ImGuiTestEngine_PushInput(Engine, ImGuiTestInput::FromNav(input, ImGuiKeyState_Up));
    Yield();
    Yield(); // For nav code to react e.g. run a query
}

void    ImGuiTestContext::NavActivate()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("NavActivate");

    Yield();

    int frames_to_hold = 2; // FIXME-TESTS: <-- number of frames could be fuzzed here
#if 1
    // Feed gamepad nav inputs
    for (int n = 0; n < frames_to_hold; n++)
    {
        ImGuiTestEngine_PushInput(Engine, ImGuiTestInput::FromNav(ImGuiNavInput_Activate, ImGuiKeyState_Down));
        Yield();
    }
    Yield();
#else
    // Feed keyboard keys
    ImGuiTestEngine_PushInput(Engine, ImGuiTestInput::FromKey(ImGuiKey_Space, ImGuiKeyState_Down));
    for (int n = 0; n < frames_to_hold; n++)
        Yield();
    ImGuiTestEngine_PushInput(Engine, ImGuiTestInput::FromKey(ImGuiKey_Space, ImGuiKeyState_Up));
    Yield();
    Yield();
#endif
}

void    ImGuiTestContext::NavInput()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("NavInput");

    int frames_to_hold = 2; // FIXME-TESTS: <-- number of frames could be fuzzed here
    for (int n = 0; n < frames_to_hold; n++)
    {
        ImGuiTestEngine_PushInput(Engine, ImGuiTestInput::FromNav(ImGuiNavInput_Input, ImGuiKeyState_Down));
        Yield();
    }
    Yield();
}

static ImVec2 GetMouseAimingPos(ImGuiTestItemInfo* item, ImGuiTestOpFlags flags)
{
    ImRect r = item->RectClipped;
    ImVec2 pos;
    //pos = r.GetCenter();
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
    item->RefCount++;

    // Focus window before scrolling/moving so things are nicely visible
    if (!(flags & ImGuiTestOpFlags_NoFocusWindow))
        WindowBringToFront(item->Window);

    ImGuiWindow* window = item->Window;
    ImRect window_inner_r_padded = window->InnerClipRect;
    window_inner_r_padded.Expand(-4.0f); // == WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS
    if (item->NavLayer == ImGuiNavLayer_Main && !window_inner_r_padded.Contains(item->RectClipped))
        ScrollToItemY(ref);

    ImVec2 pos = item->RectFull.GetCenter();
    WindowTeleportToMakePosVisibleInViewport(window, pos);

    // Move toward an actually visible point
    pos = GetMouseAimingPos(item, flags);
    MouseMoveToPos(pos);

    // Focus again in case something made us lost focus (which could happen on a simple hover)
    if (!(flags & ImGuiTestOpFlags_NoFocusWindow))
        WindowBringToFront(window);// , ImGuiTestOpFlags_Verbose);

    if (!Abort && !(flags & ImGuiTestOpFlags_NoCheckHoveredId))
    {
        const ImGuiID hovered_id = g.HoveredIdPreviousFrame;
        if (hovered_id != item->ID)
        {
            if (!(window->Flags & ImGuiWindowFlags_NoResize) && !(flags & ImGuiTestOpFlags_IsSecondAttempt))
            {
                bool is_resize_corner = false;
                for (int n = 0; n < 2; n++)
                    is_resize_corner |= (hovered_id == ImGui::GetWindowResizeID(window, n));
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

    ImRect visible_r;
    visible_r.Min = GetMainViewportPos();
    visible_r.Max = visible_r.Min + GetMainViewportSize();
    if (!visible_r.Contains(pos))
    {
        // Fallback move window directly to make our item reachable with the mouse.
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

void	ImGuiTestContext::MouseMoveToPos(ImVec2 target)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MouseMoveToPos from (%.0f,%.0f) to (%.0f,%.0f)", Inputs->MousePosValue.x, Inputs->MousePosValue.y, target.x, target.y);

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
            Inputs->MousePosValue = ImBezierCalc(start_pos, intermediate_pos_a, intermediate_pos_b, target, t);
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

void    ImGuiTestContext::MouseDown(int button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MouseDown %d", button);

    UiContext->IO.MouseClickedTime[button] = -FLT_MAX; // Prevent accidental double-click from happening ever
    Inputs->MouseButtonsValue = (1 << button);
    Yield();
}

void    ImGuiTestContext::MouseUp(int button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MouseUp %d", button);

    Inputs->MouseButtonsValue &= ~(1 << button);
    Yield();
}

// TODO: click time argument (seconds and/or frames)
void    ImGuiTestContext::MouseClick(int button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MouseClick %d", button);

    // Make sure mouse buttons are released
    IM_ASSERT(Inputs->MouseButtonsValue == 0);

    Yield();

    // Press
    UiContext->IO.MouseClickedTime[button] = -FLT_MAX; // Prevent accidental double-click from happening ever
    Inputs->MouseButtonsValue = (1 << button);
    Yield();
    Inputs->MouseButtonsValue = 0;

    Yield(); // Let the imgui frame finish, start a new frame.
    // Now NewFrame() has seen the mouse release.
    Yield(); // Let the imgui frame finish, now e.g. Button() function will return true. Start a new frame.
    // At this point, we are in a new frame but our windows haven't been Begin()-ed into, so anything processed by Begin() is not valid yet.
}

// TODO: click time argument (seconds and/or frames)
void    ImGuiTestContext::MouseDoubleClick(int button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MouseDoubleClick %d", button);

    Yield();
    UiContext->IO.MouseClickedTime[button] = -FLT_MAX; // Prevent accidental double-click followed by single click
    for (int n = 0; n < 2; n++)
    {
        Inputs->MouseButtonsValue = (1 << button);
        Yield();
        Inputs->MouseButtonsValue = 0;
        Yield();
    }
    Yield(); // Give a frame for items to react
}

void    ImGuiTestContext::MouseLiftDragThreshold(int button)
{
    if (IsError())
        return;

    ImGuiContext& g = *UiContext;
    g.IO.MouseDragMaxDistanceAbs[button] = ImVec2(g.IO.MouseDragThreshold, g.IO.MouseDragThreshold);
    g.IO.MouseDragMaxDistanceSqr[button] = (g.IO.MouseDragThreshold * g.IO.MouseDragThreshold) + (g.IO.MouseDragThreshold * g.IO.MouseDragThreshold);
}

void    ImGuiTestContext::MouseClickOnVoid(int mouse_button)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("MouseClickOnVoid %d", mouse_button);

    // FIXME-TESTS: Would be nice if we could find a suitable position (e.g. by sampling points in a grid)
    ImVec2 void_pos = GetMainViewportPos() + ImVec2(1, 1);
    ImVec2 window_min_pos = void_pos + g.Style.TouchExtraPadding + ImVec2(4.0f, 4.0f) + ImVec2(1.0f, 1.0f); // FIXME: Should use WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS

    for (ImGuiWindow* window : g.Windows)
        if (window->RootWindow == window && window->WasActive)
            if (window->Rect().Contains(window_min_pos))
                WindowMove(window->Name, window_min_pos);

    // Click top-left corner which now is empty space.
    MouseMoveToPos(void_pos);
    IM_CHECK(g.HoveredWindow == NULL);

    // Clicking empty space should clear navigation focus.
    MouseClick(mouse_button);
    //IM_CHECK(g.NavId == 0); // FIXME: Clarify specs
    IM_CHECK(g.NavWindow == NULL);
}

void    ImGuiTestContext::MouseDragWithDelta(ImVec2 delta, int button)
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
    KeyPressMap(ImGuiKey_A, ImGuiKeyModFlags_Ctrl);
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
    KeyPressMap(ImGuiKey_A, ImGuiKeyModFlags_Ctrl);
    if (chars[0])
        KeyChars(chars);
    else
        KeyPressMap(ImGuiKey_Delete);
    KeyPressMap(ImGuiKey_Enter);
}

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
        ImGui::FocusWindow(window);
        Yield();
        Yield();
        //IM_CHECK(g.NavWindow == window);
    }
    else if (window->RootWindow != g.Windows.back()->RootWindow)
    {
        IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
        LogDebug("BringWindowToDisplayFront('%s') (window.back=%s)", window->Name, g.Windows.back()->Name);
        ImGui::BringWindowToDisplayFront(window);
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
            KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Ctrl);
            MouseClick(0);
            KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Ctrl);
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
        }
    }
    else if (action == ImGuiTestAction_Check)
    {
        IM_ASSERT(action_arg == NULL); // Unused
        if ((item->StatusFlags & ImGuiItemStatusFlags_Checkable) && !(item->StatusFlags & ImGuiItemStatusFlags_Checked))
        {
            ItemClick(ref, 0, flags);
            //Yield();
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
    int max_depth = filter->MaxDepth;
    if (max_depth == -1)
        max_depth = 99;
    int max_passes = filter->MaxPasses;
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

            if (filter->RequireAllStatusFlags != 0)
                if ((item.StatusFlags & filter->RequireAllStatusFlags) != filter->RequireAllStatusFlags)
                    continue;

            if (filter->RequireAnyStatusFlags != 0)
                if ((item.StatusFlags & filter->RequireAnyStatusFlags) != 0)
                    continue;

            if (filter->MaxItemCountPerDepth != NULL)
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
                ItemAction(action, item.ID);
                actioned_total++;
                break;
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

void    ImGuiTestContext::ItemDragAndDrop(ImGuiTestRef ref_src, ImGuiTestRef ref_dst)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestItemInfo* item_src = ItemInfo(ref_src);
    ImGuiTestItemInfo* item_dst = ItemInfo(ref_dst);
    ImGuiTestRefDesc desc_src(ref_src, item_src);
    ImGuiTestRefDesc desc_dst(ref_dst, item_dst);
    LogDebug("ItemDragAndDrop %s to %s", desc_src.c_str(), desc_dst.c_str());

    MouseMove(ref_src, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepShort();
    MouseDown(0);

    // Enforce lifting drag threshold even if both item are exactly at the same location.
    MouseLiftDragThreshold();

    MouseMove(ref_dst, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepShort();
    MouseUp(0);
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
    Str128 buf;
    while (path < path_end)
    {
        const char* p = ImStrchrRangeWithEscaping(path, path_end, '/');
        if (p == NULL)
            p = path_end;
        const bool is_target_item = (p == path_end);
        if (depth == 0)
        {
            // Click menu in menu bar
            IM_ASSERT(RefStr[0] != 0); // Unsupported: window needs to be in Ref
            buf.setf("##menubar/%.*s", (int)(p - path), path);
        }
        else
        {
            // Click sub menu in its own window
            buf.setf("/##Menu_%02d/%.*s", depth - 1, (int)(p - path), path);
        }

        // We cannot move diagonally to a menu item because depending on the angle and other items we cross on our path we could close our target menu.
        // First move horizontally into the menu, then vertically!
        if (depth > 0)
        {
            ImGuiTestItemInfo* item = ItemInfo(buf.c_str());
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
        }
        else
        {
            // Then aim at the menu item
            ItemAction(ImGuiTestAction_Click, buf.c_str());
        }

        path = p + 1;
        depth++;
    }
}

void    ImGuiTestContext::MenuActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent)
{
    ImGuiTestItemList items;
    MenuAction(ImGuiTestAction_Open, ref_parent);
    GatherItems(&items, GetFocusWindowRef(), 1);
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

    ImGuiTestRef popup_ref = GetFocusWindowRef();
    ImGuiWindow* popup = GetWindowByRef(popup_ref);
    IM_CHECK_SILENT(popup && IsWindowACombo(popup));

    Str128f combo_item_buf = Str128f("/%s/**/%s", popup->Name, p + 1);
    ItemClick(combo_item_buf.c_str());
}

void    ImGuiTestContext::ComboClickAll(ImGuiTestRef ref_parent)
{
    ItemClick(ref_parent);

    ImGuiTestRef popup_ref = GetFocusWindowRef();
    ImGuiWindow* popup = GetWindowByRef(popup_ref);
    IM_CHECK_SILENT(popup && IsWindowACombo(popup));

    ImGuiTestItemList items;
    GatherItems(&items, popup_ref);
    for (auto item : items)
    {
        ItemClick(ref_parent); // We assume that every interaction will close the combo again
        ItemClick(item.ID);
    }
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
        ImGui::FocusWindow(window);
        Yield();
    }
}

void    ImGuiTestContext::WindowMove(ImGuiTestRef ref, ImVec2 input_pos, ImVec2 pivot)
{
    if (IsError())
        return;

    ImGuiWindow* window = GetWindowByRef(ref);
    IM_CHECK_SILENT(window != NULL);

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("WindowMove %s (%.1f,%.1f) ", window->Name, input_pos.x, input_pos.y);
    ImVec2 target_pos = ImFloor(input_pos - pivot * window->Size);
    if (ImLengthSqr(target_pos - window->Pos) < 0.001f)
        return;

    WindowBringToFront(window);
    WindowCollapse(window, false);

    // FIXME-TESTS: Need to find a -visible- click point. drag_pos may end up being outside of main viewport.
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

    MouseMoveToPos(drag_pos);
    //IM_CHECK_SILENT(UiContext->HoveredWindow == window);
    MouseDown(0);

    // Disable docking
#ifdef IMGUI_HAS_DOCK
    if (UiContext->IO.ConfigDockingWithShift)
        KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
    else
        KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
#endif

    ImVec2 delta = target_pos - window->Pos;
    MouseMoveToPos(Inputs->MousePosValue + delta);
    Yield();

    MouseUp();
#ifdef IMGUI_HAS_DOCK
    KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
#endif
}

void    ImGuiTestContext::WindowResize(ImGuiTestRef ref, ImVec2 size)
{
    if (IsError())
        return;

    ImGuiWindow* window = GetWindowByRef(ref);
    size = ImFloor(size);

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("WindowResize %s (%.1f,%.1f)", window->Name, size.x, size.y);
    if (ImLengthSqr(size - window->Size) < 0.001f)
        return;

    WindowBringToFront(window);
    WindowCollapse(window, false);

    ImGuiID id = ImGui::GetWindowResizeID(window, 0);
    MouseMove(id);
    MouseDown(0);

    ImVec2 delta = size - window->Size;
    MouseMoveToPos(Inputs->MousePosValue + delta);
    Yield(); // At this point we don't guarantee the final size!

    MouseUp();
}

void    ImGuiTestContext::PopupCloseAll()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("PopupClose");
    ImGui::ClosePopupToLevel(0, true);    // FIXME
}

#ifdef IMGUI_HAS_DOCK
void    ImGuiTestContext::DockWindowInto(const char* window_name_src, const char* window_name_dst, ImGuiDir split_dir)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("DockWindowInto '%s' to '%s'", window_name_src, window_name_dst);

    ImGuiWindow* window_src = GetWindowByRef(window_name_src);
    ImGuiWindow* window_dst = GetWindowByRef(window_name_dst);
    IM_CHECK_SILENT(window_src != NULL);
    IM_CHECK_SILENT(window_dst != NULL);
    if (!window_src || !window_dst)
        return;

    ImGuiTestRef ref_src(window_src->DockIsActive ? window_src->ID : window_src->MoveId);   // FIXME-TESTS FIXME-DOCKING: Identify tab
    ImGuiTestRef ref_dst(window_dst->DockIsActive ? window_dst->ID : window_dst->MoveId);
    ImGuiTestItemInfo* item_src = ItemInfo(ref_src);
    ImGuiTestItemInfo* item_dst = ItemInfo(ref_dst);
    ImGuiTestRefDesc desc_src(ref_src, item_src);
    ImGuiTestRefDesc desc_dst(ref_dst, item_dst);

    // Avoid focusing if we don't need it (this facilitate avoiding focus flashing when recording animated gifs)
    if (g.Windows[g.Windows.Size - 2] != window_dst)
        WindowFocus(window_name_dst);
    if (g.Windows[g.Windows.Size - 1] != window_src)
        WindowFocus(window_name_src);

    MouseMove(ref_src, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepShort();

    //// FIXME-TESTS: Not handling the operation at user's level.
    //if (split_dir != ImGuiDir_None)
    //  ImGui::DockContextQueueDock(&g, window_dst->RootWindow, window_dst->DockNode, window_src, split_dir, 0.5f, false);

    ImVec2 drop_pos;
    bool drop_is_valid = ImGui::DockContextCalcDropPosForDocking(window_dst->RootWindow, window_dst->DockNode, window_src, split_dir, false, &drop_pos);
    IM_CHECK_SILENT(drop_is_valid);
    if (!drop_is_valid)
        return;

    WindowTeleportToMakePosVisibleInViewport(window_dst, drop_pos);
    drop_is_valid = ImGui::DockContextCalcDropPosForDocking(window_dst->RootWindow, window_dst->DockNode, window_src, split_dir, false, &drop_pos);
    IM_CHECK(drop_is_valid);

    MouseDown(0);
    MouseLiftDragThreshold();
    MouseMoveToPos(drop_pos);
    IM_CHECK_SILENT(g.MovingWindow == window_src);
#ifdef IMGUI_HAS_DOCK
    IM_CHECK_SILENT(g.HoveredWindowUnderMovingWindow && g.HoveredWindowUnderMovingWindow->RootWindowDockStop == window_dst);
#else
    IM_CHECK_SILENT(g.HoveredWindowUnderMovingWindow && g.HoveredWindowUnderMovingWindow->RootWindow == window_dst);
#endif
    SleepShort();

    MouseUp(0);
    Yield();
    Yield();
}

void    ImGuiTestContext::DockMultiClear(const char* window_name, ...)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("DockMultiClear");

    va_list args;
    va_start(args, window_name);
    while (window_name != NULL)
    {
        ImGui::DockBuilderDockWindow(window_name, 0);
        window_name = va_arg(args, const char*);
    }
    va_end(args);
    Yield();
}

void    ImGuiTestContext::DockMultiSet(ImGuiID dock_id, const char* window_name, ...)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("DockMultiSet %08X", dock_id);

    va_list args;
    va_start(args, window_name);
    while (window_name != NULL)
    {
        ImGui::DockBuilderDockWindow(window_name, dock_id);
        window_name = va_arg(args, const char*);
    }
    va_end(args);
    Yield();
}

ImGuiID ImGuiTestContext::DockMultiSetupBasic(ImGuiID dock_id, const char* window_name, ...)
{
    va_list args;
    va_start(args, window_name);
    dock_id = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_None);
    ImGui::DockBuilderSetNodePos(dock_id, ImGui::GetMainViewport()->Pos + ImVec2(100, 100));
    ImGui::DockBuilderSetNodeSize(dock_id, ImVec2(200, 200));
    while (window_name != NULL)
    {
        ImGui::DockBuilderDockWindow(window_name, dock_id);
        window_name = va_arg(args, const char*);
    }
    return dock_id;
}

bool    ImGuiTestContext::DockIdIsUndockedOrStandalone(ImGuiID dock_id)
{
    if (dock_id == 0)
        return true;
    if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_id))
        if (node->IsRootNode() && node->IsLeafNode() && node->Windows.Size == 1)
            return true;
    return false;
}

void    ImGuiTestContext::DockNodeHideTabBar(ImGuiDockNode* node, bool hidden)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogDebug("DockNodeHideTabBar %d", hidden);

    // FIXME-TEST: Alter WindowRef
    if (hidden)
    {
        SetRef(node->HostWindow);
        ItemClick(ImHashDecoratedPath("#COLLAPSE", NULL, node->ID));
        ItemClick(Str16f("/##Popup_%08x/Hide tab bar", GetID("#WindowMenu", node->ID)).c_str());
        IM_CHECK_SILENT(node->IsHiddenTabBar());

    }
    else
    {
        IM_CHECK_SILENT(node->VisibleWindow != NULL);
        SetRef(node->VisibleWindow);
        ItemClick("#UNHIDE", 0, ImGuiTestOpFlags_MoveToEdgeD | ImGuiTestOpFlags_MoveToEdgeR);
        IM_CHECK_SILENT(!node->IsHiddenTabBar());
    }
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

    ImGuiID dock_button_id = ImHashDecoratedPath("#COLLAPSE", NULL, dock_id); // FIXME_TESTS
    const float h = node->Windows[0]->TitleBarHeight();
    if (!UiContext->IO.ConfigDockingWithShift)
        KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
    ItemDragWithDelta(dock_button_id, ImVec2(h, h) * -2);
    if (!UiContext->IO.ConfigDockingWithShift)
        KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
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
        KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
    ItemDragWithDelta(window_name, ImVec2(h, h) * -2);
    if (!UiContext->IO.ConfigDockingWithShift)
        KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
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

void    ImGuiTestContext::PerfCapture()
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

    // Log to .csv
    FILE* f = fopen("imgui_perflog.csv", "a+t");
    if (f == NULL)
    {
        LogError("Failed to log to CSV file!");
    }
    else
    {
        fprintf(f,
            "%s,%s,%.3f,x%d,%s,%s,%s,%s,%s,%s\n",
            Test->Category, Test->Name, dt_delta_ms,
            PerfStressAmount, EngineIO->GitBranchName, build_info.Type, build_info.Cpu, build_info.OS, build_info.Compiler, build_info.Date);
        fflush(f);
        fclose(f);
    }

    // Disable the "Success" message
    RunFlags |= ImGuiTestRunFlags_NoSuccessMsg;
}

//-------------------------------------------------------------------------
