#pragma once

#ifndef IMGUI_VERSION
#include "imgui.h"
#endif

// Forward declarations
struct ImGuiTestEngine;

// Functions
IMGUI_API void    ImGuiTestEngine_ShowTestEngineWindows(ImGuiTestEngine* engine, bool* p_open);
