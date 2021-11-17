// #include this file in your imconfig.h to enable test engine.

// Compile Dear ImGui with test engine hooks
#define IMGUI_ENABLE_TEST_ENGINE

// [Optional] Enable plotting of perflog data for comparing performance of different runs.
// This feature requires ImPlot to be linked in the application.
//#define IMGUI_TEST_ENGINE_ENABLE_IMPLOT

// Test Engine Assert
extern void ImGuiTestEngineHook_AssertFunc(const char* expr, const char* file, const char* func, int line);
#define IMGUI_TEST_ENGINE_ASSERT(_EXPR)     ImGuiTestEngineHook_AssertFunc(#_EXPR, __FILE__, __func__, __LINE__)

// Bind main assert macro
// FIXME: Make it possible to combine a user-defined IM_ASSERT() macros with what we need for test engine.
// Maybe we don't redefine this if IM_ASSERT() is already defined, and require user to call IMGUI_TEST_ENGINE_ASSERT() ?
#define IM_ASSERT(_EXPR)    do { !!(_EXPR) || (IMGUI_TEST_ENGINE_ASSERT(_EXPR), true); } while (0)
// V_ASSERT_CONTRACT, assertMacro:IM_ASSERT
