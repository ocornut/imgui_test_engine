// dear imgui
// (test engine)

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_te_core.h"
#include "imgui_te_context.h"
#include "imgui_te_util.h"

//-------------------------------------------------------------------------
// [SECTION] ImGuiTestContext
// This is the interface that most tests will interact with.
//-------------------------------------------------------------------------

void    ImGuiTestContext::LogEx(ImGuiTestLogFlags flags, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogExV(flags, fmt, args);
    va_end(args);
}

void    ImGuiTestContext::LogExV(ImGuiTestLogFlags flags, const char* fmt, va_list args)
{
    ImGuiTestContext* ctx = this;
    ImGuiTest* test = ctx->Test;

    if (flags & ImGuiTestLogFlags_Verbose)
    {
        if (EngineIO->ConfigVerboseLevel <= ImGuiTestVerboseLevel_Min)
            return;

        if (EngineIO->ConfigVerboseLevel == ImGuiTestVerboseLevel_Normal && ctx->ActionDepth > 1)
            return;
    }

    const int prev_size = test->TestLog.size();

    if (flags & ImGuiTestLogFlags_Verbose)
    {
        if (!(flags & ImGuiTestLogFlags_NoHeader))
            test->TestLog.appendf("[%04d] -- %*s", ctx->FrameCount, ImMax(0, (ctx->ActionDepth - 1) * 2), "");
        test->TestLog.appendfv(fmt, args);
    }
    else
    {
        if (!(flags & ImGuiTestLogFlags_NoHeader))
            test->TestLog.appendf("[%04d] ", ctx->FrameCount);
        test->TestLog.appendfv(fmt, args);
    }
    test->TestLogScrollToBottom = true;

    if (EngineIO->ConfigLogToTTY)
        printf("%s", test->TestLog.c_str() + prev_size);
}

void    ImGuiTestContext::Log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogExV(ImGuiTestLogFlags_None, fmt, args);
    va_end(args);
}

void    ImGuiTestContext::LogVerbose(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogExV(ImGuiTestLogFlags_Verbose, fmt, args);
    va_end(args);
}

void    ImGuiTestContext::LogDebug()
{
    ImGuiID item_hovered_id = UiContext->HoveredIdPreviousFrame;
    ImGuiID item_active_id = UiContext->ActiveId;
    ImGuiTestItemInfo* item_hovered_info = item_hovered_id ? ImGuiTestEngine_ItemLocate(Engine, item_hovered_id, "") : NULL;
    ImGuiTestItemInfo* item_active_info = item_active_id ? ImGuiTestEngine_ItemLocate(Engine, item_active_id, "") : NULL;
    LogVerbose("Hovered: 0x%08X (\"%s\"), Active:  0x%08X(\"%s\")\n", 
        item_hovered_id, item_hovered_info ? item_hovered_info->DebugLabel : "",
        item_active_id, item_active_info ? item_active_info->DebugLabel : "");
}

void    ImGuiTestContext::Finish()
{
    if (RunFlags & ImGuiTestRunFlags_NoTestFunc)
        return;
    ImGuiTest* test = Test;
    if (test->Status == ImGuiTestStatus_Running)
        test->Status = ImGuiTestStatus_Success;
}

void    ImGuiTestContext::RecoverFromUiContextErrors()
{
    ImGuiContext& g = *UiContext;
    IM_ASSERT(Test != NULL);

    bool recovered = false;
    while (g.CurrentWindowStack.Size > 1) // FIXME-ERRORHANDLING
    {
        // FIXME-ERRORHANDLING: Can't recover from inside BeginTabItem/EndTabItem yet.
        // FIXME-ERRORHANDLING: Can't recover from interleaved BeginTabBar/Begin
        while (g.CurrentTabBar != NULL)
            ImGui::EndTabBar();
        ImGui::End();
        recovered = true;
    }
    if (recovered && Test->Status != ImGuiTestStatus_Error)
        LogVerbose("[warn] Recovered invalid ui state at end of frame.\n");
}

void    ImGuiTestContext::Yield()
{
    ImGuiTestEngine_Yield(Engine);
}

void    ImGuiTestContext::YieldFrames(int count = 0)
{
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

void    ImGuiTestContext::SleepDebugNoSkip(float time)
{
    if (IsError())
        return;

    while (time > 0.0f && !Abort)
    {
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

void ImGuiTestContext::SetRef(ImGuiTestRef ref)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("SetRef '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

    if (ref.Path)
    {
        size_t len = strlen(ref.Path);
        IM_ASSERT(len < IM_ARRAYSIZE(RefStr) - 1);

        strcpy(RefStr, ref.Path);
        RefID = ImHashDecoratedPath(ref.Path, 0);
    }
    else
    {
        RefStr[0] = 0;
        RefID = ref.ID;
    }
}

ImGuiWindow* ImGuiTestContext::GetWindowByRef(ImGuiTestRef ref)
{
    ImGuiID window_id = GetID(ref);
    ImGuiWindow* window = ImGui::FindWindowByID(window_id);
    return window;
}

ImGuiID ImGuiTestContext::GetID(ImGuiTestRef ref)
{
    if (ref.ID)
        return ref.ID;
    return ImHashDecoratedPath(ref.Path, RefID);
}

ImGuiID ImGuiTestContext::GetID(ImGuiTestRef seed_ref, ImGuiTestRef ref)
{
    if (ref.ID)
        return ref.ID; // FIXME: What if seed_ref != 0
    return ImHashDecoratedPath(ref.Path, GetID(seed_ref));
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

ImGuiTestItemInfo* ImGuiTestContext::ItemLocate(ImGuiTestRef ref, ImGuiTestOpFlags flags)
{
    if (IsError())
        return NULL;

    ImGuiID full_id;
    if (ref.ID)
        full_id = ref.ID;
    else
        full_id = ImHashDecoratedPath(ref.Path, RefID);

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestItemInfo* item = NULL;
    int retries = 0;
    while (retries < 2)
    {
        item = ImGuiTestEngine_ItemLocate(Engine, full_id, ref.Path);
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
            IM_ERRORF_NOHDR("Unable to locate item: '%s/%s'", RefStr, ref.Path);
        else
            IM_ERRORF_NOHDR("Unable to locate item: 0x%08X", ref.ID);
    }

    return NULL;
}

// FIXME-TESTS: scroll_ratio_y unsupported
void    ImGuiTestContext::ScrollToY(ImGuiTestRef ref, float scroll_ratio_y)
{
    IM_UNUSED(scroll_ratio_y);

    if (IsError())
        return;

    // If the item is not currently visible, scroll to get it in the center of our window
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiContext& g = *UiContext;
    ImGuiTestItemInfo* item = ItemLocate(ref);
    ImGuiTestRefDesc desc(ref, item);
    LogVerbose("ScrollToY %s\n", desc.c_str());

    ImGuiWindow* window = item->Window;

    //if (item->ID == 0xDFFBB0CE || item->ID == 0x87CBBA09)
    //    printf("[%03d] scroll_max_y %f\n", FrameCount, ImGui::GetWindowScrollMaxY(window));

    while (!Abort)
    {
        // result->Rect fields will be updated after each iteration.
        float item_curr_y = ImFloor(item->RectFull.GetCenter().y);
        float item_target_y = ImFloor(window->InnerClipRect.GetCenter().y);
        float scroll_delta_y = item_target_y - item_curr_y;
        float scroll_target_y = ImClamp(window->Scroll.y - scroll_delta_y, 0.0f, window->ScrollMax.y);

        if (ImFabs(window->Scroll.y - scroll_target_y) < 1.0f)
            break;

        // FIXME-TESTS: There's a bug which can be repro by moving #RESIZE grips to Layer 0, making window small and trying to resize a dock node host.
        // Somehow SizeContents.y keeps increase and we never reach our desired (but faulty) scroll target.
        float scroll_speed = EngineIO->ConfigRunFast ? FLT_MAX : ImFloor(EngineIO->ScrollSpeed * g.IO.DeltaTime + 0.99f);
        float scroll_y = ImLinearSweep(window->Scroll.y, scroll_target_y, scroll_speed);
        //printf("[%03d] window->Scroll.y %f + %f\n", FrameCount, window->Scroll.y, scroll_speed);
        //window->Scroll.y = scroll_y;
        ImGui::SetWindowScrollY(window, scroll_y);

        Yield();

        BringWindowToFrontFromItem(ref);
    }

    // Need another frame for the result->Rect to stabilize
    Yield();
}

void    ImGuiTestContext::NavMove(ImGuiTestRef ref)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiContext& g = *UiContext;
    ImGuiTestItemInfo* item = ItemLocate(ref);
    ImGuiTestRefDesc desc(ref, item);
    LogVerbose("NavMove to %s\n", desc.c_str());

    if (item == NULL)
        return;
    item->RefCount++;

    // Focus window before scrolling/moving so things are nicely visible
    BringWindowToFrontFromItem(ref);

    ImGui::SetNavID(item->ID, item->NavLayer);
    Yield();

    if (!Abort)
    {
        if (g.NavId != item->ID)
            IM_ERRORF_NOHDR("Unable to set NavId to %s", desc.c_str());
    }

    item->RefCount--;
}

void    ImGuiTestContext::NavActivate()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("NavActivate\n");

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
    LogVerbose("NavInput\n");

    int frames_to_hold = 2; // FIXME-TESTS: <-- number of frames could be fuzzed here
    for (int n = 0; n < frames_to_hold; n++)
    {
        ImGuiTestEngine_PushInput(Engine, ImGuiTestInput::FromNav(ImGuiNavInput_Input, ImGuiKeyState_Down));
        Yield();
    }
    Yield();
}

// FIXME: Maybe ImGuiTestOpFlags_NoCheckHoveredId could be automatic if we detect that another item is active as intended?
void    ImGuiTestContext::MouseMove(ImGuiTestRef ref, ImGuiTestOpFlags flags)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiContext& g = *UiContext;
    ImGuiTestItemInfo* item = ItemLocate(ref);
    ImGuiTestRefDesc desc(ref, item);
    LogVerbose("MouseMove to %s\n", desc.c_str());

    if (item == NULL)
        return;
    item->RefCount++;

    // Focus window before scrolling/moving so things are nicely visible
    if (!(flags & ImGuiTestOpFlags_NoFocusWindow))
        BringWindowToFrontFromItem(ref);

    ImGuiWindow* window = item->Window;
    ImRect window_inner_r_padded = window->InnerClipRect;
    window_inner_r_padded.Expand(-4.0f); // == WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS
    if (item->NavLayer == ImGuiNavLayer_Main && !window_inner_r_padded.Contains(item->RectClipped))
        ScrollToY(ref);

    ImVec2 pos = item->RectFull.GetCenter();
    WindowMoveToMakePosVisible(window, pos);
    
    // Move toward an actually visible point
    pos = item->RectClipped.GetCenter();
    MouseMoveToPos(pos);

    // Focus again in case something made us lost focus (which could happen on a simple hover)
    if (!(flags & ImGuiTestOpFlags_NoFocusWindow))
        BringWindowToFrontFromItem(ref);// , ImGuiTestOpFlags_Verbose);

    if (!Abort && !(flags & ImGuiTestOpFlags_NoCheckHoveredId))
    {
        if (g.HoveredIdPreviousFrame != item->ID)
            IM_ERRORF_NOHDR("Unable to Hover %s. Expected %08X in '%s', HoveredId was %08X in '%s'. Targeted position (%.1f,%.1f)",
                desc.c_str(), item->ID, item->Window ? item->Window->Name : "<NULL>",
                g.HoveredIdPreviousFrame, g.HoveredWindow ? g.HoveredWindow->Name : "",
                pos.x, pos.y);
    }

    item->RefCount--;
}

void    ImGuiTestContext::WindowMoveToMakePosVisible(ImGuiWindow* window, ImVec2 pos)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    ImRect visible_r(0.0f, 0.0f, g.IO.DisplaySize.x, g.IO.DisplaySize.y);   // FIXME: Viewport
    if (!visible_r.Contains(pos))
    {
        // Fallback move window directly to make our item reachable with the mouse.
        float pad = g.FontSize;
        ImVec2 delta;
        delta.x = (pos.x < visible_r.Min.x) ? (visible_r.Min.x - pos.x + pad) : (pos.x > visible_r.Max.x) ? (visible_r.Max.x - pos.x - pad) : 0.0f;
        delta.y = (pos.y < visible_r.Min.y) ? (visible_r.Min.y - pos.y + pad) : (pos.y > visible_r.Max.y) ? (visible_r.Max.y - pos.y - pad) : 0.0f;
        ImGui::SetWindowPos(window, window->Pos + delta, ImGuiCond_Always);
        LogVerbose("WindowMoveBypass %s delta (%.1f,%.1f)\n", window->Name, delta.x, delta.y);
        Yield();
    }
}

void	ImGuiTestContext::MouseMoveToPos(ImVec2 target)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MouseMove from (%.0f,%.0f) to (%.0f,%.0f)\n", Inputs->MousePosValue.x, Inputs->MousePosValue.y, target.x, target.y);

    // We manipulate Inputs->MousePosValue without reading back from g.IO.MousePos because the later is rounded.
    // To handle high framerate it is easier to bypass this rounding.
    while (true)
    {
        ImVec2 delta = target - Inputs->MousePosValue;
        float inv_length = ImInvLength(delta, FLT_MAX);
        float move_speed = EngineIO->MouseSpeed * g.IO.DeltaTime;
        //if (g.IO.KeyShift)
        //    move_speed *= 0.1f;

        //ImGui::GetOverlayDrawList()->AddCircle(target, 10.0f, IM_COL32(255, 255, 0, 255));

        if (EngineIO->ConfigRunFast || (inv_length >= 1.0f / move_speed))
        {
            Inputs->MousePosValue = target;
            ImGuiTestEngine_Yield(Engine);
            ImGuiTestEngine_Yield(Engine);
            return;
        }
        else
        {
            Inputs->MousePosValue = Inputs->MousePosValue + delta * move_speed * inv_length;
            ImGuiTestEngine_Yield(Engine);
        }
    }
}

void    ImGuiTestContext::MouseDown(int button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MouseDown %d\n", button);

    UiContext->IO.MouseClickedTime[button] = -FLT_MAX; // Prevent accidental double-click from happening ever
    Inputs->MouseButtonsValue = (1 << button);
    Yield();
}

void    ImGuiTestContext::MouseUp(int button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MouseUp %d\n", button);

    Inputs->MouseButtonsValue &= ~(1 << button);
    Yield();
}

// TODO: click time argument (seconds and/or frames)
void    ImGuiTestContext::MouseClick(int button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MouseClick %d\n", button);

    // Make sure mouse buttons are released
    IM_ASSERT(Inputs->MouseButtonsValue == 0);

    Yield();

    // Press
    UiContext->IO.MouseClickedTime[button] = -FLT_MAX; // Prevent accidental double-click from happening ever
    Inputs->MouseButtonsValue = (1 << button);
    Yield();
    Inputs->MouseButtonsValue = 0;
    Yield();
    Yield(); // Give a frame for items to react
}

// TODO: click time argument (seconds and/or frames)
void    ImGuiTestContext::MouseDoubleClick(int button)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MouseDoubleClick %d\n", button);

    Yield();
    UiContext->IO.MouseClickedTime[button] = -FLT_MAX; // Prevent accidental double-click from happening ever
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

void    ImGuiTestContext::KeyDownMap(ImGuiKey key, int mod_flags)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    char mod_flags_str[32];
    GetImGuiKeyModsPrefixStr(mod_flags, mod_flags_str, IM_ARRAYSIZE(mod_flags_str));
    LogVerbose("KeyDownMap(%s%s)\n", mod_flags_str, (key != ImGuiKey_COUNT) ? GetImGuiKeyName(key) : "");
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
    LogVerbose("KeyUpMap(%s%s)\n", mod_flags_str, (key != ImGuiKey_COUNT) ? GetImGuiKeyName(key) : "");
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
    LogVerbose("KeyPressMap(%s%s, %d)\n", mod_flags_str, (key != ImGuiKey_COUNT) ? GetImGuiKeyName(key) : "", count);
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
    LogVerbose("KeyChars('%s')\n", chars);
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
    LogVerbose("KeyCharsAppend('%s')\n", chars);
    KeyPressMap(ImGuiKey_End);
    KeyChars(chars);
}

void    ImGuiTestContext::KeyCharsAppendEnter(const char* chars)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("KeyCharsAppendValidate('%s')\n", chars);
    KeyPressMap(ImGuiKey_End);
    KeyChars(chars);
    KeyPressMap(ImGuiKey_Enter);
}

void    ImGuiTestContext::FocusWindow(ImGuiTestRef ref)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestRefDesc desc(ref, NULL);
    LogVerbose("FocusWindow('%s')\n", desc.c_str());

    ImGuiID window_id = GetID(ref);
    ImGuiWindow* window = ImGui::FindWindowByID(window_id);
    IM_CHECK_SILENT(window != NULL);
    if (window)
    {
        ImGui::FocusWindow(window);
        Yield();
    }
}

bool    ImGuiTestContext::BringWindowToFront(ImGuiWindow* window, ImGuiTestOpFlags flags)
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
        LogVerbose("BringWindowToFront->FocusWindow('%s')\n", window->Name);
        ImGui::FocusWindow(window);
        Yield();
        Yield();
        //IM_CHECK(g.NavWindow == window);
    }
    else if (window->RootWindow != g.Windows.back()->RootWindow)
    {
        IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
        LogVerbose("BringWindowToDisplayFront('%s') (window.back=%s)\n", window->Name, g.Windows.back()->Name);
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
        LogVerbose("-- [warn] Expected focused window '%s', but '%s' got focus back.\n", window->Name, g.NavWindow ? g.NavWindow->Name : "<NULL>");
    
    return ret;
}

bool    ImGuiTestContext::BringWindowToFrontFromItem(ImGuiTestRef ref)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return false;

    const ImGuiTestItemInfo* item = ItemLocate(ref);
    if (item == NULL)
        IM_CHECK_RETV(item != NULL, false);
    //LogVerbose("FocusWindowForItem %s\n", ImGuiTestRefDesc(ref, item).c_str());
    //LogVerbose("Lost focus on window '%s', getting again..\n", item->Window->Name);
    bool ret = BringWindowToFront(item->Window, ImGuiTestOpFlags_NoError);
    if (!ret)
        LogVerbose("-- [warn] Expected focused window '%s' for item '%s', but '%s' got focus back.\n", item->Window->Name, item->DebugLabel, g.NavWindow ? g.NavWindow->Name : "<NULL>");

    return ret;
}

void    ImGuiTestContext::GatherItems(ImGuiTestItemList* out_list, ImGuiTestRef parent, int depth)
{
    IM_ASSERT(out_list != NULL);
    IM_ASSERT(depth > 0 || depth == -1);
    IM_ASSERT(GatherTask->ParentID == 0);
    IM_ASSERT(GatherTask->LastItemInfo == NULL);

    if (IsError())
        return;

    // Register gather tasks
    if (depth == -1)
        depth = 99;
    if (parent.ID == 0)
        parent.ID = GetID(parent);
    GatherTask->ParentID = parent.ID;
    GatherTask->Depth = depth;
    GatherTask->OutList = out_list;

    // Keep running while gathering
    const int begin_gather_size = out_list->Size;
    while (true)
    {
        const int begin_gather_size_for_frame = out_list->Size;
        Yield();
        const int end_gather_size_for_frame = out_list->Size;
        if (begin_gather_size_for_frame == end_gather_size_for_frame)
            break;
    }
    const int end_gather_size = out_list->Size;

    ImGuiTestItemInfo* parent_item = ItemLocate(parent, ImGuiTestOpFlags_NoError);
    LogVerbose("GatherItems from %s, %d deep: found %d items.\n", ImGuiTestRefDesc(parent, parent_item).c_str(), depth, end_gather_size - begin_gather_size);

    GatherTask->ParentID = 0;
    GatherTask->Depth = 0;
    GatherTask->OutList = NULL;
    GatherTask->LastItemInfo = NULL;
}

void    ImGuiTestContext::ItemAction(ImGuiTestAction action, ImGuiTestRef ref)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);

    // [DEBUG] Breakpoint
    //if (ref.ID == 0x0d4af068)
    //    printf("");

    ImGuiTestItemInfo* item = ItemLocate(ref);
    ImGuiTestRefDesc desc(ref, item);
    if (item == NULL)
        return;

    LogVerbose("Item%s %s%s\n", GetActionName(action), desc.c_str(), (InputMode == ImGuiInputSource_Mouse) ? "" : " (w/ Nav)");

    if (action == ImGuiTestAction_Click || action == ImGuiTestAction_DoubleClick)
    {
        if (InputMode == ImGuiInputSource_Mouse)
        {
            MouseMove(ref);
            if (!EngineIO->ConfigRunFast)
                Sleep(0.05f);
            if (action == ImGuiTestAction_DoubleClick)
                MouseDoubleClick(0);
            else
                MouseClick(0);
        }
        else
        {
            NavMove(ref);
            NavActivate();
            if (action == ImGuiTestAction_DoubleClick)
                IM_ASSERT(0);
        }
        return;
    }

    if (action == ImGuiTestAction_Input)
    {
        if (InputMode == ImGuiInputSource_Mouse)
        {
            MouseMove(ref);
            KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Ctrl);
            MouseClick(0);
            KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Ctrl);
        }
        else
        {
            NavMove(ref);
            NavInput();
        }
        return;
    }

    if (action == ImGuiTestAction_Open)
    {
        if (item && (item->StatusFlags & ImGuiItemStatusFlags_Opened) == 0)
        {
            item->RefCount++;
            MouseMove(ref); // Some item just on hover, give them that chance
            if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) == 0)
            {
                ItemClick(ref);
                if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) == 0)
                {
                    ItemDoubleClick(ref); // Attempt a double-click // FIXME-TESTS: let's not start doing those fuzzy things..
                    if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) == 0)
                        IM_ERRORF_NOHDR("Unable to Open item: %s", ImGuiTestRefDesc(ref, item).c_str());
                }
            }
            item->RefCount--;
            Yield();
        }
        return;
    }

    if (action == ImGuiTestAction_Close)
    {
        if (item && (item->StatusFlags & ImGuiItemStatusFlags_Opened) != 0)
        {
            item->RefCount++;
            ItemClick(ref);
            if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) != 0)
            {
                ItemDoubleClick(ref); // Attempt a double-click // FIXME-TESTS: let's not start doing those fuzzy things..
                if ((item->StatusFlags & ImGuiItemStatusFlags_Opened) != 0)
                    IM_ERRORF_NOHDR("Unable to Close item: %s", ImGuiTestRefDesc(ref, item).c_str());
            }
            item->RefCount--;
            Yield();
        }
        return;
    }

    if (action == ImGuiTestAction_Check)
    {
        if ((item->StatusFlags & ImGuiItemStatusFlags_Checkable) && !(item->StatusFlags & ImGuiItemStatusFlags_Checked))
        {
            ItemClick(ref);
            Yield();
        }
        ItemVerifyCheckedIfAlive(ref, true);     // We can't just IM_ASSERT(ItemIsChecked()) because the item may disappear and never update its StatusFlags any more!

        return;
    }

    if (action == ImGuiTestAction_Uncheck)
    {
        if ((item->StatusFlags & ImGuiItemStatusFlags_Checkable) && (item->StatusFlags & ImGuiItemStatusFlags_Checked))
        {
            ItemClick(ref);
            Yield();
        }
        ItemVerifyCheckedIfAlive(ref, false);       // We can't just IM_ASSERT(ItemIsChecked()) because the item may disappear and never update its StatusFlags any more!

        return;
    }

    IM_ASSERT(0);
}

void    ImGuiTestContext::ItemActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent, int max_depth, int max_passes)
{
    if (max_depth == -1)
        max_depth = 99;
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
        {
            for (int n = 0; n < items.Size; n++)
            {
                const ImGuiTestItemInfo* info = items[n];
                if ((info->StatusFlags & ImGuiItemStatusFlags_Openable) && (info->StatusFlags & ImGuiItemStatusFlags_Opened))
                    highest_depth = ImMax(highest_depth, info->Depth);
            }
        }

        int actioned_total_at_beginning_of_pass = actioned_total;

        // Process top-to-bottom in most cases
        int scan_start = 0;
        int scan_end = items.Size;
        int scan_dir = +1;
        if (action == ImGuiTestAction_Close)
        {
            // Close bottom-to-top because 
            // 1) it is more likely to handle same-depth parent/child relationship better (e.g. CollapsingHeader)
            // 2) it gives a nicer sense of symmetry with the corresponding open operation.
            scan_start = items.Size - 1;
            scan_end = -1;
            scan_dir = -1;
        }

        for (int n = scan_start; n != scan_end; n += scan_dir)
        {
            if (IsError())
                break;

            const ImGuiTestItemInfo* info = items[n];
            switch (action)
            {
            case ImGuiTestAction_Click:
                ItemAction(action, info->ID);
                actioned_total++;
                break;
            case ImGuiTestAction_Check:
                if ((info->StatusFlags & ImGuiItemStatusFlags_Checkable) && !(info->StatusFlags & ImGuiItemStatusFlags_Checked))
                {
                    ItemAction(action, info->ID);
                    actioned_total++;
                }
                break;
            case ImGuiTestAction_Uncheck:
                if ((info->StatusFlags & ImGuiItemStatusFlags_Checkable) && (info->StatusFlags & ImGuiItemStatusFlags_Checked))
                {
                    ItemAction(action, info->ID);
                    actioned_total++;
                }
                break;
            case ImGuiTestAction_Open:
                if ((info->StatusFlags & ImGuiItemStatusFlags_Openable) && !(info->StatusFlags & ImGuiItemStatusFlags_Opened))
                {
                    ItemAction(action, info->ID);
                    actioned_total++;
                }
                break;
            case ImGuiTestAction_Close:
                if (info->Depth == highest_depth && (info->StatusFlags & ImGuiItemStatusFlags_Openable) && (info->StatusFlags & ImGuiItemStatusFlags_Opened))
                {
                    ItemClose(info->ID);
                    actioned_total++;
                }
                break;
            default:
                IM_ASSERT(0);
            }
        }

        if (IsError())
            break;

        if (actioned_total_at_beginning_of_pass == actioned_total)
            break;
    }
    LogVerbose("%s %d items in total!\n", GetActionVerb(action), actioned_total);
}

void    ImGuiTestContext::ItemHold(ImGuiTestRef ref, float time)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("ItemHold '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

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
    LogVerbose("ItemHoldForFrames '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

    MouseMove(ref);
    Yield();
    Inputs->MouseButtonsValue = (1 << 0);
    YieldFrames(frames);
    Inputs->MouseButtonsValue = 0;
    Yield();
}

void    ImGuiTestContext::ItemDragAndDrop(ImGuiTestRef ref_src, ImGuiTestRef ref_dst)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    ImGuiTestItemInfo* item_src = ItemLocate(ref_src);
    ImGuiTestItemInfo* item_dst = ItemLocate(ref_dst);
    ImGuiTestRefDesc desc_src(ref_src, item_src);
    ImGuiTestRefDesc desc_dst(ref_dst, item_dst);
    LogVerbose("ItemDragAndDrop %s to %s\n", desc_src.c_str(), desc_dst.c_str());

    MouseMove(ref_src, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepShort();
    MouseDown(0);

    // Enforce lifting drag threshold even if both item are exactly at the same location.
    MouseLiftDragThreshold();

    MouseMove(ref_dst, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepShort();
    MouseUp(0);
}

void    ImGuiTestContext::ItemVerifyCheckedIfAlive(ImGuiTestRef ref, bool checked)
{
    Yield();
    ImGuiTestItemInfo* item = ItemLocate(ref, ImGuiTestOpFlags_NoError);
    if (item
     && (item->TimestampMain + 1 >= ImGuiTestEngine_GetFrameCount(Engine))
     && (item->TimestampStatus == item->TimestampMain)
     && (((item->StatusFlags & ImGuiItemStatusFlags_Checked) != 0) != checked))
    {
        IM_CHECK(((item->StatusFlags & ImGuiItemStatusFlags_Checked) != 0) == checked);
    }
}

void    ImGuiTestContext::MenuAction(ImGuiTestAction action, ImGuiTestRef ref)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("MenuAction '%s' %08X\n", ref.Path ? ref.Path : "NULL", ref.ID);

    IM_ASSERT(ref.Path != NULL);

    int depth = 0;
    const char* path = ref.Path;
    const char* path_end = path + strlen(path);
    char buf[128];
    while (path < path_end)
    {
        const char* p = ImStrchrRange(path, path_end, '/');
        if (p == NULL)
            p = path_end;
        const bool is_target_item = (p == path_end);
        if (depth == 0)
        {
            // Click menu in menu bar
            IM_ASSERT(RefStr[0] != 0); // Unsupported: window needs to be in Ref
            ImFormatString(buf, IM_ARRAYSIZE(buf), "##menubar/%.*s", (int)(p - path), path);
        }
        else
        {
            // Click sub menu in its own window
            ImFormatString(buf, IM_ARRAYSIZE(buf), "/##Menu_%02d/%.*s", depth - 1, (int)(p - path), path);
        }

        // We cannot move diagonally to a menu item because depending on the angle and other items we cross on our path we could close our target menu.
        // First move horizontal into the menu...
        if (depth > 0)
        {
            ImGuiTestItemInfo* item = ItemLocate(buf);
            IM_CHECK(item != NULL);
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
            ItemAction(action, buf);
        }
        else
        {
            // Then aim at the menu item
            ItemAction(ImGuiTestAction_Click, buf);
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
    for (int n = 0; n < items.Size; n++)
    {
        const ImGuiTestItemInfo* item = items[n];
        MenuAction(ImGuiTestAction_Open, ref_parent); // We assume that every interaction will close the menu again
        ItemAction(action, item->ID);
    }
}

void    ImGuiTestContext::WindowClose()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("WindowClose\n");
    ItemClick("#CLOSE");
}

void    ImGuiTestContext::WindowSetCollapsed(ImGuiTestRef ref, bool collapsed)
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("WindowSetCollapsed(%d)\n", collapsed);
    ImGuiWindow* window = GetWindowByRef(ref);
    if (window == NULL)
    {
        IM_ERRORF_NOHDR("Unable to find Ref window: %s / %08X", RefStr, RefID);
        return;
    }

    if (window->Collapsed != collapsed)
    {
        ItemClick(GetID(ref, "#COLLAPSE"));
        IM_CHECK(window->Collapsed == collapsed);
    }
}

void    ImGuiTestContext::WindowMove(ImGuiTestRef ref, ImVec2 input_pos, ImVec2 pivot)
{
    if (IsError())
        return;

    ImGuiWindow* window = GetWindowByRef(ref);
    IM_CHECK_SILENT(window != NULL);

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("WindowMove %s (%.1f,%.1f) \n", window->Name, input_pos.x, input_pos.y);
    ImVec2 target_pos = ImFloor(input_pos - pivot * window->Size);
    if (ImLengthSqr(target_pos - window->Pos) < 0.001f)
        return;

    BringWindowToFront(window);
    WindowSetCollapsed(ref, false);

    float h = ImGui::GetFrameHeight();

    // FIXME-TESTS: Need to find a -visible- click point
    MouseMoveToPos(window->Pos + ImVec2(h * 2.0f, h * 0.5f));
    //IM_CHECK_SILENT(UiContext->HoveredWindow == window);  // FIXME-TESTS: 
    MouseDown(0);

    // Disable docking
#if IMGUI_HAS_DOCK
    if (UiContext->IO.ConfigDockingWithShift)
        KeyUpMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
    else
        KeyDownMap(ImGuiKey_COUNT, ImGuiKeyModFlags_Shift);
#endif

    ImVec2 delta = target_pos - window->Pos;
    MouseMoveToPos(Inputs->MousePosValue + delta);
    Yield();

    MouseUp();
#if IMGUI_HAS_DOCK
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
    LogVerbose("WindowResize %s (%.1f,%.1f)\n", window->Name, size.x, size.y);
    if (ImLengthSqr(size - window->Size) < 0.001f)
        return;

    BringWindowToFront(window);
    WindowSetCollapsed(ref, false);

    ImGuiID id = 0;
#ifdef IMGUI_HAS_DOCK
    if (window->DockIsActive)
        id = GetID(window->DockNode->HostWindow->ID, "#RESIZE");
    else
#endif
        id = GetID(ref, "#RESIZE");

    // FIXME-TESTS: Resize grip are pushing a void* index
    const void* zero_ptr = (void*)0;
    id = ImHashData(&zero_ptr, sizeof(void*), id);

    MouseMove(id);
    MouseDown(0);

    ImVec2 delta = size - window->Size;
    MouseMoveToPos(Inputs->MousePosValue + delta);
    Yield();

    MouseUp();
}

void    ImGuiTestContext::PopupClose()
{
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("PopupClose\n");
    ImGui::ClosePopupToLevel(0, true);    // FIXME
}

#ifdef IMGUI_HAS_DOCK
void    ImGuiTestContext::DockWindowInto(const char* window_name_src, const char* window_name_dst, ImGuiDir split_dir)
{
    ImGuiContext& g = *UiContext;
    if (IsError())
        return;

    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("DockWindowInto '%s' to '%s'\n", window_name_src, window_name_dst);

    ImGuiWindow* window_src = GetWindowByRef(window_name_src);
    ImGuiWindow* window_dst = GetWindowByRef(window_name_dst);
    IM_CHECK_SILENT(window_src != NULL);
    IM_CHECK_SILENT(window_dst != NULL);
    if (!window_src || !window_dst)
        return;

    ImGuiTestRef ref_src(window_src->DockIsActive ? window_src->ID : window_src->MoveId);   // FIXME-TESTS FIXME-DOCKING: Identify tab
    ImGuiTestRef ref_dst(window_dst->DockIsActive ? window_dst->ID : window_dst->MoveId);
    ImGuiTestItemInfo* item_src = ItemLocate(ref_src);
    ImGuiTestItemInfo* item_dst = ItemLocate(ref_dst);
    ImGuiTestRefDesc desc_src(ref_src, item_src);
    ImGuiTestRefDesc desc_dst(ref_dst, item_dst);
    FocusWindow(window_name_dst);
    FocusWindow(window_name_src);

    MouseMove(ref_src, ImGuiTestOpFlags_NoCheckHoveredId);
    SleepShort();

    //// FIXME-TESTS: Not handling the operation at user's level.
    //if (split_dir != ImGuiDir_None)
    //  ImGui::DockContextQueueDock(&g, window_dst->RootWindow, window_dst->DockNode, window_src, split_dir, 0.5f, false);

    ImVec2 drop_pos;
    bool drop_is_valid = ImGui::DockContextCalcDropPosForDocking(window_dst->RootWindow, window_dst->DockNode, window_src, split_dir, false, &drop_pos);
    IM_CHECK(drop_is_valid);
    if (!drop_is_valid)
        return;

    WindowMoveToMakePosVisible(window_dst, drop_pos);
    drop_is_valid = ImGui::DockContextCalcDropPosForDocking(window_dst->RootWindow, window_dst->DockNode, window_src, split_dir, false, &drop_pos);
    IM_CHECK(drop_is_valid);

    MouseDown(0);
    MouseLiftDragThreshold();
    MouseMoveToPos(drop_pos);
    IM_CHECK_SILENT(g.MovingWindow == window_src);
    IM_CHECK_SILENT(g.HoveredWindowUnderMovingWindow == window_dst);
    SleepShort();

    MouseUp(0);
    Yield();
    Yield();
}

void    ImGuiTestContext::DockMultiClear(const char* window_name, ...)
{
    IMGUI_TEST_CONTEXT_REGISTER_DEPTH(this);
    LogVerbose("DockMultiClear\n");
    
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
    LogVerbose("DockMultiSet %08X\n", dock_id);

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

#endif // #ifdef IMGUI_HAS_DOCK

//-------------------------------------------------------------------------
// ImGuiTestContext - Performance Tools
//-------------------------------------------------------------------------

void    ImGuiTestContext::PerfCalcRef()
{
    LogVerbose("Measuring ref dt...\n");
    SetGuiFuncEnabled(false);
    for (int n = 0; n < 500 && !Abort; n++)
        Yield();
    PerfRefDt = ImGuiTestEngine_GetPerfDeltaTime500Average(Engine);
    SetGuiFuncEnabled(true);
}

void    ImGuiTestContext::PerfCapture()
{
    if (PerfRefDt < 0.0)
        PerfCalcRef();
    IM_ASSERT(PerfRefDt >= 0.0);

    // Yield for the average to stabilize
    LogVerbose("Measuring gui dt...\n");
    for (int n = 0; n < 500 && !Abort; n++)
        Yield();
    if (Abort)
        return;

    double dt_curr = ImGuiTestEngine_GetPerfDeltaTime500Average(Engine);
    double dt_ref_ms = PerfRefDt * 1000;
    double dt_delta_ms = (dt_curr - PerfRefDt) * 1000;

    const ImGuiTestsBuildInfo& build_info = ImGetBuildInfo();

    //Log("[PERF] Name: %s\n", Test->Name);
    Log("[PERF] Conditions: Stress x%d, %s, %s, %s, %s, %s\n", 
        PerfStressAmount, build_info.Type, build_info.Cpu, build_info.OS, build_info.Compiler, build_info.Date);
    Log("[PERF] Result: %+6.3f ms (from ref %+6.3f)\n", dt_delta_ms, dt_ref_ms);

    // Log to .csv
    FILE* csv_perf_log = ImGuiTestEngine_GetPerfPersistentLogCsv(Engine);
    if (csv_perf_log != NULL)
    {
        fprintf(csv_perf_log,
            ",%s,%s,%.3f,,%d,%s,%s,%s,%s,%s\n",
            Test->Category, Test->Name, dt_delta_ms,
            PerfStressAmount, build_info.Type, build_info.Cpu, build_info.OS, build_info.Compiler, build_info.Date);
        fflush(csv_perf_log);
    }

    // Disable the "Success" message
    RunFlags |= ImGuiTestRunFlags_NoSuccessMsg;
}

//-------------------------------------------------------------------------
