#pragma once

#include "imgui.h"
#include "imgui_te_core.h"

#if defined(IMGUI_TESTS_BACKEND_WIN32_DX11) || defined(IMGUI_TESTS_BACKEND_SDL_GL3)
static const bool DEFAULT_OPT_GUI = true;
#else
static const bool DEFAULT_OPT_GUI = false;
#endif

struct TestApp
{
    bool                    Quit = false;
    ImGuiTestEngine*        TestEngine = NULL;
    bool                    OptGUI = DEFAULT_OPT_GUI;
    bool                    OptFast = true;
    ImGuiTestVerboseLevel   OptVerboseLevel = ImGuiTestVerboseLevel_COUNT; // Set in main.cpp
    bool                    OptNoThrottle = false;
    bool                    OptPauseOnExit = true;
    char*                   OptFileOpener = NULL;
    ImVector<char*>         TestsToRun;
    ImU64                   LastTime = 0;
    ImVec4                  ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};

extern TestApp g_App;
