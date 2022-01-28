#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui_utils.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "../imgui_test_engine/thirdparty/Str/Str.h"

#if defined(_WIN32)
#if !defined(_WINDOWS_)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <shellapi.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#endif
#if defined(__linux) || defined(__linux__) || defined(__MACH__) || defined(__MSL__)
#include <pthread.h>    // pthread_setname_np()
#endif
#include <chrono>
#include <thread>

//-----------------------------------------------------------------------------
// Path handling helpers (strictly string manipulation!)
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Time helpers
//-----------------------------------------------------------------------------
// - ImTimeGetInMicroseconds()
// - ImTimestampToISO8601()
//-----------------------------------------------------------------------------

uint64_t ImTimeGetInMicroseconds()
{
    // Trying std::chrono out of unfettered optimism that it may actually work..
    using namespace std;
    chrono::microseconds ms = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch());
    return (uint64_t)ms.count();
}

void ImTimestampToISO8601(uint64_t timestamp, Str* out_date)
{
    time_t unix_time = (time_t)(timestamp / 1000000); // Convert to seconds.
    tm* time = gmtime(&unix_time);
    const char* time_format = "%Y-%m-%dT%H:%M:%S";
    size_t size_req = strftime(out_date->c_str(), out_date->capacity(), time_format, time);
    if (size_req >= (size_t)out_date->capacity())
    {
        out_date->reserve((int)size_req);
        strftime(out_date->c_str(), out_date->capacity(), time_format, time);
    }
}

//-----------------------------------------------------------------------------
// Threading helpers
//-----------------------------------------------------------------------------
// - ImThreadSleepInMilliseconds()
// - ImThreadSetCurrentThreadDescription()
//-----------------------------------------------------------------------------

void ImThreadSleepInMilliseconds(int ms)
{
    using namespace std;
    this_thread::sleep_for(chrono::milliseconds(ms));
}

#if defined(_WIN32)
// Helper function for setting thread name on Win32
// This is a separate function because __try cannot coexist with local objects that need destructors called on stack unwind
static void ImThreadSetCurrentThreadDescriptionWin32OldStyle(const char* description)
{
    // Old-style Win32 thread name setting method
    // See https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code
    const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#pragma pack(pop)

    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = description;
    info.dwThreadID = (DWORD)-1;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
#pragma warning(pop)
}
#endif // #ifdef _WIN32

// Set the description (name) of the current thread for debugging purposes
void ImThreadSetCurrentThreadDescription(const char* description)
{
#if defined(_WIN32) // Windows
    // New-style thread name setting
    // Only supported from Win 10 version 1607/Server 2016 onwards, hence the need for dynamic linking

    typedef HRESULT(WINAPI* SetThreadDescriptionFunc)(HANDLE hThread, PCWSTR lpThreadDescription);

    SetThreadDescriptionFunc set_thread_description = (SetThreadDescriptionFunc)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "SetThreadDescription");

    if (set_thread_description)
    {
        ImVector<ImWchar> buf;
        const int description_wsize = ImTextCountCharsFromUtf8(description, NULL) + 1;
        buf.resize(description_wsize);
        ImTextStrFromUtf8(&buf[0], description_wsize, description, NULL);
        set_thread_description(GetCurrentThread(), (wchar_t*)&buf[0]);
    }

    // Also do the old-style method too even if the new-style one worked, as the two work in slightly different sets of circumstances
    ImThreadSetCurrentThreadDescriptionWin32OldStyle(description);
#elif defined(__linux) || defined(__linux__) // Linux
    pthread_setname_np(pthread_self(), description);
#elif defined(__MACH__) || defined(__MSL__) // OSX
    pthread_setname_np(description);
#else
    // This is a nice-to-have rather than critical functionality, so fail silently if we don't support this platform
#endif
}

//-----------------------------------------------------------------------------
// Parsing helpers
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Build info helpers
//-----------------------------------------------------------------------------
// - ImBuildGetCompilationInfo()
// - ImBuildGetGitBranchName()
//-----------------------------------------------------------------------------

// Turn __DATE__ "Jan 10 2019" into "2019-01-10"
static void ImBuildParseDateFromCompilerIntoYMD(const char* in_date, char* out_buf, size_t out_buf_size)
{
    char month_str[5];
    int year, month, day;
    sscanf(in_date, "%3s %d %d", month_str, &day, &year);
    const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char* p = strstr(month_names, month_str);
    month = p ? (int)(1 + (p - month_names) / 3) : 0;
    ImFormatString(out_buf, out_buf_size, "%04d-%02d-%02d", year, month, day);
}

// Those strings are used to output easily identifiable markers in compare logs. We only need to support what we use for testing.
// We can probably grab info in eaplatform.h/eacompiler.h etc. in EASTL
const ImBuildInfo& ImBuildGetCompilationInfo()
{
    static ImBuildInfo build_info;

    if (build_info.Type[0] == '\0')
    {
        // Build Type
#if defined(DEBUG) || defined(_DEBUG)
        build_info.Type = "Debug";
#else
        build_info.Type = "Release";
#endif

        // CPU
#if defined(_M_X86) || defined(_M_IX86) || defined(__i386) || defined(__i386__) || defined(_X86_) || defined(_M_AMD64) || defined(_AMD64_) || defined(__x86_64__)
        build_info.Cpu = (sizeof(size_t) == 4) ? "X86" : "X64";
#else
#error
        build_info.Cpu = (sizeof(size_t) == 4) ? "Unknown32" : "Unknown64";
#endif

        // Platform/OS
#if defined(_WIN32)
        build_info.OS = "Windows";
#elif defined(__linux) || defined(__linux__)
        build_info.OS = "Linux";
#elif defined(__MACH__) || defined(__MSL__)
        build_info.OS = "OSX";
#elif defined(__ORBIS__)
        build_info.OS = "PS4";
#elif defined(_DURANGO)
        build_info.OS = "XboxOne";
#else
        build_info.OS = "Unknown";
#endif

        // Compiler
#if defined(_MSC_VER)
        build_info.Compiler = "MSVC";
#elif defined(__clang__)
        build_info.Compiler = "Clang";
#elif defined(__GNUC__)
        build_info.Compiler = "GCC";
#else
        build_info.Compiler = "Unknown";
#endif

        // Date/Time
        ImBuildParseDateFromCompilerIntoYMD(__DATE__, build_info.Date, IM_ARRAYSIZE(build_info.Date));
        build_info.Time = __TIME__;
    }

    return build_info;
}

bool ImBuildGetGitBranchName(const char* git_repo_path, Str* branch_name)
{
    IM_ASSERT(git_repo_path != NULL);
    IM_ASSERT(branch_name != NULL);
    Str256f head_path("%s/.git/HEAD", git_repo_path);
    size_t head_size = 0;
    bool result = false;
    if (char* git_head = (char*)ImFileLoadToMemory(head_path.c_str(), "r", &head_size, 1))
    {
        const char prefix[] = "ref: refs/heads/";       // Branch name is prefixed with this in HEAD file.
        const int prefix_length = IM_ARRAYSIZE(prefix) - 1;
        strtok(git_head, "\r\n");                       // Trim new line
        if (head_size > prefix_length&& strncmp(git_head, prefix, prefix_length) == 0)
        {
            strcpy(branch_name->c_str(), git_head + prefix_length);
            result = true;
        }
        IM_FREE(git_head);
    }
    return result;
}

//-----------------------------------------------------------------------------

// Generate a random convex shape with 'points_count' points, writing them into 'points'.
// 'poly_seed' specifies the random seed value.
// 'shape_center' and 'shape_size' define where the shape will be located and the size (of the largest axis).
// (based on algorithm from http://cglab.ca/~sander/misc/ConvexGeneration/convex.html)
void ImGeomGenerateRandomConvexShape(ImVec2* points, int points_count, ImVec2 shape_center, float shape_size, unsigned int poly_seed)
{
    IM_ASSERT(points_count >= 3);

    srand(poly_seed);

    // Generate two lists of numbers
    float* x_points = (float*)alloca(points_count * sizeof(float));
    float* y_points = (float*)alloca(points_count * sizeof(float));
    for (int i = 0; i < points_count; i++)
    {
        x_points[i] = (float)rand() / (float)RAND_MAX;
        y_points[i] = (float)rand() / (float)RAND_MAX;
    }

    // Sort
    ImQsort(x_points, points_count, sizeof(float), [](const void* a, const void* b) { if (*(float*)a < *(float*)b) return -1; else if (*(float*)a > *(float*)b) return 1; else return 0; });
    ImQsort(y_points, points_count, sizeof(float), [](const void* a, const void* b) { if (*(float*)a < *(float*)b) return -1; else if (*(float*)a > *(float*)b) return 1; else return 0; });

    // Get the extremities
    float min_x = x_points[0];
    float max_x = x_points[points_count - 1];
    float min_y = y_points[0];
    float max_y = y_points[points_count - 1];

    // Split into pairs of chains, one for each "side" of the shape
    float* x_chain = (float*)alloca(points_count * sizeof(float));
    float* y_chain = (float*)alloca(points_count * sizeof(float));

    float x_chain_current_a = min_x;
    float x_chain_current_b = min_x;
    float y_chain_current_a = min_y;
    float y_chain_current_b = min_y;

    for (int i = 1; i < (points_count - 1); i++)
    {
        if ((rand() % 100) < 50)
        {
            x_chain[i - 1] = x_points[i] - x_chain_current_a;
            x_chain_current_a = x_points[i];
            y_chain[i - 1] = y_points[i] - y_chain_current_a;
            y_chain_current_a = y_points[i];
        }
        else
        {
            x_chain[i - 1] = x_chain_current_b - x_points[i];
            x_chain_current_b = x_points[i];
            y_chain[i - 1] = y_chain_current_b - y_points[i];
            y_chain_current_b = y_points[i];
        }
    }

    x_chain[points_count - 2] = max_x - x_chain_current_a;
    x_chain[points_count - 1] = x_chain_current_b - max_x;
    y_chain[points_count - 2] = max_y - y_chain_current_a;
    y_chain[points_count - 1] = y_chain_current_b - max_y;

    // Build shuffle list
    int* shuffle_list = (int*)alloca(points_count * sizeof(int));
    for (int i = 0; i < points_count; i++)
        shuffle_list[i] = i;

    for (int i = 0; i < points_count * 2; i++)
    {
        int index_a = rand() % points_count;
        int index_b = rand() % points_count;
        int temp = shuffle_list[index_a];
        shuffle_list[index_a] = shuffle_list[index_b];
        shuffle_list[index_b] = temp;
    }

    // Generate random vectors from the X/Y chains
    for (int i = 0; i < points_count; i++)
        points[i] = ImVec2(x_chain[i], y_chain[shuffle_list[i]]);

    // Sort by angle of vector
    ImQsort(points, points_count, sizeof(ImVec2), [](const void* a, const void* b)
        {
            float angle_a = atan2f(((ImVec2*)a)->y, ((ImVec2*)a)->x);
            float angle_b = atan2f(((ImVec2*)b)->y, ((ImVec2*)b)->x);
            if (angle_a < angle_b)
                return -1;
            else if (angle_a > angle_b)
                return 1;
            else
                return 0;
        });

    // Convert into absolute co-ordinates
    ImVec2 current_pos(0.0f, 0.0f);
    ImVec2 center_pos(0.0f, 0.0f);
    ImVec2 min_pos(FLT_MAX, FLT_MAX);
    ImVec2 max_pos(FLT_MIN, FLT_MIN);
    for (int i = 0; i < points_count; i++)
    {
        ImVec2 new_pos(current_pos.x + points[i].x, current_pos.y + points[i].y);
        points[i] = current_pos;
        center_pos = ImVec2(center_pos.x + current_pos.x, center_pos.y + current_pos.y);
        min_pos.x = ImMin(min_pos.x, current_pos.x);
        min_pos.y = ImMin(min_pos.y, current_pos.y);
        max_pos.x = ImMax(max_pos.x, current_pos.x);
        max_pos.y = ImMax(max_pos.y, current_pos.y);
        current_pos = new_pos;
    }

    // Re-scale and center
    center_pos = ImVec2(center_pos.x / (float)points_count, center_pos.y / (float)points_count);

    ImVec2 size = ImVec2(max_pos.x - min_pos.x, max_pos.y - min_pos.y);
    float scale = shape_size / ImMax(size.x, size.y);
    for (int i = 0; i < points_count; i++)
    {
        points[i].x = shape_center.x + ((points[i].x - center_pos.x) * scale);
        points[i].y = shape_center.y + ((points[i].y - center_pos.y) * scale);
    }
}
