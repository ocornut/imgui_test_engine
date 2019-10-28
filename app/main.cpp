// dear imgui - standalone example application for DirectX 11
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.

// Interactive mode, e.g.
//   main.exe [tests]
//   main.exe -gui -fileopener ..\..\tools\win32_open_with_sublime.cmd -slow
//   main.exe -gui -fileopener ..\..\tools\win32_open_with_sublime.cmd -nothrottle

// Command-line mode, e.g.
//   main.exe -nogui -v -nopause
//   main.exe -nogui -nopause perf_

#define CMDLINE_ARGS  "-fileopener ../tools/win32_open_with_sublime.cmd"
//#define CMDLINE_ARGS  "-gui -nothrottle"
//#define CMDLINE_ARGS    "-slow widgets_inputtext_5_deactivate_flags"
//#define CMDLINE_ARGS  "-gui perf_stress_text_unformatted_2"
//#define CMDLINE_ARGS    "-slow widgets_inputtext_5_deactivate_flags"
//#define CMDLINE_ARGS  "-nogui -v3 nav"
//#define CMDLINE_ARGS  "-slow"
//#define CMDLINE_ARGS  "-gui docking_focus -slow"
//#define CMDLINE_ARGS  "-nogui -nothrottle perf_stress_hash"

//-------------------------------------------------------------------------

#ifdef _WIN32
#define DEBUG_CRT
#endif

#ifdef DEBUG_CRT
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
static inline void DebugCrtInit(long break_alloc)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    if (break_alloc != 0)
        _CrtSetBreakAlloc(break_alloc);
}

static inline void DebugCrtDumpLeaks()
{
    _CrtDumpMemoryLeaks();
}
#endif // #ifdef DEBUG_CRT

//-------------------------------------------------------------------------

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

//-------------------------------------------------------------------------

#include "imgui.h"
#include <stdio.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_te_core.h"
#include "imgui_te_util.h"
#include "imgui_tests.h"
#include "imgui_te_app.h"

//-------------------------------------------------------------------------
// Test Application
//-------------------------------------------------------------------------

TestApp g_App;

// Main loop function implemented per-backend.
void MainLoop();
// Main loop for null backend. May be used in builds with graphics api backend.
void MainLoopNull();

bool MainLoopEndFrame()
{
    ImGuiTestEngine_ShowTestWindow(g_App.TestEngine, NULL);

    static bool show_demo_window = true;
    static bool show_another_window = false;

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
        ImGui::ColorEdit3("clear color", (float*)&g_App.ClearColor); // Edit 3 floats representing a color

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

    return true;
}

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
            else if (strcmp(argv[n], "-fast") == 0)
            {
                g_App.OptFast = true;
                g_App.OptNoThrottle = true;
            }
            else if (strcmp(argv[n], "-slow") == 0)
            {
                g_App.OptFast = false;
                g_App.OptNoThrottle = false;
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
            g_App.TestsToRun.push_back(strdup(argv[n]));
        }
    }
    return true;
}

// Return value for main()
enum ImGuiTestApp_Status
{
    ImGuiTestApp_Status_Success = 0,
    ImGuiTestApp_Status_CommandLineError = 1,
    ImGuiTestApp_Status_TestFailed = 2,
};

int main(int argc, char** argv)
{
#ifdef DEBUG_CRT
    DebugCrtInit(0);
#endif

#ifdef CMDLINE_ARGS
    if (argc == 1)
    {
        printf("# [exe] %s\n", CMDLINE_ARGS);
        ImParseSplitCommandLine(&argc, (const char***)&argv, CMDLINE_ARGS);
        if (!ParseCommandLineOptions(argc, argv))
            return ImGuiTestApp_Status_CommandLineError;
        free(argv);
    }
    else
#endif
    {
        if (!ParseCommandLineOptions(argc, argv))
            return ImGuiTestApp_Status_CommandLineError;
    }
    argv = NULL;

    // Default verbose level differs whether we are in in GUI or Command-Line mode
    if (g_App.OptVerboseLevel == ImGuiTestVerboseLevel_COUNT)
        g_App.OptVerboseLevel = g_App.OptGUI ? ImGuiTestVerboseLevel_Normal : ImGuiTestVerboseLevel_Min;

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
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigDockingTabBarOnSingleWindows = true;
#endif

    // Load Fonts
    io.Fonts->AddFontDefault();
    //ImFontConfig cfg;
    //cfg.RasterizerMultiply = 1.1f;
    //io.Fonts->AddFontFromFileTTF("../../../imgui/misc/fonts/RobotoMono-Regular.ttf", 16.0f, &cfg);
    //io.Fonts->AddFontFromFileTTF("../../../imgui/misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../../imgui/misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../../imgui/misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Setup TestEngine context
    IM_ASSERT(g_App.TestEngine == NULL);
    g_App.TestEngine = ImGuiTestEngine_CreateContext(ImGui::GetCurrentContext());
    RegisterTests(g_App.TestEngine);
    ImGuiTestEngine_CalcSourceLineEnds(g_App.TestEngine);

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);

    // Non-interactive mode queue all tests by default
    if (!g_App.OptGUI && g_App.TestsToRun.empty())
        g_App.TestsToRun.push_back(strdup("all"));

    // Queue requested tests
    for (int n = 0; n < g_App.TestsToRun.Size; n++)
    {
        char* test_spec = g_App.TestsToRun[n];
        if (strcmp(test_spec, "all") == 0)
            ImGuiTestEngine_QueueTests(g_App.TestEngine, NULL, ImGuiTestRunFlags_CommandLine);
        else
            ImGuiTestEngine_QueueTests(g_App.TestEngine, test_spec, ImGuiTestRunFlags_CommandLine);
        IM_FREE(test_spec);
    }
    g_App.TestsToRun.clear();

    // Apply options
    test_io.ConfigRunFast = g_App.OptFast;
    test_io.ConfigVerboseLevel = g_App.OptVerboseLevel;
    test_io.ConfigNoThrottle = g_App.OptNoThrottle;
    test_io.PerfStressAmount = 5;
    if (!g_App.OptGUI && ImOsIsDebuggerPresent())
        test_io.ConfigBreakOnError = true;

    if (g_App.OptGUI)
        MainLoop();
    else
        MainLoopNull();

    // Gather results
    int count_tested = 0;
    int count_success = 0;
    ImGuiTestEngine_GetResult(g_App.TestEngine, count_tested, count_success);
    ImGuiTestEngine_PrintResultSummary(g_App.TestEngine);
    ImGuiTestApp_Status error_code = ImGuiTestApp_Status_Success;
    if (count_tested != count_success)
        error_code = ImGuiTestApp_Status_TestFailed;

    // Shutdown
    // We shutdown the Dear ImGui context _before_ the test engine context, so .ini data may be saved.
    ImGui::DestroyContext();
    ImGuiTestEngine_ShutdownContext(g_App.TestEngine);

    if (g_App.OptFileOpener)
        free(g_App.OptFileOpener);

    if (g_App.OptPauseOnExit && !g_App.OptGUI)
    {
        printf("Press Enter to exit.\n");
        getc(stdin);
    }

    return error_code;
}
