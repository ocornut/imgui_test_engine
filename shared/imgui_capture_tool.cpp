// Dear ImGui Screen/Video Capture Tool

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_capture_tool.h"
#include "../shared/imgui_utils.h"

// stb_image_write
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4456)                             // declaration of 'xx' hides previous local declaration
#pragma warning (disable: 4457)                             // declaration of 'xx' hides function parameter
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../libs/stb/stb_image_write.h"
#define GIF_TEMP_MALLOC IM_ALLOC
#define GIF_TEMP_FREE IM_FREE
#define GIF_MALLOC IM_ALLOC
#define GIF_FREE IM_FREE
#include "../libs/gif-h/gif.h"
#ifdef _MSC_VER
#pragma warning (pop)
#else
#pragma GCC diagnostic pop
#endif

//-----------------------------------------------------------------------------
// ImGuiCaptureImageBuf
// Helper class for simple bitmap manipulation (not particularly efficient!)
//-----------------------------------------------------------------------------

void ImGuiCaptureImageBuf::Clear()
{
    if (Data)
        free(Data);
    Data = NULL;
}

void ImGuiCaptureImageBuf::CreateEmpty(int w, int h)
{
    CreateEmptyNoMemClear(w, h);
    memset(Data, 0, (size_t)(Width * Height * 4));
}

void ImGuiCaptureImageBuf::CreateEmptyNoMemClear(int w, int h)
{
    Clear();
    Width = w;
    Height = h;
    Data = (unsigned int*)malloc((size_t)(Width * Height * 4));
}

bool ImGuiCaptureImageBuf::SaveFile(const char* filename)
{
    int ret = stbi_write_png(filename, Width, Height, 4, Data, Width * 4);
    return ret != 0;
}

void ImGuiCaptureImageBuf::RemoveAlpha()
{
    unsigned int* p = Data;
    int n = Width * Height;
    while (n-- > 0)
    {
        *p |= 0xFF000000;
        p++;
    }
}

void ImGuiCaptureImageBuf::BlitSubImage(int dst_x, int dst_y, int src_x, int src_y, int w, int h, const ImGuiCaptureImageBuf* source)
{
    IM_ASSERT(source && "Source image is null.");
    IM_ASSERT(dst_x >= 0 && dst_y >= 0 && "Destination coordinates can not be negative.");
    IM_ASSERT(src_x >= 0 && src_y >= 0 && "Source coordinates can not be negative.");
    IM_ASSERT(dst_x + w <= Width && dst_y + h <= Height && "Destination image is too small.");
    IM_ASSERT(src_x + w <= source->Width && src_y + h <= source->Height && "Source image is too small.");

    for (int y = 0; y < h; y++)
        memcpy(&Data[(dst_y + y) * Width + dst_x], &source->Data[(src_y + y) * source->Width + src_x], (size_t)source->Width * 4);
}

//-----------------------------------------------------------------------------
// ImGuiCaptureContext
//-----------------------------------------------------------------------------

// Returns true when capture is in progress.
ImGuiCaptureStatus ImGuiCaptureContext::CaptureUpdate(ImGuiCaptureArgs* args)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    IM_ASSERT(args != NULL);
    IM_ASSERT(ScreenCaptureFunc != NULL);
    IM_ASSERT(args->InOutputImageBuf != NULL || args->InOutputFileTemplate[0]);
    IM_ASSERT(args->InRecordFPSTarget != 0);
    IM_ASSERT((!_Recording || args->InOutputFileTemplate[0]) && "Output file must be specified when recording gif.");
    IM_ASSERT((!_Recording || !(args->InFlags & ImGuiCaptureFlags_StitchFullContents)) && "Image stitching is not supported when recording gifs.");

    ImGuiCaptureImageBuf* output = args->InOutputImageBuf ? args->InOutputImageBuf : &_Output;

    // Hide other windows so they can't be seen visible behind captured window
    for (ImGuiWindow* window : g.Windows)
    {
#ifdef IMGUI_HAS_VIEWPORT
        if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) && (args->InFlags & ImGuiCaptureFlags_StitchFullContents))
        {
            // FIXME-VIEWPORTS: Content stitching is not possible because window would get moved out of main viewport and detach from it. We need a way to force captured windows to remain in main viewport here.
            IM_ASSERT(false);
            return ImGuiCaptureStatus_Error;
        }
#endif

        bool is_window_hidden = !args->InCaptureWindows.contains(window);
        if (window->Flags & ImGuiWindowFlags_ChildWindow)
            is_window_hidden = false;
#if IMGUI_HAS_DOCK
        else if ((window->Flags & ImGuiWindowFlags_DockNodeHost))
            for (ImGuiWindow* capture_window : args->InCaptureWindows)
            {
                if (capture_window->DockNode != NULL && capture_window->DockNode->HostWindow == window)
                {
                    is_window_hidden = false;
                    break;
                }
            }
#endif
        else if ((window->Flags & ImGuiWindowFlags_Popup) != 0 && (args->InFlags & ImGuiCaptureFlags_ExpandToIncludePopups) != 0)
            is_window_hidden = false;

        if (is_window_hidden)
        {
            window->Hidden = true;
            window->HiddenFramesCannotSkipItems = 2;
        }
    }

    // Recording will be set to false when we are stopping GIF capture.
    const bool is_recording_gif = IsCapturingGif();
    const double current_time_sec = ImGui::GetTime();
    if (is_recording_gif && _LastRecordedFrameTimeSec > 0)
    {
        double delta_sec = current_time_sec - _LastRecordedFrameTimeSec;
        if (delta_sec < 1.0 / args->InRecordFPSTarget)
            return ImGuiCaptureStatus_InProgress;
    }

    //-----------------------------------------------------------------
    // Frame 0: Initialize capture state
    //-----------------------------------------------------------------
    if (_FrameNo == 0)
    {
        if (args->InOutputFileTemplate[0])
        {
            size_t file_name_size = IM_ARRAYSIZE(args->OutSavedFileName);
            ImFormatString(args->OutSavedFileName, file_name_size, args->InOutputFileTemplate, args->InFileCounter + 1);
            ImPathFixSeparatorsForCurrentOS(args->OutSavedFileName);
            if (!ImFileCreateDirectoryChain(args->OutSavedFileName, ImPathFindFilename(args->OutSavedFileName)))
            {
                printf("Capture Tool: unable to create directory for file '%s'.\n", args->OutSavedFileName);
                return ImGuiCaptureStatus_Error;
            }

            // File template will most likely end with .png, but we need .gif for animated images.
            if (is_recording_gif)
                if (char* ext = (char*)ImPathFindExtension(args->OutSavedFileName))
                    ImStrncpy(ext, ".gif", (size_t)(ext - args->OutSavedFileName));
        }

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
                    if ((window->Flags & ImGuiWindowFlags_Popup || window->Flags & ImGuiWindowFlags_Tooltip) && !(args->InFlags & ImGuiCaptureFlags_ExpandToIncludePopups))
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

        if (args->InFlags & ImGuiCaptureFlags_StitchFullContents)
        {
            IM_ASSERT(!is_capturing_rect && "Capture Tool: capture of full window contents is not possible when capturing specified rect.");
            IM_ASSERT(args->InCaptureWindows.Size == 1 && "Capture Tool: capture of full window contents is not possible when capturing more than one window.");

            // Resize window to it's contents and capture it's entire width/height. However if window is bigger than
            // it's contents - keep original size.
            ImVec2 full_size;
            ImGuiWindow* window = args->InCaptureWindows.front();
            full_size.x = ImMax(window->SizeFull.x, window->ContentSize.x + window->WindowPadding.y * 2);
            full_size.y = ImMax(window->SizeFull.y, window->ContentSize.y + window->WindowPadding.y * 2 + window->TitleBarHeight() + window->MenuBarHeight());
            ImGui::SetWindowSize(window, full_size);
        }

        _FrameNo++;
        return ImGuiCaptureStatus_InProgress;
    }

    //-----------------------------------------------------------------
    // Frame 1: Position windows, lock rectangle, create capture buffer
    //-----------------------------------------------------------------
    if (_FrameNo == 1)
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
            if (args->InFlags & ImGuiCaptureFlags_StitchFullContents)
                ImGui::SetWindowPos(window, window->Pos + move_offset);
            _CaptureRect.Add(window->Rect());
        }

        // Include padding in capture.
        _CaptureRect.Expand(args->InPadding);

        ImRect clip_rect(ImVec2(0, 0), io.DisplaySize);
#ifdef IMGUI_HAS_VIEWPORT
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            clip_rect = ImRect(main_viewport->Pos, main_viewport->Pos + main_viewport->Size);
        }
#endif
        if (args->InFlags & ImGuiCaptureFlags_StitchFullContents)
            IM_ASSERT(_CaptureRect.Min.x >= clip_rect.Min.x && _CaptureRect.Max.x <= clip_rect.Max.x);  // Horizontal stitching is not implemented. Do not allow capture that does not fit into viewport horizontally.
        else
            _CaptureRect.ClipWith(clip_rect);   // Can not capture area outside of screen. Clip capture rect, since we capturing only visible rect anyway.

        // Initialize capture buffer.
        args->OutImageSize = _CaptureRect.GetSize();
        output->CreateEmpty((int)_CaptureRect.GetWidth(), (int)_CaptureRect.GetHeight());
        _FrameNo++;
        return ImGuiCaptureStatus_InProgress;
    }

    //-----------------------------------------------------------------
    // Frame 4+N*4: Capture a frame
    //-----------------------------------------------------------------
    if ((_FrameNo % 4) == 0 || is_recording_gif)
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
            IM_ASSERT(w == output->Width);
            if (args->InFlags & ImGuiCaptureFlags_StitchFullContents)
                IM_ASSERT(h <= output->Height);     // When stitching, image can be taller than captured viewport.
            else
                IM_ASSERT(h == output->Height);

            if (!ScreenCaptureFunc(x1, y1, w, h, &output->Data[_ChunkNo * w * capture_height], ScreenCaptureUserData))
            {
                printf("Screen capture function failed.\n");
                return ImGuiCaptureStatus_Error;
            }

            if (args->InFlags & ImGuiCaptureFlags_StitchFullContents)
            {
                // Window moves up in order to expose it's lower part.
                for (ImGuiWindow* window : args->InCaptureWindows)
                    ImGui::SetWindowPos(window, window->Pos - ImVec2(0, (float)h));
                _CaptureRect.TranslateY(-(float)h);
                _ChunkNo++;
            }

            if (is_recording_gif)
            {
                // _GifWriter is NULL when recording just started. Initialize recording state.
                const int gif_frame_interval = 100 / args->InRecordFPSTarget;
                if (_GifWriter == NULL)
                {
                    // First GIF frame, initialize now that dimensions are known.
                    unsigned int width = (unsigned int)capture_rect.GetWidth();
                    unsigned int height = (unsigned int)capture_rect.GetHeight();
                    IM_ASSERT(_GifWriter == NULL);
                    _GifWriter = IM_NEW(GifWriter)();
                    GifBegin(_GifWriter, args->OutSavedFileName, width, height, (uint32_t)gif_frame_interval);
                }

                // Save new GIF frame
                // FIXME: Not optimal at all (e.g. compare to gifsicle -O3 output)
                GifWriteFrame(_GifWriter, (const uint8_t*)output->Data, (uint32_t)output->Width, (uint32_t)output->Height, (uint32_t)gif_frame_interval, 8, false);
                _LastRecordedFrameTimeSec = current_time_sec;
            }
        }

        // Image is finalized immediately when we are not stitching. Otherwise image is finalized when we have captured and stitched all frames.
        if (!_Recording && (!(args->InFlags & ImGuiCaptureFlags_StitchFullContents) || h <= 0))
        {
            output->RemoveAlpha();

            if (_GifWriter != NULL)
            {
                // At this point _Recording is false, but we know we were recording because _GifWriter is not NULL. Finalize gif animation here.
                GifEnd(_GifWriter);
                IM_DELETE(_GifWriter);
                _GifWriter = NULL;
            }
            else if (args->InOutputImageBuf == NULL)
            {
                // Save single frame.
                args->InFileCounter++;
                output->SaveFile(args->OutSavedFileName);
                output->Clear();
            }

            // Restore window positions unconditionally. We may have moved them ourselves during capture.
            for (int i = 0; i < _WindowBackupRects.Size; i++)
            {
                ImGuiWindow* window = _WindowBackupRectsWindows[i];
                if (window->Hidden)
                    continue;

                ImRect rect = _WindowBackupRects[i];
                ImGui::SetWindowPos(window, rect.Min, ImGuiCond_Always);
                ImGui::SetWindowSize(window, rect.GetSize(), ImGuiCond_Always);
            }

            _FrameNo = _ChunkNo = 0;
            _LastRecordedFrameTimeSec = 0;
            g.Style.DisplayWindowPadding = _DisplayWindowPaddingBackup;
            g.Style.DisplaySafeAreaPadding = _DisplaySafeAreaPaddingBackup;
            args->_Capturing = false;
            return ImGuiCaptureStatus_Done;
        }
    }

    // Keep going
    _FrameNo++;
    return ImGuiCaptureStatus_InProgress;
}

void ImGuiCaptureContext::BeginGifCapture(ImGuiCaptureArgs* args)
{
    IM_ASSERT(args != NULL);
    IM_ASSERT(_Recording == false);
    IM_ASSERT(_GifWriter == NULL);
    _Recording = true;
    args->InOutputImageBuf = &_Output;
    IM_ASSERT(args->InRecordFPSTarget >= 1);
    IM_ASSERT(args->InRecordFPSTarget <= 100);
}

void ImGuiCaptureContext::EndGifCapture(ImGuiCaptureArgs* args)
{
    IM_ASSERT(args != NULL);
    IM_ASSERT(_Recording == true);
    IM_ASSERT(_GifWriter != NULL);
    _Recording = false;
}

bool ImGuiCaptureContext::IsCapturingGif()
{
    return _Recording || _GifWriter;
}

//-----------------------------------------------------------------------------
// ImGuiCaptureTool
//-----------------------------------------------------------------------------

ImGuiCaptureTool::ImGuiCaptureTool(ImGuiScreenCaptureFunc capture_func)
{
    Context.ScreenCaptureFunc = capture_func;
    ImStrncpy(SaveFileName, "captures/imgui_capture_%04d.png", (size_t)IM_ARRAYSIZE(SaveFileName));
}

// Interactively pick a single window
void ImGuiCaptureTool::CaptureWindowPicker(const char* title, ImGuiCaptureArgs* args)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    if (_CaptureState == ImGuiCaptureToolState_Capturing && args->_Capturing)
    {
        ImGuiCaptureStatus status = Context.CaptureUpdate(args);
        if (ImGui::IsKeyPressedMap(ImGuiKey_Escape) || status != ImGuiCaptureStatus_InProgress)
        {
            _CaptureState = ImGuiCaptureToolState_None;
            if (status == ImGuiCaptureStatus_Done)
                ImStrncpy(LastSaveFileName, args->OutSavedFileName, IM_ARRAYSIZE(LastSaveFileName));
            //else
            //    ImFileDelete(args->OutSavedFileName);
       }
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
        if (Flags & ImGuiCaptureFlags_HideCaptureToolWindow)
            if (capture_window == ImGui::GetCurrentWindow())
                return;

        // Draw rect that is about to be captured
        ImRect r = capture_window->Rect();
        r.Expand(args->InPadding);
        r.ClipWith(ImRect(ImVec2(0, 0), io.DisplaySize));
        r.Expand(1.0f);
        fg_draw_list->AddRect(r.Min, r.Max, IM_COL32_WHITE, 0.0f, ~0, 2.0f);
    }

    ImGui::SetTooltip("Capture window: %s\nPress ESC to cancel.", capture_window ? capture_window->Name : "<None>");
    if (ImGui::IsMouseClicked(0) && capture_window)
    {
        ImGui::FocusWindow(capture_window);
        args->InCaptureWindows.clear();
        args->InCaptureWindows.push_back(capture_window);
        _CaptureState = ImGuiCaptureToolState_Capturing;
        // We cheat a little. args->_Capturing is set to true when Capture.CaptureUpdate(args), but we use this
        // field to differentiate which capture is in progress (windows picker or selector), therefore we set it to true
        // in advance and execute Capture.CaptureUpdate(args) only when args->_Capturing is true.
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

        const bool is_popup = (window->Flags & ImGuiWindowFlags_Popup) || (window->Flags & ImGuiWindowFlags_Tooltip);
        if (args->InFlags & ImGuiCaptureFlags_ExpandToIncludePopups && is_popup)
        {
            capture_rect.Add(window->Rect());
            args->InCaptureWindows.push_back(window);
            continue;
        }

        if (is_popup)
            continue;

        if (window->Flags & ImGuiWindowFlags_ChildWindow)
            continue;

        if (args->InFlags & ImGuiCaptureFlags_HideCaptureToolWindow && window == ImGui::GetCurrentWindow())
            continue;

        ImGui::PushID(window);
        bool* p_selected = (bool*)g.CurrentWindow->StateStorage.GetIntRef(window->RootWindow->ID, 0);

        if (_CaptureState == ImGuiCaptureToolState_SelectRectUpdate)
            *p_selected = select_rect.Contains(window->Rect());

        // Ensure that text after the ## is actually displayed to the user (FIXME: won't be able to check/uncheck from that portion of the text)
        ImGui::Checkbox(window->Name, p_selected);
        if (const char* remaining_text = ImGui::FindRenderedTextEnd(window->Name))
            if (remaining_text[0] != 0)
            {
                if (remaining_text > window->Name)
                    ImGui::SameLine(0, 1);
                else
                    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
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

    // Draw GIF recording controls
    // (Prefer 100/FPS to be an integer)
    ImGui::DragInt("##FPS", &args->InRecordFPSTarget, 0.1f, 10, 100, "FPS=%d");
    ImGui::SameLine();
    if (ImGui::Button(Context.IsCapturingGif() ? "Stop###StopRecord" : "Record###StopRecord") || (Context._Recording && ImGui::IsKeyPressedMap(ImGuiKey_Escape)))
    {
        if (!Context.IsCapturingGif())
        {
            if (can_capture)
            {
                Context.BeginGifCapture(args);
                do_capture = true;
            }
        }
        else
        {
            Context.EndGifCapture(args);
        }
    }

    // Process capture
    if (can_capture && do_capture)
    {
        // We cheat a little. args->_Capturing is set to true when Capture.CaptureUpdate(args), but we use this
        // field to differentiate which capture is in progress (windows picker or selector), therefore we set it to true
        // in advance and execute Capture.CaptureUpdate(args) only when args->_Capturing is true.
        args->_Capturing = true;
        _CaptureState = ImGuiCaptureToolState_Capturing;
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Alternatively press Alt+C to capture selection.");

    if (_CaptureState == ImGuiCaptureToolState_Capturing && args->_Capturing)
    {
        if (Context.IsCapturingGif())
            args->InFlags &= ~ImGuiCaptureFlags_StitchFullContents;

        ImGuiCaptureStatus status = Context.CaptureUpdate(args);
        if (status != ImGuiCaptureStatus_InProgress)
        {
            _CaptureState = ImGuiCaptureToolState_None;
            if (status == ImGuiCaptureStatus_Done)
                ImStrncpy(LastSaveFileName, args->OutSavedFileName, IM_ARRAYSIZE(LastSaveFileName));
            //else
            //    ImFileDelete(args->OutSavedFileName);
        }
    }
}

static void PushDisabled()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4 col = style.Colors[ImGuiCol_Text];
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(col.x, col.y, col.z, col.w * 0.5f));
}

static void PopDisabled()
{
    ImGui::PopStyleColor();
    ImGui::PopItemFlag();
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
        const bool has_last_file_name = (LastSaveFileName[0] != 0);
        if (!has_last_file_name)
            PushDisabled();
        if (ImGui::Button("Open Last"))             // FIXME-CAPTURE: Running tests changes last captured file name.
            ImOsOpenInShell(LastSaveFileName);
        if (!has_last_file_name)
            PopDisabled();
        if (has_last_file_name && ImGui::IsItemHovered())
            ImGui::SetTooltip("Open %s", LastSaveFileName);
        ImGui::SameLine();

        char save_file_dir[256];
        strcpy(save_file_dir, SaveFileName);
        if (!save_file_dir[0])
            PushDisabled();
        else if (char* slash_pos = ImMax(strrchr(save_file_dir, '/'), strrchr(save_file_dir, '\\')))
            *slash_pos = 0;                         // Remove file name.
        else
            strcpy(save_file_dir, ".");             // Only filename is present, open current directory.
        if (ImGui::Button("Open Directory"))
            ImOsOpenInShell(save_file_dir);
        if (save_file_dir[0] && ImGui::IsItemHovered())
            ImGui::SetTooltip("Open %s/", save_file_dir);
        if (!save_file_dir[0])
            PopDisabled();

        ImGui::PushItemWidth(-200.0f);

        ImGui::InputText("Out filename template", SaveFileName, IM_ARRAYSIZE(SaveFileName));
        ImGui::DragFloat("Padding", &Padding, 0.1f, 0, 32, "%.0f");

        if (ImGui::Button("Snap Windows To Grid", ImVec2(-200, 0)))
            SnapWindowsToGrid(SnapGridSize, Padding);
        ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(50.0f);
        ImGui::DragFloat("##SnapGridSize", &SnapGridSize, 1.0f, 1.0f, 128.0f, "%.0f");

        ImGui::Checkbox("Software Mouse Cursor", &io.MouseDrawCursor);  // FIXME-TESTS: Test engine always resets this value.

        bool content_stitching_available = _CaptureArgsSelector.InCaptureWindows.size() <= 1;
#ifdef IMGUI_HAS_VIEWPORT
        content_stitching_available &= !(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable);
#endif
        if (!content_stitching_available)
            PushDisabled();
        ImGui::CheckboxFlags("Stitch and capture full contents height", &Flags, ImGuiCaptureFlags_StitchFullContents);
        if (!content_stitching_available)
        {
            Flags &= ~ImGuiCaptureFlags_StitchFullContents;
            PopDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Content stitching is not possible when using viewports.");
        }

        ImGui::CheckboxFlags("Hide capture tool window", &Flags, ImGuiCaptureFlags_HideCaptureToolWindow);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Full height of picked window will be captured.");
        ImGui::CheckboxFlags("Include tooltips", &Flags, ImGuiCaptureFlags_ExpandToIncludePopups);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Capture area will be expanded to include visible tooltips.");

        ImGui::PopItemWidth();
        ImGui::TreePop();
    }

    ImGui::Separator();

    // Ensure that use of different contexts use same file counter and don't overwrite previously created files.
    _CaptureArgsPicker.InFileCounter = _CaptureArgsSelector.InFileCounter = ImMax(_CaptureArgsPicker.InFileCounter, _CaptureArgsSelector.InFileCounter);
    // Propagate settings from UI to args.
    _CaptureArgsPicker.InPadding = _CaptureArgsSelector.InPadding = Padding;
    _CaptureArgsPicker.InFlags = _CaptureArgsSelector.InFlags = Flags;
    ImStrncpy(_CaptureArgsPicker.InOutputFileTemplate, SaveFileName, (size_t)IM_ARRAYSIZE(_CaptureArgsPicker.InOutputFileTemplate));
    ImStrncpy(_CaptureArgsSelector.InOutputFileTemplate, SaveFileName, (size_t)IM_ARRAYSIZE(_CaptureArgsSelector.InOutputFileTemplate));

    // Hide tool window unconditionally.
    if (Flags & ImGuiCaptureFlags_HideCaptureToolWindow)
        if (_CaptureState == ImGuiCaptureToolState_Capturing || _CaptureState == ImGuiCaptureToolState_PickingSingleWindow)
        {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
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
void ImGuiCaptureTool::SnapWindowsToGrid(float cell_size, float padding)
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
        ImGui::SetWindowPos(window, rect.Min + ImVec2(padding, padding));
        ImGui::SetWindowSize(window, rect.GetSize());
    }
}
