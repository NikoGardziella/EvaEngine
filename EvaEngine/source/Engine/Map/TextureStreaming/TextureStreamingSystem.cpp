#include "pch.h"
#include "TextureStreamingSystem.h"
#include "TextureStreamingUtils.h"
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Renderer/VulkanRenderer2D.h>
#include "Engine/Platform/Vulkan/VulkanUtils.h"
#include <Engine/Scene/Components/Render/ChunkRendererComponent.h>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Map/Utils/PixelUtils.h>


namespace Engine {

    const int mapWidth = 4096; // Example width in pixels
    const int mapHeight = 4096; // Example height in pixels
 // Size of each chunk in pixels
	TextureStreamingSystem::TextureStreamingSystem()
    {
      
    }

    TextureStreamingSystem::~TextureStreamingSystem()
    {

    }

    void TextureStreamingSystem::Update(const glm::vec2& playerPos, entt::registry& gameRegistry)
    {
        EE_PROFILE_FUNCTION();

        glm::ivec2 playerChunk = glm::ivec2(glm::floor(playerPos / float(CHUNK_SIZE)));
        if (m_chunkMap.empty())
        {
            return;
        }

        // --- Load nearby chunks ---
        for (auto& [id, chunk] : m_chunkMap)
        {
            if (chunk.IsLoaded)
                continue;
           
            //if (!chunk.IsDirty)
            //   continue;

            glm::ivec2 chunkCoords = chunk.ChunkCoords;
            int dist = glm::abs(chunkCoords.x - playerChunk.x) + glm::abs(chunkCoords.y - playerChunk.y);
            if (dist <= LOAD_RADIUS)
            {
                EE_CORE_INFO("Chunks currently being loaded:");
                EE_CORE_INFO(" - Chunk at coords: ({}, {})", chunk.ChunkCoords.x, chunk.ChunkCoords.y);
                LoadChunkToGPU(chunk, gameRegistry);

				EE_CORE_INFO("chunk count: {}", m_chunkMap.size());
            }
        }

        // --- Unload far-away chunks ---
        for (auto& [id, chunk] : m_chunkMap)
        {
            if (!chunk.IsLoaded)
                continue;

            //EE_CORE_INFO("Chunks currently loaded:");
            //EE_CORE_INFO(" - Chunk at coords: ({}, {})", chunk.ChunkCoords.x, chunk.ChunkCoords.y);

            glm::ivec2 chunkCoords = chunk.ChunkCoords;
            int dist = glm::abs(chunkCoords.x - playerChunk.x) + glm::abs(chunkCoords.y - playerChunk.y);

            if (dist > UNLOAD_RADIUS)
            {
                UnloadChunkFromGPU(chunk, gameRegistry);
            }
        }
    }
    void TextureStreamingSystem::UploadToChunkFromTexture(
        const glm::vec2& worldPosition, UUID ID, std::string name,
        const std::vector<uint8_t>& textureData,           // RGBA
        const std::vector<uint8_t>& healthData,            // 1 byte per pixel
        uint32_t textureWidth, uint32_t textureHeight)
    {
        EE_PROFILE_FUNCTION();
        constexpr uint32_t chunkPixelSize = CHUNK_SIZE * PIXELS_IN_TILE;

        glm::ivec2 chunkCoords = glm::floor(glm::vec2(worldPosition) / float(CHUNK_SIZE));
        glm::ivec2 chunkOrigin = chunkCoords * (int)CHUNK_SIZE;
        glm::ivec2 offsetInChunkTiles = glm::ivec2(glm::floor(worldPosition)) - chunkOrigin;
        glm::ivec2 offsetInChunk = offsetInChunkTiles * (int)PIXELS_IN_TILE;

        TextureChunk& chunk = m_chunkMap[HashCoords(chunkCoords)];
        chunk.TextureCount += 1;

        const uint32_t totalChunkPixels = chunkPixelSize * chunkPixelSize;

        if (chunk.PixelData.empty())
        {
            chunk.PixelData.resize(totalChunkPixels * 4, 0);   // RGBA
            chunk.HealthData.resize(totalChunkPixels, 0);      // 1 byte per pixel
            chunk.Width = chunkPixelSize;
            chunk.Height = chunkPixelSize;
            chunk.IsLoaded = false;
            chunk.Name = "Chunk_" + std::to_string(chunkCoords.x) + "_" + std::to_string(chunkCoords.y);
            chunk.AssetName = name;
            chunk.ID = HashCoords(chunkCoords);
            chunk.ChunkCoords = chunkCoords;


           
        }

        uint32_t copyWidth = std::min(textureWidth, chunkPixelSize - offsetInChunk.x);
        uint32_t copyHeight = std::min(textureHeight, chunkPixelSize - offsetInChunk.y);

        if (offsetInChunk.x < 0 || offsetInChunk.y < 0 || copyWidth == 0 || copyHeight == 0)
        {
            EE_CORE_WARN("Texture '{}' is outside chunk bounds", name.c_str());
            return;
        }

        for (uint32_t y = 0; y < copyHeight; ++y)
        {
            for (uint32_t x = 0; x < copyWidth; ++x)
            {
                int dstX = offsetInChunk.x + x;
                int dstY = offsetInChunk.y + y;

                size_t dstColorIndex = (dstY * chunkPixelSize + dstX) * 4;
                size_t dstHealthIndex = (dstY * chunkPixelSize + dstX);

                size_t srcIndex = (y * textureWidth + x) * 4;
                size_t healthIndex = (y * textureWidth + x);

                EE_CORE_ASSERT(dstColorIndex + 3 < chunk.PixelData.size(), "OOB color dst");
                EE_CORE_ASSERT(srcIndex + 3 < textureData.size(), "OOB color src");
                EE_CORE_ASSERT(dstHealthIndex < chunk.HealthData.size(), "OOB health dst");
                EE_CORE_ASSERT(healthIndex < healthData.size(), "OOB health src");

                const uint8_t alpha = textureData[srcIndex + 3];
                if (alpha != 0)
                {
                    std::memcpy(&chunk.PixelData[dstColorIndex], &textureData[srcIndex], 4); // RGBA
                    chunk.HealthData[dstHealthIndex] = healthData[healthIndex];             // Health
                }
            }
        }

        chunk.IsDirty = true;
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



    void TextureStreamingSystem::LoadChunkToGPU(TextureChunk& chunk, entt::registry& gameRegistry)
    {
        EE_PROFILE_FUNCTION();

        if (chunk.PixelData.empty())
        {
			EE_CORE_ERROR("Chunk pixel data is empty for: {}", chunk.Name);
            return;
        }
        constexpr int CHUNK_RES = CHUNK_SIZE; // Assuming square chunks

        bool hasHealthData = false;
        if (!chunk.HealthData.empty())
        {
            hasHealthData = true;
        }
        chunk.GPUTexture = std::make_shared<VulkanTexture>(hasHealthData, chunk.Height, chunk.Width);

        VulkanUtils::TransitionImageLayout(chunk.GPUTexture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		chunk.GPUTexture->SetData(chunk.PixelData.data(), chunk.Height * chunk.Width * 4);



     
        chunk.GPUTexture->SetHealtData(chunk.HealthData.data(), chunk.Height * chunk.Width);

        int count = 0;
        for (size_t i = 0; i < chunk.HealthData.size(); i++)
        {
            if (chunk.HealthData[i] != 0)
            {
                count++;
            }
        }
        EE_CORE_INFO("health count {}", count);
       

        auto entityView = gameRegistry.view<IDComponent, SpriteRendererComponent>();

        for (auto entity : entityView)
        {

            auto [IDComp, spriteRendComp] = entityView.get<IDComponent, SpriteRendererComponent>(entity);
			if (IDComp.ID == chunk.ID)
			{
				spriteRendComp.Texture = chunk.GPUTexture;
               
                break;
			}

        }

        auto chynkentityView = gameRegistry.view<IDComponent, ChunkRendererComponent>();

        for (auto entity : chynkentityView)
        {

            auto [IDComp, chunkRendComp] = chynkentityView.get<IDComponent, ChunkRendererComponent>(entity);
            if (chunkRendComp.ChunkCoords == chunk.ChunkCoords)
            {
                chunkRendComp.Texture = chunk.GPUTexture;
                chunkRendComp.IsLoaded = true;
                chunk.GPUTexture = nullptr;
                break;
            }

        }
        chunk.IsLoaded = true;
    }



    void TextureStreamingSystem::UnloadChunkFromGPU(TextureChunk& chunk, entt::registry& gameRegistry)
    {
        EE_PROFILE_FUNCTION();
        EE_CORE_INFO("Unloading chunk at coords: {}, {}", chunk.ChunkCoords.x, chunk.ChunkCoords.y);

        auto entityView = gameRegistry.view<IDComponent, SpriteRendererComponent>();

        for (auto entity : entityView)
        {

            auto [IDComp, spriteRendComp] = entityView.get<IDComponent, SpriteRendererComponent>(entity);
            if (IDComp.ID == chunk.ID)
            {
                spriteRendComp.Texture = nullptr;
                break;
            }

        }

        auto chynkentityView = gameRegistry.view<IDComponent, ChunkRendererComponent>();

        for (auto entity : chynkentityView)
        {

            auto [IDComp, chunkRendComp] = chynkentityView.get<IDComponent, ChunkRendererComponent>(entity);
            if (IDComp.ID == chunk.ID)
            {
                chunkRendComp.Texture = nullptr;
                chunkRendComp.IsLoaded = false;
                break;
            }

        }
        chunk.GPUTexture = nullptr;
        chunk.IsLoaded = false;
    }

    void TextureStreamingSystem::ResetAllChunks(entt::registry& gameRegistry)
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
            auto chynkentityView = gameRegistry.view<IDComponent, ChunkRendererComponent>();

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
    void TextureStreamingSystem::DebugDrawChunkOutlines(entt::registry& gameRegistry)
    {
        EE_PROFILE_FUNCTION();
        // 1) Find the player's position
        glm::vec2 playerPos{ 0.0f };
        auto playerView = gameRegistry.view<TransformComponent, CharacterControllerComponent>();
        for (auto entity : playerView)
        {
            auto& xf = playerView.get<TransformComponent>(entity);
            playerPos = { xf.Translation.x, xf.Translation.y };
            break; // Assume only one player
        }

        constexpr float cs = float(CHUNK_SIZE);
        constexpr int DEBUG_RADIUS = 1;

        glm::ivec2 playerChunk = glm::floor(playerPos / cs);
        std::unordered_set<glm::ivec2, IVec2Hasher> loadedCoords;
        glm::vec2 halfChunkOffset = glm::vec2(cs * 0.5f);

        // 2) Draw nearby unloaded chunks in red
        for (int dy = -DEBUG_RADIUS; dy <= DEBUG_RADIUS; ++dy)
        {
            for (int dx = -DEBUG_RADIUS; dx <= DEBUG_RADIUS; ++dx)
            {
                glm::ivec2 coords = playerChunk + glm::ivec2(dx, dy);

                if (loadedCoords.count(coords) > 0)
                    continue;

                glm::vec2 origin = glm::vec2(coords) * cs - halfChunkOffset;
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(origin, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(cs, cs, 1.0f));

                glm::vec4 color = glm::vec4(1, 0, 0, 1); // Red = not loaded
                Engine::VulkanRenderer2D::DrawLineRect(transform, color, -1);
            }
        }

        // 3) Draw loaded chunks in green if near player
        for (const auto& [coord, chunk] : m_chunkMap)
        {
            if (chunk.IsLoaded)
            {
                glm::ivec2 chunkCoords = chunk.ChunkCoords;
                loadedCoords.insert(chunkCoords);

                // Only draw if near player
                glm::ivec2 delta = chunkCoords - playerChunk;
                if (abs(delta.x) > DEBUG_RADIUS || abs(delta.y) > DEBUG_RADIUS)
                    continue;

                glm::vec2 origin = glm::vec2(chunkCoords) * cs - halfChunkOffset;
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(origin, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(cs, cs, 1.0f));

                glm::vec4 color = glm::vec4(0, 1, 0, 1); // Green = loaded
                Engine::VulkanRenderer2D::DrawLineRect(transform, color, -1);
            }
        }
    }



    void TextureStreamingSystem::BakeTilesIntoChunks(entt::registry& registry)
    {
        EE_PROFILE_FUNCTION();

        auto view = registry.view<TransformComponent, TileComponent>();
        for (auto entity : view)
        {
            auto& transformComp = view.get<TransformComponent>(entity);
            const auto& tcomp = view.get<TileComponent>(entity);


            // check if this entity has roof tiles.
            // if it does, make a new roof texture that is combinatio of all roof tiles
            TextureStreamingUtils::BakeRoofTextureIfNeeded(registry, entity);
            TextureStreamingUtils::BakeVehicleTextureIfNeeded(registry, entity);

            for (const auto& tile : tcomp.tiles)
            {
                glm::ivec2 worldTilePos = MapUtils::GetWorldTileCoords(tile.position, transformComp.Translation);

                
                std::vector<uint8_t> pixelData;
                std::vector<uint8_t> healthData;
                int width, height;
                if (!AssetManager::ExtractPixelsFromTilePallette(tile.UV, pixelData, healthData, width, height))
                    continue;

               
                uint8_t health = 1; //MaterialDatabase::GetMaterial(tile.MaterialName); // e.g., Wood = 1, Steel = 2
                uint8_t durability = 1; //MaterialDatabase::GetDurability(mat);

              

                if (tile.IsDestructible)
                {
                    m_gridMap->MarkBlockedSubtilesFromTexture(worldTilePos, pixelData,
                        width, height);


                    UploadToChunkFromTexture(worldTilePos, tcomp.TileID,tile.name,pixelData, 
                        healthData, uint32_t(width), uint32_t(height));
                }
                


            }
        }

        //DebugMarkChunks();
    }



    void TextureStreamingSystem::AddChunkEntitiesToRegistry(entt::registry& registry)
    {
        EE_PROFILE_FUNCTION();
        for (auto& [uuid, chunk] : m_chunkMap)
        {
            
            auto entity = registry.create();
            auto& chunkRenderer = registry.emplace<ChunkRendererComponent>(entity);
            TransformComponent& transformComp = registry.emplace<TransformComponent>(entity);
            transformComp.Translation.x = chunk.WorldPosition.x;
            transformComp.Translation.y = chunk.WorldPosition.y;

			EE_CORE_INFO("Creating chunk entity at position: ({}, {})",
				transformComp.Translation.x, transformComp.Translation.y);

            IDComponent id;
            id.ID = HashCoords(chunk.ChunkCoords);
			EE_CORE_INFO("Adding chunk entity with ID: {}", (uint64_t)id.ID);
            IDComponent& idComp = registry.emplace<IDComponent>(entity);
            idComp = id;

            chunkRenderer.Texture = chunk.GPUTexture;
            chunkRenderer.ChunkCoords = chunk.ChunkCoords;
            chunkRenderer.ChunkSize = CHUNK_SIZE;
			chunkRenderer.IsLoaded = false;
            //FlipChunkHorizontally(chunk);
           // FlipChunkVertically(chunk);

        }
		EE_CORE_INFO("Added {} chunk entities to registry", m_chunkMap.size());
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





}
