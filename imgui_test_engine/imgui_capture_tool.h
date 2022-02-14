// Dear ImGui Screen/Video Capture Tool
// This is usable as a standalone applet or controlled by the test engine.
// (headers)

#pragma once

#include "imgui_te_utils.h"             // ImFuncPtr

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

// Our types
struct ImGuiCaptureArgs;                // Parameters for Capture
struct ImGuiCaptureContext;             // State of an active capture tool
struct ImGuiCaptureImageBuf;            // Simple helper to store an RGBA image in memory
struct ImGuiCaptureToolUI;                // Capture tool instance + UI window

typedef unsigned int ImGuiCaptureFlags; // See enum: ImGuiCaptureFlags_

// Capture function which needs to be provided by user application
typedef bool (ImGuiScreenCaptureFunc)(ImGuiID viewport_id, int x, int y, int w, int h, unsigned int* pixels, void* user_data);

// External types
struct ImGuiWindow; // imgui.h

//-----------------------------------------------------------------------------

// [Internal]
// Helper class for simple bitmap manipulation (not particularly efficient!)
struct ImGuiCaptureImageBuf
{
    int             Width;
    int             Height;
    unsigned int*   Data;       // RGBA8

    ImGuiCaptureImageBuf()      { Width = Height = 0; Data = NULL; }
    ~ImGuiCaptureImageBuf()     { Clear(); }

    void Clear();                                           // Free allocated memory buffer if such exists.
    void CreateEmpty(int w, int h);                         // Reallocate buffer for pixel data, and zero it.
    void CreateEmptyNoMemClear(int w, int h);               // Reallocate buffer for pixel data, but do not zero memory buffer.
    bool SaveFile(const char* filename);                    // Save pixel data to specified image file.
    void RemoveAlpha();                                     // Clear alpha channel from all pixels.
    void BlitSubImage(int dst_x, int dst_y, int src_x, int src_y, int w, int h, const ImGuiCaptureImageBuf* source);
};

enum ImGuiCaptureFlags_ : unsigned int
{
    ImGuiCaptureFlags_None                      = 0,
    ImGuiCaptureFlags_StitchAll                 = 1 << 0,   // Capture entire window scroll area (by scrolling and taking multiple screenshot). Only works for a single window.
    ImGuiCaptureFlags_ExpandToIncludePopups     = 1 << 1,   // Expand capture area to automatically include visible popups and tooltips.
    ImGuiCaptureFlags_HideMouseCursor           = 1 << 2,   // Do not render software mouse cursor during capture.
    ImGuiCaptureFlags_Instant                   = 1 << 3,   // Perform capture on very same frame. Only works when capturing a rectangular region. Unsupported features: content stitching, window hiding, window relocation.
    ImGuiCaptureFlags_NoSave                    = 1 << 4    // Do not save output image.
};

enum ImGuiCaptureToolState : unsigned int
{
    ImGuiCaptureToolState_None,
    ImGuiCaptureToolState_PickingSingleWindow,
    ImGuiCaptureToolState_Capturing
};

// Defines input and output arguments for capture process.
struct ImGuiCaptureArgs
{
    // [Input]
    ImGuiCaptureFlags       InFlags = 0;                    // Flags for customizing behavior of screenshot tool.
    ImVector<ImGuiWindow*>  InCaptureWindows;               // Windows to capture. All other windows will be hidden. May be used with InCaptureRect to capture only some windows in specified rect.
    ImRect                  InCaptureRect;                  // Screen rect to capture. Does not include padding.
    float                   InPadding = 16.0f;              // Extra padding at the edges of the screenshot. Ensure that there is available space around capture rect horizontally, also vertically if ImGuiCaptureFlags_StitchFullContents is not used.
    int                     InFileCounter = 0;              // Counter which may be appended to file name when saving. By default counting starts from 1. When done this field holds number of saved files.
    ImGuiCaptureImageBuf*   InOutputImageBuf = NULL;        // Output will be saved to image buffer if specified.
    char                    InOutputFileTemplate[256] = ""; // Output will be saved to a file if InOutputImageBuf is NULL.
    int                     InRecordFPSTarget = 30;         // FPS target for recording videos.
    int                     InRecordQuality = 20;           // 0 = lossless, 18 = visually lossless, 23 = default, 51 = worst
    int                     InSizeAlign = 0;                // Resolution alignment (0 = auto, 1 = no alignment, >= 2 = align width/height to be multiple of given value)

    // [Output]
    ImVec2                  OutImageSize;                   // Produced image size.
    char                    OutSavedFileName[256] = "";     // Saved file name, if any.
};

enum ImGuiCaptureStatus
{
    ImGuiCaptureStatus_InProgress,
    ImGuiCaptureStatus_Done,
    ImGuiCaptureStatus_Error
};

// Implements functionality for capturing images
struct ImGuiCaptureContext
{
    // IO
    ImFuncPtr(ImGuiScreenCaptureFunc) ScreenCaptureFunc = NULL; // Graphics backend specific function that captures specified portion of framebuffer and writes RGBA data to `pixels` buffer.
    void*                   ScreenCaptureUserData = NULL;       // Custom user pointer which is passed to ScreenCaptureFunc. (Optional)
    const char*             VideoCaptureExt = NULL;             // Video file extension (e.g. ".gif" or ".mp4")
    const char*             VideoCapturePathToFFMPEG = NULL;    // Path to ffmpeg executable.

    // [Internal]
    ImRect                  _CaptureRect;                   // Viewport rect that is being captured.
    ImRect                  _CapturedWindowRect;            // Top-left corner of region that covers all windows included in capture. This is not same as _CaptureRect.Min when capturing explicitly specified rect.
    int                     _ChunkNo = 0;                   // Number of chunk that is being captured when capture spans multiple frames.
    int                     _FrameNo = 0;                   // Frame number during capture process that spans multiple frames.
    ImVec2                  _MouseRelativeToWindowPos;      // Mouse cursor position relative to captured window (when _StitchAll is in use).
    ImGuiWindow*            _HoveredWindow = NULL;          // Window which was hovered at capture start.
    ImGuiCaptureImageBuf    _CaptureBuf;                    // Output image buffer.
    const ImGuiCaptureArgs* _CaptureArgs = NULL;            // Current capture args. Set only if capture is in progress.
    bool                    _MouseDrawCursorBackup = false; // Initial value of g.IO.MouseDrawCursor.

    // [Internal] Video recording
    bool                    _VideoRecording = false;        // Flag indicating that video recording is in progress.
    double                  _VideoLastFrameTime = 0;        // Time when last video frame was recorded.
    FILE*                   _VideoFFMPEGPipe = NULL;        // File writing to stdin of ffmpeg process.

    // [Internal] Backups
    ImVector<ImGuiWindow*>  _BackupWindows;                 // Backup windows that will have their rect modified and restored. args->InCaptureWindows can not be used because popups may get closed during capture and no longer appear in that list.
    ImVector<ImRect>        _BackupWindowsRect;             // Backup window state that will be restored when screen capturing is done. Size and order matches windows of ImGuiCaptureArgs::InCaptureWindows.
    ImVec2                  _BackupDisplayWindowPadding;    // Backup padding. We set it to {0, 0} during capture.
    ImVec2                  _BackupDisplaySafeAreaPadding;  // Backup padding. We set it to {0, 0} during capture.

    //-------------------------------------------------------------------------
    // Functions
    //-------------------------------------------------------------------------

    ImGuiCaptureContext(ImGuiScreenCaptureFunc capture_func = NULL) { ScreenCaptureFunc = capture_func; _MouseRelativeToWindowPos = ImVec2(-FLT_MAX, -FLT_MAX); }

    // Should be called after ImGui::NewFrame() and before submitting any UI.
    // (ImGuiTestEngine automatically calls that for you, so this only apply to independently created instance)
    void                    PostNewFrame();

    // Update capturing. If this function returns true then it should be called again with same arguments on the next frame.
    ImGuiCaptureStatus      CaptureUpdate(ImGuiCaptureArgs* args);

    // Begin video capture. Call CaptureUpdate() every frame afterwards until it returns false.
    void                    BeginVideoCapture(ImGuiCaptureArgs* args);
    void                    EndVideoCapture();
    bool                    IsCapturingVideo();
};

// Implements UI for capturing images
// (when using ImGuiTestEngine scripting API you may not need to use this at all)
struct ImGuiCaptureToolUI
{
    ImGuiCaptureContext     Context;                                // Screenshot capture context.
    float                   SnapGridSize = 32.0f;                   // Size of the grid cell for "snap to grid" functionality.
    char                    LastOutputFileName[256] = "";           // File name of last captured file.

    ImGuiCaptureArgs        _CaptureArgs;                           // Capture args
    ImGuiCaptureToolState   _CaptureState = ImGuiCaptureToolState_None; // Which capture function is in progress.
    ImVector<ImGuiID>       _SelectedWindows;

    // Public
    ImGuiCaptureToolUI();
    void    ShowCaptureToolWindow(bool* p_open = NULL);             // Render a capture tool window with various options and utilities.
    void    SetCaptureFunc(ImGuiScreenCaptureFunc capture_func);

    // [Internal]
    void    CaptureWindowPicker(ImGuiCaptureArgs* args);            // Render a window picker that captures picked window to file specified in file_name.
    void    CaptureWindowsSelector(ImGuiCaptureArgs* args);         // Render a selector for selecting multiple windows for capture.
    void    SnapWindowsToGrid(float cell_size, float padding);      // Snaps edges of all visible windows to a virtual grid.
};
