// dear imgui
// (tests)

#pragma once

// Forward declaration
struct ImGuiTestContext;
struct ImGuiTestEngine;

// Tests
void RegisterTests(ImGuiTestEngine* e);

void RegisterTests_Window(ImGuiTestEngine* e);
void RegisterTests_Scrolling(ImGuiTestEngine* e);
void RegisterTests_Button(ImGuiTestEngine* e);
void RegisterTests_Widgets(ImGuiTestEngine* e);
void RegisterTests_Nav(ImGuiTestEngine* e);
void RegisterTests_Docking(ImGuiTestEngine* e);
void RegisterTests_Draw(ImGuiTestEngine* e);
void RegisterTests_Misc(ImGuiTestEngine* e);
void RegisterTests_Perf(ImGuiTestEngine* e);
void RegisterTests_Capture(ImGuiTestEngine* e);
