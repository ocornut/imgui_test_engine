#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#include "imgui_te_util.h"
#include "imgui_internal.h"
#define STR_IMPLEMENTATION
#include "libs/Str/Str.h"

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

const char* GetImGuiKeyName(ImGuiKey key)
{
    // Create switch-case from enum with regexp: ImGuiKey_{.*}, --> case ImGuiKey_\1: return "\1";
    switch (key)
    {
    case ImGuiKey_Tab: return "Tab";
    case ImGuiKey_LeftArrow: return "LeftArrow";
    case ImGuiKey_RightArrow: return "RightArrow";
    case ImGuiKey_UpArrow: return "UpArrow";
    case ImGuiKey_DownArrow: return "DownArrow";
    case ImGuiKey_PageUp: return "PageUp";
    case ImGuiKey_PageDown: return "PageDown";
    case ImGuiKey_Home: return "Home";
    case ImGuiKey_End: return "End";
    case ImGuiKey_Insert: return "Insert";
    case ImGuiKey_Delete: return "Delete";
    case ImGuiKey_Backspace: return "Backspace";
    case ImGuiKey_Space: return "Space";
    case ImGuiKey_Enter: return "Enter";
    case ImGuiKey_Escape: return "Escape";
    case ImGuiKey_A: return "A";
    case ImGuiKey_C: return "C";
    case ImGuiKey_V: return "V";
    case ImGuiKey_X: return "X";
    case ImGuiKey_Y: return "Y";
    case ImGuiKey_Z: return "Z";
    }
    IM_ASSERT(0);
    return "Unknown";
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

//-----------------------------------------------------------------------------
// Misc ImGui extensions
//-----------------------------------------------------------------------------

void ImGui::PushDisabled()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4 col = style.Colors[ImGuiCol_Text];
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(col.x, col.y, col.z, col.w * 0.5f));
}

void ImGui::PopDisabled()
{
    ImGui::PopStyleColor();
    ImGui::PopItemFlag();
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

#if IMGUI_HAS_TABLE
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

void TableDiscardInstanceAndSettings(ImGuiID table_id)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.CurrentTable == NULL);
    if (ImGuiTableSettings* settings = ImGui::TableSettingsFindByID(table_id))
        settings->ID = 0;
    if (ImGuiTable* table = ImGui::FindTableByID(table_id))
        g.Tables.Remove(table->ID, table);
}
#endif
