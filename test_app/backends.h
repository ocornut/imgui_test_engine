
void MainLoop();
void MainLoopNull();
bool CaptureFramebufferScreenshot(int x, int y, int w, int h, unsigned int* pixels, void* user_data);
bool CaptureScreenshotNull(int x, int y, int w, int h, unsigned int* pixels, void* user_data);

ImGuiTestCoroutineHandle CreateCoroutine(ImGuiTestCoroutineFunc func, const char* name, void* ctx);
void DestroyCoroutine(ImGuiTestCoroutineHandle handle);
bool RunCoroutine(ImGuiTestCoroutineHandle handle);
void YieldFromCoroutine();
