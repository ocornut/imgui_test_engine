#include "editor_widgets.h"
#include "imgui.h"
#include "test_engine/imgui_te_util.h"
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

    ImGui::MarkdownConfig config;
    config.linkCallback = LinkCallback;
    config.linkIcon = ICON_FA_LINK;
    config.headingFormats[0] = { ImGui::GetIO().Fonts->Fonts[1], true };
    config.headingFormats[1] = { ImGui::GetIO().Fonts->Fonts[1], true };
    config.headingFormats[2] = { ImGui::GetIO().Fonts->Fonts[0], false };

    // FIXME-EDITORDEMO FIXME-STYLE: Ultimately we'd need a nicer way of passing style blocks to custom.
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.800f, 0.656f, 0.140f, 1.000f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.000f, 0.656f, 0.140f, 1.000f));
    ImGui::Markdown(markdown, markdown_end - markdown, config);
    ImGui::PopStyleColor(2);
}
