#include "pch.h"
#include "TileEditorPanel.h"

#include <Engine/Debug/Instrumentor.h>
#include <Engine/AssetManager/AssetManager.h>
#include "Engine/Platform/Vulkan/VulkanUtils.h"
#include "Utils/TilePaletteSerializer.h"

#include <stb_image/stb_image.h>
#include <imgui/imgui.h>



namespace Engine {

    TileEditorPanel::TileEditorPanel()
    {
        CreateTileAtlas();

        

    }


    void TileEditorPanel::OnImGuiRender()
    {
       EE_PROFILE_FUNCTION();

       DrawTilePalette();
       
    
    }

    void TileEditorPanel::CreateTileAtlas()
    {
        EE_PROFILE_FUNCTION();

        namespace fs = std::filesystem;

        const std::string TilePaletteName = "TilePalette.yaml";
        fs::path atlasPath = AssetManager::GetAssetFolderPath() / "textures" / "tiles" / TilePaletteName;

        m_tileNames.clear();
        m_tileUVMap.clear();

        TilePaletteSerializer::Deserialize(atlasPath.string(), m_tileNames, m_tileUVMap);

        std::vector<fs::path> files;
        for (auto& p : fs::directory_iterator(AssetManager::GetAssetPath("textures/tiles")))
        {
            if (p.path().extension() == ".png")
                files.push_back(p.path());
        }

        if (files.empty())
        {
            EE_CORE_WARN("No tile images found.");
            return;
        }

        // Sort tiles by height (tallest first) for shelf-packing
        struct TileInfo { fs::path path; int width, height; stbi_uc* pixels; };
        std::vector<TileInfo> loadedTiles;
        int atlasWidth = 1024;
        int currentX = 0, currentY = 0, rowHeight = 0;

        for (const auto& file : files)
        {
            int w, h, channels;
            stbi_uc* pixels = stbi_load(file.string().c_str(), &w, &h, &channels, STBI_rgb_alpha);
            if (!pixels)
            {
                EE_CORE_WARN("Failed to load tile {}", file.string());
                continue;
            }

            // Start new row if needed
            if (currentX + w > atlasWidth)
            {
                currentY += rowHeight;
                currentX = 0;
                rowHeight = 0;
            }

            TileInfo tile = { file, w, h, pixels };
            loadedTiles.push_back(tile);

            currentX += w;
            rowHeight = std::max(rowHeight, h);
        }

        int atlasHeight = currentY + rowHeight;
        std::vector<uint8_t> atlasData(atlasWidth * atlasHeight * 4, 0);

        currentX = 0;
        currentY = 0;
        rowHeight = 0;
        m_tileNames.clear();
        m_tileUVMap.clear();

        for (const auto& tile : loadedTiles)
        {
            const int w = tile.width;
            const int h = tile.height;

            if (currentX + w > atlasWidth)
            {
                currentY += rowHeight;
                currentX = 0;
                rowHeight = 0;
            }

            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    size_t srcIdx = (y * w + x) * 4;
                    size_t dstX = currentX + x;
                    size_t dstY = currentY + y;
                    size_t dstIdx = (dstY * atlasWidth + dstX) * 4;

                    memcpy(&atlasData[dstIdx], &tile.pixels[srcIdx], 4);
                }
            }

            std::string name = tile.path.filename().stem().string();
            m_tileNames.push_back(name);

            float u0 = float(currentX) / float(atlasWidth);
            float v0 = float(currentY) / float(atlasHeight);
            float u1 = float(currentX + w) / float(atlasWidth);
            float v1 = float(currentY + h) / float(atlasHeight);
            m_tileUVMap[name] = glm::vec4(u0, v0, u1, v1);

            currentX += w;
            rowHeight = std::max(rowHeight, h);
            stbi_image_free(tile.pixels);
        }

        m_tileTextureIconAtlas = std::make_shared<VulkanTexture>(atlasWidth, atlasHeight, "tilePalette", true);
        VulkanUtils::TransitionImageLayout(m_tileTextureIconAtlas->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        m_tileTextureIconAtlas->SetData(atlasData.data(), uint32_t(atlasData.size()));
        m_tileTextureIconAtlas->SetPixelData(atlasData);
        AssetManager::AddTextureToCache("tilePalette", m_tileTextureIconAtlas);

        TilePaletteSerializer::Serialize(atlasPath.string(), m_tileNames, m_tileUVMap);
        EE_CORE_INFO("Created tile atlas with dimensions {}x{}", atlasWidth, atlasHeight);
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
            ImTextureID textureID = (ImTextureID)m_tileTextureIconAtlas->GetTextureDescriptor();

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