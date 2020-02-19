//-----------------------------------------------------------------------------
// Simple Dear ImGui App Framework (using standard bindings)
//-----------------------------------------------------------------------------

#pragma once

#ifdef _WIN32
#define IMGUI_APP_WIN32_DX11
#endif

#include "imgui.h"

struct ImVec2;

struct ImGuiApp
{
    bool            DpiAware = true;
    bool            Quit = false;
    bool            Vsync = true;
    float           DpiScale = 1.0f;
    ImVec4          ClearColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

    bool            (*InitCreateWindow)(ImGuiApp* app, const char* window_title, ImVec2 window_size) = NULL;
    void            (*InitBackends)(ImGuiApp* app) = NULL;
    bool            (*NewFrame)(ImGuiApp* app) = NULL;
    void            (*Render)(ImGuiApp* app) = NULL;
    void            (*ShutdownCloseWindow)(ImGuiApp* app) = NULL;
    void            (*ShutdownBackends)(ImGuiApp* app) = NULL;
    void            (*Destroy)(ImGuiApp* app) = NULL;
    bool            (*CaptureFramebuffer)(ImGuiApp* app, int x, int y, int w, int h, unsigned int* pixels_rgba, void* user_data) = NULL;
       
};

ImGuiApp*     ImGuiApp_ImplNull_Create();

#ifdef IMGUI_APP_WIN32_DX11
ImGuiApp*     ImGuiApp_ImplWin32DX11_Create();
#endif
