#pragma once

#include <stdlib.h>

class Str;
struct ImVec2;
struct ImGuiTextBuffer;
template<typename T> struct ImVector;

//-----------------------------------------------------------------------------
// Data types
//-----------------------------------------------------------------------------

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

// Build Info helpers
const ImBuildInfo&  ImBuildGetCompilationInfo();
bool                ImBuildGetGitBranchName(const char* git_repo_path, Str* branch_name);


//-----------------------------------------------------------------------------
