//-----------------------------------------------------------------------------
// Simple Dear ImGui App Framework (using standard backends)
//-----------------------------------------------------------------------------
// THIS IS FOR OUR OWN USE AND IS NOT SUPPORTED.
// IF YOU USE IT PLEASE DON'T ASK QUESTIONS. WE MAY CHANGE API AT ANY TIME.
//-----------------------------------------------------------------------------
// Example usage:
/*
    #include "imgui_app.h"
    ImGuiApp* app = ImGuiApp_ImplDefault_Create();
    app->DpiAware = true;
    app->Vsync = true;
    app->InitCreateWindow(app, "My Application", ImVec2(1600, 1200));
    app->InitBackends(app);
    while (true)
    {
        if (!app->NewFrame(app))
            break;
        ImGui::NewFrame();
        [...]
        ImGui::Render();
        app->Render(app);
    }
    app->ShutdownBackends(app);
    app->ShutdownCloseWindow(app);
    ImGui::DestroyContext();
    app->Destroy(app);
*/
//-----------------------------------------------------------------------------

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef IMGUI_APP_IMPLEMENTATION
#define IMGUI_APP_IMPLEMENTATION 1
#endif

#include "imgui_app.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <chrono>   // time_since_epoch
#ifdef __linux__
#include <unistd.h> // sleep
#endif

/*

Index of this file:

// [SECTION] Defines
// [SECTION] ImGuiApp Implementation: NULL
// [SECTION] ImGuiApp Implementation: Win32 + DX11
// [SECTION] ImGuiApp Implementation: SDL + OpenGL2
// [SECTION] ImGuiApp Implementation: SDL + OpenGL3
// [SECTION] ImGuiApp Implementation: GLFW + OpenGL3
// [SECTION] ImGuiApp Implementation: OpenGL (Shared)

*/

//-----------------------------------------------------------------------------
// [SECTION] Defines
//-----------------------------------------------------------------------------

#ifdef IMGUI_APP_WIN32_DX11
#define IMGUI_APP_WIN32
#define IMGUI_APP_DX11
#endif

#ifdef IMGUI_APP_SDL2_GL2
#define IMGUI_APP_SDL2
#define IMGUI_APP_GL2
#endif

#ifdef IMGUI_APP_SDL2_GL3
#define IMGUI_APP_SDL2
#define IMGUI_APP_GL3
#endif

#ifdef IMGUI_APP_GLFW_GL3
#define IMGUI_APP_GLFW
#define IMGUI_APP_GL3
#endif

// Forward declarations
#if defined(IMGUI_APP_GL2) || defined(IMGUI_APP_GL3)
static bool ImGuiApp_ImplGL_CaptureFramebuffer(ImGuiApp* app, ImGuiViewport* viewport, int x, int y, int w, int h, unsigned int* pixels, void* user_data);
#endif

// Linking
#if defined(_MSC_VER) && defined(IMGUI_APP_SDL2)
#pragma comment(lib, "sdl2")      // Link with sdl2.lib. MinGW will require linking with '-lsdl2'
#pragma comment(lib, "sdl2main")  // Link with sdl2main.lib. MinGW will require linking with '-lsdl2main'
#endif
#if defined(_MSC_VER) && defined(IMGUI_APP_GL2)
#pragma comment(lib, "opengl32")  // Link with opengl32.lib. MinGW will require linking with '-lopengl32'
#endif

//-----------------------------------------------------------------------------
// [SECTION] Helper stub to store directly in ImGuiTestEngineIO::ScreenCaptureFunc when using test engine (prototype is same as ImGuiScreenCaptureFunc)
//-----------------------------------------------------------------------------

bool ImGuiApp_ScreenCaptureFunc(ImGuiID viewport_id, int x, int y, int w, int h, unsigned int* pixels, void* user_data)
{
    ImGuiApp* app = (ImGuiApp*)user_data;
    if (app->CaptureFramebuffer == NULL)
        return false;
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiViewport* viewport = ImGui::FindViewportByID(viewport_id);
#else
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    IM_UNUSED(viewport_id);
#endif
    IM_ASSERT(viewport != NULL);
    return app->CaptureFramebuffer(app, viewport, x, y, w, h, pixels, NULL);
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiApp Implementation: NULL
//-----------------------------------------------------------------------------

#ifdef IMGUI_HAS_VIEWPORT
static void ImGuiApp_InstalMockViewportsBackend(ImGuiApp*);
#endif

// Data
struct ImGuiApp_ImplNull : public ImGuiApp
{
    ImU64 LastTime = 0;
};

// Functions
static void ImGuiApp_ImplNull_InitBackends(ImGuiApp* app)
{
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiIO& io = ImGui::GetIO();
    if (app->MockViewports && (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
        ImGuiApp_InstalMockViewportsBackend(app);
#else
    IM_UNUSED(app);
#endif
}

static bool ImGuiApp_ImplNull_CreateWindow(ImGuiApp* app, const char*, ImVec2 size)
{
    IM_UNUSED(app);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = size;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_HasMouseCursors;
    //io.Fonts->Build();
#if IMGUI_VERSION_NUM < 18603
    for (int n = 0; n < ImGuiKey_COUNT; n++)
        io.KeyMap[n] = n;
#endif

    return true;
}

static uint64_t ImGuiApp_GetTimeInMicroseconds()
{
    using namespace std;
    chrono::microseconds ms = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch());
    return (uint64_t)ms.count();
}

static bool ImGuiApp_ImplNull_NewFrame(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplNull* app = (ImGuiApp_ImplNull*)app_opaque;
    ImGuiIO& io = ImGui::GetIO();

    //unsigned char* pixels = NULL;
    //int width = 0;
    //int height = 0;
    //io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    uint64_t time = ImGuiApp_GetTimeInMicroseconds();
    if (app->LastTime == 0)
        app->LastTime = time;
    io.DeltaTime = (float)((double)(time - app->LastTime) / 1000000.0);
    if (io.DeltaTime <= 0.0f)
        io.DeltaTime = 0.000001f;
    app->LastTime = time;

    return true;
}

static bool ImGuiApp_ImplNull_CaptureFramebuffer(ImGuiApp* app, ImGuiViewport* viewport, int x, int y, int w, int h, unsigned int* pixels, void* user_data)
{
    IM_UNUSED(viewport);
    IM_UNUSED(app);
    IM_UNUSED(user_data);
    IM_UNUSED(x);
    IM_UNUSED(y);
    memset(pixels, 0, (size_t)(w * h) * sizeof(unsigned int));
    return false;
}

static void ImGuiApp_ImplNull_RenderDrawData(ImDrawData* draw_data)
{
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL)
            {
                if (pcmd->UserCallback != ImDrawCallback_ResetRenderState)
                    pcmd->UserCallback(cmd_list, pcmd);
            }
        }
    }
}

static void ImGuiApp_ImplNull_Render(ImGuiApp* app_opaque)
{
    IM_UNUSED(app_opaque);
    ImDrawData* draw_data = ImGui::GetDrawData();

#ifdef IMGUI_HAS_VIEWPORT
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
#endif

    ImGuiApp_ImplNull_RenderDrawData(draw_data);
}

ImGuiApp* ImGuiApp_ImplNull_Create()
{
    ImGuiApp_ImplNull* intf = new ImGuiApp_ImplNull();
    intf->InitCreateWindow      = ImGuiApp_ImplNull_CreateWindow;
    intf->InitBackends          = ImGuiApp_ImplNull_InitBackends;
    intf->NewFrame              = ImGuiApp_ImplNull_NewFrame;
    intf->Render                = ImGuiApp_ImplNull_Render;
    intf->ShutdownCloseWindow   = [](ImGuiApp* app) { IM_UNUSED(app); };
    intf->ShutdownBackends      = [](ImGuiApp* app) { IM_UNUSED(app); };
    intf->CaptureFramebuffer    = ImGuiApp_ImplNull_CaptureFramebuffer;
    intf->Destroy               = [](ImGuiApp* app) { delete (ImGuiApp_ImplNull*)app; };
    return intf;
}


//-----------------------------------------------------------------------------
// [SECTION] ImGuiApp Implementation: Win32 + DX11
//-----------------------------------------------------------------------------

#ifdef IMGUI_APP_WIN32_DX11

// Include
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <stdlib.h>

// Data
struct ImGuiApp_ImplWin32DX11 : public ImGuiApp
{
    HWND                    Hwnd = NULL;
    WNDCLASSEXW             WC = {};
    ID3D11Device*           pd3dDevice = NULL;
    ID3D11DeviceContext*    pd3dDeviceContext = NULL;
    IDXGISwapChain*         pSwapChain = NULL;
    ID3D11RenderTargetView* mainRenderTargetView = NULL;
};

// Forward declarations of helper functions
static bool CreateDeviceD3D(ImGuiApp_ImplWin32DX11* app);
static void CleanupDeviceD3D(ImGuiApp_ImplWin32DX11* app);
static void CreateRenderTarget(ImGuiApp_ImplWin32DX11* app);
static void CleanupRenderTarget(ImGuiApp_ImplWin32DX11* app);
static ImGuiApp* g_AppForWndProc = NULL;
static LRESULT WINAPI ImGuiApp_ImplWin32_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool ImGuiApp_ImplWin32DX11_InitCreateWindow(ImGuiApp* app_opaque, const char* window_title_a, ImVec2 window_size)
{
    ImGuiApp_ImplWin32DX11* app = (ImGuiApp_ImplWin32DX11*)app_opaque;
    if (app->DpiAware)
        ImGui_ImplWin32_EnableDpiAwareness();

    // Create application window
    app->WC = { sizeof(app->WC), CS_CLASSDC, ImGuiApp_ImplWin32_WndProc, 0L, 0L, ::GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGuiApp", NULL };
    ::RegisterClassExW(&app->WC);

    POINT pos = { 1, 1 };
    HMONITOR monitor = ::MonitorFromPoint(pos, 0);
    float dpi_scale = app->DpiAware ? ImGui_ImplWin32_GetDpiScaleForMonitor(monitor) : 1.0f;
    window_size.x = ImFloor(window_size.x * dpi_scale);
    window_size.y = ImFloor(window_size.y * dpi_scale);

    // Center in monitor
    MONITORINFO monitor_info = { };
    monitor_info.cbSize = sizeof(MONITORINFO);
    if (::GetMonitorInfo(monitor, &monitor_info))
    {
        pos.x = monitor_info.rcWork.left + ImMax((LONG)0, ((monitor_info.rcWork.right - monitor_info.rcWork.left) - (LONG)window_size.x) / 2);
        pos.y = monitor_info.rcWork.top + ImMax((LONG)0, ((monitor_info.rcWork.bottom - monitor_info.rcWork.top) - (LONG)window_size.y) / 2);
    }

    const int count = ::MultiByteToWideChar(CP_UTF8, 0, window_title_a, -1, NULL, 0);
    WCHAR* window_title_t = (WCHAR*)calloc(count, sizeof(WCHAR));
    if (!::MultiByteToWideChar(CP_UTF8, 0, window_title_a, -1, window_title_t, count))
    {
        free(window_title_t);
        return false;
    }
    app->Hwnd = ::CreateWindowExW(0L, app->WC.lpszClassName, window_title_t, WS_OVERLAPPEDWINDOW, pos.x, pos.y, (int)window_size.x, (int)window_size.y, NULL, NULL, app->WC.hInstance, NULL);
    free(window_title_t);

    // Initialize Direct3D
    if (!CreateDeviceD3D(app))
    {
        CleanupDeviceD3D(app);
        ::UnregisterClassW(app->WC.lpszClassName, app->WC.hInstance);
        return 1;
    }

    // Show the window
    g_AppForWndProc = app;
    ::ShowWindow(app->Hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(app->Hwnd);
    g_AppForWndProc = NULL;

    app->DpiScale = app->DpiAware ? ImGui_ImplWin32_GetDpiScaleForHwnd(app->Hwnd) : 1.0f;

    return true;
}

static void ImGuiApp_ImplWin32DX11_ShutdownCloseWindow(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplWin32DX11* app = (ImGuiApp_ImplWin32DX11*)app_opaque;
    CleanupDeviceD3D(app);
    ::DestroyWindow(app->Hwnd);
    ::UnregisterClassW(app->WC.lpszClassName, app->WC.hInstance);
}

static void ImGuiApp_ImplWin32DX11_InitBackends(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplWin32DX11* app = (ImGuiApp_ImplWin32DX11*)app_opaque;
    ImGui_ImplWin32_Init(app->Hwnd);
    ImGui_ImplDX11_Init(app->pd3dDevice, app->pd3dDeviceContext);
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiIO& io = ImGui::GetIO();
    if (app->MockViewports && (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
        ImGuiApp_InstalMockViewportsBackend(app);
#endif
}

static void ImGuiApp_ImplWin32DX11_ShutdownBackends(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplWin32DX11* app = (ImGuiApp_ImplWin32DX11*)app_opaque;
    IM_UNUSED(app);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

static bool ImGuiApp_ImplWin32DX11_NewFrame(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplWin32DX11* app = (ImGuiApp_ImplWin32DX11*)app_opaque;
    app->DpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(app->Hwnd);

    g_AppForWndProc = app;
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
        {
            g_AppForWndProc = NULL;
            return false;
        }
    }
    g_AppForWndProc = NULL;

    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();

    return true;
}

static void ImGuiApp_ImplWin32DX11_Render(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplWin32DX11* app = (ImGuiApp_ImplWin32DX11*)app_opaque;

    // Update and Render additional Platform Windows
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
#endif

    // Render main viewport
    app->pd3dDeviceContext->OMSetRenderTargets(1, &app->mainRenderTargetView, NULL);
    app->pd3dDeviceContext->ClearRenderTargetView(app->mainRenderTargetView, (float*)&app->ClearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Present
    if (app->Vsync)
        app->pSwapChain->Present(1, 0);
    else
        app->pSwapChain->Present(0, 0);
}

static bool ImGuiApp_ImplWin32DX11_CaptureFramebuffer(ImGuiApp* app_opaque, ImGuiViewport* viewport, int x, int y, int w, int h, unsigned int* pixels_rgba, void* user_data)
{
    IM_UNUSED(user_data);
    ImGuiApp_ImplWin32DX11* app = (ImGuiApp_ImplWin32DX11*)app_opaque;
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    ID3D11RenderTargetView* rtView = app->mainRenderTargetView;
    UINT rtWidth = (UINT)io.DisplaySize.x;
    UINT rtHeight = (UINT)io.DisplaySize.y;
#ifdef IMGUI_HAS_VIEWPORT
    if (viewport != NULL)
        if (ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData)
        {
            rtView = vd->RTView;
            rtWidth = (UINT)viewport->Size.x;
            rtHeight = (UINT)viewport->Size.y;
        }
#else
    IM_UNUSED(viewport);
#endif

    D3D11_TEXTURE2D_DESC texture_desc;
    memset(&texture_desc, 0, sizeof(texture_desc));
    texture_desc.Width = rtWidth;
    texture_desc.Height = rtHeight;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* texture = NULL;
    HRESULT hr = app->pd3dDevice->CreateTexture2D(&texture_desc, NULL, &texture);
    if (FAILED(hr))
    {
        if (texture)
        {
            texture->Release();
            texture = nullptr;
        }
        return false;
    }

    ID3D11Resource* source = NULL;
    rtView->GetResource(&source);
    app->pd3dDeviceContext->CopyResource(texture, source);
    source->Release();

    D3D11_MAPPED_SUBRESOURCE mapped;
    mapped.pData = NULL;
    hr = app->pd3dDeviceContext->Map(texture, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr) || !mapped.pData)
    {
        texture->Release();
        return false;
    }

    // D3D11 does not provide means to capture a partial screenshot. We copy rect x,y,w,h on CPU side.
    for (int index_y = y; index_y < y + h; ++index_y)
    {
        unsigned int* src = (unsigned int*)((unsigned char*)mapped.pData + index_y * mapped.RowPitch) + x;
        unsigned int* dst = &pixels_rgba[(index_y - y) * w];
        memcpy(dst, src, w * 4);
    }

    app->pd3dDeviceContext->Unmap(texture, 0);
    texture->Release();
    return true;
}

static void ImGuiApp_ImplWin32DX11_Destroy(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplWin32DX11* app = (ImGuiApp_ImplWin32DX11*)app_opaque;
    delete app;
}

ImGuiApp* ImGuiApp_ImplWin32DX11_Create()
{
    ImGuiApp* intf = new ImGuiApp_ImplWin32DX11();
    intf->InitCreateWindow = ImGuiApp_ImplWin32DX11_InitCreateWindow;
    intf->InitBackends = ImGuiApp_ImplWin32DX11_InitBackends;
    intf->NewFrame = ImGuiApp_ImplWin32DX11_NewFrame;
    intf->Render = ImGuiApp_ImplWin32DX11_Render;
    intf->ShutdownCloseWindow = ImGuiApp_ImplWin32DX11_ShutdownCloseWindow;
    intf->ShutdownBackends = ImGuiApp_ImplWin32DX11_ShutdownBackends;
    intf->CaptureFramebuffer = ImGuiApp_ImplWin32DX11_CaptureFramebuffer;
    intf->Destroy = ImGuiApp_ImplWin32DX11_Destroy;
    return intf;
}

// Helper functions
static bool CreateDeviceD3D(ImGuiApp_ImplWin32DX11* app)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = app->SrgbFramebuffer ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = app->Hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &app->pSwapChain, &app->pd3dDevice, &featureLevel, &app->pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget(app);
    return true;
}

static void CleanupDeviceD3D(ImGuiApp_ImplWin32DX11* app)
{
    CleanupRenderTarget(app);
    if (app->pSwapChain)        { app->pSwapChain->Release(); app->pSwapChain = NULL; }
    if (app->pd3dDeviceContext) { app->pd3dDeviceContext->Release(); app->pd3dDeviceContext = NULL; }
    if (app->pd3dDevice)        { app->pd3dDevice->Release(); app->pd3dDevice = NULL; }
}

static void CreateRenderTarget(ImGuiApp_ImplWin32DX11* app)
{
    ID3D11Texture2D* pBackBuffer = NULL;
    app->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    app->pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &app->mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget(ImGuiApp_ImplWin32DX11* app)
{
    if (app->mainRenderTargetView) { app->mainRenderTargetView->Release(); app->mainRenderTargetView = NULL; }
}

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT WINAPI ImGuiApp_ImplWin32_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    ImGuiApp_ImplWin32DX11* app = (ImGuiApp_ImplWin32DX11*)g_AppForWndProc;
    switch (msg)
    {
    case WM_SIZE:
        if (app->pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget(app);
            app->pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget(app);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

#endif // #ifdef IMGUI_APP_WIN32_DX11

//-----------------------------------------------------------------------------
// [SECTION] ImGuiApp Implementation: OpenGL2 bindings
//-----------------------------------------------------------------------------

#ifdef IMGUI_APP_GL2
#include "imgui_impl_opengl2.h"

// Include OpenGL header (without an OpenGL loader) requires a bit of fiddling
#if defined(_WIN32) && !defined(APIENTRY)
#define APIENTRY __stdcall                  // It is customary to use APIENTRY for OpenGL function pointer declarations on all platforms.  Additionally, the Windows OpenGL header needs APIENTRY.
#endif
#if defined(_WIN32) && !defined(WINGDIAPI)
#define WINGDIAPI __declspec(dllimport)     // Some Windows OpenGL headers need this
#endif
#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#endif

//-----------------------------------------------------------------------------
// [SECTION] ImGuiApp Implementation: OpenGL3 bindings
//-----------------------------------------------------------------------------

#ifdef IMGUI_APP_GL3
#ifndef IMGUI_APP_IMPLEMENTATION
#include "imgui_impl_opengl3.h"

// GL includes
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#elif defined(IMGUI_IMPL_OPENGL_ES3)
#if (defined(__APPLE__) && (TARGET_OS_IOS || TARGET_OS_TV))
#include <OpenGLES/ES3/gl.h>    // Use GL ES 3
#else
#include <GLES3/gl3.h>          // Use GL ES 3
#endif
#else
#include "imgui_impl_opengl3_loader.h"
#endif
#endif

#endif

//-----------------------------------------------------------------------------
// [SECTION] ImGuiApp Implementation: SDL + OpenGL3
//-----------------------------------------------------------------------------

#ifdef IMGUI_APP_SDL2

// Include
#include "imgui_impl_sdl2.h"
#include <SDL.h>

#define SDL_HAS_PER_MONITOR_DPI             SDL_VERSION_ATLEAST(2,0,4)

// Data
struct ImGuiApp_ImplSdlGLX : public ImGuiApp
{
    SDL_Window*     window;
    SDL_GLContext   gl_context;
    const char*     glsl_version;
};

// Forward declarations of helper functions
static float ImGuiApp_ImplSdl_GetDPI(int display_index)
{
#if SDL_HAS_PER_MONITOR_DPI
    if (display_index >= 0)
    {
        float dpi;
        if (SDL_GetDisplayDPI(display_index, &dpi, NULL, NULL) == 0)
            return dpi / 96.0f;
    }
#endif
    return 1.0f;
}

#endif

#ifdef IMGUI_APP_SDL2_GL2

// Functions
static bool ImGuiApp_ImplSdlGL2_CreateWindow(ImGuiApp* app_opaque, const char* window_title, ImVec2 window_size)
{
    ImGuiApp_ImplSdlGLX* app = (ImGuiApp_ImplSdlGLX*)app_opaque;
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return false;
    }

    // Setup GL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    app->DpiScale = app->DpiAware ? ImGuiApp_ImplSdl_GetDPI(0) : 1.0f;    // Main display scale
    window_size.x = ImFloor(window_size.x * app->DpiScale);
    window_size.y = ImFloor(window_size.y * app->DpiScale);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    app->window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)window_size.x, (int)window_size.y, window_flags);
    app->gl_context = SDL_GL_CreateContext(app->window);
    SDL_GL_MakeCurrent(app->window, app->gl_context);

    return true;
}

static void ImGuiApp_ImplSdlGL2_InitBackends(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplSdlGLX* app = (ImGuiApp_ImplSdlGLX*)app_opaque;
    ImGui_ImplSDL2_InitForOpenGL(app->window, app->gl_context);
    ImGui_ImplOpenGL2_Init();
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiIO& io = ImGui::GetIO();
    if (app->MockViewports && (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
        ImGuiApp_InstalMockViewportsBackend(app);
#endif
}

static bool ImGuiApp_ImplSdlGL2_NewFrame(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplSdlGLX* app = (ImGuiApp_ImplSdlGLX*)app_opaque;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            return false;
        else if (event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(app->window))
        {
            if (event.window.event == SDL_WINDOWEVENT_MOVED)
                app->DpiScale = ImGuiApp_ImplSdl_GetDPI(SDL_GetWindowDisplayIndex(app->window));
            if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                return false;
        }
    }
    SDL_GL_MakeCurrent(app->window, app->gl_context);
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame(app->window);
    return true;
}

static void ImGuiApp_ImplSdlGL2_Render(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplSdlGLX* app = (ImGuiApp_ImplSdlGLX*)app_opaque;
    ImGuiIO& io = ImGui::GetIO();
#ifdef IMGUI_HAS_VIEWPORT
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
#endif
    SDL_GL_MakeCurrent(app->window, app->gl_context);
    SDL_GL_SetSwapInterval(app->Vsync ? 1 : 0);
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(app->ClearColor.x, app->ClearColor.y, app->ClearColor.z, app->ClearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(app->window);
}

static void ImGuiApp_ImplSdlGLX_ShutdownCloseWindow(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplSdlGLX* app = (ImGuiApp_ImplSdlGLX*)app_opaque;
    SDL_GL_DeleteContext(app->gl_context);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
}

static void ImGuiApp_ImplSdlGL2_ShutdownBackends(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplSdlGLX* app = (ImGuiApp_ImplSdlGLX*)app_opaque;
    IM_UNUSED(app);
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
}

static bool ImGuiApp_ImplSdlGL2_CaptureFramebuffer(ImGuiApp* app, ImGuiViewport* viewport, int x, int y, int w, int h, unsigned int* pixels, void* user_data)
{
#ifdef IMGUI_HAS_VIEWPORT
    if (ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData)
        if (vd->GLContext)
            SDL_GL_MakeCurrent(vd->Window, vd->GLContext);
#endif
    return ImGuiApp_ImplGL_CaptureFramebuffer(app, viewport, x, y, w, h, pixels, user_data);
}

ImGuiApp* ImGuiApp_ImplSdlGL2_Create()
{
    ImGuiApp_ImplSdlGLX* intf = new ImGuiApp_ImplSdlGLX();
    intf->InitCreateWindow      = ImGuiApp_ImplSdlGL2_CreateWindow;
    intf->InitBackends          = ImGuiApp_ImplSdlGL2_InitBackends;
    intf->NewFrame              = ImGuiApp_ImplSdlGL2_NewFrame;
    intf->Render                = ImGuiApp_ImplSdlGL2_Render;
    intf->ShutdownCloseWindow   = ImGuiApp_ImplSdlGLX_ShutdownCloseWindow;
    intf->ShutdownBackends      = ImGuiApp_ImplSdlGL2_ShutdownBackends;
    intf->CaptureFramebuffer    = ImGuiApp_ImplSdlGL2_CaptureFramebuffer;
    intf->Destroy               = [](ImGuiApp* app) { SDL_Quit(); delete (ImGuiApp_ImplSdlGLX*)app; };
    return intf;
}

#endif // #ifdef IMGUI_APP_SDL2_GL3

#ifdef IMGUI_APP_SDL2_GL3

// Functions
static bool ImGuiApp_ImplSdlGL3_CreateWindow(ImGuiApp* app_opaque, const char* window_title, ImVec2 window_size)
{
    ImGuiApp_ImplSdlGLX* app = (ImGuiApp_ImplSdlGLX*)app_opaque;
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return false;
    }

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 Core + GLSL 150
    app->glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    app->glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    app->DpiScale = app->DpiAware ? ImGuiApp_ImplSdl_GetDPI(0) : 1.0f;    // Main display scale
    window_size.x = ImFloor(window_size.x * app->DpiScale);
    window_size.y = ImFloor(window_size.y * app->DpiScale);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    app->window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)window_size.x, (int)window_size.y, window_flags);
    app->gl_context = SDL_GL_CreateContext(app->window);
    SDL_GL_MakeCurrent(app->window, app->gl_context);
    return true;
}

static void ImGuiApp_ImplSdlGL3_InitBackends(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplSdlGLX* app = (ImGuiApp_ImplSdlGLX*)app_opaque;
    ImGui_ImplSDL2_InitForOpenGL(app->window, app->gl_context);
    ImGui_ImplOpenGL3_Init(app->glsl_version);
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiIO& io = ImGui::GetIO();
    if (app->MockViewports && (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
        ImGuiApp_InstalMockViewportsBackend(app);
#endif
}

static bool ImGuiApp_ImplSdlGL3_NewFrame(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplSdlGLX* app = (ImGuiApp_ImplSdlGLX*)app_opaque;
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            return false;
        else if (event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(app->window))
        {
            if (event.window.event == SDL_WINDOWEVENT_MOVED)
                app->DpiScale = ImGuiApp_ImplSdl_GetDPI(SDL_GetWindowDisplayIndex(app->window));
            if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                return false;
        }
    }
    SDL_GL_MakeCurrent(app->window, app->gl_context);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    return true;
}

static void ImGuiApp_ImplSdlGL3_Render(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplSdlGLX* app = (ImGuiApp_ImplSdlGLX*)app_opaque;
    ImGuiIO& io = ImGui::GetIO();
#ifdef IMGUI_HAS_VIEWPORT
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
#endif
    SDL_GL_MakeCurrent(app->window, app->gl_context);
    SDL_GL_SetSwapInterval(app->Vsync ? 1 : 0);
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(app->ClearColor.x, app->ClearColor.y, app->ClearColor.z, app->ClearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(app->window);
}

static void ImGuiApp_ImplSdlGLX_ShutdownCloseWindow(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplSdlGLX* app = (ImGuiApp_ImplSdlGLX*)app_opaque;
    SDL_GL_DeleteContext(app->gl_context);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
}

static void ImGuiApp_ImplSdlGL3_ShutdownBackends(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplSdlGLX* app = (ImGuiApp_ImplSdlGLX*)app_opaque;
    IM_UNUSED(app);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
}

static bool ImGuiApp_ImplSdlGL3_CaptureFramebuffer(ImGuiApp* app, ImGuiViewport* viewport, int x, int y, int w, int h, unsigned int* pixels, void* user_data)
{
#ifdef IMGUI_HAS_VIEWPORT
    if (ImGui_ImplSDL2_ViewportData* vd = (ImGui_ImplSDL2_ViewportData*)viewport->PlatformUserData)
        if (vd->GLContext)
            SDL_GL_MakeCurrent(vd->Window, vd->GLContext);
#endif
    return ImGuiApp_ImplGL_CaptureFramebuffer(app, viewport, x, y, w, h, pixels, user_data);
}

ImGuiApp* ImGuiApp_ImplSdlGL3_Create()
{
    ImGuiApp_ImplSdlGLX* intf = new ImGuiApp_ImplSdlGLX();
    intf->InitCreateWindow      = ImGuiApp_ImplSdlGL3_CreateWindow;
    intf->InitBackends          = ImGuiApp_ImplSdlGL3_InitBackends;
    intf->NewFrame              = ImGuiApp_ImplSdlGL3_NewFrame;
    intf->Render                = ImGuiApp_ImplSdlGL3_Render;
    intf->ShutdownCloseWindow   = ImGuiApp_ImplSdlGLX_ShutdownCloseWindow;
    intf->ShutdownBackends      = ImGuiApp_ImplSdlGL3_ShutdownBackends;
    intf->CaptureFramebuffer    = ImGuiApp_ImplSdlGL3_CaptureFramebuffer;
    intf->Destroy               = [](ImGuiApp* app) { SDL_Quit(); delete (ImGuiApp_ImplSdlGLX*)app; };
    return intf;
}

#endif // #ifdef IMGUI_APP_SDL2_GL3


//-----------------------------------------------------------------------------
// [SECTION] ImGuiApp Implementation: GLFW + OpenGL3
//-----------------------------------------------------------------------------

#ifdef IMGUI_APP_GLFW_GL3

// Include
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Data
struct ImGuiApp_ImplGlfwGL3 : public ImGuiApp
{
    GLFWwindow* window;
    // Glfw_GLContext gl_context;
    const char* glsl_version;
};

// Helper functions
static float ImGuiApp_ImplGlfw_GetDPI(GLFWmonitor* monitor)
{
#if GLFW_HAS_PER_MONITOR_DPI
    float x_scale, y_scale;
    glfwGetMonitorContentScale(monitor, &x_scale, &y_scale);
    return x_scale;
#else
    IM_UNUSED(monitor);
    return 1.0f;
#endif
}

static float ImGuiApp_ImplGlfw_GetDPI(GLFWwindow* window)
{
#if GLFW_HAS_MONITOR_WORK_AREA
    int window_x, window_y, window_width, window_height;
    glfwGetWindowPos(window, &window_x, &window_y);
    glfwGetWindowSize(window, &window_width, &window_height);
    // Find window center
    window_x += (int)((float)window_width * 0.5f);
    window_y += (int)((float)window_height * 0.5f);
    int num_monitors = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&num_monitors);
    while (num_monitors-- > 0)
    {
        GLFWmonitor* monitor = *monitors;
        int monitor_x, monitor_y, monitor_width, monitor_height;
        glfwGetMonitorWorkarea(monitor, &monitor_x, &monitor_y, &monitor_width, &monitor_height);
        if (monitor_x <= window_x && window_x < (monitor_x + monitor_width) && monitor_y <= window_y && window_y < (monitor_y + monitor_height))
            return ImGuiApp_ImplGlfw_GetDPI(monitor);
        monitors++;
    }
#else
    IM_UNUSED(window);
#endif
    return 1.0f;
}

static void ImGuiApp_ImplGlfw_ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static bool ImGuiApp_ImplGlfw_CreateWindow(ImGuiApp* app_opaque, const char* window_title, ImVec2 window_size)
{
    ImGuiApp_ImplGlfwGL3* app = (ImGuiApp_ImplGlfwGL3*)app_opaque;

    // Setup window
    glfwSetErrorCallback(ImGuiApp_ImplGlfw_ErrorCallback);
    if (!glfwInit())
        return false;

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    app->glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    app->glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif
    // Create window with graphics context
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    float dpi_scale = app->DpiAware ? ImGuiApp_ImplGlfw_GetDPI(primaryMonitor) : 1.0f;
    window_size.x = ImFloor(window_size.x * app->DpiScale);
    window_size.y = ImFloor(window_size.y * app->DpiScale);
    IM_UNUSED(dpi_scale); // FIXME 2020/04/12: is the code above correct?
    app->window = glfwCreateWindow((int)window_size.x, (int)window_size.y, window_title, NULL, NULL);
    if (app->window == NULL)
        return false;
    glfwMakeContextCurrent(app->window);
    return true;
}

static void ImGuiApp_ImplGlfw_InitBackends(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplGlfwGL3* app = (ImGuiApp_ImplGlfwGL3*)app_opaque;
    ImGui_ImplGlfw_InitForOpenGL(app->window, true);
    ImGui_ImplOpenGL3_Init(app->glsl_version);
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiIO& io = ImGui::GetIO();
    if (app->MockViewports && (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
        ImGuiApp_InstalMockViewportsBackend(app);
#endif
}

static bool ImGuiApp_ImplGlfwGL3_NewFrame(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplGlfwGL3* app = (ImGuiApp_ImplGlfwGL3*)app_opaque;
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwMakeContextCurrent(app->window);
    glfwPollEvents();
    if (glfwWindowShouldClose(app->window))
        return false;
    app->DpiScale = ImGuiApp_ImplGlfw_GetDPI(app->window);
#if !defined(IMGUI_IMPL_OPENGL_ES3) && !defined(IMGUI_IMPL_OPENGL_ES2)
    if (app->SrgbFramebuffer)
        glEnable(GL_FRAMEBUFFER_SRGB);
    else
        glDisable(GL_FRAMEBUFFER_SRGB);
#endif
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    return true;
}

static void ImGuiApp_ImplGlfwGL3_Render(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplGlfwGL3* app = (ImGuiApp_ImplGlfwGL3*)app_opaque;
    ImGuiIO& io = ImGui::GetIO();
    glfwMakeContextCurrent(app->window);
    glfwSwapInterval(app->Vsync ? 1 : 0);
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(app->ClearColor.x, app->ClearColor.y, app->ClearColor.z, app->ClearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#ifdef IMGUI_HAS_VIEWPORT
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
#endif
    glfwSwapBuffers(app->window);
}

static void ImGuiApp_ImplGlfwGL3_ShutdownCloseWindow(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplGlfwGL3* app = (ImGuiApp_ImplGlfwGL3*)app_opaque;
    glfwDestroyWindow(app->window);
    glfwTerminate();
}

static void ImGuiApp_ImplGlfwGL3_ShutdownBackends(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplGlfwGL3* app = (ImGuiApp_ImplGlfwGL3*)app_opaque;
    IM_UNUSED(app);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}

static bool ImGuiApp_ImplGlfwGL3_CaptureFramebuffer(ImGuiApp* app, ImGuiViewport* viewport, int x, int y, int w, int h, unsigned int* pixels, void* user_data)
{
#ifdef IMGUI_HAS_VIEWPORT
    if (ImGui_ImplGlfw_ViewportData* vd = (ImGui_ImplGlfw_ViewportData*)viewport->PlatformUserData)
        if (vd->Window)
            glfwMakeContextCurrent(vd->Window);
#endif
    return ImGuiApp_ImplGL_CaptureFramebuffer(app, viewport, x, y, w, h, pixels, user_data);
}

ImGuiApp* ImGuiApp_ImplGlfwGL3_Create()
{
    ImGuiApp_ImplGlfwGL3* intf = new ImGuiApp_ImplGlfwGL3();
    intf->InitCreateWindow      = ImGuiApp_ImplGlfw_CreateWindow;
    intf->InitBackends          = ImGuiApp_ImplGlfw_InitBackends;
    intf->NewFrame              = ImGuiApp_ImplGlfwGL3_NewFrame;
    intf->Render                = ImGuiApp_ImplGlfwGL3_Render;
    intf->ShutdownCloseWindow   = ImGuiApp_ImplGlfwGL3_ShutdownCloseWindow;
    intf->ShutdownBackends      = ImGuiApp_ImplGlfwGL3_ShutdownBackends;
    intf->CaptureFramebuffer    = ImGuiApp_ImplGlfwGL3_CaptureFramebuffer;
    intf->Destroy               = [](ImGuiApp* app) { delete (ImGuiApp_ImplGlfwGL3*)app; };
    return intf;
}

#endif // #ifdef IMGUI_APP_GLFW_GL3

//-----------------------------------------------------------------------------
// [SECTION] ImGuiApp Implementation: OpenGL (Shared)
//-----------------------------------------------------------------------------

#if defined(IMGUI_APP_GL2) || defined(IMGUI_APP_GL3)
static bool ImGuiApp_ImplGL_CaptureFramebuffer(ImGuiApp* app, ImGuiViewport* viewport, int x, int y, int w, int h, unsigned int* pixels, void* user_data)
{
    IM_UNUSED(app);
    IM_UNUSED(user_data);
    IM_UNUSED(viewport); // Expecting calling code to have set the right GL context

#ifdef __linux__
    // FIXME: Odd timing issue is observed on linux (Plasma/X11 specifically), which causes outdated frames to be captured, unless we give compositor some time to update screen.
    // glFlush() didn't seem enough. Will probably need to revisit that.
    usleep(1000);   // 1ms
#endif

    int y2 = (int)ImGui::GetIO().DisplaySize.y - (y + h);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y2, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Flip vertically
    size_t comp = 4;
    size_t stride = (size_t)w * comp;
    unsigned char* line_tmp = new unsigned char[stride];
    unsigned char* line_a = (unsigned char*)pixels;
    unsigned char* line_b = (unsigned char*)pixels + (stride * ((size_t)h - 1));
    while (line_a < line_b)
    {
        memcpy(line_tmp, line_a, stride);
        memcpy(line_a, line_b, stride);
        memcpy(line_b, line_tmp, stride);
        line_a += stride;
        line_b -= stride;
    }
    delete[] line_tmp;
    return true;
}
#endif

//-----------------------------------------------------------------------------
// [SECTION] Mock backend for multi-viewports (Shared)
// The viewports are not visible
//-----------------------------------------------------------------------------

#ifdef IMGUI_HAS_VIEWPORT


// A virtual window
struct ImGui_ImplMockViewport_ViewportData
{
    ImGuiID     ID = 0;
    ImVec2      Pos;
    ImVec2      Size;
    char        Title[256] = ""; // There's no getter so we don't mind if the title is truncated
};

struct ImGui_ImplMockViewport_Data
{
    ImVector<ImGui_ImplMockViewport_ViewportData>   MockViewports;  // We can't store in viewport->PlatformUserData to allow combining with real backends.
    ImGuiID                                         FocusedViewportId = 0;
    ImGuiPlatformIO                                 OriginalPlatformIO;
};

static ImGui_ImplMockViewport_Data g_NullViewportBackendData;

static ImGui_ImplMockViewport_Data* ImGui_ImplNullViewport_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplMockViewport_Data*)&g_NullViewportBackendData : NULL;
}

static ImGui_ImplMockViewport_ViewportData* ImGui_ImplNullViewport_FindViewportData(ImGui_ImplMockViewport_Data* bd, ImGuiViewport* viewport)
{
    for (ImGui_ImplMockViewport_ViewportData& vd : bd->MockViewports)
        if (vd.ID == viewport->ID)
            return &vd;
    return NULL;
}

// Mock backend (viewports not visible)
static void ImGuiApp_InstalMockViewportsBackend(ImGuiApp*)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    ImGui_ImplMockViewport_Data* bd = ImGui_ImplNullViewport_GetBackendData();
    bd->OriginalPlatformIO = platform_io;
    memset((void*)&platform_io, 0, offsetof(ImGuiPlatformIO, Monitors)); // FIXME: Clear all handlers by default

    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports | ImGuiBackendFlags_RendererHasViewports;

    // This data may be filled in already, in case we use a mock viewports with a regular backend like GLFW/SDL.
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    if (main_viewport->PlatformUserData == NULL && main_viewport->PlatformHandle == NULL)
    {
        ImGuiPlatformMonitor monitor;
        monitor.DpiScale = 1.0f;
        monitor.MainPos = ImVec2(0, 0);
        monitor.MainSize = ImVec2(1920, 1080);
        monitor.WorkPos = ImVec2(0, 0);
        monitor.WorkSize = ImVec2(1920, 1000);
        platform_io.Monitors.resize(1);
        platform_io.Monitors[0] = monitor;
        main_viewport->PlatformUserData = (void*)-1; // This is merely to satisfy ErrorCheckNewFrameSanityChecks()
    }
    {
        ImGui_ImplMockViewport_ViewportData vd;
        vd.ID = main_viewport->ID;
        vd.Pos = ImVec2(100.0f, 100.0f);
        vd.Size = ImVec2(1600.0f, 1200.0f);
        bd->MockViewports.push_back(vd);
    }

    platform_io.Platform_CreateWindow = [](ImGuiViewport* viewport)
    {
        ImGui_ImplMockViewport_Data* bd = ImGui_ImplNullViewport_GetBackendData();
        ImGui_ImplMockViewport_ViewportData vd;
        vd.ID = viewport->ID;
        vd.Pos = viewport->Pos;
        vd.Size = viewport->Size;
        bd->MockViewports.push_back(vd);
    };
    platform_io.Platform_DestroyWindow = [](ImGuiViewport* viewport)
    {
        ImGui_ImplMockViewport_Data* bd = ImGui_ImplNullViewport_GetBackendData();
        if (ImGui_ImplMockViewport_ViewportData* vd = ImGui_ImplNullViewport_FindViewportData(bd, viewport))
            bd->MockViewports.erase(vd);

        // Transfer focus to previous viewport
        if (bd->FocusedViewportId == viewport->ID)
        {
            ImGuiViewportP* top_most_viewport_under_this_one = NULL;
            for (ImGuiViewportP* other_viewport : ImGui::GetCurrentContext()->Viewports)
                if (other_viewport != viewport && (top_most_viewport_under_this_one == NULL || top_most_viewport_under_this_one->LastFocusedStampCount < other_viewport->LastFocusedStampCount))
                    top_most_viewport_under_this_one = other_viewport;
            bd->FocusedViewportId = top_most_viewport_under_this_one ? top_most_viewport_under_this_one->ID : 0;
        }

        // Because default backends tend to create data for main viewport during init we give them a change of destroying it.
        // (otherwise we could null the PlatformUserData/RendererUserData fields would it would lead to a leak when using that strange mock-viewport + real backend debug feature)
        if (bd->OriginalPlatformIO.Platform_DestroyWindow != NULL)
            bd->OriginalPlatformIO.Platform_DestroyWindow(viewport);
        else if (viewport->PlatformUserData == (void*)-1)
            viewport->PlatformUserData = NULL;
    };
    platform_io.Platform_ShowWindow = [](ImGuiViewport*) {};
    platform_io.Platform_SetWindowPos = [](ImGuiViewport* viewport, ImVec2 pos)
    {
        ImGui_ImplMockViewport_Data* bd = ImGui_ImplNullViewport_GetBackendData();
        if (ImGui_ImplMockViewport_ViewportData* vd = ImGui_ImplNullViewport_FindViewportData(bd, viewport))
            vd->Pos = pos;
    };
    platform_io.Platform_GetWindowPos = [](ImGuiViewport* viewport)
    {
        ImGui_ImplMockViewport_Data* bd = ImGui_ImplNullViewport_GetBackendData();
        ImGui_ImplMockViewport_ViewportData* vd = ImGui_ImplNullViewport_FindViewportData(bd, viewport);

        // When combining mock viewport with a real backend we need to ensure they can access window pos/size through this call otherwise mouse position will be off.
        if (viewport == ImGui::GetMainViewport() && bd->OriginalPlatformIO.Platform_GetWindowPos != NULL)
            vd->Pos = bd->OriginalPlatformIO.Platform_GetWindowPos(viewport);

        return vd->Pos;
    };
    platform_io.Platform_SetWindowSize = [](ImGuiViewport* viewport, ImVec2 size)
    {
        ImGui_ImplMockViewport_Data* bd = ImGui_ImplNullViewport_GetBackendData();
        if (ImGui_ImplMockViewport_ViewportData* vd = ImGui_ImplNullViewport_FindViewportData(bd, viewport))
            vd->Size = size;
    };
    platform_io.Platform_GetWindowSize = [](ImGuiViewport* viewport)
    {
        ImGui_ImplMockViewport_Data* bd = ImGui_ImplNullViewport_GetBackendData();
        ImGui_ImplMockViewport_ViewportData* vd = ImGui_ImplNullViewport_FindViewportData(bd, viewport);

        // When combining mock viewport with a real backend we need to ensure they can access window pos/size through this call otherwise mouse position will be off.
        if (viewport == ImGui::GetMainViewport() && bd->OriginalPlatformIO.Platform_GetWindowSize != NULL)
            vd->Size = bd->OriginalPlatformIO.Platform_GetWindowSize(viewport);

        return vd->Size;
    };
    platform_io.Platform_GetWindowDpiScale = [](ImGuiViewport* viewport)
    {
        IM_UNUSED(viewport);
        return 1.0f;
    };
    platform_io.Platform_SetWindowFocus = [](ImGuiViewport* viewport)
    {
        ImGui_ImplMockViewport_Data* bd = ImGui_ImplNullViewport_GetBackendData();
        bd->FocusedViewportId = viewport->ID;
    };
    platform_io.Platform_GetWindowFocus = [](ImGuiViewport* viewport)
    {
        ImGui_ImplMockViewport_Data* bd = ImGui_ImplNullViewport_GetBackendData();
        return viewport->ID == bd->FocusedViewportId;
    };
    platform_io.Platform_GetWindowMinimized = [](ImGuiViewport*) { return false; };
    platform_io.Platform_SetWindowTitle = [](ImGuiViewport* viewport, const char* title)
    {
        ImGui_ImplMockViewport_Data* bd = ImGui_ImplNullViewport_GetBackendData();
        ImGui_ImplMockViewport_ViewportData* vd = ImGui_ImplNullViewport_FindViewportData(bd, viewport);
        if (vd != NULL)
            ImFormatString(vd->Title, IM_ARRAYSIZE(vd->Title), "%s", title);
    };
    platform_io.Platform_OnChangedViewport = [](ImGuiViewport* viewport)
    {
        // [DEBUG] Display a rectangle in main viewport corresponding to our virtual viewports (only meaningful when they overlap of course)
        // Ideally we should do that before the main viewport render, but let's hook wherever we can...
        ImGuiViewportP* main_viewport = (ImGuiViewportP*)ImGui::GetMainViewport();
        if (viewport != main_viewport)
        {
            ImRect r = ((ImGuiViewportP*)viewport)->GetMainRect();
            ImGui::GetForegroundDrawList(main_viewport)->AddRect(r.Min, r.Max, IM_COL32(255, 0, 0, 255), 0.0f, ImDrawFlags_None, 3.0f);
        }
    };
    platform_io.Platform_RenderWindow = NULL;
    platform_io.Platform_SwapBuffers = [](ImGuiViewport*, void*) {};
    platform_io.Platform_SetWindowAlpha = [](ImGuiViewport*, float) {};

    // Disable existing rendering backend as well, if any
    platform_io.Renderer_CreateWindow = NULL;
    platform_io.Renderer_DestroyWindow = NULL;
    platform_io.Renderer_SetWindowSize = NULL;
    platform_io.Renderer_RenderWindow = [](ImGuiViewport* viewport, void*) { ImGuiApp_ImplNull_RenderDrawData(viewport->DrawData); };
    platform_io.Renderer_SwapBuffers = NULL;
}

#endif // #ifdef IMGUI_HAS_VIEWPORT
