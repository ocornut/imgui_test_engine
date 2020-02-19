#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "test_app.h"

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

extern bool MainLoopEndFrame(); // FIXME

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

bool CaptureScreenshotNull(int x, int y, int w, int h, unsigned int* pixels, void* user_data)
{
    IM_UNUSED(x);
    IM_UNUSED(y);
    IM_UNUSED(user_data);
    memset(pixels, 0, w * h * sizeof(unsigned int));
    return true;
}

#if !defined(IMGUI_TESTS_BACKEND_WIN32_DX11) && !defined(IMGUI_TESTS_BACKEND_SDL_GL3) && !defined(IMGUI_TESTS_BACKEND_GLFW_GL3)
void MainLoop()
{
    // No graphics backend is used. Do same thing no matter if g_App.OptGUI is enabled or disabled.
    MainLoopNull();
}

bool CaptureFramebufferScreenshot(int x, int y, int w, int h, unsigned* pixels, void* user)
{
    // Could use software renderer for off-screen captures.
    return false;
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

bool CaptureFramebufferScreenshot(int x, int y, int w, int h, unsigned int* pixels, void* user_data)
{
    IM_UNUSED(user_data);
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

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&texture_desc, nullptr, &texture);
    if (FAILED(hr))
    {
        if (texture)
        {
            texture->Release();
            texture = nullptr;
        }
        return false;
    }

    ID3D11Resource* source = nullptr;
    g_mainRenderTargetView->GetResource(&source);
    g_pd3dDeviceContext->CopyResource(texture, source);
    source->Release();

    D3D11_MAPPED_SUBRESOURCE mapped;
    mapped.pData = nullptr;
    hr = g_pd3dDeviceContext->Map(texture, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr) || !mapped.pData)
    {
        texture->Release();
        return false;
    }

    // D3D11 does not provide means to capture a partial screenshot. We copy rect x,y,w,h on CPU side.
    for (int index_y = y; index_y < y + h; ++index_y)
    {
        unsigned int* src = (unsigned int*)((unsigned char*)mapped.pData + index_y * mapped.RowPitch);
        memcpy(&pixels[(index_y - y) * w], &src[x], w * 4);
    }

    g_pd3dDeviceContext->Unmap(texture, 0);
    texture->Release();
    return true;
}
#endif

//------------------------------------------------------------------------
// Coroutine implementation using std::thread
// This implements a coroutine using std::thread, with a helper thread for each coroutine (with serialised execution, so threads never actually run concurrently)
//------------------------------------------------------------------------

#include <thread>
#include <mutex>
#include <condition_variable>

struct ImGuiTestCoroutineData
{
    std::thread*                    Thread;             // The thread this coroutine is using
    std::condition_variable         StateChange;        // Condition variable notified when the coroutine state changes
    std::mutex                      StateMutex;         // Mutex to protect coroutine state
    bool                            CoroutineRunning;   // Is the coroutine currently running? Lock StateMutex before access and notify StateChange on change
    bool                            CoroutineTerminated;// Has the coroutine terminated? Lock StateMutex before access and notify StateChange on change
    ImVector<char>                  Name;               // The name of this coroutine
};

// The coroutine executing on the current thread (if it is a coroutine thread)
static thread_local ImGuiTestCoroutineData* GThreadCoroutine = NULL;

// The main function for a coroutine thread
void CoroutineThreadMain(ImGuiTestCoroutineData* data, ImGuiTestCoroutineFunc func, void* ctx)
{
    // Set our thread name
    ImThreadSetCurrentThreadDescription(data->Name.Data);

    // Set the thread coroutine
    GThreadCoroutine = data;

    // Wait for initial Run()
    while (1)
    {
        std::unique_lock<std::mutex> lock(data->StateMutex);
        if (data->CoroutineRunning)
        {
            break;
        }
        data->StateChange.wait(lock);
    }

    // Run user code, which will then call Yield() when it wants to yield control
    func(ctx);

    // Mark as terminated
    {
        std::lock_guard<std::mutex> lock(data->StateMutex);

        data->CoroutineTerminated = true;
        data->CoroutineRunning = false;
        data->StateChange.notify_all();
    }
}

ImGuiTestCoroutineHandle CreateCoroutine(ImGuiTestCoroutineFunc func, const char* name, void* ctx)
{
    ImGuiTestCoroutineData* data = new ImGuiTestCoroutineData();

    data->Name.resize((int)strlen(name) + 1);
    strcpy(data->Name.Data, name);
    data->CoroutineRunning = false;
    data->CoroutineTerminated = false;

    data->Thread = new std::thread(CoroutineThreadMain, data, func, ctx);

    return (ImGuiTestCoroutineHandle)data;
}

void DestroyCoroutine(ImGuiTestCoroutineHandle handle)
{
    ImGuiTestCoroutineData* data = (ImGuiTestCoroutineData*)handle;

    IM_ASSERT(data->CoroutineTerminated); // The coroutine needs to run to termination otherwise it may leak all sorts of things and this will deadlock    

    if (data->Thread)
    {
        data->Thread->join();

        delete data->Thread;
        data->Thread = NULL;
    }

    delete data;
    data = NULL;
}

// Run the coroutine until the next call to Yield(). Returns TRUE if the coroutine yielded, FALSE if it terminated (or had previously terminated)
bool RunCoroutine(ImGuiTestCoroutineHandle handle)
{
    ImGuiTestCoroutineData* data = (ImGuiTestCoroutineData*)handle;

    // Wake up coroutine thread
    {
        std::lock_guard<std::mutex> lock(data->StateMutex);

        if (data->CoroutineTerminated)
            return false; // Coroutine has already finished

        data->CoroutineRunning = true;
        data->StateChange.notify_all();
    }

    // Wait for coroutine to stop
    while (1)
    {
        std::unique_lock<std::mutex> lock(data->StateMutex);
        if (!data->CoroutineRunning)
        {
            // Breakpoint here to catch the point where we return from the coroutine
            if (data->CoroutineTerminated)
                return false; // Coroutine finished
            break;
        }
        data->StateChange.wait(lock);
    }

    return true;
}

// Yield the current coroutine (can only be called from a coroutine)
void YieldFromCoroutine()
{
    IM_ASSERT(GThreadCoroutine); // This can only be called from a coroutine thread

    ImGuiTestCoroutineData* data = GThreadCoroutine;

    // Flag that we are not running any more
    {
        std::lock_guard<std::mutex> lock(data->StateMutex);

        data->CoroutineRunning = false;
        data->StateChange.notify_all();
    }

    // At this point the thread that called RunCoroutine() will leave the "Wait for coroutine to stop" loop

    // Wait until we get started up again
    while (1)
    {
        std::unique_lock<std::mutex> lock(data->StateMutex);
        if (data->CoroutineRunning)
        {
            // Breakpoint here if you want to catch the point where execution of this coroutine resumes
            break;
        }
        data->StateChange.wait(lock);
    }
}

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
        ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
#if 0
        // Super fast mode doesn't render/present
        if (test_io.RunningTests && test_io.ConfigRunFast)
            return true;
#endif
        ImGui::Render();
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
        SDL_GL_SetSwapInterval(((test_io.RunningTests && test_io.ConfigRunFast) || test_io.ConfigNoThrottle) ? 0 : 1); // Enable vsync
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

#if defined(IMGUI_TESTS_BACKEND_GLFW_GL3)

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/gl3w.h>
#include "imgui_te_core.h"

static bool MainLoopNewFrameGLFWGL3(GLFWwindow* window)
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();
    if (glfwWindowShouldClose(window))
    {
        g_App.Quit = true;
        ImGuiTestEngine_Abort(g_App.TestEngine);
    }
    // Start the Dear ImGui frame
    if (!g_App.Quit)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
    return !g_App.Quit;
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void MainLoop()
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return;

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return;
    glfwMakeContextCurrent(window);

    // Initialize OpenGL loader
    bool err = gl3wInit() != 0;
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return;
    }

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Init
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Build();

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
    test_io.UserData = window;
    test_io.ConfigLogToTTY = true;
    test_io.NewFrameFunc = [](ImGuiTestEngine*, void* window) { return MainLoopNewFrameGLFWGL3((GLFWwindow *)window); };
    test_io.EndFrameFunc = [](ImGuiTestEngine*, void* window)
    {
        if (!MainLoopEndFrame())
            return false;
        ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
#if 0
        // Super fast mode doesn't render/present
        if (test_io.RunningTests && test_io.ConfigRunFast)
            return true;
#endif
        ImGui::Render();
        ImGuiIO& io = ImGui::GetIO();
#ifdef IMGUI_HAS_VIEWPORT
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
#endif
        glfwSwapInterval(((test_io.RunningTests && test_io.ConfigRunFast) || test_io.ConfigNoThrottle) ? 0 : 1); // Enable vsync
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(g_App.ClearColor.x, g_App.ClearColor.y, g_App.ClearColor.z, g_App.ClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers((GLFWwindow *)window);
        return true;
    };

    while (1)
    {
        if (!MainLoopNewFrameGLFWGL3(window))
            break;
        if (!test_io.EndFrameFunc(nullptr, window))
            break;
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();
}

#endif // IMGUI_TESTS_BACKEND_GLFW_GL3 

#if defined(IMGUI_TESTS_BACKEND_SDL_GL3) || defined(IMGUI_TESTS_BACKEND_GLFW_GL3)

bool CaptureFramebufferScreenshot(int x, int y, int w, int h, unsigned* pixels, void* user)
{
    int y2 = (int)ImGui::GetIO().DisplaySize.y - (y + h);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y2, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Flip vertically
    int comp = 4;
    int stride = w * comp;
    unsigned char* line_tmp = new unsigned char[stride];
    unsigned char* line_a = (unsigned char*)pixels;
    unsigned char* line_b = (unsigned char*)pixels + (stride * (h - 1));
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

#endif // IMGUI_TESTS_BACKEND_SDL_GL3 || IMGUI_TESTS_BACKEND_GLFW_GL3
