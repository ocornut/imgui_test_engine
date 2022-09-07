// dear imgui
// (test suite app)

#pragma once

#define TEST_SUITE_ALT_FONT_NAME   "Roboto-Medium.ttf"      // Prefix

// Tests Registration Functions
struct ImGuiTestEngine;
extern void RegisterTests_All(ImGuiTestEngine* e);          // imgui_tests_core.cpp
