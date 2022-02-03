// #include this file in your imconfig.h to enable test engine.

// Compile Dear ImGui with test engine hooks
#define IMGUI_ENABLE_TEST_ENGINE

// [Optional] Enable plotting of perflog data for comparing performance of different runs.
// This feature requires ImPlot to be linked in the application.
//#define IMGUI_TEST_ENGINE_ENABLE_IMPLOT

// [Optional] Disable screen capture and PNG/GIF saving functionalities
// There's not much point to disable this but we provide it to reassure user that the dependencies on stb_image_write.h and gif.h are technically optional.
//#define IMGUI_TEST_ENGINE_DISABLE_CAPTURE

// Automatically fill ImGuiTestEngineIO::CoroutineFuncs with a default implementation using std::thread
// #define IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL

// Define our own IM_DEBUG_BREAK
// This allows us to define a macro below that will let us break directly in the right call-stack (instead of a function)
// (ignore the similar one in imgui_internal.h. if the one in imgui_internal.h were to be defined at the top of imgui.h we would use it)
#if defined (_MSC_VER)
#define IM_DEBUG_BREAK()    __debugbreak()
#elif defined(__clang__)
#define IM_DEBUG_BREAK()    __builtin_debugtrap()
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define IM_DEBUG_BREAK()    __asm__ volatile("int $0x03")
#elif defined(__GNUC__) && defined(__thumb__)
#define IM_DEBUG_BREAK()    __asm__ volatile(".inst 0xde01")
#elif defined(__GNUC__) && defined(__arm__) && !defined(__thumb__)
#define IM_DEBUG_BREAK()    __asm__ volatile(".inst 0xe7f001f0");
#else
#define IM_DEBUG_BREAK()    IM_ASSERT(0)    // It is expected that you define IM_DEBUG_BREAK() into something that will break nicely in a debugger!
#endif

extern void ImGuiTestEngine_Assert(const char* expr, const char* file, const char* func, int line);

// Bind our main assert macro
// FIXME: Make it possible to combine a user-defined IM_ASSERT() macros with what we need for test engine.
// Maybe we don't redefine this if IM_ASSERT() is already defined, and require user to call IMGUI_TEST_ENGINE_ASSERT() ?
#define IM_ASSERT(_EXPR)    do { if (!(_EXPR)) { ImGuiTestEngine_Assert(#_EXPR, __FILE__, __func__, __LINE__); IM_DEBUG_BREAK(); } } while (0)
// V_ASSERT_CONTRACT, assertMacro:IM_ASSERT
