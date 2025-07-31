#include "pch.h"
#include "TileEditorPanel.h"

#include <Engine/Debug/Instrumentor.h>

#include <imgui/imgui.h>
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/AssetManager/Utils/TileSerializer.h"

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

        struct CategoryEntry { const char* name; eTileCategory category; };
        static const CategoryEntry categories[] = {
            { "Buildings", eTileCategory::Buildings },
            { "Terrain",   eTileCategory::Terrain },
            { "Roofs",     eTileCategory::Roofs },
            { "Vehicles",  eTileCategory::Vehicles }
        };


        for (const auto& entry : categories)
        {
            std::string id = std::string(entry.name) + std::to_string((int)entry.category);
            bool open = ImGui::TreeNodeEx(id.c_str(), ImGuiTreeNodeFlags_DefaultOpen, "%s", entry.name);

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                m_selectedTileCategory = entry.category;

            if (open)
            {
                float tileSize = 48.0f;
                int tilesPerRow = 6;
                int tileCount = AssetManager::GetTileCountForCategory(entry.category);
                int rows = (tileCount + tilesPerRow - 1) / tilesPerRow;
                float childHeight = rows * tileSize;

                ImGui::BeginChild(("Palette_" + std::string(entry.name)).c_str(), ImVec2(0, childHeight), true);
                DrawTilePalette(entry.category);
                ImGui::EndChild();
                ImGui::TreePop();
            }
        }
        ImGui::End(); 

        if (!m_selectedTileName.empty())
        {
            ImGui::Begin("Tile Properties");

            // Initialize defaults if uninitialized
            if (m_selectedTileprops.health == 0 && m_selectedTileprops.material == eTileMaterial::None)
            {
                auto it = m_tilePropertyDefaults.find(m_selectedTileName);
                if (it != m_tilePropertyDefaults.end())
                {
                    m_selectedTileprops = it->second;
                }
            }

            ImGui::Text("Editing: %s", m_selectedTileName.c_str());

            bool tilePropertiesChanged = false;

            if (ImGui::InputScalar("Health", ImGuiDataType_U32, &m_selectedTileprops.health))
            {
                tilePropertiesChanged = true;
            }

            static const char* materialOptions[] = { "None", "Wood", "Concrete", "Metal", "Glass" };
            constexpr int materialCount = IM_ARRAYSIZE(materialOptions);

            int currentMaterialIdx = static_cast<int>(m_selectedTileprops.material);
            if (ImGui::Combo("Material", &currentMaterialIdx, materialOptions, materialCount))
            {
                m_selectedTileprops.material = static_cast<eTileMaterial>(currentMaterialIdx);
                tilePropertiesChanged = true;
            }

            if (tilePropertiesChanged)
            {
                m_modifiedTileProperties[m_selectedTileName] = m_selectedTileprops;
                TileSerializer::Save(m_modifiedTileProperties);
            }

            if (ImGui::Button("Reset to Default"))
            {
                auto it = m_tilePropertyDefaults.find(m_selectedTileName);
                if (it != m_tilePropertyDefaults.end())
                {
                    m_selectedTileprops = it->second;
                    m_modifiedTileProperties[m_selectedTileName] = m_selectedTileprops;
                    TileSerializer::Save(m_modifiedTileProperties);
                }
            }

            ImGui::End();
        }




    }



   

    void TileEditorPanel::DrawTilePalette(eTileCategory category)
    {
        const float iconSize = 32.0f;
        const int itemsPerRow = 8;

        ImTextureID textureID = (ImTextureID)AssetManager::GetTileTextureIconAtlas()->GetTextureDescriptor();

        if (category == eTileCategory::Buildings)
        {
            for (int m = 0; m < (int)eTileMaterial::COUNT; ++m)
            {
                eTileMaterial material = static_cast<eTileMaterial>(m);

                // Only show if there are tiles in this material
                const auto& tileNames = AssetManager::GetTileNamesByCategoryAndMaterial(category, material);
                if (tileNames.empty()) continue;

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;

                std::string materialName = GetTileMaterialName(material); //
                if (ImGui::TreeNodeEx(materialName.c_str(), flags))
                {
                    DrawTiles(tileNames, textureID, category, material);
                    ImGui::TreePop();
                }
            }
        }
        else
        {
            std::vector<std::string> allTiles;

            for (int m = 0; m < (int)eTileMaterial::COUNT; ++m)
            {
                eTileMaterial material = static_cast<eTileMaterial>(m);
                const auto& tileNames = AssetManager::GetTileNamesByCategoryAndMaterial(category, material);
                allTiles.insert(allTiles.end(), tileNames.begin(), tileNames.end());
            }

            DrawTiles(allTiles, textureID, category, eTileMaterial::None);
        }

    }
    void TileEditorPanel::DrawTiles(const std::vector<std::string>& tileNames, ImTextureID textureID, eTileCategory category, eTileMaterial /*material*/)
    {
        const float iconSize = 32.0f;
        const int itemsPerRow = 8;

        for (size_t i = 0; i < tileNames.size(); ++i)
        {
            const std::string& name = tileNames[i];
            const glm::vec4 uv = GetTileUV(name);

            ImGui::PushID((int)i);
            if (ImGui::ImageButton("##tileIcon", textureID, ImVec2(iconSize, iconSize), ImVec2(uv.x, uv.y), ImVec2(uv.z, uv.w)))
            {
                m_selectedTileName = name;
                m_selectedTileCategory = category;

                // If not already modified, copy from AssetManager (or default)
                if (m_modifiedTileProperties.find(name) == m_modifiedTileProperties.end())
                    m_modifiedTileProperties[name] = AssetManager::GetTileProperties(name);

                m_selectedTileprops = m_modifiedTileProperties[name];
                m_selectedTileMaterial = m_selectedTileprops.material;
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