#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "editor_assets_browser.h"
#include "imgui.h"
#include "imgui_internal.h" // FIXME: We want to avoid that.

// [Advanced] Helper class to simulate storage of a multi-selection state, used by the advanced multi-selection demos.
// We use ImGuiStorage (simple key->value storage) to avoid external dependencies but it's probably not optimal.
// To store a single-selection:
// - You only need a single variable and don't need any of this!
// To store a multi-selection, in your real application you could:
// - Use intrusively stored selection (e.g. 'bool IsSelected' inside your object). This is by far the simplest
//   way to store your selection data, but it means you cannot have multiple simultaneous views over your objects.
//   This is what may of the simpler demos in this file are using (so they are not using this class).
// - Otherwise, any externally stored unordered_set/set/hash/map/interval trees (storing indices, objects id, etc.)
//   are generally appropriate. Even a large array of bool might work for you...
struct ExampleSelectionData
{
    ImGuiStorage                        Storage;
    int                                 SelectedCount;  // Number of selected items (storage will keep this updated)

    ExampleSelectionData()              { Clear(); }
    void Clear()                        { Storage.Clear(); SelectedCount = 0; }
    bool GetSelected(int id) const      { return Storage.GetInt((ImGuiID)id) != 0; }
    void SetSelected(int id, bool v)    { int* p_int = Storage.GetIntRef((ImGuiID)id); if (*p_int == (int)v) return; SelectedCount = v ? (SelectedCount + 1) : (SelectedCount - 1); *p_int = (bool)v; }
    int GetSelectedCount() const        { return SelectedCount; }

    // When using SelectAll() / SetRange() we assume that our objects ID are indices.
    // In this demo we always store selection using indices and never in another manner (e.g. object ID or pointers).
    // If your selection system is storing selection using object ID and you want to support Shift+Click range-selection,
    // you will need a way to iterate from one object to another given the ID you use.
    // You are likely to need some kind of data structure to convert 'view index' from/to 'ID'.
    // FIXME-MULTISELECT: Would be worth providing a demo of doing this.
    // FIXME-MULTISELECT: SetRange() is currently very inefficient since it doesn't take advantage of the fact that ImGuiStorage stores sorted key.
    void SetRange(int a, int b, bool v) { if (b < a) { int tmp = b; b = a; a = tmp; } for (int n = a; n <= b; n++) SetSelected(n, v); }
    void SelectAll(int count)           { Storage.Data.resize(count); for (int n = 0; n < count; n++) Storage.Data[n] = ImGuiStorage::ImGuiStoragePair((ImGuiID)n, 1); SelectedCount = count; } // This could be using SetRange() but this is faster.
};

struct ExampleAssetBrowser
{
    int                     ItemCount;
    ExampleSelectionData    Selection;
    int                     SelectionRef;   // Last clicked/navigated item
    int                     IconSize;

    ExampleAssetBrowser()
    {
        ItemCount = 10000;
        SelectionRef = -1;
        IconSize = 64;
    }

#ifdef IMGUI_HAS_MULTI_SELECT
    void ApplySelectionRequests(ImGuiMultiSelectData* ms_data)
    {
            if (ms_data->RequestClear)
            Selection.Clear();
        if (ms_data->RequestSelectAll)
            Selection.SelectAll(ItemCount);
        if (ms_data->RequestSetRange)
            Selection.SetRange((int)(intptr_t)ms_data->RangeSrc, (int)(intptr_t)ms_data->RangeDst, ms_data->RangeValue);
    }
#endif

    void Draw(const char *title, bool* p_open)
    {
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        const ImGuiStyle& style = ImGui::GetStyle();

        ImGui::PushItemWidth(ImGui::GetFontSize() * 10);
        ImGui::SliderInt("Icon Size", &IconSize, 32, 128, "");
        ImGui::PopItemWidth();

#ifndef IMGUI_HAS_MULTI_SELECT
        ImGui::Text("Error: This demo is designed for branches with IMGUI_HAS_MULTI_SELECT!\nMulti-selection and range-selection with mouse and keyboard won't be functional.");
#endif

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
#ifdef IMGUI_HAS_MULTI_SELECT
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableSpacing, ImVec2(2, 2)); // FIXME: Distinguish visual vs hit spacing
#endif

        ImVec2 item_size((float)IconSize, (float)IconSize);
        if (ImGui::BeginChild("entries", ImVec2(0, 0), true))
        {
            // Columns
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            const float width = ImGui::GetContentRegionAvail().x - item_size.x;
            const int spacing_count = (int)(width / (item_size.x + style.ItemSpacing.x));
            const int column_count = 1 + spacing_count;

            // Item and Line Count
            const int line_count = (column_count > 0) ? ((ItemCount + column_count - 1) / column_count) : 0;

            // Item spacing
            float item_spacing_x = style.ItemSpacing.x;
            //float item_spacing_x = ImFloor(width / spacing_count) - item_size.x;

            // Line Height
            float line_height = item_size.y + style.ItemSpacing.y;

            ImVec2 start_pos = ImGui::GetCursorPos();
            start_pos.x += item_spacing_x * 0.5f;
            start_pos.y += style.ItemSpacing.y * 0.5f;
            ImGui::SetCursorPos(start_pos);

#ifdef IMGUI_HAS_MULTI_SELECT
            ImGuiMultiSelectData* ms_data = ImGui::BeginMultiSelect(0, (void*)(intptr_t)SelectionRef, Selection.GetSelected(SelectionRef));
            ApplySelectionRequests(ms_data);
#endif

            ImGuiListClipper clipper;
            clipper.Begin(line_count, line_height);
            while (clipper.Step())
            {
                for (int line_index = clipper.DisplayStart; line_index < clipper.DisplayEnd; line_index++)
                {
                    int item_min_index_for_line = line_index * column_count;
                    int item_max_index_for_line = ImMin((line_index + 1) * column_count, ItemCount);

#ifdef IMGUI_HAS_MULTI_SELECT
                    if (item_min_index_for_line >= (int)(intptr_t)ms_data->RangeSrc)
                        ms_data->RangeSrcPassedBy = true;
#endif

                    for (int item_index = item_min_index_for_line; item_index < item_max_index_for_line; ++item_index)
                    {
                        ImGui::PushID(item_index);

                        ImVec2 pos = ImVec2(start_pos.x + (item_index % column_count) * (item_size.x + item_spacing_x), start_pos.y + (line_index * line_height));
                        ImGui::SetCursorPos(pos);

                        ImVec2 screen_pos = ImGui::GetCursorScreenPos();
                        ImVec2 box_min(screen_pos.x - 1, screen_pos.y - 1);
                        ImVec2 box_max(box_min.x + IconSize + 2, box_min.y + IconSize + 2);

                        ImGuiSelectableFlags selectable_flags = 0;//ImGuiSelectableFlags_NavAutoPress | ImGuiSelectableFlags_NavAutoSelect | ImGuiSelectableFlags_PressedOnClick;

                        bool visible = ImGui::IsRectVisible(box_min, box_max);
                        if (visible)
                            draw_list->AddRect(box_min, box_max, IM_COL32(90, 90, 90, 255));

                        bool item_selected = Selection.GetSelected(item_index);
#ifdef IMGUI_HAS_MULTI_SELECT
                        ImGui::SetNextItemSelectionData((void*)(intptr_t)item_index);
#endif
                        if (ImGui::Selectable("##select", item_selected, selectable_flags, item_size))
                            if (ImGui::IsItemToggledSelection())
                            {
                                item_selected = !item_selected;
                                Selection.SetRange(item_index, item_index, item_selected);
                            }

                        // Demonstrate dragging
                        // FIXME-MULTISELECT: Selectable() currently has a hacky workaround to make this work,
                        // the side-effect of which makes CTRL+Shift+ tends to react on _release_ (acceptable side-effect)
                        if (ImGui::BeginDragDropSource())
                        {
                            ImGui::SetDragDropPayload("blah", "hi", 2);
                            ImGui::Text("%d assets", (int)Selection.GetSelectedCount());
                            ImGui::EndDragDropSource();
                        }
                        else
                        {
                            ImGui::SetItemAllowOverlap();
                        }

                        // Demonstrate the effect of right-clicking (currently handled in MultiSelectable but should probably because a flag of Selectable)
                        if (ImGui::BeginPopupContextItem())
                        {
                            ImGui::Text("Selection: %d items", (int)Selection.GetSelectedCount());
                            if (ImGui::Button("Close"))
                                ImGui::CloseCurrentPopup();
                            ImGui::EndPopup();
                        }

                        // FIXME: Thumbnail here
                        if (visible)
                        {
                            char label[32];
                            sprintf(label, "%d", item_index);
                            //drawList->AddRect(box_min, box_max, IM_COL32(90, 90, 90, 255));
                            //ImU32 random_color = IM_COL32((item_index * 61) & 0xFF, (item_index * 89) & 0xFF, (item_index * 167) & 0xFF, 0xFF);
                            //drawList->AddRectFilled(ImVec2(box_min.x + 2, box_min.y + 2), ImVec2(box_max.x - 2, box_max.y - 2), random_color);
                            draw_list->AddRectFilled(box_min, box_max, IM_COL32(48, 48, 48, 128));
                            draw_list->AddText(ImVec2(box_min.x, box_max.y - ImGui::GetFontSize()), (item_selected || 0) ? IM_COL32(255, 255, 255, 255) : ImGui::GetColorU32(ImGuiCol_TextDisabled), label);
                        }

                        ImGui::PopID();

                        if ((item_index % column_count) < (column_count - 1))
                            ImGui::SameLine();
                    }
                }
            }
            clipper.End();

#ifdef IMGUI_HAS_MULTI_SELECT
            ms_data = ImGui::EndMultiSelect(); // FIXME: Wrapping fallback would be processed here?
            SelectionRef = (int)(intptr_t)ms_data->RangeSrc;
            // FIXME-MULTISELECT
            if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
                if (ImGui::IsMouseReleased(0) && !ImGui::IsMouseDragPastThreshold(0))
                    ms_data->RequestClear = true;
            ApplySelectionRequests(ms_data);
#endif

            ImGui::NavMoveRequestTryWrapping(ImGui::GetCurrentWindow(), ImGuiNavMoveFlags_WrapX);
        }

        ImGui::EndChild();
#ifdef IMGUI_HAS_MULTI_SELECT
        ImGui::PopStyleVar();
#endif
        ImGui::PopStyleVar();
        ImGui::End();
    }
};

void ShowExampleAppAssetBrowser(bool* p_open)
{
    static ExampleAssetBrowser asset_browser;
    asset_browser.Draw("Example: Asset Browser", p_open);
}
