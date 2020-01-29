#include "imgui.h"
#include "../helpers/imgui_simple_app.h"

int main(int argc, char** argv)
{
#ifdef _WIN32
    ImGuiSimpleApp* app = ImGuiSimpleApp_ImplWin32DX11_Create();
#endif

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup application
    app->InitCreateWindow(app, "Dear ImGui: Sample Editor", ImVec2(1600, 1000));
    app->InitBackends(app);

    // Main loop
    while (app->NewFrame(app))
    {
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        ImGui::Render();
        app->Render(app);
    }

    // Shutdown
    app->ShutdownBackends(app);
    app->ShutdownCloseWindow(app);
    ImGui::DestroyContext();

#ifdef _WIN32
    ImGuiSimpleApp_ImplWin32DX11_Destroy(app);
#endif

    return 0;
}
