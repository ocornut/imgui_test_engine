//-----------------------------------------------------------------------------
// Simple Dear ImGui App Framework (using standard backends)
//-----------------------------------------------------------------------------
// THIS IS FOR OUR OWN USE AND IS NOT SUPPORTED.
// IF YOU USE IT PLEASE DON'T ASK QUESTIONS. WE MAY CHANGE API AT ANY TIME.
//-----------------------------------------------------------------------------
// To use graphics backends, define one of the following in your project:
//   #define IMGUI_APP_WIN32_DX11
//   #define IMGUI_APP_SDL2_GL2
//   #define IMGUI_APP_SDL2_GL3
//   #define IMGUI_APP_GLFW_GL3
//-----------------------------------------------------------------------------
// IMPORTANT: BACKENDS IMPLEMENTATIONS ARE AUTOMATICALLY LINKED IN imgui_app.cpp
//-----------------------------------------------------------------------------

#pragma once

//#if defined(_WIN32) && !defined(IMGUI_APP_WIN32_DX11)
//#define IMGUI_APP_WIN32_DX11
//#endif

#include "imgui.h"  // for ImVec4 only

struct ImGuiApp
{
    bool    DpiAware = true;                            // [In]  InitCreateWindow()
    bool    SrgbFramebuffer = false;                    // [In]  InitCreateWindow()
    bool    Quit = false;                               // [In]  NewFrame()
    ImVec4  ClearColor = { 0.f, 0.f, 0.f, 1.f };        // [In]  Render()
    bool    MockViewports = false;                      // [In]  InitBackends()
    float   DpiScale = 1.0f;                            // [Out] InitCreateWindow() / NewFrame()
    bool    Vsync = true;                               // [Out] Render()

    bool    (*InitCreateWindow)(ImGuiApp* app, const char* window_title, ImVec2 window_size) = nullptr;
    void    (*InitBackends)(ImGuiApp* app) = nullptr;
    bool    (*NewFrame)(ImGuiApp* app) = nullptr;
    void    (*Render)(ImGuiApp* app) = nullptr;
    void    (*ShutdownCloseWindow)(ImGuiApp* app) = nullptr;
    void    (*ShutdownBackends)(ImGuiApp* app) = nullptr;
    void    (*Destroy)(ImGuiApp* app) = nullptr;
    bool    (*CaptureFramebuffer)(ImGuiApp* app, ImGuiViewport* viewport, int x, int y, int w, int h, unsigned int* pixels_rgba, void* user_data) = nullptr;
};

// Helper stub to store directly in ImGuiTestEngineIO::ScreenCaptureFunc when using test engine (prototype is same as ImGuiScreenCaptureFunc)
bool ImGuiApp_ScreenCaptureFunc(ImGuiID viewport_id, int x, int y, int w, int h, unsigned int* pixels, void* user_data);

//-----------------------------------------------------------------------------
// Create function for each Backends
// - In most case you can use the shortcut ImGuiApp_ImplCreate() which will be defined to the first available backend.
// - It is technically possible to compile with multiple backends simultaneously (very few users will care/need this).
//-----------------------------------------------------------------------------

#ifdef IMGUI_APP_WIN32_DX11
ImGuiApp* ImGuiApp_ImplWin32DX11_Create();
#ifndef ImGuiApp_ImplDefault_Create
#define ImGuiApp_ImplDefault_Create ImGuiApp_ImplWin32DX11_Create
#endif
#endif

#ifdef IMGUI_APP_SDL2_GL2
ImGuiApp* ImGuiApp_ImplSdlGL2_Create();
#ifndef ImGuiApp_ImplDefault_Create
#define ImGuiApp_ImplDefault_Create ImGuiApp_ImplSdlGL2_Create
#endif
#endif

#ifdef IMGUI_APP_SDL2_GL3
ImGuiApp* ImGuiApp_ImplSdlGL3_Create();
#ifndef ImGuiApp_ImplDefault_Create
#define ImGuiApp_ImplDefault_Create ImGuiApp_ImplSdlGL3_Create
#endif
#endif

#ifdef IMGUI_APP_GLFW_GL3
ImGuiApp* ImGuiApp_ImplGlfwGL3_Create();
#ifndef ImGuiApp_ImplDefault_Create
#define ImGuiApp_ImplDefault_Create ImGuiApp_ImplGlfwGL3_Create
#endif
#endif

// Dummy/Null Backend (last one in list so its only the default when there are no other backends compiled in)
ImGuiApp* ImGuiApp_ImplNull_Create();
#ifndef ImGuiApp_ImplDefault_Create
#define ImGuiApp_ImplDefault_Create ImGuiApp_ImplNull_Create
#endif

//-----------------------------------------------------------------------------
// Backend Implementations
//-----------------------------------------------------------------------------

#if IMGUI_APP_IMPLEMENTATION

#if defined(IMGUI_APP_WIN32_DX11)
#include "imgui_impl_win32.cpp"
#include "imgui_impl_dx11.cpp"
#endif

// Renderer before Platform backends because SDL/GLFW tend to have their own GL stuff which can conflict.
#if defined(IMGUI_APP_SDL2_GL2) || defined(IMGUI_APP_GLFW_GL2)
#include "imgui_impl_opengl2.cpp"
#endif

#if defined(IMGUI_APP_SDL2_GL3) || defined(IMGUI_APP_GLFW_GL3)
#include "imgui_impl_opengl3.cpp"
#endif

#if defined(IMGUI_APP_SDL2_GL2) || defined(IMGUI_APP_SDL2_GL3)
#include "imgui_impl_sdl2.cpp"
#endif

#if defined(IMGUI_APP_GLFW_GL3)
#include "imgui_impl_glfw.cpp"
#endif

#endif

//-----------------------------------------------------------------------------
