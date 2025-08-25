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
#include "Engine/Map/Utils/IsoTileUtils.h"


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


    static constexpr uint32_t CELL_W = uint32_t(TILE_PIXEL_WIDTH);           // 128
    static constexpr uint32_t CELL_H = uint32_t(TILE_PIXEL_WIDTH / 2);       // 64
    void TextureStreamingSystem::UploadToChunkFromTexture(
        const glm::vec2& worldPosition,   // ground point in WORLD units (keep as vec2)
        UUID id,
        std::string name,
        const std::vector<uint8_t>& textureData,  // RGBA8, row 0 = bottom (flipped vertically in extractor)
        const std::vector<uint8_t>& healthData,   // 1 bpp optional
        uint32_t textureWidth,
        uint32_t textureHeight)
    {
        EE_PROFILE_FUNCTION();

        // --- iso footprint per logical cell (diamond) ---
        const int CELL_W = int(TILE_PIXEL_WIDTH);   
        const int CELL_H = int(TILE_PIXEL_WIDTH);     

        // --- chunk texture dimensions (rectangular) ---
        const int chunkWpx = int(CHUNK_SIZE) * CELL_W;     
        const int chunkHpx = int(CHUNK_SIZE) * CELL_H;     

        // Which chunk in WORLD 32×32 space (component-wise floor)
        glm::ivec2 chunkCoords = glm::floor(worldPosition / float(CHUNK_SIZE));

        // Top-left of this chunk in WORLD units (square 32×32 layout)
        glm::vec2 chunkWorldOrigin = glm::vec2(chunkCoords) * float(CHUNK_SIZE);

        // Local WORLD offset inside this chunk (0..32 on each axis)
        glm::vec2 localWorld = worldPosition - chunkWorldOrigin;

        // Map local WORLD to chunk TEXTURE pixel coords:
        // For a 32×32 quad drawn from a 4096×2048 texture:
        //   pixelSizeX = 32 / 4096 = 1/128  => texX = localWorld.x * 128
        //   pixelSizeY = 32 / 2048 = 1/64   => texY = localWorld.y *  64
        const int groundPx = int(std::floor(localWorld.x * float(CELL_W)));  // bottom-center X in pixels
        const int groundPy = int(std::floor(localWorld.y * float(CELL_H)));  // bottom-center Y in pixels

        // Fetch / allocate chunk
        TextureChunk& chunk = m_chunkMap[HashCoords(chunkCoords)];
        chunk.TextureCount += 1;

        const size_t totalPixels = size_t(chunkWpx) * size_t(chunkHpx);
        if (chunk.PixelData.empty())
        {
            chunk.PixelData.assign(totalPixels * 4, 0);
            chunk.HealthData.assign(totalPixels * 4, 0);
            chunk.Width = uint32_t(chunkWpx);
            chunk.Height = uint32_t(chunkHpx);
            chunk.IsLoaded = false;
            chunk.Name = "Chunk_" + std::to_string(chunkCoords.x) + "_" + std::to_string(chunkCoords.y);
            chunk.AssetName = name;
            chunk.ID = HashCoords(chunkCoords);
            chunk.ChunkCoords = chunkCoords;
        }

        // Stamp the extracted buffer:
        //   - Source is V-FLIPPED (row 0 = bottom)
        //   - We want source row 0 to land on 'groundPy - 1' (bottom contact)
        //     so the top-left destination becomes:
        const int destX0 = groundPx - int(textureWidth) / 2;          // center horizontally on ground
        const int destY0 = (groundPy - 1) - 0;                        // row 0 at groundPy - 1

        // Clip to chunk bounds
        const int left = std::max(0, destX0);
        const int top = std::max(0, destY0);
        const int right = std::min(chunkWpx, destX0 + int(textureWidth));
        const int bottom = std::min(chunkHpx, destY0 + int(textureHeight));
        if (left >= right || top >= bottom) {
            EE_CORE_WARN("Texture '{}' outside chunk", name.c_str());
            return;
        }

        EE_CORE_ASSERT(textureData.size() >= size_t(textureWidth) * textureHeight * 4, "textureData too small");
        const bool has1BppHealth = (healthData.size() >= size_t(textureWidth) * textureHeight);

        // Tight top->bottom copy. Because the source is already V-FLIPPED (row 0 bottom),
        // we copy with srcY = (dstY - destY0) and row 0 lands at groundPy - 1 as intended.
        for (int dstY = top; dstY < bottom; ++dstY)
        {
            const int srcY = dstY - destY0;                               // 0..textureHeight-1
            if (srcY < 0 || srcY >= int(textureHeight)) continue;

            const size_t dstRow = size_t(dstY) * size_t(chunkWpx) * 4;
            const size_t srcRow = size_t(srcY) * size_t(textureWidth) * 4;

            for (int dstX = left; dstX < right; ++dstX)
            {
                const int srcX = dstX - destX0;                           // 0..textureWidth-1
                if (srcX < 0 || srcX >= int(textureWidth)) continue;

                const size_t si = srcRow + size_t(srcX) * 4;
                const size_t di = dstRow + size_t(dstX) * 4;

                const uint8_t a = textureData[si + 3];
                if (!a) continue;

                // Color
                chunk.PixelData[di + 0] = textureData[si + 0];
                chunk.PixelData[di + 1] = textureData[si + 1];
                chunk.PixelData[di + 2] = textureData[si + 2];
                chunk.PixelData[di + 3] = a;

                // Health (R), others zero
                const uint8_t hR = has1BppHealth
                    ? healthData[size_t(srcY) * size_t(textureWidth) + size_t(srcX)]
                    : 255u;
                chunk.HealthData[di + 0] = hR;
                chunk.HealthData[di + 1] = 0;
                chunk.HealthData[di + 2] = 0;
                chunk.HealthData[di + 3] = 0;
            }
        }

        chunk.IsDirty = true;
    }




    void TextureStreamingSystem::UploadTerrainToChunkFromTexture(
        const glm::vec2& worldPosition, UUID ID, std::string name,
        const std::vector<uint8_t>& textureData,   // RGBA (4 bpp)
        uint32_t textureWidth, uint32_t textureHeight)
    {
        EE_PROFILE_FUNCTION();

        constexpr uint32_t chunkPixelSize = CHUNK_SIZE * TILE_PIXEL_WIDTH;

        // Chunk addressing (same as your other uploader)
        glm::ivec2 chunkCoords = glm::floor(glm::vec2(worldPosition) / float(CHUNK_SIZE));
        glm::ivec2 chunkOriginTiles = chunkCoords * (int)CHUNK_SIZE;
        glm::ivec2 offsetInChunkTiles = glm::ivec2(glm::floor(worldPosition)) - chunkOriginTiles;
        glm::ivec2 offsetInChunk = offsetInChunkTiles * (int)TILE_PIXEL_WIDTH;

        TextureChunk& chunk = m_chunkMap[HashCoords(chunkCoords)];
        chunk.TextureCount += 1;

        const uint32_t totalChunkPixels = chunkPixelSize * chunkPixelSize;

        // Allocate terrain storage the first time we touch this chunk
        if (chunk.TerrainData.empty())
        {
            chunk.TerrainData.resize(size_t(totalChunkPixels) * 4, 0); // RGBA8 terrain
            // If this chunk was never initialized by the dynamic uploader, set metadata
            if (chunk.Width == 0 || chunk.Height == 0)
            {
                chunk.Width = chunkPixelSize;
                chunk.Height = chunkPixelSize;
                chunk.IsLoaded = false;
                chunk.Name = "Chunk_" + std::to_string(chunkCoords.x) + "_" + std::to_string(chunkCoords.y);
                chunk.AssetName = name;
                chunk.ID = HashCoords(chunkCoords);
                chunk.ChunkCoords = chunkCoords;
            }
        }
        else
        {
            // Ensure correct size if something changed
            const size_t wanted = size_t(totalChunkPixels) * 4;
            if (chunk.TerrainData.size() != wanted)
                chunk.TerrainData.assign(wanted, 0);
        }

        // Bounds check for copy region
        if (offsetInChunk.x < 0 || offsetInChunk.y < 0)
        {
            EE_CORE_WARN("Terrain '{}' is outside chunk bounds (negative offset)", name.c_str());
            return;
        }

        const uint32_t maxCopyW = chunkPixelSize - uint32_t(offsetInChunk.x);
        const uint32_t maxCopyH = chunkPixelSize - uint32_t(offsetInChunk.y);
        const uint32_t copyWidth = std::min(textureWidth, maxCopyW);
        const uint32_t copyHeight = std::min(textureHeight, maxCopyH);
        if (copyWidth == 0 || copyHeight == 0)
        {
            EE_CORE_WARN("Terrain '{}' has zero copy area into chunk", name.c_str());
            return;
        }

        // Expect 4 bpp color source
        EE_CORE_ASSERT(textureData.size() >= size_t(textureWidth) * textureHeight * 4, "terrain textureData too small");

        // Copy RGBA into TerrainData. We skip fully transparent source texels to preserve existing terrain underneath.
        // If you want transparent areas to clear previous terrain, remove the alpha check and always write.
        for (uint32_t y = 0; y < copyHeight; ++y)
        {
            for (uint32_t x = 0; x < copyWidth; ++x)
            {
                const int dstX = offsetInChunk.x + int(x);
                const int dstY = offsetInChunk.y + int(y);

                const size_t dstIdx = (size_t(dstY) * chunkPixelSize + size_t(dstX)) * 4;
                const size_t srcIdx = (size_t(y) * textureWidth + size_t(x)) * 4;

                EE_CORE_ASSERT(dstIdx + 3 < chunk.TerrainData.size(), "OOB terrain dst");
                EE_CORE_ASSERT(srcIdx + 3 < textureData.size(), "OOB terrain src");

                const uint8_t r = textureData[srcIdx + 0];
                const uint8_t g = textureData[srcIdx + 1];
                const uint8_t b = textureData[srcIdx + 2];
                const uint8_t a = textureData[srcIdx + 3];

                if (a != 0)
                {
                    chunk.TerrainData[dstIdx + 0] = r;
                    chunk.TerrainData[dstIdx + 1] = g;
                    chunk.TerrainData[dstIdx + 2] = b;
                    chunk.TerrainData[dstIdx + 3] = a;
                }
                // else: leave existing terrain as-is under transparent texel
            }
        }

        chunk.IsDirty = true; // mark for GPU upload
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

        
        if (!chunk.TerrainData.empty())
        {
            chunk.TerrainTexture = std::make_shared<VulkanTexture>(chunk.Height, chunk.Width, VK_FORMAT_R8G8B8A8_UNORM);

            VulkanUtils::TransitionImageLayout(chunk.TerrainTexture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            chunk.TerrainTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            chunk.TerrainTexture->SetData(chunk.TerrainData.data(), chunk.Height * chunk.Width * 4);

        }


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
                chunkRendComp.TerrainTexture = chunk.TerrainTexture;
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

     
            for (const TileInfo& tile : tcomp.tiles)
            {

                if (tile.Category != eTileCategory::Terrain)
                {
                    continue;
                }

                glm::ivec2 worldTilePos = MapUtils::GetWorldTileCoords(tile.position, transformComp.Translation);
                std::vector<uint8_t> pixelData;
                std::vector<uint8_t> healthData;
                int width, height;
                if (!AssetManager::ExtractPixelsFromTilePallette(tile, pixelData, healthData, width, height))
                    continue;
              
                    //  m_gridMap->MarkBlockedSubtilesFromTexture(worldTilePos, pixelData,
                     //     width, height);

                UploadTerrainToChunkFromTexture(worldTilePos, tcomp.TileID, tile.name, pixelData,
                    uint32_t(width), uint32_t(height));
                

            }
        }


        for (auto entity : view)
        {
            // check if this entity has roof tiles.
            // if it does, make a new roof texture that is combinatio of all roof tiles
            TextureStreamingUtils::BakeRoofTextureIfNeeded(registry, entity);
            TextureStreamingUtils::BakeVehicleTextureIfNeeded(registry, entity);

            auto& transformComp = view.get<TransformComponent>(entity);
            const auto& tcomp = view.get<TileComponent>(entity);

            for (const TileInfo& tile : tcomp.tiles)
            {

                if (tile.Category == eTileCategory::Terrain)
                {
                    continue;
                }
                const glm::vec2 worldTilePos = glm::vec2(transformComp.Translation) + tile.position;
                
                std::vector<uint8_t> pixelData;
                std::vector<uint8_t> healthData;
                int width, height;
                if (!AssetManager::ExtractPixelsFromTilePallette(tile, pixelData, healthData, width, height))
                    continue;
                EE_CORE_INFO("worldTilePos {}, {}", worldTilePos.x, worldTilePos.y);


                  //  m_gridMap->MarkBlockedSubtilesFromTexture(worldTilePos, pixelData,
                   //     width, height);

                UploadToChunkFromTexture(worldTilePos, tcomp.TileID,tile.name,pixelData,
                    healthData, uint32_t(width), uint32_t(height));
                
            
            }
        }

        //DebugMarkChunks();
    }

    void TextureStreamingSystem::SortIsoTilesByY(entt::registry& registry)
    {
        // A) Entities: higher Y first (draw earlier)
        registry.sort<TransformComponent>(
            [&registry](entt::entity a, entt::entity b)
            {
                const auto& ta = registry.get<TransformComponent>(a).Translation;
                const auto& tb = registry.get<TransformComponent>(b).Translation;

                if (ta.y != tb.y) return ta.y > tb.y;        // DESC by Y
                return (uint32_t)a < (uint32_t)b;            // stable tie-break
            }
        );

        // B) Tiles within each entity: higher ground Y first (draw earlier)
        auto view = registry.view<TileComponent, TransformComponent>();
        for (auto e : view)
        {
            auto& tc = view.get<TileComponent>(e);
            const auto& tr = view.get<TransformComponent>(e);

            std::stable_sort(tc.tiles.begin(), tc.tiles.end(),
                [&](const TileInfo& A, const TileInfo& B)
                {
                    const float yA = tr.Translation.y + A.position.y; // A/B.position = WORLD delta to GROUND
                    const float yB = tr.Translation.y + B.position.y;
                    return yA > yB;                                    // DESC by Y
                }
            );
        }
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
        fs::path folder = "C:/EvaEngine/debug";
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
