#include "editor_widgets.h"
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "shared/IconsFontAwesome5.h"
#include "thirdparty/imgui_markdown/imgui_markdown.h"
#include "imgui_test_engine/imgui_te_utils.h"
#include "imgui_test_engine/thirdparty/Str/Str.h"

static void LinkCallback(ImGui::MarkdownLinkCallbackData data)
{
    Str128f link("%.*s", data.linkLength, data.link);
    ImOsOpenInShell(link.c_str());
}

// FIXME-SANDBOX: Support texture fetching/rendering
// FIXME-SANDBOX: Generally it looks that imgui_markdown needs some polish.
void RenderMarkdown(const char* markdown, const char* markdown_end)
{
    if (!markdown_end)
        markdown_end = markdown + strlen(markdown);

    // FIXME-SANDBOX: Standardize access to font
    ImFontAtlas* atlas = ImGui::GetIO().Fonts;
    ImFont* font_regular = atlas->Fonts[0];
    ImFont* font_large = (atlas->Fonts.Size > 1) ? atlas->Fonts[1] : atlas->Fonts[0];

    ImGui::MarkdownConfig config;
    config.linkCallback = LinkCallback;
    config.linkIcon = ICON_FA_LINK;
    config.headingFormats[0] = { font_large, true };
    config.headingFormats[1] = { font_large, true };
    config.headingFormats[2] = { font_regular, false };

    // FIXME-SANDBOX FIXME-STYLE: Ultimately we'd need a nicer way of passing style blocks to custom.
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.800f, 0.656f, 0.140f, 1.000f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.000f, 0.656f, 0.140f, 1.000f));
    ImGui::Markdown(markdown, markdown_end - markdown, config);
    ImGui::PopStyleColor(2);
}

void ImGui::BeginButtonGroup()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiStorage* storage = ImGui::GetStateStorage();

    ImVec2 pos = GetCursorScreenPos();
    PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);    // FIXME-SANDBOX: Scaling
    storage->SetFloat(GetID("button-group-x"), pos.x);  // FIXME-SANDBOX: Not sure we want to promote using this..
    storage->SetFloat(GetID("button-group-y"), pos.y);
    draw_list->ChannelsSplit(2);
    draw_list->ChannelsSetCurrent(1);
}

void ImGui::EndButtonGroup()
{
    ImGuiStyle& style = GetStyle();
    ImDrawList* draw_list = GetWindowDrawList();
    ImGuiStorage* storage = GetStateStorage();

    ImVec2 min(
        storage->GetFloat(GetID("button-group-x")),
        storage->GetFloat(GetID("button-group-y"))
    );
    draw_list->ChannelsSetCurrent(0);
    draw_list->AddRectFilled(min, GetItemRectMax(), GetColorU32(ImGuiCol_Button), style.FrameRounding);
    draw_list->ChannelsMerge();
    PopStyleVar();
}

bool ImGui::SquareButton(const char* label)
{
    ImGuiContext& g = *GImGui;
    float dimension = g.FontSize + g.Style.ItemInnerSpacing.y * 2.0f;
    return ButtonEx(label, ImVec2(dimension, dimension), ImGuiButtonFlags_PressedOnClick);
}

bool ImGui::ToggleButton(const char* text, const char* tooltip, bool active)
{
    const auto& style = ImGui::GetStyle();
    if (active)
        PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
    bool result = SquareButton(text);
    if (active)
        PopStyleColor();
    SameLine(0, 0);
    if (IsItemHovered() && tooltip)
        SetTooltip("%s", tooltip);
    return result;
}
