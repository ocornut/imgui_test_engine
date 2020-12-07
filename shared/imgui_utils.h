#pragma once

#include <stdlib.h>
#include <stdint.h> // uint64_t

class Str;
struct ImVec2;
struct ImGuiTextBuffer;
template<typename T> struct ImVector;

//-----------------------------------------------------------------------------
// Data types
//-----------------------------------------------------------------------------

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
    const char*     Type = "";
    const char*     Cpu = "";
    const char*     OS = "";
    const char*     Compiler = "";
    char            Date[32];           // "YYYY-MM-DD"
    const char*     Time = "";          //
};

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

// OS helpers
bool                ImOsCreateProcess(const char* cmd_line);
void                ImOsOpenInShell(const char* path);
void                ImOsConsoleSetTextColor(ImOsConsoleStream stream, ImOsConsoleTextColor color);
bool                ImOsIsDebuggerPresent();
void                ImOsOutputDebugString(const char* message);

// File/Directory helpers
bool                ImFileExist(const char* filename);
bool                ImFileDelete(const char* filename);
bool                ImFileCreateDirectoryChain(const char* path, const char* path_end = NULL);
bool                ImFileFindInParents(const char* sub_path, int max_parent_count, Str* output);
bool                ImFileLoadSourceBlurb(const char* filename, int line_no_start, int line_no_end, ImGuiTextBuffer* out_buf);

// String helpers
const char*         ImStrchrRangeWithEscaping(const char* str, const char* str_end, char find_c);

// Path helpers (strictly string manipulation!)
const char*         ImPathFindFilename(const char* path, const char* path_end = NULL);      // Return value always between path and path_end
const char*         ImPathFindExtension(const char* path, const char* path_end = NULL);     // Return value always between path and path_end
void                ImPathFixSeparatorsForCurrentOS(char* buf);

// Time helpers
uint64_t            ImTimeGetInMicroseconds();

// Threading helpers
void                ImThreadSleepInMilliseconds(int ms);
void                ImThreadSetCurrentThreadDescription(const char* description);

// Parsing helpers
void                ImParseSplitCommandLine(int* out_argc, char const*** out_argv, const char* cmd_line);
bool                ImParseFindIniSection(const char* ini_config, const char* header, ImVector<char>* result);

// Build Info helpers
const ImBuildInfo&  ImBuildGetCompilationInfo();
bool                ImBuildGetGitBranchName(const char* git_repo_path, Str* branch_name);

// Maths/Geometry helpers
void                ImGeomGenerateRandomConvexShape(ImVec2* points, int points_count, ImVec2 shape_center, float shape_size, unsigned int poly_seed);

//-----------------------------------------------------------------------------
