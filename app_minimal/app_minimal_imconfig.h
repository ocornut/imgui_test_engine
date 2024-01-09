
// Enable graphics backends for imgui_app.h/cpp (otherwise this will compile as a command-line app)
#ifdef _WIN32
#define IMGUI_APP_WIN32_DX11
#endif

// Disable legacy features / enforce using latest
#ifndef IMGUI_HAS_IMSTR
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#endif

// Enable coroutine implementation using std::thread
// In your own application you may want to implement them using your own facilities (own thread or coroutine)
#define IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL 1

// Bind Main Assert macro to we can easily recover/skip over an assert
#ifndef IM_ASSERT
#define IM_ASSERT(_EXPR)                IM_TEST_ENGINE_ASSERT(_EXPR)
// V_ASSERT_CONTRACT, assertMacro:IM_ASSERT
#endif

// Remaining template
// Use relative path as this file may be compiled with different settings
// Include base config
#include "../imgui_test_engine/imgui_te_imconfig.h"
