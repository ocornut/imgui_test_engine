#include "editor_widgets.h"
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "shared/imgui_utils.h"
#include "shared/IconsFontAwesome5.h"
#include "libs/Str/Str.h"
#include "libs/imgui_markdown/imgui_markdown.h"

static void LinkCallback(ImGui::MarkdownLinkCallbackData data)
{
    Str128f link("%.*s", data.linkLength, data.link);
    ImOsOpenInShell(link.c_str());
}

// FXIME-EDITORDEMO: Support texture fetching/rendering
// FXIME-EDITORDEMO: Generally it looks that imgui_markdown needs some polish.
void RenderMarkdown(const char* markdown, const char* markdown_end)
{
    if (!markdown_end)
        markdown_end = markdown + strlen(markdown);

    // FIXME-EDITORDEMO: Standardize access to font
    ImFontAtlas* atlas = ImGui::GetIO().Fonts;
    ImFont* font_regular = atlas->Fonts[0];
    ImFont* font_large = (atlas->Fonts.Size > 1) ? atlas->Fonts[1] : atlas->Fonts[0];

    ImGui::MarkdownConfig config;
    config.linkCallback = LinkCallback;
    config.linkIcon = ICON_FA_LINK;
    config.headingFormats[0] = { font_large, true };
    config.headingFormats[1] = { font_large, true };
    config.headingFormats[2] = { font_regular, false };

    // FIXME-EDITORDEMO FIXME-STYLE: Ultimately we'd need a nicer way of passing style blocks to custom.
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.800f, 0.656f, 0.140f, 1.000f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.000f, 0.656f, 0.140f, 1.000f));
    ImGui::Markdown(markdown, markdown_end - markdown, config);
    ImGui::PopStyleColor(2);
}

void ImGui::BeginButtonGroup()
{
    auto* storage = ImGui::GetStateStorage();
    auto* lists = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    storage->SetFloat(ImGui::GetID("button-group-x"), pos.x);
    storage->SetFloat(ImGui::GetID("button-group-y"), pos.y);
    lists->ChannelsSplit(2);
    lists->ChannelsSetCurrent(1);
}

void ImGui::EndButtonGroup()
{
    auto& style = ImGui::GetStyle();
    auto* lists = ImGui::GetWindowDrawList();
    auto* storage = ImGui::GetStateStorage();
    ImVec2 min(
        storage->GetFloat(ImGui::GetID("button-group-x")),
        storage->GetFloat(ImGui::GetID("button-group-y"))
    );
    lists->ChannelsSetCurrent(0);
    lists->AddRectFilled(min, ImGui::GetItemRectMax(), ImColor(style.Colors[ImGuiCol_Button]), style.FrameRounding);
    lists->ChannelsMerge();
    ImGui::PopStyleVar();
}

bool ImGui::SquareButton(const char* label)
{
    ImGuiContext& g = *GImGui;
    float dimension = g.FontSize + g.Style.ItemInnerSpacing.y * 2.0f;
    return ImGui::ButtonEx(label, ImVec2(dimension, dimension), ImGuiButtonFlags_PressedOnClick);
}

bool ImGui::ToggleButton(const char* text, const char* tooltip, bool active)
{
    const auto& style = ImGui::GetStyle();
    if (active)
        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
    else
        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);
    bool result = SquareButton(text);
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 0);
    if (ImGui::IsItemHovered() && tooltip)
        ImGui::SetTooltip("%s", tooltip);
    return result;
}
