#pragma once

void RenderMarkdown(const char* markdown, const char* markdown_end = 0);

namespace ImGui
{

void BeginButtonGroup();
void EndButtonGroup();
bool SquareButton(const char* label);
bool ToggleButton(const char* text, const char* tooltip, bool active);

}   // namespace ImGui
