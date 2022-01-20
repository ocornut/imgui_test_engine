// dear imgui
// Minimal Application demonstrating integrating the Dear ImGui Test Engine

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

// imgui_app is a helper to wrap multiple Dear ImGui platform/renderer backends
#ifndef IMGUI_APP_IMPLEMENTATION
#define IMGUI_APP_IMPLEMENTATION 1  // Include the backend .cpp files
#endif
#include "shared/imgui_app.h"

// Test Engine
#include "imgui_test_engine/imgui_te_engine.h"
#include "imgui_test_engine/imgui_te_ui.h"
#include "imgui_test_engine/thirdparty/implot/implot.h"     // FIXME: Remove or make optional -> need to remove implot from imgui_test_engine_imconfig

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
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
#ifdef IMGUI_HAS_VIEWPORT
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

    // Setup application
    app->DpiAware = false;
    app->SrgbFramebuffer = false;
    app->ClearColor = ImVec4(0.120f, 0.120f, 0.120f, 1.000f);
    app->InitCreateWindow(app, "Dear ImGui: Minimal App With Test Engine", ImVec2(1600, 1000));
    app->InitBackends(app);

    // Setup test engine
    ImGuiTestEngine* engine = ImGuiTestEngine_CreateContext(ImGui::GetCurrentContext());
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(engine);
    test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
    test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
    test_io.ConfigRunWithGui = true;
    test_io.ConfigRunFast = false; // Default to slow mode in this demo
    test_io.ScreenCaptureFunc = ImGuiApp_ScreenCaptureFunc;
    test_io.ScreenCaptureUserData = (void*)app;

    // Optional: save test output in junit-compatible XML format.
    //test_io.ExportResultsFile = "./results.xml";
    //test_io.ExportResultsFormat = ImGuiTestEngineExportFormat_JUnitXml;

    // Register tests
    RegisterAppMinimalTests(engine);

    // Start test engine
    ImGuiTestEngine_Start(engine);

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

        ImGui::ShowDemoWindow();

        ImGuiTestEngine_ShowTestWindows(engine, NULL);

        app->Vsync = test_io.RenderWantMaxSpeed ? false : true;

        ImGui::Render();

        app->Render(app);

        ImGuiTestEngine_PostSwap(engine);
    }

    // Shutdown
    ImGuiTestEngine_Stop(engine);
    app->ShutdownBackends(app);
    app->ShutdownCloseWindow(app);
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    ImGuiTestEngine_ShutdownContext(engine);
    app->Destroy(app);

    return 0;
}
