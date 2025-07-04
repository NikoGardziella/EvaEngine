#include "pch.h"
#include "TileEditorPanel.h"

#include <Engine/Debug/Instrumentor.h>
#include <Engine/AssetManager/AssetManager.h>
#include "Engine/Platform/Vulkan/VulkanUtils.h"

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


   
    // Call this once at startup
    void TileEditorPanel::CreateTileAtlas()
    {
        EE_PROFILE_FUNCTION();

        namespace fs = std::filesystem;

        uint32_t tilePx = PIXELS_IN_TILE;

        // 1) Gather all PNG paths
        std::vector<fs::path> files;
        uint32_t max_files = 100;
        uint32_t index = 0;

        for (auto& p : fs::directory_iterator(Engine::AssetManager::GetAssetPath("textures/tiles").string()))
        {
            if (p.path().extension() == ".png")
            {
                files.push_back(p.path());
                index++;
            }
            if (index >= max_files)
            {
                EE_CORE_WARN("max tile files reached: {}", index);
                break;
            }
        }

        if (index <= 0)
        {
            EE_CORE_WARN("no tile files found");
            return;
        }

        // We'll prepare a new vector for tile names (in correct order) and a vector for tile UVs
        std::vector<std::string> newTileNames;
        std::vector<glm::vec4> newTileUVs;

        // 2) First, preserve existing UVs and tile names (to keep stable order)
        // We'll build a map from tile name to pixel index (in atlas)
        std::unordered_map<std::string, size_t> existingTilesMap;
        for (size_t i = 0; i < m_tileNames.size(); ++i)
        {
            existingTilesMap[m_tileNames[i]] = i;
        }

        // 3) Determine the total tile count (existing + new unique tiles)
        size_t uniqueTilesCount = m_tileNames.size();

        // 4) Identify new tiles (not in existing map)
        std::vector<fs::path> newFilesToAdd;
        for (const auto& file : files)
        {
            std::string tileName = file.filename().string();
            const std::string extension = ".png";
            if (tileName.size() >= extension.size() &&
                tileName.compare(tileName.size() - extension.size(), extension.size(), extension) == 0)
            {
                tileName = tileName.substr(0, tileName.size() - extension.size());
            }

            if (existingTilesMap.find(tileName) == existingTilesMap.end())
            {
                // This is a new tile
                newFilesToAdd.push_back(file);
                uniqueTilesCount++;
            }
        }

        // 5) Compute grid size for the atlas based on total unique tiles
        uint32_t cols = uint32_t(std::ceil(std::sqrt((float)uniqueTilesCount)));
        uint32_t rows = uint32_t(std::ceil(uniqueTilesCount / float(cols)));

        uint32_t atlasW = cols * tilePx;
        uint32_t atlasH = rows * tilePx;

        // 6) Create CPU-side buffer for atlas pixels
        std::vector<uint8_t> atlasData(atlasW * atlasH * 4, 0);

        // 7) Copy existing tiles into atlas first, preserving their UVs
        for (size_t i = 0; i < m_tileNames.size(); ++i)
        {
            const std::string& tileName = m_tileNames[i];
            glm::vec4 uv = m_tileUVMap[tileName];

            // Calculate pixel rect from UV
            uint32_t col = uint32_t(uv.x * atlasW);
            uint32_t row = uint32_t(uv.y * atlasH);

            // Load pixels from old tile texture atlas? (You need to store old pixel data)
            // If you have previous atlas pixel data, copy the tile pixels to new atlasData here.
            // For now, assume you keep old pixel data somewhere accessible as m_tileTextureIconAtlasPixelData
            // Otherwise, you'd need to reload original tile image from disk or keep pixels cached.

            // TODO: Implement copying pixels from old atlas texture or original tile image
            // This is critical for preserving old tiles in the new atlas.
        }

        // 8) Append new tiles to atlas, assign new UVs
        for (size_t i = 0; i < newFilesToAdd.size(); ++i)
        {
            const auto& file = newFilesToAdd[i];

            int w, h, channels;
            stbi_uc* pixels = stbi_load(file.string().c_str(), &w, &h, &channels, STBI_rgb_alpha);
            EE_CORE_ASSERT(pixels && w == int(tilePx) && h == int(tilePx), "Tile load failed or wrong size");

            uint32_t tileIndex = uint32_t(m_tileNames.size() + i);
            uint32_t col = tileIndex % cols;
            uint32_t row = tileIndex / cols;

            // Copy pixels into atlasData
            for (uint32_t y = 0; y < tilePx; ++y)
            {
                for (uint32_t x = 0; x < tilePx; ++x)
                {
                    // I flipped the textures here because they are rendered upside down
					// maybe there is a better way to do this?
                    // doing it in shader messes up the UVs
                    // could be done in UV ?
                    // in rendering pipeline ?
                    bool flipX = false;
                    bool flipY = false;

                    uint32_t fx = flipX ? (tilePx - 1 - x) : x;
                    uint32_t fy = flipY ? (tilePx - 1 - y) : y;
                    size_t srcIdx = (fy * tilePx + fx) * 4;
                    size_t dstX = col * tilePx + x;
                    size_t dstY = row * tilePx + y;
                    size_t dstIdx = (dstY * atlasW + dstX) * 4;

                    memcpy(&atlasData[dstIdx], &pixels[srcIdx], 4);
                }
            }
            stbi_image_free(pixels);

            std::string tileName = file.filename().string();
            if (tileName.size() >= 4 && tileName.compare(tileName.size() - 4, 4, ".png") == 0)
                tileName = tileName.substr(0, tileName.size() - 4);

            newTileNames.push_back(tileName);

            // Compute UVs in [0,1]
            float u0 = float(col * tilePx) / float(atlasW);
            float v0 = float(row * tilePx) / float(atlasH);
            float u1 = float((col + 1) * tilePx) / float(atlasW);
            float v1 = float((row + 1) * tilePx) / float(atlasH);

            // Swap u0 and u1 to flip horizontally
            glm::vec4 uv = { u0, v0, u1, v1 }; // <-- flipped horizontally

            newTileUVs.push_back(uv);
        }

        // 9) Update m_tileNames and m_tileUVMap by appending new tiles
        for (size_t i = 0; i < newTileNames.size(); ++i)
        {
            m_tileNames.push_back(newTileNames[i]);
            m_tileUVMap[newTileNames[i]] = newTileUVs[i];
        }

        // 10) Create VulkanTexture and upload atlas data
        m_tileTextureIconAtlas = std::make_shared<VulkanTexture>(atlasW, atlasH, "tilePalette", true);
        VulkanUtils::TransitionImageLayout(m_tileTextureIconAtlas->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        m_tileTextureIconAtlas->SetData(atlasData.data(), uint32_t(atlasData.size()));
        m_tileTextureIconAtlas->SetPixelData(atlasData);
        AssetManager::AddTextureToCache("tilePalette", m_tileTextureIconAtlas);
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