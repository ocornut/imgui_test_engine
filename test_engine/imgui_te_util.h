#pragma once

#include <math.h>   // fabsf
#include "imgui.h"  // ImGuiID, ImGuiKey, ImFont

// FIXME: Setting IO.ConfigMacOSXBehaviors to non-default value breaks this assumption.
#if defined(__APPLE__)
#define ImGuiKeyModFlags_Shortcut   ImGuiKeyModFlags_Super
#else
#define ImGuiKeyModFlags_Shortcut   ImGuiKeyModFlags_Ctrl
#endif

enum ImGuiKeyState
{
    ImGuiKeyState_Unknown,
    ImGuiKeyState_Up,       // Released
    ImGuiKeyState_Down      // Pressed/held
};

#ifdef IMGUI_HAS_TABLE
struct ImGuiTable;
#endif

// Maths helpers
static inline bool  ImFloatEq(float f1, float f2) { float d = f2 - f1; return fabsf(d) <= 1.192092896e-07F; }

// Miscellaneous functions
ImGuiID             ImHashDecoratedPath(const char* str, const char* str_end = NULL, ImGuiID seed = 0);
const char*         GetImGuiKeyName(ImGuiKey key);
void                GetImGuiKeyModsPrefixStr(ImGuiKeyModFlags mod_flags, char* out_buf, size_t out_buf_size);
ImFont*             FindFontByName(const char* name);

#ifdef IMGUI_HAS_TABLE
ImGuiID             TableGetHeaderID(ImGuiTable* table, const char* column, int instance_no = 0);
ImGuiID             TableGetHeaderID(ImGuiTable* table, int column_n, int instance_no = 0);
void                TableDiscardInstanceAndSettings(ImGuiID table_id);
#endif

// Helper: maintain/calculate moving average
template<typename TYPE>
struct ImMovingAverage
{
    ImVector<TYPE>  Samples;
    TYPE            Accum;
    int             Idx;
    int             FillAmount;

    ImMovingAverage()               { Accum = (TYPE)0; Idx = FillAmount = 0; }
    void    Init(int count)         { Samples.resize(count); memset(Samples.Data, 0, (size_t)Samples.Size * sizeof(TYPE)); Accum = (TYPE)0; Idx = FillAmount = 0; }
    void    AddSample(TYPE v)       { Accum += v - Samples[Idx]; Samples[Idx] = v; if (++Idx == Samples.Size) Idx = 0; if (FillAmount < Samples.Size) FillAmount++;  }
    TYPE    GetAverage() const      { return Accum / (TYPE)FillAmount; }
    int     GetSampleCount() const  { return Samples.Size; }
    bool    IsFull() const          { return FillAmount == Samples.Size; }
};

//-----------------------------------------------------------------------------
// Misc ImGui extensions
//-----------------------------------------------------------------------------

class Str;

namespace ImGui
{

void    PushDisabled();
void    PopDisabled();

// STR + InputText bindings
bool    InputText(const char* label, Str* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
bool    InputTextMultiline(const char* label, Str* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);

}
