#include "pch.h"
#include "TileEditorPanel.h"

#include <Engine/Debug/Instrumentor.h>

#include <imgui.h>
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/AssetManager/Utils/TileSerializer.h"

namespace Engine {

    
    TileEditorPanel::TileEditorPanel(Ref<Scene> scene) : m_scene(scene)
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
            { "dynamicObjects",  eTileCategory::dynamicObjects},
            { "doors",  eTileCategory::Doors },
            { "windows",  eTileCategory::Windows }
        };

        // Thumbnail metrics for iso tiles (height = width * 256/128)
        const float iconW = 48.0f; // tweak if you want bigger/smaller thumbs
        const float iconH = iconW * (float)TILE_PIXEL_HEIGHT / (float)TILE_PIXEL_WIDTH; // 2x iconW for 128x256
        const float spacingX = ImGui::GetStyle().ItemSpacing.x;
        const float spacingY = ImGui::GetStyle().ItemSpacing.y;

        for (const auto& entry : categories)
        {
            std::string id = std::string(entry.name) + std::to_string((int)entry.category);
            bool open = ImGui::TreeNodeEx(id.c_str(), 0, "%s", entry.name);

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
                // 1. Keep property name synced with selected tile
                m_selectedTileprops.name = m_selectedTileName;

                // 2. Store modified props in editor-side map
                m_modifiedTileProperties[m_selectedTileName] = m_selectedTileprops;

                // 3. Save to disk
                TileSerializer::Save(m_modifiedTileProperties);

                // 4. Update live tile definition immediately
                TileDefinition* def = AssetManager::GetTileDefinitions().GetMutable(m_selectedTileTypeId);

                if (def)
                {
                    def->Name = m_selectedTileprops.name;
                    def->BaseHealth = m_selectedTileprops.health;
                    def->Material = static_cast<eTileMaterial>(currentMaterialIdx);

                 
                }
                TileSerializer::SaveTileDefinitions(AssetManager::GetTileDefinitions());
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
        const float iconW = 48.0f;
        const float iconH = iconW * (float)TILE_PIXEL_HEIGHT / (float)TILE_PIXEL_WIDTH;
        const float spacingX = ImGui::GetStyle().ItemSpacing.x;

        const float leftPadding = 1.0f;
        const float rightPadding = 30.0f;

        const float fullAvailW = ImGui::GetContentRegionAvail().x;
        const float usableW = fullAvailW - leftPadding - rightPadding;

        const int itemsPerRow = std::max(1, (int)std::floor((usableW + spacingX) / (iconW + spacingX)));

        for (size_t i = 0; i < tileNames.size(); ++i)
        {
            const int col = (int)(i % itemsPerRow);

            if (col == 0)
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + leftPadding);
            }
            else
            {
                ImGui::SameLine();
            }

            const std::string& name = tileNames[i];
            const glm::vec4 uv = GetTileUV(name);

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

                TileDefinitionRegistry& registry = AssetManager::GetTileDefinitions();

                m_selectedTileTypeId = registry.GetTypeIdByName(name);

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
        }
    }







}