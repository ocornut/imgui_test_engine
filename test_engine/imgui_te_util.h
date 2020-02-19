#pragma once

typedef int ImGuiKeyModFlags;       // See ImGuiKeyModFlags_

enum ImGuiKeyModFlags_
{
    ImGuiKeyModFlags_None           = 0,
    ImGuiKeyModFlags_Ctrl           = 1 << 0,
    ImGuiKeyModFlags_Alt            = 1 << 1,
    ImGuiKeyModFlags_Shift          = 1 << 2,
    ImGuiKeyModFlags_Super          = 1 << 3,
#if defined(__APPLE__)
    ImGuiKeyModFlags_Shortcut = ImGuiKeyModFlags_Super
#else
    ImGuiKeyModFlags_Shortcut = ImGuiKeyModFlags_Ctrl
#endif
};

enum ImGuiKeyState
{
    ImGuiKeyState_Unknown,
    ImGuiKeyState_Up,       // Released
    ImGuiKeyState_Down      // Pressed/held
};

enum ImOsConsoleStream
{
    ImOsConsoleStream_StandardOutput,
    ImOsConsoleStream_StandardError
};

enum ImOsConsoleTextColor
{
    ImOsConsoleTextColor_Black,
    ImOsConsoleTextColor_White,
    ImOsConsoleTextColor_BrightWhite,
    ImOsConsoleTextColor_BrightRed,
    ImOsConsoleTextColor_BrightGreen,
    ImOsConsoleTextColor_BrightBlue,
    ImOsConsoleTextColor_BrightYellow
};

struct ImBuildInfo
{
    const char* Type = "";
    const char* Cpu = "";
    const char* OS = "";
    const char* Compiler = "";
    char        Date[32];           // "YYYY-MM-DD"
    const char* Time = "";          //
};

// Helpers: miscellaneous functions
ImGuiID     ImHashDecoratedPath(const char* str, ImGuiID seed = 0);
void        ImSleepInMilliseconds(int ms);
ImU64       ImGetTimeInMicroseconds();

bool        ImOsCreateProcess(const char* cmd_line);
void        ImOsOpenInShell(const char* path);
void        ImOsConsoleSetTextColor(ImOsConsoleStream stream, ImOsConsoleTextColor color);
bool        ImOsIsDebuggerPresent();

const char* ImPathFindFilename(const char* path, const char* path_end = NULL);
void        ImPathFixSeparatorsForCurrentOS(char* buf);

void        ImParseSplitCommandLine(int* out_argc, char const*** out_argv, const char* cmd_line);
void        ImParseDateFromCompilerIntoYMD(const char* in_data, char* out_buf, size_t out_buf_size);

bool        ImFileCreateDirectoryChain(const char* path, const char* path_end = NULL);
bool        ImFileLoadSourceBlurb(const char* filename, int line_no_start, int line_no_end, ImGuiTextBuffer* out_buf);
bool        ImFileExist(const char* filename);
void        ImDebugShowInputTextState();

const char* GetImGuiKeyName(ImGuiKey key);
void        GetImGuiKeyModsPrefixStr(ImGuiKeyModFlags mod_flags, char* out_buf, size_t out_buf_size);
const ImBuildInfo&  ImGetBuildInfo();
ImFont*     FindFontByName(const char* name);

void        ImThreadSetCurrentThreadDescription(const char* description); // Set the description/name of the current thread (for debugging purposes)

// Helper: maintain/calculate moving average
template<typename TYPE>
struct ImMovingAverage
{
    ImVector<TYPE>  Samples;
    TYPE            Accum;
    int             Idx;
    int             FillAmount;

    ImMovingAverage()               { Accum = (TYPE)0; Idx = FillAmount = 0; }
    void    Init(int count)         { Samples.resize(count); memset(Samples.Data, 0, Samples.Size * sizeof(TYPE)); Accum = (TYPE)0; Idx = FillAmount = 0; }
    void    AddSample(TYPE v)       { Accum += v - Samples[Idx]; Samples[Idx] = v; if (++Idx == Samples.Size) Idx = 0; if (FillAmount < Samples.Size) FillAmount++;  }
    TYPE    GetAverage() const      { return Accum / (TYPE)FillAmount; }
    int     GetSampleCount() const  { return Samples.Size; }
    bool    IsFull() const          { return FillAmount == Samples.Size; }
};

//-----------------------------------------------------------------------------
// Misc ImGui extensions
//-----------------------------------------------------------------------------

namespace ImGui
{
void    PushDisabled();
void    PopDisabled();
}

//-----------------------------------------------------------------------------
// STR + InputText bindings (FIXME: move to Str.cpp?)
//-----------------------------------------------------------------------------

class Str;
namespace ImGui
{
bool    InputText(const char* label, Str* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
bool    InputTextMultiline(const char* label, Str* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
}
