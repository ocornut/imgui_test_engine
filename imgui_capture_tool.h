#pragma once

enum CaptureWindowFlags_
{
    CaptureWindowFlags_Default = 1 << 0,            //
    CaptureWindowFlags_FullSize = 1 << 1,           // Expand window to it's content size and capture it's full height.
    CaptureWindowFlags_IgnoreCurrentWindow = 1 << 2,// Current window will not appear in screenshots or helper UI.
};
using CaptureWindowFlags = unsigned;

// Helper class for simple bitmap manipulation (not particularly efficient!)
struct ImageBuf
{
    typedef unsigned int u32;

    int             Width, Height;
    u32*            Data;

    ImageBuf()      { Width = Height = 0; Data = NULL; }
    ~ImageBuf()     { Clear(); }

    // Free allocated memory buffer if such exists.
    void Clear();
    // Reallocate buffer for pixel data, and zero it.
    void CreateEmpty(int w, int h);
    // Reallocate buffer for pixel data, but do not zero memory buffer.
    void CreateEmptyNoMemClear(int w, int h);
    // Save pixel data to specified file.
    void SaveFile(const char* filename);
    // Clear alpha channel from all pixels.
    void RemoveAlpha();
};

struct ImGuiCaptureTool
{
    enum ScreenshotCaptureType
    {
        CaptureType_None,                                  // No capture in progress.
        CaptureType_MultipleWindows,                       // Capture of multiple windows in progress.
        CaptureType_SelectRectStart,                       // Next mouse click will create selection rectangle.
        CaptureType_SelectRectUpdate,                      // Update selection rectangle until mouse is released.
    };
    CaptureWindowFlags    Flags;                           // Optional. Flags for customizing behavior of screenshot tool.
    bool                  (*CaptureFramebuffer)(           // Required. Graphics-backend-specific function that captures specified portion of framebuffer and writes RGBA data to `pixels` buffer. UserData will be passed to this function as `user` parameter.
        int x, int y, int w, int h, unsigned* pixels, void* user);
    float                 Padding;                         // Optional Extra padding at the edges of the screenshot.
    const char*           SaveFileName;                    // Required. File name to which captured image will be saved.
    void*                 UserData;                        // Optional. Custom user pointer.

    ImRect                _CaptureRect;                    // Viewport rect that is being captured.
    ScreenshotCaptureType _CaptureType;                    // Hint used to determine which capture function is in progress. This flag is not set for some capture types that dont span multiple frames.
    int                   _Chunk;                          // Number of chunk that is being captured when capture spans multiple frames.
    int                   _Frame;                          // Frame number during capture process that spans multiple frames.
    ImageBuf              _Output;                         // Output image buffer.
    ImGuiWindow*          _Window;                         // Window to be captured.
    float                 _WindowNameMaxPosX;              // X post after longest window name in CaptureWindowsSelector().
    ImRect                _WindowState;                    // Backup window state that will be restored when screen capturing is done.

    ImGuiCaptureTool(bool (*CaptureFramebufferFunc)(int, int, int, int, unsigned*, void*)=NULL)
    {
        Flags = CaptureWindowFlags_Default;
        CaptureFramebuffer = CaptureFramebufferFunc;
        Padding = 13.f;
        SaveFileName = NULL;
        UserData = NULL;

        _Frame = 0;
        _CaptureRect = ImRect();
        _CaptureType = CaptureType_None;
        _Chunk = 0;
        _Window = NULL;
        _WindowNameMaxPosX = 170.f;
        _WindowState = ImRect();
    }

    // Capture a specific window into file specified in file_name. When this function returns `true` it must be called
    // on the next frame as well. Capture process for windows that do not fit into the screen take multiple frames.
    bool CaptureWindow(ImGuiWindow* window);
    // Capture a specified portion of main viewport framebuffer and save it.
    void CaptureRect(const ImRect& rect);
    // Render a window picker that captures picked window to file specified in file_name.
    void CaptureWindowPicker(const char* title);
    // Render a selector for selecting multiple windows for capture.
    void CaptureWindowsSelector(const char* title);
    // Render a screenshot maker window with various options and utilities.
    void ScreenshotMaker(bool* open = NULL);
    // Snaps edges of all visible windows to a virtual grid.
    void SnapWindowsToGrid(float cell_size);

private:
    // Returns true when capture is in progress.
    bool _CaptureUpdate();
};
