#include "pch.h"
#include "TextureStreamingSystem.h"
#include "TextureStreamingUtils.h"
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include "Engine/Platform/Vulkan/VulkanUtils.h"
#include <Engine/Scene/Components/Render/ChunkRendererComponent.h>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include "Engine/Map/Utils/IsoTileUtils.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Entity.h"
#include <utility>
#include <Engine/Map/Utils/IVec2Hasher.h>
#include <Engine/Core/Assert.h>
#include <lz4hc.c>
#include <filesystem>


namespace Engine {


	TextureStreamingSystem::TextureStreamingSystem()
    {
        std::filesystem::create_directories(m_chunkCachePath);
    }

    TextureStreamingSystem::~TextureStreamingSystem()
    {

    }
    void TextureStreamingSystem::Update(const glm::vec2& playerPos, Scene* scene)
    {
        EE_PROFILE_FUNCTION();

        bool chunksPackedDirty = false;
        glm::ivec2 playerChunk = glm::ivec2(glm::floor(playerPos / float(CHUNK_SIZE)));

        if (m_chunkMap.empty()) return;

      
        int transitions = 0;

        for (auto& [id, chunk] : m_chunkMap)
        {
            if (transitions >= MAX_TRANSITIONS) break;

            glm::ivec2 chunkCoords = chunk.ChunkCoords;
            int dist = std::max(
                glm::abs(chunkCoords.x - playerChunk.x),
                glm::abs(chunkCoords.y - playerChunk.y));

            switch (chunk.Residency)
            {
            case ChunkResidency::GPU:
                if (dist > UNLOAD_RADIUS)
                {
                    UnloadChunkFromGPU(chunk, scene);
                    chunk.Residency = ChunkResidency::CPU;
                    chunksPackedDirty = true;
                    transitions++;

                    if (dist > DISK_RADIUS)
                    {
                        FlushChunkToDisk(chunk);
                        chunk.Residency = ChunkResidency::Disk;
                        transitions++;
                    }
                }
                break;

            case ChunkResidency::CPU:
                if (dist <= LOAD_RADIUS)
                {
                    LoadChunkToGPU(chunk, scene);
                    chunk.Residency = ChunkResidency::GPU;
                    chunksPackedDirty = true;
                    transitions++;
                }
                else if (dist > DISK_RADIUS)
                {
                    FlushChunkToDisk(chunk);
                    chunk.Residency = ChunkResidency::Disk;
                    transitions++;
                }
                break;

            case ChunkResidency::Disk:
                if (dist <= LOAD_RADIUS)
                {
                    LoadChunkFromDisk(chunk, scene);
                    chunk.Residency = ChunkResidency::CPU;
                    transitions++;

                    if (transitions < MAX_TRANSITIONS)
                    {
                        LoadChunkToGPU(chunk, scene);
                        chunk.Residency = ChunkResidency::GPU;
                        chunksPackedDirty = true;
                        transitions++;
                    }
                }
                else if (dist <= UNLOAD_RADIUS)
                {
                    LoadChunkFromDisk(chunk, scene);
                    chunk.Residency = ChunkResidency::CPU;
                    transitions++;
                }
                break;
            }
        }

        if (chunksPackedDirty)
        {
            SortChunksRowMajor(scene);
        }
    }

    void TextureStreamingSystem::UnloadAllChunks(Scene* scene)
    {
        for (auto& [id, chunk] : m_chunkMap)
        {
            UnloadChunkFromGPU(chunk, scene);
        }


        m_chunkMap.clear();
        m_chunkMap.rehash(0);
    }

    void TextureStreamingSystem::UploadToChunkFromTexture(const glm::vec2& worldPosition,
        UUID id, const std::string& name, const std::vector<uint8_t>& textureData,
        const std::vector<uint8_t>& propertiesData,   // RGBA8UI: R=health, G=height, B=mask/effect scratch, A=[category:flags]
        uint32_t textureWidth, uint32_t textureHeight)
    {
        EE_PROFILE_FUNCTION();

        const int CELL = int(TILE_PIXEL_WIDTH);
        const int chunkWpx = int(CHUNK_SIZE) * CELL;
        const int chunkHpx = chunkWpx;

        const int groundPxX = int(std::floor(worldPosition.x * float(CELL)));
        const int groundPxY = int(std::floor(worldPosition.y * float(CELL)));

        const int destX0_global = groundPxX - int(textureWidth) / 2;
        const int destY0_global = groundPxY - 1;
        const int dstX1_global = destX0_global + int(textureWidth);
        const int dstY1_global = destY0_global + int(textureHeight);

        const int minChunkX = TextureStreamingUtils::FloorDiv(destX0_global, chunkWpx);
        const int maxChunkX = TextureStreamingUtils::FloorDiv(dstX1_global - 1, chunkWpx);
        const int minChunkY = TextureStreamingUtils::FloorDiv(destY0_global, chunkHpx);
        const int maxChunkY = TextureStreamingUtils::FloorDiv(dstY1_global - 1, chunkHpx);

        if (!textureData.empty())
        {
            EE_CORE_ASSERT(textureData.size() >= size_t(textureWidth) * textureHeight * 4, "textureData too small");
        }
        if (!propertiesData.empty())
        {
            EE_CORE_ASSERT(propertiesData.size() >= size_t(textureWidth) * textureHeight * 4, "propertiesData too small");
        }

        for (int cy = minChunkY; cy <= maxChunkY; ++cy)
        {
            for (int cx = minChunkX; cx <= maxChunkX; ++cx)
            {
                const glm::ivec2 chunkCoords(cx, cy);
                TextureChunk& chunk = m_chunkMap[HashCoords(chunkCoords)];
                chunk.TextureCount += 1;

                const size_t totalPixels = size_t(chunkWpx) * size_t(chunkHpx);

                if (!textureData.empty() && chunk.PixelData.empty())
                {
                    chunk.PixelData.assign(totalPixels * 4, 0);
                }
                if (!propertiesData.empty() && chunk.PropertiesData.empty())
                {
                    chunk.PropertiesData.assign(totalPixels * 4, 0);
                }

                if (chunk.Width == 0 || chunk.Height == 0)
                {
                    chunk.Width = uint32_t(chunkWpx);
                    chunk.Height = uint32_t(chunkHpx);
                    chunk.IsLoaded = false;
                    chunk.Name = "Chunk_" + std::to_string(cx) + "_" + std::to_string(cy);
                    chunk.AssetName = name;
                    chunk.ID = HashCoords(chunkCoords);
                    chunk.ChunkCoords = chunkCoords;
                }
                else
                {
                    EE_CORE_ASSERT(chunk.Width == uint32_t(chunkWpx), "Chunk width mismatch");
                    EE_CORE_ASSERT(chunk.Height == uint32_t(chunkHpx), "Chunk height mismatch");
                }

                const int chunkX0 = cx * chunkWpx;
                const int chunkY0 = cy * chunkHpx;
                const int chunkX1 = chunkX0 + chunkWpx;
                const int chunkY1 = chunkY0 + chunkHpx;

                const int left = std::max(destX0_global, chunkX0);
                const int right = std::min(dstX1_global, chunkX1);
                const int top = std::max(destY0_global, chunkY0);
                const int bottom = std::min(dstY1_global, chunkY1);
                if (left >= right || top >= bottom)
                {
                    continue;
                }

                bool wroteColor = false;
                bool wroteProps = false;

                for (int dstY = top; dstY < bottom; ++dstY)
                {
                    const int srcY = dstY - destY0_global;
                    if ((unsigned)srcY >= textureHeight)
                    {
                        continue;
                    }

                    const int dstY_inChunk = dstY - chunkY0;
                    const size_t dstRow = size_t(dstY_inChunk) * size_t(chunkWpx) * 4;
                    const size_t srcRow = size_t(srcY) * size_t(textureWidth) * 4;

                    for (int dstX = left; dstX < right; ++dstX)
                    {
                        const int srcX = dstX - destX0_global;
                        if ((unsigned)srcX >= textureWidth)
                        {
                            continue;
                        }

                        const int dstX_inChunk = dstX - chunkX0;

                        const size_t si = srcRow + size_t(srcX) * 4;
                        const size_t di = dstRow + size_t(dstX_inChunk) * 4;

                        // ---------- COLOR ----------
                        if (!textureData.empty())
                        {
                            uint8_t& dR = chunk.PixelData[di + 0];
                            uint8_t& dG = chunk.PixelData[di + 1];
                            uint8_t& dB = chunk.PixelData[di + 2];
                            uint8_t& dA = chunk.PixelData[di + 3];

                            const uint8_t sR = textureData[si + 0];
                            const uint8_t sG = textureData[si + 1];
                            const uint8_t sB = textureData[si + 2];
                            const uint8_t sA = textureData[si + 3];

                            wroteColor |= TextureStreamingUtils::AlphaOver(sR, sG, sB, sA, dR, dG, dB, dA);
                        }

                        // ---------- PROPERTIES ----------
                        if (!propertiesData.empty())
                        {
                            uint8_t& dPr = chunk.PropertiesData[di + 0]; // Health
                            uint8_t& dPg = chunk.PropertiesData[di + 1]; // Height (rows above pivot)
                            uint8_t& dPb = chunk.PropertiesData[di + 2]; // Mask / effect scratch
                            uint8_t& dPa = chunk.PropertiesData[di + 3]; // A = [category:flags]

                            const uint8_t sPr = propertiesData[si + 0];
                            const uint8_t sPg = propertiesData[si + 1];
                            const uint8_t sPb = propertiesData[si + 2];
                            const uint8_t sPa = propertiesData[si + 3];

                            const uint8_t sAcov = textureData.empty() ? 255 : textureData[si + 3];

                            wroteProps |= TextureStreamingUtils::MergePropertiesPixel(
                                sPr, sPg, sPb, sPa, sAcov,
                                dPr, dPg, dPb, dPa
                            );
                        }
                    }
                }

                if (wroteColor || wroteProps)
                {
                    chunk.IsDirty = true;
                }
            }
        }
            
    }

   
    void TextureStreamingSystem::UploadTerrainToChunkFromTexture(glm::vec2& worldPosition, UUID id, std::string name,
        const std::vector<uint8_t>& textureData, uint32_t textureWidth, uint32_t textureHeight)
    {
        EE_PROFILE_FUNCTION();

        const int CELL_W = int(TILE_PIXEL_WIDTH);
       // const int CELL_H = int(TILE_PIXEL_WIDTH); // this is intended 
        const int CELL_H = int(TILE_PIXEL_WIDTH);
        const int chunkWpx = int(CHUNK_SIZE) * CELL_W;
        const int chunkHpx = int(CHUNK_SIZE) * CELL_H;


   


        const int groundPxX = floor(worldPosition.x * CELL_W);
        const int groundPxY = floor(worldPosition.y * CELL_H);


        const int destX0_global = groundPxX - int(textureWidth) / 2;
        const int destY0_global = groundPxY - int(textureHeight);
        const int dstX1_global = destX0_global + int(textureWidth);
        const int dstY1_global = destY0_global + int(textureHeight);
      


        auto floorDiv = [](int a, int b) {
            int q = a / b, r = a % b;
            if ((r != 0) && ((r > 0) != (b > 0))) --q;
            return q;
            };
        const int minChunkX = floorDiv(destX0_global, chunkWpx);
        const int maxChunkX = floorDiv(dstX1_global - 1, chunkWpx);
        const int minChunkY = floorDiv(destY0_global, chunkHpx);
        const int maxChunkY = floorDiv(dstY1_global - 1, chunkHpx);
        
        EE_CORE_ASSERT(textureData.size() >= size_t(textureWidth) * textureHeight * 4, "terrain textureData too small");
        for (int cy = minChunkY; cy <= maxChunkY; ++cy)
        {

            for (int cx = minChunkX; cx <= maxChunkX; ++cx)
            {
                const glm::ivec2 chunkCoords(cx, cy);
                TextureChunk& chunk = m_chunkMap[HashCoords(chunkCoords)];
                chunk.TextureCount += 1;


                const size_t totalPixels = size_t(chunkWpx) * size_t(chunkHpx);
                if (chunk.TerrainData.empty())
                {
                    chunk.TerrainData.assign(totalPixels * 4, 0);
                    if (chunk.Width == 0 || chunk.Height == 0)
                    {
                        chunk.Width = uint32_t(chunkWpx);
                        chunk.Height = uint32_t(chunkHpx);
                        chunk.IsLoaded = false;
                        chunk.Name = "Chunk_" + std::to_string(cx) + "_" + std::to_string(cy);
                        chunk.AssetName = name;
                        chunk.ID = HashCoords(chunkCoords);
                        chunk.ChunkCoords = chunkCoords;
                    }
                }

                const int chunkX0 = cx * chunkWpx;
                const int chunkY0 = cy * chunkHpx;
                const int chunkX1 = chunkX0 + chunkWpx;
                const int chunkY1 = chunkY0 + chunkHpx;

                const int left = std::max(destX0_global, chunkX0);
                const int right = std::min(dstX1_global, chunkX1);
                const int top = std::max(destY0_global, chunkY0);
                const int bottom = std::min(dstY1_global, chunkY1);
                if (left >= right || top >= bottom) continue;

                for (int dstY = top; dstY < bottom; ++dstY)
                {
                    const int srcY = dstY - destY0_global; // source row 0 = bottom
                    if (srcY < 0 || srcY >= int(textureHeight)) continue;

                    const int dstY_inChunk = dstY - chunkY0;
                    const size_t dstRow = size_t(dstY_inChunk) * size_t(chunkWpx) * 4;
                    const size_t srcRow = size_t(srcY) * size_t(textureWidth) * 4;

                    for (int dstX = left; dstX < right; ++dstX)
                    {
                        const int srcX = dstX - destX0_global;
                        if (srcX < 0 || srcX >= int(textureWidth)) continue;

                        const int dstX_inChunk = dstX - chunkX0;

                        const size_t si = srcRow + size_t(srcX) * 4;
                        const size_t di = dstRow + size_t(dstX_inChunk) * 4;

                        const uint8_t a = textureData[si + 3];
                        if (a == 0) continue; // skip transparent atlas background
                        if (a < chunk.TerrainData[di + 3]) continue; // don't overwrite more opaque pixels


                        chunk.TerrainData[di + 0] = textureData[si + 0];
                        chunk.TerrainData[di + 1] = textureData[si + 1];
                        chunk.TerrainData[di + 2] = textureData[si + 2];
                        chunk.TerrainData[di + 3] = a;
                    }
                }

                chunk.IsDirty = true;
            }
        }

    }


 

    void TextureStreamingSystem::SortChunksRowMajor(Scene* scene)
    {
        scene->GetRegistry().sort<ChunkRendererComponent>([](const ChunkRendererComponent& a,
            const ChunkRendererComponent& b)
            {
                // Put loaded chunks first
                if (a.IsLoaded != b.IsLoaded) return a.IsLoaded && !b.IsLoaded;

                // Then row-major by (y, x)
                if (a.ChunkCoords.y != b.ChunkCoords.y)
                    return a.ChunkCoords.y < b.ChunkCoords.y;
                return a.ChunkCoords.x < b.ChunkCoords.x;
            });
    }



    
    uint64_t TextureStreamingSystem::HashCoords(const glm::ivec2& coords)
    {
        constexpr int64_t OFFSET = int64_t(1) << 31;

        int64_t x = static_cast<int64_t>(coords.x) + OFFSET;
        int64_t y = static_cast<int64_t>(coords.y) + OFFSET;

        return (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(y);
    }

    void TextureStreamingSystem::FlipChunkHorizontally(TextureChunk& chunk)
    {
        uint32_t rowSize = chunk.Width * 4;
        std::vector<uint8_t> tempPixel(4);

        for (uint32_t y = 0; y < chunk.Height; ++y)
        {
            uint8_t* row = &chunk.PixelData[y * rowSize];

            for (uint32_t x = 0; x < chunk.Width / 2; ++x)
            {
                uint8_t* leftPixel = &row[x * 4];
                uint8_t* rightPixel = &row[(chunk.Width - 1 - x) * 4];

                // Swap left and right pixels
                std::memcpy(tempPixel.data(), leftPixel, 4);
                std::memcpy(leftPixel, rightPixel, 4);
                std::memcpy(rightPixel, tempPixel.data(), 4);
            }
        }
    }

    void TextureStreamingSystem::FlipChunkVertically(TextureChunk& chunk)
    {
        uint32_t rowSize = chunk.Width * 4; // 4 bytes per pixel
        std::vector<uint8_t> tempRow(rowSize);

        for (uint32_t y = 0; y < chunk.Height / 2; ++y)
        {
            uint8_t* rowTop = &chunk.PixelData[y * rowSize];
            uint8_t* rowBottom = &chunk.PixelData[(chunk.Height - 1 - y) * rowSize];

            std::memcpy(tempRow.data(), rowTop, rowSize);
            std::memcpy(rowTop, rowBottom, rowSize);
            std::memcpy(rowBottom, tempRow.data(), rowSize);
        }
    }



    void TextureStreamingSystem::LoadChunkToGPU(TextureChunk& chunk, Scene* scene)
    {
        EE_PROFILE_FUNCTION();


        constexpr int CHUNK_RES = CHUNK_SIZE; // Assuming square chunks
       // EE_CORE_INFO("Loading chunk at coords: {}, {}", chunk.ChunkCoords.x, chunk.ChunkCoords.y);

        
        if (!chunk.TerrainData.empty())
        {
            chunk.TerrainTexture = std::make_shared<VulkanTexture>(chunk.Width, chunk.Height, VK_FORMAT_R8G8B8A8_UNORM);

            VulkanUtils::TransitionImageLayout(chunk.TerrainTexture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            chunk.TerrainTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            chunk.TerrainTexture->SetData(chunk.TerrainData.data(), chunk.Height * chunk.Width * 4);
            EE_CORE_INFO("Draw chunk {},{} -> image={}",
                chunk.ChunkCoords.x, chunk.ChunkCoords.y,  (void*)chunk.TerrainTexture->GetImage());
        }




        auto chynkentityView = scene->GetRegistry().view<IDComponent, ChunkRendererComponent>();

        for (auto entity : chynkentityView)
        {

            auto [IDComp, chunkRendComp] = chynkentityView.get<IDComponent, ChunkRendererComponent>(entity);
            if (chunkRendComp.ChunkCoords == chunk.ChunkCoords)
            {
                //chunkRendComp.Texture = chunk.GPUTexture;
               // chunkRendComp.PropertiesTexture = chunk.PropertiesTexture;
              //  chunkRendComp.PropertiesTexture->SetCPUPixelData(std::move(chunk.PropertiesData));
                chunkRendComp.TerrainTexture = chunk.TerrainTexture;
                chunkRendComp.VisualEffectTexture = std::make_shared<VulkanTexture>(chunk.Width, chunk.Height, VK_FORMAT_R8G8B8A8_UNORM);
               
                /*
                VulkanUtils::TransitionImageLayout(chunkRendComp.VisualEffectTexture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                chunkRendComp.VisualEffectTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                */
                
                //chunkRendComp.VisualEffectTexture = std::make_shared<VulkanTexture>(1, 1, VK_FORMAT_R8G8B8A8_UINT);
                chunkRendComp.VisualEffectTexture->ResetData(); // set everything to 0 so nothing gets rendered at start
                
                chunkRendComp.IsLoaded = true;
                chunk.GPUTexture = nullptr;
                break;
            }

        }
        chunk.IsLoaded = true;
    }



    void TextureStreamingSystem::UnloadChunkFromGPU(TextureChunk& chunk, Scene* scene)
    {
        EE_PROFILE_FUNCTION();
        //EE_CORE_INFO("Unloading chunk at coords: {}, {}", chunk.ChunkCoords.x, chunk.ChunkCoords.y);

        auto entityView = scene->GetRegistry().view<IDComponent, SpriteRendererComponent>();

        for (auto entity : entityView)
        {

            auto [IDComp, spriteRendComp] = entityView.get<IDComponent, SpriteRendererComponent>(entity);
            if (IDComp.ID == chunk.ID)
            {
                spriteRendComp.Texture = nullptr;
                break;
            }

        }

        auto chynkentityView = scene->GetRegistry().view<IDComponent, ChunkRendererComponent>();

        for (auto entity : chynkentityView)
        {

            auto [IDComp, chunkRendComp] = chynkentityView.get<IDComponent, ChunkRendererComponent>(entity);
            if (IDComp.ID == chunk.ID)
            {

                // This Texture is still in s_VulkanData.TextureSlotIndex 
                // which means it would be rendered inside REcordCommands()
                // this will prevent it. Feels a bit crappy fix but lets see.
                //chunkRendComp.Texture->SetCheckCollision(false);

                chunkRendComp.Texture = nullptr;
                chunkRendComp.PropertiesTexture = nullptr;
                chunkRendComp.TerrainTexture = nullptr;
                chunkRendComp.VisualEffectTexture = nullptr;
                chunkRendComp.IsLoaded = false;
                break;
            }

        }
        chunk.GPUTexture = nullptr;
        chunk.IsLoaded = false;

    }

    void TextureStreamingSystem::ResetAllChunks(Scene* scene)
    {
        EE_PROFILE_FUNCTION();
        EE_CORE_INFO("Resetting all chunks (scheduled unload)...");


        // Phase 1: gather IDs of all loaded chunks
        std::vector<UUID> toUnload;
		uint32_t chunkCount = static_cast<uint32_t>(m_chunkMap.size());
        if (m_chunkMap.empty() || chunkCount <= 0)
        {
			EE_CORE_WARN("No chunks to unload.");
			return;
        }

        for (auto& [id, chunk] : m_chunkMap)
        {
            auto chynkentityView = scene->GetRegistry().view<IDComponent, ChunkRendererComponent>();

            for (auto entity : chynkentityView)
            {

                auto [IDComp, chunkRendComp] = chynkentityView.get<IDComponent, ChunkRendererComponent>(entity);
               
                chunkRendComp.Texture = nullptr;
                chunkRendComp.IsLoaded = false;
                break;
                

            }
            chunk.GPUTexture = nullptr;
            chunk.IsLoaded = false;
        }
		m_chunkMap.clear();
        
    }


    void TextureStreamingSystem::DebugDrawChunkOutlines(Scene* scene)
    {
        EE_PROFILE_FUNCTION();
        // 1) Find the player's position
        glm::vec2 playerPos{ 0.0f };
        auto playerView = scene->GetRegistry().view<TransformComponent, CharacterControllerComponent>();
        for (auto entity : playerView)
        {
            auto& xf = playerView.get<TransformComponent>(entity);
            playerPos = { xf.Translation.x, xf.Translation.y };
            break; // Assume only one player
        }

        constexpr float cs = float(CHUNK_SIZE);
        constexpr int DEBUG_RADIUS = 2;
       
      
        glm::ivec2 playerChunk = glm::floor(playerPos / cs);
        std::unordered_set<glm::ivec2, IVec2Hasher> loadedCoords;
        for (const auto& [coord, chunk] : m_chunkMap)
        {
            if (chunk.IsLoaded)
                loadedCoords.insert(chunk.ChunkCoords);
        }
        glm::vec2 halfChunkOffset = glm::vec2(cs * 0.5f);

        // 2) Draw nearby unloaded chunks in red
        for (int dy = -DEBUG_RADIUS; dy <= DEBUG_RADIUS; ++dy)
        {
            for (int dx = -DEBUG_RADIUS; dx <= DEBUG_RADIUS; ++dx)
            {
                const glm::ivec2 coords = playerChunk + glm::ivec2(dx, dy);

                if (loadedCoords.count(coords) > 0)
                    continue;

                // Center-based unit-rect transform (matches your green version)
                const glm::vec2 center = (glm::vec2(coords) + glm::vec2(0.5f)) * cs;
                glm::mat4 transform =
                    glm::translate(glm::mat4(1.0f), glm::vec3(center, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(cs, cs, 1.0f));

                const glm::vec4 color = glm::vec4(1, 0, 0, 1); // Red = not loaded
                Engine::VulkanRenderer2D::DrawLineRect(transform, color, -1);
            }
        }

        // 3) Draw loaded chunks in green if near player

        for (const auto& [coord, chunk] : m_chunkMap)
        {
            if (!chunk.IsLoaded) continue;

            const glm::ivec2 delta = chunk.ChunkCoords - playerChunk;
            if (abs(delta.x) > DEBUG_RADIUS || abs(delta.y) > DEBUG_RADIUS) continue;

            const glm::vec2 center = (glm::vec2(chunk.ChunkCoords) + glm::vec2(0.5f)) * cs;
            glm::mat4 debugChunkTransform = glm::translate(glm::mat4(1.0f), glm::vec3(center, 0.0f)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(cs, cs, 1.0f));

            glm::vec4 loadedChunkColor = glm::vec4({ 0, 1, 0, 1 });
            VulkanRenderer2D::DrawLineRect(debugChunkTransform, loadedChunkColor, -1);
        }

    }

     // terrain only
    void TextureStreamingSystem::BakeTilesIntoChunks(Scene* scene)
    {
        EE_PROFILE_FUNCTION();

        // Pass A: bake all TERRAIN tiles first
        scene->ForEach<TransformComponent, TileComponent>(
            [&](Engine::Entity entity, TransformComponent& transformComp, TileComponent& tileComp)
            {
                for (const TileInfo& tile : tileComp.tiles)
                {
                    


                    if (tile.Category != eTileCategory::Terrain)
                        continue;

                    const float yoffset = 1.0f;

                    glm::vec2 worldTilePos = glm::vec2(transformComp.Translation) + tile.position;
                    worldTilePos.y += yoffset;

                    std::vector<uint8_t> pixelData;
                 
                    int w = 0, h = 0;
                    if (!AssetManager::ExtractPixelsFromTilePallette(tile, pixelData, w, h))
                        continue;


                    
                    // If you also mark blocked subtiles, do it here
                    // m_gridMap->MarkBlockedSubtilesFromTexture(worldTilePos, pixelData, w, h);

                    UploadTerrainToChunkFromTexture(worldTilePos, tileComp.TileID,
                        tile.name, pixelData, static_cast<uint32_t>(w), static_cast<uint32_t>(h));


                    

                }
            }
        );

        
       

        // DebugMarkChunks();
    }

    void TextureStreamingSystem::SortIsoTilesByY(Scene* scene)
    {
        auto& reg = scene->GetRegistry();

        // A) Sort entities: higher Y first (draw earlier)
        reg.sort<TransformComponent>(
            [&reg](entt::entity a, entt::entity b)
            {
                const auto& ta = reg.get<TransformComponent>(a).Translation;
                const auto& tb = reg.get<TransformComponent>(b).Translation;

                if (ta.y != tb.y) {
                    return ta.y > tb.y; // DESC by Y
                }
                // stable tie-break by entity id

                using underlying = std::underlying_type_t<entt::entity>;
                return static_cast<underlying>(a) < static_cast<underlying>(b);

            }
        );

        // B) Sort tiles within each entity: higher ground Y first (draw earlier)
        auto view = reg.view<TileComponent, TransformComponent>();
        for (auto e : view)
        {
            auto& tileComp = view.get<TileComponent>(e);
            const auto& tr = view.get<TransformComponent>(e);

            std::stable_sort(tileComp.tiles.begin(), tileComp.tiles.end(),
                [&](const TileInfo& A, const TileInfo& B)
                {
                    const float yA = tr.Translation.y + A.position.y; // world Y of tile A’s ground
                    const float yB = tr.Translation.y + B.position.y; // world Y of tile B’s ground
                    return yA > yB;                                   // DESC by Y
                }
            );
        }
    }




    void TextureStreamingSystem::AddChunkEntitiesToRegistry(Scene* scene)
    {
        EE_PROFILE_FUNCTION();
        for (auto& [uuid, chunk] : m_chunkMap)
        {
            
            auto entity = scene->GetRegistry().create();
            auto& chunkRenderer = scene->GetRegistry().emplace<ChunkRendererComponent>(entity);
            
			
            IDComponent id;
            id.ID = HashCoords(chunk.ChunkCoords);
            IDComponent& idComp = scene->GetRegistry().emplace<IDComponent>(entity);
            idComp = id;

            chunkRenderer.Texture = chunk.GPUTexture;
            chunkRenderer.ChunkCoords = chunk.ChunkCoords;
			chunkRenderer.IsLoaded = false;

          
            //FlipChunkHorizontally(chunk);
           // FlipChunkVertically(chunk);

        }
		EE_CORE_INFO("Added {} chunk entities to registry", m_chunkMap.size());
    }

    void TextureStreamingSystem::FlushChunkToDisk(TextureChunk& chunk)
    {
        EE_PROFILE_FUNCTION();

        if (!chunk.IsDirtyPixels)
        {
            // Never modified — can reconstruct from BakeTilesIntoChunks later
            chunk.TerrainData.clear();
            chunk.TerrainData.shrink_to_fit();
            chunk.PixelData.clear();
            chunk.PixelData.shrink_to_fit();
            chunk.PropertiesData.clear();
            chunk.PropertiesData.shrink_to_fit();

            EE_CORE_TRACE("ChunkStreaming: CPU -> Disk (clean) [{},{}]",
                chunk.ChunkCoords.x, chunk.ChunkCoords.y);
            return;
        }

        std::string path = GetChunkDiskPath(chunk.ChunkCoords);
        std::ofstream out(path, std::ios::binary);
        if (!out.is_open())
        {
            EE_CORE_ERROR("ChunkStreaming: failed to write '{}'", path);
            return;
        }

        // Magic + version
        uint32_t magic = 0x434E4B32; // "CNK2"
        uint32_t version = 1;
        out.write((const char*)&magic, 4);
        out.write((const char*)&version, 4);

        // Chunk metadata
        out.write((const char*)&chunk.Width, 4);
        out.write((const char*)&chunk.Height, 4);
        out.write((const char*)&chunk.ChunkCoords, sizeof(glm::ivec2));

        // Flags: which buffers are present
        uint8_t flags = 0;
        if (!chunk.TerrainData.empty())    flags |= 0x01;
        if (!chunk.PixelData.empty())      flags |= 0x02;
        if (!chunk.PropertiesData.empty()) flags |= 0x04;
        out.write((const char*)&flags, 1);

        auto writeCompressed = [&](const std::vector<uint8_t>& data)
            {
                uint32_t originalSize = static_cast<uint32_t>(data.size());
                int maxSize = LZ4_compressBound(originalSize);
                std::vector<uint8_t> compressed(maxSize);
                int compressedSize = LZ4_compress_HC(
                    (const char*)data.data(),
                    (char*)compressed.data(),
                    originalSize, maxSize,
                    LZ4HC_CLEVEL_DEFAULT);

                uint32_t compSize = static_cast<uint32_t>(compressedSize);
                out.write((const char*)&originalSize, 4);
                out.write((const char*)&compSize, 4);
                out.write((const char*)compressed.data(), compressedSize);
            };

        if (flags & 0x01) writeCompressed(chunk.TerrainData);
        if (flags & 0x02) writeCompressed(chunk.PixelData);
        if (flags & 0x04) writeCompressed(chunk.PropertiesData);

        out.close();

        // Free RAM
        chunk.TerrainData.clear();
        chunk.TerrainData.shrink_to_fit();
        chunk.PixelData.clear();
        chunk.PixelData.shrink_to_fit();
        chunk.PropertiesData.clear();
        chunk.PropertiesData.shrink_to_fit();

       
    }

    void TextureStreamingSystem::LoadChunkFromDisk(TextureChunk& chunk, Scene* scene)
    {
        EE_PROFILE_FUNCTION();

        if (!chunk.IsDirtyPixels)
        {
            // Clean chunk — reconstruct from atlas
            ReconstructChunkFromAtlas(chunk, scene);
            EE_CORE_TRACE("ChunkStreaming: Disk -> CPU (from atlas) [{},{}]",
                chunk.ChunkCoords.x, chunk.ChunkCoords.y);
            return;
        }

        std::string path = GetChunkDiskPath(chunk.ChunkCoords);
        std::ifstream in(path, std::ios::binary);
        if (!in.is_open())
        {
            EE_CORE_ERROR("ChunkStreaming: failed to read '{}', falling back to atlas", path);
            ReconstructChunkFromAtlas(chunk, scene);
            chunk.IsDirtyPixels = false;
            return;
        }

        uint32_t magic, version;
        in.read((char*)&magic, 4);
        in.read((char*)&version, 4);

        if (magic != 0x434E4B32 || version != 1)
        {
            EE_CORE_ERROR("ChunkStreaming: invalid file '{}'", path);
            in.close();
            ReconstructChunkFromAtlas(chunk, scene);
            chunk.IsDirtyPixels = false;
            return;
        }

        in.read((char*)&chunk.Width, 4);
        in.read((char*)&chunk.Height, 4);
        in.read((char*)&chunk.ChunkCoords, sizeof(glm::ivec2));

        uint8_t flags;
        in.read((char*)&flags, 1);

        auto readCompressed = [&](std::vector<uint8_t>& data)
            {
                uint32_t originalSize, compSize;
                in.read((char*)&originalSize, 4);
                in.read((char*)&compSize, 4);

                std::vector<uint8_t> compressed(compSize);
                in.read((char*)compressed.data(), compSize);

                data.resize(originalSize);
                LZ4_decompress_safe(
                    (const char*)compressed.data(),
                    (char*)data.data(),
                    compSize, originalSize);
            };

        if (flags & 0x01) readCompressed(chunk.TerrainData);
        if (flags & 0x02) readCompressed(chunk.PixelData);
        if (flags & 0x04) readCompressed(chunk.PropertiesData);

        in.close();

        EE_CORE_TRACE("ChunkStreaming: Disk -> CPU [{},{}]",
            chunk.ChunkCoords.x, chunk.ChunkCoords.y);
    }

    std::string TextureStreamingSystem::GetChunkDiskPath(const glm::ivec2& coords) const
    {
        return m_chunkCachePath + "/chunk_" +
            std::to_string(coords.x) + "_" +
            std::to_string(coords.y) + ".chunk";
    }
    void TextureStreamingSystem::ReconstructChunkFromAtlas(TextureChunk& chunk, Scene* scene)
    {
        EE_PROFILE_FUNCTION();

        const int CELL = int(TILE_PIXEL_WIDTH);
        const int chunkWpx = int(CHUNK_SIZE) * CELL;
        const int chunkHpx = chunkWpx;

        size_t totalPixels = size_t(chunkWpx) * size_t(chunkHpx);
        chunk.TerrainData.assign(totalPixels * 4, 0);
        chunk.Width = uint32_t(chunkWpx);
        chunk.Height = uint32_t(chunkHpx);

        const int chunkX0 = chunk.ChunkCoords.x * chunkWpx;
        const int chunkY0 = chunk.ChunkCoords.y * chunkHpx;
        const int chunkX1 = chunkX0 + chunkWpx;
        const int chunkY1 = chunkY0 + chunkHpx;

        scene->ForEach<TransformComponent, TileComponent>(
            [&](Entity entity, TransformComponent& transformComp, TileComponent& tileComp)
            {
                for (const TileInfo& tile : tileComp.tiles)
                {
                    if (tile.Category != eTileCategory::Terrain)
                        continue;

                    const float yoffset = 1.0f;
                    glm::vec2 worldTilePos = glm::vec2(transformComp.Translation) + tile.position;
                    worldTilePos.y += yoffset;

                    const int groundPxX = int(std::round(worldTilePos.x * float(CELL)));
                    const int groundPxY = int(std::round(worldTilePos.y * float(CELL)));

                    const int estX0 = groundPxX - int(TILE_PIXEL_WIDTH) / 2;
                    const int estY0 = groundPxY - 1;
                    const int estX1 = estX0 + int(TILE_PIXEL_WIDTH);
                    const int estY1 = estY0 + int(TILE_PIXEL_HEIGHT);

                    if (estX1 <= chunkX0 || estX0 >= chunkX1 ||
                        estY1 <= chunkY0 || estY0 >= chunkY1)
                        continue;

                    std::vector<uint8_t> pixelData;
                    int w = 0, h = 0;
                    if (!AssetManager::ExtractPixelsFromTilePallette(tile, pixelData, w, h))
                        continue;

                    const int destX0 = groundPxX - w / 2;
                    const int destY0 = groundPxY - 1;

                    const int left = std::max(destX0, chunkX0);
                    const int right = std::min(destX0 + w, chunkX1);
                    const int top = std::max(destY0, chunkY0);
                    const int bottom = std::min(destY0 + h, chunkY1);

                    if (left >= right || top >= bottom)
                        continue;

                    for (int dstY = top; dstY < bottom; ++dstY)
                    {
                        const int srcY = dstY - destY0;
                        if (srcY < 0 || srcY >= h) continue;

                        const int dstY_inChunk = dstY - chunkY0;
                        const size_t dstRow = size_t(dstY_inChunk) * size_t(chunkWpx) * 4;
                        const size_t srcRow = size_t(srcY) * size_t(w) * 4;

                        for (int dstX = left; dstX < right; ++dstX)
                        {
                            const int srcX = dstX - destX0;
                            if (srcX < 0 || srcX >= w) continue;

                            const int dstX_inChunk = dstX - chunkX0;
                            const size_t si = srcRow + size_t(srcX) * 4;
                            const size_t di = dstRow + size_t(dstX_inChunk) * 4;

                            const uint8_t a = pixelData[si + 3];
                            if (!a) continue;

                            chunk.TerrainData[di + 0] = pixelData[si + 0];
                            chunk.TerrainData[di + 1] = pixelData[si + 1];
                            chunk.TerrainData[di + 2] = pixelData[si + 2];
                            chunk.TerrainData[di + 3] = a;
                        }
                    }
                }
            }
        );

        
    }

    void TextureStreamingSystem::DebugMarkChunks()
    {
        EE_PROFILE_FUNCTION();
        constexpr uint32_t markerSize = 30; // Size of the red/green square in pixels

        for (auto& [chunkID, chunk] : m_chunkMap)
        {
            if (chunk.PixelData.empty())
                continue;

            uint32_t chunkW = chunk.Width;
            uint32_t chunkH = chunk.Height;

            for (uint32_t y = 0; y < chunkH; ++y)
            {
                for (uint32_t x = 0; x < chunkW; ++x)
                {
                    size_t index = (y * chunkW + x) * 4;

                    // Top-left red square
                    if (x < markerSize && y < markerSize)
                    {
                        chunk.PixelData[index + 0] = 255; // R
                        chunk.PixelData[index + 1] = 0;   // G
                        chunk.PixelData[index + 2] = 0;   // B
                        chunk.PixelData[index + 3] = 255; // A
                    }
                    // Bottom-right green square
                    else if (x >= chunkW - markerSize && y >= chunkH - markerSize)
                    {
                        chunk.PixelData[index + 0] = 0;   // R
                        chunk.PixelData[index + 1] = 255; // G
                        chunk.PixelData[index + 2] = 0;   // B
                        chunk.PixelData[index + 3] = 255; // A
                    }
                }
            }

            chunk.IsDirty = true; // Mark as dirty so it will be uploaded
        }
    }



    bool TextureStreamingSystem::DebugWriteTGA32(const char* path, int w, int h, const std::vector<uint8_t>& rgba)
    {
        if (w <= 0 || h <= 0) return false;
        if (rgba.size() < size_t(w) * h * 4) return false;

        FILE* f = std::fopen(path, "wb");
        if (!f) return false;

        // 18-byte TGA header
        uint8_t hdr[18] = {};
        hdr[2] = 2;                       // uncompressed true-color
        hdr[12] = uint8_t(w & 0xFF);
        hdr[13] = uint8_t((w >> 8) & 0xFF);
        hdr[14] = uint8_t(h & 0xFF);
        hdr[15] = uint8_t((h >> 8) & 0xFF);
        hdr[16] = 32;                      // 32 bpp
        hdr[17] = 0x28;                    // 8 alpha bits, origin at top-left (bit 5)
        std::fwrite(hdr, 1, 18, f);

        // TGA expects BGRA. We wrote origin=top-left, so we can stream rows 0..h-1.
        std::vector<uint8_t> row(size_t(w) * 4);
        for (int y = 0; y < h; ++y) {
            const uint8_t* src = &rgba[size_t(y) * size_t(w) * 4];
            for (int x = 0; x < w; ++x) {
                const uint8_t r = src[x * 4 + 0];
                const uint8_t g = src[x * 4 + 1];
                const uint8_t b = src[x * 4 + 2];
                const uint8_t a = src[x * 4 + 3];
                row[size_t(x) * 4 + 0] = b;
                row[size_t(x) * 4 + 1] = g;
                row[size_t(x) * 4 + 2] = r;
                row[size_t(x) * 4 + 3] = a;
            }
            std::fwrite(row.data(), 1, row.size(), f);
        }

        std::fclose(f);
        return true;
    }

    bool TextureStreamingSystem::DebugWritePPM(const char* path, int w, int h, const std::vector<uint8_t>& rgba)
    {
        if (w <= 0 || h <= 0) return false;
        if (rgba.size() < size_t(w) * h * 4) return false;

        FILE* f = std::fopen(path, "wb");
        if (!f) return false;
        std::fprintf(f, "P6\n%d %d\n255\n", w, h);
        for (int y = 0; y < h; ++y) {
            const uint8_t* src = &rgba[size_t(y) * size_t(w) * 4];
            for (int x = 0; x < w; ++x) {
                std::fwrite(&src[x * 4 + 0], 1, 1, f); // R
                std::fwrite(&src[x * 4 + 1], 1, 1, f); // G
                std::fwrite(&src[x * 4 + 2], 1, 1, f); // B
            }
        }
        std::fclose(f);
        return true;
    }

    namespace fs = std::filesystem;
    void TextureStreamingSystem::DumpRGBA(const std::string& filename, int w, int h, const std::vector<uint8_t>& rgba) {
        fs::path folder = "D:/EvaEngine/debug";
        fs::create_directories(folder);
        fs::path out = folder / filename;

        if (!DebugWriteTGA32(out.string().c_str(), w, h, rgba)) {
            EE_CORE_ERROR("Failed to write {}", out.string());
        }
        else {
            EE_CORE_INFO("Wrote {}", out.string());
        }
    }
}
