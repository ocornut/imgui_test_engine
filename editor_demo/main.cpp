#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "editor_assets_browser.h"
#include "editor_tests.h"
#include "editor_widgets.h"
#include "libs/Str/Str.h"
#include "shared/imgui_app.h"
#include "shared/IconsFontAwesome5.h"
#include "shared/imgui_capture_tool.h"
#ifdef EDITOR_DEMO_ENABLE_TEST_ENGINE
#include "test_engine/imgui_te_core.h"
#include "test_engine/imgui_te_coroutine.h"
#endif

// FIXME-SAMPLE FIXME-FONT: This is looking very poor with DpiScale == 1.0f, switch to FreeType?
static void LoadFonts(ImGuiApp* app)
{
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig font_cfg;
    //font_cfg.GlyphRanges = io.Fonts->GetGlyphRangesJapanese();
    //font_cfg.PixelSnapH = true;
    //font_cfg.SizePixels = 17.0f * app->DpiScale;
    //io.Fonts->AddFontFromFileTTF("../data/fonts/selawk.ttf", 16.0f, &font_cfg);
    //io.Fonts->AddFontFromFileTTF("../data/fonts/selawksb.ttf", 16.0f);

    static const ImWchar icon_fa_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig font_cfg_icons;
    font_cfg_icons.MergeMode = true;
    font_cfg_icons.GlyphMinAdvanceX = 18.0f * app->DpiScale; // Make icon font more monospace looking to simplify alignment
    font_cfg_icons.GlyphRanges = icon_fa_ranges;

    // FIXME-SAMPLE FIXME-FONT: FontAwesome5 is rasterized multiple times needlessly!

    // Font 0
    io.Fonts->AddFontFromFileTTF("../data/fonts/NotoSans-Regular.ttf", 16.0f * app->DpiScale, &font_cfg);
    io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-regular-400.otf", 13.0f * app->DpiScale, &font_cfg_icons);
    io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-solid-900.otf", 13.0f * app->DpiScale, &font_cfg_icons);

    // Font 1
    io.Fonts->AddFontFromFileTTF("../data/fonts/NotoSans-Regular.ttf", 24.0f * app->DpiScale, &font_cfg);
    io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-regular-400.otf", 19.5f * app->DpiScale, &font_cfg_icons);
    io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-solid-900.otf", 19.5f * app->DpiScale, &font_cfg_icons);

    // Font 2 (Mono)
    io.Fonts->AddFontFromFileTTF("../data/fonts/NotoSansMono-Regular.ttf", 16.0f * app->DpiScale, &font_cfg);
    io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-regular-400.otf", 13.0f * app->DpiScale, &font_cfg_icons);
    io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-solid-900.otf", 13.0f * app->DpiScale, &font_cfg_icons);

    //io.Fonts->AddFontFromFileTTF("../data/fonts/Roboto-Medium.ttf", 16.0f * app->DpiScale, &font_cfg);
    //io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-regular-400.otf", 14.0f * app->DpiScale, &font_cfg_icons);
    //io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-solid-900.otf", 14.0f * app->DpiScale, &font_cfg_icons);
}

static void DarkTheme(ImGuiStyle* style)
{
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.99f, 1.00f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 0.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.24f, 0.24f, 0.24f, 0.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.39f, 0.39f, 0.39f, 0.22f);
#ifdef IMGUI_HAS_DOCK
    colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_HeaderActive] * ImVec4(1.0f, 1.0f, 1.0f, 0.4f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
#endif
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
}

int main(int argc, char** argv)
{
    // Setup application backend
#ifdef IMGUI_APP_WIN32_DX11
    ImGuiApp* app = ImGuiApp_ImplWin32DX11_Create();
#elif IMGUI_APP_SDL_GL3
    ImGuiApp* app = ImGuiApp_ImplSdlGL3_Create();
#elif IMGUI_APP_GLFW_GL3
    ImGuiApp* app = ImGuiApp_ImplGlfwGL3_Create();
#endif

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

    // Setup application
    app->DpiAware = true;
    app->SrgbFramebuffer = false;
    app->ClearColor = ImVec4(0.120f, 0.120f, 0.120f, 1.000f);
    app->InitCreateWindow(app, "Dear ImGui: Editor Demo", ImVec2(1600, 1000));
    app->InitBackends(app);

#ifdef EDITOR_DEMO_ENABLE_TEST_ENGINE
    // Initialize test engine
    ImGuiTestEngine* engine = ImGuiTestEngine_CreateContext(ImGui::GetCurrentContext());
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(engine);
    test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
    test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
    test_io.CoroutineFuncs = Coroutine_ImplStdThread_GetInterface();

    // Register tests
    RegisterTests(engine);
    ImGuiTestEngine_Start(engine);
#endif

    // Setup style, load fonts
    // FIXME-SAMPLE FIXME-DPI: Reload at runtime according to changing DPI.
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    DarkTheme(&style);
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    style.WindowRounding = 0.0f;
    style.ScaleAllSizes(app->DpiScale);
    LoadFonts(app);

#if 0
    Str64 imgui_folder;
    ImFileFindInParents("imgui/", 4, &imgui_folder);
    size_t readme_md_size = 0;
    char* readme_md = (char*)ImFileLoadToMemory(Str128f("%s/docs/README.md", imgui_folder.c_str()).c_str(), "rb", &readme_md_size, +1);
#else
    size_t readme_md_size = 0;
    char* readme_md = (char*)ImFileLoadToMemory("docs/README.md", "rb", &readme_md_size, +1);
#endif

    bool show_readme = true;
    bool show_capture_tool = false;
    bool show_assets_browser = false;
    bool show_test_engine = false;
    ImGuiCaptureTool capture_tool;
    capture_tool.Context.UserData = app;
    capture_tool.Context.ScreenCaptureFunc = [](int x, int y, int w, int h, unsigned int* pixels, void* user_data) { ImGuiApp* app = (ImGuiApp*)user_data; return app->CaptureFramebuffer(app, x, y, w, h, pixels, NULL); };

    // Main loop
    bool aborted = false;
    while (!aborted)
    {
        if (!app->NewFrame(app))
            aborted = true;
        if (app->Quit)
            aborted = true;

#ifdef EDITOR_DEMO_ENABLE_TEST_ENGINE
        if (aborted)
        {
            ImGuiTestEngine_Abort(engine);
            ImGuiTestEngine_CoroutineStopRequest(engine);
            if (!ImGuiTestEngine_IsRunningTests(engine))
                break;
        }
#endif

        ImGui::NewFrame();

#ifdef IMGUI_HAS_DOCK
        ImGui::DockSpaceOverViewport(NULL, ImGuiDockNodeFlags_PassthruCentralNode);
#endif

        ImGui::ShowDemoWindow();

        // Contents
        if (ImGui::Begin("Game", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            float scale = app->DpiScale / 4.0f;
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 size = ImFloor(ImVec2(1920, 1080) * scale);
            ImGui::InvisibleButton("##gamescreen", size);
            ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + size, IM_COL32(0, 0, 0, 255));
        }
        ImGui::End();

        // Dummy/Skeleton editor
        if (ImGui::BeginMainMenuBar())
        {
            // FIXME-SAMPLE: Actual shortcut key handling
            // FIXME-SAMPLE: Input dispatching with multiple levels of focus (e.g. selected window could want to use Ctrl+O)
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "Ctrl+N"))
                {
                }
                if (ImGui::MenuItem("Open", "Ctrl+O"))
                {
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Ctrl+W"))
                    app->Quit = true;
                ImGui::EndMenu();
            }

            // FIXME: The MenuItem alignment system could provide a left-most slot for icons? See TODO
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem(ICON_FA_UNDO " Undo", "Ctrl+Z"))
                {
                }
                if (ImGui::MenuItem(ICON_FA_REDO" Redo", "Ctrl+Y"))
                {
                }
                ImGui::Separator();
                if (ImGui::MenuItem(ICON_FA_CUT " Cut", "Ctrl+X"))
                {
                }
                if (ImGui::MenuItem(ICON_FA_COPY " Copy", "Ctrl+C"))
                {
                }
                if (ImGui::MenuItem(ICON_FA_PASTE " Paste", "Ctrl+V"))
                {
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                // Demo
                ImGui::MenuItem("Assets Browser", NULL, &show_assets_browser);
                ImGui::Separator();

                // Tools
                ImGui::MenuItem("Capture Tool", NULL, &show_capture_tool);
#ifdef EDITOR_DEMO_ENABLE_TEST_ENGINE
                ImGui::MenuItem("Test Engine", NULL, &show_test_engine);
#else
                ImGui::MenuItem("Test Engine", NULL, &show_test_engine, false);
#endif
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                ImGui::MenuItem("Readme", NULL, &show_readme);
                ImGui::MenuItem("About Editor Demo", "", false, false);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (show_readme && readme_md != NULL)
        {
            ImGui::Begin("Readme", &show_readme);
            RenderMarkdown(readme_md, readme_md + readme_md_size);
            ImGui::End();
        }

        ImGui::Begin("Editor Demo");
        ImGui::Text(ICON_FA_SEARCH " Search");
        ImGui::ColorEdit4("ClearColor", &app->ClearColor.x);
        ImGui::End();

        if (show_assets_browser)
            ShowExampleAppAssetBrowser(&show_assets_browser);

        if (show_capture_tool)
            capture_tool.ShowCaptureToolWindow(&show_capture_tool);

#ifdef EDITOR_DEMO_ENABLE_TEST_ENGINE
        if (show_test_engine)
            ImGuiTestEngine_ShowTestWindow(engine, &show_test_engine);
        app->Vsync = test_io.RenderWantMaxSpeed ? false : true;
#endif

        ImGui::Render();

        app->Render(app);

#ifdef EDITOR_DEMO_ENABLE_TEST_ENGINE
        ImGuiTestEngine_PostRender(engine);
#endif
    }

    // Shutdown
#ifdef EDITOR_DEMO_ENABLE_TEST_ENGINE
    ImGuiTestEngine_Stop(engine);
#endif
    app->ShutdownBackends(app);
    app->ShutdownCloseWindow(app);
    ImGui::DestroyContext();
#ifdef EDITOR_DEMO_ENABLE_TEST_ENGINE
    ImGuiTestEngine_ShutdownContext(engine);
#endif
    app->Destroy(app);

    return 0;
}
