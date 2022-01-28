#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui_te_utils.h"
#include "imgui.h"
#include "imgui_internal.h"
#define STR_IMPLEMENTATION
#include "thirdparty/Str/Str.h"

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
#include <chrono>       // FIXME
#include <thread>       // FIXME

//-----------------------------------------------------------------------------
// File/Directory Helpers
//-----------------------------------------------------------------------------
// - ImFileExist()
// - ImFileCreateDirectoryChain()
// - ImFileFindInParents()
// - ImFileLoadSourceBlurb()
//-----------------------------------------------------------------------------

#if _WIN32
static const char IM_DIR_SEPARATOR = '\\';
#else
static const char IM_DIR_SEPARATOR = '/';
#endif

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
    size_t path_len = (size_t)(path_end - path);
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
// Operating System Helpers
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
    BOOL ret = ::CreateProcessA(NULL, cmd_line_copy, NULL, NULL, FALSE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    free(cmd_line_copy);
    ::CloseHandle(siStartInfo.hStdInput);
    ::CloseHandle(siStartInfo.hStdOutput);
    ::CloseHandle(siStartInfo.hStdError);
    ::CloseHandle(piProcInfo.hProcess);
    ::CloseHandle(piProcInfo.hThread);
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
#elif defined(__linux__)
    int debugger_pid = 0;
    char buf[2048];                                 // TracerPid is located near the start of the file. If end of the buffer gets cut off thats fine.
    FILE* fp = fopen("/proc/self/status", "rb");    // Can not use ImFileLoadToMemory because size detection of /proc/self/status would fail.
    if (fp == NULL)
        return false;
    fread(buf, 1, IM_ARRAYSIZE(buf), fp);
    fclose(fp);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    if (char* tracer_pid = strstr(buf, "TracerPid:"))
    {
        tracer_pid += 10;   // Skip label
        while (isspace(*tracer_pid))
            tracer_pid++;
        debugger_pid = atoi(tracer_pid);
    }
    return debugger_pid != 0;
#else
    // FIXME
    return false;
#endif
}

void    ImOsOutputDebugString(const char* message)
{
#ifdef _WIN32
    OutputDebugStringA(message);
#else
    IM_UNUSED(message);
#endif
}

//-----------------------------------------------------------------------------
// Miscellaneous functions
//-----------------------------------------------------------------------------

// Hash "hello/world" as if it was "helloworld"
// To hash a forward slash we need to use "hello\\/world"
//   IM_ASSERT(ImHashDecoratedPath("Hello/world")   == ImHash("Helloworld", 0));
//   IM_ASSERT(ImHashDecoratedPath("Hello\\/world") == ImHash("Hello/world", 0));
// Adapted from ImHash(). Not particularly fast!
ImGuiID ImHashDecoratedPath(const char* str, const char* str_end, ImGuiID seed)
{
    static ImU32 crc32_lut[256] = { 0 };
    if (!crc32_lut[1])
    {
        const ImU32 polynomial = 0xEDB88320;
        for (ImU32 i = 0; i < 256; i++)
        {
            ImU32 crc = i;
            for (ImU32 j = 0; j < 8; j++)
                crc = (crc >> 1) ^ (ImU32(-int(crc & 1)) & polynomial);
            crc32_lut[i] = crc;
        }
    }

    // Prefixing the string with / ignore the seed
    if (str != str_end && str[0] == '/')
        seed = 0;

    seed = ~seed;
    ImU32 crc = seed;

    // Zero-terminated string
    bool inhibit_one = false;
    const unsigned char* current = (const unsigned char*)str;
    while (true)
    {
        if (str_end != NULL && current == (const unsigned char*)str_end)
            break;

        const unsigned char c = *current++;
        if (c == 0)
            break;
        if (c == '\\' && !inhibit_one)
        {
            inhibit_one = true;
            continue;
        }

        // Forward slashes are ignored unless prefixed with a backward slash
        if (c == '/' && !inhibit_one)
        {
            inhibit_one = false;
            continue;
        }

        // Reset the hash when encountering ###
        if (c == '#' && current[0] == '#' && current[1] == '#')
            crc = seed;

        crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        inhibit_one = false;
    }
    return ~crc;
}

// FIXME: Think they are 16 combinations we may as well store them in literals?
void GetImGuiKeyModsPrefixStr(ImGuiKeyModFlags mod_flags, char* out_buf, size_t out_buf_size)
{
    if (mod_flags == 0)
    {
        out_buf[0] = 0;
        return;
    }
    ImFormatString(out_buf, out_buf_size, "%s%s%s%s",
        (mod_flags & ImGuiKeyModFlags_Ctrl) ? "Ctrl+" : "",
        (mod_flags & ImGuiKeyModFlags_Alt) ? "Alt+" : "",
        (mod_flags & ImGuiKeyModFlags_Shift) ? "Shift+" : "",
        (mod_flags & ImGuiKeyModFlags_Super) ? "Super+" : "");
}

// FIXME-TESTS: Should eventually remove.
ImFont* FindFontByName(const char* name)
{
    ImGuiContext& g = *GImGui;
    for (ImFont* font : g.IO.Fonts->Fonts)
        if (strcmp(font->ConfigData->Name, name) == 0)
            return font;
    return NULL;
}

static const char* ImStrStr(const char* haystack, size_t hlen, const char* needle, int nlen)
{
    const char* end = haystack + hlen;
    const char* p = haystack;
    while ((p = (const char*)memchr(p, *needle, end - p)) != NULL)
    {
        if (end - p < nlen)
            return NULL;
        if (memcmp(p, needle, nlen) == 0)
            return p;
        p++;
    }
    return NULL;
}

void ImStrReplace(Str* s, const char* find, const char* repl)
{
    IM_ASSERT(find != NULL && *find);
    IM_ASSERT(repl != NULL);
    int find_len = (int)strlen(find);
    int repl_len = (int)strlen(repl);
    int repl_diff = repl_len - find_len;

    // Estimate required length of new buffer if string size increases.
    int need_capacity = s->capacity();
    int num_matches = INT_MAX;
    if (repl_diff > 0)
    {
        num_matches = 0;
        need_capacity = s->length() + 1;
        for (char* p = s->c_str(), *end = s->c_str() + s->length(); p != NULL && p < end;)
        {
            p = (char*)ImStrStr(p, end - p, find, find_len);
            if (p)
            {
                need_capacity += repl_diff;
                p += find_len;
                num_matches++;
            }
        }
    }

    if (num_matches == 0)
        return;

    const char* not_owned_data = s->owned() ? NULL : s->c_str();
    if (!s->owned() || need_capacity > s->capacity())
        s->reserve(need_capacity);
    if (not_owned_data != NULL)
        s->set(not_owned_data);

    // Replace data.
    for (char* p = s->c_str(), *end = s->c_str() + s->length(); p != NULL && p < end && num_matches--;)
    {
        p = (char*)ImStrStr(p, end - p, find, find_len);
        if (p)
        {
            memmove(p + repl_len, p + find_len, end - p - find_len + 1);
            memcpy(p, repl, repl_len);
            p += repl_len;
            end += repl_diff;
        }
    }
}

void ImStrXmlEscape(Str* s)
{
    ImStrReplace(s, "<", "&lt;");
    ImStrReplace(s, ">", "&gt;");
    ImStrReplace(s, "\"", "&quot;");
}

// Based on code from https://github.com/EddieBreeg/C_b64 by @EddieBreeg.
int ImBase64Encode(const unsigned char* src, char* dst, int length)
{
    static const char* b64Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    int i, j, k, l, encoded_len = 0;

    while (length > 0)
    {
        switch (length)
        {
        case 1:
            i = src[0] >> 2;
            j = (src[0] & 3) << 4;
            k = 64;
            l = 64;
            break;
        case 2:
            i = src[0] >> 2;
            j = ((src[0] & 3) << 4) | (src[1] >> 4);
            k = (src[1] & 15) << 2;
            l = 64;
            break;
        default:
            i = src[0] >> 2;
            j = ((src[0] & 3) << 4) | (src[1] >> 4);
            k = ((src[1] & 0xf) << 2) | (src[2] >> 6 & 3);
            l = src[2] & 0x3f;
            break;
        }
        dst[0] = b64Table[i];
        dst[1] = b64Table[j];
        dst[2] = b64Table[k];
        dst[3] = b64Table[l];
        src += 3;
        dst += 4;
        length -= 3;
        encoded_len += 4;
    }
    return encoded_len;
}

//-----------------------------------------------------------------------------
// STR + InputText bindings
//-----------------------------------------------------------------------------

struct InputTextCallbackStr_UserData
{
    Str*                    StrObj;
    ImGuiInputTextCallback  ChainCallback;
    void*                   ChainCallbackUserData;
};

static int InputTextCallbackStr(ImGuiInputTextCallbackData* data)
{
    InputTextCallbackStr_UserData* user_data = (InputTextCallbackStr_UserData*)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
        Str* str = user_data->StrObj;
        IM_ASSERT(data->Buf == str->c_str());
        str->reserve(data->BufTextLen + 1);
        data->Buf = (char*)str->c_str();
    }
    else if (user_data->ChainCallback)
    {
        // Forward to user callback, if any
        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback(data);
    }
    return 0;
}

bool ImGui::InputText(const char* label, Str* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallbackStr_UserData cb_user_data;
    cb_user_data.StrObj = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputText(label, (char*)str->c_str(), (size_t)str->capacity() + 1, flags, InputTextCallbackStr, &cb_user_data);
}

bool ImGui::InputTextWithHint(const char* label, const char* hint, Str* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallbackStr_UserData cb_user_data;
    cb_user_data.StrObj = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputTextWithHint(label, hint, (char*)str->c_str(), (size_t)str->capacity() + 1, flags, InputTextCallbackStr, &cb_user_data);
}

bool ImGui::InputTextMultiline(const char* label, Str* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallbackStr_UserData cb_user_data;
    cb_user_data.StrObj = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputTextMultiline(label, (char*)str->c_str(), (size_t)str->capacity() + 1, size, flags, InputTextCallbackStr, &cb_user_data);
}

// anchor parameter indicates which split would retain it's constant size.
// anchor = 0 - both splits resize when parent container size changes. Both value_1 and value_2 should be persistent.
// anchor = -1 - top/left split would have a constant size. bottom/right split would resize when parent container size changes. value_1 should be persistent, value_2 will always be recalculated from value_1.
// anchor = +1 - bottom/right split would have a constant size. top/left split would resize when parent container size changes. value_2 should be persistent, value_1 will always be recalculated from value_2.
bool ImGui::Splitter(const char* id, float* value_1, float* value_2, int axis, int anchor, float min_size_0, float min_size_1)
{
    // FIXME-DOGFOODING: This needs further refining.
    // FIXME-SCROLL: When resizing either we'd like to keep scroll focus on something (e.g. last clicked item for list, bottom for log)
    // See https://github.com/ocornut/imgui/issues/319
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (min_size_0 < 0)
        min_size_0 = ImGui::GetFrameHeight();
    if (min_size_1)
        min_size_1 = ImGui::GetFrameHeight();

    IM_ASSERT(axis == ImGuiAxis_X || axis == ImGuiAxis_Y);

    float& v_1 = *value_1;
    float& v_2 = *value_2;
    ImRect splitter_bb;
    const float avail = axis == ImGuiAxis_X ? ImGui::GetContentRegionAvail().x - style.ItemSpacing.x : ImGui::GetContentRegionAvail().y - style.ItemSpacing.y;
    if (anchor < 0)
    {
        v_2 = ImMax(avail - v_1, min_size_1);   // First split is constant size.
    }
    else if (anchor > 0)
    {
        v_1 = ImMax(avail - v_2, min_size_0);   // Second split is constant size.
    }
    else
    {
        float r = v_1 / (v_1 + v_2);            // Both splits maintain same relative size to parent.
        v_1 = IM_ROUND(avail * r) - 1;
        v_2 = IM_ROUND(avail * (1.0f - r)) - 1;
    }
    if (axis == ImGuiAxis_X)
    {
        float x = window->DC.CursorPos.x + v_1 + IM_ROUND(style.ItemSpacing.x * 0.5f);
        splitter_bb = ImRect(x - 1, window->WorkRect.Min.y, x + 1, window->WorkRect.Max.y);
    }
    else if (axis == ImGuiAxis_Y)
    {
        float y = window->DC.CursorPos.y + v_1 + IM_ROUND(style.ItemSpacing.y * 0.5f);
        splitter_bb = ImRect(window->WorkRect.Min.x, y - 1, window->WorkRect.Max.x, y + 1);
    }
    return ImGui::SplitterBehavior(splitter_bb, ImGui::GetID(id), (ImGuiAxis)axis, &v_1, &v_2, min_size_0, min_size_1, 3.0f);
}

ImGuiID TableGetHeaderID(ImGuiTable* table, const char* column, int instance_no)
{
    IM_ASSERT(table != NULL);
    int column_n = -1;
    for (int n = 0; n < table->Columns.size() && column_n < 0; n++)
        if (strcmp(ImGui::TableGetColumnName(table, n), column) == 0)
            column_n = n;
    IM_ASSERT(column_n != -1);
    int column_id = instance_no * table->ColumnsCount + column_n;
    return ImHashData(column, strlen(column), ImHashData(&column_id, sizeof(column_id), table->ID + instance_no));
}

ImGuiID TableGetHeaderID(ImGuiTable* table, int column_n, int instance_no)
{
    IM_ASSERT(column_n >= 0 && column_n < table->ColumnsCount);
    int column_id = instance_no * table->ColumnsCount + column_n;
    const char* column_name = ImGui::TableGetColumnName(table, column_n);
    return ImHashData(column_name, strlen(column_name), ImHashData(&column_id, sizeof(column_id), table->ID + instance_no));
}

void TableDiscardInstanceAndSettings(ImGuiID table_id)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.CurrentTable == NULL);
    if (ImGuiTableSettings* settings = ImGui::TableSettingsFindByID(table_id))
        settings->ID = 0;

    if (ImGuiTable* table = ImGui::TableFindByID(table_id))
        ImGui::TableRemove(table);
    // FIXME-TABLE: We should be able to use TableResetSettings() instead of TableRemove()! Maybe less of a clean slate but would be good to check that it does the job
    //ImGui::TableResetSettings(table);
}

//-----------------------------------------------------------------------------
// Simple CSV parser
//-----------------------------------------------------------------------------

void ImGuiCSVParser::Clear()
{
    Rows = Columns = 0;
    if (_Data != NULL)
        IM_FREE(_Data);
    _Data = NULL;
    _Index.clear();
}

bool ImGuiCSVParser::Load(const char* filename)
{
    size_t len = 0;
    _Data = (char*)ImFileLoadToMemory(filename, "rb", &len, 1);
    if (_Data == NULL)
        return false;

    int columns = 1;
    if (Columns > 0)
    {
        columns = Columns;                                          // User-provided expected column count.
    }
    else
    {
        for (const char* c = _Data; *c != '\n' && *c != '\0'; c++)  // Count columns. Quoted columns with commas are not supported.
            if (*c == ',')
                columns++;
    }

    // Count rows. Extra new lines anywhere in the file are ignored.
    int max_rows = 0;
    for (const char* c = _Data, *end = c + len; c < end; c++)
        if ((*c == '\n' && c[1] != '\r' && c[1] != '\n') || *c == '\0')
            max_rows++;

    if (columns == 0 || max_rows == 0)
        return false;

    // Create index
    _Index.resize(columns * max_rows);

    int col = 0;
    char* col_data = _Data;
    for (char* c = _Data; *c != '\0'; c++)
    {
        const bool is_comma = (*c == ',');
        const bool is_eol = (*c == '\n' || *c == '\r');
        const bool is_eof = (*c == '\0');
        if (is_comma || is_eol || is_eof)
        {
            _Index[Rows * columns + col] = col_data;
            col_data = c + 1;
            if (is_comma)
            {
                col++;
            }
            else
            {
                if (col + 1 == columns)
                    Rows++;
                else
                    fprintf(stderr, "%s: Unexpected number of columns on line %d, ignoring.\n", filename, Rows + 1); // FIXME
                col = 0;
            }
            *c = 0;
            if (is_eol)
                while (c[1] == '\r' || c[1] == '\n')
                    c++;
        }
    }

    Columns = columns;
    return true;
}

//-----------------------------------------------------------------------------
