
// FIXME: Need imgui_te_core.h to be included

// Coroutine implementation using std::thread
ImGuiTestCoroutineHandle    Coroutine_ImplStdThread_Create(ImGuiTestCoroutineMainFunc func, const char* name, void* ctx);
void                        Coroutine_ImplStdThread_Destroy(ImGuiTestCoroutineHandle handle);
bool                        Coroutine_ImplStdThread_Run(ImGuiTestCoroutineHandle handle);
void                        Coroutine_ImplStdThread_Yield();
