#include "pch.h"
#include "TextureStreamingSystem.h"
#include <glm/geometric.hpp>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Renderer/VulkanRenderer2D.h>
#include "Engine/Platform/Vulkan/VulkanUtils.h"
#include <Engine/Scene/Components/Render/ChunkRendererComponent.h>


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
                LoadChunkToGPU(chunk, gameRegistry);
            }
        }

        // --- Unload far-away chunks ---
        for (auto& [id, chunk] : m_chunkMap)
        {
            if (!chunk.IsLoaded)
                continue;

            glm::ivec2 chunkCoords = chunk.ChunkCoords;
            int dist = glm::abs(chunkCoords.x - playerChunk.x) + glm::abs(chunkCoords.y - playerChunk.y);

            if (dist > UNLOAD_RADIUS)
            {
                UnloadChunkFromGPU(chunk, gameRegistry);
            }
        }
    }

    void TextureStreamingSystem::UploadToChunkFromTexture(
        const glm::vec3& worldPosition, UUID ID, std::string name,
        const std::vector<uint8_t>& textureData,
        uint32_t textureWidth, uint32_t textureHeight)
    {
        constexpr uint32_t chunkPixelSize = CHUNK_SIZE * PIXELS_IN_TILE;

        glm::ivec2 chunkCoords = glm::floor(glm::vec2(worldPosition) / float(CHUNK_SIZE));
        glm::ivec2 chunkOrigin = chunkCoords * (int)CHUNK_SIZE;
        glm::ivec2 offsetInChunkTiles = glm::ivec2(worldPosition) - chunkOrigin;
        glm::ivec2 offsetInChunk = offsetInChunkTiles * (int)PIXELS_IN_TILE;
        EE_CORE_INFO(
            "Tile '{}' at worldPos = ({:.1f}, {:.1f}), "
            "offsetInChunkTiles = ({}, {}), offsetInChunkPixels = ({}, {})",
            name.c_str(),
            worldPosition.x, worldPosition.y,
            offsetInChunkTiles.x, offsetInChunkTiles.y,
            offsetInChunk.x, offsetInChunk.y
        );

        // Resize CPU buffer if needed
        TextureChunk& chunk = m_chunkMap[HashCoords(chunkCoords)];
        chunk.TextureCount += 1;
        if (chunk.PixelData.empty())
        {
            uint32_t chunkPixelSize = CHUNK_SIZE * PIXELS_IN_TILE;
            chunk.PixelData.resize(chunkPixelSize * chunkPixelSize * 4, 0);
            chunk.Width = chunkPixelSize;
            chunk.Height = chunkPixelSize;
            chunk.IsLoaded = false;
            chunk.Name = "Chunk_" + std::to_string(chunkCoords.x) + "_" + std::to_string(chunkCoords.y);
            chunk.AssetName = name;
            chunk.ID = HashCoords(chunkCoords);
        }
        // Clamp to chunk pixel bounds
        uint32_t copyWidth = std::min(textureWidth, chunkPixelSize - offsetInChunk.x);
        uint32_t copyHeight = std::min(textureHeight, chunkPixelSize - offsetInChunk.y);

        if (offsetInChunk.x < 0 || offsetInChunk.y < 0 ||
            copyWidth == 0 || copyHeight == 0)
        {
            EE_CORE_WARN("Texture '{}' is outside chunk bounds", name.c_str());
            return;
        }

        int copied = 0;
        for (uint32_t y = 0; y < copyHeight; ++y)
        {
            for (uint32_t x = 0; x < copyWidth; ++x)
            {
                int dstX = offsetInChunk.x + x;
                int dstY = offsetInChunk.y + y;

                size_t dstIndex = (dstY * chunkPixelSize + dstX) * 4;
                size_t srcIndex = (y * textureWidth + x) * 4;
                EE_CORE_ASSERT(dstIndex + 3 < chunk.PixelData.size(), "OOB dst");
                EE_CORE_ASSERT(srcIndex + 3 < textureData.size(), "OOB src");

                std::memcpy(&chunk.PixelData[dstIndex],
                    &textureData[srcIndex], 4);
                ++copied;
            }
        }

        chunk.IsDirty = true;
    }

    
    uint64_t TextureStreamingSystem::HashCoords(const glm::ivec2& coords)
    {
        // Offset to support negative coordinates
        // change this when texture is rendered in same position?
        int64_t x = static_cast<int64_t>(coords.x) + static_cast<int64_t>(1) << 31;
        int64_t y = static_cast<int64_t>(coords.y) + static_cast<int64_t>(1) << 31;

        return (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(y);
    }



    void TextureStreamingSystem::LoadChunkToGPU(TextureChunk& chunk, entt::registry& gameRegistry)
    {
        EE_PROFILE_FUNCTION();


        EE_CORE_INFO("Loading chunk at coords: {}, {}", chunk.ChunkCoords.x, chunk.ChunkCoords.y);
        EE_CORE_INFO("Loading chunk {}", chunk.Name);
        EE_CORE_INFO("Uploading to GPU: size {}x{}, PixelData size: {}", chunk.Width, chunk.Height, chunk.PixelData.size());

        if (chunk.PixelData.empty())
        {
			EE_CORE_ERROR("Chunk pixel data is empty for: {}", chunk.Name);
            return;
        }
        constexpr int CHUNK_RES = CHUNK_SIZE; // Assuming square chunks

        chunk.GPUTexture = std::make_shared<VulkanTexture>(chunk.Height, chunk.Width);
        //EE_CORE_INFO("Texture size: {}x{}", chunk.GPUTexture->GetWidth(), chunk.GPUTexture->GetHeight());

        VulkanUtils::TransitionImageLayout(chunk.GPUTexture->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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
            if (IDComp.ID == chunk.ID)
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
        EE_CORE_INFO("Unloading chunk at coords: {}, {}", chunk.ChunkCoords.x, chunk.ChunkCoords.y);

        // Reset the owning smart pointer — its destructor will free Vulkan resources
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
        EE_CORE_INFO("Resetting all chunks (scheduled unload)...");

        // Phase 1: gather IDs of all loaded chunks
        std::vector<UUID> toUnload;
		uint32_t chunkCount = static_cast<uint32_t>(m_chunkMap.size());
        if (m_chunkMap.empty() || chunkCount <= 0)
        {
			EE_CORE_WARN("No chunks to unload.");
			return;
        }

        toUnload.reserve(m_chunkMap.size());
        for (auto& [id, chunk] : m_chunkMap)
        {
            if (chunk.IsLoaded)
                toUnload.push_back(id);
        }

        // Phase 2: safely unload each, one by one
        for (auto const& id : toUnload)
        {
            auto it = m_chunkMap.find(id);
            if (it == m_chunkMap.end())
                continue;

            // Now it->second is guaranteed to exist, and no one else is iterating the map
            UnloadChunkFromGPU(it->second, gameRegistry);
        }
    }
    void TextureStreamingSystem::DebugDrawChunkOutlines(entt::registry& gameRegistry)
    {
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

    void TextureStreamingSystem::AddDeserializedTile(const DeserializedTile& tile)
    {
		m_tiles.push_back(tile);
    }


    void TextureStreamingSystem::BakeTilesIntoChunks()
    {
        constexpr int TILE_SIZE = 1; // Size in world units (not pixels), adjust accordingly

        for (const auto& tile : m_tiles)
        {
           

            // Convert grid position to world space
            glm::vec3 worldPos = {
                tile.GridPos.x * TILE_SIZE,
                tile.GridPos.y * TILE_SIZE,
                0.0f
            };

            std::vector<uint8_t> pixelData;
            int width, height;
            if (AssetManager::GetTexturePixelData(tile.TextureName, pixelData, width, height))
            {
                if (pixelData.empty())
                {
                    continue;
                }
            }
            
            UploadToChunkFromTexture(tile.WorldPos, tile.TileID, tile.TextureName, pixelData, width, height);
        }
    }

    void TextureStreamingSystem::AddChunkEntitiesToRegistry(entt::registry& registry)
    {
        for (auto& [uuid, chunk] : m_chunkMap)
        {
            
            auto entity = registry.create();
            auto& chunkRenderer = registry.emplace<ChunkRendererComponent>(entity);
            TransformComponent& transformComp = registry.emplace<TransformComponent>(entity);
            transformComp.Translation.x = chunk.WorldPosition.x;
            transformComp.Translation.y = chunk.WorldPosition.y;

            IDComponent id;
            id.ID = HashCoords(chunk.ChunkCoords);

            IDComponent& idComp = registry.emplace<IDComponent>(entity);
            idComp = id;

            chunkRenderer.Texture = chunk.GPUTexture;
            chunkRenderer.ChunkCoords = chunk.ChunkCoords;
            chunkRenderer.ChunkSize = CHUNK_SIZE;
			chunkRenderer.IsLoaded = false;

        }
    }







}
