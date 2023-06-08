// Use relative path as this file may be compiled with different settings
#include "../imgui_test_suite/imgui_test_suite_imconfig.h"

// Bind Main Assert macro to we can easily recover/skip over an assert
#ifndef IM_ASSERT
#define IM_ASSERT(_EXPR)                IM_TEST_ENGINE_ASSERT(_EXPR)
// V_ASSERT_CONTRACT, assertMacro:IM_ASSERT
#endif
