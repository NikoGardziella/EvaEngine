#include "pch.h"
#include "TileEditorPanel.h"

#include <Engine/Debug/Instrumentor.h>

#include <imgui/imgui.h>
#include "Engine/AssetManager/AssetManager.h"


namespace Engine {

    TileEditorPanel::TileEditorPanel()
    {
       // CreateTileAtlas();
        AssetManager::CreateTileAtlas();
		m_tileUVMap = AssetManager::GetTileTextureAtalsUVs();

    }

    void TileEditorPanel::OnImGuiRender()
    {
        EE_PROFILE_FUNCTION();

        ImGui::Begin("Tile Editor");

        // Array of categories and their enum values
        struct CategoryEntry { const char* name; eTileCategory category; };
        static const CategoryEntry categories[] = {
            { "Buildings", eTileCategory::Buildings },
            { "Terrain", eTileCategory::Terrain },
            { "Roofs", eTileCategory::Roofs },
            { "Vehicles", eTileCategory::Vehicles }
        };

        for (const auto& entry : categories)
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
            bool isSelected = (m_selectedTileCategory == entry.category);
            if (isSelected)
                flags |= ImGuiTreeNodeFlags_Selected;

            // Use unique ID by combining name with category enum to avoid ImGui ID conflicts
            std::string treeNodeId = std::string(entry.name) + std::to_string((int)entry.category);

            bool open = ImGui::TreeNodeEx(treeNodeId.c_str(), flags, "%s", entry.name);

            if (ImGui::IsItemClicked())
            {
                m_selectedTileCategory = entry.category;
            }

            if (open)
            {
                // Add some spacing before drawing palette so it doesn't visually overlap
                ImGui::Spacing();

                // Use BeginChild for clipping and scrolling if palette is big
                const float iconSize = 32.0f;
                const int itemsPerRow = 4;
                const float padding = 4.0f; // spacing between icons and top/bottom

                const auto& tileNames = AssetManager::GetTileNamesByCategory(entry.category);
                int tileCount = (int)tileNames.size();
                int rowCount = (tileCount + itemsPerRow - 1) / itemsPerRow; // ceiling division

                // Calculate child height: rows * icon size + padding * (rows + 1)
                float childHeight = rowCount * iconSize + padding * (rowCount + 1);

                ImGui::BeginChild(("PaletteChild_" + std::string(entry.name)).c_str(), ImVec2(0, childHeight), true);

                DrawTilePalette(entry.category);

                ImGui::EndChild();

                ImGui::TreePop();
            }
        }

        ImGui::End();
    }




    void TileEditorPanel::DrawTilePalette(eTileCategory category)
    {
        const float iconSize = 32.0f;
        int itemsPerRow = 8;

        // Get tile names and UV map for the category
        const auto& tileNames = AssetManager::GetTileNamesByCategory(category);
        const auto& tileUVMap = AssetManager::GetTileUVMap(category);

        // Get the atlas texture to display (shared big atlas)
        ImTextureID textureID = (ImTextureID)AssetManager::GetTileTextureIconAtlas()->GetTextureDescriptor();

        for (size_t i = 0; i < tileNames.size(); ++i)
        {
            const std::string& name = tileNames[i];

            auto uvIt = tileUVMap.find(name);
            if (uvIt == tileUVMap.end())
                continue;  // No UV for this tile, skip

            const glm::vec4& uv = uvIt->second;

            ImGui::PushID((int)i);

            if (ImGui::ImageButton("##tileIcon", textureID,
                ImVec2(iconSize, iconSize),
                ImVec2(uv.x, uv.y),
                ImVec2(uv.z, uv.w)))
            {
                m_selectedTileName = name;
                m_selectedTileCategory = category;  // Also update category if you want
            }

            if (m_selectedTileName == name)
            {
                auto pMin = ImGui::GetItemRectMin();
                auto pMax = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddRect(pMin, pMax, IM_COL32(255, 255, 0, 255), 0.0f, 0, 3.0f);
            }

            ImGui::PopID();

            if ((i + 1) % itemsPerRow != 0)
                ImGui::SameLine();
        }
    }





}