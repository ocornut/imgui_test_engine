//-----------------------------------------------------------------------------
// Simple Dear ImGui App Framework (using standard bindings)
//-----------------------------------------------------------------------------
// To use graphics backends, define one of the following in your project:
//   #define IMGUI_APP_WIN32_DX11
//   #define IMGUI_APP_SDL_GL2
//   #define IMGUI_APP_SDL_GL3
//   #define IMGUI_APP_GLFW_GL3
//-----------------------------------------------------------------------------

#pragma once

//#if defined(_WIN32) && !defined(IMGUI_APP_WIN32_DX11)
//#define IMGUI_APP_WIN32_DX11
//#endif

#include "imgui.h"  // for ImVec4 only

struct ImGuiApp
{
    bool    DpiAware = true;                            // [In]  InitCreateWindow()
    bool    SrgbFramebuffer = false;                    // [In]  InitCreateWindow() FIXME-WIP
    bool    Quit = false;                               // [In]  NewFrame()
    ImVec4  ClearColor = { 0.f, 0.f, 0.f, 1.f };        // [In]  Render()
    float   DpiScale = 1.0f;                            // [Out] InitCreateWindow() / NewFrame()
    bool    Vsync = true;                               // [Out] Render()

    bool    (*InitCreateWindow)(ImGuiApp* app, const char* window_title, ImVec2 window_size) = nullptr;
    void    (*InitBackends)(ImGuiApp* app) = nullptr;
    bool    (*NewFrame)(ImGuiApp* app) = nullptr;
    void    (*Render)(ImGuiApp* app) = nullptr;
    void    (*ShutdownCloseWindow)(ImGuiApp* app) = nullptr;
    void    (*ShutdownBackends)(ImGuiApp* app) = nullptr;
    void    (*Destroy)(ImGuiApp* app) = nullptr;
    bool    (*CaptureFramebuffer)(ImGuiApp* app, int x, int y, int w, int h, unsigned int* pixels_rgba, void* user_data) = nullptr;
};

ImGuiApp*     ImGuiApp_ImplNull_Create();

#ifdef IMGUI_APP_WIN32_DX11
ImGuiApp*     ImGuiApp_ImplWin32DX11_Create();
#endif

#ifdef IMGUI_APP_SDL_GL2
ImGuiApp*     ImGuiApp_ImplSdlGL2_Create();
#endif

#ifdef IMGUI_APP_SDL_GL3
ImGuiApp*     ImGuiApp_ImplSdlGL3_Create();
#endif

#ifdef IMGUI_APP_GLFW_GL3
ImGuiApp*     ImGuiApp_ImplGlfwGL3_Create();
#endif
