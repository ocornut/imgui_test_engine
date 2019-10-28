#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_te_app.h"

bool MainLoopEndFrame();

//-------------------------------------------------------------------------
// Backend: Null
//-------------------------------------------------------------------------

bool MainLoopNewFrameNull()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);

    ImU64 time = ImGetTimeInMicroseconds();
    if (g_App.LastTime == 0)
        g_App.LastTime = time;
    io.DeltaTime = (float)((double)(time - g_App.LastTime) / 1000000.0);
    if (io.DeltaTime <= 0.0f)
        io.DeltaTime = 0.000001f;
    g_App.LastTime = time;

    ImGui::NewFrame();

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
    if (!test_io.RunningTests)
        return false;

    return true;
}

void MainLoopNull()
{
    // Init
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Build();
    for (int n = 0; n < ImGuiKey_COUNT; n++)
        io.KeyMap[n] = n;

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
    test_io.ConfigLogToTTY = true;
    test_io.NewFrameFunc = [](ImGuiTestEngine*, void*) { return MainLoopNewFrameNull(); };
    test_io.EndFrameFunc = [](ImGuiTestEngine*, void*) { return MainLoopEndFrame(); };

    while (1)
    {
        if (!MainLoopNewFrameNull())
            break;
        if (!MainLoopEndFrame())
            break;
    }
}

#if !defined(IMGUI_TESTS_BACKEND_WIN32_DX11) && !defined(IMGUI_TESTS_BACKEND_SDL_GL3)
void MainLoop()
{
    // No graphics backend is used. Do same thing no matter if g_App.OptGUI is enabled or disabled.
    MainLoopNull();
}
#endif

//-------------------------------------------------------------------------
// Backend: Win32 + DX11
//-------------------------------------------------------------------------

#ifdef IMGUI_TESTS_BACKEND_WIN32_DX11
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

HRESULT CreateDeviceD3D(HWND hWnd)
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
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return E_FAIL;

    CreateRenderTarget();

    return S_OK;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool MainLoopNewFrameDX11()
{
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    // Poll and handle messages (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    while (true)
    {
        if (!PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
            break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
        {
            g_App.Quit = true;
            ImGuiTestEngine_Abort(g_App.TestEngine);
            break;
        }
        continue;
    }

    // Start the Dear ImGui frame
    if (!g_App.Quit)
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }
    else
    {
        // It would be tempting to call NewFrame() even if g_App.Quit==true because it makes winding down the Yield stack easier,
        // however, NewFrame() calls Platform_GetWindowPos() which will return (0,0) after the WM_QUIT message and damage our .ini data.
        //ImGui::NewFrame();
    }

    return !g_App.Quit;
}

static void MainLoopEndFrameRenderDX11()
{
    // Rendering
    ImGui::Render();

    // Update and Render additional Platform Windows
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
#endif
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&g_App.ClearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
    if ((test_io.RunningTests && test_io.ConfigRunFast) || test_io.ConfigNoThrottle)
        g_pSwapChain->Present(0, 0); // Present without vsync
    else
        g_pSwapChain->Present(1, 0); // Present with vsync
}

void MainLoop()
{
    // Initialize back-end
    HWND hwnd = 0;
    WNDCLASSEX wc = { 0 };

    // Create application window
    wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    RegisterClassEx(&wc);
    hwnd = CreateWindow(_T("ImGui Example"), _T("Dear ImGui - Tests"), WS_OVERLAPPEDWINDOW, 60, 60, 1280 + 160, 800 + 100, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (CreateDeviceD3D(hwnd) < 0)
    {
        CleanupDeviceD3D();
        UnregisterClass(_T("ImGui Example"), wc.hInstance);
        return;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
    //test_io.ConfigLogToTTY = true;
    //test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Max;

    auto end_frame_func =
        [](ImGuiTestEngine*, void*)
        {
            if (!MainLoopEndFrame())
                return false;
#if 0
            // Super fast mode doesn't render/present
            if (test_io.RunningTests && test_io.ConfigRunFast)
                return true;
#endif
            MainLoopEndFrameRenderDX11();
            return true;
        };

    test_io.NewFrameFunc = [](ImGuiTestEngine*, void*) { return MainLoopNewFrameDX11(); };
    test_io.EndFrameFunc = end_frame_func;

    while (1)
    {
        if (!MainLoopNewFrameDX11())
            break;

        if (!end_frame_func(NULL, NULL))
            break;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClass(_T("ImGui Example"), wc.hInstance);
}
#endif

//-------------------------------------------------------------------------
// Backend: SDL + OpenGL3
//------------------------------------------------------------------------

#ifdef IMGUI_TESTS_BACKEND_SDL_GL3
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <GL/gl3w.h>
#include "imgui_te_core.h"

static bool MainLoopNewFrameSDLGL3(SDL_Window* window)
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)))
        {
            g_App.Quit = true;
            ImGuiTestEngine_Abort(g_App.TestEngine);
            break;
        }
    }

    // Start the Dear ImGui frame
    if (!g_App.Quit)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();
    }
    return !g_App.Quit;
}

void MainLoop()
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return;
    }

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);

    bool err = gl3wInit() != 0;
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return;
    }

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Init
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Build();

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
    test_io.UserData = window;
    test_io.ConfigLogToTTY = true;
    test_io.NewFrameFunc = [](ImGuiTestEngine*, void* window) { return MainLoopNewFrameSDLGL3((SDL_Window*)window); };
    test_io.EndFrameFunc = [](ImGuiTestEngine*, void* window)
    {
        if (!MainLoopEndFrame())
            return false;
        // Super fast mode doesn't render/present
        ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
        if (test_io.RunningTests && test_io.ConfigRunFast)
            return true;
        ImGui::Render();
        ImGuiIO& io = ImGui::GetIO();
        SDL_GL_SetSwapInterval(test_io.ConfigNoThrottle ? 1 : 0); // Enable vsync
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(g_App.ClearColor.x, g_App.ClearColor.y, g_App.ClearColor.z, g_App.ClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow((SDL_Window*)window);
        return true;
    };

    while (1)
    {
        if (!MainLoopNewFrameSDLGL3(window))
            break;
        if (!test_io.EndFrameFunc(nullptr, window))
            break;
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
#endif // IMGUI_TESTS_BACKEND_SDL_GL3
