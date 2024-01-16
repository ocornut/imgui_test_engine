// dear imgui - Standalone GUI/command-line app for Test Engine + all regression tests for Dear ImGui

// Interactive mode, e.g.
//   main.exe [tests]
//   main.exe -gui -fileopener ..\..\tools\win32_open_with_sublime.cmd -slow
//   main.exe -gui -fileopener ..\..\tools\win32_open_with_sublime.cmd -nothrottle

// Command-line mode, e.g.
//   main.exe -list                         // List available tests
//   main.exe -list table_                  // List tests matching "table_"
//   main.exe -list "^table_"               // List tests starting with "table_"
//   main.exe -nogui -v -nopause            // Run all tests
//   main.exe -nogui -nopause testname      // Run tests matching "testname"
//   main.exe -nogui -viewport-mock         // Run with viewport emulation

// Examples
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
// Includes & Compiler Stuff
//-------------------------------------------------------------------------

#ifdef _WIN32
#define DEBUG_CRT
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif // #ifdef DEBUG_CRT

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

// Includes
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include <stdio.h>
#include "imgui_test_suite.h"
#include "imgui_test_engine/imgui_te_engine.h"
#include "imgui_test_engine/imgui_te_exporters.h"
#include "imgui_test_engine/imgui_te_coroutine.h"
#include "imgui_test_engine/imgui_te_utils.h"
#include "imgui_test_engine/imgui_te_ui.h"
#include "imgui_test_engine/imgui_capture_tool.h"
#include "imgui_test_engine/thirdparty/Str/Str.h"

// imgui_app (this is a helper to wrap multiple backends)
#include "shared/imgui_app.h"

// ImPlot (optional for users of test engine, but we use it in test suite)
#if IMGUI_TEST_ENGINE_ENABLE_IMPLOT
#include "thirdparty/implot/implot.h"
#endif

//-------------------------------------------------------------------------
// Allocators
//-------------------------------------------------------------------------

static void*   MallocWrapper(size_t size, void* user_data)    { IM_UNUSED(user_data); return malloc(size); }
static void    FreeWrapper(void* ptr, void* user_data)        { IM_UNUSED(user_data); free(ptr); }

//-------------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------------

struct TestSuiteApp;
static void TestSuite_ShowUI(TestSuiteApp* app);
static void TestSuite_PrintCommandLineHelp();
static bool TestSuite_ParseCommandLineOptions(TestSuiteApp* app, int argc, char** argv);
static void TestSuite_QueueTests(TestSuiteApp* app, ImGuiTestRunFlags run_flags);
static void TestSuite_LoadFonts(float dpi_scale);

//-------------------------------------------------------------------------
// Test Application
//-------------------------------------------------------------------------

struct ImGuiApp;

struct TestSuiteApp
{
    // Main State
    bool                        Quit = false;
    ImGuiApp*                   AppWindow = NULL;
    ImGuiTestEngine*            TestEngine = NULL;
    ImVec4                      ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Command-line options
    bool                        OptGui = false;
    bool                        OptGuiFunc = false;
    bool                        OptListTests = false;
    ImGuiTestRunSpeed           OptRunSpeed = ImGuiTestRunSpeed_Fast;
    ImGuiTestVerboseLevel       OptVerboseLevelBasic = ImGuiTestVerboseLevel_COUNT; // Default is set in main.cpp depending on -gui/-nogui
    ImGuiTestVerboseLevel       OptVerboseLevelError = ImGuiTestVerboseLevel_COUNT; // "
    bool                        OptNoThrottle = false;
    bool                        OptPauseOnExit = true;
    bool                        OptViewports = false;
    bool                        OptMockViewports = false;
    bool                        OptCaptureEnabled = true;
    int                         OptStressAmount = 5;
    Str128                      OptSourceFileOpener;
    Str128                      OptExportFilename;
    ImGuiTestEngineExportFormat OptExportFormat = ImGuiTestEngineExportFormat_JUnitXml;
    ImVector<char*>             TestsToRun;
};

static void TestSuite_ShowUI(TestSuiteApp* app)
{
    ImGuiTestEngine_ShowTestEngineWindows(app->TestEngine, NULL);

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
        ImGui::ColorEdit3("clear color", (float*)&app->ClearColor); // Edit 3 floats representing a color

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

static void TestSuite_PrintCommandLineHelp()
{
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
    printf("  -nocapture               : don't capture any images or video.\n");
    printf("  -stressamount <int>      : set performance test duration multiplier (default: 5)\n");
    printf("  -fileopener <file>       : provide a bat/cmd/shell script to open source file.\n");
    printf("  -export-file <file>      : save test run results in specified file.\n");
    printf("  -export-format <format>  : save test run results in specified format. (default: junit)\n");
    printf("  -list                    : list queued tests (one per line) and exit.\n");
    printf("Tests:\n");
    printf("   all/tests/perf          : queue by groups: all, only tests, only performance benchmarks.\n");
    printf("   [pattern]               : queue all tests containing the word [pattern].\n");
    printf("   [-pattern]              : queue all tests not containing the word [pattern].\n");
    printf("   [^pattern]              : queue all tests starting with the word [pattern].\n");
}

static bool TestSuite_ParseCommandLineOptions(TestSuiteApp* app, int argc, char** argv)
{
    bool end_of_options = false;
    for (int n = 1; n < argc; n++)
    {
        if (end_of_options || argv[n][0] != '-')
        {
            // Queue test name or test pattern
            app->TestsToRun.push_back(ImStrdup(argv[n]));
            continue;
        }

        // Parse Command-line option
        if (strcmp(argv[n], "-v") == 0)
        {
            app->OptVerboseLevelBasic = ImGuiTestVerboseLevel_Info;
            app->OptVerboseLevelError = ImGuiTestVerboseLevel_Debug;
        }
        else if (strcmp(argv[n], "--") == 0)            { end_of_options = true; }
        else if (strcmp(argv[n], "-v0") == 0)           { app->OptVerboseLevelBasic = ImGuiTestVerboseLevel_Silent; }
        else if (strcmp(argv[n], "-v1") == 0)           { app->OptVerboseLevelBasic = ImGuiTestVerboseLevel_Error; }
        else if (strcmp(argv[n], "-v2") == 0)           { app->OptVerboseLevelBasic = ImGuiTestVerboseLevel_Warning; }
        else if (strcmp(argv[n], "-v3") == 0)           { app->OptVerboseLevelBasic = ImGuiTestVerboseLevel_Info; }
        else if (strcmp(argv[n], "-v4") == 0)           { app->OptVerboseLevelBasic = ImGuiTestVerboseLevel_Debug; }
        else if (strcmp(argv[n], "-ve0") == 0)          { app->OptVerboseLevelError = ImGuiTestVerboseLevel_Silent; }
        else if (strcmp(argv[n], "-ve1") == 0)          { app->OptVerboseLevelError = ImGuiTestVerboseLevel_Error; }
        else if (strcmp(argv[n], "-ve2") == 0)          { app->OptVerboseLevelError = ImGuiTestVerboseLevel_Warning; }
        else if (strcmp(argv[n], "-ve3") == 0)          { app->OptVerboseLevelError = ImGuiTestVerboseLevel_Info; }
        else if (strcmp(argv[n], "-ve4") == 0)          { app->OptVerboseLevelError = ImGuiTestVerboseLevel_Debug; }
        else if (strcmp(argv[n], "-gui") == 0)          { app->OptGui = true; }
        else if (strcmp(argv[n], "-nogui") == 0)        { app->OptGui = false; }
        else if (strcmp(argv[n], "-guifunc") == 0)      { app->OptGuiFunc = true; }
        else if (strcmp(argv[n], "-fast") == 0)         { app->OptRunSpeed = ImGuiTestRunSpeed_Fast; app->OptNoThrottle = true; }
        else if (strcmp(argv[n], "-slow") == 0)         { app->OptRunSpeed = ImGuiTestRunSpeed_Normal; app->OptNoThrottle = false; }
        else if (strcmp(argv[n], "-nothrottle") == 0)   { app->OptNoThrottle = true; }
        else if (strcmp(argv[n], "-nopause") == 0)      { app->OptPauseOnExit = false; }
        else if (strcmp(argv[n], "-nocapture") == 0)    { app->OptCaptureEnabled = false; }
        else if (strcmp(argv[n], "-viewport") == 0)     { app->OptViewports = true; }
        else if (strcmp(argv[n], "-viewport-mock") == 0){ app->OptViewports = app->OptMockViewports = true; }
        else if (strcmp(argv[n], "-stressamount") == 0 && n + 1 < argc)
        {
            app->OptStressAmount = atoi(argv[n + 1]);
            n++;
        }
        else if (strcmp(argv[n], "-fileopener") == 0 && n + 1 < argc)
        {
            app->OptSourceFileOpener = argv[n + 1];
            ImPathFixSeparatorsForCurrentOS(app->OptSourceFileOpener.c_str());
            n++;
        }
        else if (strcmp(argv[n], "-export-format") == 0 && n + 1 < argc)
        {
            if (strcmp(argv[n + 1], "junit") == 0)
            {
                app->OptExportFormat = ImGuiTestEngineExportFormat_JUnitXml;
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
            app->OptExportFilename = argv[n + 1];
        }
        else if (strcmp(argv[n], "-list") == 0)
        {
            app->OptListTests = true;
            app->OptGui = false;
        }
        else
        {
            printf("Syntax: %s <options> [tests...]\n", argv[0]);
            TestSuite_PrintCommandLineHelp();
            return false;
        }
    }
    return true;
}

// Source file opener
static void SrcFileOpenerFunc(const char* filename, int line, void* user_data)
{
    TestSuiteApp* app = (TestSuiteApp*)user_data;
    if (app->OptSourceFileOpener.empty())
    {
        fprintf(stderr, "Executable needs to be called with a -fileopener argument!\n");
        return;
    }

    Str256f cmd_line("%s %s %d", app->OptSourceFileOpener.c_str(), filename, line);
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

static void TestSuite_LoadFonts(float dpi_scale)
{
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig cfg;
    cfg.SizePixels = IM_ROUND(13.0f * dpi_scale);
    io.Fonts->AddFontDefault(&cfg);
    //ImFontConfig cfg;
    //cfg.RasterizerMultiply = 1.1f;

    Str64 base_font_dir;
    if (ImFileFindInParents("imgui_test_suite/assets/fonts/", 3, &base_font_dir))
    {
        io.Fonts->AddFontFromFileTTF(Str64f("%s/%s", base_font_dir.c_str(), "NotoSans-Regular.ttf").c_str(), IM_ROUND(16.0f * dpi_scale));
        io.Fonts->AddFontFromFileTTF(Str64f("%s/%s", base_font_dir.c_str(), "Roboto-Medium.ttf").c_str(), IM_ROUND(16.0f * dpi_scale));
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

static void TestSuite_QueueTests(TestSuiteApp* app, ImGuiTestRunFlags run_flags)
{
    // Non-interactive mode queue all tests by default
    if (!app->OptGui && app->TestsToRun.empty())
        app->TestsToRun.push_back(strdup("tests"));

    // Special groups are supported by ImGuiTestEngine_QueueTests(): "all", "tests", "perfs"
    // Following command line examples are functionally identical:
    //  ./imgui_test_suite tests,-window
    //  ./imgui_test_suite -- tests -window
    // See comments above ImGuiTestEngine_QueueTests() for more details.
    Str256 filter;
    for (int n = 0; n < app->TestsToRun.Size; n++)
    {
        char* test_spec = app->TestsToRun[n];
        if (!filter.empty())
            filter.append(",");
        filter.append(test_spec);
        IM_FREE(test_spec);
    }
    ImGuiTestEngine_QueueTests(app->TestEngine, ImGuiTestGroup_Unknown, filter.c_str(), run_flags);
    app->TestsToRun.clear();
}

static void FindVideoEncoder(char* out, int out_len)
{
    IM_ASSERT(out != NULL);
    IM_ASSERT(out_len > 0);
    Str64 encoder_path("imgui_test_suite/tools/");           // Assume execution from root repository folder by default (linux/macos)
    if (!ImFileExist(encoder_path.c_str()))
        ImFileFindInParents("tools/", 3, &encoder_path);     // Assume execution from imgui_test_suite/$(Configuration) (visual studio)
    encoder_path.append("ffmpeg");                           // Look for ffmpeg executable in tools folder (windows/macos)
#if _WIN32
    encoder_path.append(".exe");
#else
    if (!ImFileExist(encoder_path.c_str()))
        encoder_path.set("/usr/bin/ffmpeg");                 // Use system version path (linux)
#endif
    if (ImFileExist(encoder_path.c_str()))
        ImStrncpy(out, encoder_path.c_str(), out_len);
    else
        *out = 0;
}

// Win32 Debug CRT to help catch leaks. Replace parameter in main() to track a given allocation from the ID given in leak report.
#ifdef DEBUG_CRT
static inline void DebugCrtInit(long break_alloc)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
    if (break_alloc != 0)
        _CrtSetBreakAlloc(break_alloc);
}

static inline void DebugCrtDumpLeaks()
{
    _CrtDumpMemoryLeaks();
}
#endif // #ifdef DEBUG_CRT

// Application entry point
int main(int argc, char** argv)
{
#ifdef DEBUG_CRT
    DebugCrtInit(0);
#endif

    TestSuiteApp GAppInstance;
    TestSuiteApp* app = &GAppInstance;

    // Default to GUI mode when a graphics backend is compiled
#if defined(IMGUI_APP_WIN32_DX11) || defined(IMGUI_APP_SDL2_GL2) || defined(IMGUI_APP_SDL2_GL3) || defined(IMGUI_APP_GLFW_GL3)
    app->OptGui = true;
#endif

    // Parse command-line arguments
#ifdef CMDLINE_ARGS
    if (argc == 1)
    {
        printf("# [exe] %s\n", CMDLINE_ARGS);
        ImParseExtractArgcArgvFromCommandLine(&argc, (const char***)&argv, CMDLINE_ARGS);
        if (!TestSuite_ParseCommandLineOptions(app, argc, argv))
            return ImGuiTestAppErrorCode_CommandLineError;
        free(argv);
    }
    else
#endif
    {
        if (!TestSuite_ParseCommandLineOptions(app, argc, argv))
            return ImGuiTestAppErrorCode_CommandLineError;
    }
    argv = NULL;

    // Default verbose levels differs whether we are in in GUI or Command-Line mode
    if (app->OptGui)
    {
        // Default -v4 -ve4
        if (app->OptVerboseLevelBasic == ImGuiTestVerboseLevel_COUNT)
            app->OptVerboseLevelBasic = ImGuiTestVerboseLevel_Debug;
        if (app->OptVerboseLevelError == ImGuiTestVerboseLevel_COUNT)
            app->OptVerboseLevelError = ImGuiTestVerboseLevel_Debug;
    }
    else
    {
        // Default -v2 -ve4
        if (app->OptVerboseLevelBasic == ImGuiTestVerboseLevel_COUNT)
            app->OptVerboseLevelBasic = ImGuiTestVerboseLevel_Warning;
        if (app->OptVerboseLevelError == ImGuiTestVerboseLevel_COUNT)
            app->OptVerboseLevelError = ImGuiTestVerboseLevel_Debug;
    }

    // Setup Dear ImGui binding
    // (We use a custom allocator but mostly to exercise that overriding)
    IMGUI_CHECKVERSION();
    ImGui::SetAllocatorFunctions(&MallocWrapper, &FreeWrapper, app);
    ImGui::CreateContext();
#if IMGUI_TEST_ENGINE_ENABLE_IMPLOT
    ImPlot::CreateContext();
#endif

    // Setup Configuration & Style
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "imgui.ini";
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
#ifdef IMGUI_HAS_VIEWPORT
    if (app->OptViewports)
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
#if IMGUI_VERSION_NUM >= 19004
    io.ConfigDebugIsDebuggerPresent = ImOsIsDebuggerPresent();
#endif
    ImGui::StyleColorsDark();

    // Creates Application Wrapper
    if (app->OptGui)
        app->AppWindow = ImGuiApp_ImplDefault_Create();
    if (app->AppWindow == NULL)
        app->AppWindow = ImGuiApp_ImplNull_Create();
    app->AppWindow->DpiAware = false;
    app->AppWindow->MockViewports = app->OptViewports && app->OptMockViewports;

    // Create TestEngine context
    IM_ASSERT(app->TestEngine == NULL);
    ImGuiTestEngine* engine = ImGuiTestEngine_CreateContext();
    app->TestEngine = engine;

    // Apply Options to TestEngine
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(engine);
    test_io.ConfigRunSpeed = app->OptRunSpeed;
    test_io.ConfigVerboseLevel = app->OptVerboseLevelBasic;
    test_io.ConfigVerboseLevelOnError = app->OptVerboseLevelError;
    test_io.ConfigNoThrottle = app->OptNoThrottle;
    test_io.PerfStressAmount = app->OptStressAmount;
    test_io.ConfigCaptureEnabled = app->OptCaptureEnabled;
    FindVideoEncoder(test_io.VideoCaptureEncoderPath, IM_ARRAYSIZE(test_io.VideoCaptureEncoderPath));
    ImStrncpy(test_io.VideoCaptureEncoderParams, IMGUI_CAPTURE_DEFAULT_VIDEO_PARAMS_FOR_FFMPEG, IM_ARRAYSIZE(test_io.VideoCaptureEncoderParams));
    ImStrncpy(test_io.GifCaptureEncoderParams, IMGUI_CAPTURE_DEFAULT_GIF_PARAMS_FOR_FFMPEG, IM_ARRAYSIZE(test_io.GifCaptureEncoderParams));
    test_io.CheckDrawDataIntegrity = true;

    if (app->OptGui)
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

    // Set up source file opener and framebuffer capture functions
    test_io.SrcFileOpenFunc = app->OptSourceFileOpener.empty() ? NULL : SrcFileOpenerFunc;
    test_io.SrcFileOpenUserData = (void*)app;
    test_io.ScreenCaptureFunc = ImGuiApp_ScreenCaptureFunc;
    test_io.ScreenCaptureUserData = (void*)app->AppWindow;

    // Enable test result export
    if (!app->OptExportFilename.empty())
    {
        if (!app->TestsToRun.empty())
        {
            test_io.ExportResultsFilename = app->OptExportFilename.c_str();
            test_io.ExportResultsFormat = !app->OptExportFilename.empty() ? app->OptExportFormat : ImGuiTestEngineExportFormat_None;
        }
        else
        {
            fprintf(stderr, "-junit-xml parameter is ignored in interactive runs.");
        }
    }

    // Create Application Window, Initialize Backends
    ImGuiApp* app_window = app->AppWindow;
    app_window->InitCreateWindow(app_window, "Dear ImGui Test Suite", ImVec2(1440, 900));
    app_window->InitBackends(app_window);

    // Register and queue our tests
    RegisterTests_All(engine);

    // Queue requested tests
    ImGuiTestRunFlags test_run_flags = ImGuiTestRunFlags_RunFromCommandLine;
    if (app->OptGuiFunc)
        test_run_flags |= ImGuiTestRunFlags_GuiFuncOnly;
    TestSuite_QueueTests(app, test_run_flags);
    const bool exit_after_tests = !ImGuiTestEngine_IsTestQueueEmpty(engine) && !app->OptPauseOnExit;

    // Retrieve Git branch name, store in annotation field by default
    Str64 git_repo_path;
    Str64 git_branch;
    if (ImFileFindInParents("imgui/", 4, &git_repo_path))
        if (ImBuildFindGitBranchName(git_repo_path.c_str(), &git_branch))
            strncpy(test_io.GitBranchName, git_branch.c_str(), IM_ARRAYSIZE(test_io.GitBranchName));
    if (!test_io.GitBranchName[0])
    {
        strcpy(test_io.GitBranchName, "unknown");
        fprintf(stderr, "Dear ImGui git repository was not found.\n");
    }
    printf("Git branch: \"%s\"\n", test_io.GitBranchName);

    // List all queued tests and exit the program
    if (app->OptListTests)
    {
        ImVector<ImGuiTestRunTask> tests;
        ImGuiTestEngine_GetTestQueue(engine, &tests);
        for (ImGuiTestRunTask& test_task : tests)
            printf("Test: '%s' '%s'\n", test_task.Test->Category, test_task.Test->Name);
        return 0;
    }

    // Start engine
    ImGuiTestEngine_Start(engine, ImGui::GetCurrentContext());
    ImGuiTestEngine_InstallDefaultCrashHandler();

    // Load fonts, Set DPI scale
    //const float dpi_scale = app_window->DpiScale;
    const float dpi_scale = 1.0f;
    TestSuite_LoadFonts(dpi_scale);
    ImGui::GetStyle().ScaleAllSizes(dpi_scale);

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
        TestSuite_ShowUI(app);

        // Optionally draw a non-ambiguous mouse cursor when simulated inputs are running
#if IMGUI_VERSION_NUM >= 18701
        if (!test_io.ConfigMouseDrawCursor && !test_io.IsCapturing && ImGuiTestEngine_IsUsingSimulatedInputs(engine))
            ImGui::RenderMouseCursor(io.MousePos, 1.0f, ImGui::GetMouseCursor(), IM_COL32_WHITE, IM_COL32_BLACK, IM_COL32(0, 0, 0, 48));
            //ImGui::RenderMouseCursor(io.MousePos, 1.2f, ImGui::GetMouseCursor(), IM_COL32(255, 255, 120, 255), IM_COL32(0, 0, 0, 255), IM_COL32(0, 0, 0, 60)); // Custom yellow cursor
#endif

        ImGui::Render();

        if (!app->OptGui && !test_io.IsRunningTests)
            break;

        app_window->Vsync = test_io.IsRequestingMaxAppSpeed ? false : true;
        app_window->ClearColor = app->ClearColor;
        app_window->Render(app_window);

        // Post-swap handler is REQUIRED in order to support screen capture
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

    // Shutdown Application Window
    app_window->ShutdownBackends(app_window);
    app_window->ShutdownCloseWindow(app_window);

    // Shutdown
    // IMPORTANT: we need to destroy the Dear ImGui context BEFORE the test engine context, so .ini data may be saved.
#if IMGUI_TEST_ENGINE_ENABLE_IMPLOT
    ImPlot::DestroyContext();
#endif
    ImGui::DestroyContext();
    ImGuiTestEngine_DestroyContext(app->TestEngine);
    app_window->Destroy(app_window);

    if (app->OptPauseOnExit && !app->OptGui)
    {
        printf("Press Enter to exit.\n");
        getc(stdin);
    }

    return error_code;
}
