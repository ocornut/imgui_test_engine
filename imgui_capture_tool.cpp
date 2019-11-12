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

// Returns true when capture is in progress.
bool ImGuiCaptureTool::_CaptureUpdate()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    if (_Frame == 0)
    {
        // Initialize capture state.
        _Chunk = 0;
        _Window->Collapsed = false;
        _WindowState = _Window->Rect();
        ImGui::SetWindowPos(_Window, ImVec2(Padding, Padding));

        _CaptureRect.Min = ImVec2(0, 0);
        _CaptureRect.Max = ImVec2(Padding, Padding) * 2;
        if (Flags & ImGuiCaptureWindowFlags_FullSize)
        {
            // Resize window to it's contents and capture it's entire width/height.
            ImVec2 full_size(
                ImMax(_Window->SizeFull.x, _Window->ContentSize.x + _Window->WindowPadding.y * 2),
                ImMax(_Window->SizeFull.y, _Window->ContentSize.y + _Window->WindowPadding.y * 2 + _Window->TitleBarHeight() + _Window->MenuBarHeight()));
            _CaptureRect.Max += full_size;
            ImGui::SetWindowSize(_Window, full_size);
        }
        else
            _CaptureRect.Max += _Window->Size;

        _Output.CreateEmpty((int)_CaptureRect.GetWidth(), (int)_CaptureRect.GetHeight());

        if (Flags & ImGuiCaptureWindowFlags_FullSize)
            _CaptureRect.Max.y = io.DisplaySize.y;
    }
    else if ((_Frame % 6) == 0)
    {
        // Capture a portion of image. Capturing of windows wider than viewport is not implemented yet.
        int h = (int)ImMin(_Output.Height - _Chunk * _CaptureRect.GetHeight(), _CaptureRect.GetHeight());
        if (h > 0)
        {
            if (!ScreenCaptureFunc((int)_CaptureRect.Min.x, (int)_CaptureRect.Min.y, (int)_CaptureRect.GetWidth(), h,
                &_Output.Data[_Chunk * (int)_CaptureRect.GetWidth() * (int)_CaptureRect.GetHeight()], UserData))
                return false;
            _Chunk++;

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
            _Output.RemoveAlpha();
            _Output.SaveFile(SaveFileName);
            _Output.Clear();
            ImGui::SetWindowPos(_Window, _WindowState.Min, ImGuiCond_Always);
            ImGui::SetWindowSize(_Window, _WindowState.GetSize(), ImGuiCond_Always);
            return false;
        }
    }

    _Frame++;
    return true;
}

bool ImGuiCaptureTool::CaptureWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(ScreenCaptureFunc != NULL);
    IM_ASSERT(SaveFileName != NULL);
    IM_ASSERT(window != NULL);

    _Window = window;
    ImGui::FocusWindow(window);

    // Hide other windows so they cant be seen visible behind captured window
    for (ImGuiWindow* w : g.Windows)
    {
        if (w == _Window || window->Flags & ImGuiWindowFlags_ChildWindow)
            continue;
        w->Hidden = true;
        w->HiddenFramesCannotSkipItems = 2;
    }

    const bool result = _CaptureUpdate();
    if (!result)
    {
        _Frame = 0;
        _Window = NULL;
    }

    return result;
}

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

    ImGui::PushID(title);
    ImGui::Button(title);
    if (ImGui::IsItemActive())
    {
        // Picking a window
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();

        // _Window is meant for _CaptureUpdate() to store window that is being captured. However it is pointless to
        // introduce another variable for window storage just for this function therefore we reuse _Window while it is
        // not in use yet.
        _Window = NULL;
        for (ImGuiWindow* w : g.Windows)
        {
            if (g.HoveredRootWindow == w)
            {
                // Draw rect that is about to be captured
                ImRect capture_rect = w->Rect();
                capture_rect.Expand(Padding);
                capture_rect.ClipWith(ImRect(ImVec2(0, 0), io.DisplaySize));
                draw_list->AddRect(
                    ImVec2(capture_rect.Min.x - 1.0f, capture_rect.Min.y - 1.0f),
                    ImVec2(capture_rect.Max.x + 1.0f, capture_rect.Max.y + 1.0f), IM_COL32_WHITE);
                _Window = w;
                break;
            }
        }
        // Render picker cross
        draw_list->AddLine(io.MousePos - ImVec2(10.f, 0.f), io.MousePos + ImVec2(10.f, 0.f), IM_COL32_WHITE);
        draw_list->AddLine(io.MousePos - ImVec2(0.f, 10.f), io.MousePos + ImVec2(0.f, 10.f), IM_COL32_WHITE);
        g.MouseCursor = ImGuiMouseCursor_None;
    }
    else if (_Window != NULL)
    {
        // Window was picked, capture it.
        g.MouseCursor = ImGuiMouseCursor_Arrow;
        if (!CaptureWindow(_Window))
            _Window = NULL;
    }
    ImGui::PopID();
}

void ImGuiCaptureTool::CaptureWindowsSelector(const char* title)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    // Capture Button
    bool do_capture = ImGui::Button(title);
    do_capture |= io.KeyAlt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C));
    if (_CaptureType == ImGuiCaptureType_SelectRectUpdate && !ImGui::IsMouseDown(0))
    {
        // Exit rect-capture even if selection is invalid and capture does not execute.
        _CaptureType = ImGuiCaptureType_None;
        do_capture = true;
    }

    if (ImGui::Button("Select Rect"))
        _CaptureType = ImGuiCaptureType_SelectRectStart;
    if (_CaptureType == ImGuiCaptureType_SelectRectStart || _CaptureType == ImGuiCaptureType_SelectRectUpdate)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImGui::SetTooltip("Select multiple windows by pressing left mouse button and dragging.");
    }
    ImGui::Separator();

    // Show window list and update rectangles
    ImRect select_rect;
    if (_CaptureType == ImGuiCaptureType_SelectRectStart && ImGui::IsMouseDown(0))
    {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        {
            _CaptureType = ImGuiCaptureType_None;
        }
        else
        {
            _CaptureRect.Min = io.MousePos;
            _CaptureType = ImGuiCaptureType_SelectRectUpdate;
        }
    }
    else if (_CaptureType == ImGuiCaptureType_SelectRectUpdate)
    {
        // Avoid inverted-rect issue
        select_rect.Min = ImMin(_CaptureRect.Min, io.MousePos);
        select_rect.Max = ImMax(_CaptureRect.Min, io.MousePos);
    }

    float max_window_name_x = 0.0f;
    ImRect capture_rect;
    ImGui::Text("Windows:");
    for (ImGuiWindow* window : g.Windows)
    {
        if (!window->WasActive)
            continue;

        if (Flags & ImGuiCaptureWindowFlags_IgnoreCurrentWindow && window == g.CurrentWindow)
        {
            // Skip
            if (_CaptureType == ImGuiCaptureType_MultipleWindows)
            {
                window->Hidden = true;
                window->HiddenFramesCannotSkipItems = 1;
            }
            continue;
        }

        if ((window->Flags & ImGuiWindowFlags_Popup) || (window->Flags & ImGuiWindowFlags_Tooltip))
        {
            capture_rect.Add(window->Rect());
            continue;
        }

        if (window->Flags & ImGuiWindowFlags_ChildWindow)
            continue;

        ImGui::PushID(window);
        bool* p_selected = (bool*)g.CurrentWindow->StateStorage.GetIntRef(window->RootWindow->ID, 0);

        if (_CaptureType == ImGuiCaptureType_SelectRectUpdate)
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
            capture_rect.Add(window->Rect());
        ImGui::SameLine(_WindowNameMaxPosX + g.Style.ItemSpacing.x);
        ImGui::SetNextItemWidth(100);
        ImGui::DragFloat2("Pos", &window->Pos.x, 0.05f, 0.0f, 0.0f, "%.0f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::DragFloat2("Size", &window->SizeFull.x, 0.05f, 0.0f, 0.0f, "%.0f");
        ImGui::PopID();

        if (_CaptureType == ImGuiCaptureType_MultipleWindows)
        {
            if (!*p_selected)
            {
                // Hide windows that are not selected during capture.
                window->Hidden = true;
                window->HiddenFramesCannotSkipItems = 1;
            }
        }
    }
    _WindowNameMaxPosX = max_window_name_x;

    // Draw capture rectangle
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    const bool can_capture = capture_rect.GetWidth() > 0 && capture_rect.GetHeight() > 0;
    if (can_capture)
    {
        capture_rect.Expand(Padding);
        capture_rect.ClipWith(ImRect(ImVec2(0, 0), io.DisplaySize));
        draw_list->AddRect(capture_rect.Min - ImVec2(1.0f, 1.0f), capture_rect.Max + ImVec2(1.0f, 1.0f), IM_COL32_WHITE);
    }

    if (_CaptureType == ImGuiCaptureType_SelectRectUpdate)
        draw_list->AddRect(select_rect.Min - ImVec2(1.0f, 1.0f), select_rect.Max + ImVec2(1.0f, 1.0f), IM_COL32_WHITE);

    // Process capture
    if (can_capture && do_capture)
    {
        _Frame = 0;
        _CaptureType = ImGuiCaptureType_MultipleWindows;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Alternatively press Alt+C to capture selection.");

    if (_CaptureType == ImGuiCaptureType_MultipleWindows)
    {
        if (_Frame < 3)
        {
            // Render for few frames to allow windows to get hidden.
            _Frame++;
        }
        else
        {
            CaptureRect(capture_rect);
            _CaptureType = ImGuiCaptureType_None;
        }
    }
}

void ImGuiCaptureTool::ShowCaptureToolWindow(bool* p_open)
{
    if (!ImGui::Begin("Dear ImGui Capture Tool", p_open))
    {
        ImGui::End();
        return;
    }

    if (ScreenCaptureFunc == NULL)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Back-end is missing ScreenCaptureFunc!");
        ImGui::End();
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
        ImGui::CheckboxFlags("Full Height", &Flags, ImGuiCaptureWindowFlags_FullSize);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Full height of picked window will be captured.");

        ImGui::PopItemWidth();
        ImGui::TreePop();
    }

    ImGui::Separator();

    Flags |= ImGuiCaptureWindowFlags_IgnoreCurrentWindow;

    CaptureWindowPicker("Capture Window");
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
