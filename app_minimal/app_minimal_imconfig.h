
#include "imconfig.h"                                       // Import main Dear ImGui configuration file, if any
#include "../imgui_test_engine/imgui_te_imconfig.h"         // Import configuration settings required for test engine
#define IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL   // Use std::thread instead of providing our own ImGuiTestCoroutineInterface
//#define IMGUI_TEST_ENGINE_ENABLE_IMPLOT                   // Use ImPlot in the performance tool

// Enforce backend to use
#ifdef _WIN32
#define IMGUI_APP_WIN32_DX11
#endif
//#define IMGUI_APP_SDL_GL3
//#define IMGUI_APP_GLFW_GL3
