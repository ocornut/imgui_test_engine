#pragma once

//------------------------------------------------------------------------
// Coroutine abstraction
//------------------------------------------------------------------------

// An arbitrary handle used internally to represent coroutines (NULL indicates no handle)
typedef void* ImGuiTestCoroutineHandle;

// A coroutine function - ctx is an arbitrary context object
typedef void                (*ImGuiTestCoroutineMainFunc)(void* ctx);

typedef ImGuiTestCoroutineHandle (*ImGuiTestEngineCoroutineCreateFunc)(ImGuiTestCoroutineMainFunc func, const char* name, void* ctx);
typedef void                (*ImGuiTestEngineCoroutineDestroyFunc)(ImGuiTestCoroutineHandle handle);
typedef bool                (*ImGuiTestEngineCoroutineRunFunc)(ImGuiTestCoroutineHandle handle);
typedef void                (*ImGuiTestEngineCoroutineYieldFunc)();

//------------------------------------------------------------------------
// Coroutine implementation using std::thread
//------------------------------------------------------------------------

#ifdef IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL

ImGuiTestCoroutineHandle    Coroutine_ImplStdThread_Create(ImGuiTestCoroutineMainFunc func, const char* name, void* ctx);
void                        Coroutine_ImplStdThread_Destroy(ImGuiTestCoroutineHandle handle);
bool                        Coroutine_ImplStdThread_Run(ImGuiTestCoroutineHandle handle);
void                        Coroutine_ImplStdThread_Yield();

#endif // #ifdef IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL
