// dear imgui
// Minimal Application demonstrating integrating the Dear ImGui Test Engine

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

// imgui_app is a helper to wrap multiple Dear ImGui platform/renderer backends
#include "shared/imgui_app.h"

// Test Engine
#include "imgui_test_engine/imgui_te_engine.h"
#include "imgui_test_engine/imgui_te_ui.h"
#include "imgui_test_engine/imgui_te_utils.h"   // ImOsIsDebuggerPresent()

extern void RegisterAppMinimalTests(ImGuiTestEngine* engine);

int main(int argc, char** argv)
{
    IM_UNUSED(argc);
    IM_UNUSED(argv);

    // Setup application backend
    ImGuiApp* app = ImGuiApp_ImplDefault_Create();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
#ifdef IMGUI_HAS_VIEWPORT
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
#if IMGUI_VERSION_NUM >= 19004
    io.ConfigDebugIsDebuggerPresent = ImOsIsDebuggerPresent();
#endif

    // Setup application
    app->DpiAware = false;
    app->SrgbFramebuffer = false;
    app->ClearColor = ImVec4(0.120f, 0.120f, 0.120f, 1.000f);
    app->InitCreateWindow(app, "Dear ImGui: Minimal App With Test Engine", ImVec2(1600, 1000));
    app->InitBackends(app);

    // Setup test engine
    ImGuiTestEngine* engine = ImGuiTestEngine_CreateContext();
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(engine);
    test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
    test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
    test_io.ConfigRunSpeed = ImGuiTestRunSpeed_Cinematic; // Default to slowest mode in this demo
    test_io.ScreenCaptureFunc = ImGuiApp_ScreenCaptureFunc;
    test_io.ScreenCaptureUserData = (void*)app;

    // Optional: save test output in junit-compatible XML format.
    //test_io.ExportResultsFile = "./results.xml";
    //test_io.ExportResultsFormat = ImGuiTestEngineExportFormat_JUnitXml;

    // Start test engine
    ImGuiTestEngine_Start(engine, ImGui::GetCurrentContext());
    ImGuiTestEngine_InstallDefaultCrashHandler();

    // Register tests
    RegisterAppMinimalTests(engine);

    // Main loop
    bool aborted = false;
    while (!aborted)
    {
        if (!aborted && !app->NewFrame(app))
            aborted = true;
        if (app->Quit)
            aborted = true;

        if (aborted && ImGuiTestEngine_TryAbortEngine(engine))
            break;

        ImGui::NewFrame();

        // Show windows
        ImGui::ShowDemoWindow();
        ImGuiTestEngine_ShowTestEngineWindows(engine, NULL);

        // Render and swap
        app->Vsync = test_io.IsRequestingMaxAppSpeed ? false : true;
        ImGui::Render();
        app->Render(app);

        // Post-swap handler is REQUIRED in order to support screen capture
        ImGuiTestEngine_PostSwap(engine);
    }

    // Shutdown
    ImGuiTestEngine_Stop(engine);
    app->ShutdownBackends(app);
    app->ShutdownCloseWindow(app);
    ImGui::DestroyContext();

    // IMPORTANT: we need to destroy the Dear ImGui context BEFORE the test engine context, so .ini data may be saved.
    ImGuiTestEngine_DestroyContext(engine);

    app->Destroy(app);

    return 0;
}
