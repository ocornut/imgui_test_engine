// #include this file in your imconfig.h to enable test engine.

// Compile Dear ImGui with test engine hooks
#define IMGUI_ENABLE_TEST_ENGINE

// Bind assert macro
extern void ImGuiTestEngineHook_AssertFunc(const char* expr, const char* file, const char* func, int line);
#define IM_ASSERT(_EXPR)    do { !!(_EXPR) || (ImGuiTestEngineHook_AssertFunc(#_EXPR, __FILE__, __func__, __LINE__), true); } while (0)
// V_ASSERT_CONTRACT, assertMacro:IM_ASSERT
