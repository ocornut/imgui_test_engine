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
#include <stb/stb_image_write.h>
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

bool ImageBuf::SaveFile(const char* filename)
{
    int ret = stbi_write_png(filename, Width, Height, 4, Data, Width * 4);
    return ret != 0;
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

void ImageBuf::BlitSubImage(int dst_x, int dst_y, int src_x, int src_y, int w, int h, const ImageBuf* source)
{
    IM_ASSERT(source && "Source image is null.");
    IM_ASSERT(dst_x >= 0 && dst_y >= 0 && "Destination coordinates can not be negative.");
    IM_ASSERT(src_x >= 0 && src_y >= 0 && "Source coordinates can not be negative.");
    IM_ASSERT(dst_x + w <= Width && dst_y + h <= Height && "Destination image is too small.");
    IM_ASSERT(src_x + w <= source->Width && src_y + h <= source->Height && "Source image is too small.");

    for (int y = 0; y < h; y++)
        memcpy(&Data[(dst_y + y) * Width + dst_x], &source->Data[(src_y + y) * source->Width + src_x], source->Width * 4);
}

//-----------------------------------------------------------------------------
// ImGuiCaptureContext
//-----------------------------------------------------------------------------

// Returns true when capture is in progress.
bool ImGuiCaptureContext::CaptureScreenshot(ImGuiCaptureArgs* args)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    IM_ASSERT(args != NULL);
    IM_ASSERT(ScreenCaptureFunc != NULL);
    IM_ASSERT(args->OutImageBuf != NULL || args->OutImageFileTemplate[0]);

    ImageBuf* output = args->OutImageBuf ? args->OutImageBuf : &_Output;

    // Hide other windows so they can't be seen visible behind captured window
    for (ImGuiWindow* window : g.Windows)
    {
#ifdef IMGUI_HAS_VIEWPORT
        if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) && (args->InFlags & ImGuiCaptureToolFlags_StitchFullContents))
        {
            // FIXME-VIEWPORTS: Content stitching is not possible because window would get moved out of main viewport and detach from it. We need a way to force captured windows to remain in main viewport here.
            return false;
        }
#endif
        if (window->Flags & ImGuiWindowFlags_ChildWindow || args->InCaptureWindows.contains(window))
            continue;
        window->Hidden = true;
        window->HiddenFramesCannotSkipItems = 2;
    }

    if (_FrameNo == 0)
    {
        // Initialize capture state.
        _ChunkNo = 0;
        _CaptureRect = ImRect(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
        _WindowBackupRects.clear();
        _CombinedWindowRectPos = ImVec2(FLT_MAX, FLT_MAX);
        _DisplayWindowPaddingBackup = g.Style.DisplayWindowPadding;
        _DisplaySafeAreaPaddingBackup = g.Style.DisplaySafeAreaPadding;
        g.Style.DisplayWindowPadding = ImVec2(0, 0);                    // Allow window to be positioned fully outside
        g.Style.DisplaySafeAreaPadding = ImVec2(0, 0);                  // of visible viewport.
        args->_Capturing = true;

        bool is_capturing_rect = args->InCaptureRect.GetWidth() > 0 && args->InCaptureRect.GetHeight() > 0;
        if (is_capturing_rect)
        {
            // Capture arbitrary rectangle. If any windows are specified in this mode only they will appear in captured region.
            _CaptureRect = args->InCaptureRect;
            if (args->InCaptureWindows.empty())
            {
                // Gather all top level windows. We will need to move them in order to capture regions larger than viewport.
                for (ImGuiWindow* window : g.Windows)
                {
                    // Child windows will be included by their parents.
                    if (window->ParentWindow != NULL)
                        continue;

                    if ((window->Flags & ImGuiWindowFlags_Popup || window->Flags & ImGuiWindowFlags_Tooltip) && !(args->InFlags & ImGuiCaptureToolFlags_ExpandToIncludePopups))
                        continue;

                    args->InCaptureWindows.push_back(window);
                }
            }
        }

        // Save rectangle covering all windows and find top-left corner of combined rect which will be used to
        // translate this group of windows to top-left corner of the screen.
        for (ImGuiWindow* window : args->InCaptureWindows)
        {
            _WindowBackupRects.push_back(window->Rect());
            _WindowBackupRectsWindows.push_back(window);
            _CombinedWindowRectPos = ImVec2(ImMin(_CombinedWindowRectPos.x, window->Pos.x), ImMin(_CombinedWindowRectPos.y, window->Pos.y));
        }

        if (args->InFlags & ImGuiCaptureToolFlags_StitchFullContents)
        {
            if (is_capturing_rect)
                ImGui::LogText("Capture Tool: capture of full window contents is not possible when capturing specified rect.");
            else if (args->InCaptureWindows.size() != 1)
                ImGui::LogText("Capture Tool: capture of full window contents is not possible when capturing more than one window.");
            else
            {
                // Resize window to it's contents and capture it's entire width/height. However if window is bigger than
                // it's contents - keep original size.
                ImVec2 full_size;
                ImGuiWindow* window = args->InCaptureWindows.front();
                full_size.x = ImMax(window->SizeFull.x, window->ContentSize.x + window->WindowPadding.y * 2);
                full_size.y = ImMax(window->SizeFull.y, window->ContentSize.y + window->WindowPadding.y * 2 + window->TitleBarHeight() + window->MenuBarHeight());
                ImGui::SetWindowSize(window, full_size);
            }
        }
    }
    else if (_FrameNo == 1)
    {
        // Move group of windows so combined rectangle position is at the top-left corner + padding and create combined
        // capture rect of entire area that will be saved to screenshot. Doing this on the second frame because when
        // ImGuiCaptureToolFlags_StitchFullContents flag is used we need to allow window to reposition.
        ImVec2 move_offset = ImVec2(args->InPadding, args->InPadding) - _CombinedWindowRectPos;
#ifdef IMGUI_HAS_VIEWPORT
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            move_offset += main_viewport->Pos;
        }
#endif
        for (ImGuiWindow* window : args->InCaptureWindows)
        {
            // Repositioning of a window may take multiple frames, depending on whether window was already rendered or not.
            ImGui::SetWindowPos(window, window->Pos + move_offset);
            _CaptureRect.Add(window->Rect());
        }

        // Include padding in capture.
        _CaptureRect.Expand(args->InPadding);

        // Initialize capture buffer.
        output->CreateEmpty((int)_CaptureRect.GetWidth(), (int)_CaptureRect.GetHeight());
    }
    else if ((_FrameNo % 4) == 0)
    {
        // FIXME: Implement capture of regions wider than viewport.
        // Capture a portion of image. Capturing of windows wider than viewport is not implemented yet.
        ImRect capture_rect = _CaptureRect;
        ImRect clip_rect(ImVec2(0, 0), io.DisplaySize);
#ifdef IMGUI_HAS_VIEWPORT
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            clip_rect = ImRect(main_viewport->Pos, main_viewport->Pos + main_viewport->Size);
        }
#endif
        capture_rect.ClipWith(clip_rect);
        const int capture_height = ImMin((int)io.DisplaySize.y, (int)_CaptureRect.GetHeight());
        const int x1 = (int)(capture_rect.Min.x - clip_rect.Min.x);
        const int y1 = (int)(capture_rect.Min.y - clip_rect.Min.y);
        const int w = (int)capture_rect.GetWidth();
        const int h = (int)ImMin(output->Height - _ChunkNo * capture_height, capture_height);
        if (h > 0)
        {
            if (!ScreenCaptureFunc(x1, y1, w, h, &output->Data[_ChunkNo * w * capture_height], UserData))
                return false;
            _ChunkNo++;

            // Window moves up in order to expose it's lower part.
            for (ImGuiWindow* window : args->InCaptureWindows)
                ImGui::SetWindowPos(window, window->Pos - ImVec2(0, (float)h));
            _CaptureRect.TranslateY(-(float)h);
        }
        else
        {
            output->RemoveAlpha();

            if (args->OutImageBuf == NULL)
            {
                // Save file only if custom buffer was not specified.
                int file_name_size = IM_ARRAYSIZE(_SaveFileNameFinal);
                if (ImFormatString(_SaveFileNameFinal, file_name_size, args->OutImageFileTemplate, args->OutFileCounter + 1) >= file_name_size)
                {
                    ImGui::LogText("Capture Tool: file name is too long.");
                }
                else
                {
                    ImPathFixSeparatorsForCurrentOS(_SaveFileNameFinal);
                    if (!ImFileCreateDirectoryChain(_SaveFileNameFinal, ImPathFindFilename(_SaveFileNameFinal)))
                    {
                        ImGui::LogText("Capture Tool: unable to create directory for file '%s'.", _SaveFileNameFinal);
                    }
                    else
                    {
                        args->OutFileCounter++;
                        output->SaveFile(_SaveFileNameFinal);
                    }
                }
                output->Clear();
            }

            // Restore window position
            for (int i = 0; i < _WindowBackupRects.size(); i++)
            {
                ImGuiWindow* window = _WindowBackupRectsWindows[i];
                if (window->Hidden)
                    continue;

                ImRect rect = _WindowBackupRects[i];
                ImGui::SetWindowPos(window, rect.Min, ImGuiCond_Always);
                ImGui::SetWindowSize(window, rect.GetSize(), ImGuiCond_Always);
            }

            _FrameNo = _ChunkNo = 0;
            g.Style.DisplayWindowPadding = _DisplayWindowPaddingBackup;
            g.Style.DisplaySafeAreaPadding = _DisplaySafeAreaPaddingBackup;
            args->_Capturing = false;
            args->InCaptureWindows.clear(); // FIXME-TESTS: Why clearing this? aka why isn't args a read-only structure
            args->InCaptureRect = ImRect(); // FIXME-TESTS: "
            return false;
        }
    }

    // Keep going
    _FrameNo++;
    return true;
}

//-----------------------------------------------------------------------------
// ImGuiCaptureTool
//-----------------------------------------------------------------------------

ImGuiCaptureTool::ImGuiCaptureTool(ImGuiScreenCaptureFunc capture_func)
{
    Context.ScreenCaptureFunc = capture_func;
    ImStrncpy(SaveFileName, "captures/imgui_capture_%04d.png", (size_t)IM_ARRAYSIZE(SaveFileName));
}

void ImGuiCaptureTool::CaptureWindowPicker(const char* title, ImGuiCaptureArgs* args)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    if (_CaptureState == ImGuiCaptureToolState_Capturing && args->_Capturing)
    {
        if (ImGui::IsKeyPressedMap(ImGuiKey_Escape) || !Context.CaptureScreenshot(args))
            _CaptureState = ImGuiCaptureToolState_None;
    }

    const ImVec2 button_sz = ImVec2(ImGui::CalcTextSize("M").x * 30, 0.0f);
    ImGuiID picking_id = ImGui::GetID("##picking");
    if (ImGui::Button(title, button_sz))
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
        if (Flags & ImGuiCaptureToolFlags_IgnoreCaptureToolWindow && capture_window == ImGui::GetCurrentWindow())
            return;

        // Draw rect that is about to be captured
        ImRect r = capture_window->Rect();
        r.Expand(args->InPadding);
        r.ClipWith(ImRect(ImVec2(0, 0), io.DisplaySize));
        r.Expand(1.0f);
        fg_draw_list->AddRect(r.Min, r.Max, IM_COL32_WHITE, 0.0f, ~0, 2.0f);
    }

    ImGui::SetTooltip("Capture window: %s\nPress ESC to cancel.", capture_window ? capture_window->Name : "<None>");
    if (ImGui::IsMouseClicked(0))
    {
        args->InCaptureWindows.clear();
        args->InCaptureWindows.push_back(capture_window);
        _CaptureState = ImGuiCaptureToolState_Capturing;
        // We cheat a little. args->_Capturing is set to true when Capture.CaptureScreenshot(args), but we use this
        // field to differentiate which capture is in progress (windows picker or selector), therefore we set it to true
        // in advance and execute Capture.CaptureScreenshot(args) only when args->_Capturing is true.
        args->_Capturing = true;
    }
}

void ImGuiCaptureTool::CaptureWindowsSelector(const char* title, ImGuiCaptureArgs* args)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImVec2 button_sz = ImVec2(ImGui::CalcTextSize("M").x * 30, 0.0f);

    // Capture Button
    bool do_capture = ImGui::Button(title, button_sz);
    do_capture |= io.KeyAlt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C));
    if (_CaptureState == ImGuiCaptureToolState_SelectRectUpdate && !ImGui::IsMouseDown(0))
    {
        // Exit rect-capture even if selection is invalid and capture does not execute.
        _CaptureState = ImGuiCaptureToolState_None;
        do_capture = true;
    }

    if (ImGui::Button("Rect-Select Windows", button_sz))
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
            _CaptureState = ImGuiCaptureToolState_None;
        else
        {
            args->InCaptureRect.Min = io.MousePos;
            _CaptureState = ImGuiCaptureToolState_SelectRectUpdate;
        }
    }
    else if (_CaptureState == ImGuiCaptureToolState_SelectRectUpdate)
    {
        // Avoid inverted-rect issue
        select_rect.Min = ImMin(args->InCaptureRect.Min, io.MousePos);
        select_rect.Max = ImMax(args->InCaptureRect.Min, io.MousePos);
    }

    args->InCaptureWindows.clear();

    float max_window_name_x = 0.0f;
    ImRect capture_rect(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    ImGui::Text("Windows:");
    for (ImGuiWindow* window : g.Windows)
    {
        if (!window->WasActive)
            continue;

        if (args->InFlags & ImGuiCaptureToolFlags_ExpandToIncludePopups && ((window->Flags & ImGuiWindowFlags_Popup) || (window->Flags & ImGuiWindowFlags_Tooltip)))
        {
            capture_rect.Add(window->Rect());
            args->InCaptureWindows.push_back(window);
            continue;
        }

        if (window->Flags & ImGuiWindowFlags_ChildWindow)
            continue;

        if (args->InFlags & ImGuiCaptureToolFlags_IgnoreCaptureToolWindow && window == ImGui::GetCurrentWindow())
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
            args->InCaptureWindows.push_back(window);
        }
        ImGui::SameLine(_WindowNameMaxPosX + g.Style.ItemSpacing.x);
        ImGui::SetNextItemWidth(100);
        ImGui::DragFloat2("Pos", &window->Pos.x, 0.05f, 0.0f, 0.0f, "%.0f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::DragFloat2("Size", &window->SizeFull.x, 0.05f, 0.0f, 0.0f, "%.0f");
        ImGui::PopID();
    }
    _WindowNameMaxPosX = max_window_name_x;

    // Draw capture rectangle
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    const bool can_capture = !capture_rect.IsInverted() && !args->InCaptureWindows.empty();
    if (can_capture && (_CaptureState == ImGuiCaptureToolState_None || _CaptureState == ImGuiCaptureToolState_SelectRectUpdate))
    {
        IM_ASSERT(capture_rect.GetWidth() > 0);
        IM_ASSERT(capture_rect.GetHeight() > 0);
        capture_rect.Expand(args->InPadding);
        ImVec2 display_pos(0, 0);
        ImVec2 display_size = io.DisplaySize;
#ifdef IMGUI_HAS_VIEWPORT
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            display_pos = main_viewport->Pos;
            display_size = main_viewport->Size;
        }
#endif
        capture_rect.ClipWith(ImRect(display_pos, display_pos + display_size));
        draw_list->AddRect(capture_rect.Min - ImVec2(1.0f, 1.0f), capture_rect.Max + ImVec2(1.0f, 1.0f), IM_COL32_WHITE);
    }

    if (_CaptureState == ImGuiCaptureToolState_SelectRectUpdate)
        draw_list->AddRect(select_rect.Min - ImVec2(1.0f, 1.0f), select_rect.Max + ImVec2(1.0f, 1.0f), IM_COL32_WHITE);

    // Process capture
    if (can_capture && do_capture)
    {
        // We cheat a little. args->_Capturing is set to true when Capture.CaptureScreenshot(args), but we use this
        // field to differentiate which capture is in progress (windows picker or selector), therefore we set it to true
        // in advance and execute Capture.CaptureScreenshot(args) only when args->_Capturing is true.
        args->_Capturing = true;
        _CaptureState = ImGuiCaptureToolState_Capturing;
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Alternatively press Alt+C to capture selection.");

    if (_CaptureState == ImGuiCaptureToolState_Capturing && args->_Capturing)
    {
        if (!Context.CaptureScreenshot(args))
            _CaptureState = ImGuiCaptureToolState_None;
    }
}

void ImGuiCaptureTool::ShowCaptureToolWindow(bool* p_open)
{
    if (!ImGui::Begin("Dear ImGui Capture Tool", p_open))
    {
        ImGui::End();
        return;
    }

    if (Context.ScreenCaptureFunc == NULL)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Back-end is missing ScreenCaptureFunc!");
        ImGui::End();
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    // Options
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Options"))
    {
        ImGui::PushItemWidth(-200.0f);
        ImGui::InputText("##", SaveFileName, IM_ARRAYSIZE(SaveFileName));
        ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
        const bool has_last_file_name = (Context._SaveFileNameFinal[0] != 0);
        if (!has_last_file_name)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_Text] * ImVec4(1,1,1,0.5f));
        }
        if (ImGui::Button("Open Last") && has_last_file_name)           // FIXME-CAPTURE: Running tests changes last captured file name.
            ImOsOpenInShell(Context._SaveFileNameFinal);
        if (!has_last_file_name)
        {
            ImGui::PopStyleColor();
            ImGui::PopItemFlag();
        }
        if (has_last_file_name && ImGui::IsItemHovered())
            ImGui::SetTooltip("Open %s", Context._SaveFileNameFinal);
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
        ImGui::CheckboxFlags("Always ignore capture tool window", &Flags, ImGuiCaptureToolFlags_IgnoreCaptureToolWindow);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Full height of picked window will be captured.");
        ImGui::CheckboxFlags("Include tooltips", &Flags, ImGuiCaptureToolFlags_ExpandToIncludePopups);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Capture area will be expanded to include visible tooltips.");

        ImGui::PopItemWidth();
        ImGui::TreePop();
    }

    ImGui::Separator();

    // Ensure that use of different contexts use same file counter and don't overwrite previously created files.
    _CaptureArgsPicker.OutFileCounter = _CaptureArgsSelector.OutFileCounter = ImMax(_CaptureArgsPicker.OutFileCounter, _CaptureArgsSelector.OutFileCounter);
    // Propagate settings from UI to args.
    _CaptureArgsPicker.InPadding = _CaptureArgsSelector.InPadding = Padding;
    _CaptureArgsPicker.InFlags = _CaptureArgsSelector.InFlags = Flags;
    ImStrncpy(_CaptureArgsPicker.OutImageFileTemplate, SaveFileName, (size_t)IM_ARRAYSIZE(_CaptureArgsPicker.OutImageFileTemplate));
    ImStrncpy(_CaptureArgsSelector.OutImageFileTemplate, SaveFileName, (size_t)IM_ARRAYSIZE(_CaptureArgsSelector.OutImageFileTemplate));

    // Hide tool window unconditionally.
    if (Flags & ImGuiCaptureToolFlags_IgnoreCaptureToolWindow && _CaptureState == ImGuiCaptureToolState_Capturing)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindowRead();
        window->Hidden = true;
        window->HiddenFramesCannotSkipItems = 2;
    }

    CaptureWindowPicker("Capture Window", &_CaptureArgsPicker);
    CaptureWindowsSelector("Capture Selected", &_CaptureArgsSelector);
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
