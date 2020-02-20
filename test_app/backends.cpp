#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "test_app.h"
#include <Str/Str.h>

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

extern bool MainLoopEndFrame(); // FIXME

//-------------------------------------------------------------------------
// Backend: Null
//-------------------------------------------------------------------------

bool MainLoopNewFrameNull()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);

    ImU64 time = ImGetTimeInMicroseconds();
    if (g_App.LastTime == 0)
        g_App.LastTime = time;
    io.DeltaTime = (float)((double)(time - g_App.LastTime) / 1000000.0);
    if (io.DeltaTime <= 0.0f)
        io.DeltaTime = 0.000001f;
    g_App.LastTime = time;

    ImGui::NewFrame();

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
    if (!test_io.RunningTests)
        return false;

    return true;
}

void MainLoopNull()
{
    // Init
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Build();
    for (int n = 0; n < ImGuiKey_COUNT; n++)
        io.KeyMap[n] = n;

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
    test_io.ConfigLogToTTY = true;

    while (1)
    {
        if (!MainLoopNewFrameNull())
            break;
        if (!MainLoopEndFrame())
            break;
    }
}

bool CaptureScreenshotNull(int x, int y, int w, int h, unsigned int* pixels, void* user_data)
{
    IM_UNUSED(x);
    IM_UNUSED(y);
    IM_UNUSED(user_data);
    memset(pixels, 0, w * h * sizeof(unsigned int));
    return true;
}

#if !defined(IMGUI_TESTS_BACKEND_WIN32_DX11) && !defined(IMGUI_TESTS_BACKEND_SDL_GL3) && !defined(IMGUI_TESTS_BACKEND_GLFW_GL3)
void MainLoop()
{
    // No graphics backend is used. Do same thing no matter if g_App.OptGUI is enabled or disabled.
    MainLoopNull();
}

bool CaptureFramebufferScreenshot(int x, int y, int w, int h, unsigned* pixels, void* user)
{
    // Could use software renderer for off-screen captures.
    return false;
}
#endif

//------------------------------------------------------------------------
// Coroutine implementation using std::thread
// This implements a coroutine using std::thread, with a helper thread for each coroutine (with serialised execution, so threads never actually run concurrently)
//------------------------------------------------------------------------

#include <thread>
#include <mutex>
#include <condition_variable>

struct ImGuiTestCoroutineData
{
    std::thread*                Thread;                 // The thread this coroutine is using
    std::condition_variable     StateChange;            // Condition variable notified when the coroutine state changes
    std::mutex                  StateMutex;             // Mutex to protect coroutine state
    bool                        CoroutineRunning;       // Is the coroutine currently running? Lock StateMutex before access and notify StateChange on change
    bool                        CoroutineTerminated;    // Has the coroutine terminated? Lock StateMutex before access and notify StateChange on change
    Str64                       Name;                   // The name of this coroutine
};

// The coroutine executing on the current thread (if it is a coroutine thread)
static thread_local ImGuiTestCoroutineData* GThreadCoroutine = NULL;

// The main function for a coroutine thread
void CoroutineThreadMain(ImGuiTestCoroutineData* data, ImGuiTestCoroutineFunc func, void* ctx)
{
    // Set our thread name
    ImThreadSetCurrentThreadDescription(data->Name.c_str());

    // Set the thread coroutine
    GThreadCoroutine = data;

    // Wait for initial Run()
    while (1)
    {
        std::unique_lock<std::mutex> lock(data->StateMutex);
        if (data->CoroutineRunning)
        {
            break;
        }
        data->StateChange.wait(lock);
    }

    // Run user code, which will then call Yield() when it wants to yield control
    func(ctx);

    // Mark as terminated
    {
        std::lock_guard<std::mutex> lock(data->StateMutex);

        data->CoroutineTerminated = true;
        data->CoroutineRunning = false;
        data->StateChange.notify_all();
    }
}

ImGuiTestCoroutineHandle ImCoroutineCreate(ImGuiTestCoroutineFunc func, const char* name, void* ctx)
{
    ImGuiTestCoroutineData* data = new ImGuiTestCoroutineData();

    data->Name = name;
    data->CoroutineRunning = false;
    data->CoroutineTerminated = false;
    data->Thread = new std::thread(CoroutineThreadMain, data, func, ctx);

    return (ImGuiTestCoroutineHandle)data;
}

void ImCoroutineDestroy(ImGuiTestCoroutineHandle handle)
{
    ImGuiTestCoroutineData* data = (ImGuiTestCoroutineData*)handle;

    IM_ASSERT(data->CoroutineTerminated); // The coroutine needs to run to termination otherwise it may leak all sorts of things and this will deadlock    
    if (data->Thread)
    {
        data->Thread->join();

        delete data->Thread;
        data->Thread = NULL;
    }

    delete data;
    data = NULL;
}

// Run the coroutine until the next call to Yield(). Returns TRUE if the coroutine yielded, FALSE if it terminated (or had previously terminated)
bool ImCoroutineRun(ImGuiTestCoroutineHandle handle)
{
    ImGuiTestCoroutineData* data = (ImGuiTestCoroutineData*)handle;

    // Wake up coroutine thread
    {
        std::lock_guard<std::mutex> lock(data->StateMutex);

        if (data->CoroutineTerminated)
            return false; // Coroutine has already finished

        data->CoroutineRunning = true;
        data->StateChange.notify_all();
    }

    // Wait for coroutine to stop
    while (1)
    {
        std::unique_lock<std::mutex> lock(data->StateMutex);
        if (!data->CoroutineRunning)
        {
            // Breakpoint here to catch the point where we return from the coroutine
            if (data->CoroutineTerminated)
                return false; // Coroutine finished
            break;
        }
        data->StateChange.wait(lock);
    }

    return true;
}

// Yield the current coroutine (can only be called from a coroutine)
void ImCoroutineYield()
{
    IM_ASSERT(GThreadCoroutine); // This can only be called from a coroutine thread

    ImGuiTestCoroutineData* data = GThreadCoroutine;

    // Flag that we are not running any more
    {
        std::lock_guard<std::mutex> lock(data->StateMutex);
        data->CoroutineRunning = false;
        data->StateChange.notify_all();
    }

    // At this point the thread that called RunCoroutine() will leave the "Wait for coroutine to stop" loop
    // Wait until we get started up again
    while (1)
    {
        std::unique_lock<std::mutex> lock(data->StateMutex);
        if (data->CoroutineRunning)
        {
            break; // Breakpoint here if you want to catch the point where execution of this coroutine resumes
        }
        data->StateChange.wait(lock);
    }
}

#if defined(IMGUI_TESTS_BACKEND_GLFW_GL3)

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/gl3w.h>
#include "imgui_te_core.h"

static bool MainLoopNewFrameGLFWGL3(GLFWwindow* window)
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();
    if (glfwWindowShouldClose(window))
    {
        g_App.Quit = true;
        ImGuiTestEngine_Abort(g_App.TestEngine);
    }
    // Start the Dear ImGui frame
    if (!g_App.Quit)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
    return !g_App.Quit;
}

static void MainLoopEndFrameGLFWGL3(GLFWwindow* window)
{
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);

    // Super fast mode doesn't render/present
    //if (test_io.RunningTests && test_io.ConfigRunFast)
    //    return;

    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
#ifdef IMGUI_HAS_VIEWPORT
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
#endif
    glfwSwapInterval(((test_io.RunningTests && test_io.ConfigRunFast) || test_io.ConfigNoThrottle) ? 0 : 1); // Enable vsync
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(g_App.ClearColor.x, g_App.ClearColor.y, g_App.ClearColor.z, g_App.ClearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers((GLFWwindow*)window);
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void MainLoop()
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return;

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return;
    glfwMakeContextCurrent(window);

    // Initialize OpenGL loader
    bool err = gl3wInit() != 0;
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return;
    }

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Init
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Build();

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(g_App.TestEngine);
    test_io.UserData = window;
    test_io.ConfigLogToTTY = true;

    while (1)
    {
        if (!MainLoopNewFrameGLFWGL3(window))
            break;

        if (!MainLoopEndFrame())
            return false;

        MainLoopEndFrameGLFWGL3();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();
}

#endif // IMGUI_TESTS_BACKEND_GLFW_GL3 

#if defined(IMGUI_TESTS_BACKEND_SDL_GL3) || defined(IMGUI_TESTS_BACKEND_GLFW_GL3)

bool CaptureFramebufferScreenshot(int x, int y, int w, int h, unsigned* pixels, void* user)
{
    int y2 = (int)ImGui::GetIO().DisplaySize.y - (y + h);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y2, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Flip vertically
    int comp = 4;
    int stride = w * comp;
    unsigned char* line_tmp = new unsigned char[stride];
    unsigned char* line_a = (unsigned char*)pixels;
    unsigned char* line_b = (unsigned char*)pixels + (stride * (h - 1));
    while (line_a < line_b)
    {
        memcpy(line_tmp, line_a, stride);
        memcpy(line_a, line_b, stride);
        memcpy(line_b, line_tmp, stride);
        line_a += stride;
        line_b -= stride;
    }
    delete[] line_tmp;
    return true;
}

#endif // IMGUI_TESTS_BACKEND_SDL_GL3 || IMGUI_TESTS_BACKEND_GLFW_GL3
