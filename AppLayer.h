#pragma once
#include <Layer.h>
#include <Renderer/Renderer.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#include <Renderer/Texture.h>

#include "Maths/Maths.h"

#include "DrawFunctions.h"





class AppLayer : public DXE::Layer {
public:

    AppLayer() {};
    AppLayer(const std::string& _name) : Layer(_name) {};
    bool HandleInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void OnAttach();
    void OnDetach();
    void Update(float dt);
    void Render(float dt);

    using GeneratorFactory = std::function<std::unique_ptr<IFrameGenerator>()>;
    std::unique_ptr<IFrameGenerator> ActiveGenerator;
    std::unordered_map<std::string, GeneratorFactory> Generators;
    std::string SelectedGeneratorName;

    std::vector<DXE::Texture*> TextureFrames;
    int SelectedFrame = -1;
    int Size = 256;
    int FrameCount = 30;
    bool Playing = false;
    std::atomic<bool> IsGenerating = false;

    void RegisterGenerators() {
        Generators = {
            { "Slash Trail",    []() { return std::make_unique<SlashTrailGenerator>();}},
            { "Lightning Beam", []() { return std::make_unique<LightningBeamGenerator>();}}
        };
    }




    void GenerateFramesMultiThreaded();

    void DeleteFrame(std::vector<DXE::Texture*>& textures, size_t index) {
        if (index >= textures.size() || index < 0) return;
        delete textures[index];                  // free memory
        textures.erase(textures.begin() + index); // remove from vector
    }
    void AddFrame(std::vector<DXE::Texture*>& textures, int size) {
        TextureFrames.push_back(new DXE::Texture(size,size,4));
    }

    void DrawFrameTimeline(std::vector<DXE::Texture*>& frames, int& selectedFrame)
    {
        ImGui::Begin("Timeline");

        const float frameSize = 32.0f;
        const float spacing = 4.0f;

        float contentWidth = (frameSize + spacing) * frames.size();
        ImVec2 regionAvail = ImGui::GetContentRegionAvail();

        // Calculate horizontal offset to center the timeline
        float offsetX = 0.0f;
        if (contentWidth < regionAvail.x)
            offsetX = (regionAvail.x - contentWidth) * 0.5f;  // center horizontally

        // BeginChild with NO scrollbars
        ImGui::BeginChild(
            "TimelineRegion",
            regionAvail,
            false,
            ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();
        origin.x += offsetX;

        for (size_t i = 0; i < frames.size(); ++i)
        {
            ImVec2 p_min = { origin.x + i * (frameSize + spacing), origin.y };
            ImVec2 p_max = { p_min.x + frameSize, p_min.y + frameSize };

            draw_list->AddRectFilled(p_min, p_max, IM_COL32(40, 40, 40, 255));
            draw_list->AddRect(p_min, p_max, IM_COL32(80, 80, 80, 255));

            if (frames[i]) draw_list->AddImage((ImTextureID)frames[i]->GetShaderResourceView(), p_min, p_max);

            ImGui::SetCursorScreenPos(p_min);
            ImGui::InvisibleButton(("frame_" + std::to_string(i)).c_str(), ImVec2(frameSize, frameSize));

            if (ImGui::IsItemClicked()) {
                selectedFrame = (int)i;
                Playing = false;
            }

            if ((int)i == selectedFrame)
                draw_list->AddRect(p_min, p_max, IM_COL32(255, 200, 0, 255), 0.0f, 0, 2.0f);
        }

        ImGui::EndChild();
        ImGui::End();
    }

    void DrawCheckerboard(float tile_size,
        ImU32 col1 = IM_COL32(200, 200, 200, 255),
        ImU32 col2 = IM_COL32(100, 100, 100, 255))
    {
        // Get the draw list for the current window
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Get current window position and content region size
        ImVec2 win_pos = ImGui::GetCursorScreenPos();
        ImVec2 win_size = ImGui::GetContentRegionAvail();

        int tiles_x = (int)(win_size.x / tile_size) + 1;
        int tiles_y = (int)(win_size.y / tile_size) + 1;

        for (int y = 0; y < tiles_y; ++y) {
            for (int x = 0; x < tiles_x; ++x) {
                bool isEven = (x + y) % 2 == 0;
                ImVec2 tile_min = ImVec2(win_pos.x + x * tile_size, win_pos.y + y * tile_size);
                ImVec2 tile_max = ImVec2(tile_min.x + tile_size, tile_min.y + tile_size);
                draw_list->AddRectFilled(tile_min, tile_max, isEven ? col1 : col2);
            }
        }
    }
    void DrawMainFramePreview(const std::vector<DXE::Texture*>& frames, int selectedFrame)
    {
        ImGui::Begin("Frame Preview", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        DrawCheckerboard(16.f, IM_COL32(3, 3, 3, 255), IM_COL32(1, 1, 1, 255));

        if (selectedFrame >= 0 && selectedFrame < (int)frames.size() && frames[selectedFrame])  {
            // Get the texture to display
            ImTextureID texID = (ImTextureID)frames[selectedFrame]->GetShaderResourceView();

            // Compute available region and aspect ratio
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float texWidth = (float)frames[selectedFrame]->Width();
            float texHeight = (float)frames[selectedFrame]->Height();
            float aspect = texWidth / texHeight;

            // Fit image inside available area
            ImVec2 size;
            if (avail.x / avail.y > aspect)
                size = ImVec2(avail.y * aspect, avail.y);
            else
                size = ImVec2(avail.x, avail.x / aspect);

            // Center it
            ImVec2 cursor = ImGui::GetCursorPos();
            ImVec2 centerOffset = ImVec2((avail.x - size.x) * 0.5f, (avail.y - size.y) * 0.5f);
            ImVec2 pos = { cursor.x + centerOffset.x, cursor.y + centerOffset.y };
            ImGui::SetCursorPos(pos);

            // Draw texture
            ImGui::Image(texID, size);
        }
        else
        {
            ImGui::TextDisabled("No frame selected");
        }

        ImGui::End();
    }

    void ResizeFrames(int size) { for (auto& tex : TextureFrames) { tex->Resize(size, size, 4);} }
    
    void GenerateFrames(std::vector<DXE::Texture*>& frames, int frameCount) {
        //  for (DXE::Texture* tex : frames) {
        //      delete tex;  // free the Texture object
        //  }
        //  frames.clear();  // remove all pointers from the vector
        //  
        //  for (int i = 0;i < FrameCount;i++) {
        //      TextureFrames.push_back(new DXE::Texture(Size, Size, 4));
        //      double time = (double)i / (FrameCount - 1.f);
        //      //Draw::Crescent(TextureFrames[i], time);
        //      //Draw::UnevenCapsule(TextureFrames[i], time);
        //      //Draw::OpenRing(TextureFrames[i], time);
        //      //Draw::OpenRingRounded(TextureFrames[i], time);
        //      Draw::SlashTrail(TextureFrames[i], time);
        //  }
        GenerateFramesMultiThreaded();

    }

    void DrawGeneratorUI() {
        const char* current = SelectedGeneratorName.empty()
            ? "Select Generator"
            : SelectedGeneratorName.c_str();

        if (ImGui::BeginCombo("Generator", current)) {
            for (auto& [name, factory] : Generators) {
                bool isSelected = (name == SelectedGeneratorName);
                if (ImGui::Selectable(name.c_str(), isSelected)) {
                    SelectedGeneratorName = name;
                    ActiveGenerator = factory(); // create new instance
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ActiveGenerator) {
            if (ImGui::Button("Generate Frames")) {
                GenerateFramesMultiThreaded();
            }

            ImGui::SeparatorText("Parameters");
            static bool changed = false;
            changed |= ActiveGenerator->DrawImGui();

            if (changed && ImGui::IsMouseReleased(0)) {
                GenerateFramesMultiThreaded();
                changed = false;
            }

        }


    }
    
};




