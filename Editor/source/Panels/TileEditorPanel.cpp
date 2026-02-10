#include "pch.h"
#include "TileEditorPanel.h"

#include <Engine/Debug/Instrumentor.h>

#include <imgui.h>
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
            { "Vehicles",  eTileCategory::Vehicles },
            { "dynamicObjects",  eTileCategory::dynamicObjects
 }
        };

        // Thumbnail metrics for iso tiles (height = width * 256/128)
        const float iconW = 48.0f; // tweak if you want bigger/smaller thumbs
        const float iconH = iconW * (float)TILE_PIXEL_HEIGHT / (float)TILE_PIXEL_WIDTH; // 2x iconW for 128x256
        const float spacingX = ImGui::GetStyle().ItemSpacing.x;
        const float spacingY = ImGui::GetStyle().ItemSpacing.y;

        for (const auto& entry : categories)
        {
            std::string id = std::string(entry.name) + std::to_string((int)entry.category);
            bool open = ImGui::TreeNodeEx(id.c_str(), ImGuiTreeNodeFlags_DefaultOpen, "%s", entry.name);

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                m_selectedTileCategory = entry.category;

            if (open)
            {
                // Compute how many items fit per row with current panel width
                float availW = ImGui::GetContentRegionAvail().x;
                int itemsPerRow = std::max(1, (int)std::floor((availW + spacingX) / (iconW + spacingX)));

                int tileCount = AssetManager::GetTileCountForCategory(entry.category);
                int rows = (tileCount + itemsPerRow - 1) / itemsPerRow;

                // Each item roughly: iconH + label height + spacing
                float labelH = ImGui::GetTextLineHeightWithSpacing();
                float perItemH = iconH + labelH + spacingY;

                float childHeight = rows * perItemH + 6.0f; // small padding

                ImGui::BeginChild(("Palette_" + std::string(entry.name)).c_str(), ImVec2(0, childHeight), true);
                // Uses your existing drawer (no new functions introduced)
                DrawTilePalette(entry.category);
                ImGui::EndChild();
                ImGui::TreePop();
            }
        }
        ImGui::End();

        if (!m_selectedTileName.empty())
        {
            ImGui::Begin("Tile Properties");

            // Initialize defaults if uninitialized (keep your original logic)
            if (m_selectedTileprops.health == 0 && m_selectedTileprops.material == eTileMaterial::None)
            {
                auto it = m_tilePropertyDefaults.find(m_selectedTileName);
                if (it != m_tilePropertyDefaults.end())
                    m_selectedTileprops = it->second;
            }

            ImGui::Text("Editing: %s", m_selectedTileName.c_str());

            bool tilePropertiesChanged = false;

            if (ImGui::InputScalar("Health", ImGuiDataType_U32, &m_selectedTileprops.health))
                tilePropertiesChanged = true;

            static const char* materialOptions[] = {
                ToString(eTileMaterial::Undefined),
                ToString(eTileMaterial::Default),
                ToString(eTileMaterial::None),
                ToString(eTileMaterial::Wood),
                ToString(eTileMaterial::Concrete),
                ToString(eTileMaterial::Steel),
                ToString(eTileMaterial::Stone),
                ToString(eTileMaterial::Plastic),
                ToString(eTileMaterial::Metal),
                ToString(eTileMaterial::Glass)
            };
            constexpr int materialCount = sizeof(materialOptions) / sizeof(materialOptions[0]);

            int currentMaterialIdx = static_cast<int>(m_selectedTileprops.material);
            if (ImGui::Combo("Material", &currentMaterialIdx, materialOptions, materialCount))
            {
                m_selectedTileprops.material = static_cast<eTileMaterial>(currentMaterialIdx);
                tilePropertiesChanged = true;
            }

            if (tilePropertiesChanged)
            {
                m_selectedTileprops.name = m_selectedTileName;
                m_modifiedTileProperties[m_selectedTileName] = m_selectedTileprops;
                TileSerializer::Save(m_modifiedTileProperties);
            }

            ImGui::End();
        }
    }



    void TileEditorPanel::DrawTilePalette(eTileCategory category)
    {
        ImTextureID textureID = (ImTextureID)AssetManager::GetTileTextureIconAtlas()->GetTextureDescriptor();

        if (category == eTileCategory::Buildings)
        {
            for (int m = 0; m < (int)eTileMaterial::COUNT; ++m)
            {
                eTileMaterial material = static_cast<eTileMaterial>(m);

                const auto& tileNames = AssetManager::GetTileNamesByCategoryAndMaterial(category, material);
                if (tileNames.empty()) continue;

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
                std::string materialName = GetTileMaterialName(material);

                if (ImGui::TreeNodeEx(materialName.c_str(), flags))
                {
                    DrawTiles(tileNames, textureID, category, material);
                    ImGui::TreePop();
                }

                ImGui::Dummy(ImVec2(0.0f, 4.0f));
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

    void TileEditorPanel::DrawTiles(const std::vector<std::string>& tileNames,
        ImTextureID textureID,
        eTileCategory category,
        eTileMaterial /*material*/)
    {
        // Iso thumbnails: keep 128x256 aspect -> H = W * 256/128
        const float iconW = 48.0f;
        const float iconH = iconW * (float)TILE_PIXEL_HEIGHT / (float)TILE_PIXEL_WIDTH; // 2 * iconW
        const float spacingX = ImGui::GetStyle().ItemSpacing.x;

        // Compute items per row from available width inside the current region/child
        const float availW = ImGui::GetContentRegionAvail().x;
        const int itemsPerRow = std::max(1, (int)std::floor((availW + spacingX) / (iconW + spacingX)));

        for (size_t i = 0; i < tileNames.size(); ++i)
        {
            const std::string& name = tileNames[i];
            const glm::vec4 uv = GetTileUV(name);

            // Stable unique ID per tile (avoids collisions across multiple material sections)
            ImGui::PushID(name.c_str());

            if (ImGui::ImageButton("##tileIcon",
                textureID,
                ImVec2(iconW, iconH),
                ImVec2(uv.x, uv.y),
                ImVec2(uv.z, uv.w)))
            {
                m_selectedTileName = name;
                m_selectedTileCategory = category;

                if (m_modifiedTileProperties.find(name) == m_modifiedTileProperties.end())
                    m_modifiedTileProperties[name] = AssetManager::GetTileProperties(name);

                m_selectedTileprops = m_modifiedTileProperties[name];
                m_selectedTileMaterial = m_selectedTileprops.material;
            }

            if (m_selectedTileName == name)
            {
                ImVec2 pMin = ImGui::GetItemRectMin();
                ImVec2 pMax = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddRect(pMin, pMax, IM_COL32(255, 255, 0, 255), 0.0f, 0, 3.0f);
            }

            ImGui::PopID();

            // Wrap to next row
            if (((int)i + 1) % itemsPerRow != 0)
                ImGui::SameLine();
        }
    }







}