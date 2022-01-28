#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include <math.h>           // fabsf
#include "imgui.h"          // ImGuiID, ImGuiKey, ImFont

//-----------------------------------------------------------------------------
// Forward Declarations
//-----------------------------------------------------------------------------

struct ImGuiTable;
class Str;

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

// Miscellaneous functions
ImGuiID             ImHashDecoratedPath(const char* str, const char* str_end = NULL, ImGuiID seed = 0);
ImFont*             FindFontByName(const char* name);
void                ImStrReplace(Str* s, const char* find, const char* repl);
void                ImStrXmlEscape(Str* s);
int                 ImBase64Encode(const unsigned char* src, char* dst, int length);

// Inputs functions
#if defined(__APPLE__) // FIXME: Setting IO.ConfigMacOSXBehaviors to non-default value breaks this assumption.
#define ImGuiKeyModFlags_Shortcut   ImGuiKeyModFlags_Super
#else
#define ImGuiKeyModFlags_Shortcut   ImGuiKeyModFlags_Ctrl
#endif
void                GetImGuiKeyModsPrefixStr(ImGuiKeyModFlags mod_flags, char* out_buf, size_t out_buf_size);

// Tables functions
ImGuiID             TableGetHeaderID(ImGuiTable* table, const char* column, int instance_no = 0);
ImGuiID             TableGetHeaderID(ImGuiTable* table, int column_n, int instance_no = 0);
void                TableDiscardInstanceAndSettings(ImGuiID table_id);

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
// Simple CSV parser
//-----------------------------------------------------------------------------

struct ImGuiCSVParser
{
    int             Columns = 0;                    // Number of columns in CSV file.
    int             Rows = 0;                       // Number of rows in CSV file.

    char*           _Data = NULL;                   // CSV file data.
    ImVector<char*> _Index;                         // CSV table: _Index[row * _Columns + col].

    ImGuiCSVParser(int columns = -1)                { Columns = columns; }
    ~ImGuiCSVParser()                               { Clear(); }

    bool            Load(const char* file_name);    // Open and parse a CSV file.
    void            Clear();                        // Free allocated buffers.
    const char*     GetCell(int row, int col)       { IM_ASSERT(0 <= row && row < Rows && 0 <= col && col < Columns); return _Index[row * Columns + col]; }
};

//-----------------------------------------------------------------------------
// Misc Dear ImGui extensions
//-----------------------------------------------------------------------------

namespace ImGui
{

// STR + InputText bindings
bool    InputText(const char* label, Str* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
bool    InputTextWithHint(const char* label, const char* hint, Str* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
bool    InputTextMultiline(const char* label, Str* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
bool    Splitter(const char* id, float* value_1, float* value_2, int axis, int anchor = 0, float min_size_0 = -1.0f, float min_size_1 = -1.0f);

}

