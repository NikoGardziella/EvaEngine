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

        bool chunksPackedDirty = false;

        glm::ivec2 playerChunk = glm::ivec2(glm::floor(playerPos / float(CHUNK_SIZE)));
        if (m_chunkMap.empty())
        {
            return;
        }
        for (auto& [id, chunk] : m_chunkMap)
        {
            glm::ivec2 chunkCoords = chunk.ChunkCoords;
            int dist = std::max(glm::abs(chunkCoords.x - playerChunk.x), glm::abs(chunkCoords.y - playerChunk.y));

            if (dist <= LOAD_RADIUS && !chunk.IsLoaded)
            {
                LoadChunkToGPU(chunk, gameRegistry);
                chunksPackedDirty = true;
            }
            else if (dist > UNLOAD_RADIUS && chunk.IsLoaded)
            {
                UnloadChunkFromGPU(chunk, gameRegistry);
                chunksPackedDirty = true;

            }
        }

        if (chunksPackedDirty)
        {
            SortChunksRowMajor(gameRegistry);
        }

    }

    void TextureStreamingSystem::UploadToChunkFromTexture(
        const glm::vec2& worldPosition, UUID ID, std::string name,
        const std::vector<uint8_t>& textureData,           // RGBA (4 bpp)
        const std::vector<uint8_t>& healthData,            // 1 bpp (optional; can be empty)
        uint32_t textureWidth, uint32_t textureHeight)
    {
        EE_PROFILE_FUNCTION();
        constexpr uint32_t chunkPixelSize = CHUNK_SIZE * PIXELS_IN_TILE;
        constexpr uint8_t  kDefaultHealth = 255; // initial health for solid pixels

        // Chunk addressing
        glm::ivec2 chunkCoords = glm::floor(glm::vec2(worldPosition) / float(CHUNK_SIZE));
        glm::ivec2 chunkOriginTiles = chunkCoords * (int)CHUNK_SIZE;
        glm::ivec2 offsetInChunkTiles = glm::ivec2(glm::floor(worldPosition)) - chunkOriginTiles;
        glm::ivec2 offsetInChunk = offsetInChunkTiles * (int)PIXELS_IN_TILE;

        TextureChunk& chunk = m_chunkMap[HashCoords(chunkCoords)];
        chunk.TextureCount += 1;

        const uint32_t totalChunkPixels = chunkPixelSize * chunkPixelSize;

        // First time we touch this chunk: allocate color (RGBA8) and health (RGBA8UI) storage
        if (chunk.PixelData.empty())
        {
            chunk.PixelData.resize(size_t(totalChunkPixels) * 4, 0); // RGBA8 color
            chunk.HealthData.resize(size_t(totalChunkPixels) * 4, 0); // RGBA8UI: R=health, G=timer, B/A=0
            chunk.Width = chunkPixelSize;
            chunk.Height = chunkPixelSize;
            chunk.IsLoaded = false;
            chunk.Name = "Chunk_" + std::to_string(chunkCoords.x) + "_" + std::to_string(chunkCoords.y);
            chunk.AssetName = name;
            chunk.ID = HashCoords(chunkCoords);
            chunk.ChunkCoords = chunkCoords;
        }

        // Bounds check for copy region
        if (offsetInChunk.x < 0 || offsetInChunk.y < 0) {
            EE_CORE_WARN("Texture '{}' is outside chunk bounds (negative offset)", name.c_str());
            return;
        }

        const uint32_t maxCopyW = chunkPixelSize - uint32_t(offsetInChunk.x);
        const uint32_t maxCopyH = chunkPixelSize - uint32_t(offsetInChunk.y);
        const uint32_t copyWidth = std::min(textureWidth, maxCopyW);
        const uint32_t copyHeight = std::min(textureHeight, maxCopyH);
        if (copyWidth == 0 || copyHeight == 0) {
            EE_CORE_WARN("Texture '{}' has zero copy area into chunk", name.c_str());
            return;
        }

        // Expect 4 bpp color source; health source is optional 1 bpp
        EE_CORE_ASSERT(textureData.size() >= size_t(textureWidth) * textureHeight * 4, "textureData too small");
        const bool has1BppHealth = (healthData.size() >= size_t(textureWidth) * textureHeight);

        for (uint32_t y = 0; y < copyHeight; ++y)
        {
            for (uint32_t x = 0; x < copyWidth; ++x)
            {
                const int dstX = offsetInChunk.x + int(x);
                const int dstY = offsetInChunk.y + int(y);

                const size_t dstColorIndex = (size_t(dstY) * chunkPixelSize + size_t(dstX)) * 4;
                const size_t dstHealthIndex = (size_t(dstY) * chunkPixelSize + size_t(dstX)) * 4; // 4 bpp RGBA8UI

                const size_t srcColorIndex = (size_t(y) * textureWidth + size_t(x)) * 4;
                const size_t srcHealthIndex = (size_t(y) * textureWidth + size_t(x));             // 1 bpp

                EE_CORE_ASSERT(dstColorIndex + 3 < chunk.PixelData.size(), "OOB color dst");
                EE_CORE_ASSERT(srcColorIndex + 3 < textureData.size(), "OOB color src");
                EE_CORE_ASSERT(dstHealthIndex + 3 < chunk.HealthData.size(), "OOB health dst");
                // srcHealthIndex is only used if has1BppHealth

                const uint8_t srcR = textureData[srcColorIndex + 0];
                const uint8_t srcG = textureData[srcColorIndex + 1];
                const uint8_t srcB = textureData[srcColorIndex + 2];
                const uint8_t srcA = textureData[srcColorIndex + 3];

                if (srcA != 0)
                {
                    // Write color as-is
                    chunk.PixelData[dstColorIndex + 0] = srcR;
                    chunk.PixelData[dstColorIndex + 1] = srcG;
                    chunk.PixelData[dstColorIndex + 2] = srcB;
                    chunk.PixelData[dstColorIndex + 3] = srcA;

                    // Expand health to RGBA8UI
                    const uint8_t healthR =
                        has1BppHealth ? healthData[srcHealthIndex]
                        : kDefaultHealth; // if no source, initialize from alpha presence

                    chunk.HealthData[dstHealthIndex + 0] = healthR; // R = health
                    chunk.HealthData[dstHealthIndex + 1] = 0;       // G = effect timer (starts at 0)
                    chunk.HealthData[dstHealthIndex + 2] = 0;       // B = unused
                    chunk.HealthData[dstHealthIndex + 3] = 0;       // A = unused
                }
                // else: transparent source pixel; leave color and health at 0s
            }
        }

        chunk.IsDirty = true;
    }


 

    void TextureStreamingSystem::SortChunksRowMajor(entt::registry& reg)
    {
        reg.sort<ChunkRendererComponent>([](const ChunkRendererComponent& a,
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



    void TextureStreamingSystem::LoadChunkToGPU(TextureChunk& chunk, entt::registry& gameRegistry)
    {
        EE_PROFILE_FUNCTION();

        if (chunk.PixelData.empty())
        {
			EE_CORE_ERROR("Chunk pixel data is empty for: {}", chunk.Name);
            return;
        }
        constexpr int CHUNK_RES = CHUNK_SIZE; // Assuming square chunks

        
        if (!chunk.HealthData.empty())
        {
            chunk.HealthTexture = std::make_shared<VulkanTexture>(chunk.Height, chunk.Width, VK_FORMAT_R8G8B8A8_UINT);

           
            VulkanUtils::TransitionImageLayout(chunk.HealthTexture->GetImage(), VK_FORMAT_R8G8B8A8_UINT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            chunk.HealthTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            chunk.HealthTexture->SetData(chunk.HealthData.data(), chunk.Height * chunk.Width * 4);
            // *****************************
        }

        chunk.GPUTexture = std::make_shared<VulkanTexture>(chunk.Height, chunk.Width, VK_FORMAT_R8G8B8A8_UNORM);

        VulkanUtils::TransitionImageLayout(chunk.GPUTexture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        chunk.GPUTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		chunk.GPUTexture->SetData(chunk.PixelData.data(), chunk.Height * chunk.Width * 4);



      

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
                chunkRendComp.HealthTexture = chunk.HealthTexture;
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

                // This Texture is still in s_VulkanData.TextureSlotIndex 
                // which means it would be rendered inside REcordCommands()
                // this will prevent it. Feels a bit crappy fix but lets see.
                chunkRendComp.Texture->SetCheckCollision(false);

                chunkRendComp.Texture = nullptr;
                chunkRendComp.HealthTexture = nullptr;
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

            for (const TileInfo& tile : tcomp.tiles)
            {
                glm::ivec2 worldTilePos = MapUtils::GetWorldTileCoords(tile.position, transformComp.Translation);

                
                std::vector<uint8_t> pixelData;
                std::vector<uint8_t> healthData;
                int width, height;
                if (!AssetManager::ExtractPixelsFromTilePallette(tile, pixelData, healthData, width, height))
                    continue;

               

                if (tile.IsDestructible)
                {
                  //  m_gridMap->MarkBlockedSubtilesFromTexture(worldTilePos, pixelData,
                   //     width, height);


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
            
			EE_CORE_INFO("Creating chunk entity at position: ({}, {})",
                chunk.ChunkCoords.x, chunk.ChunkCoords.y);

            IDComponent id;
            id.ID = HashCoords(chunk.ChunkCoords);
			EE_CORE_INFO("Adding chunk entity with ID: {}", (uint64_t)id.ID);
            IDComponent& idComp = registry.emplace<IDComponent>(entity);
            idComp = id;

            chunkRenderer.Texture = chunk.GPUTexture;
            chunkRenderer.ChunkCoords = chunk.ChunkCoords;
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
