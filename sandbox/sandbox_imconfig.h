#include "imconfig.h"

#define IMGUI_SANDBOX
#define IMGUI_SANDBOX_ENABLE_TEST_ENGINE
#define IMGUI_SANDBOX_ENABLE_NATIVE_FILE_DIALOG

// Use relative path as this file may be compiled with different settings
#ifdef IMGUI_SANDBOX_ENABLE_TEST_ENGINE
#define IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL
#include "../test_engine/imgui_te_imconfig.h"
#endif

