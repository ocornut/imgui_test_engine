#include "imconfig.h"

// Enable coroutine implementation using std::thread
// In your own application you may want to implement them using your own facilities (own thread or coroutine)
#define IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL

// Use relative path as this file may be compiled with different settings
#include "../test_engine/imgui_te_imconfig.h"
