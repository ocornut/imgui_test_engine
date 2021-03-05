// Dear ImGui Screen/Video Capture Tool
// This is usable as a standalone applet or controlled by the test engine.
// (code)

// FIXME: This desperately needs rewrite.
// FIXME: This currently depends on imgui_dev/shared/imgui_utils.cpp
// FIXME: GIF compression are substandard with current gif.h. May be simpler to pipe to ffmpeg?

/*

Index of this file:

// [SECTION] Includes
// [SECTION] ImGuiCaptureImageBuf
// [SECTION] ImGuiCaptureContext
// [SECTION] ImGuiCaptureTool

*/

//-----------------------------------------------------------------------------
// [SECTIONS Includes
//-----------------------------------------------------------------------------

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_capture_tool.h"
#include "../shared/imgui_utils.h"  // ImPathFindFilename, ImPathFindExtension, ImPathFixSeparatorsForCurrentOS, ImFileCreateDirectoryChain, ImOsOpenInShell

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
// [SECTION] ImGuiCaptureImageBuf
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
    IM_ASSERT(Data != NULL);
    int ret = stbi_write_png(filename, Width, Height, 4, Data, Width * 4);
    return ret != 0;
}

void ImGuiCaptureImageBuf::RemoveAlpha()
{
    unsigned int* p = Data;
    int n = Width * Height;
    while (n-- > 0)
    {
        *p |= IM_COL32_A_MASK;
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
// [SECTION] ImGuiCaptureContext
//-----------------------------------------------------------------------------

static void HideOtherWindows(const ImGuiCaptureArgs* args)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    IM_UNUSED(io);

    for (ImGuiWindow* window : g.Windows)
    {
#ifdef IMGUI_HAS_VIEWPORT
        // FIXME-VIEWPORTS: Content stitching is not possible because window would get moved out of main viewport and detach from it. We need a way to force captured windows to remain in main viewport here.
        //if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) && (args->InFlags & ImGuiCaptureFlags_StitchFullContents))
        //    IM_ASSERT(false);
#endif
        if (args->InCaptureWindows.contains(window))
            continue;
        if (window->Flags & ImGuiWindowFlags_ChildWindow)
            continue;
        if ((window->Flags & ImGuiWindowFlags_Popup) != 0 && (args->InFlags & ImGuiCaptureFlags_ExpandToIncludePopups) != 0)
            continue;

#ifdef IMGUI_HAS_DOCK
        bool should_hide_window = true;
        if ((window->Flags & ImGuiWindowFlags_DockNodeHost))
            for (ImGuiWindow* capture_window : args->InCaptureWindows)
            {
                if (capture_window->DockNode != NULL && capture_window->DockNode->HostWindow == window)
                {
                    should_hide_window = false;
                    break;
                }
            }
        if (!should_hide_window)
            continue;
#endif

        // Not overwriting HiddenFramesCanSkipItems or HiddenFramesCannotSkipItems since they have side-effects (e.g. preserving ContentsSize)
        if (window->WasActive || window->Active)
            window->HiddenFramesForRenderOnly = 2;
    }
}

static ImRect GetMainViewportRect()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    return ImRect(viewport->Pos, viewport->Pos + viewport->Size);
}

void ImGuiCaptureContext::PostNewFrame()
{
    const ImGuiCaptureArgs* args = _CaptureArgs;
    if (args == NULL)
        return;

    ImGuiContext& g = *GImGui;

    // Override mouse cursor
    // FIXME: Could override in Pre+Post Render() hooks to avoid doing a backup.
    if (args->InFlags & ImGuiCaptureFlags_HideMouseCursor)
    {
        if (_FrameNo == 0)
            _MouseDrawCursorBackup = g.IO.MouseDrawCursor;
        g.IO.MouseDrawCursor = false;
    }

    // Force mouse position. Hovered window is reset in ImGui::NewFrame() based on mouse real mouse position.
    // FIXME: Would be saner to override io.MousePos in Pre NewFrame() hook.
    if (_FrameNo > 2 && (args->InFlags & ImGuiCaptureFlags_StitchFullContents) != 0)
    {
        IM_ASSERT(args->InCaptureWindows.Size == 1);
        g.IO.MousePos = args->InCaptureWindows[0]->Pos + _MouseRelativeToWindowPos;
        g.HoveredWindow = _HoveredWindow;
    }
}

// Returns true when capture is in progress.
ImGuiCaptureStatus ImGuiCaptureContext::CaptureUpdate(ImGuiCaptureArgs* args)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    IM_ASSERT(args != NULL);
    IM_ASSERT(ScreenCaptureFunc != NULL);
    IM_ASSERT(args->InOutputImageBuf != NULL || args->InOutputFileTemplate[0]);
    IM_ASSERT(args->InRecordFPSTarget != 0);

    if (_GifRecording)
    {
        IM_ASSERT(args->InOutputFileTemplate[0] && "Output filename must be specified when recording gifs.");
        IM_ASSERT(args->InOutputImageBuf == NULL && "Output buffer cannot be specified when recording gifs.");
        IM_ASSERT(!(args->InFlags & ImGuiCaptureFlags_StitchFullContents) && "Image stitching is not supported when recording gifs.");
    }

    ImGuiCaptureImageBuf* output = args->InOutputImageBuf ? args->InOutputImageBuf : &_CaptureBuf;
    ImRect viewport_rect = GetMainViewportRect();

    // Hide other windows so they can't be seen visible behind captured window
    if (!args->InCaptureWindows.empty())
        HideOtherWindows(args);

    // Recording will be set to false when we are stopping GIF capture.
    const bool is_recording_gif = IsCapturingGif();
    const double current_time_sec = ImGui::GetTime();
    if (is_recording_gif && _GifLastFrameTime > 0)
    {
        double delta_sec = current_time_sec - _GifLastFrameTime;
        if (delta_sec < 1.0 / args->InRecordFPSTarget)
            return ImGuiCaptureStatus_InProgress;
    }

    // Capture can be performed in single frame if we are capturing a rect.
    const bool instant_capture = (args->InFlags & ImGuiCaptureFlags_Instant) != 0;
    const bool is_capturing_rect = args->InCaptureRect.GetWidth() > 0 && args->InCaptureRect.GetHeight() > 0;
    if (instant_capture)
    {
        IM_ASSERT(args->InCaptureWindows.empty());
        IM_ASSERT(is_capturing_rect);
        IM_ASSERT(!is_recording_gif);
        IM_ASSERT((args->InFlags & ImGuiCaptureFlags_StitchFullContents) == 0);
    }

    //-----------------------------------------------------------------
    // Frame 0: Initialize capture state
    //-----------------------------------------------------------------
    if (_FrameNo == 0)
    {
        // Create output folder and decide of output filename
        if (args->InOutputFileTemplate[0])
        {
            size_t file_name_size = IM_ARRAYSIZE(args->OutSavedFileName);
            ImFormatString(args->OutSavedFileName, file_name_size, args->InOutputFileTemplate, args->InFileCounter + 1);
            ImPathFixSeparatorsForCurrentOS(args->OutSavedFileName);
            if (!ImFileCreateDirectoryChain(args->OutSavedFileName, ImPathFindFilename(args->OutSavedFileName)))
            {
                printf("ImGuiCaptureContext: unable to create directory for file '%s'.\n", args->OutSavedFileName);
                return ImGuiCaptureStatus_Error;
            }

            // File template will most likely end with .png, but we need .gif for animated images.
            if (is_recording_gif)
                if (char* ext = (char*)ImPathFindExtension(args->OutSavedFileName))
                    ImStrncpy(ext, ".gif", (size_t)(ext - args->OutSavedFileName));
        }

        // When recording, same args should have been passed to BeginGifCapture().
        IM_ASSERT(!_GifRecording || _CaptureArgs == args);

        _CaptureArgs = args;
        _ChunkNo = 0;
        _CaptureRect = _CapturedWindowRect = ImRect(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
        _WindowBackupRects.clear();
        _WindowBackupRectsWindows.clear();
        _DisplayWindowPaddingBackup = g.Style.DisplayWindowPadding;
        _DisplaySafeAreaPaddingBackup = g.Style.DisplaySafeAreaPadding;
        g.Style.DisplayWindowPadding = ImVec2(0, 0);    // Allow windows to be positioned fully outside of visible viewport.
        g.Style.DisplaySafeAreaPadding = ImVec2(0, 0);

        if (is_capturing_rect)
        {
            // Capture arbitrary rectangle. If any windows are specified in this mode only they will appear in captured region.
            _CaptureRect = args->InCaptureRect;
            if (args->InCaptureWindows.empty() && !instant_capture)
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
            _CapturedWindowRect.Add(window->Rect());
            _WindowBackupRects.push_back(window->Rect());
            _WindowBackupRectsWindows.push_back(window);
        }

        if (args->InFlags & ImGuiCaptureFlags_StitchFullContents)
        {
            IM_ASSERT(is_capturing_rect == false && "ImGuiCaptureContext: capture of full window contents is not possible when capturing specified rect.");
            IM_ASSERT(args->InCaptureWindows.Size == 1 && "ImGuiCaptureContext: capture of full window contents is not possible when capturing more than one window.");

            // Resize window to it's contents and capture it's entire width/height. However if window is bigger than it's contents - keep original size.
            ImGuiWindow* window = args->InCaptureWindows[0];
            ImVec2 full_size = window->SizeFull;

            // Mouse cursor is relative to captured window even if it is not hovered, in which case cursor is kept off the window to prevent appearing in screenshot multiple times by accident.
            _MouseRelativeToWindowPos = io.MousePos - window->Pos + window->Scroll;

            // FIXME-CAPTURE: Window width change may affect vertical content size if window contains text that wraps. To accurately position mouse cursor for capture we avoid horizontal resize.
            // Instead window width should be set manually before capture, as it is simple to do and most of the time we already have a window of desired width.
            //full_size.x = ImMax(window->SizeFull.x, window->ContentSize.x + (window->WindowPadding.x + window->WindowBorderSize) * 2);
            full_size.y = ImMax(window->SizeFull.y, window->ContentSize.y + (window->WindowPadding.y + window->WindowBorderSize) * 2 + window->TitleBarHeight() + window->MenuBarHeight());
            ImGui::SetWindowSize(window, full_size);
            _HoveredWindow = g.HoveredWindow;
        }
        else
        {
            _MouseRelativeToWindowPos = ImVec2(-FLT_MAX, -FLT_MAX);
            _HoveredWindow = NULL;
        }
    }
    else
    {
        IM_ASSERT(args == _CaptureArgs); // Capture args can not change mid-capture.
    }

    //-----------------------------------------------------------------
    // Frame 1: Skipped to allow window size to update fully
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    // Frame 2: Position windows, lock rectangle, create capture buffer
    //-----------------------------------------------------------------
    if (_FrameNo == 2 || instant_capture)
    {
        // Move group of windows so combined rectangle position is at the top-left corner + padding and create combined
        // capture rect of entire area that will be saved to screenshot. Doing this on the second frame because when
        // ImGuiCaptureToolFlags_StitchFullContents flag is used we need to allow window to reposition.
        ImVec2 move_offset = ImVec2(args->InPadding, args->InPadding) - _CapturedWindowRect.Min + viewport_rect.Min;
        for (ImGuiWindow* window : args->InCaptureWindows)
        {
            // Repositioning of a window may take multiple frames, depending on whether window was already rendered or not.
            if (args->InFlags & ImGuiCaptureFlags_StitchFullContents)
                ImGui::SetWindowPos(window, window->Pos + move_offset);
            _CaptureRect.Add(window->Rect());
        }

        // Include padding in capture.
        if (!is_capturing_rect)
            _CaptureRect.Expand(args->InPadding);

        const ImRect clip_rect = viewport_rect;
        if (args->InFlags & ImGuiCaptureFlags_StitchFullContents)
            IM_ASSERT(_CaptureRect.Min.x >= clip_rect.Min.x && _CaptureRect.Max.x <= clip_rect.Max.x);  // Horizontal stitching is not implemented. Do not allow capture that does not fit into viewport horizontally.
        else
            _CaptureRect.ClipWith(clip_rect);   // Can not capture area outside of screen. Clip capture rect, since we capturing only visible rect anyway.

        // Initialize capture buffer.
        args->OutImageSize = _CaptureRect.GetSize();
        output->CreateEmpty((int)_CaptureRect.GetWidth(), (int)_CaptureRect.GetHeight());
    }

    //-----------------------------------------------------------------
    // Frame 4+N*4: Capture a frame
    //-----------------------------------------------------------------
    if (((_FrameNo > 2) && (_FrameNo % 4) == 0) || (is_recording_gif && _FrameNo > 2) || instant_capture)
    {
        // FIXME: Implement capture of regions wider than viewport.
        // Capture a portion of image. Capturing of windows wider than viewport is not implemented yet.
        const ImRect clip_rect = viewport_rect;
        ImRect capture_rect = _CaptureRect;
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

            const ImGuiID viewport_id = 0;
            if (!ScreenCaptureFunc(viewport_id, x1, y1, w, h, &output->Data[_ChunkNo * w * capture_height], ScreenCaptureUserData))
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
                    const unsigned int width = (unsigned int)capture_rect.GetWidth();
                    const unsigned int height = (unsigned int)capture_rect.GetHeight();
                    IM_ASSERT(_GifWriter == NULL);
                    _GifWriter = IM_NEW(GifWriter)();
                    GifBegin(_GifWriter, args->OutSavedFileName, width, height, (uint32_t)gif_frame_interval);
                }

                // Save new GIF frame
                // FIXME: Not optimal at all (e.g. compare to gifsicle -O3 output)
                GifWriteFrame(_GifWriter, (const uint8_t*)output->Data, (uint32_t)output->Width, (uint32_t)output->Height, (uint32_t)gif_frame_interval, 8, false);
                _GifLastFrameTime = current_time_sec;
            }
        }

        // Image is finalized immediately when we are not stitching. Otherwise image is finalized when we have captured and stitched all frames.
        if (!_GifRecording && (!(args->InFlags & ImGuiCaptureFlags_StitchFullContents) || h <= 0))
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
                ImGui::SetWindowPos(window, _WindowBackupRects[i].Min, ImGuiCond_Always);
                ImGui::SetWindowSize(window, _WindowBackupRects[i].GetSize(), ImGuiCond_Always);
            }
            if (args->InFlags & ImGuiCaptureFlags_HideMouseCursor)
                g.IO.MouseDrawCursor = _MouseDrawCursorBackup;
            g.Style.DisplayWindowPadding = _DisplayWindowPaddingBackup;
            g.Style.DisplaySafeAreaPadding = _DisplaySafeAreaPaddingBackup;

            _FrameNo = _ChunkNo = 0;
            _GifLastFrameTime = 0;
            _MouseRelativeToWindowPos = ImVec2(-FLT_MAX, -FLT_MAX);
            _HoveredWindow = NULL;
            _CaptureArgs = NULL;

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
    IM_ASSERT(_GifRecording == false);
    IM_ASSERT(_GifWriter == NULL);
    _GifRecording = true;
    _CaptureArgs = args;
    IM_ASSERT(args->InRecordFPSTarget >= 1);
    IM_ASSERT(args->InRecordFPSTarget <= 100);
}

void ImGuiCaptureContext::EndGifCapture()
{
    IM_ASSERT(_CaptureArgs != NULL);
    IM_ASSERT(_GifRecording == true);
    IM_ASSERT(_GifWriter != NULL);
    _GifRecording = false;
}

bool ImGuiCaptureContext::IsCapturingGif()
{
    return _GifRecording || _GifWriter;
}

//-----------------------------------------------------------------------------
// ImGuiCaptureTool
//-----------------------------------------------------------------------------

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

ImGuiCaptureTool::ImGuiCaptureTool()
{
    // Filename template for where screenshots will be saved. May contain directories or variation of %d format.
    strcpy(_CaptureArgs.InOutputFileTemplate, "captures/imgui_capture_%04d.png");
}

void ImGuiCaptureTool::SetCaptureFunc(ImGuiScreenCaptureFunc capture_func)
{
    Context.ScreenCaptureFunc = capture_func;
}

// Interactively pick a single window
void ImGuiCaptureTool::CaptureWindowPicker(ImGuiCaptureArgs* args)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
    const ImVec2 button_sz = ImVec2(TEXT_BASE_WIDTH * 30, 0.0f);
    const ImGuiID picking_id = ImGui::GetID("##picking");

    if (ImGui::Button("Capture Single Window..", button_sz))
        _CaptureState = ImGuiCaptureToolState_PickingSingleWindow;

    if (_CaptureState == ImGuiCaptureToolState_PickingSingleWindow)
    {
        // Picking a window
        ImGuiWindow* capture_window = g.HoveredWindow ? g.HoveredWindow->RootWindow : NULL;
        ImDrawList* fg_draw_list = ImGui::GetForegroundDrawList();
        ImGui::SetActiveID(picking_id, g.CurrentWindow);    // Steal active ID so our click won't interact with something else.
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImGui::SetTooltip("Capture window: '%s'\nPress ESC to cancel.", capture_window ? capture_window->Name : "<None>");

        // FIXME: Would be nice to have a way to enforce front-most windows. Perhaps make this Render() feature more generic.
        //if (capture_window)
        //    g.NavWindowingTarget = capture_window;

        // Draw rect that is about to be captured
        const ImRect viewport_rect = GetMainViewportRect();
        const ImU32 col_dim_overlay = IM_COL32(0, 0, 0, 40);
        if (capture_window)
        {
            ImRect r = capture_window->Rect();
            r.Expand(args->InPadding);
            r.ClipWith(ImRect(ImVec2(0, 0), io.DisplaySize));
            r.Expand(1.0f);
            fg_draw_list->AddRect(r.Min, r.Max, IM_COL32_WHITE, 0.0f, ~0, 2.0f);
            ImGui::RenderRectFilledWithHole(fg_draw_list, viewport_rect, r, col_dim_overlay, 0.0f);
        }
        else
        {
            fg_draw_list->AddRectFilled(viewport_rect.Min, viewport_rect.Max, col_dim_overlay);
        }

        if (ImGui::IsMouseClicked(0) && capture_window)
        {
            ImGui::FocusWindow(capture_window);
            _SelectedWindows.resize(0);
            _CaptureState = ImGuiCaptureToolState_Capturing;
            args->InCaptureWindows.clear();
            args->InCaptureWindows.push_back(capture_window);
        }
        if (ImGui::IsKeyPressedMap(ImGuiKey_Escape))
            _CaptureState = ImGuiCaptureToolState_None;
    }
    else
    {
        if (ImGui::GetActiveID() == picking_id)
            ImGui::ClearActiveID();
    }
}

void ImGuiCaptureTool::CaptureWindowsSelector(ImGuiCaptureArgs* args)
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    // Gather selected windows
    ImRect capture_rect(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (ImGuiWindow* window : g.Windows)
    {
        if (window->WasActive == false)
            continue;
        if (window->Flags & ImGuiWindowFlags_ChildWindow)
            continue;
        const bool is_popup = (window->Flags & ImGuiWindowFlags_Popup) || (window->Flags & ImGuiWindowFlags_Tooltip);
        if ((args->InFlags & ImGuiCaptureFlags_ExpandToIncludePopups) && is_popup)
        {
            capture_rect.Add(window->Rect());
            args->InCaptureWindows.push_back(window);
            continue;
        }
        if (is_popup)
            continue;
        if (_SelectedWindows.contains(window->RootWindow->ID))
        {
            capture_rect.Add(window->Rect());
            args->InCaptureWindows.push_back(window);
        }
    }
    const bool allow_capture = !capture_rect.IsInverted() && args->InCaptureWindows.Size > 0;

    const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
    const ImVec2 button_sz = ImVec2(TEXT_BASE_WIDTH * 30, 0.0f);

    // Capture Multiple Button
    {
        char label[64];
        ImFormatString(label, 64, "Capture Multiple (%d)###CaptureMultiple", args->InCaptureWindows.Size);

        if (!allow_capture)
            PushDisabled();
        bool do_capture = ImGui::Button(label, button_sz);
        do_capture |= io.KeyAlt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C));
        if (!allow_capture)
            PopDisabled();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Alternatively press Alt+C to capture selection.");
        if (do_capture)
            _CaptureState = ImGuiCaptureToolState_Capturing;
    }

    // Record GIF button
    // (Prefer 100/FPS to be an integer)
    {
        const bool is_capturing_gif = Context.IsCapturingGif();
        if (is_capturing_gif)
        {
            if (ImGui::Button("Stop capturing gif###CaptureGif", button_sz))
                Context.EndGifCapture();
        }
        else
        {
            char label[64];
            ImFormatString(label, 64, "Capture Gif (%d)###CaptureGif", args->InCaptureWindows.Size);
            if (!allow_capture)
                PushDisabled();
            if (ImGui::Button(label, button_sz))
            {
                _CaptureState = ImGuiCaptureToolState_Capturing;
                Context.BeginGifCapture(args);
            }
            if (!allow_capture)
                PopDisabled();
        }
    }

    // Draw capture rectangle
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    if (allow_capture && _CaptureState == ImGuiCaptureToolState_None)
    {
        IM_ASSERT(capture_rect.GetWidth() > 0);
        IM_ASSERT(capture_rect.GetHeight() > 0);
        const ImRect viewport_rect = GetMainViewportRect();
        capture_rect.Expand(args->InPadding);
        capture_rect.ClipWith(viewport_rect);
        draw_list->AddRect(capture_rect.Min - ImVec2(1.0f, 1.0f), capture_rect.Max + ImVec2(1.0f, 1.0f), IM_COL32_WHITE);
    }

    ImGui::Separator();

    // Show window list and update rectangles
    ImGui::Text("Windows:");
    if (ImGui::BeginTable("split", 2))
    {
        ImGui::TableSetupColumn(NULL, ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn(NULL, ImGuiTableColumnFlags_WidthStretch);
        for (ImGuiWindow* window : g.Windows)
        {
            if (!window->WasActive)
                continue;

            const bool is_popup = (window->Flags & ImGuiWindowFlags_Popup) || (window->Flags & ImGuiWindowFlags_Tooltip);
            if (is_popup)
                continue;

            if (window->Flags & ImGuiWindowFlags_ChildWindow)
                continue;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushID(window);

            // Ensure that text after the ## is actually displayed to the user (FIXME: won't be able to check/uncheck from that portion of the text)
            bool is_selected = _SelectedWindows.contains(window->RootWindow->ID);
            if (ImGui::Checkbox(window->Name, &is_selected))
            {
                if (is_selected)
                    _SelectedWindows.push_back(window->RootWindow->ID);
                else
                    _SelectedWindows.find_erase_unsorted(window->RootWindow->ID);
            }

            if (const char* remaining_text = ImGui::FindRenderedTextEnd(window->Name))
                if (remaining_text[0] != 0)
                {
                    if (remaining_text > window->Name)
                        ImGui::SameLine(0, 1);
                    else
                        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
                    ImGui::TextUnformatted(remaining_text);
                }

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(TEXT_BASE_WIDTH * 9.0f);
            ImGui::DragFloat2("Pos", &window->Pos.x, 0.05f, 0.0f, 0.0f, "%.0f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(TEXT_BASE_WIDTH * 9.0f);
            ImGui::DragFloat2("Size", &window->SizeFull.x, 0.05f, 0.0f, 0.0f, "%.0f");
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

void ImGuiCaptureTool::ShowCaptureToolWindow(bool* p_open)
{
    // Update capturing
    if (_CaptureState == ImGuiCaptureToolState_Capturing)
    {
        ImGuiCaptureArgs* args = &_CaptureArgs;
        if (Context.IsCapturingGif() || args->InCaptureWindows.Size > 1)
            args->InFlags &= ~ImGuiCaptureFlags_StitchFullContents;

        if (Context._GifRecording && ImGui::IsKeyPressedMap(ImGuiKey_Escape))
            Context.EndGifCapture();

        ImGuiCaptureStatus status = Context.CaptureUpdate(args);
        if (status != ImGuiCaptureStatus_InProgress)
        {
            if (status == ImGuiCaptureStatus_Done)
                ImFormatString(LastOutputFileName, IM_ARRAYSIZE(LastOutputFileName), "%s", args->OutSavedFileName);
            _CaptureState = ImGuiCaptureToolState_None;
        }
    }

    // Update UI
    if (!ImGui::Begin("Dear ImGui Capture Tool", p_open))
    {
        ImGui::End();
        return;
    }
    if (Context.ScreenCaptureFunc == NULL)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Backend is missing ScreenCaptureFunc!");
        ImGui::End();
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    // Options
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Options"))
    {
        // Open Last
        {
            const bool has_last_file_name = (LastOutputFileName[0] != 0);
            if (!has_last_file_name)
                PushDisabled();
            if (ImGui::Button("Open Last"))
                ImOsOpenInShell(LastOutputFileName);
            if (!has_last_file_name)
                PopDisabled();
            if (has_last_file_name && ImGui::IsItemHovered())
                ImGui::SetTooltip("Open %s", LastOutputFileName);
            ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
        }

        // Open Directory
        {
            char save_file_dir[256];
            strcpy(save_file_dir, _CaptureArgs.InOutputFileTemplate);
            char* save_file_name = (char*)ImPathFindFilename(save_file_dir);
            if (save_file_name > save_file_dir)
                save_file_name[-1] = 0;
            else
                strcpy(save_file_dir, ".");
            if (ImGui::Button("Open Directory"))
                ImOsOpenInShell(save_file_dir);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Open %s/", save_file_dir);
        }

        ImGui::PushItemWidth(-200.0f);
        ImGui::InputText("Output template", _CaptureArgs.InOutputFileTemplate, IM_ARRAYSIZE(_CaptureArgs.InOutputFileTemplate));
        ImGui::DragFloat("Padding", &_CaptureArgs.InPadding, 0.1f, 0, 32, "%.0f");
        ImGui::DragInt("Gif FPS", &_CaptureArgs.InRecordFPSTarget, 0.1f, 10, 100, "%d fps");

        if (ImGui::Button("Snap Windows To Grid", ImVec2(-200, 0)))
            SnapWindowsToGrid(SnapGridSize, _CaptureArgs.InPadding);
        ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(50.0f);
        ImGui::DragFloat("##SnapGridSize", &SnapGridSize, 1.0f, 1.0f, 128.0f, "%.0f");

        ImGui::Checkbox("Software Mouse Cursor", &io.MouseDrawCursor);  // FIXME-TESTS: Test engine always resets this value.

        bool content_stitching_available = _CaptureArgs.InCaptureWindows.Size <= 1;
#ifdef IMGUI_HAS_VIEWPORT
        content_stitching_available &= !(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable);
#endif
        if (!content_stitching_available)
            PushDisabled();
        ImGui::CheckboxFlags("Stitch full contents height", &_CaptureArgs.InFlags, ImGuiCaptureFlags_StitchFullContents);
        if (!content_stitching_available)
        {
            PopDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Content stitching is not possible when using viewports.");
        }

        ImGui::CheckboxFlags("Include tooltips", &_CaptureArgs.InFlags, ImGuiCaptureFlags_ExpandToIncludePopups);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Capture area will be expanded to include visible tooltips.");

        ImGui::PopItemWidth();
        ImGui::TreePop();
    }

    ImGui::Separator();

    if (_CaptureState != ImGuiCaptureToolState_Capturing)
        _CaptureArgs.InCaptureWindows.clear();
    CaptureWindowPicker(&_CaptureArgs);
    CaptureWindowsSelector(&_CaptureArgs);
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

//-----------------------------------------------------------------------------
