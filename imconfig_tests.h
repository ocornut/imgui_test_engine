#include "imconfig.h"
//#define IMGUI_USE_FNV1A
#define IMGUI_ENABLE_TEST_ENGINE    1
#define ImDrawIdx                   unsigned int

#ifdef _WIN32
#define IMGUI_TESTS_ENABLE_DX11
#endif

// Assert handler
extern void ImGuiTestEngineHook_AssertFunc(const char* expr, const char* file, const char* function, int line);
#define IM_ASSERT(_EXPR)            do { !!(_EXPR) || (ImGuiTestEngineHook_AssertFunc(#_EXPR, __FILE__, __func__, __LINE__),true); } while (0)
