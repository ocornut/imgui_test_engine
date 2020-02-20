// dear imgui - Standalone GUI/command-line app for Test Engine
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.

// Interactive mode, e.g.
//   main.exe [tests]
//   main.exe -gui -fileopener ..\..\tools\win32_open_with_sublime.cmd -slow
//   main.exe -gui -fileopener ..\..\tools\win32_open_with_sublime.cmd -nothrottle

// Command-line mode, e.g.
//   main.exe -nogui -v -nopause
//   main.exe -nogui -nopause perf_

#define CMDLINE_ARGS  "-fileopener tools/win32_open_with_sublime.cmd"
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
#include <sys/types.h>
#include <sys/stat.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_te_core.h"
#include "imgui_te_util.h"
#include "imgui_tests.h"
#include "imgui_capture_tool.h"
#include "backends.h"
#include "test_app.h"
#include "../helpers/imgui_app.h"
#include <Str/Str.h>

//-------------------------------------------------------------------------
// Test Application
//-------------------------------------------------------------------------

TestApp g_App;

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

    return true;
}

static bool ParseCommandLineOptions(int argc, char** argv)
{
    for (int n = 1; n < argc; n++)
    {
        if (argv[n][0] == '-')
        {
            // Command-line option
            if (strcmp(argv[n], "-v") == 0)
            {
                g_App.OptVerboseLevel = ImGuiTestVerboseLevel_Info;
                g_App.OptVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
            }
            else if (strncmp(argv[n], "-v", 2) == 0 && strlen(argv[n] + 2) == 1 && *(argv[n] + 2) >= '0' && *(argv[n] + 2) <= '5')
            {
                g_App.OptVerboseLevel = (ImGuiTestVerboseLevel)atoi(argv[n] + 2);
            }
            else if (strncmp(argv[n], "-ve", 3) == 0 && strlen(argv[n] + 3) == 1 && *(argv[n] + 3) >= '0' && *(argv[n] + 3) <= '5')
            {
                g_App.OptVerboseLevelOnError = (ImGuiTestVerboseLevel)atoi(argv[n] + 3);
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
            else if (strcmp(argv[n], "-stressamount") == 0 && n+1 < argc)
            {
                g_App.OptStressAmount = atoi(argv[n + 1]);
                n++;
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
                printf("  -h                       : show command-line help.\n");
                printf("  -v                       : verbose mode (same as -v2 -ve4)\n");
                printf("  -v0/-v1/-v2/-v3/-v4      : verbose level [v0: silent, v1: errors, v2: warnings: v3: info, v4: debug]\n");
                printf("  -ve0/-ve1/-ve2/-ve3/-ve4 : verbose level for failing tests [v0: silent, v1: errors, v2: warnings: v3: info, v4: debug]\n");
                printf("  -gui/-nogui              : enable interactive mode.\n");
                printf("  -slow                    : run automation at feeble human speed.\n");
                printf("  -nothrottle              : run GUI app without throlling/vsync by default.\n");
                printf("  -nopause                 : don't pause application on exit.\n");
                printf("  -stressamount <int>      : set performance test duration multiplier (default: 5)\n");
                printf("  -fileopener <file>       : provide a bat/cmd/shell script to open source file.\n");
                printf("Tests:\n");
                printf("   all/tests/perf          : queue by groups: all, only tests, only performance benchmarks.\n");
                printf("   [pattern]               : queue all tests containing the word [pattern].\n");
                return false;
            }
        }
        else
        {
            // Add tests
            g_App.TestsToRun.push_back(ImStrdup(argv[n]));
        }
    }
    return true;
}

// Source file opener
static void SrcFileOpenerFunc(const char* filename, int line, void*)
{
    if (!g_App.OptFileOpener)
    {
        fprintf(stderr, "Executable needs to be called with a -fileopener argument!\n");
        return;
    }

    ImGuiTextBuffer cmd_line;
    cmd_line.appendf("%s %s %d", g_App.OptFileOpener, filename, line);
    printf("Calling: '%s'\n", cmd_line.c_str());
    bool ret = ImOsCreateProcess(cmd_line.c_str());
    if (!ret)
        fprintf(stderr, "Error creating process!\n");
}

// Return value for main()
enum ImGuiTestAppErrorCode
{
    ImGuiTestAppErrorCode_Success = 0,
    ImGuiTestAppErrorCode_CommandLineError = 1,
    ImGuiTestAppErrorCode_TestFailed = 2
};

static void LoadFonts(float dpi_scale)
{
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig cfg;
    cfg.SizePixels = 13.0f * dpi_scale;
    io.Fonts->AddFontDefault(&cfg);
    //ImFontConfig cfg;
    //cfg.RasterizerMultiply = 1.1f;

    // Find font directory
    Str64 base_font_dir;
    for (int parent_level = 0; parent_level < 3; parent_level++)
    {
        base_font_dir.clear();
        for (int j = 0; j < parent_level; j++)
            base_font_dir.append("../");
        base_font_dir.append("data/fonts/");
        if (ImFileExist(base_font_dir.c_str()))
            break;
    }

    if (!base_font_dir.empty())
    {
        io.Fonts->AddFontFromFileTTF(Str64f("%s/%s", base_font_dir.c_str(), "NotoSans-Regular.ttf").c_str(), 16.0f * dpi_scale);
        io.Fonts->AddFontFromFileTTF(Str64f("%s/%s", base_font_dir.c_str(), "Roboto-Medium.ttf").c_str(), 16.0f * dpi_scale);
        //io.Fonts->AddFontFromFileTTF(Str64f("%s/%s", base_font_dir.c_str(), "RobotoMono-Regular.ttf").c_str(), 16.0f * dpi_scale, &cfg);
        //io.Fonts->AddFontFromFileTTF(Str64f("%s/%s", base_font_dir.c_str(), "Cousine-Regular.ttf").c_str(), 15.0f * dpi_scale);
        //io.Fonts->AddFontFromFileTTF(Str64f("%s/%s", base_font_dir.c_str(), "DroidSans.ttf").c_str(), 16.0f * dpi_scale);
        //io.Fonts->AddFontFromFileTTF(Str64f("%s/%s", base_font_dir.c_str(), "ProggyTiny.ttf").c_str(), 10.0f * dpi_scale);
        //IM_ASSERT(font != NULL);
    }
    else
    {
        fprintf(stderr, "Font directory not found. Fonts not loaded.\n");
    }
}

int main(int argc, char** argv)
{
#ifdef DEBUG_CRT
    DebugCrtInit(0);
#endif

    // Parse command-line arguments
#ifdef CMDLINE_ARGS
    if (argc == 1)
    {
        printf("# [exe] %s\n", CMDLINE_ARGS);
        ImParseSplitCommandLine(&argc, (const char***)&argv, CMDLINE_ARGS);
        if (!ParseCommandLineOptions(argc, argv))
            return ImGuiTestAppErrorCode_CommandLineError;
        free(argv);
    }
    else
#endif
    {
        if (!ParseCommandLineOptions(argc, argv))
            return ImGuiTestAppErrorCode_CommandLineError;
    }
    argv = NULL;

    // Default verbose level differs whether we are in in GUI or Command-Line mode
    if (g_App.OptVerboseLevel == ImGuiTestVerboseLevel_COUNT)
        g_App.OptVerboseLevel = g_App.OptGUI ? ImGuiTestVerboseLevel_Debug : ImGuiTestVerboseLevel_Silent;
    if (g_App.OptVerboseLevelOnError == ImGuiTestVerboseLevel_COUNT)
        g_App.OptVerboseLevelOnError = g_App.OptGUI ? ImGuiTestVerboseLevel_Debug : ImGuiTestVerboseLevel_Debug;

    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "imgui.ini";
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

    // Creates window
    if (g_App.OptGUI)
    {
#ifdef _WIN32
        g_App.AppWindow = ImGuiApp_ImplWin32DX11_Create();
        g_App.AppWindow->DpiAware = true;
#elif IMGUI_APP_SDL_GL3
        g_App.AppWindow = ImGuiApp_ImplSdlGL3_Create();
        g_App.AppWindow->DpiAware = true;
#elif IMGUI_APP_GLFW_GL3
        g_App.AppWindow = ImGuiApp_ImplGlfwGL3_Create();
        g_App.AppWindow->DpiAware = true;
#endif
    }
    if (g_App.AppWindow == NULL)
        g_App.AppWindow = ImGuiApp_ImplNull_Create();

    // Create TestEngine context
    IM_ASSERT(g_App.TestEngine == NULL);
    ImGuiTestEngine* engine = ImGuiTestEngine_CreateContext(ImGui::GetCurrentContext());
    g_App.TestEngine = engine;

    // Apply options
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(engine);
    test_io.ConfigRunWithGui = g_App.OptGUI;
    test_io.ConfigRunFast = g_App.OptFast;
    test_io.ConfigVerboseLevel = g_App.OptVerboseLevel;
    test_io.ConfigVerboseLevelOnError = g_App.OptVerboseLevelOnError;
    test_io.ConfigNoThrottle = g_App.OptNoThrottle;
    test_io.PerfStressAmount = g_App.OptStressAmount;
    if (!g_App.OptGUI)
        test_io.ConfigLogToTTY = true;
    if (!g_App.OptGUI && ImOsIsDebuggerPresent())
        test_io.ConfigBreakOnError = true;
    test_io.SrcFileOpenFunc = SrcFileOpenerFunc;
    test_io.UserData = (void*)&g_App;
    test_io.ScreenCaptureFunc = [](int x, int y, int w, int h, unsigned int* pixels, void* user_data) { ImGuiApp* app = g_App.AppWindow; return app->CaptureFramebuffer(app, x, y, w, h, pixels, user_data); };

    test_io.CoroutineCreateFunc = &ImCoroutineCreate;
    test_io.CoroutineDestroyFunc = &ImCoroutineDestroy;
    test_io.CoroutineRunFunc = &ImCoroutineRun;
    test_io.CoroutineYieldFunc = &ImCoroutineYield;

    // Set up TestEngine context
    RegisterTests(engine);
    ImGuiTestEngine_CalcSourceLineEnds(engine);

    // Non-interactive mode queue all tests by default
    if (!g_App.OptGUI && g_App.TestsToRun.empty())
        g_App.TestsToRun.push_back(strdup("tests"));

    // Queue requested tests
    // FIXME: Maybe need some cleanup to not hard-coded groups.
    for (int n = 0; n < g_App.TestsToRun.Size; n++)
    {
        char* test_spec = g_App.TestsToRun[n];
        if (strcmp(test_spec, "tests") == 0)
            ImGuiTestEngine_QueueTests(engine, ImGuiTestGroup_Tests, NULL, ImGuiTestRunFlags_CommandLine);
        else if (strcmp(test_spec, "perf") == 0)
            ImGuiTestEngine_QueueTests(engine, ImGuiTestGroup_Perf, NULL, ImGuiTestRunFlags_CommandLine);
        else
        {
            if (strcmp(test_spec, "all") == 0)
                test_spec = NULL;
            for (int group = 0; group < ImGuiTestGroup_COUNT; group++)
                ImGuiTestEngine_QueueTests(engine, (ImGuiTestGroup)group, test_spec, ImGuiTestRunFlags_CommandLine);
        }
        IM_FREE(test_spec);
    }
    g_App.TestsToRun.clear();

    // Branch name stored in annotation field by default
    // FIXME-TESTS: Obtain from git? maybe pipe from a batch-file?
#if defined(IMGUI_HAS_DOCK)
    strcpy(test_io.PerfAnnotation, "docking");
#elif defined(IMGUI_HAS_TABLE)
    strcpy(test_io.PerfAnnotation, "tables");
#else
    strcpy(test_io.PerfAnnotation, "master");
#endif

    // Create window
    ImGuiApp* app_window = g_App.AppWindow;
    app_window->InitCreateWindow(app_window, "Dear ImGui: Test Engine", ImVec2(1440, 900));
    app_window->InitBackends(app_window);

    // Load fonts, Set DPI scale
    LoadFonts(app_window->DpiScale);
    ImGui::GetStyle().ScaleAllSizes(app_window->DpiScale);
    test_io.DpiScale = app_window->DpiScale;

    // Main loop
    bool aborted = false;
    while (true)
    {
        if (!app_window->NewFrame(app_window))
            aborted = true;
        if (aborted)
        {
            ImGuiTestEngine_Abort(engine);
            ImGuiTestEngine_CoroutineStopRequest(engine);
            if (!ImGuiTestEngine_IsRunningTests(engine))
                break;
        }

        ImGui::NewFrame();
        MainLoopEndFrame();
        ImGui::Render();

        if (!g_App.OptGUI && !test_io.RunningTests)
            break;

        app_window->Vsync = true;
        if ((test_io.RunningTests && test_io.ConfigRunFast) || test_io.ConfigNoThrottle)
            app_window->Vsync = false;
        app_window->ClearColor = g_App.ClearColor;
        app_window->Render(app_window);
    }
    ImGuiTestEngine_CoroutineStopAndJoin(engine);

    // Print results (command-line mode)
    ImGuiTestAppErrorCode error_code = ImGuiTestAppErrorCode_Success;
    if (!aborted)
    {
        int count_tested = 0;
        int count_success = 0;
        ImGuiTestEngine_GetResult(engine, count_tested, count_success);
        ImGuiTestEngine_PrintResultSummary(engine);
        if (count_tested != count_success)
            error_code = ImGuiTestAppErrorCode_TestFailed;
    }

    // Shutdown window
    app_window->ShutdownBackends(app_window);
    app_window->ShutdownCloseWindow(app_window);

    // Shutdown
    // We shutdown the Dear ImGui context _before_ the test engine context, so .ini data may be saved.
    ImGui::DestroyContext();
    ImGuiTestEngine_ShutdownContext(g_App.TestEngine);
    app_window->Destroy(app_window);

    if (g_App.OptFileOpener)
        free(g_App.OptFileOpener);

    if (g_App.OptPauseOnExit && !g_App.OptGUI)
    {
        printf("Press Enter to exit.\n");
        getc(stdin);
    }

    return error_code;
}
