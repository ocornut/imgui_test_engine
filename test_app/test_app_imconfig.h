#include "imconfig.h"

// Enable graphics backends
#ifdef _WIN32
#define IMGUI_APP_WIN32_DX11 1
#endif

#define IMGUI_TEST_ENGINE_DEBUG

// Disable tests that are known to be broken. This mainly exist as a way to grep them.
#define IMGUI_BROKEN_TESTS 0

//#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

// Enable coroutine implementation using std::thread
// In your own application you may want to implement them using your own facilities (own thread or coroutine)
#define IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL

// Use relative path as this file may be compiled with different settings
#include "../test_engine/imgui_te_imconfig.h"

// Extra warnings
#ifdef _MSC_VER
//#pragma warning (4: 4668)             // 'IMGUI_XXX' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#endif
