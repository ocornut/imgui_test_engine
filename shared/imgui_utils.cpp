#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui_utils.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "../libs/Str/Str.h"

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
// OS helpers
//-----------------------------------------------------------------------------
// - ImOsCreateProcess()
// - ImOsOpenInShell()
// - ImOsConsoleSetTextColor()
// - ImOsIsDebuggerPresent()
//-----------------------------------------------------------------------------

bool    ImOsCreateProcess(const char* cmd_line)
{
#ifdef _WIN32
    STARTUPINFOA siStartInfo;
    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
    char* cmd_line_copy = ImStrdup(cmd_line);
    BOOL ret = CreateProcessA(NULL, cmd_line_copy, NULL, NULL, FALSE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    free(cmd_line_copy);
    CloseHandle(siStartInfo.hStdInput);
    CloseHandle(siStartInfo.hStdOutput);
    CloseHandle(siStartInfo.hStdError);
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);
    return ret != 0;
#else
    IM_UNUSED(cmd_line);
    return false;
#endif
}

void    ImOsOpenInShell(const char* path)
{
#ifdef _WIN32
    ::ShellExecuteA(NULL, "open", path, NULL, NULL, SW_SHOWDEFAULT);
#else
    char command[1024];
#if __APPLE__
    const char* open_executable = "open";
#else
    const char* open_executable = "xdg-open";
#endif
    if (ImFormatString(command, IM_ARRAYSIZE(command), "%s \"%s\"", open_executable, path) < IM_ARRAYSIZE(command))
        system(command);
#endif
}

void    ImOsConsoleSetTextColor(ImOsConsoleStream stream, ImOsConsoleTextColor color)
{
#ifdef _WIN32
    HANDLE hConsole = 0;
    switch (stream)
    {
    case ImOsConsoleStream_StandardOutput: hConsole = ::GetStdHandle(STD_OUTPUT_HANDLE); break;
    case ImOsConsoleStream_StandardError:  hConsole = ::GetStdHandle(STD_ERROR_HANDLE);  break;
    }
    WORD wAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    switch (color)
    {
    case ImOsConsoleTextColor_Black:        wAttributes = 0x00; break;
    case ImOsConsoleTextColor_White:        wAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
    case ImOsConsoleTextColor_BrightWhite:  wAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
    case ImOsConsoleTextColor_BrightRed:    wAttributes = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
    case ImOsConsoleTextColor_BrightGreen:  wAttributes = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
    case ImOsConsoleTextColor_BrightBlue:   wAttributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
    case ImOsConsoleTextColor_BrightYellow: wAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
    default: IM_ASSERT(0);
    }
    ::SetConsoleTextAttribute(hConsole, wAttributes);
#elif defined(__linux) || defined(__linux__) || defined(__MACH__) || defined(__MSL__)
    // FIXME: check system capabilities (with environment variable TERM)
    FILE* handle = 0;
    switch (stream)
    {
    case ImOsConsoleStream_StandardOutput: handle = stdout; break;
    case ImOsConsoleStream_StandardError:  handle = stderr; break;
    }

    const char* modifier = "";
    switch (color)
    {
    case ImOsConsoleTextColor_Black:        modifier = "\033[30m";   break;
    case ImOsConsoleTextColor_White:        modifier = "\033[0m";    break;
    case ImOsConsoleTextColor_BrightWhite:  modifier = "\033[1;37m"; break;
    case ImOsConsoleTextColor_BrightRed:    modifier = "\033[1;31m"; break;
    case ImOsConsoleTextColor_BrightGreen:  modifier = "\033[1;32m"; break;
    case ImOsConsoleTextColor_BrightBlue:   modifier = "\033[1;34m"; break;
    case ImOsConsoleTextColor_BrightYellow: modifier = "\033[1;33m"; break;
    default: IM_ASSERT(0);
    }

    fprintf(handle, "%s", modifier);
#endif
}

bool    ImOsIsDebuggerPresent()
{
#ifdef _WIN32
    return ::IsDebuggerPresent() != 0;
#else
    // FIXME
    return false;
#endif
}

void    ImOsOutputDebugString(const char* message)
{
#ifdef _WIN32
    OutputDebugStringA(message);
#endif
}

//-----------------------------------------------------------------------------
// File/Directory handling helpers
//-----------------------------------------------------------------------------
// - ImFileExist()
// - ImFileCreateDirectoryChain()
// - ImFileFindInParents()
// - ImFileLoadSourceBlurb()
//-----------------------------------------------------------------------------

bool ImFileExist(const char* filename)
{
    struct stat dir_stat;
    int ret = stat(filename, &dir_stat);
    return (ret == 0);
}

bool ImFileDelete(const char* filename)
{
#if _WIN32
    const int filename_wsize = ::MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0);
    ImVector<wchar_t> buf;
    buf.resize(filename_wsize);
    ::MultiByteToWideChar(CP_UTF8, 0, filename, -1, (wchar_t*)&buf[0], filename_wsize);
    return DeleteFileW(&buf[0]) == TRUE;
#else
    unlink(filename);
#endif
    return false;
}

#if _WIN32
static const char IM_DIR_SEPARATOR = '\\';
#else
static const char IM_DIR_SEPARATOR = '/';
#endif

// Create directories for specified path. Slashes will be replaced with platform directory separators.
// e.g. ImFileCreateDirectoryChain("aaaa/bbbb/cccc.png")
// will try to create "aaaa/" then "aaaa/bbbb/".
bool ImFileCreateDirectoryChain(const char* path, const char* path_end)
{
    IM_ASSERT(path != NULL);
    IM_ASSERT(path[0] != 0);

    if (path_end == NULL)
        path_end = path + strlen(path);

    // Copy in a local, zero-terminated buffer
    size_t path_len = path_end - path;
    char* path_local = (char*)IM_ALLOC(path_len + 1);
    memcpy(path_local, path, path_len);
    path_local[path_len] = 0;

#if defined(_WIN32)
    ImVector<ImWchar> buf;
#endif
    // Modification of passed file_name allows us to avoid extra temporary memory allocation.
    // strtok() pokes \0 into places where slashes are, we create a directory using directory_name and restore slash.
    for (char* token = strtok(path_local, "\\/"); token != NULL; token = strtok(NULL, "\\/"))
    {
        // strtok() replaces slashes with NULLs. Overwrite removed slashes here with the type of slashes the OS needs (win32 functions need backslashes).
        if (token != path_local)
            *(token - 1) = IM_DIR_SEPARATOR;

#if defined(_WIN32)
        // Use ::CreateDirectoryW() because ::CreateDirectoryA() treat filenames in the local code-page instead of UTF-8.
        const int filename_wsize = ImTextCountCharsFromUtf8(path_local, NULL) + 1;
        buf.resize(filename_wsize);
        ImTextStrFromUtf8(&buf[0], filename_wsize, path_local, NULL);
        if (!::CreateDirectoryW((wchar_t*)&buf[0], NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
#else
        if (mkdir(path_local, S_IRWXU) != 0 && errno != EEXIST)
#endif
        {
            IM_FREE(path_local);
            return false;
        }
    }
    IM_FREE(path_local);
    return true;
}

bool ImFileFindInParents(const char* sub_path, int max_parent_count, Str* output)
{
    IM_ASSERT(sub_path != NULL);
    IM_ASSERT(output != NULL);
    for (int parent_level = 0; parent_level < max_parent_count; parent_level++)
    {
        output->clear();
        for (int j = 0; j < parent_level; j++)
            output->append("../");
        output->append(sub_path);
        if (ImFileExist(output->c_str()))
            return true;
    }
    output->clear();
    return false;
}

bool ImFileLoadSourceBlurb(const char* file_name, int line_no_start, int line_no_end, ImGuiTextBuffer* out_buf)
{
    size_t file_size = 0;
    char* file_begin = (char*)ImFileLoadToMemory(file_name, "rb", &file_size, 1);
    if (file_begin == NULL)
        return false;

    char* file_end = file_begin + file_size;
    int line_no = 0;
    const char* test_src_begin = NULL;
    const char* test_src_end = NULL;
    for (const char* p = file_begin; p < file_end; )
    {
        line_no++;
        const char* line_begin = p;
        const char* line_end = ImStrchrRange(line_begin + 1, file_end, '\n');
        if (line_end == NULL)
            line_end = file_end;
        if (line_no >= line_no_start && line_no <= line_no_end)
        {
            if (test_src_begin == NULL)
                test_src_begin = line_begin;
            test_src_end = ImMax(test_src_end, line_end);
        }
        p = line_end + 1;
    }

    if (test_src_begin != NULL)
        out_buf->append(test_src_begin, test_src_end);
    else
        out_buf->clear();

    ImGui::MemFree(file_begin);
    return true;
}

//-----------------------------------------------------------------------------
// Path handling helpers (strictly string manipulation!)
//-----------------------------------------------------------------------------
// - ImPathFindFilename()
// - ImPathFindFileExt()
// - ImPathFixSeparatorsForCurrentOS()
//-----------------------------------------------------------------------------

const char* ImPathFindFilename(const char* path, const char* path_end)
{
    if (!path_end)
        path_end = path + strlen(path);
    const char* p = path_end;
    while (p > path)
    {
        if (p[-1] == '/' || p[-1] == '\\')
            break;
        p--;
    }
    return p;
}

// "folder/filename" -> return pointer to "" (end of string)
// "folder/filename.png" -> return pointer to ".png"
// "folder/filename.png.bak" -> return pointer to ".png.bak"
const char* ImPathFindExtension(const char* path, const char* path_end)
{
    if (!path_end)
        path_end = path + strlen(path);
    const char* filename = ImPathFindFilename(path, path_end);
    const char* p = filename;
    while (p < path_end)
    {
        if (p[0] == '.')
            break;
        p++;
    }
    return p;
}

void ImPathFixSeparatorsForCurrentOS(char* buf)
{
#ifdef _WIN32
    for (char* p = buf; *p != 0; p++)
        if (*p == '/')
            *p = '\\';
#else
    for (char* p = buf; *p != 0; p++)
        if (*p == '\\')
            *p = '/';
#endif
}

//-----------------------------------------------------------------------------
// Time helpers
//-----------------------------------------------------------------------------
// - ImTimeGetInMicroseconds()
//-----------------------------------------------------------------------------

uint64_t ImTimeGetInMicroseconds()
{
    // Trying std::chrono out of unfettered optimism that it may actually work..
    using namespace std;
    chrono::microseconds ms = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch());
    return ms.count();
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
// - ImParseSplitCommandLine()
// - ImFindIniSection()
//-----------------------------------------------------------------------------

void    ImParseSplitCommandLine(int* out_argc, char const*** out_argv, const char* cmd_line)
{
    size_t cmd_line_len = strlen(cmd_line);

    int n = 1;
    {
        const char* p = cmd_line;
        while (*p != 0)
        {
            const char* arg = p;
            while (*arg == ' ')
                arg++;
            const char* arg_end = strchr(arg, ' ');
            if (arg_end == NULL)
                p = arg_end = cmd_line + cmd_line_len;
            else
                p = arg_end + 1;
            n++;
        }
    }

    int argc = n;
    char const** argv = (char const**)malloc(sizeof(char*) * (argc + 1) + (cmd_line_len + 1));
    char* cmd_line_dup = (char*)argv + sizeof(char*) * (argc + 1);
    strcpy(cmd_line_dup, cmd_line);

    {
        argv[0] = "main.exe";
        argv[argc] = NULL;

        char* p = cmd_line_dup;
        for (n = 1; n < argc; n++)
        {
            char* arg = p;
            char* arg_end = strchr(arg, ' ');
            if (arg_end == NULL)
                p = arg_end = cmd_line_dup + cmd_line_len;
            else
                p = arg_end + 1;
            argv[n] = arg;
            arg_end[0] = 0;
        }
    }

    *out_argc = argc;
    *out_argv = argv;
}

bool ImFindIniSection(const char* ini_config, const char* header, ImVector<char>* result)
{
    IM_ASSERT(ini_config != NULL);
    IM_ASSERT(header != NULL);
    IM_ASSERT(result != NULL);

    size_t ini_len = strlen(ini_config);
    size_t header_len = strlen(header);

    IM_ASSERT(header_len > 0);

    if (ini_len == 0)
        return false;

    const char* section_start = strstr(ini_config, header);
    if (section_start == NULL)
        return false;

    const char* section_end = strstr(section_start + header_len, "\n[");
    if (section_end == NULL)
        section_end = section_start + ini_len;

    // "\n[" matches next header start on all platforms, but it cuts new line marker in half on windows.
    if (*(section_end - 1) == '\r')
        --section_end;

    size_t section_len = section_end - section_start;
    result->resize(section_len + 1);
    ImStrncpy(result->Data, section_start, section_len);

    return true;
}

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
    sscanf(in_date, "%s %d %d", month_str, &day, &year);
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
