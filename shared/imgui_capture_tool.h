#pragma once

//-----------------------------------------------------------------------------
// Forward declarations and basic types
//-----------------------------------------------------------------------------

struct ImGuiCaptureArgs;
struct ImGuiCaptureContext;
struct ImGuiCaptureImageBuf;
struct ImGuiCaptureTool;
typedef unsigned int ImGuiCaptureFlags;

struct ImGuiWindow;

//-----------------------------------------------------------------------------

struct GifWriter;

// [Internal]
// Helper class for simple bitmap manipulation (not particularly efficient!)
struct ImGuiCaptureImageBuf
{
    int             Width;
    int             Height;
    unsigned int*   Data;

    ImGuiCaptureImageBuf()      { Width = Height = 0; Data = NULL; }
    ~ImGuiCaptureImageBuf()     { Clear(); }

    void Clear();                                           // Free allocated memory buffer if such exists.
    void CreateEmpty(int w, int h);                         // Reallocate buffer for pixel data, and zero it.
    void CreateEmptyNoMemClear(int w, int h);               // Reallocate buffer for pixel data, but do not zero memory buffer.
    bool SaveFile(const char* filename);                    // Save pixel data to specified file.
    void RemoveAlpha();                                     // Clear alpha channel from all pixels.
    void BlitSubImage(int dst_x, int dst_y, int src_x, int src_y, int w, int h, const ImGuiCaptureImageBuf* source);
};

typedef bool (*ImGuiScreenCaptureFunc)(int x, int y, int w, int h, unsigned int* pixels, void* user_data);

enum ImGuiCaptureFlags_ : unsigned int
{
    ImGuiCaptureFlags_None                      = 0,        //
    ImGuiCaptureFlags_StitchFullContents        = 1 << 1,   // Expand window to it's content size and capture its full height.
    ImGuiCaptureFlags_HideCaptureToolWindow     = 1 << 2,   // Current window will not appear in screenshots or helper UI.
    ImGuiCaptureFlags_ExpandToIncludePopups     = 1 << 3,   // Expand capture area to automatically include visible popups and tooltips.
    ImGuiCaptureFlags_Default_                  = ImGuiCaptureFlags_StitchFullContents | ImGuiCaptureFlags_HideCaptureToolWindow
};

enum ImGuiCaptureToolState : unsigned int
{
    ImGuiCaptureToolState_None,                             // No capture in progress.
    ImGuiCaptureToolState_PickingSingleWindow,              // CaptureWindowPicker() is selecting a window under mouse cursor.
    ImGuiCaptureToolState_SelectRectStart,                  // Next mouse click will create selection rectangle.
    ImGuiCaptureToolState_SelectRectUpdate,                 // Update selection rectangle until mouse is released.
    ImGuiCaptureToolState_Capturing                         // Capture is in progress.
};

// Defines input and output arguments for capture process.
struct ImGuiCaptureArgs
{
    // [Input]
    ImGuiCaptureFlags       InFlags = 0;                    // Flags for customizing behavior of screenshot tool.
    ImVector<ImGuiWindow*>  InCaptureWindows;               // Windows to capture. All other windows will be hidden. May be used with InCaptureRect to capture only some windows in specified rect.
    ImRect                  InCaptureRect;                  // Screen rect to capture. Does not include padding.
    float                   InPadding = 10.0f;              // Extra padding at the edges of the screenshot. Ensure that there is available space around capture rect horizontally, also vertically if ImGuiCaptureFlags_StitchFullContents is not used.
    int                     InFileCounter = 0;              // Counter which may be appended to file name when saving. By default counting starts from 1. When done this field holds number of saved files.
    ImGuiCaptureImageBuf*   InOutputImageBuf = NULL;        // Output will be saved to image buffer if specified.
    char                    InOutputFileTemplate[256] = ""; // Output will be saved to a file if InOutputImageBuf is NULL.
    int                     InRecordFPSTarget = 25;         // FPS target for recording gifs.

    // [Output]
    ImVec2                  OutImageSize;                   // Produced image size.
    char                    OutSavedFileName[256] = "";     // Saved file name, if any.

    // [Internal]
    bool                    _Capturing = false;             // FIXME-TESTS: ???
};

enum ImGuiCaptureToolStatus
{
    ImGuiCaptureToolStatus_InProgress,
    ImGuiCaptureToolStatus_Done,
    ImGuiCaptureToolStatus_Error,
};

// Implements functionality for capturing images
struct ImGuiCaptureContext
{
    ImGuiScreenCaptureFunc  ScreenCaptureFunc = NULL;       // Graphics-backend-specific function that captures specified portion of framebuffer and writes RGBA data to `pixels` buffer.
    void*                   ScreenCaptureUserData = NULL;   // Custom user pointer which is passed to ScreenCaptureFunc. (Optional)

    // [Internal]
    ImRect                  _CaptureRect;                   // Viewport rect that is being captured.
    ImVec2                  _CombinedWindowRectPos;         // Top-left corner of region that covers all windows included in capture. This is not same as _CaptureRect.Min when capturing explicitly specified rect.
    ImGuiCaptureImageBuf    _Output;                        // Output image buffer.
    int                     _ChunkNo = 0;                   // Number of chunk that is being captured when capture spans multiple frames.
    int                     _FrameNo = 0;                   // Frame number during capture process that spans multiple frames.
    ImVector<ImRect>        _WindowBackupRects;             // Backup window state that will be restored when screen capturing is done. Size and order matches windows of ImGuiCaptureArgs::InCaptureWindows.
    ImVector<ImGuiWindow*>  _WindowBackupRectsWindows;      // Backup windows that will have their state restored. args->InCaptureWindows can not be used because popups may get closed during capture and no longer appear in that list.
    ImVec2                  _DisplayWindowPaddingBackup;    // Backup padding. We set it to {0, 0} during capture.
    ImVec2                  _DisplaySafeAreaPaddingBackup;  // Backup padding. We set it to {0, 0} during capture.
    bool                    _Recording = false;             // Flag indicating that GIF recording is in progress.
    double                  _LastRecordedFrameTimeSec = 0;  // Time when last GIF frame was recorded.
    GifWriter*              _GifWriter = NULL;              // GIF image writer state.

    ImGuiCaptureContext(ImGuiScreenCaptureFunc capture_func = NULL) { ScreenCaptureFunc = capture_func; }

    // Capture a screenshot. If this function returns true then it should be called again with same arguments on the next frame.
    ImGuiCaptureToolStatus CaptureUpdate(ImGuiCaptureArgs* args);

    // Begin gif capture. args->InOutputFileTemplate must be specified. Call CaptureUpdate() every frame afterwards until it returns false.
    void    BeginGifCapture(ImGuiCaptureArgs* args);
    void    EndGifCapture(ImGuiCaptureArgs* args);
    bool    IsCapturingGif();
};

// Implements UI for capturing images
struct ImGuiCaptureTool
{
    ImGuiCaptureContext     Context;                        // Screenshot capture context.
    ImGuiCaptureFlags       Flags = ImGuiCaptureFlags_Default_; // Customize behavior of screenshot capture process. Flags are used by both ImGuiCaptureTool and ImGuiCaptureContext.
    bool                    Visible = false;                // Tool visibility state.
    float                   Padding = 10.0f;                // Extra padding around captured area.
    char                    SaveFileName[256];              // File name where screenshots will be saved. May contain directories or variation of %d format.
    float                   SnapGridSize = 32.0f;           // Size of the grid cell for "snap to grid" functionality.
    char                    LastSaveFileName[256];          // File name of last captured file.

    ImGuiCaptureArgs        _CaptureArgsPicker;             // Capture args for single window picker widget.
    ImGuiCaptureArgs        _CaptureArgsSelector;           // Capture args for multiple window selector widget.
    ImGuiCaptureToolState   _CaptureState = ImGuiCaptureToolState_None; // Which capture function is in progress.
    float                   _WindowNameMaxPosX = 170.0f;    // X post after longest window name in CaptureWindowsSelector().

    ImGuiCaptureTool(ImGuiScreenCaptureFunc capture_func = NULL);

    // Render a window picker that captures picked window to file specified in file_name.
    void    CaptureWindowPicker(const char* title, ImGuiCaptureArgs* args);

    // Render a selector for selecting multiple windows for capture.
    void    CaptureWindowsSelector(const char* title, ImGuiCaptureArgs* args);

    // Render a capture tool window with various options and utilities.
    void    ShowCaptureToolWindow(bool* p_open = NULL);

    // Snaps edges of all visible windows to a virtual grid.
    void    SnapWindowsToGrid(float cell_size);
};
