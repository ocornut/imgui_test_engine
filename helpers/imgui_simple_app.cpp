
#include "imgui_simple_app.h"
#include "imgui.h"
#include "imgui_internal.h"

#ifdef IMGUI_SIMPLE_APP_WIN32_DX11

#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <stdlib.h>

struct ImGuiSimpleApp_ImplWin32DX11 : public ImGuiSimpleApp
{
    HWND                    Hwnd = NULL;
    WNDCLASSEX              WC;
    ID3D11Device*           pd3dDevice = NULL;
    ID3D11DeviceContext*    pd3dDeviceContext = NULL;
    IDXGISwapChain*         pSwapChain = NULL;
    ID3D11RenderTargetView* mainRenderTargetView = NULL;
};

// Forward declarations of helper functions
static bool CreateDeviceD3D(ImGuiSimpleApp_ImplWin32DX11* app);
static void CleanupDeviceD3D(ImGuiSimpleApp_ImplWin32DX11* app);
static void CreateRenderTarget(ImGuiSimpleApp_ImplWin32DX11* app);
static void CleanupRenderTarget(ImGuiSimpleApp_ImplWin32DX11* app);
static ImGuiSimpleApp_ImplWin32DX11* g_AppForWndProc = NULL;
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool ImGuiSimpleApp_ImplWin32DX11_InitCreateWindow(ImGuiSimpleApp* app_opaque, const char* window_title_a, ImVec2 window_size)
{
    ImGuiSimpleApp_ImplWin32DX11* app = (ImGuiSimpleApp_ImplWin32DX11*)app_opaque;
    ImGui_ImplWin32_EnableDpiAwareness();

    // Create application window
    app->WC = { sizeof(WNDCLASSEXA), CS_CLASSDC, WndProc, 0L, 0L, ::GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGuiSimpleApp", NULL };
    ::RegisterClassEx(&app->WC);

    POINT pos = { 100, 100 };
    HMONITOR monitor = ::MonitorFromPoint(pos, 0);
    float dpi_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(monitor);
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

    app->DpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(app->Hwnd);

    return true;
}

static void ImGuiSimpleApp_ImplWin32DX11_ShutdownCloseWindow(ImGuiSimpleApp* app_opaque)
{
    ImGuiSimpleApp_ImplWin32DX11* app = (ImGuiSimpleApp_ImplWin32DX11*)app_opaque;
    CleanupDeviceD3D(app);
    ::DestroyWindow(app->Hwnd);
    ::UnregisterClass(app->WC.lpszClassName, app->WC.hInstance);
}

static void ImGuiSimpleApp_ImplWin32DX11_InitBackends(ImGuiSimpleApp* app_opaque)
{
    ImGuiSimpleApp_ImplWin32DX11* app = (ImGuiSimpleApp_ImplWin32DX11*)app_opaque;
    ImGui_ImplWin32_Init(app->Hwnd);
    ImGui_ImplDX11_Init(app->pd3dDevice, app->pd3dDeviceContext);
}

static void ImGuiSimpleApp_ImplWin32DX11_ShutdownBackends(ImGuiSimpleApp* app_opaque)
{
    ImGuiSimpleApp_ImplWin32DX11* app = (ImGuiSimpleApp_ImplWin32DX11*)app_opaque;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

static bool ImGuiSimpleApp_ImplWin32DX11_NewFrame(ImGuiSimpleApp* app_opaque)
{
    ImGuiSimpleApp_ImplWin32DX11* app = (ImGuiSimpleApp_ImplWin32DX11*)app_opaque;
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

static void ImGuiSimpleApp_ImplWin32DX11_Render(ImGuiSimpleApp* app_opaque)
{
    ImGuiSimpleApp_ImplWin32DX11* app = (ImGuiSimpleApp_ImplWin32DX11*)app_opaque;

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
    app->pSwapChain->Present(1, 0); // Present with vsync
}

ImGuiSimpleApp* ImGuiSimpleApp_ImplWin32DX11_Create()
{
    auto intf = new ImGuiSimpleApp_ImplWin32DX11();
    intf->InitCreateWindow = ImGuiSimpleApp_ImplWin32DX11_InitCreateWindow;
    intf->InitBackends = ImGuiSimpleApp_ImplWin32DX11_InitBackends;
    intf->NewFrame = ImGuiSimpleApp_ImplWin32DX11_NewFrame;
    intf->Render = ImGuiSimpleApp_ImplWin32DX11_Render;
    intf->ShutdownCloseWindow = ImGuiSimpleApp_ImplWin32DX11_ShutdownCloseWindow;
    intf->ShutdownBackends = ImGuiSimpleApp_ImplWin32DX11_ShutdownBackends;
    return intf;
}

void ImGuiSimpleApp_ImplWin32DX11_Destroy(ImGuiSimpleApp* app_opaque)
{
    ImGuiSimpleApp_ImplWin32DX11* app = (ImGuiSimpleApp_ImplWin32DX11*)app_opaque;
    delete app;
}

// Helper functions
static bool CreateDeviceD3D(ImGuiSimpleApp_ImplWin32DX11* app)
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

static void CleanupDeviceD3D(ImGuiSimpleApp_ImplWin32DX11* app)
{
    CleanupRenderTarget(app);
    if (app->pSwapChain)        { app->pSwapChain->Release(); app->pSwapChain = NULL; }
    if (app->pd3dDeviceContext) { app->pd3dDeviceContext->Release(); app->pd3dDeviceContext = NULL; }
    if (app->pd3dDevice)        { app->pd3dDevice->Release(); app->pd3dDevice = NULL; }
}

static void CreateRenderTarget(ImGuiSimpleApp_ImplWin32DX11* app)
{
    ID3D11Texture2D* pBackBuffer = NULL;
    app->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    app->pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &app->mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget(ImGuiSimpleApp_ImplWin32DX11* app)
{
    if (app->mainRenderTargetView) { app->mainRenderTargetView->Release(); app->mainRenderTargetView = NULL; }
}

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    ImGuiSimpleApp_ImplWin32DX11* app = g_AppForWndProc;
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

#endif // #ifdef IMGUI_SIMPLE_APP_WIN32_DX11
