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

// Time helpers
uint64_t            ImTimeGetInMicroseconds();
void                ImTimestampToISO8601(uint64_t timestamp, Str* out_date);

// Threading helpers
void                ImThreadSleepInMilliseconds(int ms);
void                ImThreadSetCurrentThreadDescription(const char* description);

// Build Info helpers
const ImBuildInfo&  ImBuildGetCompilationInfo();
bool                ImBuildGetGitBranchName(const char* git_repo_path, Str* branch_name);

// Maths/Geometry helpers
void                ImGeomGenerateRandomConvexShape(ImVec2* points, int points_count, ImVec2 shape_center, float shape_size, unsigned int poly_seed);

//-----------------------------------------------------------------------------
