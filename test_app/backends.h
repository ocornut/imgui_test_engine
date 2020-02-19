
void    MainLoop();
void    MainLoopNull();
bool    CaptureFramebufferScreenshot(int x, int y, int w, int h, unsigned int* pixels, void* user_data);
bool    CaptureScreenshotNull(int x, int y, int w, int h, unsigned int* pixels, void* user_data);

// Coroutine implementation using std::thread
ImGuiTestCoroutineHandle ImCoroutineCreate(ImGuiTestCoroutineFunc func, const char* name, void* ctx);
void    ImCoroutineDestroy(ImGuiTestCoroutineHandle handle);
bool    ImCoroutineRun(ImGuiTestCoroutineHandle handle);
void    ImCoroutineYield();
