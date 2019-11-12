#pragma once

// Helper class for simple bitmap manipulation (not particularly efficient!)
struct ImageBuf
{
    typedef unsigned int u32;

    int             Width, Height;
    u32*            Data;

    ImageBuf()      { Width = Height = 0; Data = NULL; }
    ~ImageBuf()     { Clear(); }
    
    void Clear();                               // Free allocated memory buffer if such exists.
    void CreateEmpty(int w, int h);             // Reallocate buffer for pixel data, and zero it.
    void CreateEmptyNoMemClear(int w, int h);   // Reallocate buffer for pixel data, but do not zero memory buffer.
    void SaveFile(const char* filename);        // Save pixel data to specified file.
    void RemoveAlpha();                         // Clear alpha channel from all pixels.
};

typedef bool (*ImGuiScreenCaptureFunc)(int x, int y, int w, int h, unsigned int* pixels, void* user_data);

enum ImGuiCaptureToolFlags_
{
    ImGuiCaptureToolFlags_None                      = 0,        //
    ImGuiCaptureToolFlags_StitchFullContents        = 1 << 1,   // Expand window to it's content size and capture its full height.
    ImGuiCaptureToolFlags_IgnoreCaptureToolWindow   = 1 << 2    // Current window will not appear in screenshots or helper UI.
};

typedef unsigned int ImGuiCaptureToolFlags;

enum ImGuiCaptureToolState
{
    ImGuiCaptureToolState_None,                     // No capture in progress.
    ImGuiCaptureToolState_PickingSingleWindow,      //
    ImGuiCaptureToolState_CapturingSingleWIndow,    // Capture in progress

    ImGuiCaptureToolState_MultipleWindows,          // Capture of multiple windows in progress.
    ImGuiCaptureToolState_SelectRectStart,          // Next mouse click will create selection rectangle.
    ImGuiCaptureToolState_SelectRectUpdate          // Update selection rectangle until mouse is released.
};

struct ImGuiCaptureTool
{
    bool                    Visible;                // Tool visibility state
    ImGuiCaptureToolFlags   Flags;                  // Flags for customizing behavior of screenshot tool.
    ImGuiScreenCaptureFunc  ScreenCaptureFunc;      // Graphics-backend-specific function that captures specified portion of framebuffer and writes RGBA data to `pixels` buffer. UserData will be passed to this function as `user` parameter.
    float                   Padding;                // Extra padding at the edges of the screenshot.
    float                   SnapGridSize;           //
    char                    SaveFileName[256];      // File name to which captured image will be saved.
    void*                   UserData;               // Custom user pointer. (Optional)

    ImRect                  _CaptureRect;           // Viewport rect that is being captured.
    ImGuiCaptureToolState   _CaptureState;          // Hint used to determine which capture function is in progress. This flag is not set for some capture types that dont span multiple frames.
    int                     _ChunkNo;               // Number of chunk that is being captured when capture spans multiple frames.
    int                     _FrameNo;               // Frame number during capture process that spans multiple frames.
    ImageBuf                _Output;                // Output image buffer.
    ImGuiWindow*            _Window;                // Window to be captured.
    float                   _WindowNameMaxPosX;     // X post after longest window name in CaptureWindowsSelector().
    ImRect                  _WindowBackupRect;      // Backup window state that will be restored when screen capturing is done.

    ImGuiCaptureTool(ImGuiScreenCaptureFunc capture_func = NULL);

    // Capture a specific window into file specified in file_name. When this function returns `true` it must be called
    // on the next frame as well. Capture process for windows that do not fit into the screen take multiple frames.
    void    CaptureWindowStart(ImGuiWindow* window);
    bool    CaptureWindowUpdate();

    // Capture a specified portion of main viewport framebuffer and save it.
    void    CaptureRect(const ImRect& rect);

    // Render a window picker that captures picked window to file specified in file_name.
    void    CaptureWindowPicker(const char* title);

    // Render a selector for selecting multiple windows for capture.
    void    CaptureWindowsSelector(const char* title);

    // Render a screenshot maker window with various options and utilities.
    void    ShowCaptureToolWindow(bool* p_open = NULL);

    // Snaps edges of all visible windows to a virtual grid.
    void    SnapWindowsToGrid(float cell_size);

    // [Internal]
    void    ShowWindowList();
    bool    CaptureUpdate();   // Returns true when capture is in progress.
};
