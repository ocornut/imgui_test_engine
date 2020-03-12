#include "imconfig.h"

#define EDITOR_DEMO_ENABLE_TEST_ENGINE
#define IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL

// Use relative path as this file may be compiled with different settings
#ifdef EDITOR_DEMO_ENABLE_TEST_ENGINE
#include "../test_engine/imgui_te_imconfig.h"
#endif

