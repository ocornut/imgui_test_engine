#pragma once

#define IMGUI_SIMPLE_APP_WIN32_DX11

struct ImVec2;

struct ImGuiSimpleApp
{
    bool            (*InitCreateWindow)(ImGuiSimpleApp* app, const char* window_title, const ImVec2& window_size);
    void            (*InitBackends)(ImGuiSimpleApp* app);
    bool            (*NewFrame)(ImGuiSimpleApp* app);;
    void            (*Render)(ImGuiSimpleApp* app);;
    void            (*ShutdownCloseWindow)(ImGuiSimpleApp* app);;
    void            (*ShutdownBackends)(ImGuiSimpleApp* app);;
};

#ifdef IMGUI_SIMPLE_APP_WIN32_DX11
ImGuiSimpleApp*     ImGuiSimpleApp_ImplWin32DX11_Create();
void                ImGuiSimpleApp_ImplWin32DX11_Destroy(ImGuiSimpleApp* app);
#endif
