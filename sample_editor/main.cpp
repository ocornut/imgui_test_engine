#include "imgui.h"
#include "../helpers/imgui_app.h"
#include "../helpers/IconsFontAwesome5.h"

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
    font_cfg_icons.GlyphRanges = icon_fa_ranges;

    // FIXME-SAMPLE FIXME-FONT: FontAwesome5 is rasterized multiple times needlessly!
    io.Fonts->AddFontFromFileTTF("../data/fonts/NotoSans-Regular.ttf", 16.0f * app->DpiScale, &font_cfg);
    io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-regular-400.otf", 13.0f * app->DpiScale, &font_cfg_icons);
    io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-solid-900.otf", 13.0f * app->DpiScale, &font_cfg_icons);

    io.Fonts->AddFontFromFileTTF("../data/fonts/NotoSansMono-Regular.ttf", 16.0f * app->DpiScale, &font_cfg);
    io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-regular-400.otf", 13.0f * app->DpiScale, &font_cfg_icons);
    io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-solid-900.otf", 13.0f * app->DpiScale, &font_cfg_icons);

    //io.Fonts->AddFontFromFileTTF("../data/fonts/Roboto-Medium.ttf", 16.0f * app->DpiScale, &font_cfg);
    //io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-regular-400.otf", 14.0f * app->DpiScale, &font_cfg_icons);
    //io.Fonts->AddFontFromFileTTF("../data/fonts/fa5-solid-900.otf", 14.0f * app->DpiScale, &font_cfg_icons);
}

int main(int argc, char** argv)
{
#ifdef _WIN32
    ImGuiApp* app = ImGuiApp_ImplWin32DX11_Create();
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
    app->InitCreateWindow(app, "Dear ImGui: Sample Editor", ImVec2(1600, 1000));
    app->InitBackends(app);
    app->ClearColor = ImVec4(0.021f, 0.337f, 0.253f, 1.000f);

    // Setup style, load fonts
    // FIXME-SAMPLE FIXME-DPI: Reload at runtime according to changing DPI.
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    style.WindowRounding = 0.0f;
    style.ScaleAllSizes(app->DpiScale);
    LoadFonts(app);

    // Main loop
    while (app->NewFrame(app) && !app->Quit)
    {
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

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
            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About Sample Editor", ""))
                {
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::Begin("Dear ImGui Sample Editor");
        ImGui::Text(ICON_FA_SEARCH " Search");
        ImGui::ColorEdit4("ClearColor", &app->ClearColor.x);
        ImGui::End();

        ImGui::Render();
        app->Render(app);
    }

    // Shutdown
    app->ShutdownBackends(app);
    app->ShutdownCloseWindow(app);
    ImGui::DestroyContext();
    app->Destroy(app);

    return 0;
}
