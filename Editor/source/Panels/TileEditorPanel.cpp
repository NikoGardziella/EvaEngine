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
		m_tileNames = AssetManager::GetTileNames();
		m_tileUVMap = AssetManager::GetTileUVMap();

    }


    void TileEditorPanel::OnImGuiRender()
    {
       EE_PROFILE_FUNCTION();

       DrawTilePalette();
       
    
    }


    void TileEditorPanel::DrawTilePalette()
    {
        EE_PROFILE_FUNCTION();

        ImGui::Begin("Tile Palette");

        const float iconSize = 32.0f;
        int itemsPerRow = 8;

        for (size_t i = 0; i < m_tileNames.size(); ++i)
        {
            const std::string& name = m_tileNames[i];

            auto it = m_tileUVMap.find(name);
            if (it == m_tileUVMap.end())
                continue;

            const glm::vec4& uv = it->second;

            ImGui::PushID((int)i);
            ImTextureID textureID = (ImTextureID)AssetManager::GetTileTextureIconAtlas()->GetTextureDescriptor();

            if (ImGui::ImageButton("##tileIcon", textureID,
                ImVec2(iconSize, iconSize),
                ImVec2(uv.x, uv.y),
                ImVec2(uv.z, uv.w)))
            {
                m_selectedTileName = name;
            }

            if (m_selectedTileName == name)
            {
                auto pMin = ImGui::GetItemRectMin();
                auto pMax = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddRect(pMin, pMax,
                    IM_COL32(255, 255, 0, 255), 0.0f, 0, 3.0f);
            }

            ImGui::PopID();

            if ((i + 1) % itemsPerRow != 0)
                ImGui::SameLine();
        }

        ImGui::End();
    }


}