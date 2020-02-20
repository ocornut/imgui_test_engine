// Coroutine implementation using std::thread
ImGuiTestCoroutineHandle ImCoroutineCreate(ImGuiTestCoroutineFunc func, const char* name, void* ctx);
void    ImCoroutineDestroy(ImGuiTestCoroutineHandle handle);
bool    ImCoroutineRun(ImGuiTestCoroutineHandle handle);
void    ImCoroutineYield();
