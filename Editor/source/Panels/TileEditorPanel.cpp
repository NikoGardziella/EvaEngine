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

        const uint32_t tilePx = PIXELS_IN_TILE;
        const std::string TilePaletteName = "TilePalette.yaml";
        fs::path atlasPath = AssetManager::GetAssetFolderPath() / "textures" / "tiles" / TilePaletteName;

        m_tileNames.clear();
        m_tileUVMap.clear();

        // 1) Load previous UV data
        TilePaletteSerializer::Deserialize(atlasPath.string(), m_tileNames, m_tileUVMap);

        // 2) Gather all tile PNGs
        std::vector<fs::path> pngFiles;
        uint32_t index = 0;
        const uint32_t maxFiles = 100;

        for (auto& p : fs::directory_iterator(Engine::AssetManager::GetAssetPath("textures/tiles").string()))
        {
            if (p.path().extension() == ".png")
            {
                pngFiles.push_back(p.path());
                if (++index >= maxFiles)
                {
                    EE_CORE_WARN("Max tile files reached: {}", index);
                    break;
                }
            }
        }

        if (pngFiles.empty())
        {
            EE_CORE_WARN("No tile PNGs found");
            return;
        }

        // 3) Track existing tiles
        std::unordered_map<std::string, size_t> existingTilesMap;
        for (size_t i = 0; i < m_tileNames.size(); ++i)
            existingTilesMap[m_tileNames[i]] = i;

        size_t uniqueTileCount = m_tileNames.size();

        // 4) Identify new tiles
        std::vector<fs::path> newTilesToAdd;
        for (const auto& file : pngFiles)
        {
            std::string tileName = file.stem().string();
            if (existingTilesMap.find(tileName) == existingTilesMap.end())
            {
                newTilesToAdd.push_back(file);
                ++uniqueTileCount;
            }
        }

        // 5) Compute atlas size
        uint32_t cols = uint32_t(std::ceil(std::sqrt((float)uniqueTileCount)));
        uint32_t rows = uint32_t(std::ceil(uniqueTileCount / float(cols)));
        uint32_t atlasW = cols * tilePx;
        uint32_t atlasH = rows * tilePx;

        std::vector<uint8_t> atlasData(atlasW * atlasH * 4, 0);

        // 6) Copy existing tiles to atlas
        for (size_t i = 0; i < m_tileNames.size(); ++i)
        {
            const std::string& tileName = m_tileNames[i];
            const glm::vec4& uv = m_tileUVMap[tileName];
            uint32_t col = uint32_t(uv.x * atlasW) / tilePx;
            uint32_t row = uint32_t(uv.y * atlasH) / tilePx;

            std::string filePath = (AssetManager::GetAssetFolderPath() / "textures" / "tiles" / (tileName + ".png")).string();
            int w, h, channels;
            stbi_uc* pixels = stbi_load(filePath.c_str(), &w, &h, &channels, STBI_rgb_alpha);
            if (!pixels)
            {
                EE_CORE_WARN("Failed to load tile: {}", filePath);
                continue;
            }

            for (uint32_t y = 0; y < tilePx; ++y)
            {
                for (uint32_t x = 0; x < tilePx; ++x)
                {
                    size_t srcIdx = (y * tilePx + x) * 4;
                    size_t dstX = col * tilePx + x;
                    size_t dstY = row * tilePx + y;
                    size_t dstIdx = (dstY * atlasW + dstX) * 4;
                    memcpy(&atlasData[dstIdx], &pixels[srcIdx], 4);
                }
            }

            stbi_image_free(pixels);
        }

        // 7) Copy new tiles and assign new UVs
        for (size_t i = 0; i < newTilesToAdd.size(); ++i)
        {
            const auto& file = newTilesToAdd[i];
            int w, h, channels;
            stbi_uc* pixels = stbi_load(file.string().c_str(), &w, &h, &channels, STBI_rgb_alpha);
            EE_CORE_ASSERT(pixels && w == int(tilePx) && h == int(tilePx), "Tile load failed or wrong size");

            uint32_t tileIndex = uint32_t(m_tileNames.size());
            uint32_t col = tileIndex % cols;
            uint32_t row = tileIndex / cols;

            for (uint32_t y = 0; y < tilePx; ++y)
            {
                for (uint32_t x = 0; x < tilePx; ++x)
                {
                    size_t srcIdx = (y * tilePx + x) * 4;
                    size_t dstX = col * tilePx + x;
                    size_t dstY = row * tilePx + y;
                    size_t dstIdx = (dstY * atlasW + dstX) * 4;
                    memcpy(&atlasData[dstIdx], &pixels[srcIdx], 4);
                }
            }

            stbi_image_free(pixels);

            std::string tileName = file.stem().string();
            m_tileNames.push_back(tileName);

            float u0 = float(col * tilePx) / float(atlasW);
            float v0 = float(row * tilePx) / float(atlasH);
            float u1 = float((col + 1) * tilePx) / float(atlasW);
            float v1 = float((row + 1) * tilePx) / float(atlasH);

            glm::vec4 uv = { u0, v0, u1, v1 };
            m_tileUVMap[tileName] = uv;
        }

        // 8) Upload atlas to GPU
        m_tileTextureIconAtlas = std::make_shared<VulkanTexture>(atlasW, atlasH, "tilePalette", true);
        VulkanUtils::TransitionImageLayout(m_tileTextureIconAtlas->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        m_tileTextureIconAtlas->SetData(atlasData.data(), static_cast<uint32_t>(atlasData.size()));
        m_tileTextureIconAtlas->SetPixelData(atlasData);
        AssetManager::AddTextureToCache("tilePalette", m_tileTextureIconAtlas);

        // 9) Save updated UV map
        TilePaletteSerializer::Serialize(atlasPath.string(), m_tileNames, m_tileUVMap);
        EE_CORE_INFO("Tile atlas created at {}", atlasPath.string());
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