#pragma once

#include "imgui.h"
#include "imgui_te_core.h"

#if defined(IMGUI_APP_WIN32_DX11) || defined(IMGUI_APP_SDL_GL3)
static const bool DEFAULT_OPT_GUI = true;
#else
static const bool DEFAULT_OPT_GUI = false;
#endif

struct ImGuiApp;

struct TestApp
{
    bool                    Quit = false;
    ImGuiApp*               AppWindow = NULL;
    ImGuiTestEngine*        TestEngine = NULL;
    ImU64                   LastTime = 0;
    ImVec4                  ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Command-line options
    bool                    OptGUI = DEFAULT_OPT_GUI;
    bool                    OptFast = true;
    ImGuiTestVerboseLevel   OptVerboseLevel = ImGuiTestVerboseLevel_COUNT; // Set in main.cpp
    ImGuiTestVerboseLevel   OptVerboseLevelOnError = ImGuiTestVerboseLevel_COUNT; // Set in main.cpp
    bool                    OptNoThrottle = false;
    bool                    OptPauseOnExit = true;
    int                     OptStressAmount = 5;
    char*                   OptFileOpener = NULL;
    ImVector<char*>         TestsToRun;
};

extern TestApp g_App;
