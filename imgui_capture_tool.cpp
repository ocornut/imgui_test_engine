// Dear ImGui Screen/Video Capture Tool

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_capture_tool.h"
#include "imgui_te_util.h"

// stb_image_write
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4456)                             // declaration of 'xx' hides previous local declaration
#pragma warning (disable: 4457)                             // declaration of 'xx' hides function parameter
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libs/stb_image_write.h"
#ifdef _MSC_VER
#pragma warning (pop)
#endif

//-----------------------------------------------------------------------------
// ImageBuf
//-----------------------------------------------------------------------------

void ImageBuf::Clear()
{
    if (Data)
        free(Data);
    Data = NULL;
}

void ImageBuf::CreateEmpty(int w, int h)
{
    CreateEmptyNoMemClear(w, h);
    memset(Data, 0, Width * Height * 4);
}

void ImageBuf::CreateEmptyNoMemClear(int w, int h)
{
    Clear();
    Width = w;
    Height = h;
    Data = (u32*) malloc(Width * Height * 4);
}

void ImageBuf::SaveFile(const char* filename)
{
    stbi_write_png(filename, Width, Height, 4, Data, Width * 4);
}

void ImageBuf::RemoveAlpha()
{
    u32* p = Data;
    int n = Width * Height;
    while (n-- > 0)
    {
        *p |= 0xFF000000;
        p++;
    }
}

//-----------------------------------------------------------------------------
// ImGuiCaptureTool
//-----------------------------------------------------------------------------

ImGuiCaptureTool::ImGuiCaptureTool(ImGuiScreenCaptureFunc capture_func)
{
    Visible = false;
    Flags = ImGuiCaptureToolFlags_StitchFullContents;
    ScreenCaptureFunc = capture_func;
    Padding = 10.0f;
    SnapGridSize = 32.0f;
    memset(SaveFileName, 0, sizeof(SaveFileName));
    ImStrncpy(SaveFileName, "imgui_capture.png", (size_t)IM_ARRAYSIZE(SaveFileName));
    UserData = NULL;

    _FrameNo = 0;
    _CaptureRect = ImRect();
    _CaptureState = ImGuiCaptureToolState_None;
    _ChunkNo = 0;
    _Window = NULL;
    _WindowNameMaxPosX = 170.0f;
    _WindowBackupRect = ImRect();
}

void ImGuiCaptureTool::CaptureWindowStart(ImGuiWindow* window)
{
    IM_ASSERT(window != NULL);
    _CaptureState = ImGuiCaptureToolState_CapturingSingleWIndow;
    _Window = window;
    _ChunkNo = _FrameNo = 0;
}


// Returns true when capture is in progress.
bool ImGuiCaptureTool::CaptureWindowUpdate()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    IM_ASSERT(ScreenCaptureFunc != NULL);
    IM_ASSERT(SaveFileName != NULL);
    IM_ASSERT(_Window != NULL);
    IM_ASSERT(_CaptureState == ImGuiCaptureToolState_CapturingSingleWIndow);

    // Hide other windows so they can't be seen visible behind captured window
    for (ImGuiWindow* w : g.Windows)
    {
        if (w == _Window || (w->Flags & ImGuiWindowFlags_ChildWindow))
            continue;
        w->Hidden = true;
        w->HiddenFramesCannotSkipItems = 2;
    }

    if (_FrameNo == 0)
    {
        // Initialize capture state.
        _ChunkNo = 0;
        _WindowBackupRect = _Window->Rect();
        ImGui::FocusWindow(_Window);
        ImGui::SetWindowCollapsed(_Window, false);
        ImGui::SetWindowPos(_Window, ImVec2(Padding, Padding)); // FIXME-VIEWPORTS

        _CaptureRect.Min = ImVec2(0, 0);
        _CaptureRect.Max = ImVec2(Padding, Padding) * 2;
        if (Flags & ImGuiCaptureToolFlags_StitchFullContents)
        {
            // Resize window to it's contents and capture it's entire width/height.
            ImVec2 full_size(
                ImMax(_Window->SizeFull.x, _Window->ContentSize.x + _Window->WindowPadding.y * 2),
                ImMax(_Window->SizeFull.y, _Window->ContentSize.y + _Window->WindowPadding.y * 2 + _Window->TitleBarHeight() + _Window->MenuBarHeight()));
            _CaptureRect.Max += full_size;
            ImGui::SetWindowSize(_Window, full_size);
        }
        else
        {
            _CaptureRect.Max += _Window->Size;
        }

        _Output.CreateEmpty((int)_CaptureRect.GetWidth(), (int)_CaptureRect.GetHeight());

        if (Flags & ImGuiCaptureToolFlags_StitchFullContents)
            _CaptureRect.Max.y = io.DisplaySize.y;
    }
    else if ((_FrameNo % 3) == 0)
    {
        // Capture a portion of image. Capturing of windows wider than viewport is not implemented yet.
        const int x1 = (int)_CaptureRect.Min.x;
        const int y1 = (int)_CaptureRect.Min.y;
        const int w = (int)_CaptureRect.GetWidth();
        const int h = (int)ImMin(_Output.Height - _ChunkNo * _CaptureRect.GetHeight(), _CaptureRect.GetHeight());
        if (h > 0)
        {
            if (!ScreenCaptureFunc(x1, y1, w, h,
                &_Output.Data[_ChunkNo * (int)_CaptureRect.GetWidth() * (int)_CaptureRect.GetHeight()], UserData))
                return false;
            _ChunkNo++;

            // Window moves up in order to expose it's lower part.
            _Window->Pos.y -= h;

            if (_Window->Pos.y + _Window->Size.y < Padding)
            {
                // A fix for edge case where ImGui tries to keep window visible (see ClampWindowRect), but
                // we need to take a snapshot of few remaining pixels of padding. And so we cheat. Window
                // is moved to the very bottom of the screen in order to avoid covering padding space at
                // the top of the screen. Next iteration will take a snapshot of remaining padding at the
                // top and screenshot taking process will stop.
                _Window->Pos.y = _CaptureRect.GetHeight();
            }
        }
        else
        {
            // Save file
            _Output.RemoveAlpha();
            _Output.SaveFile(SaveFileName);
            _Output.Clear();

            // Restore window position
            ImGui::SetWindowPos(_Window, _WindowBackupRect.Min, ImGuiCond_Always);
            ImGui::SetWindowSize(_Window, _WindowBackupRect.GetSize(), ImGuiCond_Always);

            _CaptureState = ImGuiCaptureToolState_None;
            _Window = NULL;
            _FrameNo = _ChunkNo = 0;

            return false;
        }
    }

    // Keep going
    _FrameNo++;
    return true;
}

// Capturing a rectangle is simpler than capture a window, because we don't need to be doing any stiching.
void ImGuiCaptureTool::CaptureRect(const ImRect& rect)
{
    IM_ASSERT(ScreenCaptureFunc != NULL);
    IM_ASSERT(SaveFileName != NULL);
    IM_ASSERT(!rect.IsInverted());
    IM_ASSERT(rect.GetWidth() < 100000 && rect.GetHeight() < 100000);

    _Output.CreateEmpty((int)rect.GetWidth(), (int)rect.GetHeight());
    if (ScreenCaptureFunc((int)rect.Min.x, (int)rect.Min.y, (int)rect.GetWidth(), (int)rect.GetHeight(), _Output.Data, UserData))
    {
        _Output.RemoveAlpha();
        _Output.SaveFile(SaveFileName);
        _Output.Clear();
    }
}

void ImGuiCaptureTool::CaptureWindowPicker(const char* title)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    IM_ASSERT(ScreenCaptureFunc != NULL);
    IM_ASSERT(SaveFileName != NULL);

    ImGuiID picking_id = ImGui::GetID("##picking");
    if (ImGui::Button(title))
        _CaptureState = ImGuiCaptureToolState_PickingSingleWindow;

    if (_CaptureState != ImGuiCaptureToolState_PickingSingleWindow)
    {
        if (ImGui::GetActiveID() == picking_id)
            ImGui::ClearActiveID();
        return;
    }

    // Picking a window
    ImDrawList* fg_draw_list = ImGui::GetForegroundDrawList();
    ImGui::SetActiveID(picking_id, g.CurrentWindow);    // Steal active ID so our click won't interact with something else.
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    ImGuiWindow* capture_window = g.HoveredRootWindow;
    if (capture_window)
    {
        // Draw rect that is about to be captured
        ImRect r = capture_window->Rect();
        r.Expand(Padding);
        r.ClipWith(ImRect(ImVec2(0, 0), io.DisplaySize));
        r.Expand(1.0f);
        fg_draw_list->AddRect(r.Min, r.Max, IM_COL32_WHITE, 0.0f, ~0, 2.0f);
    }

    ImGui::SetTooltip("Capture window: %s\nPress ESC to cancel.", capture_window ? capture_window->Name : "<None>");
    if (ImGui::IsMouseClicked(0))
    {
        CaptureWindowStart(capture_window);
    }
    else if (ImGui::IsKeyPressedMap(ImGuiKey_Escape))
    {
        _CaptureState = ImGuiCaptureToolState_None;
    }
}

void ImGuiCaptureTool::CaptureWindowsSelector(const char* title)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    // Capture Button
    bool do_capture = ImGui::Button(title);
    do_capture |= io.KeyAlt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C));
    if (_CaptureState == ImGuiCaptureToolState_SelectRectUpdate && !ImGui::IsMouseDown(0))
    {
        // Exit rect-capture even if selection is invalid and capture does not execute.
        _CaptureState = ImGuiCaptureToolState_None;
        do_capture = true;
    }

    if (ImGui::Button("Select Rect"))
        _CaptureState = ImGuiCaptureToolState_SelectRectStart;
    if (_CaptureState == ImGuiCaptureToolState_SelectRectStart || _CaptureState == ImGuiCaptureToolState_SelectRectUpdate)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Select multiple windows by pressing left mouse button and dragging.");
    }
    ImGui::Separator();

    // Show window list and update rectangles
    ImRect select_rect;
    if (_CaptureState == ImGuiCaptureToolState_SelectRectStart && ImGui::IsMouseDown(0))
    {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        {
            _CaptureState = ImGuiCaptureToolState_None;
        }
        else
        {
            _CaptureRect.Min = io.MousePos;
            _CaptureState = ImGuiCaptureToolState_SelectRectUpdate;
        }
    }
    else if (_CaptureState == ImGuiCaptureToolState_SelectRectUpdate)
    {
        // Avoid inverted-rect issue
        select_rect.Min = ImMin(_CaptureRect.Min, io.MousePos);
        select_rect.Max = ImMax(_CaptureRect.Min, io.MousePos);
    }

    float max_window_name_x = 0.0f;
    int capture_windows_count = 0;
    ImRect capture_rect;
    ImGui::Text("Windows:");
    for (ImGuiWindow* window : g.Windows)
    {
        if (!window->WasActive)
            continue;

        if (Flags & ImGuiCaptureToolFlags_IgnoreCaptureToolWindow && window == g.CurrentWindow)
        {
            // Skip
            if (_CaptureState == ImGuiCaptureToolState_MultipleWindows)
            {
                window->Hidden = true;
                window->HiddenFramesCannotSkipItems = 2;
            }
            continue;
        }

        if (Flags & ImGuiCaptureToolFlags_ExpandToIncludePopups && ((window->Flags & ImGuiWindowFlags_Popup) || (window->Flags & ImGuiWindowFlags_Tooltip)))
        {
            capture_rect.Add(window->Rect());
            continue;
        }

        if (window->Flags & ImGuiWindowFlags_ChildWindow)
            continue;

        ImGui::PushID(window);
        bool* p_selected = (bool*)g.CurrentWindow->StateStorage.GetIntRef(window->RootWindow->ID, 0);

        if (_CaptureState == ImGuiCaptureToolState_SelectRectUpdate)
            *p_selected = select_rect.Contains(window->Rect());

        // Ensure that text after the ## is actually displayed to the user (FIXME: won't be able to check/uncheck from it)
        ImGui::Checkbox(window->Name, p_selected);
        if (const char* remaining_text = ImGui::FindRenderedTextEnd(window->Name))
            if (remaining_text[0] != 0)
            {
                ImGui::SameLine(0, 1);
                ImGui::TextUnformatted(remaining_text);
            }

        max_window_name_x = ImMax(max_window_name_x, g.CurrentWindow->DC.CursorPosPrevLine.x - g.CurrentWindow->Pos.x);
        if (*p_selected)
        {
            capture_rect.Add(window->Rect());
            capture_windows_count++;
        }
        ImGui::SameLine(_WindowNameMaxPosX + g.Style.ItemSpacing.x);
        ImGui::SetNextItemWidth(100);
        ImGui::DragFloat2("Pos", &window->Pos.x, 0.05f, 0.0f, 0.0f, "%.0f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::DragFloat2("Size", &window->SizeFull.x, 0.05f, 0.0f, 0.0f, "%.0f");
        ImGui::PopID();

        if (_CaptureState == ImGuiCaptureToolState_MultipleWindows)
        {
            if (!*p_selected)
            {
                // Hide windows that are not selected during capture.
                window->Hidden = true;
                window->HiddenFramesCannotSkipItems = 2;
            }
        }
    }
    _WindowNameMaxPosX = max_window_name_x;

    // Draw capture rectangle
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    const bool can_capture = capture_rect.Min.x != FLT_MIN && capture_rect.Min.y != FLT_MIN &&
        capture_rect.Max.x != FLT_MAX && capture_rect.Max.y != FLT_MAX && capture_windows_count > 0;
    if (can_capture && _CaptureState != ImGuiCaptureToolState_PickingSingleWindow)
    {
        IM_ASSERT(capture_rect.GetWidth() > 0);
        IM_ASSERT(capture_rect.GetHeight() > 0);
        capture_rect.Expand(Padding);
        capture_rect.ClipWith(ImRect(ImVec2(0, 0), io.DisplaySize));
        draw_list->AddRect(capture_rect.Min - ImVec2(1.0f, 1.0f), capture_rect.Max + ImVec2(1.0f, 1.0f), IM_COL32_WHITE);
    }

    if (_CaptureState == ImGuiCaptureToolState_SelectRectUpdate)
        draw_list->AddRect(select_rect.Min - ImVec2(1.0f, 1.0f), select_rect.Max + ImVec2(1.0f, 1.0f), IM_COL32_WHITE);

    // Process capture
    if (can_capture && do_capture)
    {
        _FrameNo = 0;
        _CaptureState = ImGuiCaptureToolState_MultipleWindows;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Alternatively press Alt+C to capture selection.");

    if (_CaptureState == ImGuiCaptureToolState_MultipleWindows)
    {
        if (_FrameNo < 3)
        {
            // Render for few frames to allow windows to get hidden.
            _FrameNo++;
        }
        else
        {
            CaptureRect(capture_rect);
            _CaptureState = ImGuiCaptureToolState_None;
        }
    }
}

void ImGuiCaptureTool::ShowCaptureToolWindow(bool* p_open)
{
    if (_CaptureState == ImGuiCaptureToolState_CapturingSingleWIndow)
        CaptureWindowUpdate();

    if (!ImGui::Begin("Dear ImGui Capture Tool", p_open))
    {
        ImGui::End();
        return;
    }

    if (ScreenCaptureFunc == NULL)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Back-end is missing ScreenCaptureFunc!");
        ImGui::End();
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Options"))
    {
        ImGui::PushItemWidth(-200.0f);
        ImGui::InputText("##", SaveFileName, IM_ARRAYSIZE(SaveFileName));
        ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
        if (ImGui::Button("Open"))
            ImOsOpenInShell(SaveFileName);
        ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
        ImGui::TextUnformatted("Out filename");
        ImGui::DragFloat("Padding", &Padding, 0.1f, 0, 32, "%.0f");

        if (ImGui::Button("Snap Windows To Grid", ImVec2(-200, 0)))
            SnapWindowsToGrid(SnapGridSize);
        ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(50.0f);
        ImGui::DragFloat("##SnapGridSize", &SnapGridSize, 1.0f, 1.0f, 128.0f, "%.0f");

        ImGui::Checkbox("Software Mouse Cursor", &io.MouseDrawCursor);  // FIXME-TESTS: Test engine always resets this value.
        ImGui::CheckboxFlags("Stitch and capture full contents height", &Flags, ImGuiCaptureToolFlags_StitchFullContents);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Full height of picked window will be captured.");
        ImGui::CheckboxFlags("Include tooltips", &Flags, ImGuiCaptureToolFlags_ExpandToIncludePopups);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Capture area will be expanded to include visible tooltips.");

        ImGui::PopItemWidth();
        ImGui::TreePop();
    }

    ImGui::Separator();

    Flags |= ImGuiCaptureToolFlags_IgnoreCaptureToolWindow;

    CaptureWindowPicker("Capture Window..");
    CaptureWindowsSelector("Capture Selected");

    ImGui::Separator();

    ImGui::End();
}

// Move/resize all windows so they are neatly aligned on a grid
// This is an easy way of ensuring some form of alignment without specifying detailed constraints.
void ImGuiCaptureTool::SnapWindowsToGrid(float cell_size)
{
    ImGuiContext& g = *GImGui;

    for (ImGuiWindow* window : g.Windows)
    {
        if (!window->WasActive)
            continue;

        if (window->Flags & ImGuiWindowFlags_ChildWindow)
            continue;

        if ((window->Flags & ImGuiWindowFlags_Popup) || (window->Flags & ImGuiWindowFlags_Tooltip))
            continue;

        ImRect rect = window->Rect();
        rect.Min.x = ImFloor(rect.Min.x / cell_size) * cell_size;
        rect.Min.y = ImFloor(rect.Min.y / cell_size) * cell_size;
        rect.Max.x = ImFloor(rect.Max.x / cell_size) * cell_size;
        rect.Max.y = ImFloor(rect.Max.y / cell_size) * cell_size;
        rect.Min += ImVec2(Padding, Padding);
        rect.Max += ImVec2(Padding, Padding);
        ImGui::SetWindowPos(window, rect.Min);
        ImGui::SetWindowSize(window, rect.GetSize());
    }
}
