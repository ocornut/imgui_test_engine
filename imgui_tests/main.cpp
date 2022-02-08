// dear imgui - Standalone GUI/command-line app for Test Engine + all regression tests for Dear ImGui

// Interactive mode, e.g.
//   main.exe [tests]
//   main.exe -gui -fileopener ..\..\tools\win32_open_with_sublime.cmd -slow
//   main.exe -gui -fileopener ..\..\tools\win32_open_with_sublime.cmd -nothrottle

// Command-line mode, e.g.
//   main.exe -nogui -v -nopause
//   main.exe -nogui -nopause perf_
//   main.exe -nogui -viewport-mock

#define CMDLINE_ARGS  "-fileopener tools/win32_open_with_sublime.cmd"
//#define CMDLINE_ARGS  "-nogui -export-format junit -export-file output/tests.junit.xml widgets_input"
//#define CMDLINE_ARGS  "-viewport-mock -nogui viewport_"               // Test mock viewports on TTY mode
//#define CMDLINE_ARGS  "-gui -nothrottle"
//#define CMDLINE_ARGS  "-slow widgets_inputtext_5_deactivate_flags"
//#define CMDLINE_ARGS  "-gui perf_stress_text_unformatted_2"
//#define CMDLINE_ARGS  "-slow widgets_inputtext_5_deactivate_flags"
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
#include "imgui_tests.h"
#include "imgui_test_engine/imgui_te_engine.h"
#include "imgui_test_engine/imgui_te_exporters.h"
#include "imgui_test_engine/imgui_te_coroutine.h"
#include "imgui_test_engine/imgui_te_utils.h"
#include "imgui_test_engine/imgui_te_ui.h"
#include "imgui_test_engine/imgui_capture_tool.h"
#include "imgui_test_engine/thirdparty/Str/Str.h"

// imgui_app
#ifndef IMGUI_APP_IMPLEMENTATION
#define IMGUI_APP_IMPLEMENTATION 1
#endif
#include "shared/imgui_app.h"

// implot
#if IMGUI_TEST_ENGINE_ENABLE_IMPLOT
#include "imgui_test_engine/thirdparty/implot/implot.h"
#endif

//-------------------------------------------------------------------------
// Allocators
//-------------------------------------------------------------------------

static void*   MallocWrapper(size_t size, void* user_data)    { IM_UNUSED(user_data); return malloc(size); }
static void    FreeWrapper(void* ptr, void* user_data)        { IM_UNUSED(user_data); free(ptr); }

//-------------------------------------------------------------------------
// Test Application
//-------------------------------------------------------------------------

struct ImGuiApp;

struct TestApp
{
    bool                    Quit = false;
    ImGuiApp*               AppWindow = NULL;
    ImGuiTestEngine*        TestEngine = NULL;
    ImU64                   LastTime = 0;
    ImVec4                  ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Command-line options
    bool                    OptGui = false;
    bool                    OptFast = true;
    bool                    OptGuiFunc = false;
    ImGuiTestVerboseLevel   OptVerboseLevelBasic = ImGuiTestVerboseLevel_COUNT; // Default is set in main.cpp depending on -gui/-nogui
    ImGuiTestVerboseLevel   OptVerboseLevelError = ImGuiTestVerboseLevel_COUNT; // "
    bool                    OptNoThrottle = false;
    bool                    OptPauseOnExit = true;
    bool                    OptViewports = false;
    bool                    OptMockViewports = false;
    int                     OptStressAmount = 5;
    Str128                  OptSourceFileOpener;
    Str128                  OptExportFilename;
    ImGuiTestEngineExportFormat OptExportFormat = ImGuiTestEngineExportFormat_JUnitXml;
    ImVector<char*>         TestsToRun;
};

TestApp g_App;

static void ShowUI()
{
    ImGuiTestEngine_ShowTestEngineWindows(g_App.TestEngine, NULL);

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
                g_App.OptVerboseLevelBasic = ImGuiTestVerboseLevel_Info;
                g_App.OptVerboseLevelError = ImGuiTestVerboseLevel_Debug;
            }
            else if (strcmp(argv[n], "-v0") == 0)   { g_App.OptVerboseLevelBasic = ImGuiTestVerboseLevel_Silent; }
            else if (strcmp(argv[n], "-v1") == 0)   { g_App.OptVerboseLevelBasic = ImGuiTestVerboseLevel_Error; }
            else if (strcmp(argv[n], "-v2") == 0)   { g_App.OptVerboseLevelBasic = ImGuiTestVerboseLevel_Warning; }
            else if (strcmp(argv[n], "-v3") == 0)   { g_App.OptVerboseLevelBasic = ImGuiTestVerboseLevel_Info; }
            else if (strcmp(argv[n], "-v4") == 0)   { g_App.OptVerboseLevelBasic = ImGuiTestVerboseLevel_Debug; }
            else if (strcmp(argv[n], "-ve0") == 0)  { g_App.OptVerboseLevelError = ImGuiTestVerboseLevel_Silent; }
            else if (strcmp(argv[n], "-ve1") == 0)  { g_App.OptVerboseLevelError = ImGuiTestVerboseLevel_Error; }
            else if (strcmp(argv[n], "-ve2") == 0)  { g_App.OptVerboseLevelError = ImGuiTestVerboseLevel_Warning; }
            else if (strcmp(argv[n], "-ve3") == 0)  { g_App.OptVerboseLevelError = ImGuiTestVerboseLevel_Info; }
            else if (strcmp(argv[n], "-ve4") == 0)  { g_App.OptVerboseLevelError = ImGuiTestVerboseLevel_Debug; }
            else if (strcmp(argv[n], "-gui") == 0)
            {
                g_App.OptGui = true;
            }
            else if (strcmp(argv[n], "-nogui") == 0)
            {
                g_App.OptGui = false;
            }
            else if (strcmp(argv[n], "-guifunc") == 0)
            {
                g_App.OptGuiFunc = true;
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
            else if (strcmp(argv[n], "-viewport") == 0)
            {
                g_App.OptViewports = true;
            }
            else if (strcmp(argv[n], "-viewport-mock") == 0)
            {
                g_App.OptViewports = g_App.OptMockViewports = true;
            }
            else if (strcmp(argv[n], "-stressamount") == 0 && n+1 < argc)
            {
                g_App.OptStressAmount = atoi(argv[n + 1]);
                n++;
            }
            else if (strcmp(argv[n], "-fileopener") == 0 && n + 1 < argc)
            {
                g_App.OptSourceFileOpener = argv[n + 1];
                ImPathFixSeparatorsForCurrentOS(g_App.OptSourceFileOpener.c_str());
                n++;
            }
            else if (strcmp(argv[n], "-export-format") == 0 && n + 1 < argc)
            {
                if (strcmp(argv[n + 1], "junit") == 0)
                {
                    g_App.OptExportFormat = ImGuiTestEngineExportFormat_JUnitXml;
                }
                else
                {
                    fprintf(stderr, "Unknown value '%s' passed to '-export-format'.", argv[n + 1]);
                    fprintf(stderr, "Possible values:\n");
                    fprintf(stderr, "- junit\n");
                }
            }
            else if (strcmp(argv[n], "-export-file") == 0 && n + 1 < argc)
            {
                g_App.OptExportFilename = argv[n + 1];
            }
            else
            {
                printf("Syntax: %s <options> [tests]\n", argv[0]);
                printf("Options:\n");
                printf("  -h                       : show command-line help.\n");
                printf("  -v                       : verbose mode (same as -v3 -ve4)\n");
                printf("  -v0/-v1/-v2/-v3/-v4      : verbose level [v0: silent, v1: errors, v2: headers & warnings, v3: info, v4: debug]\n");
                printf("  -ve0/-ve1/-ve2/-ve3/-ve4 : verbose level for errored tests [same as above]\n");
                printf("  -gui/-nogui              : enable gui/interactive mode.\n");
                printf("  -guifunc                 : run test GuiFunc only (no TestFunc).\n");
                printf("  -slow                    : run automation at feeble human speed.\n");
                printf("  -nothrottle              : run GUI app without throttling/vsync by default.\n");
                printf("  -nopause                 : don't pause application on exit.\n");
                printf("  -stressamount <int>      : set performance test duration multiplier (default: 5)\n");
                printf("  -fileopener <file>       : provide a bat/cmd/shell script to open source file.\n");
                printf("  -export-file <file>      : save test run results in specified file.\n");
                printf("  -export-format <format>   : save test run results in specified format. (default: junit)\n");
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
    if (g_App.OptSourceFileOpener.empty())
    {
        fprintf(stderr, "Executable needs to be called with a -fileopener argument!\n");
        return;
    }

    Str256f cmd_line("%s %s %d", g_App.OptSourceFileOpener.c_str(), filename, line);
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

    Str64 base_font_dir;
    if (ImFileFindInParents("data/fonts/", 3, &base_font_dir))
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
    io.Fonts->Build();
}

static void QueueTests(ImGuiTestEngine* engine)
{
    // Non-interactive mode queue all tests by default
    if (!g_App.OptGui && g_App.TestsToRun.empty())
        g_App.TestsToRun.push_back(strdup("tests"));

    // Queue requested tests
    // FIXME: Maybe need some cleanup to not hard-coded groups.
    ImGuiTestRunFlags run_flags = ImGuiTestRunFlags_CommandLine;
    if (g_App.OptGuiFunc)
        run_flags |= ImGuiTestRunFlags_GuiFuncOnly;
    for (int n = 0; n < g_App.TestsToRun.Size; n++)
    {
        char* test_spec = g_App.TestsToRun[n];
        if (strcmp(test_spec, "tests") == 0)
            ImGuiTestEngine_QueueTests(engine, ImGuiTestGroup_Tests, NULL, run_flags);
        else if (strcmp(test_spec, "perf") == 0)
            ImGuiTestEngine_QueueTests(engine, ImGuiTestGroup_Perfs, NULL, run_flags);
        else
        {
            if (strcmp(test_spec, "all") == 0)
                test_spec = NULL;
            for (int group = 0; group < ImGuiTestGroup_COUNT; group++)
                ImGuiTestEngine_QueueTests(engine, (ImGuiTestGroup)group, test_spec, run_flags);
        }
        IM_FREE(test_spec);
    }
    g_App.TestsToRun.clear();
}

int main(int argc, char** argv)
{
#ifdef DEBUG_CRT
    DebugCrtInit(0);
#endif

    // Parse command-line arguments
#if defined(IMGUI_APP_WIN32_DX11) || defined(IMGUI_APP_SDL_GL2) || defined(IMGUI_APP_SDL_GL3) || defined(IMGUI_APP_GLFW_GL3)
    g_App.OptGui = true;
#endif

#ifdef CMDLINE_ARGS
    if (argc == 1)
    {
        printf("# [exe] %s\n", CMDLINE_ARGS);
        ImParseExtractArgcArgvFromCommandLine(&argc, (const char***)&argv, CMDLINE_ARGS);
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

    // Default verbose levels differs whether we are in in GUI or Command-Line mode
    if (g_App.OptGui)
    {
        // Default -v4 -ve4
        if (g_App.OptVerboseLevelBasic == ImGuiTestVerboseLevel_COUNT)
            g_App.OptVerboseLevelBasic = ImGuiTestVerboseLevel_Debug;
        if (g_App.OptVerboseLevelError == ImGuiTestVerboseLevel_COUNT)
            g_App.OptVerboseLevelError = ImGuiTestVerboseLevel_Debug;
    }
    else
    {
        // Default -v2 -ve4
        if (g_App.OptVerboseLevelBasic == ImGuiTestVerboseLevel_COUNT)
            g_App.OptVerboseLevelBasic = ImGuiTestVerboseLevel_Warning;
        if (g_App.OptVerboseLevelError == ImGuiTestVerboseLevel_COUNT)
            g_App.OptVerboseLevelError = ImGuiTestVerboseLevel_Debug;
    }

    // Custom allocator functions, only to test overriding of allocators.
    ImGui::SetAllocatorFunctions(&MallocWrapper, &FreeWrapper, &g_App);
    ImGuiMemAllocFunc alloc_func;
    ImGuiMemFreeFunc free_func;
    void* alloc_user_data;
    ImGui::GetAllocatorFunctions(&alloc_func, &free_func, &alloc_user_data);
    IM_ASSERT(alloc_func == &MallocWrapper);
    IM_ASSERT(free_func == &FreeWrapper);
    IM_ASSERT(alloc_user_data == &g_App);

    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
#if IMGUI_TEST_ENGINE_ENABLE_IMPLOT
    ImPlot::CreateContext();
#endif
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "imgui.ini";
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //ImGuiStyle& style = ImGui::GetStyle();
    //style.Colors[ImGuiCol_Border] = style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.0f, 0, 0, 1.0f);
    //style.FrameBorderSize = 1.0f;
    //style.FrameRounding = 5.0f;
#ifdef IMGUI_HAS_VIEWPORT
    if (g_App.OptViewports)
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigDockingTabBarOnSingleWindows = true;
#endif

    // Creates window
    if (g_App.OptGui)
        g_App.AppWindow = ImGuiApp_ImplDefault_Create();
    if (g_App.AppWindow == NULL)
        g_App.AppWindow = ImGuiApp_ImplNull_Create();
    g_App.AppWindow->DpiAware = false;
    g_App.AppWindow->MockViewports = g_App.OptViewports && g_App.OptMockViewports;

    // Create TestEngine context
    IM_ASSERT(g_App.TestEngine == NULL);
    ImGuiTestEngine* engine = ImGuiTestEngine_CreateContext(ImGui::GetCurrentContext());
    g_App.TestEngine = engine;

    // Apply options
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(engine);
    test_io.ConfigRunFast = g_App.OptFast;
    test_io.ConfigVerboseLevel = g_App.OptVerboseLevelBasic;
    test_io.ConfigVerboseLevelOnError = g_App.OptVerboseLevelError;
    test_io.ConfigNoThrottle = g_App.OptNoThrottle;
    test_io.PerfStressAmount = g_App.OptStressAmount;

    if (g_App.OptGui)
    {
        test_io.ConfigWatchdogWarning = 30.0f;
        test_io.ConfigWatchdogKillTest = 60.0f;
        test_io.ConfigWatchdogKillApp = FLT_MAX;
        test_io.ConfigMouseDrawCursor = false;
    }
    else
    {
        test_io.ConfigWatchdogWarning = 15.0f;
        test_io.ConfigWatchdogKillTest = 30.0f;
        test_io.ConfigWatchdogKillApp = 35.0f;
        test_io.ConfigLogToTTY = true;
        if (ImOsIsDebuggerPresent())
        {
            test_io.ConfigLogToDebugger = true;
            test_io.ConfigBreakOnError = true;
        }
    }

    // Set up functions
    test_io.SrcFileOpenFunc = g_App.OptSourceFileOpener.empty() ? NULL : SrcFileOpenerFunc;
    test_io.SrcFileOpenUserData = NULL;
    test_io.ScreenCaptureFunc = ImGuiApp_ScreenCaptureFunc;
    test_io.ScreenCaptureUserData = (void*)g_App.AppWindow;

    // Enable test result export
    if (!g_App.OptExportFilename.empty())
    {
        if (!g_App.TestsToRun.empty())
        {
            test_io.ExportResultsFilename = g_App.OptExportFilename.c_str();
            test_io.ExportResultsFormat = !g_App.OptExportFilename.empty() ? g_App.OptExportFormat : ImGuiTestEngineExportFormat_None;
        }
        else
        {
            fprintf(stderr, "-junit-xml parameter is ignored in interactive runs.");
        }
    }

    // Create window
    ImGuiApp* app_window = g_App.AppWindow;
    app_window->InitCreateWindow(app_window, "Dear ImGui: Test Engine", ImVec2(1440, 900));
    app_window->InitBackends(app_window);

    // Register and queue our tests
    RegisterTests(engine);
    QueueTests(engine);
    bool exit_after_tests = !ImGuiTestEngine_IsTestQueueEmpty(engine) && !g_App.OptPauseOnExit;

    // Retrieve Git branch name, store in annotation field by default
    Str64 git_repo_path;
    Str64 git_branch;
    if (ImFileFindInParents("imgui/", 4, &git_repo_path))
        if (ImBuildGetGitBranchName(git_repo_path.c_str(), &git_branch))
            strncpy(test_io.GitBranchName, git_branch.c_str(), IM_ARRAYSIZE(test_io.GitBranchName));
    if (!test_io.GitBranchName[0])
    {
        strcpy(test_io.GitBranchName, "unknown");
        fprintf(stderr, "Dear ImGui git repository was not found.\n");
    }
    printf("Git branch: \"%s\"\n", test_io.GitBranchName);

    // Start engine
    ImGuiTestEngine_Start(engine);

    // Load fonts, Set DPI scale
    LoadFonts(app_window->DpiScale);
    ImGui::GetStyle().ScaleAllSizes(app_window->DpiScale);
    test_io.DpiScale = app_window->DpiScale;

    // Main loop
    bool aborted = false;
    while (true)
    {
        // Backend update
        // (stop updating them once we started aborting, as e.g. closed windows will have zero size etc.)
        if (!aborted && !app_window->NewFrame(app_window))
            aborted = true;

        // Abort logic
        if (aborted && ImGuiTestEngine_TryAbortEngine(engine))
            break;

        if (exit_after_tests && ImGuiTestEngine_IsTestQueueEmpty(engine))
            break;

        ImGui::NewFrame();
        ShowUI();

#if IMGUI_VERSION_NUM >= 18701
        if (test_io.RunningTests && !test_io.ConfigMouseDrawCursor)
            ImGui::RenderMouseCursor(io.MousePos, 1.2f, ImGui::GetMouseCursor(), IM_COL32(255, 255, 120, 255), IM_COL32(0, 0, 0, 255), IM_COL32(0, 0, 0, 60));
#endif

        ImGui::Render();

        if (!g_App.OptGui && !test_io.RunningTests)
            break;

        app_window->Vsync = test_io.RenderWantMaxSpeed ? false : true;
        app_window->ClearColor = g_App.ClearColor;
        app_window->Render(app_window);

        ImGuiTestEngine_PostSwap(engine);
    }

    ImGuiTestEngine_Stop(engine);

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
    // IMPORTANT: we need to destroy the Dear ImGui context BEFORE the test engine context, so .ini data may be saved.
#if IMGUI_TEST_ENGINE_ENABLE_IMPLOT
    ImPlot::DestroyContext();
#endif
    ImGui::DestroyContext();
    ImGuiTestEngine_DestroyContext(g_App.TestEngine);
    app_window->Destroy(app_window);

    if (g_App.OptPauseOnExit && !g_App.OptGui)
    {
        printf("Press Enter to exit.\n");
        getc(stdin);
    }

    return error_code;
}
