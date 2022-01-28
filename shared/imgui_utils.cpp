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


