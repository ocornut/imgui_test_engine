#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "test_app.h"
#include <Str/Str.h>

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
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
