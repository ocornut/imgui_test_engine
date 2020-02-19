//-----------------------------------------------------------------------------
// Simple Dear ImGui App Framework (using standard bindings)
//-----------------------------------------------------------------------------

#include "imgui_app.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <chrono>

/*

Index of this file:

// [SECTION] ImGuiApp Implementation: Null
// [SECTION] ImGuiApp Implementation: Win32 + DX11

*/

//-----------------------------------------------------------------------------
// [SECTION] ImGuiApp Implementation: Null
//-----------------------------------------------------------------------------

// Data
struct ImGuiApp_ImplNull : public ImGuiApp
{
    ImU64 LastTime = 0;
};

// Functions
static bool ImGuiApp_ImplNull_CreateWindow(ImGuiApp* app, const char*, ImVec2 size)
{
    IM_UNUSED(app);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = size;
    io.Fonts->Build();
    for (int n = 0; n < ImGuiKey_COUNT; n++)
        io.KeyMap[n] = n;

    return true;
}

static ImU64 ImGetTimeInMicroseconds()
{
    using namespace std;
    chrono::microseconds ms = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch());
    return ms.count();
}

static bool ImGuiApp_ImplNull_NewFrame(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplNull* app = (ImGuiApp_ImplNull*)app_opaque;
    ImGuiIO& io = ImGui::GetIO();

    ImU64 time = ImGetTimeInMicroseconds();
    if (app->LastTime == 0)
        app->LastTime = time;
    io.DeltaTime = (float)((double)(time - app->LastTime) / 1000000.0);
    if (io.DeltaTime <= 0.0f)
        io.DeltaTime = 0.000001f;
    app->LastTime = time;

    return true;
}

static bool ImGuiApp_ImplNull_CaptureFramebuffer(ImGuiApp* app, int x, int y, int w, int h, unsigned int* pixels, void* user_data)
{
    IM_UNUSED(app);
    IM_UNUSED(user_data);
    IM_UNUSED(x);
    IM_UNUSED(y);
    memset(pixels, 0, w * h * sizeof(unsigned int));
    return true;
}

ImGuiApp* ImGuiApp_ImplNull_Create()
{
    ImGuiApp_ImplNull* intf = new ImGuiApp_ImplNull();
    intf->InitCreateWindow      = ImGuiApp_ImplNull_CreateWindow;
    intf->InitBackends          = [](ImGuiApp* app) { IM_UNUSED(app); };
    intf->NewFrame              = ImGuiApp_ImplNull_NewFrame;
    intf->Render                = [](ImGuiApp* app) { IM_UNUSED(app); };
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
    WNDCLASSEX              WC;
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
static ImGuiApp_ImplWin32DX11* g_AppForWndProc = NULL;
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool ImGuiApp_ImplWin32DX11_InitCreateWindow(ImGuiApp* app_opaque, const char* window_title_a, ImVec2 window_size)
{
    ImGuiApp_ImplWin32DX11* app = (ImGuiApp_ImplWin32DX11*)app_opaque;
    if (app->DpiAware)
        ImGui_ImplWin32_EnableDpiAwareness();

    // Create application window
    app->WC = { sizeof(WNDCLASSEXA), CS_CLASSDC, WndProc, 0L, 0L, ::GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGuiApp", NULL };
    ::RegisterClassEx(&app->WC);

    POINT pos = { 100, 100 };
    HMONITOR monitor = ::MonitorFromPoint(pos, 0);
    float dpi_scale = app->DpiAware ? ImGui_ImplWin32_GetDpiScaleForMonitor(monitor) : 1.0f;
    window_size.x = ImFloor(window_size.x * dpi_scale);
    window_size.y = ImFloor(window_size.y * dpi_scale);

#ifdef UNICODE
    const int count = ::MultiByteToWideChar(CP_UTF8, 0, window_title_a, -1, NULL, 0);
    WCHAR* window_title_t = (WCHAR*)calloc(count, sizeof(WCHAR));
    if (!::MultiByteToWideChar(CP_UTF8, 0, window_title_a, -1, window_title_t, count))
    {
        free(window_title_t);
        return false;
    }
    app->Hwnd = ::CreateWindowEx(0L, app->WC.lpszClassName, window_title_t, WS_OVERLAPPEDWINDOW, pos.x, pos.y, (int)window_size.x, (int)window_size.y, NULL, NULL, app->WC.hInstance, NULL);
    free(window_title_t);
#else
    const char* window_title_t = window_titLe_a;
    app->Hwnd = ::CreateWindowEx(0L, app->WC.lpszClassName, window_titLe_t, WS_OVERLAPPEDWINDOW, pos.x, pos.y, (int)window_size.x, (int)window_size.y, NULL, NULL, app->WC.hInstance, NULL);
#endif

    // Initialize Direct3D
    if (!CreateDeviceD3D(app))
    {
        CleanupDeviceD3D(app);
        ::UnregisterClass(app->WC.lpszClassName, app->WC.hInstance);
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
    ::UnregisterClass(app->WC.lpszClassName, app->WC.hInstance);
}

static void ImGuiApp_ImplWin32DX11_InitBackends(ImGuiApp* app_opaque)
{
    ImGuiApp_ImplWin32DX11* app = (ImGuiApp_ImplWin32DX11*)app_opaque;
    ImGui_ImplWin32_Init(app->Hwnd);
    ImGui_ImplDX11_Init(app->pd3dDevice, app->pd3dDeviceContext);
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

static bool ImGuiApp_ImplWin32DX11_CaptureFramebuffer(ImGuiApp* app_opaque, int x, int y, int w, int h, unsigned int* pixels_rgba, void* user_data)
{
    IM_UNUSED(user_data);
    ImGuiApp_ImplWin32DX11* app = (ImGuiApp_ImplWin32DX11*)app_opaque;
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    D3D11_TEXTURE2D_DESC texture_desc;
    memset(&texture_desc, 0, sizeof(texture_desc));
    texture_desc.Width = (UINT)io.DisplaySize.x;
    texture_desc.Height = (UINT)io.DisplaySize.y;
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
    app->mainRenderTargetView->GetResource(&source);
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
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    ImGuiApp_ImplWin32DX11* app = g_AppForWndProc;
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
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

#endif // #ifdef IMGUI_APP_WIN32_DX11
