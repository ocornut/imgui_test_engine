// dear imgui
// (tests)

#pragma once

// Forward declaration
struct ImGuiTestContext;
struct ImGuiTestEngine;

// Tests
void RegisterTests(ImGuiTestEngine* e);

void RegisterTests_Window(ImGuiTestContext* ctx);
void RegisterTests_Scrolling(ImGuiTestContext* ctx);
void RegisterTests_Button(ImGuiTestContext* ctx);
void RegisterTests_Widgets(ImGuiTestContext* ctx);
void RegisterTests_Nav(ImGuiTestContext* ctx);
void RegisterTests_Docking(ImGuiTestContext* ctx);
void RegisterTests_Misc(ImGuiTestContext* ctx);
void RegisterTests_Perf(ImGuiTestContext* ctx);
void RegisterTests_Capture(ImGuiTestContext* ctx);
