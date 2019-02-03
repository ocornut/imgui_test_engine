// dear imgui - standalone example application for DirectX 11
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.

// Interactive mode, e.g.
//   main.exe [tests]
//   main.exe -gui -fileopener ..\..\tools\win32_open_with_sublime.cmd -slow
//   main.exe -gui -fileopener ..\..\tools\win32_open_with_sublime.cmd -nothrottle

// Command-line mode, e.g.
//   main.exe -nogui -v -nopause
//   main.exe -nogui -nopause perf_

//#define CMDLINE_ARGS    "-fileopener ../../tools/win32_open_with_sublime.cmd"
//#define CMDLINE_ARGS  "-gui -nothrottle"
//#define CMDLINE_ARGS  "-nogui perf_stress_slider"
//#define CMDLINE_ARGS  "-nogui -nothrottle perf_stress_hash"

//-------------------------------------------------------------------------

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

//-------------------------------------------------------------------------

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <stdio.h>
#include <conio.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_te_core.h"
#include "imgui_te_util.h"
#include "imgui_tests.h"

//-------------------------------------------------------------------------
// DX11 Stuff
//-------------------------------------------------------------------------

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

// User state
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
static bool show_demo_window = true;
static bool show_another_window = false;

//-------------------------------------------------------------------------
// Test Application
//-------------------------------------------------------------------------

enum ImGuiBackend
{
    ImGuiBackend_Null,
    ImGuiBackend_DX11
};

struct TestApp
{
    bool                    Quit            = false;
    ImGuiTestEngine*        TestEngine      = NULL;
    ImGuiBackend            Backend         = ImGuiBackend_DX11;
    bool                    OptGUI          = true;
    bool                    OptFast         = true;
    ImGuiTestVerboseLevel   OptVerboseLevel = ImGuiTestVerboseLevel_COUNT; // Set in main.cpp
    bool                    OptNoThrottle   = false;
    bool                    OptPauseOnExit  = true;
    char*                   OptFileOpener   = NULL;
    ImVector<char*>         Tests;
    ImU64                   LastTime = 0;
};

TestApp g_App;

static bool OsCreateProcess(const char* cmd_line)
{
    STARTUPINFOA siStartInfo;
    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
    char* cmd_line_copy = strdup(cmd_line);
    BOOL ret = CreateProcessA(NULL, cmd_line_copy, NULL, NULL, FALSE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    free(cmd_line_copy);
    CloseHandle(siStartInfo.hStdInput);
    CloseHandle(siStartInfo.hStdOutput);
    CloseHandle(siStartInfo.hStdError);
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);
    return ret != 0;
}

static void FileOpenerFunc(const char* filename, int line, void*)
{
    if (!g_App.OptFileOpener)
    {
        printf("Executable needs to be called with a -fileopener argument!\n");
        return;
    }

    ImGuiTextBuffer cmd_line;
    cmd_line.appendf("%s %s %d", g_App.OptFileOpener, filename, line);
    printf("Calling: '%s'\n", cmd_line.c_str());
    bool ret = OsCreateProcess(cmd_line.c_str());
    if (!ret)
        printf("Error creating process!\n");
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
    }
    ImGui::NewFrame();

	return !g_App.Quit;
}

bool MainLoopNewFrameNull()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);

    ImU64 time = ImGetTimeInMicroseconds();
    if (g_App.LastTime == 0)
        g_App.LastTime = time;
    io.DeltaTime = (float)((time - g_App.LastTime) / 1000) / 1000.0f;
    g_App.LastTime = time;

    ImGui::NewFrame();

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
    if (!test_io.RunningTests)
        return false;

    return true;
}

bool MainLoopEndFrame()
{
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
    ImGuiTestEngine_ShowTestWindow(g_App.TestEngine, NULL);

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    ImGui::EndFrame();

#if 0
    // Super fast mode doesn't render/present
    if (test_io.RunningTests && test_io.ConfigRunFast)
        return true;
#endif

    // Rendering
    if (g_App.Backend == ImGuiBackend_DX11)
    {
        ImGui::Render();

        // Update and Render additional Platform Windows
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (test_io.RunningTests || test_io.ConfigNoThrottle)
            g_pSwapChain->Present(0, 0); // Present without vsync
        else
            g_pSwapChain->Present(1, 0); // Present with vsync
    }

    return true;
};

static bool ParseCommandLineOptions(int argc, char** argv)
{
    for (int n = 1; n < argc; n++)
    {
        if (argv[n][0] == '-' || argv[n][0] == '/')
        {
            // Command-line option
            if (strcmp(argv[n], "-v0") == 0)
            {
                g_App.OptVerboseLevel = ImGuiTestVerboseLevel_Silent;
            }
            else if (strcmp(argv[n], "-v1") == 0)
            {
                g_App.OptVerboseLevel = ImGuiTestVerboseLevel_Min;
            }
            else if (strcmp(argv[n], "-v2") == 0 || strcmp(argv[n], "-v") == 0)
            {
                g_App.OptVerboseLevel = ImGuiTestVerboseLevel_Normal;
            }
            else if (strcmp(argv[n], "-v3") == 0)
            {
                g_App.OptVerboseLevel = ImGuiTestVerboseLevel_Max;
            }
            else if (strcmp(argv[n], "-gui") == 0)
            {
                g_App.OptGUI = true;
            }
            else if (strcmp(argv[n], "-nogui") == 0)
            {
                g_App.OptGUI = false;
            }
            else if (strcmp(argv[n], "-slow") == 0)
            {
                g_App.OptFast = false;
            }
            else if (strcmp(argv[n], "-nothrottle") == 0)
            {
                g_App.OptNoThrottle = true;
            }
            else if (strcmp(argv[n], "-nopause") == 0)
            {
                g_App.OptPauseOnExit = false;
            }
            else if (strcmp(argv[n], "-fileopener") == 0 && n + 1 < argc)
            {
                g_App.OptFileOpener = strdup(argv[n + 1]);
                ImPathFixSeparatorsForCurrentOS(g_App.OptFileOpener);
                n++;
            }
            else
            {
                printf("Syntax: %s <options> [tests]\n", argv[0]);
                printf("Options:\n");
                printf("  -h                  : show command-line help.\n");
                printf("  -v                  : verbose mode (same as -v2)\n");
                printf("  -v0/-v1/-v2/-v3     : verbose level [v0: silent, v1: min, v2: normal: v3: max]\n");
                printf("  -gui/-nogui         : enable interactive mode.\n");
                printf("  -slow               : run automation at feeble human speed.\n");
                printf("  -nothrottle         : run GUI app without throlling/vsync by default.\n");
                printf("  -nopause            : don't pause application on exit.\n");
                printf("  -fileopener <file>  : provide a bat/cmd/shell script to open source file.\n");
                printf("Tests:\n");
                printf("   all                : queue all tests.\n");
                printf("   [pattern]          : queue all tests containing the word [pattern].\n");
                return false;
            }
        }
        else
        {
            // Add tests
            g_App.Tests.push_back(strdup(argv[n]));
        }
    }
    return true;
}

int main(int argc, char** argv)
{
#ifdef CMDLINE_ARGS
    if (argc == 1)
    {
        printf("# [exe] %s\n", CMDLINE_ARGS);
        ImParseSplitCommandLine(&argc, &argv, CMDLINE_ARGS);
    }
#endif

    if (!ParseCommandLineOptions(argc, argv))
        return 0;

    // Default verbose level differs whether we are in in GUI or Command-Line mode
    if (g_App.OptVerboseLevel == ImGuiTestVerboseLevel_COUNT)
        g_App.OptVerboseLevel = g_App.OptGUI ? ImGuiTestVerboseLevel_Normal : ImGuiTestVerboseLevel_Min;

    if (g_App.OptGUI == false)
        g_App.Backend = ImGuiBackend_Null;

    // Initialize back-end
    HWND hwnd = 0;
    WNDCLASSEX wc = { 0 };
    if (g_App.Backend == ImGuiBackend_DX11)
    {
        // Create application window
        wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
        RegisterClassEx(&wc);
        hwnd = CreateWindow(_T("ImGui Example"), _T("Dear ImGui - Tests"), WS_OVERLAPPEDWINDOW, 60, 60, 1280+160, 800+100, NULL, NULL, wc.hInstance, NULL);

        // Initialize Direct3D
        if (CreateDeviceD3D(hwnd) < 0)
        {
            CleanupDeviceD3D();
            UnregisterClass(_T("ImGui Example"), wc.hInstance);
            return 1;
        }

        // Show the window
        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);
    }

    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //ImGuiStyle& style = ImGui::GetStyle();
    //style.Colors[ImGuiCol_Border] = style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.0f, 0, 0, 1.0f);
    //style.FrameBorderSize = 1.0f;
    //style.FrameRounding = 5.0f;
#ifdef IMGUI_HAS_VIEWPORT
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

    // Load Fonts
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Setup TestEngine context
    IM_ASSERT(g_App.TestEngine == NULL);
    g_App.TestEngine = ImGuiTestEngine_CreateContext(ImGui::GetCurrentContext());
    RegisterTests(g_App.TestEngine);

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);

    // Non-interactive mode queue all tests by default
    if (!g_App.OptGUI && g_App.Tests.empty())
        g_App.Tests.push_back(strdup("all"));

    // Queue requested tests
    for (int n = 0; n < g_App.Tests.Size; n++)
    {
        char* test_spec = g_App.Tests[n];
        if (strcmp(test_spec, "all") == 0)
            ImGuiTestEngine_QueueTests(g_App.TestEngine, NULL);
        else
            ImGuiTestEngine_QueueTests(g_App.TestEngine, test_spec);
    }

    // Apply options
    test_io.ConfigRunFast = g_App.OptFast;
    test_io.ConfigVerboseLevel = g_App.OptVerboseLevel;
    test_io.ConfigNoThrottle = g_App.OptNoThrottle;

    switch (g_App.Backend)
    {
    case ImGuiBackend_Null:
    {
        // Init
        io.Fonts->Build();
        for (int n = 0; n < ImGuiKey_COUNT; n++)
            io.KeyMap[n] = n;
        test_io.ConfigLogToTTY = true;
        test_io.NewFrameFunc = [](ImGuiTestEngine*, void*) { return MainLoopNewFrameNull(); };
        test_io.EndFrameFunc = [](ImGuiTestEngine*, void*) { return MainLoopEndFrame(); };
        test_io.FileOpenerFunc = FileOpenerFunc;

        while (1)
        {
            if (!MainLoopNewFrameNull())
                break;
            if (!MainLoopEndFrame())
                break;
        }

        ImGuiTestEngine_PrintResultSummary(g_App.TestEngine);

        ImGui::DestroyContext();
        break;
    }
    case ImGuiBackend_DX11:
    {
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        //test_io.ConfigLogToTTY = true;
        //test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Max;

        test_io.NewFrameFunc = [](ImGuiTestEngine*, void*) { return MainLoopNewFrameDX11(); };
        test_io.EndFrameFunc = [](ImGuiTestEngine*, void*) { return MainLoopEndFrame(); };
        test_io.FileOpenerFunc = FileOpenerFunc;

        while (1)
        {
            if (!MainLoopNewFrameDX11())
                break;
            if (!MainLoopEndFrame())
                break;
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        DestroyWindow(hwnd);
        UnregisterClass(_T("ImGui Example"), wc.hInstance);

        break;
    }
    }

    ImGuiTestEngine_ShutdownContext(g_App.TestEngine);

    if (g_App.OptPauseOnExit && !g_App.OptGUI)
    {
        printf("Press Enter to exit.\n");
        getc(stdin);
    }

    return 0;
}
