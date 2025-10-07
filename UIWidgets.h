#pragma once
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include <algorithm>
#include "Maths/Maths.h"
namespace Widgets {
    inline bool XYPad(const char* label, DXM::Vector2& value, float minX, float maxX, float minY, float maxY, ImVec2 size = { 200,200 })
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 end = { pos.x + size.x, pos.y + size.y };

        ImGui::InvisibleButton(label, size);
        bool active = ImGui::IsItemActive();

        // Background
        draw_list->AddRectFilled(pos, end, IM_COL32(50, 50, 50, 255), 4.0f);
        draw_list->AddRect(pos, end, IM_COL32(255, 255, 255, 255), 4.0f);

        bool changed = false;

        if (active) {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            float nx = (mouse.x - pos.x) / size.x;
            float ny = (mouse.y - pos.y) / size.y;

            nx = std::clamp(nx, 0.0f, 1.0f);
            ny = std::clamp(ny, 0.0f, 1.0f);

            value.x = minX + nx * (maxX - minX);
            value.y = minY + ny * (maxY - minY);

            changed = true;
        }

        // Draw indicator circle
        float cx = pos.x + (value.x - minX) / (maxX - minX) * size.x;
        float cy = pos.y + (value.y - minY) / (maxY - minY) * size.y;
        draw_list->AddCircleFilled(ImVec2(cx, cy), 6.0f, IM_COL32(200, 200, 200, 255));

        // Draw label
        float label_height = 0.0f;
        if (label && label[0] != '\0') {
            ImVec2 textSize = ImGui::CalcTextSize(label);
            draw_list->AddText(ImVec2(pos.x, pos.y + size.y + 2), IM_COL32(255, 255, 255, 255), label);
            label_height = textSize.y + 2; // small padding
        }

        // Advance cursor only by pad + label height
        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + size.y + label_height + ImGui::GetStyle().ItemSpacing.y));

        return changed;
    }

}